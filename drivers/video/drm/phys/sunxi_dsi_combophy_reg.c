/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2023 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <common.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <compiler.h>
#include <linux/math64.h>
#include <linux/compat.h>
#include <asm/arch/clock.h>
#include <phy-mipi-dphy.h>
#include "sunxi_dsi_combophy_reg.h"
#define SUPPORT_COMBO_DPHY
#if IS_ENABLED(CONFIG_MACH_SUN60IW2) || IS_ENABLED(CONFIG_MACH_SUN65IW1) \
	|| IS_ENABLED(CONFIG_MACH_SUN55IW3)
#define DISPLL_LINEAR_FREQ
#endif
u32 sunxi_dsi_lane_den[4] = { 0x1, 0x3, 0x7, 0xf };

static s32 sunxi_dsi_lane_set(struct sunxi_dphy_lcd *dphy, u32 lanes)
{
#ifdef SUPPORT_COMBO_DPHY
	u32 lane_den = 0;
	u32 i = 0;

	for (i = 0; i < lanes; i++)
		lane_den |= (1 << i);

	dphy->reg->dphy_ana3.bits.envttd = lane_den;
	dphy->reg->dphy_ana2.bits.enp2s_cpu = lane_den;
#else
	dphy->reg->dphy_ana0.bits.reg_dmpd = sunxi_dsi_lane_den[lanes - 1];
	dphy->reg->dphy_ana0.bits.reg_den = sunxi_dsi_lane_den[lanes - 1];
	dphy->reg->dphy_ana4.bits.reg_dmplvd = sunxi_dsi_lane_den[lanes - 1];
	dphy->reg->dphy_ana3.bits.envttd = sunxi_dsi_lane_den[lanes - 1];
	dphy->reg->dphy_ana2.bits.enp2s_cpu = sunxi_dsi_lane_den[lanes - 1];

#endif
	dphy->reg->dphy_gctl.bits.lane_num = lanes - 1;
	return 0;
}

