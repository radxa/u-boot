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
#include "sunxi_mmc_host_tm1.h"
#include "sunxi_mmc_host_tm4.h"

int sunxi_mmc_set_clk_fpga_tm1(struct sunxi_mmc_priv *priv, unsigned int hz,
					unsigned int mhz, unsigned int *fid)
{
#ifdef FPGA_PLATFORM
	unsigned int freq_id, mod_hz;
	struct mmc *mmc = priv->mmc;
	unsigned int pll, pll_hz, div, n;

	/*
	 * The MMC clock has an extra /2 post-divider when operating in the new
	 * mode.
	 */
	if (mmc->speed_mode == HSDDR52_DDR50)
		mod_hz = hz * 4;
	else
		mod_hz = hz * 2;

	if (mod_hz <= 24000000) {
		pll = CCM_MMC_CTRL_OSCM24;
		pll_hz = 24000000;
	} else {
		pll = sunxi_mmc_get_src_clk_no(priv->mmc_no, mod_hz, 1);
		pll_hz = sunxi_host_src_clk(priv->mmc_no, (pll>>24), 1);
	}

	MMCDBG("pll config :%d : %d\n", pll, pll_hz);

	div = pll_hz / mod_hz;
	if (pll_hz % mod_hz)
		div++;

	n = 0;
	while (div > 16) {
		n++;
		div = (div + 1) / 2;
	}

	if (n > 3) {
		MMCINFO("mmc %u error cannot set clock to %u\n", priv->mmc_no,
		       hz);
		return -1;
	}
	freq_id = CLK_50M;
	/* determine delays */
	if (hz <= 400000)
		freq_id = CLK_400K;
	else if (hz <= 25000000)
		freq_id = CLK_25M;
	else if (hz <= 52000000)
		freq_id = CLK_50M;
	else if (hz <= 100000000)
		freq_id = CLK_100M;
	else if (hz <= 150000000)
		freq_id = CLK_150M;
	else if (hz <= 200000000)
		freq_id = CLK_200M;
	else {
		/* hz > 52000000 */
		freq_id = CLK_50M;
	}

	*fid = freq_id;
	MMCDBG("freq_id:%d\n", freq_id);

	if (mod_hz > (400000 * 2)) {
		sunxi_r_op(priv, writel(CCM_MMC_CTRL_ENABLE,  priv->mclkreg));
	} else {
		sunxi_r_op(priv, writel(pll | CCM_MMC_CTRL_N(n) |
			CCM_MMC_CTRL_M(div), priv->mclkreg));
	}
	sunxi_r_op(priv, writel(readl(&priv->reg->drv_dl) | (0x1 << 7), &priv->reg->drv_dl));

#ifndef CONFIG_MACH_SUN300IW1
	unsigned int rval;
	if (readl(IOMEM_ADDR(SUNXI_MMMC_1X_2X_MODE_CTL_REG)) & (0x1 << 3)) {
		rval = readl(&priv->reg->ntsr);
		rval |= SUNXI_MMC_NTSR_MODE_SEL_NEW;
		writel(rval, &priv->reg->ntsr);
	} else {
		rval = readl(&priv->reg->ntsr);
		rval &= ~SUNXI_MMC_NTSR_MODE_SEL_NEW;
		writel(rval, &priv->reg->ntsr);
	}
#endif
	setbits_le32(&priv->reg->ntsr, SUNXI_MMC_NTSR_MODE_SEL_NEW);

	priv->tm1.cur_spd_md = mmc->speed_mode;
	priv->tm1.cur_freq = freq_id;
	mmc_config_delay_tm1(priv);

	MMCDBG("mclk reg***%x\n", readl(priv->mclkreg));
	MMCDBG("clkcr reg***%x\n", readl(&priv->reg->clkcr));

	return 0;
#else
	MMCINFO("%s: FPGA clk driver only can be used for fpga, please check!!!\n", __func__);
	return -1;
#endif
}

