/*
 * sunxi_drm_panel.h
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
#ifndef _SUNXI_DRM_PANEL_H
#define _SUNXI_DRM_PANEL_H

#include <drm/drm_modes.h>
#include <drm/drm_connector.h>
#include "sunxi_drm_drv.h"
struct sunxi_drm_panel;
struct sunxi_drm_connector;

struct sunxi_drm_panel_funcs {
	int (*prepare)(struct sunxi_drm_panel *panel);
	int (*unprepare)(struct sunxi_drm_panel *panel);
	int (*enable)(struct sunxi_drm_panel *panel);
	int (*disable)(struct sunxi_drm_panel *panel);
	int (*get_mode)(struct sunxi_drm_panel *panel,
			struct drm_display_mode *mode);
	int (*uboot_reset)(struct sunxi_drm_panel *panel);
};

struct sunxi_drm_panel {
	struct udevice *dev;
	u32 bus_format;
	unsigned int bpc;
	enum drm_panel_orientation orientation;
	const struct sunxi_drm_panel_funcs *funcs;
	const void *data;

	struct sunxi_drm_connector *conn;
	struct display_state *state;
};


static inline void sunxi_drm_panel_init(struct sunxi_drm_panel *panel,
				       struct sunxi_drm_connector *conn,
				       struct display_state *state)
{
	if (!panel)
		return;

	panel->conn = conn;
	panel->state = state;

	drm_connector_set_panel_orientation(&state->conn_state.info, panel->orientation);

	if (panel->bus_format)
		drm_display_info_set_bus_formats(&state->conn_state.info, &panel->bus_format, 1);

	if (panel->bpc)
		state->conn_state.info.bpc = panel->bpc;
}

static inline void sunxi_drm_panel_prepare(struct sunxi_drm_panel *panel)
{
	if (!panel)
		return;

	if (panel->funcs && panel->funcs->prepare)
		panel->funcs->prepare(panel);
}

static inline void sunxi_drm_panel_enable(struct sunxi_drm_panel *panel)
{
	if (!panel)
		return;

	if (panel->funcs && panel->funcs->enable)
		panel->funcs->enable(panel);
}
static inline void sunxi_drm_panel_uboot_reset(struct sunxi_drm_panel *panel)
{
	if (!panel)
		return;

	if (panel->funcs && panel->funcs->uboot_reset)
		panel->funcs->uboot_reset(panel);
}

static inline void sunxi_drm_panel_unprepare(struct sunxi_drm_panel *panel)
{
	if (!panel)
		return;

	if (panel->funcs && panel->funcs->unprepare)
		panel->funcs->unprepare(panel);
}

static inline void sunxi_drm_panel_disable(struct sunxi_drm_panel *panel)
{
	if (!panel)
		return;

	if (panel->funcs && panel->funcs->disable)
		panel->funcs->disable(panel);
}



#endif /*End of file*/