static s32 sunxi_dsi_dphy_cfg(struct sunxi_dphy_lcd *dphy)
{
	struct __disp_dsi_dphy_timing_t *dphy_timing_p;
	struct __disp_dsi_dphy_timing_t dphy_timing_cfg1 = {
		/* lp_clk_div(100ns) hs_prepare(70ns) hs_trail(100ns) */
		14,
		6,
		4,
		/* clk_prepare(70ns) clk_zero(300ns) clk_pre */
		7,
		50,
		3,
		/*
		 * clk_post: 400*6.734 for nop inst  2.5us
		 * clk_trail: 200ns
		 * hs_dly_mode
		 */
		10,
		30,
		0,
		/* hs_dly lptx_ulps_exit hstx_ana1 hstx_ana0 */
		10,
		3,
		3,
		3,
	};

	dphy->reg->dphy_gctl.bits.module_en = 0;
//	dphy->reg->dphy_gctl.bits.lane_num = panel->lcd_dsi_lane - 1;

	dphy->reg->dphy_tx_ctl.bits.hstx_clk_cont = 1;

	dphy_timing_p = &dphy_timing_cfg1;

/*
	dphy->reg->dphy_tx_time0.bits.lpx_tm_set =
	    dphy_timing_p->lp_clk_div;
	dphy->reg->dphy_tx_time0.bits.hs_pre_set =
	    dphy_timing_p->hs_prepare;
	dphy->reg->dphy_tx_time0.bits.hs_trail_set =
	    dphy_timing_p->hs_trail;
*/
	dphy->reg->dphy_tx_time1.bits.ck_prep_set =
	    dphy_timing_p->clk_prepare;
	dphy->reg->dphy_tx_time1.bits.ck_zero_set = dphy_timing_p->clk_zero;
	dphy->reg->dphy_tx_time1.bits.ck_pre_set = dphy_timing_p->clk_pre;
	dphy->reg->dphy_tx_time1.bits.ck_post_set = dphy_timing_p->clk_post;
	dphy->reg->dphy_tx_time2.bits.ck_trail_set =
	    dphy_timing_p->clk_trail;
	dphy->reg->dphy_tx_time2.bits.hs_dly_mode = 0;
	dphy->reg->dphy_tx_time2.bits.hs_dly_set = 0;
	dphy->reg->dphy_tx_time3.bits.lptx_ulps_exit_set = 0;
	dphy->reg->dphy_tx_time4.bits.hstx_ana0_set = 3;
	dphy->reg->dphy_tx_time4.bits.hstx_ana1_set = 3;
	dphy->reg->dphy_gctl.bits.module_en = 1;

	/* dphy->reg->dphy_ana1.bits.reg_vttmode =	0;
	dphy->reg->dphy_ana0.bits.reg_selsck = 0;
	dphy->reg->dphy_ana0.bits.reg_pws =	1;
	dphy->reg->dphy_ana0.bits.reg_sfb =	0;
	dphy->reg->dphy_ana0.bits.reg_dmpc = 1;
	dphy->reg->dphy_ana0.bits.reg_dmpd =
	    sunxi_dsi_lane_den[panel->lcd_dsi_lane - 1];
	dphy->reg->dphy_ana0.bits.reg_slv =	0x7;
	dphy->reg->dphy_ana0.bits.reg_den =
	    sunxi_dsi_lane_den[panel->lcd_dsi_lane - 1];
	dphy->reg->dphy_ana1.bits.reg_svtt = 7;
	dphy->reg->dphy_ana4.bits.reg_ckdv = 0x1;
	dphy->reg->dphy_ana4.bits.reg_tmsc = 0x1;
	dphy->reg->dphy_ana4.bits.reg_tmsd = 0x1;
	dphy->reg->dphy_ana4.bits.reg_txdnsc = 0x1;
	dphy->reg->dphy_ana4.bits.reg_txdnsd = 0x1;
	dphy->reg->dphy_ana4.bits.reg_txpusc = 0x1;
	dphy->reg->dphy_ana4.bits.reg_txpusd = 0x1;
	dphy->reg->dphy_ana4.bits.reg_dmplvc = 0x1;
	dphy->reg->dphy_ana4.bits.reg_dmplvd =
	    sunxi_dsi_lane_den[panel->lcd_dsi_lane - 1];

	dphy->reg->dphy_ana2.bits.enib = 1;
	udelay(5);
	dphy->reg->dphy_ana3.bits.enldor = 1;
	dphy->reg->dphy_ana3.bits.enldoc = 1;
	dphy->reg->dphy_ana3.bits.enldod = 1;
	udelay(1);
	dphy->reg->dphy_ana3.bits.envttc = 1;
	dphy->reg->dphy_ana3.bits.envttd =
	    sunxi_dsi_lane_den[panel->lcd_dsi_lane - 1];
	udelay(1);
	dphy->reg->dphy_ana3.bits.endiv	= 1;
	udelay(1);
	dphy->reg->dphy_ana2.bits.enck_cpu = 1;
	udelay(1);

	dphy->reg->dphy_ana1.bits.reg_vttmode =	1;
	dphy->reg->dphy_ana2.bits.enp2s_cpu	=
	    sunxi_dsi_lane_den[panel->lcd_dsi_lane - 1];

	dphy->reg->dphy_dbg1.bits.lptx_set_ck =	0x3;
	dphy->reg->dphy_dbg1.bits.lptx_set_d0 =	0x3;
	dphy->reg->dphy_dbg1.bits.lptx_set_d1 =	0x3;
	dphy->reg->dphy_dbg1.bits.lptx_set_d2 =	0x3;
	dphy->reg->dphy_dbg1.bits.lptx_set_d3 =	0x3;
	dphy->reg->dphy_dbg1.bits.lptx_dbg_en =	1; */
	return 0;
}

#ifdef SUPPORT_COMBO_DPHY

