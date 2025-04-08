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

#if defined(CONFIG_MACH_SUN55IW6)
static void sunxi_mmc_get_src_clk_calc(int sdc_no, int mod_hz,
				unsigned int *pll, unsigned int *pll_hz)
{
	unsigned int pll1, pll_hz1, pll2, pll_hz2;

	if (mod_hz <= 24000000) {
		*pll = CCM_MMC_CTRL_OSCM24;
		*pll_hz = 24000000;
	} else {
		if (sdc_no == 2) {
			pll1 = CCM_MMC_CTRL_PERI0_800M;
			pll_hz1 = CCM_MMC_CTRL_PERI0_800M_freq;
			pll2 = CCM_MMC_CTRL_PERI0_600M;
			pll_hz2 = CCM_MMC_CTRL_PERI0_600M_freq;
		} else {
			pll1 = CCM_MMC_CTRL_PERI0_400M;
			pll_hz1 = CCM_MMC_CTRL_PERI0_400M_freq;
			pll2 = CCM_MMC_CTRL_PERI0_300M;
			pll_hz2 = CCM_MMC_CTRL_PERI0_300M_freq;
		}

		if ((pll_hz1 % mod_hz) <= (pll_hz2 % mod_hz)) {
			*pll = pll1;
			*pll_hz = pll_hz1;
		} else {
			*pll = pll2;
			*pll_hz = pll_hz2;
		}
	}
}
#endif

