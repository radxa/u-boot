
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

int sunxi_mmc_set_clk_common(struct sunxi_mmc_priv *priv, unsigned int hz,
				unsigned int mod_hz, unsigned int *fid)
{
	unsigned int freq_id;
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
	else
		/* hz > 52000000 */
		freq_id = CLK_50M;

	*fid = freq_id;
	MMCDBG("freq_id:%d\n", freq_id);

	return 0;
}
