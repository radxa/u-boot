/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/* sunxi_tcon.h
 *
 * Copyright (C) 2023 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _SUNXI_TCON_H_
#define _SUNXI_TCON_H_

#include <asm/arch/gic.h>
#include "hardware/include.h"
#include "tcon_lcd.h"
#include "tcon_tv.h"
#include "tcon_top.h"
#include "dsi_v1.h"


enum disp_interface_type {
	INTERFACE_LCD = 0,
	INTERFACE_DSI = 1,
	INTERFACE_HDMI = 2,
	INTERFACE_TVE = 3,
	INTERFACE_EDP = 4,
	INTERFACE_LVDS = 5,
	INTERFACE_RGB = 6,
	INTERFACE_INVALID,
};


struct disp_output_config {
	enum disp_interface_type type;
	unsigned int de_id;
	bool sw_enable;
	bool slave_dsi;
	bool displl_clk;
	unsigned int tcon_lcd_div;
	unsigned int pixel_mode;
	struct disp_dsi_para dsi_para;
	struct disp_lvds_para lvds_para;
	struct disp_rgb_para rgb_para;
	struct disp_video_timings timing;
	enum disp_csc_type format;
	interrupt_handler_t *irq_handler;
	void *irq_data;
	void *private_data;
};

struct tcon_device {
	struct udevice *dev;
	unsigned int hw_id;
	struct disp_output_config cfg;
};

enum tcon_builin_pattern {
	PATTERN_DE = 0,
	PATTERN_COLORBAR,
	PATTERN_GRAYSCALE,
	PATTERN_BLACK_WHITE,
	PATTERN_ALL0,
	PATTERN_ALL1,
	PATTERN_RESERVED,
	PATTERN_GRIDDING,
};

bool sunxi_tcon_check_fifo_status(struct udevice *tcon_dev);
int sunxi_lvds_enable_output(struct udevice *tcon_dev);
int sunxi_lvds_disable_output(struct udevice *tcon_dev);
int sunxi_rgb_enable_output(struct udevice *tcon_dev);
int sunxi_rgb_disable_output(struct udevice *tcon_dev);
int sunxi_tcon_show_pattern(struct udevice *tcon_dev, int pattern);
int sunxi_tcon_pattern_get(struct udevice *tcon_dev);
int sunxi_tcon_of_get_id(struct udevice *tcon_dev);
int sunxi_tcon_of_get_top_id(struct udevice *tcon_dev);
void sunxi_tcon_enable_vblank(struct udevice *tcon_dev, bool enable);
int sunxi_tcon_hdmi_open(struct udevice *dev);
int sunxi_tcon_hdmi_src(struct udevice *dev, u8 src);
int sunxi_tcon_mode_init(struct udevice *tcon_dev, struct disp_output_config *disp_cfg);
int sunxi_tcon_mode_exit(struct udevice *tcon_dev);
int sunxi_tcon_enable_output(struct udevice *dev);
struct udevice *sunxi_tcon_of_get_tcon_dev(struct udevice *remote_dev);
int sunxi_tcon_print(struct udevice *tcon_dev);

#endif