static void displl_clk_enable(struct sunxi_dphy_lcd *dphy)
{
	dphy->reg->dphy_pll_reg0.bits.ldo_en = 1;
	dphy->reg->dphy_pll_reg1.bits.hs_gating = 1;
	dphy->reg->dphy_pll_reg1.bits.ls_gating = 1;
	dphy->reg->dphy_pll_reg0.bits.pll_en = 1;
	dphy->reg->dphy_pll_reg0.bits.en_lvs = 1;
	dphy->reg->dphy_pll_reg1.bits.lockdet_en = 1;
	dphy->reg->dphy_pll_reg0.bits.reg_update = 1;
	udelay(20);
}

/* description: dsi comb phy pll configuration
 * @param sel:dsi channel select
 * @param hs_clk_rate: pixel clk * bpp
 * @param mode: dsi operation mode,0:video mode; 1:command mode;
 * @param format:dis pixel format
 * @param lane: dis output lane number
 * return ret: clk
 */
static u32 sunxi_dsi_comb_dphy_pll_set(struct sunxi_dphy_lcd *dphy, u64 hs_clk_rate,
					u64 lp_clk_rate, enum phy_mode mode)
{
	u64 frq = hs_clk_rate;
	u32 vco = 1260000000, dcxo = 24000000, m;
	u32 n, div_p, div_m0 = 0, div_m1 = 0, div_m2 = 0, div_m3 = 0;

#if IS_ENABLED(CONFIG_MACH_SUN60IW2)
	dcxo = get_hosc();
#endif
	if (!hs_clk_rate)
		return 0;
	for (m = 1; frq < vco;) {
		m++;
		frq = hs_clk_rate * m;
	}
	n = DIV_ROUND_CLOSEST(frq, dcxo);
	div_p = 1;
	div_m0 = 1;
	div_m1 = m;
	div_m2 = 1;
	if (mode == PHY_MODE_MIPI_DPHY) {
		if (!((hs_clk_rate / lp_clk_rate) % 3))
			div_m2 = 3;
		else if (!((hs_clk_rate / lp_clk_rate) % 2))
			div_m2 = 2;
		div_m3 = m * (hs_clk_rate / lp_clk_rate) / div_m2;
	} else
		div_m3 = m / div_m2;

#ifdef DISPLL_LINEAR_FREQ
	displl_clk_enable(dphy);
#endif

	/* clk_hs:24MHz*n/(p+1)/(m0+1)/(m1+1); */
	/* clk_ls:24MHz*n/(p+1)/(m2+1)/(m3+1) */
	dphy->reg->dphy_pll_reg0.bits.p = div_p ? div_p - 1 : 0;
	dphy->reg->dphy_pll_reg0.bits.m0 = div_m0 ? div_m0 - 1 : 0;
	dphy->reg->dphy_pll_reg0.bits.m1 = div_m1 ? div_m1 - 1 : 0;
	dphy->reg->dphy_pll_reg0.bits.m2 = div_m2 ? div_m2 - 1 : 0;
	dphy->reg->dphy_pll_reg0.bits.m3 = div_m3 ? div_m3 - 1 : 0;
	udelay(20);
	dphy->reg->dphy_pll_reg0.bits.n = n;
	udelay(20);
	dphy->reg->dphy_pll_reg0.bits.reg_update = 1;

	dphy->reg->dphy_pll_reg0.bits.reg_update = 1;
#ifndef DISPLL_LINEAR_FREQ
	displl_clk_enable(dphy);
#endif

	return 0;
}

