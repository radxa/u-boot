// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2007-2011
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Aaron <leafy.myeh@allwinnertech.com>
 *
 * MMC driver for allwinner sunxi platform.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch-sunxi/clock.h>
#include <asm/arch-sunxi/cpu.h>
#include <asm/arch-sunxi/mmc.h>
#include <asm/arch-sunxi/timer.h>
#include <malloc.h>
#include <mmc.h>
#include <sys_config.h>
#include <linux/libfdt.h>
#include <fdt_support.h>
#include <private_uboot.h>

#include "../sunxi_mmc.h"
#include "../mmc_def.h"
#include "sunxi_mmc_host_common.h"
#include "sunxi_mmc_host_tm4.h"

#define  SMC_DATA_TIMEOUT     0xffffffU
#define  SMC_RESP_TIMEOUT     0xff

#define SUNXI_DMA_TL_TM4_V4P5X                ((0x3<<28)|(15<<16)|240)
extern char *spd_name[];
extern struct sunxi_mmc_priv mmc_host[4];

static int mmc_init_default_timing_para(int sdc_no)
{
	struct sunxi_mmc_priv *mmcpriv = &mmc_host[sdc_no];
	struct mmc_config *cfg = &mmcpriv->cfg;

	/* timing mode 4 */
	mmcpriv->tm4.cur_spd_md = DS26_SDR12;
	mmcpriv->tm4.cur_freq = CLK_400K;
	mmcpriv->tm4.sample_point_cnt = MMC_CLK_SAMPLE_POINIT_MODE_4;

	mmcpriv->tm4.def_odly[DS26_SDR12*MAX_CLK_FREQ_NUM+CLK_400K] = TM4_OUT_PH180;
	mmcpriv->tm4.def_sdly[DS26_SDR12*MAX_CLK_FREQ_NUM+CLK_400K] = 0x0;
	mmcpriv->tm4.def_odly[DS26_SDR12*MAX_CLK_FREQ_NUM+CLK_25M] = TM4_OUT_PH180;
	mmcpriv->tm4.def_sdly[DS26_SDR12*MAX_CLK_FREQ_NUM+CLK_25M] = 0x0;

	mmcpriv->tm4.def_odly[HSSDR52_SDR25*MAX_CLK_FREQ_NUM+CLK_400K] = TM4_OUT_PH180;
	mmcpriv->tm4.def_sdly[HSSDR52_SDR25*MAX_CLK_FREQ_NUM+CLK_400K] = 0;
	mmcpriv->tm4.def_odly[HSSDR52_SDR25*MAX_CLK_FREQ_NUM+CLK_25M] = TM4_OUT_PH180;
	mmcpriv->tm4.def_sdly[HSSDR52_SDR25*MAX_CLK_FREQ_NUM+CLK_25M] = 0;
	mmcpriv->tm4.def_odly[HSSDR52_SDR25*MAX_CLK_FREQ_NUM+CLK_50M] = TM4_OUT_PH180;
	mmcpriv->tm4.def_sdly[HSSDR52_SDR25*MAX_CLK_FREQ_NUM+CLK_50M] = 0;

	if (cfg->host_caps & MMC_MODE_8BIT) {
		mmcpriv->tm4.def_odly[HSDDR52_DDR50*MAX_CLK_FREQ_NUM+CLK_400K] = TM4_OUT_PH180;
		mmcpriv->tm4.def_sdly[HSDDR52_DDR50*MAX_CLK_FREQ_NUM+CLK_400K] = 0xe;
		mmcpriv->tm4.def_odly[HSDDR52_DDR50*MAX_CLK_FREQ_NUM+CLK_25M] = TM4_OUT_PH180;
		mmcpriv->tm4.def_sdly[HSDDR52_DDR50*MAX_CLK_FREQ_NUM+CLK_25M] = 0xe;
		mmcpriv->tm4.def_odly[HSDDR52_DDR50*MAX_CLK_FREQ_NUM+CLK_50M] = TM4_OUT_PH180;
		mmcpriv->tm4.def_sdly[HSDDR52_DDR50*MAX_CLK_FREQ_NUM+CLK_50M] = 0xe;
	} else if (cfg->host_caps & MMC_MODE_4BIT) {
		mmcpriv->tm4.def_odly[HSDDR52_DDR50*MAX_CLK_FREQ_NUM+CLK_400K] = TM4_OUT_PH90;
		mmcpriv->tm4.def_sdly[HSDDR52_DDR50*MAX_CLK_FREQ_NUM+CLK_400K] = 0xe;
		mmcpriv->tm4.def_odly[HSDDR52_DDR50*MAX_CLK_FREQ_NUM+CLK_25M] = TM4_OUT_PH90;
		mmcpriv->tm4.def_sdly[HSDDR52_DDR50*MAX_CLK_FREQ_NUM+CLK_25M] = 0xe;
		mmcpriv->tm4.def_odly[HSDDR52_DDR50*MAX_CLK_FREQ_NUM+CLK_50M] = TM4_OUT_PH90;
		mmcpriv->tm4.def_sdly[HSDDR52_DDR50*MAX_CLK_FREQ_NUM+CLK_50M] = 0xe;
	}

	mmcpriv->tm4.def_odly[HS200_SDR104*MAX_CLK_FREQ_NUM+CLK_400K] = TM4_OUT_PH180;
	mmcpriv->tm4.def_sdly[HS200_SDR104*MAX_CLK_FREQ_NUM+CLK_400K] = 0x0;
	mmcpriv->tm4.def_odly[HS200_SDR104*MAX_CLK_FREQ_NUM+CLK_25M] = TM4_OUT_PH90;
	mmcpriv->tm4.def_sdly[HS200_SDR104*MAX_CLK_FREQ_NUM+CLK_25M] = 0x0;
	mmcpriv->tm4.def_odly[HS200_SDR104*MAX_CLK_FREQ_NUM+CLK_50M] = TM4_OUT_PH90;
	mmcpriv->tm4.def_sdly[HS200_SDR104*MAX_CLK_FREQ_NUM+CLK_50M] = 0x11;
	mmcpriv->tm4.def_odly[HS200_SDR104*MAX_CLK_FREQ_NUM+CLK_100M] = TM4_OUT_PH90;
	mmcpriv->tm4.def_sdly[HS200_SDR104*MAX_CLK_FREQ_NUM+CLK_100M] = 0x12;
	mmcpriv->tm4.def_odly[HS200_SDR104*MAX_CLK_FREQ_NUM+CLK_150M] = TM4_OUT_PH90;
	mmcpriv->tm4.def_sdly[HS200_SDR104*MAX_CLK_FREQ_NUM+CLK_150M] = 0x13;
	mmcpriv->tm4.def_odly[HS200_SDR104*MAX_CLK_FREQ_NUM+CLK_200M] = TM4_OUT_PH90;
	mmcpriv->tm4.def_sdly[HS200_SDR104*MAX_CLK_FREQ_NUM+CLK_200M] = 0x6;

	mmcpriv->tm4.def_odly[HS400*MAX_CLK_FREQ_NUM+CLK_400K] = TM4_OUT_PH180;
	mmcpriv->tm4.def_sdly[HS400*MAX_CLK_FREQ_NUM+CLK_400K] = 0x0;
	mmcpriv->tm4.def_odly[HS400*MAX_CLK_FREQ_NUM+CLK_25M] = TM4_OUT_PH90;
	mmcpriv->tm4.def_sdly[HS400*MAX_CLK_FREQ_NUM+CLK_25M] = 0x0;
	mmcpriv->tm4.def_odly[HS400*MAX_CLK_FREQ_NUM+CLK_50M] = TM4_OUT_PH90;
	mmcpriv->tm4.def_sdly[HS400*MAX_CLK_FREQ_NUM+CLK_50M] = 0x11;
	mmcpriv->tm4.def_odly[HS400*MAX_CLK_FREQ_NUM+CLK_100M] = TM4_OUT_PH90;
	mmcpriv->tm4.def_sdly[HS400*MAX_CLK_FREQ_NUM+CLK_100M] = 0x12;
	mmcpriv->tm4.def_odly[HS400*MAX_CLK_FREQ_NUM+CLK_150M] = TM4_OUT_PH90;
	mmcpriv->tm4.def_sdly[HS400*MAX_CLK_FREQ_NUM+CLK_150M] = 0x13;
	mmcpriv->tm4.def_odly[HS400*MAX_CLK_FREQ_NUM+CLK_200M] = TM4_OUT_PH90;
	mmcpriv->tm4.def_sdly[HS400*MAX_CLK_FREQ_NUM+CLK_200M] = 0x6;

	mmcpriv->tm4.def_dsdly[CLK_25M] = 0x0;
	mmcpriv->tm4.def_dsdly[CLK_50M] = 0x18;
	mmcpriv->tm4.def_dsdly[CLK_100M] = 0xd;
	mmcpriv->tm4.def_dsdly[CLK_150M] = 0x6;
	mmcpriv->tm4.def_dsdly[CLK_200M] = 0x3;

	return 0;
}

