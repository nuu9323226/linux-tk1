/*
 * drivers/video/tegra/host/gk20a/clk_gk20a.c
 *
 * GK20A Clocks
 *
 * Copyright (c) 2011 - 2012, NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/clk.h>
#include <linux/delay.h>	/* for mdelay */

#include <mach/clk.h>

#include "../dev.h"

#include "gk20a.h"
#include "hw_trim_gk20a.h"

#define nvhost_dbg_clk(fmt, arg...) \
	nvhost_dbg(dbg_clk, fmt, ##arg)

#define KHz 1000
#define MHz 1000000

/* from vbios PLL info table */
static struct pll_parms gpc_pll_params = {
	403, 806,	/* freq */
	1100, 2200,	/* vco */
	25, 100,	/* u */
	1, 255,		/* M */
	8, 255,		/* N */
	1, 63,		/* PL */
};

/* Calculate and update M/N/PL as well as pll->freq
    ref_clk_f = clk_in_f / src_div = clk_in_f; (src_div = 1 on gk20a)
    u_f = ref_clk_f / M;
    PLL output = vco_f = u_f * N = ref_clk_f * N / M;
    gpc2clk = target clock frequency = vco_f / PL;
    gpcclk = gpc2clk / 2; */
static int clk_config_pll(struct clk_gk20a *clk, struct pll *pll,
	struct pll_parms *pll_params, u32 *target_freq, bool best_fit)
{
	u32 min_vco_f, max_vco_f;
	u32 best_M, best_N;
	u32 low_PL, high_PL, best_PL;
	u32 pl, m, n, n2;
	u32 target_vco_f, vco_f;
	u32 ref_clk_f, target_clk_f, u_f;
	u32 delta, lwv, best_delta = ~0;

	BUG_ON(target_freq == NULL);

	nvhost_dbg_fn("request target freq %d MHz", *target_freq);

	ref_clk_f = pll->clk_in;
	target_clk_f = *target_freq;
	max_vco_f = pll_params->max_vco;
	min_vco_f = pll_params->min_vco;
	best_M = pll_params->max_M;
	best_N = pll_params->min_N;
	best_PL = pll_params->min_PL;

	target_vco_f = target_clk_f + target_clk_f / 50;
	if (max_vco_f < target_vco_f)
		max_vco_f = target_vco_f;

	high_PL = (max_vco_f + target_vco_f - 1) / target_vco_f;
	high_PL = min(high_PL, pll_params->max_PL);
	high_PL = max(high_PL, pll_params->min_PL);

	low_PL = min_vco_f / target_vco_f;
	low_PL = min(low_PL, pll_params->max_PL);
	low_PL = max(low_PL, pll_params->min_PL);

	nvhost_dbg_info("low_PL %d, high_PL %d", low_PL, high_PL);

	for (pl = high_PL; pl >= low_PL; pl--) {
		target_vco_f = target_clk_f * pl;

		for (m = pll_params->min_M; m <= pll_params->max_M; m++) {
			u_f = ref_clk_f / m;

			if (u_f < pll_params->min_u)
				break;
			if (u_f > pll_params->max_u)
				continue;

			n = (target_vco_f * m) / ref_clk_f;
			n2 = ((target_vco_f * m) + (ref_clk_f - 1)) / ref_clk_f;

			if (n > pll_params->max_N)
				break;

			for (; n <= n2; n++) {
				if (n < pll_params->min_N)
					continue;
				if (n > pll_params->max_N)
					break;

				vco_f = ref_clk_f * n / m;

				if (vco_f >= min_vco_f && vco_f <= max_vco_f) {
					lwv = (vco_f + (pl / 2)) / pl;
					delta = abs(lwv - target_clk_f);

					if (delta < best_delta) {
						best_delta = delta;
						best_M = m;
						best_N = n;
						best_PL = pl;

						if (best_delta == 0 ||
						    /* 0.45% for non best fit */
						    (!best_fit && (vco_f / best_delta > 218))) {
							goto found_match;
						}

						nvhost_dbg_info("delta %d @ M %d, N %d, PL %d",
							delta, m, n, pl);
					}
				}
			}
		}
	}

found_match:
	BUG_ON(best_delta == ~0);

	if (best_fit && best_delta != 0)
		nvhost_dbg_clk("no best match for target @ %dMHz on gpc_pll",
			target_clk_f);

	pll->M = best_M;
	pll->N = best_N;
	pll->PL = best_PL;

	/* save current frequency */
	pll->freq = ref_clk_f * pll->N / (pll->M * pll->PL);

	*target_freq = pll->freq;

	nvhost_dbg_clk("actual target freq %d MHz, M %d, N %d, PL %d",
		*target_freq, pll->M, pll->N, pll->PL);

	nvhost_dbg_fn("done");

	return 0;
}

static int clk_program_gpc_pll(struct gk20a *g, struct clk_gk20a *clk)
{
	u32 data, cfg, coeff, timeout;

	nvhost_dbg_fn("");

	/* put PLL in bypass before programming it */
	data = gk20a_readl(g, trim_sys_sel_vco_r());
	data = set_field(data, trim_sys_sel_vco_gpc2clk_out_m(),
		trim_sys_sel_vco_gpc2clk_out_bypass_f());
	gk20a_writel(g, trim_sys_sel_vco_r(), data);

	/* disable PLL before changing coefficients */
	cfg = gk20a_readl(g, trim_sys_gpcpll_cfg_r());
	cfg = set_field(cfg, trim_sys_gpcpll_cfg_enable_m(),
			trim_sys_gpcpll_cfg_enable_no_f());
	gk20a_writel(g, trim_sys_gpcpll_cfg_r(), cfg);

	/* change coefficients */
	coeff = trim_sys_gpcpll_coeff_mdiv_f(clk->gpc_pll.M) |
		trim_sys_gpcpll_coeff_ndiv_f(clk->gpc_pll.N) |
		trim_sys_gpcpll_coeff_pldiv_f(clk->gpc_pll.PL);
	gk20a_writel(g, trim_sys_gpcpll_coeff_r(), coeff);

	/* enable PLL after changing coefficients */
	cfg = gk20a_readl(g, trim_sys_gpcpll_cfg_r());
	cfg = set_field(cfg, trim_sys_gpcpll_cfg_enable_m(),
			trim_sys_gpcpll_cfg_enable_yes_f());
	gk20a_writel(g, trim_sys_gpcpll_cfg_r(), cfg);

	/* lock pll */
	cfg = gk20a_readl(g, trim_sys_gpcpll_cfg_r());
	if (cfg & trim_sys_gpcpll_cfg_enb_lckdet_power_off_f()){
		cfg = set_field(cfg, trim_sys_gpcpll_cfg_enb_lckdet_m(),
			trim_sys_gpcpll_cfg_enb_lckdet_power_on_f());
		gk20a_writel(g, trim_sys_gpcpll_cfg_r(), cfg);
	}

	/* wait pll lock */
	timeout = clk->pll_delay / 100 + 1;
	do {
		cfg = gk20a_readl(g, trim_sys_gpcpll_cfg_r());
		if (cfg & trim_sys_gpcpll_cfg_pll_lock_true_f())
			goto pll_locked;
		udelay(100);
	} while (--timeout > 0);

	/* PLL is messed up. What can we do here? */
	BUG();
	return -EBUSY;

pll_locked:
	/* put PLL back on vco */
	data = gk20a_readl(g, trim_sys_sel_vco_r());
	data = set_field(data, trim_sys_sel_vco_gpc2clk_out_m(),
		trim_sys_sel_vco_gpc2clk_out_vco_f());
	gk20a_writel(g, trim_sys_sel_vco_r(), data);

	return 0;
}

static int gk20a_init_clk_reset_enable_hw(struct gk20a *g)
{
	nvhost_dbg_fn("");
	return 0;
}

static int gk20a_init_clk_setup_sw(struct gk20a *g)
{
	struct clk_gk20a *clk = &g->clk;

	nvhost_dbg_fn("");

	if (clk->sw_ready) {
		nvhost_dbg_fn("skip init");
		return 0;
	}

	/* TBD: set this according to different environments */
	clk->pll_delay = 5000000; /* usec */

	/* target gpc2clk = 806MHz, gpcclk = 403MHz */
	clk->gpc_pll.id = GK20A_GPC_PLL;
	clk->gpc_pll.clk_in = 26; /* MHz */
	/* settings in vbios */
	clk->gpc_pll.M = 1;
	clk->gpc_pll.N = 31;
	clk->gpc_pll.PL = 1;
	clk->gpc_pll.freq = (clk->gpc_pll.clk_in * clk->gpc_pll.N) /
		(clk->gpc_pll.M * clk->gpc_pll.PL);

	clk->tegra_clk = clk_get_sys("tegra_gk20a", "PLLG_ref");
	if (IS_ERR_OR_NULL(clk->tegra_clk)) {
		nvhost_err(dev_from_gk20a(g),
			"fail to get tegra ref clk tegra_gk20a/PLLG_ref");
		return -EINVAL;
	}

	clk->sw_ready = true;

	nvhost_dbg_fn("done");
	return 0;
}

static int gk20a_init_clk_setup_hw(struct gk20a *g)
{
	struct clk_gk20a *clk = &g->clk;
	u32 data;

	nvhost_dbg_fn("");

	data = gk20a_readl(g, trim_sys_gpc2clk_out_r());
	data = set_field(data,
			trim_sys_gpc2clk_out_sdiv14_m() |
			trim_sys_gpc2clk_out_vcodiv_m() |
			trim_sys_gpc2clk_out_bypdiv_m(),
			trim_sys_gpc2clk_out_sdiv14_indiv4_mode_f() |
			trim_sys_gpc2clk_out_vcodiv_by1_f() |
			trim_sys_gpc2clk_out_bypdiv_by1_f());
	gk20a_writel(g, trim_sys_gpc2clk_out_r(), data);

	return clk_program_gpc_pll(g, clk);
}

int gk20a_init_clk_support(struct gk20a *g)
{
	struct clk_gk20a *clk = &g->clk;
	u32 err;

	nvhost_dbg_fn("");

	clk->g = g;

	err = gk20a_init_clk_reset_enable_hw(g);
	if (err)
		return err;

	err = gk20a_init_clk_setup_sw(g);
	if (err)
		return err;

	err = gk20a_init_clk_setup_hw(g);
	if (err)
		return err;

	/* set to minimal freq */
	gk20a_clk_set_rate(g, gpc_pll_params.min_freq);

	return err;
}

u32 gk20a_clk_get_rate(struct gk20a *g)
{
	struct clk_gk20a *clk = &g->clk;
	return clk->gpc_pll.freq;
}

/* TBD: interface to change clock and dvfs in one function */
int gk20a_clk_set_rate(struct gk20a *g, u32 rate)
{
	struct clk_gk20a *clk = &g->clk;
	struct clk *tegra_clk = clk->tegra_clk;
	/* save old freq for compare and recover */
	u32 freq = clk->gpc_pll.freq;
	int err = 0;

	nvhost_dbg_fn("curr freq: %dMHz, target freq %dMHz",
		freq, rate);

	if (rate > gpc_pll_params.max_freq)
		rate = gpc_pll_params.max_freq;
	else if (rate < gpc_pll_params.min_freq)
		rate = gpc_pll_params.min_freq;

	if (rate == freq)
		return 0;

	/* gpc_pll.freq is changed to new value here */
	err = clk_config_pll(clk, &clk->gpc_pll, &gpc_pll_params,
			&rate, true);
	if (err)
		goto clean_up;

	/* raise freq, call dvfs first to raise voltage */
	if (rate > freq) {
		err = tegra_dvfs_set_rate(tegra_clk, rate * MHz);
		if (err)
			goto clean_up;
	}

	err = clk_program_gpc_pll(g, clk);
	if (err)
		goto clean_up;

	/* lower freq, call dvfs after to lower voltage */
	if (rate < freq) {
		err = tegra_dvfs_set_rate(tegra_clk, rate * MHz);
		if (err)
			goto clean_up;
	}

clean_up:
	/* Just report error but not restore PLL since dvfs could already changed
	    voltage even when it returns error. */
	if (err)
		nvhost_err(dev_from_gk20a(g),
			"failed to set rate to @ %d", rate);
	return err;
}