u32 phy_displl_ssc(struct sunxi_dphy_lcd *dphy, u32 precent, u32 dcxo_rate)
{
	u32 n, m0, m1, p;
	u32 bot, top, cent;
	u32 wb, ws;
	u32 ssc_cycle, ssc_frq, sdm_clk;
	u32 mode = 0;

	n = dphy->reg->dphy_pll_reg0.bits.n + 1;
	m0 = dphy->reg->dphy_pll_reg0.bits.m0 + 1;
	m1 = dphy->reg->dphy_pll_reg0.bits.m1 + 1;
	p = dphy->reg->dphy_pll_reg0.bits.p;

	sdm_clk = dcxo_rate / (p + 1);

	wb = dphy->reg->dphy_pll_pat0.bits.sdm_bot;
	ws = dphy->reg->dphy_pll_pat0.bits.sdm_step;
	switch (mode) {
	case 0:
		ssc_frq = 31500;
		break;
	case 1:
		ssc_frq = 32000;
		break;
	case 2:
		ssc_frq = 32500;
		break;
	case 3:
		ssc_frq = 33000;
		break;
	default:
		return -1;
	}

	bot = 10 * sdm_clk / (1 << 17) * wb / 10 + sdm_clk * n;
	top = sdm_clk * (dcxo_rate / 1000000) / (p + 1) / (1 << 17) * 1000000 *
		ws / ssc_frq / 2 + bot;
	cent = bot / 2 + top / 2;
	ssc_cycle = (sdm_clk + ssc_frq) / ssc_frq / 2;

	while ((cent / 1000 * precent) > sdm_clk) {
		if (m1 <= 1) {
			printk("ssc out of rang!\n");
			return -1;
		} else {
			m1 = m1 / 2;
			n = n / 2;
			bot = 10 * sdm_clk / (1 << 17) * wb / 10 + sdm_clk * n;
			top = sdm_clk * (dcxo_rate / 1000000) / (p + 1) / (1 << 17) * 1000000 *
				ws / ssc_frq / 2 + bot;
			cent = bot / 2 + top / 2;
		}
	}

	dphy->reg->dphy_pll_reg0.bits.n = n - 1;
	dphy->reg->dphy_pll_reg0.bits.m1 = m1 - 1;

	//new config
	bot = cent - cent / 1000 * precent / 2;
	top = cent + cent / 1000 * precent / 2;
	if (bot < sdm_clk * n) {
		top += (sdm_clk * n - bot);
		bot = sdm_clk*n;
	}
	if (top > sdm_clk * (n + 1)) {
		bot -= (top - sdm_clk * (n + 1) + 10);
		top = sdm_clk * (n + 1) - 10;
	}
	cent = (bot + top) / 2;

	wb = (bot- sdm_clk * n) / 10000 * (1 << 17) / (dcxo_rate / 10000) / (p + 1);
	ws = (top - bot) / 10000 * (1 << 17) / (dcxo_rate / 10000) / (p + 1) *
		2 / 1000 * ssc_frq / (dcxo_rate / 1000) / (p + 1);

	dphy->reg->dphy_pll_pat0.bits.sdm_bot = wb;
	dphy->reg->dphy_pll_pat0.bits.sdm_step = ws;
	dphy->reg->dphy_pll_pat0.bits.sdm_mode = 2;

	dphy->reg->dphy_pll_pat1.bits.sdm_cycle = ssc_cycle;
	dphy->reg->dphy_pll_pat1.bits.sdm_dir = 0;
	dphy->reg->dphy_pll_pat0.bits.sdm_en = 1;
	dphy->reg->dphy_pll_reg0.bits.reg_update = 1;
	udelay(20);

	printk("frq:%dMHz ~ %dMHz, center frq:%d, ssc_frq:%d, wb:0x%x, ws:0x%x\n",
			bot / (m0 + 1), top / (m0 + 1), cent / (m0 + 1), ssc_frq, wb, ws);

	return 0;
}