static void sunxi_mmc_clk_io_onoff(int sdc_no, int onoff, int reset_clk)
{
	int rval;
	struct sunxi_mmc_priv *priv = &mmc_host[sdc_no];

#if defined(CONFIG_MACH_SUN300IW1)
	if (onoff) {
		rval = readl(priv->hclkrst);
		rval |= (1 << (20 + sdc_no));
		writel(rval, priv->hclkrst);
		rval = readl(priv->hclkbase);
		rval |= (1 << (20 + sdc_no));
		writel(rval, priv->hclkbase);

		rval = readl(priv->mclkreg);
		rval |= (1U << 31);
		writel(rval, priv->mclkreg);
	} else {
		rval = readl(priv->mclkreg);
		rval &= ~(1U << 31);
		writel(rval, priv->mclkreg);

		rval = readl(priv->hclkbase);
		rval &= ~(1 << (20 + sdc_no));
		writel(rval, priv->hclkbase);

		rval = readl(priv->hclkrst);
		rval &= ~(1 << (20 + sdc_no));
		writel(rval, priv->hclkrst);
	}
#elif (defined(CONFIG_MACH_SUN60IW1) || defined(CONFIG_MACH_SUN60IW2) || defined(CONFIG_MACH_SUN55IW6) || defined(CONFIG_MACH_SUN65IW1))
	if (onoff) {
		rval = readl(priv->hclkrst);
		rval |= (1 << 16);
		writel(rval, priv->hclkrst);
		rval = readl(priv->hclkbase);
		rval |= (1 << 0);
		writel(rval, priv->hclkbase);

		rval = readl(priv->mclkreg);
		rval |= (1U << 31);
		writel(rval, priv->mclkreg);
	} else {
		rval = readl(priv->mclkreg);
		rval &= ~(1U << 31);
		writel(rval, priv->mclkreg);

		rval = readl(priv->hclkbase);
		rval &= ~(1 << 0);
		writel(rval, priv->hclkbase);

		rval = readl(priv->hclkrst);
		rval &= ~(1 << 16);
		writel(rval, priv->hclkrst);
	}
#elif (!(defined(CONFIG_MACH_SUN50IW1) || defined(CONFIG_MACH_SUN8IW11)))
	/* config ahb clock */
	if (onoff) {
		rval = readl(priv->hclkrst);
		rval |= (1 << (16 + sdc_no));
		writel(rval, priv->hclkrst);
		rval = readl(priv->hclkbase);
		rval |= (1 << (0 + sdc_no));
		writel(rval, priv->hclkbase);

		rval = readl(priv->mclkreg);
		rval |= (1U << 31);
		writel(rval, priv->mclkreg);
	} else {
		rval = readl(priv->mclkreg);
		rval &= ~(1U << 31);
		writel(rval, priv->mclkreg);

		rval = readl(priv->hclkbase);
		rval &= ~(1 << (0 + sdc_no));
		writel(rval, priv->hclkbase);

		rval = readl(priv->hclkrst);
		rval &= ~(1 << (16 + sdc_no));
		writel(rval, priv->hclkrst);
	}
#else
	/* For Special Platform: A64 */
	/* config ahb clock */
	if (onoff) {
		rval = readl(priv->hclkrst);
		rval |= (1 << (8 + sdc_no));
		writel(rval, priv->hclkrst);
		rval = readl(priv->hclkbase);
		rval |= (1 << (8 + sdc_no));
		writel(rval, priv->hclkbase);

		rval = readl(priv->mclkreg);
		rval |= (1U << 31);
		writel(rval, priv->mclkreg);
	} else {
		rval = readl(priv->mclkreg);
		rval &= ~(1U << 31);
		writel(rval, priv->mclkreg);

		rval = readl(priv->hclkbase);
		rval &= ~(1 << (8 + sdc_no));
		writel(rval, priv->hclkbase);

		rval = readl(priv->hclkrst);
		rval &= ~(1 << (8 + sdc_no));
		writel(rval, priv->hclkrst);
	}
#endif
	/* config mod clock */
	if (reset_clk) {
		rval = readl(priv->mclkreg);
		/*set to 24M default value*/
		rval &= ~(0x7fffffff);
		sunxi_r_op(priv, writel(rval, priv->mclkreg));
		priv->mod_clk = 24000000;
	}
	/*
	   dumphex32("ccmu", (char *)SUNXI_CCM_BASE, 0x100);
	   dumphex32("gpio", (char *)SUNXI_PIO_BASE, 0x100);
	   dumphex32("mmc", (char *)priv->reg, 0x100);
	   */
}

