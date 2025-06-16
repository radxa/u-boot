/*
 * Copyright (C) 2013 Allwinnertech
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "clk-sun60iw2.h"
#include "clk-sun60iw2_tbl.c"
#include "clk_factor.h"
#include "clk_periph.h"
#include <linux/compat.h>
#include <fdt_support.h>
#include <div64.h>

#define FACTOR_SIZEOF(name) (sizeof(factor_pll##name##_tbl)/ \
		sizeof(struct sunxi_clk_factor_freq))

#define FACTOR_SEARCH(name) (sunxi_clk_com_ftr_sr( \
			&sunxi_clk_factor_pll_##name, factor, \
			factor_pll##name##_tbl, index, \
			FACTOR_SIZEOF(name)))

#ifndef CONFIG_EVB_PLATFORM
#define LOCKBIT(x) 31
#else
#define LOCKBIT(x) x
#endif

DEFINE_SPINLOCK(clk_lock);
void __iomem *sunxi_clk_base;
void __iomem *sunxi_clk_rtc_base;;
int sunxi_clk_maxreg = SUNXI_CLK_MAX_REG;
int cpus_clk_maxreg = CPUS_CLK_MAX_REG;
#define REG_ADDR(x) (sunxi_clk_base + x)

/*                                	   ns  nw  ks kw   ms  mw  ps  pw  d1s d1w d2s d2w {frac   out	mode}	en-s    sdmss   sdmsw   sdmpat		sdmval*/
SUNXI_CLK_FACTORS(pll_periph0_2x,      8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  20,  3,    0,    0,  0,      31,     0,      0,      0,               0);
SUNXI_CLK_FACTORS(pll_periph0_800m,    8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  16,  3,    0,    0,  0,      31,     0,      0,      0,               0);
SUNXI_CLK_FACTORS(pll_periph0_480m,    8,  8,  0,  0,  0,  0,  0,  0,  0,  0,   2,  3,    0,    0,  0,      31,     0,      0,      0,               0);
SUNXI_CLK_FACTORS(pll_periph1_2x,      8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  20,  3,    0,    0,  0,      31,     0,      0,      0,               0);
SUNXI_CLK_FACTORS(pll_periph1_800m,    8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  16,  3,    0,    0,  0,      31,     0,      0,      0,               0);
SUNXI_CLK_FACTORS(pll_video0x4,	       8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  20,  3,    0,    0,  0,      31,     0,      0,      0,               0);
SUNXI_CLK_FACTORS(pll_video0x3,	       8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  16,  3,    0,    0,  0,      31,     0,      0,      0,               0);
SUNXI_CLK_FACTORS(pll_video1x4,	       8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  20,  3,    0,    0,  0,      31,     0,      0,      0,               0);
SUNXI_CLK_FACTORS(pll_video1x3,	       8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  16,  3,    0,    0,  0,      31,     0,      0,      0,               0);
SUNXI_CLK_FACTORS(pll_video2x4,	       8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  20,  3,    0,    0,  0,      31,     0,      0,      0,               0);
SUNXI_CLK_FACTORS(pll_dex4,	       8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  20,  3,    0,    0,  0,      31,     0,      0,      0,               0);
SUNXI_CLK_FACTORS(pll_dex3,	       8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  16,  3,    0,    0,  0,      31,     0,      0,      0,               0);

static int get_factors_pll_periph0_2x(u32 rate, u32 parent_rate,
		struct clk_factors_value *factor)
{
	int index;
	u64 tmp_rate;

	if (!factor)
		return -1;

	tmp_rate = rate > pllperiph0_2x_max ? pllperiph0_2x_max : rate;
	do_div(tmp_rate, 1000000);
	index = tmp_rate;

	if (FACTOR_SEARCH(periph0_2x))
		return -1;

	return 0;
}

static int get_factors_pll_periph1_2x(u32 rate, u32 parent_rate,
		struct clk_factors_value *factor)
{
	int index;
	u64 tmp_rate;

	if (!factor)
		return -1;

	tmp_rate = rate > pllperiph1_2x_max ? pllperiph1_2x_max : rate;
	do_div(tmp_rate, 1000000);
	index = tmp_rate;

	if (FACTOR_SEARCH(periph1_2x))
		return -1;

	return 0;
}

static int get_factors_pll_periph1_800m(u32 rate, u32 parent_rate,
		struct clk_factors_value *factor)
{
	int index;
	u64 tmp_rate;

	if (!factor)
		return -1;

	tmp_rate = rate > pllperiph1_800m_max ? pllperiph1_800m_max : rate;
	do_div(tmp_rate, 1000000);
	index = tmp_rate;

	if (FACTOR_SEARCH(periph1_800m))
		return -1;

	return 0;
}