int sunxi_mmc_set_clk_origin_tm1(struct sunxi_mmc_priv *priv, unsigned int hz,
					unsigned int mhz, unsigned int *fid)
{
	unsigned int mod_hz, freq_id;
#ifdef FPGA_PLATFORM
#ifndef CONFIG_MACH_SUN20IW5
	unsigned int rval;
#endif
#endif
	struct mmc *mmc = priv->mmc;
	u32 val = 0;
	mod_hz = 0;

	/*
	 * The MMC clock has an extra /2 post-divider when operating in the new
	 * mode.
	 */
	if (mmc->speed_mode == HSDDR52_DDR50)
		mod_hz = hz * 4;
	else
		mod_hz = hz * 2;

#if defined(CONFIG_MACH_SUN55IW6)
	unsigned int pll, pll_hz, div, m, n;

	if (mod_hz <= 24000000) {
		pll = CCM_MMC_CTRL_OSCM24;
		pll_hz = 24000000;
	} else {
		sunxi_mmc_get_src_clk_calc(priv->mmc_no, mod_hz, &pll, &pll_hz);
	}

	MMCDBG("pll config :%d : %d\n", pll, pll_hz);

	div = pll_hz / mod_hz;
	if (pll_hz % mod_hz)
		div += 1;
	for (n = 1; n <= 32; n++) {
		for (m = n; m <= 32; m++) {
			if (n * m == div) {
				MMCDBG("div=%d n=%d m=%d\n", div, n, m);
				goto freq_out;
			}
		}
	}
	MMCINFO("%s: wrong clock source,  div=%d n = %d m=%d\n", __func__, div, n, m);
	return -1;
freq_out:
	MMCDBG("Calculate frequency division success! div=%d n=%d m=%d\n", div, n, m);

#elif !defined CONFIG_MACH_SUN55IW3
	unsigned int pll, pll_hz, div, n;

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
#else
#ifdef CONFIG_CLK_SUNXI
	int err = 0;
	u32 rate = 0;
	u32 rate_2 = 0;
	u32 retry_time = 0;
	struct clk *mclk = priv->cfg.clk_mmc;
	struct clk *sclk;
	struct clk *sclk_2;
	u32 src_clk = 0;

	/* hosc */
	sclk = clk_get(NULL, priv->cfg.pll0);
	if (IS_ERR_OR_NULL(sclk)) {
		MMCINFO("Error to get source clock %s\n", priv->cfg.pll0);
		return -1;
	}

	src_clk = clk_get_rate(sclk);
	if (mod_hz > src_clk) {
		clk_put(sclk);
		sclk = clk_get(NULL, priv->cfg.pll1);
	}
	if (IS_ERR_OR_NULL(sclk)) {
		MMCINFO("Error to get source clock %s\n", priv->cfg.pll1);
		return -1;
	}

clk_set_retry:
	err = clk_set_parent(mclk, sclk);
	if (err) {
		MMCINFO("set parent failed\n");
		clk_put(sclk);
		return -1;
	}

	rate = clk_round_rate(mclk, mod_hz);

	MMCDBG("get round rate %d\n", rate);

	if ((rate != mod_hz) && (mod_hz > src_clk) && (retry_time == 0)) {
		sclk_2 = clk_get(NULL, priv->cfg.pll2);
		if (IS_ERR_OR_NULL(sclk_2)) {
			MMCINFO("Error to get source clock pll_periph_another\n");
		} else {
			err = clk_set_parent(mclk, sclk_2);
			if (err) {
				MMCINFO("%s: set parent failed\n", __func__);
				clk_put(sclk_2);
				retry_time++;
				goto clk_set_retry;
			}

			rate_2 = clk_round_rate(mclk, mod_hz);

			MMCDBG("get round rate_2 = %d\n", rate_2);

			if (abs(mod_hz - rate_2) > abs(mod_hz - rate)) {
				MMCDBG("another SourceClk is worse\n");
				clk_put(sclk_2);
				retry_time++;
				goto clk_set_retry;
			} else {
				MMCDBG("another SourceClk is better and choose it\n");
				clk_put(sclk);
				sclk = sclk_2;
				rate = rate_2;
			}
		}
	}

	err = clk_disable(mclk);
	if (err) {
		MMCINFO("disable mmc clk err\n");
		return -1;
	}

	err = clk_set_rate(mclk, rate);
	if (err) {
		MMCINFO("set mclk rate error, rate %dHz\n",
			rate);
		clk_put(sclk);
		return -1;
	}

	err = clk_prepare_enable(mclk);
	if (err) {
		MMCINFO("enable mmc clk err\n");
		return -1;
	}

	src_clk = clk_get_rate(sclk);
	clk_put(sclk);

	MMCDBG("set round clock %d, soure clk is %d, mod_hz is %d\n", rate, src_clk, mod_hz);
#else
	MMCINFO("%s: need ccu config open, set clk = %d\n", __func__, mod_hz);
	return -1;
#endif
#endif

	freq_id = CLK_50M;
	/* determine delays */
	if (hz <= 400000) {
		freq_id = CLK_400K;
	} else if (hz <= 25000000) {
		freq_id = CLK_25M;
	} else if (hz <= 52000000) {
		freq_id = CLK_50M;
	} else if (hz <= 100000000)
		freq_id = CLK_100M;
	else if (hz <= 150000000)
		freq_id = CLK_150M;
	else if (hz <= 200000000)
		freq_id = CLK_200M;
	else {
		/* hz > 52000000 */
		freq_id = CLK_50M;
	}

	MMCDBG("freq_id:%d\n", freq_id);
#if (defined (CONFIG_MACH_SUN8IW7))
	val = 0x1 << 30;//CCM_MMC_CTRL_MODE_SEL_NEW;
#endif
#ifdef FPGA_PLATFORM
#ifndef CONFIG_MACH_SUN20IW5
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
#else
	setbits_le32(&priv->reg->ntsr, SUNXI_MMC_NTSR_MODE_SEL_NEW);
#endif

#ifdef FPGA_PLATFORM
	if (mod_hz > (400000 * 2)) {
		sunxi_r_op(priv, writel(CCM_MMC_CTRL_ENABLE,  priv->mclkreg));
	} else {
#if !defined CONFIG_MACH_SUN55IW3
		sunxi_r_op(priv, writel(pll | CCM_MMC_CTRL_N(n) |
			CCM_MMC_CTRL_M(div) | val, priv->mclkreg));
#endif
	}
	if (hz <= 400000) {
		sunxi_r_op(priv, writel(readl(&priv->reg->drv_dl) & ~(0x1 << 7), &priv->reg->drv_dl));
	} else {
		sunxi_r_op(priv, writel(readl(&priv->reg->drv_dl) | (0x1 << 7), &priv->reg->drv_dl));
	}

#else
#if defined(CONFIG_MACH_SUN55IW6)
	sunxi_r_op(priv, writel(pll | CCM_MMC_CTRL_N(n-1) |
	       CCM_MMC_CTRL_M(m-1) | val, priv->mclkreg));
#elif !defined(CONFIG_MACH_SUN55IW3)
	sunxi_r_op(priv, writel(pll | CCM_MMC_CTRL_N(n) |
	       CCM_MMC_CTRL_M(div) | val, priv->mclkreg));
#endif
#endif
	val = readl(&priv->reg->clkcr);
	val &= ~0xff;
	if (mmc->speed_mode == HSDDR52_DDR50)
		val |= 0x1;
	writel(val, &priv->reg->clkcr);

	priv->tm1.cur_spd_md = mmc->speed_mode;
	priv->tm1.cur_freq = freq_id;

	mmc_config_delay_tm1(priv);


	MMCDBG("mclk reg***%x\n", readl(priv->mclkreg));
	MMCDBG("clkcr reg***%x\n", readl(&priv->reg->clkcr));
#if !defined CONFIG_MACH_SUN55IW3
	MMCDBG("mmc %u set mod-clk req %u parent %u n %u m %u rate %u\n",
	      priv->mmc_no, mod_hz, pll_hz, 1u << n, div, pll_hz / (1u << n) / div);
#endif

	return 0;
}