int mmc_config_delay_tm4(struct sunxi_mmc_priv *mmcpriv)
{
	unsigned int rval = 0;
	unsigned int spd_md, freq;
	u8 odly, sdly, dsdly = 0;

	spd_md = mmcpriv->tm4.cur_spd_md;
	freq = mmcpriv->tm4.cur_freq;

	if (mmcpriv->tm4.sdly[spd_md*MAX_CLK_FREQ_NUM+freq] != 0xFF)
		sdly = mmcpriv->tm4.sdly[spd_md*MAX_CLK_FREQ_NUM+freq];
	else
		sdly = mmcpriv->tm4.def_sdly[spd_md*MAX_CLK_FREQ_NUM+freq];

	if (mmcpriv->tm4.odly[spd_md*MAX_CLK_FREQ_NUM+freq] != 0xFF)
		odly = mmcpriv->tm4.odly[spd_md*MAX_CLK_FREQ_NUM+freq];
	else
		odly = mmcpriv->tm4.def_odly[spd_md*MAX_CLK_FREQ_NUM+freq];

	mmcpriv->tm4.cur_odly = odly;
	mmcpriv->tm4.cur_sdly = sdly;

	rval = readl(&mmcpriv->reg->drv_dl);
	rval &= (~(0x3<<16));
	rval |= (((odly&0x1)<<16) | ((odly&0x1)<<17));
	sunxi_r_op(mmcpriv, writel(rval, &mmcpriv->reg->drv_dl));

	rval = readl(&mmcpriv->reg->samp_dl);
	rval &= (~SDXC_CfgDly);
	rval |= ((sdly&SDXC_CfgDly) | SDXC_EnableDly);
	writel(rval, &mmcpriv->reg->samp_dl);

	if (spd_md == HS400) {
		if (mmcpriv->tm4.dsdly[freq] != 0xFF)
			dsdly = mmcpriv->tm4.dsdly[freq];
		else
			dsdly = mmcpriv->tm4.def_dsdly[freq];
		mmcpriv->tm4.cur_dsdly = dsdly;

		rval = readl(&mmcpriv->reg->ds_dl);
		rval &= (~SDXC_CfgDly);
		rval |= ((dsdly&SDXC_CfgDly) | SDXC_EnableDly);
#ifdef FPGA_PLATFORM
		rval &= (~0x7);
#endif
		writel(rval, &mmcpriv->reg->ds_dl);
	}

#if defined(CONFIG_MACH_SUN8IW15) || defined(CONFIG_MACH_SUN8IW16)
	rval = readl(&mmcpriv->reg->sfc);
	rval |= 0x1;
	writel(rval, &mmcpriv->reg->sfc);
	MMCDBG("sfc 0x%x\n", readl(&mmcpriv->reg->sfc));
	/* uboot use sample fifo bypass + clk always on, to fix sample bug */
#elif (defined(CONFIG_MACH_SUN55IW3) || defined(CONFIG_MACH_SUN60IW2)\
		|| defined(CONFIG_MACH_SUN55IW6) || defined(CONFIG_MACH_SUN65IW1)\
		|| defined(CONFIG_MACH_SUN8IW22))
	if (mmcpriv->tuning_smode != TUNING_END && mmcpriv->tuning_smode != TUNING_HS400) {
		rval = readl(&mmcpriv->reg->sfc);
		rval |= 0x1;
		writel(rval, &mmcpriv->reg->sfc);
		MMCDBG("sfc 0x%x\n", readl(&mmcpriv->reg->sfc));
	} else {
		rval = readl(&mmcpriv->reg->sfc);
		rval &= (~0x1);
		writel(rval, &mmcpriv->reg->sfc);
		MMCDBG("sfc 0x%x\n", readl(&mmcpriv->reg->sfc));
	}