static unsigned long calc_rate_pll_periph(u32 parent_rate,
		struct clk_factors_value *factor)
{
	u64 tmp_rate = (parent_rate ? parent_rate : 24000000);

	factor->factorn = factor->factorn >= 0xff ? 0 : factor->factorn;
	factor->factorm = factor->factorm >= 0xff ? 0 : factor->factorm;
	factor->factork = factor->factork >= 0xff ? 0 : factor->factork;
	factor->factorp = factor->factorp >= 0xff ? 0 : factor->factorp;
	factor->factord1 = factor->factord1 >= 0xff ? 0 : factor->factord1;
	factor->factord2 = factor->factord2 >= 0xff ? 0 : factor->factord2;

	tmp_rate = tmp_rate * (factor->factorn + 1);
	do_div(tmp_rate, (factor->factord1 + 1) * (factor->factord2 + 1));

	return (unsigned long)tmp_rate;
}

/*    pll_video0x4/pll_video1x4: 24*N/D1    */
static unsigned long calc_rate_video0(u32 parent_rate,
		struct clk_factors_value *factor)
{
	u64 tmp_rate = (parent_rate ? parent_rate : 24000000);
	tmp_rate = tmp_rate * (factor->factorn + 1);
	do_div(tmp_rate, (factor->factord2 + 1));
	return (unsigned long)tmp_rate;
}

static int get_factors_pll_video(u32 rate, u32 parent_rate,
		struct clk_factors_value *factor)
{
	u8 min_n = 49, max_n = 104;
	u8 min_d2 = 0, max_d2 = 1;
	u8 best_n = 0, best_d2 = 0;
	u64 best_rate = 0;

	for (factor->factorn = min_n; factor->factorn <= max_n; factor->factorn++) {
		for (factor->factord2 = min_d2; factor->factord2 <= max_d2; factor->factord2++) {
			u64 tmp_rate;

			tmp_rate = calc_rate_video0(parent_rate, factor);

			if (tmp_rate > rate)
				continue;

			if ((rate - tmp_rate) < (rate - best_rate)) {
				best_rate = tmp_rate;
				best_n = factor->factorn;
				best_d2 = factor->factord2;
			}
		}
	}

	factor->factorn = best_n;
	factor->factord2 = best_d2;

	return 0;
}

