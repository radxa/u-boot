/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/* sunxi_tcon_v35x.c
 *
 * Copyright (C) 2023 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <common.h>
#include <dm.h>
#include <clk/clk.h>
#include <reset.h>
#include <log.h>
#include <malloc.h>
#include <asm/arch/gic.h>
#include <asm/arch/clock.h>
#include <asm/io.h>
#include <memalign.h>
#include <drm/drm_print.h>

#include "sunxi_tcon_top.h"
#include "../sunxi_drm_helper_funcs.h"

struct tcon_top_data {
	unsigned int id;
};

struct tcon_top {
	uintptr_t reg_base;
	struct clk *clk_dpss;
	struct clk *clk_ahb;
	struct clk *clk_ahb_gate;
	const struct tcon_top_data *top_data;
};


static int sunxi_tcon_top_probe(struct udevice *dev)
{
	struct tcon_top *top = dev_get_priv(dev);

	DRM_INFO("%s:start\n", __func__);
	top->top_data = (struct tcon_top_data *)dev_get_driver_data(dev);
	if (!top->top_data) {
		DRM_ERROR("sunxi_tcon_top fail to get match data\n");
		return -ENODEV;
	}

	top->reg_base = (uintptr_t)dev_read_addr_ptr(dev);
	if (!top->reg_base) {
		DRM_ERROR("unable to map tcon top registers\n");
		return -EINVAL;
	}

	top->clk_dpss = clk_get_by_name(dev, "clk_bus_dpss_top");
	if (IS_ERR(top->clk_dpss)) {
		DRM_ERROR("fail to get clk dpss_top\n");
	}

	top->clk_ahb = clk_get_by_name(dev, "clk_ahb");

	if (IS_ERR(top->clk_ahb)) {
		DRM_INFO("fail to get clk ahb %ld\n", PTR_ERR(top->clk_ahb));
	}

	top->clk_ahb_gate = clk_get_by_name(dev, "clk_ahb_gate");

	if (IS_ERR(top->clk_ahb_gate)) {
		DRM_INFO("fail to get clk ahb gate\n");
	}

	tcon_top_set_reg_base(top->top_data->id, top->reg_base);

	of_periph_clk_config_setup(ofnode_to_offset(dev_ofnode(dev)));
	DRM_INFO("%s:end\n", __func__);
	return 0;
}


/* Note: sunxi-lcd is represented of sunxi tcon,
 * using lcd is order to be same with sunxi display driver
 */

static const struct tcon_top_data top0_data = {
	.id = 0,
};

static const struct tcon_top_data top1_data = {
	.id = 1,
};

static const struct udevice_id sunxi_tcon_top_match[] = {

	{ .compatible = "allwinner,tcon-top0", .data = (ulong)&top0_data },
	{ .compatible = "allwinner,tcon-top1", .data = (ulong)&top1_data },
	{},
};


U_BOOT_DRIVER(tcon_top) = {
	.name = "tcon_top",
	.id = UCLASS_MISC,
	.of_match = sunxi_tcon_top_match,
	.probe = sunxi_tcon_top_probe,
	.priv_auto_alloc_size = sizeof(struct tcon_top),
};


static bool sunxi_tcon_top_node_is_tcon_top(ofnode node)
{
	int i = 0;

	for (i = 0; sunxi_tcon_top_match[i].compatible; i++) {
		if (ofnode_device_is_compatible(node, sunxi_tcon_top_match[i].compatible))
			return true;
	}
	return false;
}

int sunxi_tcon_top_get_id(struct udevice *tcon_top)
{
	const struct tcon_top_data *match_data = (const struct tcon_top_data *)tcon_top->driver->of_match->data;

	if (!sunxi_tcon_top_node_is_tcon_top(dev_ofnode(tcon_top))) {
		DRM_ERROR("Device is not TCON TOP!\n");
		return -EINVAL;
	}

	if (!match_data) {
		DRM_ERROR("Device match_data is null!\n");
		return -EINVAL;
	}

	return match_data->id;
}

int sunxi_tcon_top_clk_enable(struct udevice *tcon_top)
{
	struct tcon_top *topif = dev_get_priv(tcon_top);
	struct clk *clks[] = {topif->clk_dpss, topif->clk_ahb, topif->clk_ahb_gate};

	if (!sunxi_tcon_top_node_is_tcon_top(dev_ofnode(tcon_top))) {
		DRM_ERROR("Device is not TCON TOP!\n");
		return -EINVAL;
	}

	return sunxi_clk_enable(clks, ARRAY_SIZE(clks));
}

int sunxi_tcon_top_clk_disable(struct udevice *tcon_top)
{
	struct tcon_top *topif = dev_get_priv(tcon_top);
	struct clk *clks[] = {topif->clk_dpss, topif->clk_ahb, topif->clk_ahb_gate};

	if (!sunxi_tcon_top_node_is_tcon_top(dev_ofnode(tcon_top))) {
		DRM_ERROR("Device is not TCON TOP!\n");
		return -EINVAL;
	}

	return sunxi_clk_disable(clks, ARRAY_SIZE(clks));
}