#endif

	MMCDBG("%s: spd_md:%d, freq:%d, odly: %d; sdly: %d; dsdly: %d\n", __FUNCTION__, spd_md, freq, odly, sdly, dsdly);

	return 0;
}

static int mmc_set_mod_clk(struct sunxi_mmc_priv *priv, unsigned int hz)
{
	unsigned int mod_hz, freq_id;
	struct mmc *mmc = priv->mmc;
	u32 val = 0, ret = 0;
	mod_hz = 0;

	/*Just to ensure that mass-produced platforms are not affected,
	 * new platforms should not take this path*/
	if (priv->clk_mode == SUNXI_MMC_CLK_ORGN || priv->clk_mode == SUNXI_MMC_CLK_FPGA) {
		ret = priv->sunxi_mmc_set_clk(priv, hz, mod_hz, &freq_id);
		if (ret < 0) {
			MMCINFO("%s--%d set clk failed!!!!\n", __func__, __LINE__);
			return -1;
		}
		return 0;
	}

	/*
	 * The MMC clock has an extra /2 post-divider when operating in the new
	 * mode.
	 */
	if ((mmc->speed_mode == HSDDR52_DDR50) && (mmc->bus_width == 8))
		mod_hz = hz * 4;/* 4xclk: DDR8(HS) */
	else
		mod_hz = hz * 2;/* 2xclk: SDR 1/4/8; DDR4(HS); DDR8(HS400) */

	ret = priv->sunxi_mmc_set_clk(priv, hz, mod_hz, &freq_id);
	if (ret < 0) {
		MMCINFO("%s--%d set clk failed!!!!\n", __func__, __LINE__);
		return -1;
	}

	/* used for some host which NTSR default use 2x mode */
	if (priv->version >= 0x50300) {
		val = readl(&priv->reg->ntsr);
		val &= ~SUNXI_MMC_NTSR_MODE_SEL_NEW;
		writel(val, &priv->reg->ntsr);
		MMCDBG("Clear NTSR bit 31 to shutdown 2x mode!\n");
	}

	val = readl(&priv->reg->clkcr);
	val &= ~0xff;
	if (mmc->speed_mode == HSDDR52_DDR50 && (mmc->bus_width == 8))
		val |= 0x1;
	writel(val, &priv->reg->clkcr);

	priv->tm4.cur_spd_md = mmc->speed_mode;
	priv->tm4.cur_freq = freq_id;

	mmc_config_delay_tm4(priv);

	debug("mclk reg***%x\n", readl(priv->mclkreg));
	debug("clkcr reg***%x\n", readl(&priv->reg->clkcr));

	return 0;
}