static const char *hosc_parents[] = {"hosc"};
struct factor_init_data sunxi_factos[] = {
	/* name        parent      parent_num, flags				reg          lock_reg		lock_bit     pll_lock_ctrl_reg lock_en_bit	lock_mode           config             get_factors					calc_rate              priv_ops*/
	{"pll_periph0_2x",   hosc_parents, 1,          0,            PLL_PERIPH0, PLL_PERIPH0, LOCKBIT(28), PLL_PERIPH0, 29,         PLL_LOCK_NEW_MODE, &sunxi_clk_factor_pll_periph0_2x,   &get_factors_pll_periph0_2x,        &calc_rate_pll_periph, (struct clk_ops *)NULL},
	{"pll_periph0_800m", hosc_parents, 1,          0,            PLL_PERIPH0, PLL_PERIPH0, LOCKBIT(28), PLL_PERIPH0, 29,         PLL_LOCK_NEW_MODE, &sunxi_clk_factor_pll_periph0_800m, &get_factors_pll_periph1_800m,      &calc_rate_pll_periph, (struct clk_ops *)NULL},
	{"pll_periph0_480m", hosc_parents, 1,          0,            PLL_PERIPH0, PLL_PERIPH0, LOCKBIT(28), PLL_PERIPH0, 29,         PLL_LOCK_NEW_MODE, &sunxi_clk_factor_pll_periph0_480m, &get_factors_pll_periph1_800m,      &calc_rate_pll_periph, (struct clk_ops *)NULL},
	{"pll_periph1_2x",   hosc_parents, 1,          0,            PLL_PERIPH1, PLL_PERIPH1, LOCKBIT(28), PLL_PERIPH1, 29,         PLL_LOCK_NEW_MODE, &sunxi_clk_factor_pll_periph1_2x,   &get_factors_pll_periph1_2x,        &calc_rate_pll_periph, (struct clk_ops *)NULL},
	{"pll_periph1_800m", hosc_parents, 1,          0,            PLL_PERIPH1, PLL_PERIPH1, LOCKBIT(28), PLL_PERIPH1, 29,         PLL_LOCK_NEW_MODE, &sunxi_clk_factor_pll_periph1_800m, &get_factors_pll_periph1_800m,      &calc_rate_pll_periph, (struct clk_ops *)NULL},
	{"pll_video0x4",     hosc_parents, 1,	CLK_NO_DISABLE,      PLL_VIDEO0,  PLL_VIDEO0,  LOCKBIT(28), PLL_VIDEO0,  29,         PLL_LOCK_NEW_MODE, &sunxi_clk_factor_pll_video0x4,     &get_factors_pll_video,             &calc_rate_video0,     (struct clk_ops *)NULL},
	{"pll_video0x3",     hosc_parents, 1,	CLK_NO_DISABLE,      PLL_VIDEO0,  PLL_VIDEO0,  LOCKBIT(28), PLL_VIDEO0,  29,         PLL_LOCK_NEW_MODE, &sunxi_clk_factor_pll_video0x3,     &get_factors_pll_video,             &calc_rate_video0,     (struct clk_ops *)NULL},
	{"pll_video1x4",     hosc_parents, 1,	CLK_NO_DISABLE,      PLL_VIDEO1,  PLL_VIDEO1,  LOCKBIT(28), PLL_VIDEO1,  29,         PLL_LOCK_NEW_MODE, &sunxi_clk_factor_pll_video1x4,     &get_factors_pll_video,             &calc_rate_video0,     (struct clk_ops *)NULL},
	{"pll_video1x3",     hosc_parents, 1,	CLK_NO_DISABLE,      PLL_VIDEO1,  PLL_VIDEO1,  LOCKBIT(28), PLL_VIDEO0,  29,         PLL_LOCK_NEW_MODE, &sunxi_clk_factor_pll_video1x3,     &get_factors_pll_video,             &calc_rate_video0,     (struct clk_ops *)NULL},
	{"pll_video2x4",     hosc_parents, 1,	CLK_NO_DISABLE,      PLL_VIDEO2,  PLL_VIDEO2,  LOCKBIT(28), PLL_VIDEO2,  29,         PLL_LOCK_NEW_MODE, &sunxi_clk_factor_pll_video2x4,     &get_factors_pll_video,             &calc_rate_video0,     (struct clk_ops *)NULL},
	{"pll_dex4",         hosc_parents, 1,	CLK_NO_DISABLE,      PLL_DE,  	  PLL_DE,      LOCKBIT(28), PLL_DE,      29,         PLL_LOCK_NEW_MODE, &sunxi_clk_factor_pll_dex4,  	    &get_factors_pll_video,             &calc_rate_video0,     (struct clk_ops *)NULL},
	{"pll_dex3",         hosc_parents, 1,	CLK_NO_DISABLE,      PLL_DE,  	  PLL_DE,      LOCKBIT(28), PLL_DE,      29,         PLL_LOCK_NEW_MODE, &sunxi_clk_factor_pll_dex3,  	    &get_factors_pll_video,             &calc_rate_video0,     (struct clk_ops *)NULL},
};

static const char *de_parents[] = {"pll_dex3", "pll_dex4", "pll_periph0_480m", "pll_periph0_400m", "pll_periph0_300m", "pll_video0x4", "pll_video2x4"};
static const char *smhc0_parents[] = {"sys24M", "pll_periph0_400m", "pll_periph0_300m", "pll_periph1_400m", "pll_periph1_300m"};
static const char *smhc1_parents[] = {"sys24M", "pll_periph0_400m", "pll_periph0_300m", "pll_periph1_400m", "pll_periph1_300m"};
static const char *smhc2_parents[] = {"sys24M", "pll_periph0_800m", "pll_periph0_600m", "pll_periph1_800m", "pll_periph1_600m"};
static const char *smhc3_parents[] = {"sys24M", "pll_periph0_800m", "pll_periph0_600m", "pll_periph1_800m", "pll_periph1_600m"};
static const char *dsi0_parents[] = {"sys24M", "pll-periph0_200m", "pll_periph0_150m"};
static const char *dsi1_parents[] = {"sys24M", "pll-periph0_200m", "pll_periph0_150m"};
static const char *combphy0_parents[] = {"pll_video0x4", "pll_video1x4", "pll_video2x4", "pll_periph0_2x", "pll_video0x3"};
static const char *combphy1_parents[] = {"pll_video0x4", "pll_video1x4", "pll_video2x4", "pll_periph0_2x", "pll_video0x3"};
static const char *tcon_lcd0_parents[] = {"pll_video0x4", "pll_video1x4", "pll_video2x4", "pll_periph0_2x"};
static const char *tcon_lcd1_parents[] = {"pll_video0x4", "pll_video1x4", "pll_video2x4", "pll_periph0_2x"};
static const char *tcon_lcd2_parents[] = {"pll_video0x4", "pll_video1x4", "pll_video2x4", "pll_periph0_2x"};
static const char *edp_parents[] = {"pll_video0x4", "pll_video1x4", "pll_video2x4", "pll_periph0_2x"};
static const char *hdmi_tv_parents[] = {"pll_video0x4", "pll_video1x4", "pll_video2x4"};
static const char *hdmi_sfr_parents[] = {"sys24M", "hosc"};
static const char *eink_parents[] = {"pll_periph0_480m", "pll_periph0_400m"};
static const char *eink_panel_parents[] = {"pll_video0x4", "pll_video0x3", "pll_video1x4", "pll_video1x3", "pll_periph0_300m"};
static const char *serdes_phy_cfg_parents[] = {"sys24M", "pll_periph0_600m"};
static const char *gmac_phy_parents[] = {"pll_periph0_150m"};
static const char *gmac_ptp_parents[] = {"sys24M", "pll_periph0_200m", "hosc"};
/*
   SUNXI_CLK_PERIPH(name,        mux_reg,         mux_sft, mux_wid,      div_reg,      div_msft,  div_mwid,      div_nsft,   div_nwid,   gate_flag,    en_reg,       rst_reg,     bus_gate_reg,   drm_gate_reg,     en_sft,     rst_sft,     bus_gate_sft,    dram_gate_sft,   lock,  com_gate,   com_gate_off)
 */
