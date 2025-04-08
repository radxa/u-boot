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

/*
 * applicable that uboot does nont provide clk interfaces,
 * and the clock source follows the following pattern:
 * sdmmc2 soruce clock:
 * 00:24M
 * 01:PERI0_800M
 * 02:PERI0_600M
 * 03:PERI1_800M
 * 04:PERI1_600M
 * sdmmc0 soruce clock:
 * 00:24M
 * 01:PERI0_400M
 * 02:PERI0_300M
 * 03:PERI1_400M
 * 04:PERI1_300M
 * */

#ifndef SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET
#define SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET 24
#endif
#ifndef SMHC2_CLK_REG_CLK_SRC_SEL_OFFSET
#define SMHC2_CLK_REG_CLK_SRC_SEL_OFFSET 24
#endif
#ifndef CCM_MMC_CTRL_PERI0_400M_freq
#define CCM_MMC_CTRL_PERI0_400M_freq         (400000000)
#endif
#ifndef CCM_MMC_CTRL_PERI0_300M_freq
#define CCM_MMC_CTRL_PERI0_300M_freq         (300000000)
#endif
#ifndef CCM_MMC_CTRL_PERI0_800M_freq
#define CCM_MMC_CTRL_PERI0_800M_freq         (800000000)
#endif
#ifndef CCM_MMC_CTRL_PERI0_600M_freq
#define CCM_MMC_CTRL_PERI0_600M_freq         (600000000)
#endif
#ifndef CCM_MMC_CTRL_PERI0_400M
#define CCM_MMC_CTRL_PERI0_400M         (0x1 << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#endif
#ifndef CCM_MMC_CTRL_PERI0_300M
#define CCM_MMC_CTRL_PERI0_300M         (0x2 << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#endif
#ifndef CCM_MMC_CTRL_PERI0_800M
#define CCM_MMC_CTRL_PERI0_800M         (0x1 << SMHC2_CLK_REG_CLK_SRC_SEL_OFFSET)
#endif
#ifndef CCM_MMC_CTRL_PERI0_600M
#define CCM_MMC_CTRL_PERI0_600M         (0x2 << SMHC2_CLK_REG_CLK_SRC_SEL_OFFSET)
#endif
#ifndef CCM_MMC_CTRL_OSCM24
#define CCM_MMC_CTRL_OSCM24		(0x0 << 24)
#endif

static void sunxi_mmc_get_src_clk_calc(int sdc_no, int mod_hz,
					unsigned int *pll, unsigned int *pll_hz)
{
	unsigned int pll1, pll_hz1, pll2, pll_hz2;

	if (mod_hz <= 24000000) {
		*pll = CCM_MMC_CTRL_OSCM24;
		*pll_hz = 24000000;
	} else {
		if (sdc_no == 2) {
#if defined(CONFIG_MACH_SUN300IW1)
		MMCINFO("%s: the sdc_no is error!!!\n", __func__);
#else
		pll1 = CCM_MMC_CTRL_PERI0_800M;
		pll_hz1 = CCM_MMC_CTRL_PERI0_800M_freq;
		pll2 = CCM_MMC_CTRL_PERI0_600M;
		pll_hz2 = CCM_MMC_CTRL_PERI0_600M_freq;
#endif
		} else {
#if defined(CONFIG_MACH_SUN300IW1)
			pll1 = SMHC_CTRL0_CLK_SEL_PERI_192M_OFFSET;
			pll_hz1 = SMHC_CTRL0_CLK_SEL_PERI_192M_freq;
			pll2 = SMHC_CTRL0_CLK_SEL_PERI_219M_OFFSET;
			pll_hz2 = SMHC_CTRL0_CLK_SEL_PERI_219M_freq;
#else
			pll1 = CCM_MMC_CTRL_PERI0_400M;
			pll_hz1 = CCM_MMC_CTRL_PERI0_400M_freq;
			pll2 = CCM_MMC_CTRL_PERI0_300M;
			pll_hz2 = CCM_MMC_CTRL_PERI0_300M_freq;
#endif
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

int sunxi_mmc_set_clk_type1(struct sunxi_mmc_priv *priv, unsigned int hz,
				unsigned int mod_hz, unsigned int *fid)
{
	unsigned int freq_id;
	u32 val = 0;

	unsigned int pll, pll_hz, div, m, n;

	sunxi_mmc_get_src_clk_calc(priv->mmc_no, mod_hz, &pll, &pll_hz);

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
	*fid = freq_id;
	MMCDBG("freq_id:%d\n", freq_id);

	sunxi_r_op(priv, writel(pll | CCM_MMC_CTRL_N(n-1) |
	       CCM_MMC_CTRL_M(m-1) | val, priv->mclkreg));

	MMCDBG("mmc %u set mod-clk req %u parent %u n %u m %u rate %u\n",
	      priv->mmc_no, mod_hz, pll_hz, 1u << n, div, pll_hz / (1u << n) / div);

	return 0;
}