static void mmc_ddr_mode_onoff(struct sunxi_mmc_priv *priv, int on)
{
	u32 rval = 0;

	rval = readl(&priv->reg->gctrl);
	rval &= (~(1U << 10));

	if (on) {
		rval |= (1U << 10);
		sunxi_r_op(priv, writel(rval, &priv->reg->gctrl));
		MMCDBG("set %d rgctrl 0x%x to enable ddr mode\n",
				priv->mmc_no, readl(&priv->reg->gctrl));
	} else {
		sunxi_r_op(priv, writel(rval, &priv->reg->gctrl));
		MMCDBG("set %d rgctrl 0x%x to disable ddr mode\n",
				priv->mmc_no, readl(&priv->reg->gctrl));
	}
}

static void mmc_hs400_mode_onoff(struct sunxi_mmc_priv *priv, int on)
{
	u32 rval = 0;

	rval = readl(&priv->reg->dsbd);
	rval &= (~(1U << 31));

	if (on) {
		rval |= (1U << 31);
		writel(rval, &priv->reg->dsbd);
		rval = readl(&priv->reg->csdc);
		rval &= ~0xF;
		rval |= 0x6;
		writel(rval, &priv->reg->csdc);
		MMCDBG("set %d dsbd 0x%x to enable hs400 mode\n",
				priv->mmc_no, readl(&priv->reg->dsbd));
	} else {
		writel(rval, &priv->reg->dsbd);
		rval = readl(&priv->reg->csdc);
		rval &= ~0xF;
		rval |= 0x3;
		writel(rval, &priv->reg->csdc);
		MMCDBG("set %d dsbd 0x%x to disable hs400 mode\n",
				priv->mmc_no, readl(&priv->reg->dsbd));
	}
}