SUNXI_CLK_PERIPH(de0,                   DE0_CFG,         24,      3,         DE0_CFG,         0,         5,          0,          0,          0,       DE0_CFG,       DE_GATE,        DE_GATE,           0,             31,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(de_rst_sys,               0,             0,      0,             0,           0,         0,          0,          0,          0,          0,          DE_SYS_GATE,        0,             0,              0,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(sdmmc0_mod,            SMHC0_CFG,       24,      3,         SMHC0_CFG,       0,         4,          8,          4,          0,       SMHC0_CFG,     SMHC0_GATE,     SMHC0_GATE,        0,             31,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(sdmmc1_mod,            SMHC1_CFG,       24,      3,         SMHC1_CFG,       0,         4,          8,          4,          0,       SMHC1_CFG,     SMHC1_GATE,     SMHC1_GATE,        0,             31,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(sdmmc2_mod,            SMHC2_CFG,       24,      3,         SMHC2_CFG,       0,         4,          8,          4,          0,       SMHC2_CFG,     SMHC2_GATE,     SMHC2_GATE,        0,             31,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(sdmmc3_mod,            SMHC3_CFG,       24,      3,         SMHC3_CFG,       0,         4,          8,          4,          0,       SMHC3_CFG,     SMHC3_GATE,     SMHC3_GATE,        0,             31,         18,         2,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(dpss_top0,                0,             0,      0,            0,            0,         0,          0,          0,          0,          0,          DPSS_TOP0_GATE, DPSS_TOP0_GATE,    0,              0,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(dpss_top1,                0,             0,      0,            0,            0,         0,          0,          0,          0,          0,          DPSS_TOP1_GATE, DPSS_TOP1_GATE,    0,              0,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(video_out0_rst,           0,             0,      0,            0,            0,         0,          0,          0,          0,          0,          VIDEO_OUT0_GATE,     0,            0,              0,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(video_out1_rst,           0,             0,      0,            0,            0,         0,          0,          0,          0,          0,          VIDEO_OUT1_GATE,     0,            0,              0,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(mipi_dsi0,             DSI_CFG0,        24,      3,         DSI_CFG0,        0,         5,          0,          0,          0,       DSI_CFG0,      DSI_GATE0,       DSI_GATE0,        0,             31,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(mipi_dsi1,             DSI_CFG1,        24,      3,         DSI_CFG1,        0,         5,          0,          0,          0,       DSI_CFG1,      DSI_GATE1,       DSI_GATE1,        0,             31,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(mipi_dsc,                 0,             0,      0,            0,            0,         0,          0,          0,          0,          0,          DSC_GATE,        DSC_GATE,         0,              0,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(tcon_lcd0,             TCON_LCD_CFG0,   24,      3,         TCON_LCD_CFG0,   0,         5,          0,          0,          0,       TCON_LCD_CFG0, TCON_LCD_GATE0,  TCON_LCD_GATE0,   0,             31,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(tcon_lcd1,             TCON_LCD_CFG1,   24,      3,         TCON_LCD_CFG1,   0,         5,          0,          0,          0,       TCON_LCD_CFG1, TCON_LCD_GATE1,  TCON_LCD_GATE1,   0,             31,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(tcon_lcd2,             TCON_LCD_CFG2,   24,      3,         TCON_LCD_CFG2,   0,         5,          0,          0,          0,       TCON_LCD_CFG2, TCON_LCD_GATE2,  TCON_LCD_GATE2,   0,             31,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(tcon_tv0,                 0,             0,      0,            0,            0,         0,          0,          0,          0,          0,          TCONTV_GATE0,    TCONTV_GATE0,     0,              0,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(tcon_tv1,                 0,             0,      0,            0,            0,         0,          0,          0,          0,          0,          TCONTV_GATE1,    TCONTV_GATE1,     0,              0,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(mipi_dsi_combphy0,     COMBPHY1_CFG,    24,      3,         COMBPHY0_CFG,    0,         5,          0,          0,          0,       COMBPHY0_CFG,     0,                 0,           0,             31,          0,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(mipi_dsi_combphy1,     COMBPHY1_CFG,    24,      3,         COMBPHY1_CFG,    0,         5,          0,          0,          0,       COMBPHY1_CFG,     0,                 0,           0,             31,          0,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(edp,                   EDP_CFG,         24,      3,         EDP_CFG,         0,         5,          8,          5,          0,       EDP_CFG,       EDP_GATE,        EDP_GATE,         0,             31,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(hdmi_tv,               HDMI_TV_CFG,     24,      3,         HDMI_TV_CFG,     0,         5,          8,          5,          0,       HDMI_TV_CFG,      0,                 0,           0,             31,          0,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(hdmi_gate,                0,             0,      0,            0,            0,         0,          0,          0,          0,          0,             0,            HDMI_GATE,        0,              0,          0,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(hdmi_sfr,              HDMI_SFR_CFG,    24,      3,            0,            0,         0,          0,          0,          0,       HDMI_SFR_CFG,     0,                 0,           0,             31,          0,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(hdmi_hdcp_rst,            0,             0,      0,            0,            0,         0,          0,          0,          0,          0,          HDMI_GATE,            0,           0,              0,         18,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(hdmi_sub_rst,             0,             0,      0,            0,            0,         0,          0,          0,          0,          0,          HDMI_GATE,            0,           0,              0,         17,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(hdmi_main_rst,            0,             0,      0,            0,            0,         0,          0,          0,          0,          0,          HDMI_GATE,            0,           0,              0,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(lvds0,                    0,             0,      0,            0,            0,         0,          0,          0,          0,          0,          LVDS_GATE0,           0,           0,              0,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(lvds1,                    0,             0,      0,            0,            0,         0,          0,          0,          0,          0,          LVDS_GATE1,           0,           0,              0,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(eink,                  EINK_CFG,        24,      3,         EINK_CFG,        0,         5,          0,          0,          0,       EINK_CFG,      EINK_GATE,       EINK_GATE,        0,             31,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(eink_panel,            EINK_PANEL_CFG,  24,      3,         EINK_PANEL_CFG,  0,         5,          0,          0,          0,       EINK_PANEL_CFG,   0,                 0,           0,             31,          0,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(serdes_phy_cfg,        SERDES_PHY_CFG,  24,      3,         SERDES_PHY_CFG,  0,         5,          0,          0,          0,       SERDES_PHY_CFG, SERDES_GATE,         0,           0,             31,         16,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(gmac0,                 0,               0,       0,         0,               0,         0,          0,          0,          0,       0,              GMAC0_BUS_GATE_RST,  GMAC0_BUS_GATE_RST,         0,          0,          16,            0,         0,  &clk_lock, NULL, 0);
SUNXI_CLK_PERIPH(gmac0_axi,             0,               0,       0,         0,               0,         0,          0,          0,          0,       0,              GMAC0_BUS_GATE_RST,  GMAC0_BUS_GATE_RST,         0,          0,          17,            0,         0,  &clk_lock, NULL, 0);
SUNXI_CLK_PERIPH(gmac1,                 0,               0,       0,         0,               0,         0,          0,          0,          0,       0,              GMAC1_BUS_GATE_RST,  GMAC1_BUS_GATE_RST,           0,        0,          16,            0,         0,  &clk_lock, NULL, 0);
SUNXI_CLK_PERIPH(gmac1_axi,             0,               0,       0,         0,               0,         0,          0,          0,          0,       0,              GMAC1_BUS_GATE_RST,  GMAC1_BUS_GATE_RST,         0,          0,          17,            0,         0,  &clk_lock, NULL, 0);
SUNXI_CLK_PERIPH(gmac0_mbus,        0,             0,      0,            0,            0,         0,          0,          0,          0,          0,          0,           GMAC_MBUS,           0,              0,         0,         11,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(gmac1_mbus,        0,             0,      0,            0,            0,         0,          0,          0,          0,          0,          0,           GMAC_MBUS,           0,              0,         0,         12,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(gmac0_phy,         0,             0,      0,            GMAC0_PHY,    0,         5,          0,          0,          0,       GMAC0_PHY,             0,           0,           0,              31,         0,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(gmac1_phy,         0,             0,      0,            GMAC1_PHY,    0,         5,          0,          0,          0,       GMAC1_PHY,             0,           0,           0,              31,         0,         0,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(gmac_ptp,          GMAC_PTP,             24,      3,    GMAC_PTP,     0,         5,          0,          0,          0,       GMAC_PTP,             0,           0,           0,              31,         0,         0,             0,             &clk_lock, NULL,             0);

struct periph_init_data sunxi_periphs_init[] = {
	{"de0",               CLK_SET_RATE_PARENT,           de_parents,               ARRAY_SIZE(de_parents),              &sunxi_clk_periph_de0            },
	{"de_rst_sys",                 0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_de_rst_sys     },
	{"sdmmc0_mod",                 0,                    smhc0_parents,            ARRAY_SIZE(smhc0_parents),           &sunxi_clk_periph_sdmmc0_mod     },
	{"sdmmc1_mod",                 0,                    smhc0_parents,            ARRAY_SIZE(smhc1_parents),           &sunxi_clk_periph_sdmmc1_mod     },
	{"sdmmc2_mod",                 0,                    smhc0_parents,            ARRAY_SIZE(smhc2_parents),           &sunxi_clk_periph_sdmmc2_mod     },
	{"sdmmc3_mod",                 0,                    smhc3_parents,            ARRAY_SIZE(smhc3_parents),           &sunxi_clk_periph_sdmmc3_mod     },
	{"dpss_top0",                  0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_dpss_top0      },
	{"dpss_top1",                  0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_dpss_top1      },
	{"video_out0",                 0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_video_out0_rst },
	{"video_out1",                 0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_video_out1_rst },
	{"tcon_lcd0",         CLK_SET_RATE_PARENT,        tcon_lcd0_parents,           ARRAY_SIZE(tcon_lcd0_parents),       &sunxi_clk_periph_tcon_lcd0      },
	{"tcon_lcd1",         CLK_SET_RATE_PARENT,        tcon_lcd1_parents,           ARRAY_SIZE(tcon_lcd1_parents),       &sunxi_clk_periph_tcon_lcd1      },
	{"tcon_lcd2",         CLK_SET_RATE_PARENT,        tcon_lcd2_parents,           ARRAY_SIZE(tcon_lcd2_parents),       &sunxi_clk_periph_tcon_lcd2      },
	{"tcon_tv0",                   0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_tcon_tv0       },
	{"tcon_tv1",                   0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_tcon_tv1       },
	{"mipi_dsi0",         CLK_SET_RATE_PARENT,        dsi0_parents,                ARRAY_SIZE(dsi0_parents),            &sunxi_clk_periph_mipi_dsi0      },
	{"mipi_dsi1",         CLK_SET_RATE_PARENT,        dsi1_parents,                ARRAY_SIZE(dsi1_parents),            &sunxi_clk_periph_mipi_dsi1      },
	{"mipi_dsc",                   0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_mipi_dsc       },
	{"mipi_dsi_combphy0", CLK_SET_RATE_PARENT,        combphy0_parents,            ARRAY_SIZE(combphy0_parents),        &sunxi_clk_periph_mipi_dsi_combphy0},
	{"mipi_dsi_combphy1", CLK_SET_RATE_PARENT,        combphy1_parents,            ARRAY_SIZE(combphy1_parents),        &sunxi_clk_periph_mipi_dsi_combphy1},
	{"edp",               CLK_SET_RATE_PARENT,        edp_parents,                 ARRAY_SIZE(edp_parents),             &sunxi_clk_periph_edp            },
	{"hdmi_tv",           CLK_SET_RATE_PARENT,        hdmi_tv_parents,             ARRAY_SIZE(hdmi_tv_parents),         &sunxi_clk_periph_hdmi_tv        },
	{"hdmi_gate",                  0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_hdmi_gate      },
	{"hdmi_sfr",          CLK_SET_RATE_PARENT,        hdmi_sfr_parents,            ARRAY_SIZE(hdmi_sfr_parents),        &sunxi_clk_periph_hdmi_sfr       },
	{"hdmi_hdcp_rst",              0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_hdmi_hdcp_rst  },
	{"hdmi_sub_rst",               0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_hdmi_sub_rst   },
	{"hdmi_main_rst",              0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_hdmi_main_rst  },
	{"lvds0",                      0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_lvds0          },
	{"lvds1",                      0,                    hosc_parents,             ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_lvds1          },
	{"eink",              CLK_SET_RATE_PARENT,         eink_parents,               ARRAY_SIZE(eink_parents),            &sunxi_clk_periph_eink           },
	{"eink_panel",        CLK_SET_RATE_PARENT,         eink_panel_parents,         ARRAY_SIZE(eink_panel_parents),      &sunxi_clk_periph_eink_panel     },
	{"serdes_phy_cfg",             0,                  serdes_phy_cfg_parents,     ARRAY_SIZE(serdes_phy_cfg_parents),  &sunxi_clk_periph_serdes_phy_cfg },
	{"gmac0",                      0,                  hosc_parents,               ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_gmac0          },
	{"gmac0_axi",                  0,                  hosc_parents,               ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_gmac0_axi      },
	{"gmac1",                      0,                  hosc_parents,               ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_gmac1          },
	{"gmac1_axi",                  0,                  hosc_parents,               ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_gmac1_axi      },
	{"gmac0_mbus",                 0,                  hosc_parents,               ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_gmac0_mbus     },
	{"gmac1_mbus",                 0,                  hosc_parents,               ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_gmac1_mbus     },
	{"gmac0_phy",                  0,                  gmac_phy_parents,           ARRAY_SIZE(gmac_phy_parents),        &sunxi_clk_periph_gmac0_phy      },
	{"gmac1_phy",                  0,                  gmac_phy_parents,           ARRAY_SIZE(gmac_phy_parents),        &sunxi_clk_periph_gmac1_phy      },
	{"gmac_ptp",                   0,                  gmac_ptp_parents,           ARRAY_SIZE(gmac_ptp_parents),        &sunxi_clk_periph_gmac_ptp       },
};

/*
   SUNXI_RTC_CLK_PERIPH(name,        mux_reg,         mux_sft, mux_wid,      div_reg,      div_msft,  div_mwid,   div_nsft,   div_nwid,  gate_flag,    en_reg,        rst_reg,      bus_gate_reg,   drm_gate_reg,    en_sft,    rst_sft,     bus_gate_sft,    dram_gate_sft,   lock,   com_gate,    com_gate_off)
 */
SUNXI_CLK_PERIPH(dcxo_serdes0_gate,        0,             0,      0,            0,            0,         0,          0,          0,          0,          0,             0,       DCXO_SERDES_GATE,      0,              0,          0,         4,             0,             &clk_lock, NULL,             0);
SUNXI_CLK_PERIPH(dcxo_serdes1_gate,        0,             0,      0,            0,            0,         0,          0,          0,          0,          0,             0,       DCXO_SERDES_GATE,      0,              0,          0,         5,             0,             &clk_lock, NULL,             0);

struct periph_init_data sunxi_rtc_periphs_init[] = {
	{"dcxo_serdes0_gate",          0,                  hosc_parents,               ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_dcxo_serdes0_gate},
	{"dcxo_serdes1_gate",          0,                  hosc_parents,               ARRAY_SIZE(hosc_parents),            &sunxi_clk_periph_dcxo_serdes1_gate},
};

/*
 * sunxi_clk_get_factor_by_name() - Get factor clk init config
 */
struct factor_init_data *sunxi_clk_get_factor_by_name(const char *name)
{
	struct factor_init_data *factor;
	int i;

	/* get pll clk init config */
	for (i = 0; i < ARRAY_SIZE(sunxi_factos); i++) {
		factor = &sunxi_factos[i];
		if (strcmp(name, factor->name))
			continue;
		return factor;
	}

	return NULL;
}

/*
 * sunxi_clk_get_periph_by_name() - Get periph clk init config
 */
struct periph_init_data *sunxi_clk_get_periph_by_name(const char *name)
{
	struct periph_init_data *perpih;
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_periphs_init); i++) {
		perpih = &sunxi_periphs_init[i];
		if (strcmp(name, perpih->name))
			continue;
		return perpih;
	}

	return NULL;
}

/*
 * sunxi_clk_get_periph_cpus_by_name() - Get periph clk init config
 */
struct periph_init_data *sunxi_clk_get_periph_cpus_by_name(const char *name)
{
	return NULL;
}

static int clk_video_set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate)
{
	unsigned long factor_m = 0;
	unsigned long reg;
	struct sunxi_clk_periph *periph = to_clk_periph(hw);
	struct sunxi_clk_periph_div *divider = &periph->divider;
	unsigned long div, div_m = 0;

	div = DIV_ROUND_UP_ULL(parent_rate, rate);

	if (!div) {
		div_m = 0;
	} else {
		div_m = 1 << divider->mwidth;

		factor_m = (div > div_m ? div_m : div) - 1;
		div_m = factor_m;
	}

	reg = periph_readl(periph, divider->reg);
	if (divider->mwidth)
		reg = SET_BITS(divider->mshift, divider->mwidth, reg, div_m);
	periph_writel(periph, reg, divider->reg);

	return 0;
}

struct clk_ops disp_priv_ops;
void set_disp_priv_ops(struct clk_ops *priv_ops)
{
	priv_ops->determine_rate = clk_divider_determine_rate;
	priv_ops->set_rate = clk_video_set_rate;
}

void sunxi_set_clk_priv_ops(char *clk_name, struct clk_ops *clk_priv_ops,
	void (*set_priv_ops)(struct clk_ops *priv_ops))
{
	int i = 0;
	sunxi_clk_get_periph_ops(clk_priv_ops);
	set_priv_ops(clk_priv_ops);
	for (i = 0; i < (ARRAY_SIZE(sunxi_periphs_init)); i++) {
		if (!strcmp(sunxi_periphs_init[i].name, clk_name))
			sunxi_periphs_init[i].periph->priv_clkops = clk_priv_ops;
	}
}

#define SUN60IW2_PLL_VIDEO0_CTRL_REG	0x120
#define SUN60IW2_PLL_VIDEO1_CTRL_REG	0x140
#define SUN60IW2_PLL_VIDEO2_CTRL_REG	0x160
#define SUN60IW2_PLL_DE_CTRL_REG	0x2E0

static const u32 sun60iw2_pll_regs[] = {
	SUN60IW2_PLL_VIDEO0_CTRL_REG,
	SUN60IW2_PLL_VIDEO1_CTRL_REG,
	SUN60IW2_PLL_VIDEO2_CTRL_REG,
	SUN60IW2_PLL_DE_CTRL_REG,
};

void init_clocks(void)
{
	int i;
	struct factor_init_data *factor;
	struct periph_init_data *periph;
	unsigned long reg;

	/* get clk register base address */
	sunxi_clk_base = (void *)0x02002000; // fixed base address.
	sunxi_clk_rtc_base = (void *)0x07090000; // fixed rtc base address.;

	sunxi_clk_factor_initlimits();
	clk_register_fixed_rate(NULL, "sys24M", NULL, CLK_IS_ROOT, 24000000);
	clk_register_fixed_rate(NULL, "hosc", NULL, CLK_IS_ROOT, 24000000);
	sunxi_set_clk_priv_ops("de0", &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("tcon_lcd0", &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("tcon_lcd1", &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("tcon_lcd2", &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("mipi_dsi0", &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("mipi_dsi1", &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("mipi_dsi_combphy0", &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("mipi_dsi_combphy1", &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("edp", &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("hdmi_tv", &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("hdmi_sfr", &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("eink", &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("eink_panel", &disp_priv_ops, set_disp_priv_ops);
	sunxi_set_clk_priv_ops("serdes_phy_cfg", &disp_priv_ops, set_disp_priv_ops);

	/* Enable the output of video Plls */
	for (i = 0; i < ARRAY_SIZE(sun60iw2_pll_regs); i++) {
		reg = readl(sunxi_clk_base + sun60iw2_pll_regs[i]);
		reg = SET_BITS(26, 2, reg, 0x3);
		writel(reg, sunxi_clk_base + sun60iw2_pll_regs[i]);
	}

	/* Enable the lock enable of video Plls */
	for (i = 0; i < ARRAY_SIZE(sun60iw2_pll_regs); i++) {
		reg = readl(sunxi_clk_base + sun60iw2_pll_regs[i]);
		reg = SET_BITS(29, 1, reg, 0x1);
		writel(reg, sunxi_clk_base + sun60iw2_pll_regs[i]);
	}

	/* register normal factors, based on sunxi factor framework */
	for (i = 0; i < ARRAY_SIZE(sunxi_factos); i++) {
		factor = &sunxi_factos[i];
		factor->priv_regops = NULL;
		sunxi_clk_register_factors(NULL, (void *)sunxi_clk_base,
				(struct factor_init_data *)factor);
	}

	clk_register_fixed_factor(NULL, "pll_periph1_600m", "pll_periph1_2x", 0, 1, 2);
	clk_register_fixed_factor(NULL, "pll_periph1_300m", "pll_periph1_600m", 0, 1, 2);
	clk_register_fixed_factor(NULL, "pll_periph1_400m", "pll_periph1_2x", 0, 1, 3);

	clk_register_fixed_factor(NULL, "pll_periph0_600m", "pll_periph0_2x", 0, 1, 2);
	clk_register_fixed_factor(NULL, "pll_periph0_400m", "pll_periph0_2x", 0, 1, 3);
	clk_register_fixed_factor(NULL, "pll_periph0_300m", "pll_periph0_600m", 0, 1, 2);
	clk_register_fixed_factor(NULL, "pll-periph0_200m", "pll_periph0_400m", 0, 1, 2);
	clk_register_fixed_factor(NULL, "pll_periph0_150m", "pll_periph0_300m", 0, 1, 2);

	/* register periph clock */
	for (i = 0; i < ARRAY_SIZE(sunxi_periphs_init); i++) {
		periph = &sunxi_periphs_init[i];
		periph->periph->priv_regops = NULL;
		sunxi_clk_register_periph(periph, sunxi_clk_base);
	}

	/* register rtc periph clock */
	for (i = 0; i < ARRAY_SIZE(sunxi_rtc_periphs_init); i++) {
		periph = &sunxi_rtc_periphs_init[i];
		periph->periph->priv_regops = NULL;
		sunxi_clk_register_periph(periph, sunxi_clk_rtc_base);
	}
}
