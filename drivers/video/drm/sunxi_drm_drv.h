/*
 * sunxi_drm_drv.h
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
#ifndef _SUNXI_DRM_DRV_H
#define _SUNXI_DRM_DRV_H

#include <common.h>
#include <drm/drm_connector.h>
#include <dm/ofnode.h>
#include <asm/arch/gic.h>
#include "sunxi_device/hardware/lowlevel_de/de_base.h"
#include "load_file.h"
#include <edid.h>

struct list_head;
struct connector_state;
struct drm_display_mode;
/**
 * description here
 */
struct sunxi_drm_device {
	struct udevice *dev;  //sunxi drm top device
	struct list_head display_list;
	/**
	 * @num_display:
	 *
	 * Number of CRTCs on this device linked with &drm_crtc.head. This is invariant over the lifetime
	 * of a device and hence doesn't need any locks.
	 */
	int num_display;

	/**
	 * @connector_list:
	 *
	 * List of connector objects linked with &drm_connector.head. Protected
	 * by @connector_list_lock. Only use drm_for_each_connector_iter() and
	 * &struct drm_connector_list_iter to walk this list.
	 */
	struct list_head connector_list;
	/**
	 * @num_connector: Number of connectors on this device. Protected by
	 * @connector_list_lock.
	 */
	int num_connector;

	/**
	 * @plane_list:
	 *
	 * List of plane objects linked with &drm_plane.head. This is invariant
	 * over the lifetime of a device and hence doesn't need any locks.
	 */
	struct list_head plane_list;
	/**
	 * @num_total_plane:
	 *
	 * Number of universal (i.e. with primary/curso) planes on this device.
	 * This is invariant over the lifetime of a device and hence doesn't
	 * need any locks.
	 */
	int num_total_plane;

	struct list_head fb_list;
	/** @num_fb: Number of entries on @fb_list. */
	int num_fb;
};


typedef void (*vblank_enable_callback_t)(bool, void *);


enum sunxi_drm_debug_mode {
	SKIP_DRM_DRIVE = 1,
	DEBUG_TIMING = 2,
};


struct de_out_exconfig {
	struct drm_color_lut *gamma_lut;
	unsigned int brightness, contrast, saturation, hue;
};

typedef bool (*fifo_status_check_callback_t)(void *);

struct crtc_state {
	struct udevice *dev;
	struct sunxi_drm_crtc *crtc;
	void *private;
	ofnode node;
	struct device_node *ports_node;
	int crtc_id;
	bool force_output;
	struct drm_display_mode force_mode;
	enum de_format_space px_fmt_space;
	enum de_yuv_sampling yuv_sampling;
	enum de_eotf eotf;
	enum de_color_space color_space;
	enum de_color_range color_range;
	enum de_data_bits data_bits;
	unsigned int tcon_id;
	unsigned int tcon_top_id;
	unsigned long clk_freq;
	unsigned int pixel_mode;
	vblank_enable_callback_t enable_vblank;
	void *vblank_enable_data;
	fifo_status_check_callback_t check_status;
	struct de_out_exconfig excfg;
	bool bcsh_changed;
	interrupt_handler_t *crtc_irq_handler;
	struct sunxi_drm_wb *wb;
};


struct panel_state {
	struct sunxi_drm_panel *panel;

	ofnode dsp_lut_node;
};

struct overscan {
	int left_margin;
	int right_margin;
	int top_margin;
	int bottom_margin;
};


struct connector_state {
	struct udevice *tcon_dev;
	struct sunxi_drm_connector *connector;
	struct sunxi_drm_connector *secondary;
	struct sunxi_drm_connector *online_wb_conn;
	struct drm_display_mode mode;
	struct overscan overscan;
	u8 edid[EDID_SIZE * 4];
	struct drm_display_info info;
	int output_mode;
	int type;
	int output_if;
};




struct display_state {
	struct sunxi_drm_device *drm;
	struct list_head head;
	ofnode node;
	struct crtc_state crtc_state;
	struct connector_state conn_state;
	struct panel_state panel_state;
	bool force_output;
	struct drm_display_mode force_mode;
	struct file_info_t *logo;
	int fb_id;
	char ulogo_name[30];
	bool is_init;
	bool is_enable;
	bool backlight;
	bool is_card_mass;
};


struct public_phy_data {
	const struct sunxi_drm_phy *phy_drv;
	int phy_node;
	int public_phy_type;
	bool phy_init;
};


struct sunxi_drm_device *sunxi_drm_device_get(void);

struct sunxi_drm_crtc *sunxi_drm_crtc_find(struct sunxi_drm_device *drm, int crtc_id);
struct display_state *display_state_get_by_crtc(struct sunxi_drm_crtc *crtc);

#define sunxi_drm_for_each_display(state, drm) \
	list_for_each_entry(state, &(drm)->display_list, head)

#endif /*End of file*/
