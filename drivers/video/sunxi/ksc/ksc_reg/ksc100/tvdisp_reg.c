// SPDX-License-Identifier: GPL-2.0
/*
 * tvdisp_reg/tvdisp_reg.c
 *
 * Copyright (c) 2007-2024 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/list.h>
#include <linux/types.h>
#include <linux/kref.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/of.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <video/sunxi_ksc.h>
#include <linux/of_address.h>
#include <linux/dma-buf.h>
#include <linux/of_irq.h>
#include <linux/dma-mapping.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <linux/device/class.h>
#include "tvdisp_reg.h"

#define TVDISP_KSC_ENABLE_OFFSET 0xa0

int tvdisp_ksc_enable(void *tvdisp_reg, bool enable)
{
	u8 __iomem *reg_base;

	if (!tvdisp_reg) {
		return -1;
	}

	reg_base = (u8 __iomem *)tvdisp_reg + TVDISP_KSC_ENABLE_OFFSET;
	if (enable) {
		writel(0xf, reg_base);
	} else {
		writel(0, reg_base);
	}

	return 0;
}
//End of File