static u32 sunxi_dsi_io_open(struct sunxi_dphy_lcd *dphy)
{
	dphy->reg->dphy_tx_time0.bits.lpx_tm_set =
				dphy->phy_config->dphy_tx_time0.bits.lpx_tm_set;
	dphy->reg->dphy_tx_time0.bits.hs_pre_set =
				dphy->phy_config->dphy_tx_time0.bits.hs_pre_set;
	dphy->reg->dphy_tx_time0.bits.hs_trail_set =
				dphy->phy_config->dphy_tx_time0.bits.hs_trail_set;
	dphy->reg->dphy_ana4.bits.reg_soft_rcal =
				dphy->phy_config->dphy_ana4.bits.reg_soft_rcal;
	dphy->reg->dphy_ana4.bits.en_soft_rcal =
				dphy->phy_config->dphy_ana4.bits.en_soft_rcal;
	dphy->reg->dphy_ana4.bits.on_rescal =
				dphy->phy_config->dphy_ana4.bits.on_rescal;
	dphy->reg->dphy_ana4.bits.en_rescal =
				dphy->phy_config->dphy_ana4.bits.en_rescal;
	dphy->reg->dphy_ana4.bits.reg_vlv_set =
				dphy->phy_config->dphy_ana4.bits.reg_vlv_set;
	dphy->reg->dphy_ana4.bits.reg_vlptx_set =
				dphy->phy_config->dphy_ana4.bits.reg_vlptx_set;
	dphy->reg->dphy_ana4.bits.reg_vtt_set =
				dphy->phy_config->dphy_ana4.bits.reg_vtt_set;
	dphy->reg->dphy_ana4.bits.reg_vres_set =
				dphy->phy_config->dphy_ana4.bits.reg_vres_set;
	dphy->reg->dphy_ana4.bits.reg_vref_source =
				dphy->phy_config->dphy_ana4.bits.reg_vref_source;
	dphy->reg->dphy_ana4.bits.reg_ib =
				dphy->phy_config->dphy_ana4.bits.reg_ib;
	dphy->reg->dphy_ana4.bits.reg_comtest =
				dphy->phy_config->dphy_ana4.bits.reg_comtest;
	dphy->reg->dphy_ana4.bits.en_comtest =
				dphy->phy_config->dphy_ana4.bits.en_comtest;

	dphy->reg->dphy_ana2.bits.enck_cpu = 1;

	dphy->reg->dphy_ana2.bits.enib = 1;
	dphy->reg->dphy_ana3.bits.enldor = 1;
	dphy->reg->dphy_ana3.bits.enldoc = 1;
	dphy->reg->dphy_ana3.bits.enldod = 1;
	dphy->reg->dphy_ana0.bits.reg_lptx_setr =
				dphy->phy_config->dphy_ana0.bits.reg_lptx_setr;
	dphy->reg->dphy_ana0.bits.reg_lptx_setc =
				dphy->phy_config->dphy_ana0.bits.reg_lptx_setc;
	dphy->reg->dphy_ana0.bits.reg_preemph3 =
				dphy->phy_config->dphy_ana0.bits.reg_preemph3;
	dphy->reg->dphy_ana0.bits.reg_preemph2 =
				dphy->phy_config->dphy_ana0.bits.reg_preemph2;
	dphy->reg->dphy_ana0.bits.reg_preemph1 =
				dphy->phy_config->dphy_ana0.bits.reg_preemph1;
	dphy->reg->dphy_ana0.bits.reg_preemph0 =
				dphy->phy_config->dphy_ana0.bits.reg_preemph0;
	dphy->reg->combo_phy_reg0.bits.en_cp =
				dphy->phy_config->combo_phy_reg0.bits.en_cp;
	dphy->reg->combo_phy_reg0.bits.en_comboldo =
				dphy->phy_config->combo_phy_reg0.bits.en_comboldo;
	dphy->reg->combo_phy_reg0.bits.en_mipi =
				dphy->phy_config->combo_phy_reg0.bits.en_mipi;
	dphy->reg->combo_phy_reg0.bits.en_test_0p8 =
				dphy->phy_config->combo_phy_reg0.bits.en_test_0p8;
	dphy->reg->combo_phy_reg0.bits.en_test_comboldo =
				dphy->phy_config->combo_phy_reg0.bits.en_test_comboldo;
	dphy->reg->dphy_ana4.bits.en_mipi =
				dphy->phy_config->dphy_ana4.bits.en_mipi;
	dphy->reg->combo_phy_reg2.bits.hs_stop_dly = 20;
	udelay(1);

	dphy->reg->dphy_ana3.bits.envttc = 1;
//	dphy->reg->dphy_ana3.bits.envttd = lane_den;
	dphy->reg->dphy_ana3.bits.endiv = 1;
	dphy->reg->dphy_ana2.bits.enck_cpu = 1;
	dphy->reg->dphy_ana1.bits.reg_vttmode = 1;
//	dphy->reg->dphy_ana2.bits.enp2s_cpu = lane_den;

	dphy->reg->dphy_gctl.bits.module_en = 1;
	return 0;
}

