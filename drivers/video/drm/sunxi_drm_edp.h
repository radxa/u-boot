/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * drv_edp2.h
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __DRV_EDP_H__
#define __DRV_EDP_H__

#include "sunxi_device/sunxi_edp.h"
#include <asm-generic/int-ll64.h>
#include <dm/device.h>
#include <drm/drm_print.h>

#include "sunxi_device/sunxi_tcon.h"
#include "sunxi_drm_helper_funcs.h"
#include "sunxi_drm_panel.h"
#include "sunxi_drm_crtc.h"

#define CEA_BASIC_AUDIO_MASK     (1 << 6) /* Version3 */
#define CEA_YCC444_MASK	         (1 << 5) /* Version3 */
#define CEA_YCC422_MASK	         (1 << 4) /* Version3 */

struct edp_debug {
	unsigned long aux_i2c_addr;
	unsigned long aux_i2c_len;
	unsigned long aux_read_start;
	unsigned long aux_read_end;
	u32 aux_write_start;
	u32 aux_write_len;
	u32 aux_write_val[16];
	u32 aux_write_val_before[16];
	u32 lane_debug_en;
	u32 hpd_mask;
	u32 hpd_mask_pre;

	/* resource lock, power&clk won't release when edp enable fail if not 0 */
	u32 edp_res_lock;

	/* bypass training for some signal test case */
	u32 bypass_training;
};

struct sunxi_drm_edp {
	struct sunxi_drm_connector connector;
	struct drm_display_mode mode;
	unsigned int tcon_id;
	unsigned int tcon_top_id;
	struct drm_panel *panel;
	struct task_struct *edp_task;
	bool bound;

	u32 enable;
	u32 irq;
	ulong base_addr;
	ulong top_addr;
	struct udevice *dev;
	struct udevice *tcon_dev;
	struct phy *dp_phy;
	struct phy *aux_phy;
	struct phy *combo_phy;
	struct clk *clk_bus;
	struct clk *clk;
	struct clk *clk_24m;
	u32 use_inner_clk;
#ifdef DRM_USE_DM_POWER
	struct udevice *vdd_regulator;
	struct udevice *vcc_regulator;
#else
	uint32_t vdd_regulator;
	uint32_t vcc_regulator;
#endif
	struct sunxi_edp_output_desc *desc;

	/* drm property */
	struct drm_property *colorspace_property;

	bool hpd_state;
	bool hpd_state_now;
	bool dpcd_parsed;
	bool use_dpcd;
	/*FIXME:TODO: optimize relate code*/
	bool fps_limit_60;
	/*end FIXME*/
	bool use_debug_para;
	bool is_enabled;

	bool sw_enable;
	bool allow_sw_enable;

	struct edp_tx_core edp_core;
	struct sunxi_edp_hw_desc edp_hw;
	struct edp_tx_cap source_cap;
	struct edp_rx_cap sink_cap;
	struct edp_debug edp_debug;
	struct sunxi_dp_hdcp hdcp;
};

struct sunxi_edp_connector_state {
	int color_format;
	int color_depth;
	int lane_cnt;
	int lane_rate;
};

#define to_drm_edp_connector_state(s) container_of(s, struct sunxi_edp_connector_state, state)

struct sunxi_edp_output_desc {
	int connector_type;
	s32 (*bind)(struct sunxi_drm_edp *drm_edp);
	s32 (*unbind)(struct sunxi_drm_edp *drm_edp);
	s32 (*enable_early)(struct sunxi_drm_edp *drm_edp);
	s32 (*enable)(struct sunxi_drm_edp *drm_edp);
	s32 (*disable)(struct sunxi_drm_edp *drm_edp);
	s32 (*plugin)(struct sunxi_drm_edp *drm_edp);
	s32 (*plugout)(struct sunxi_drm_edp *drm_edp);
	s32 (*runtime_suspend)(struct sunxi_drm_edp *drm_edp);
	s32 (*runtime_resume)(struct sunxi_drm_edp *drm_edp);
	s32 (*suspend)(struct sunxi_drm_edp *drm_edp);
	s32 (*resume)(struct sunxi_drm_edp *drm_edp);
	void (*soft_reset)(struct sunxi_drm_edp *drm_edp);
};

#endif /*End of file*/