static void sunxi_mmc_set_speed_mode(struct sunxi_mmc_priv *priv,
		struct mmc *mmc)
{
	/* set speed mode */
	if (mmc->speed_mode == HSDDR52_DDR50) {
		mmc_ddr_mode_onoff(priv, 1);
		mmc_hs400_mode_onoff(priv, 0);
	} else if (mmc->speed_mode == HS400) {
		mmc_ddr_mode_onoff(priv, 0);
		mmc_hs400_mode_onoff(priv, 1);
	} else {
		mmc_ddr_mode_onoff(priv, 0);
		mmc_hs400_mode_onoff(priv, 0);
	}
}

extern int mmc_update_clk(struct sunxi_mmc_priv *priv);
static int mmc_calibrate_delay_unit(struct sunxi_mmc_priv *mmchost)
{
 #ifndef FPGA_PLATFORM
	struct mmc_reg_v4p1 *reg = (struct mmc_reg_v4p1 *)mmchost->reg;
	unsigned rval = 0;
	unsigned clk[4] = {50*1000*1000, 100*1000*1000, 150*1000*1000, 200*1000*1000};
	/*ps, module clk is 2xclk at init phase.*/
	unsigned period[4] = {10*1000, 5*1000, 3333, 2500};
	unsigned result = 0;
	int i = 0;

	if (mmchost->tm4.dsdly_unit_ps != 0) {
		MMCDBG("%s: don't need calibrate delay unit\n", __FUNCTION__);
		return 0;
	}
	MMCDBG("start %s, don't access device...\n", __FUNCTION__);

	for (i = 3; i < 4; i++) {
		MMCINFO("%d MHz...\n", clk[i]/1000/1000);
		/* close card clock */
		rval = readl(&reg->clkcr);
		rval &= ~(1 << 16);
		writel(rval, &reg->clkcr);
		if (mmc_update_clk(mmchost))
			return -1;

		/* set card clock to 100MHz */
		if ((mmchost->timing_mode == SUNXI_MMC_TIMING_MODE_1)
			|| (mmchost->timing_mode == SUNXI_MMC_TIMING_MODE_3)
			|| (mmchost->timing_mode == SUNXI_MMC_TIMING_MODE_4))
			mmc_set_mod_clk(mmchost, clk[i]);
		else {
			MMCINFO("%s: mmc %d wrong timing mode: 0x%x\n",
				__FUNCTION__, mmchost->mmc_no, mmchost->timing_mode);
			return -1;
		}

		/* start carlibrate delay unit */
		writel(0xA0, &reg->samp_dl);
		writel(0x0, &reg->samp_dl);
		rval = SDXC_StartCal;
		writel(rval, &reg->samp_dl);
		writel(0x0, &reg->samp_dl);
		while (!(readl(&reg->samp_dl) & SDXC_CalDone)) {
			;
		}

		if (mmchost->mmc_no == 2) {
			writel(0xA0, &reg->ds_dl);
			writel(0x0, &reg->ds_dl);
			rval = SDXC_StartCal;
			writel(rval, &reg->ds_dl);
			writel(0x0, &reg->ds_dl);
			while (!(readl(&reg->ds_dl) & SDXC_CalDone)) {
				;
			}
		}

		/* update result */
		rval = readl(&reg->samp_dl);
		result = (rval & SDXC_CalDly) >> 8;
		MMCDBG("samp_dl result: 0x%x\n", result);
		if (result) {
			rval = period[i] / result;
			mmchost->tm3.sdly_unit_ps = rval;
			mmchost->tm3.dly_calibrate_done = 1;
			mmchost->tm4.sdly_unit_ps = rval;
			MMCINFO("sample: %d - %d(ps)\n", result, mmchost->tm3.sdly_unit_ps);
		} else {
			MMCINFO("%s: cal sample delay fail\n", __FUNCTION__);
		}
		if (mmchost->mmc_no == 2) {
			rval = readl(&reg->ds_dl);
			result = (rval & SDXC_CalDly) >> 8;
			MMCDBG("ds_dl result: 0x%x\n", result);
			if (result) {
				rval = period[i] / result;
				mmchost->tm4.dsdly_unit_ps = rval;
				mmchost->tm4.dly_calibrate_done = 1;
				MMCINFO("ds: %d - %d(ps)\n", result, mmchost->tm4.dsdly_unit_ps);
			} else {
				MMCINFO("%s: cal data strobe delay fail\n", __FUNCTION__);
			}
		}
	}

#endif	/* FPGA_PLATFORM */
	return 0;
}