static s32 sunxi_dsi_dphy_close(struct sunxi_dphy_lcd *dphy)
{
	dphy->reg->dphy_ana2.bits.enck_cpu = 0;
	dphy->reg->dphy_ana3.bits.endiv = 0;

	dphy->reg->dphy_ana2.bits.enib = 0;
	dphy->reg->dphy_ana3.bits.enldor = 0;
	dphy->reg->dphy_ana3.bits.enldoc = 0;
	dphy->reg->dphy_ana3.bits.enldod = 0;
	dphy->reg->dphy_ana3.bits.envttc = 0;
	dphy->reg->dphy_ana3.bits.envttd = 0;
	return 0;
}

static s32 lvds_combphy_close(struct sunxi_dphy_lcd *dphy)
{
	dphy->reg->combo_phy_reg1.dwval = 0x0;
	dphy->reg->combo_phy_reg0.dwval = 0x0;
	dphy->reg->dphy_ana4.dwval = 0x0;
	dphy->reg->dphy_ana3.dwval = 0x0;
	dphy->reg->dphy_ana1.dwval = 0x0;

	return 0;
}

static s32 lvds_combphy_open(struct sunxi_dphy_lcd *dphy)
{

	dphy->reg->combo_phy_reg1.dwval =
				dphy->phy_config->combo_phy_reg1.dwval;
	dphy->reg->combo_phy_reg0.dwval = 0x1;
	udelay(5);
	dphy->reg->combo_phy_reg0.dwval = 0x5;
	udelay(5);
	dphy->reg->combo_phy_reg0.dwval = 0x7;

	dphy->reg->dphy_ana4.dwval = 0x84000000;
	dphy->reg->dphy_ana3.dwval = 0x01040000;
	dphy->reg->dphy_ana2.dwval =
	    dphy->reg->dphy_ana2.dwval & (0x0 << 1);
	dphy->reg->dphy_ana1.dwval = 0x0;

	return 0;
}
#endif

static u32 sunxi_dsi_io_close(struct sunxi_dphy_lcd *dphy)
{
	dphy->reg->dphy_tx_time0.bits.lpx_tm_set = 0;
	dphy->reg->dphy_tx_time0.bits.hs_pre_set = 0;
	dphy->reg->dphy_tx_time0.bits.hs_trail_set = 0;
	udelay(1);
	dphy->reg->dphy_ana2.bits.enp2s_cpu = 0;
	dphy->reg->dphy_ana1.bits.reg_vttmode = 0;
	udelay(1);
	dphy->reg->dphy_ana2.bits.enck_cpu = 0;
	udelay(1);
	dphy->reg->dphy_ana3.bits.endiv = 0;
	udelay(1);
	dphy->reg->dphy_ana3.bits.envttd = 0;
	dphy->reg->dphy_ana3.bits.envttc = 0;
	udelay(1);
	dphy->reg->dphy_ana3.bits.enldod = 0;
	dphy->reg->dphy_ana3.bits.enldoc = 0;
	dphy->reg->dphy_ana3.bits.enldor = 0;
	udelay(5);
	dphy->reg->dphy_ana2.bits.enib = 0;
	dphy->reg->dphy_ana4.bits.reg_soft_rcal = 0;
	dphy->reg->dphy_ana4.bits.en_soft_rcal = 0;
	dphy->reg->dphy_ana4.bits.on_rescal = 0;
	dphy->reg->dphy_ana4.bits.on_rescal = 0;
	dphy->reg->dphy_ana4.bits.on_rescal = 0;
	dphy->reg->dphy_ana4.bits.reg_vtt_set = 0;
	dphy->reg->dphy_ana4.bits.reg_vref_source = 0;
	dphy->reg->dphy_ana4.bits.reg_ib = 0;
	dphy->reg->dphy_ana4.bits.reg_comtest = 0;
	dphy->reg->dphy_ana4.bits.en_comtest = 0;
	dphy->reg->dphy_ana4.bits.en_mipi = 0;
	dphy->reg->dphy_ana1.bits.reg_svtt = 0;
	dphy->reg->dphy_ana0.bits.reg_lptx_setr = 0;
	dphy->reg->dphy_ana0.bits.reg_lptx_setc = 0;
	dphy->reg->dphy_ana1.bits.reg_csmps = 0;
	dphy->reg->dphy_ana0.bits.reg_preemph3 = 0;
	dphy->reg->dphy_ana0.bits.reg_preemph2 = 0;
	dphy->reg->dphy_ana0.bits.reg_preemph1 = 0;
	dphy->reg->dphy_ana0.bits.reg_preemph0 = 0;
	dphy->reg->dphy_ana1.bits.reg_vttmode = 0;
#ifdef SUPPORT_COMBO_DPHY
	sunxi_dsi_dphy_close(dphy);
#endif

	return 0;
}