int sunxi_mmc_set_clk_fpga_tm4(struct sunxi_mmc_priv *priv, unsigned int hz,
					unsigned int mhz, unsigned int *fid)
{
#ifdef FPGA_PLATFORM
	unsigned int freq_id, mod_hz;
	u32 val = 0;
	struct mmc *mmc = priv->mmc;
	unsigned int pll, pll_hz, div, n;

	/*
	 * The MMC clock has an extra /2 post-divider when operating in the new
	 * mode.
	 */
	if ((mmc->speed_mode == HSDDR52_DDR50) && (mmc->bus_width == 8))
		mod_hz = hz * 4;/* 4xclk: DDR8(HS) */
	else
		mod_hz = hz * 2;/* 2xclk: SDR 1/4/8; DDR4(HS); DDR8(HS400) */

	if (mod_hz <= 24000000) {
		pll = CCM_MMC_CTRL_OSCM24;
		pll_hz = 24000000;
	} else {
		pll = sunxi_mmc_get_src_clk_no(priv->mmc_no, mod_hz, 4);
		pll_hz = sunxi_host_src_clk(priv->mmc_no, (pll>>24), 4);
	}

	MMCDBG("pll config pll:%x pll_hz:%d mod_hz:%d\n", pll, pll_hz, mod_hz);

	div = pll_hz / mod_hz;
	if (pll_hz % mod_hz)
		div++;

	n = 0;
	while (div > 16) {
		n++;
		div = (div + 1) / 2;
	}

	if (n > 3) {
		MMCINFO("mmc %u error cannot set clock to %u\n", priv->mmc_no,
		       hz);
		return -1;
	}
	freq_id = CLK_50M;
	/* determine delays */
	if (hz <= 400000)
		freq_id = CLK_400K;
	else if (hz <= 25000000)
		freq_id = CLK_25M;
	else if (hz <= 52000000)
		freq_id = CLK_50M;
	else if (hz <= 100000000)
		freq_id = CLK_100M;
	else if (hz <= 150000000)
		freq_id = CLK_150M;
	else if (hz <= 200000000)
		freq_id = CLK_200M;
	else {
		/* hz > 52000000 */
		freq_id = CLK_50M;
	}

	*fid = freq_id;
	MMCDBG("freq_id:%d\n", freq_id);

	if (mod_hz > (400000 * 2)) {
		sunxi_r_op(priv, writel(CCM_MMC_CTRL_ENABLE,  priv->mclkreg));
	} else {
		sunxi_r_op(priv, writel(pll | CCM_MMC_CTRL_N(n) |
			CCM_MMC_CTRL_M(div), priv->mclkreg));
	}
	if (hz <= 400000) {
		sunxi_r_op(priv, writel(readl(&priv->reg->drv_dl) & ~(0x1 << 7), &priv->reg->drv_dl));
	} else {
		sunxi_r_op(priv, writel(readl(&priv->reg->drv_dl) | (0x1 << 7), &priv->reg->drv_dl));
	}

	/* used for some host which NTSR default use 2x mode */
	if (priv->version >= 0x50300) {
		val = readl(&priv->reg->ntsr);
		val &= ~SUNXI_MMC_NTSR_MODE_SEL_NEW;
		writel(val, &priv->reg->ntsr);
		MMCDBG("Clear NTSR bit 31 to shutdown 2x mode!\n");
	}

	priv->tm4.cur_spd_md = mmc->speed_mode;
	priv->tm4.cur_freq = freq_id;

	mmc_config_delay_tm4(priv);

	debug("mclk reg***%x\n", readl(priv->mclkreg));
	debug("clkcr reg***%x\n", readl(&priv->reg->clkcr));

	return 0;
#else
	MMCINFO("%s: FPGA clk driver only can be used for fpga, please check!!!\n", __func__);
	return -1;
#endif
}