int sunxi_mmc_set_clk_origin_tm4(struct sunxi_mmc_priv *priv, unsigned int hz,
					unsigned int mhz, unsigned int *fid)
{
	unsigned int mod_hz, freq_id;
	struct mmc *mmc = priv->mmc;
	u32 val = 0;
	mod_hz = 0;

	/*
	 * The MMC clock has an extra /2 post-divider when operating in the new
	 * mode.
	 */
	if ((mmc->speed_mode == HSDDR52_DDR50) && (mmc->bus_width == 8))
		mod_hz = hz * 4;/* 4xclk: DDR8(HS) */
	else
		mod_hz = hz * 2;/* 2xclk: SDR 1/4/8; DDR4(HS); DDR8(HS400) */

#if defined(CONFIG_MACH_SUN55IW6)
	unsigned int pll, pll_hz, div, m, n;

	if (mod_hz <= 24000000) {
		pll = CCM_MMC_CTRL_OSCM24;
		pll_hz = 24000000;
	} else {
		sunxi_mmc_get_src_clk_calc(priv->mmc_no, mod_hz, &pll, &pll_hz);
	}

	MMCDBG("pll config :%d : %d\n", pll, pll_hz);

	div = pll_hz / mod_hz;
	if (pll_hz % mod_hz)
		div += 1;
	for (n = 1; n <= 32; n++) {
		for (m = n; m <= 32; m++) {
			if (n * m == div) {
				MMCDBG("div=%d n=%d m=%d\n", div, n, m);
				goto freq_out;
			}
		}
	}
	MMCINFO("%s: wrong clock source, div=%d n = %d m=%d\n", __func__, div, n, m);
	return -1;
freq_out:
	MMCDBG("Calculate frequency division success! div=%d n=%d m=%d\n", div, n, m);

#elif !defined CONFIG_MACH_SUN55IW3
	unsigned int pll, pll_hz, div, n;

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
#else
#ifdef CONFIG_CLK_SUNXI
	int err = 0;
	u32 rate = 0;
	u32 rate_2 = 0;
	u32 retry_time = 0;
	struct clk *mclk = priv->cfg.clk_mmc;
	struct clk *sclk;
	struct clk *sclk_2;
	u32 src_clk = 0;

	/* hosc */
	sclk = clk_get(NULL, priv->cfg.pll0);
	if (IS_ERR_OR_NULL(sclk)) {
		MMCINFO("Error to get source clock %s\n", priv->cfg.pll0);
		return -1;
	}

	src_clk = clk_get_rate(sclk);
	if (mod_hz > src_clk) {
		clk_put(sclk);
		sclk = clk_get(NULL, priv->cfg.pll1);
	}
	if (IS_ERR_OR_NULL(sclk)) {
		MMCINFO("Error to get source clock %s\n", priv->cfg.pll1);
		return -1;
	}

clk_set_retry:
	err = clk_set_parent(mclk, sclk);
	if (err) {
		MMCINFO("set parent failed\n");
		clk_put(sclk);
		return -1;
	}

	rate = clk_round_rate(mclk, mod_hz);

	MMCDBG("get round rate %d\n", rate);

	if ((rate != mod_hz) && (mod_hz > src_clk) && (retry_time == 0)) {
		sclk_2 = clk_get(NULL, priv->cfg.pll2);
		if (IS_ERR_OR_NULL(sclk_2)) {
			MMCINFO("Error to get source clock pll_periph_another\n");
		} else {
			err = clk_set_parent(mclk, sclk_2);
			if (err) {
				MMCINFO("%s: set parent failed\n", __func__);
				clk_put(sclk_2);
				retry_time++;
				goto clk_set_retry;
			}

			rate_2 = clk_round_rate(mclk, mod_hz);

			MMCDBG("get round rate_2 = %d\n", rate_2);

			if (abs(mod_hz - rate_2) > abs(mod_hz - rate)) {
				MMCDBG("another SourceClk is worse\n");
				clk_put(sclk_2);
				retry_time++;
				goto clk_set_retry;
			} else {
				MMCDBG("another SourceClk is better and choose it\n");
				clk_put(sclk);
				sclk = sclk_2;
				rate = rate_2;
			}
		}
	}

	err = clk_disable(mclk);
	if (err) {
		MMCINFO("disable mmc clk err\n");
		return -1;
	}

	err = clk_set_rate(mclk, rate);
	if (err) {
		MMCINFO("set mclk rate error, rate %dHz\n",
			rate);
		clk_put(sclk);
		return -1;
	}

	err = clk_prepare_enable(mclk);
	if (err) {
		MMCINFO("enable mmc clk err\n");
		return -1;
	}

	src_clk = clk_get_rate(sclk);
	clk_put(sclk);

	MMCDBG("set round clock %d, soure clk is %d, mod_hz is %d\n", rate, src_clk, mod_hz);
#else
	MMCINFO("%s: need ccu config open, set clk = %d\n", __func__, mod_hz);
	return -1;
#endif
#endif
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

	MMCDBG("freq_id:%d\n", freq_id);

/* used for some host which NTSR default use 2x mode */
	if (priv->version >= 0x50300) {
		val = readl(&priv->reg->ntsr);
		val &= ~SUNXI_MMC_NTSR_MODE_SEL_NEW;
		writel(val, &priv->reg->ntsr);
		MMCDBG("Clear NTSR bit 31 to shutdown 2x mode!\n");
	}

#ifdef FPGA_PLATFORM
	if (mod_hz > (400000 * 2)) {
		sunxi_r_op(priv, writel(CCM_MMC_CTRL_ENABLE,  priv->mclkreg));
	} else {
#if !defined CONFIG_MACH_SUN55IW3
		sunxi_r_op(priv, writel(pll | CCM_MMC_CTRL_N(n) |
			CCM_MMC_CTRL_M(div), priv->mclkreg));
#endif
	}
	if (hz <= 400000) {
		sunxi_r_op(priv, writel(readl(&priv->reg->drv_dl) & ~(0x1 << 7), &priv->reg->drv_dl));
	} else {
		sunxi_r_op(priv, writel(readl(&priv->reg->drv_dl) | (0x1 << 7), &priv->reg->drv_dl));
	}

#else
#if defined(CONFIG_MACH_SUN55IW6)
	sunxi_r_op(priv, writel(pll | CCM_MMC_CTRL_N(n-1) |
		CCM_MMC_CTRL_M(m-1), priv->mclkreg));
#elif !defined(CONFIG_MACH_SUN55IW3)
	sunxi_r_op(priv, writel(pll | CCM_MMC_CTRL_N(n) |
		CCM_MMC_CTRL_M(div), priv->mclkreg));
#endif
#endif
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
#if !defined CONFIG_MACH_SUN55IW3
	debug("mmc %u set mod-clk req %u parent %u n %u m %u rate %u\n",
	      priv->mmc_no, mod_hz, pll_hz, 1u << n, div, pll_hz / (1u << n) / div);
#endif

	return 0;
}

__weak int sunxi_mmc_set_clk_common(struct sunxi_mmc_priv *priv, unsigned int hz,
				unsigned int mod_hz, unsigned int *fid)
{
	return 0;
}
__weak int sunxi_mmc_set_clk_type1(struct sunxi_mmc_priv *priv, unsigned int hz,
				unsigned int mod_hz, unsigned int *fid)
{
	return 0;
}
__weak int sunxi_mmc_set_clk_fpga_tm1(struct sunxi_mmc_priv *priv, unsigned int hz,
				unsigned int mod_hz, unsigned int *fid)
{
	return 0;
}
__weak int sunxi_mmc_set_clk_fpga_tm4(struct sunxi_mmc_priv *priv, unsigned int hz,
				unsigned int mod_hz, unsigned int *fid)
{
	return 0;
}