int sunxi_dsi_combo_phy_set_reg_base(struct sunxi_dphy_lcd *dphy, uintptr_t base)
{
	dphy->reg = (struct dphy_lcd_reg *) base;
	return 0;
}

int sunxi_dsi_combophy_set_dsi_mode(struct sunxi_dphy_lcd *dphy, int mode)
{
	if (mode)
		sunxi_dsi_io_open(dphy);
	else
		sunxi_dsi_io_close(dphy);
	return 0;
}

int sunxi_dsi_combophy_configure_dsi(struct sunxi_dphy_lcd *dphy, enum phy_mode mode, struct phy_configure_opts_mipi_dphy *config)
{
	if (mode == PHY_MODE_MIPI_DPHY) {
		sunxi_dsi_dphy_cfg(dphy);
		sunxi_dsi_lane_set(dphy, config->lanes);
	}
	sunxi_dsi_comb_dphy_pll_set(dphy, config->hs_clk_rate, config->lp_clk_rate, mode);

	return 0;
}
int sunxi_dsi_combophy_print(struct sunxi_dphy_lcd *dphy)
{
	int data_mode[4] = {0}, clk_mode = 0;
	u32 dcxo = 24000000;
	u32 n, p, m2 = 0, m3 = 0;

#if IS_ENABLED(CONFIG_MACH_SUN60IW2)
	dcxo = get_hosc();
#endif

	p = dphy->reg->dphy_pll_reg0.bits.p;
	m2 = dphy->reg->dphy_pll_reg0.bits.m2;
	m3 = dphy->reg->dphy_pll_reg0.bits.m3;
	n = dphy->reg->dphy_pll_reg0.bits.n;

	data_mode[0] = dphy->reg->dphy_dbg0.bits.lptx_sta_d0;
	data_mode[1] = dphy->reg->dphy_dbg0.bits.lptx_sta_d1;
	data_mode[2] = dphy->reg->dphy_dbg0.bits.lptx_sta_d2;
	data_mode[3] = dphy->reg->dphy_dbg0.bits.lptx_sta_d3;
	clk_mode = dphy->reg->dphy_dbg0.bits.lptx_sta_clk;
	drm_printf(p, "\t clk and data lane work mode:\n");
	drm_printf(p, "\t\t  data0 | data1 | data2 | data3 | clk \n");
	drm_printf(p, "\t\t -------+-------+-------+-------+------\n");
	drm_printf(p, "\t\t   %2s   |  %2s   |  %2s   |  %2s   |  %2s   \n",
			data_mode[0] == 5 ? "HS" : "LP",
			data_mode[1] == 5 ? "HS" : "LP",
			data_mode[2] == 5 ? "HS" : "LP",
			data_mode[3] == 5 ? "HS" : "LP",
			clk_mode == 5 ? "HS" : "LP");

	return dcxo * n / (p + 1) / (m2 + 1) / (m3 + 1) / 1000;
}

int sunxi_dsi_combophy_set_lvds_mode(struct sunxi_dphy_lcd *dphy, bool enable)
{
	if (enable)
		lvds_combphy_open(dphy);
	else
		lvds_combphy_close(dphy);

	return 0;
}
