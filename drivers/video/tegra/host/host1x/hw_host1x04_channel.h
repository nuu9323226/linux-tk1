/*
 * drivers/video/tegra/host/host1x/hw_host1x04_channel.h
 *
 * Copyright (c) 2012, NVIDIA Corporation.
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
 *
 */

 /*
  * Function naming determines intended use:
  *
  *     <x>_r(void) : Returns the offset for register <x>.
  *
  *     <x>_w(void) : Returns the word offset for word (4 byte) element <x>.
  *
  *     <x>_<y>_s(void) : Returns size of field <y> of register <x> in bits.
  *
  *     <x>_<y>_f(u32 v) : Returns a value based on 'v' which has been shifted
  *         and masked to place it at field <y> of register <x>.  This value
  *         can be |'d with others to produce a full register value for
  *         register <x>.
  *
  *     <x>_<y>_m(void) : Returns a mask for field <y> of register <x>.  This
  *         value can be ~'d and then &'d to clear the value of field <y> for
  *         register <x>.
  *
  *     <x>_<y>_<z>_f(void) : Returns the constant value <z> after being shifted
  *         to place it at field <y> of register <x>.  This value can be |'d
  *         with others to produce a full register value for <x>.
  *
  *     <x>_<y>_v(u32 r) : Returns the value of field <y> from a full register
  *         <x> value 'r' after being shifted to place its LSB at bit 0.
  *         This value is suitable for direct comparison with other unshifted
  *         values appropriate for use in field <y> of register <x>.
  *
  *     <x>_<y>_<z>_v(void) : Returns the constant value for <z> defined for
  *         field <y> of register <x>.  This value is suitable for direct
  *         comparison with unshifted values appropriate for use in field <y>
  *         of register <x>.
  */

#ifndef __hw_host1x04_channel_h__
#define __hw_host1x04_channel_h__
/*This file is autogenerated.  Do not edit. */

static inline u32 host1x_channel_fifostat_r(void)
{
	return 0x0;
}
static inline u32 host1x_channel_fifostat_cfempty_s(void)
{
	return 1;
}
static inline u32 host1x_channel_fifostat_cfempty_f(u32 v)
{
	return (v & 0x1) << 11;
}
static inline u32 host1x_channel_fifostat_cfempty_m(void)
{
	return 0x1 << 11;
}
static inline u32 host1x_channel_fifostat_cfempty_v(u32 r)
{
	return (r >> 11) & 0x1;
}
static inline u32 host1x_channel_fifostat_cfempty_notempty_v(void)
{
	return 0;
}
static inline u32 host1x_channel_fifostat_cfempty_empty_v(void)
{
	return 1;
}
static inline u32 host1x_channel_fifostat_outfentries_s(void)
{
	return 5;
}
static inline u32 host1x_channel_fifostat_outfentries_f(u32 v)
{
	return (v & 0x1f) << 24;
}
static inline u32 host1x_channel_fifostat_outfentries_m(void)
{
	return 0x1f << 24;
}
static inline u32 host1x_channel_fifostat_outfentries_v(u32 r)
{
	return (r >> 24) & 0x1f;
}
static inline u32 host1x_channel_inddata_r(void)
{
	return 0xc;
}
static inline u32 host1x_channel_dmastart_r(void)
{
	return 0x14;
}
static inline u32 host1x_channel_dmaput_r(void)
{
	return 0x18;
}
static inline u32 host1x_channel_dmaget_r(void)
{
	return 0x1c;
}
static inline u32 host1x_channel_dmaend_r(void)
{
	return 0x20;
}
static inline u32 host1x_channel_dmactrl_r(void)
{
	return 0x24;
}
static inline u32 host1x_channel_dmactrl_dmastop_s(void)
{
	return 1;
}
static inline u32 host1x_channel_dmactrl_dmastop_f(u32 v)
{
	return (v & 0x1) << 0;
}
static inline u32 host1x_channel_dmactrl_dmastop_m(void)
{
	return 0x1 << 0;
}
static inline u32 host1x_channel_dmactrl_dmastop_v(u32 r)
{
	return (r >> 0) & 0x1;
}
static inline u32 host1x_channel_dmactrl_dmastop_run_v(void)
{
	return 0;
}
static inline u32 host1x_channel_dmactrl_dmastop_stop_v(void)
{
	return 1;
}
static inline u32 host1x_channel_dmactrl_dmagetrst_s(void)
{
	return 1;
}
static inline u32 host1x_channel_dmactrl_dmagetrst_f(u32 v)
{
	return (v & 0x1) << 1;
}
static inline u32 host1x_channel_dmactrl_dmagetrst_m(void)
{
	return 0x1 << 1;
}
static inline u32 host1x_channel_dmactrl_dmagetrst_v(u32 r)
{
	return (r >> 1) & 0x1;
}
static inline u32 host1x_channel_dmactrl_dmainitget_s(void)
{
	return 1;
}
static inline u32 host1x_channel_dmactrl_dmainitget_f(u32 v)
{
	return (v & 0x1) << 2;
}
static inline u32 host1x_channel_dmactrl_dmainitget_m(void)
{
	return 0x1 << 2;
}
static inline u32 host1x_channel_dmactrl_dmainitget_v(u32 r)
{
	return (r >> 2) & 0x1;
}

#endif /* __hw_host1x04_channel_h__ */