/*
 * sunxi_drm_crtc.h
 *
 * Copyright (c) 2007-2024 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
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
#ifndef _SUNXI_DRM_CRTC_H
#define _SUNXI_DRM_CRTC_H

#include <dm/ofnode.h>
#include <linux/compat.h>
#include <drm/drm_plane.h>
#include "sunxi_device/hardware/lowlevel_de/de_top.h"
#include "sunxi_drm_connector.h"
#include <wb/de_wb.h>

struct sunxi_drm_wb;
struct sunxi_drm_plane;
struct display_state;


#define WB_SIGNAL_MAX		2

enum sunxi_plane_alpha_mode {
	PIXEL_ALPHA = 0,
	GLOBAL_ALPHA = 1,
	MIXED_ALPHA = 2,
};
#define DEBUG_TIME_SIZE 100
typedef struct {
	unsigned long         sync_time[DEBUG_TIME_SIZE];//for_debug
	unsigned int          sync_time_index;//for_debug
} health_info_t;
struct sunxi_drm_crtc {
	struct sunxi_drm_device *drm;
	const struct sunxi_drm_crtc_funcs *funcs;
	const void *data;
	struct drm_display_mode active_mode;
	struct sunxi_de_out *sunxi_de;
	struct sunxi_drm_plane *plane;
	struct device_node *port;
	unsigned int plane_cnt;
	unsigned int primary_index;
	unsigned int hw_id;
	bool fbdev_output;
	unsigned int fbdev_chn_id;
	bool enabled;
	bool active;
	bool assign_plane : 1;
	unsigned long clk_freq;
	struct sunxi_drm_wb *wb;
	struct sunxi_de_wb *sunxi_wb;
	unsigned int fifo_err;
	unsigned int irqcnt;
	unsigned int fps;
	health_info_t health_info;
};


struct sunxi_drm_plane {
	struct sunxi_drm_device *drm;
	struct drm_plane plane;
	struct de_channel_handle *hdl;
	unsigned int index;
	unsigned int layer_cnt;
	struct sunxi_drm_crtc *crtc;
};


/* wb finish after two vsync, use to signal work finish after vysnc, 2 struct wb_signal_wait is enough */
struct wb_signal_wait {
	bool active;
	unsigned int vsync_cnt;
};


struct eink_wb_para {
	u32                     panel_bits;
	u32			pitch;
	bool			win_calc_en;
	bool			upd_all_en;
	enum upd_mode           upd_mode;
	struct upd_win		upd_win;
	struct upd_pic_size	size;
	enum upd_pixel_fmt      out_fmt;
	enum dither_mode        dither_mode;
	u32 gray_level_cnt;
};

struct sunxi_drm_wb {
	struct sunxi_de_wb *hw_wb;
	struct sunxi_drm_connector wb_connector;
	spinlock_t signal_lock;
	struct wb_signal_wait signal[WB_SIGNAL_MAX];
	enum de_rtwb_mode mode;
	struct eink_wb_para *eink_para;
	struct drm_framebuffer *fb;
	struct drm_framebuffer *eink_last_fb;
};

struct sunxi_drm_crtc_funcs {
	int (*prepare)(struct display_state *state);
	int (*enable)(struct display_state *state);
	int (*disable)(struct display_state *state);
	void (*unprepare)(struct display_state *state);
	void (*flush)(struct display_state *state);
	int (*fixup_dts)(struct display_state *state, void *blob);
	int (*check)(struct display_state *state);
	int (*mode_valid)(struct display_state *state);
	int (*mode_fixup)(struct display_state *state);
	int (*plane_check)(struct display_state *state);
	int (*regs_dump)(struct display_state *state);
	int (*print_state)(struct display_state *state, struct drm_printer *p);
	int (*enable_vblank)(struct display_state *state);
	void (*disable_vblank)(struct display_state *state);
};



int sunxi_drm_crtc_get_hw_id(struct sunxi_drm_crtc *crtc);
struct sunxi_drm_crtc *sunxi_drm_find_crtc_by_port(struct udevice *dev, ofnode node);
int sunxi_drm_set_wb_fb_id(struct display_state *state, int fb_id);

#endif /*End of file*/