static void sunxi_mmc_core_init(struct mmc *mmc)
{
	struct sunxi_mmc_priv *priv = mmc->priv;

	/* Reset controller */
	writel(SUNXI_MMC_GCTRL_RESET, &priv->reg->gctrl);
	udelay(1000);
	/* release eMMC reset signal */
	writel(1, &priv->reg->hwrst);
	writel(0, &priv->reg->hwrst);
	udelay(1000);
	writel(1, &priv->reg->hwrst);
	udelay(1000);
	/* Set Data & Response Timeout Value */
	writel((SMC_DATA_TIMEOUT<<8)|SMC_RESP_TIMEOUT, &priv->reg->timeout);

	writel((512<<16)|(1U<<2)|(1U<<0), &priv->reg->thldc);
	writel(3, &priv->reg->csdc);
	writel(0xdeb, &priv->reg->dbgc);
	mmc_calibrate_delay_unit(priv);

}

void sunxi_mmc_host_tm4_init(int sdc_no)
{
	struct sunxi_mmc_priv *mmcpriv = &mmc_host[sdc_no];
	struct mmc_config *cfg = &mmcpriv->cfg;

	cfg->f_max = 200000000; /* 200M */

	mmcpriv->dma_tl = SUNXI_DMA_TL_TM4_V4P5X;
	mmcpriv->timing_mode = SUNXI_MMC_TIMING_MODE_4;
	mmcpriv->mmc_init_default_timing_para = mmc_init_default_timing_para;
	mmcpriv->mmc_set_mod_clk = mmc_set_mod_clk;
	mmcpriv->sunxi_mmc_set_speed_mode = sunxi_mmc_set_speed_mode;
	mmcpriv->sunxi_mmc_core_init = sunxi_mmc_core_init;
	mmcpriv->sunxi_mmc_clk_io_onoff = sunxi_mmc_clk_io_onoff;
}
