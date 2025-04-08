/*
 * drivers/video/drm/sunxi_drm_connector/sunxi_drm_connector.c
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
#include <dm.h>
#include <linux/compat.h>
#include <drm/drm_modes.h>
#include <drm/drm_print.h>
#include <drm/drm_connector.h>
#include "sunxi_drm_bridge.h"
#include "sunxi_drm_panel.h"
#include "sunxi_drm_connector.h"
#include "sunxi_drm_drv.h"


int sunxi_drm_connector_bind(struct sunxi_drm_connector *conn, struct udevice *dev, int id,
			    const struct sunxi_drm_connector_funcs *funcs, void *data, int type)
{

	struct sunxi_drm_device *drm = sunxi_drm_device_get();

	conn->id = id;
	conn->dev = dev;
	conn->funcs = funcs;
	conn->data = data;
	conn->type = type;
	conn->drm = drm;
	INIT_LIST_HEAD(&conn->probed_modes);
	list_add_tail(&conn->head, &drm->connector_list);
	++conn->drm->num_connector;

	return 0;
}


static bool sunxi_drm_connector_path_detect(struct sunxi_drm_connector *conn,
					   struct display_state *state)
{
	int ret;

	if (conn->funcs->detect) {
		ret = conn->funcs->detect(conn, state);
		if (!ret) {
			DRM_ERROR("%s disconnected\n", conn->dev->name);
			return false;
		}
	}

	if (conn->bridge) {
		ret = sunxi_drm_bridge_detect(conn->bridge);
		if (!ret) {
			DRM_ERROR("%s disconnected\n",
			       dev_np(conn->bridge->dev)->full_name);
			return false;
		}
	}

	return true;
}


bool sunxi_drm_connector_detect(struct display_state *state)
{
	bool ret;
	struct sunxi_drm_connector *conn;

	conn = state->conn_state.connector;
	ret = sunxi_drm_connector_path_detect(conn, state);
	if (!ret)
		return false;
	if (state->conn_state.secondary) {
		conn = state->conn_state.secondary;
		ret = sunxi_drm_connector_path_detect(conn, state);
		if (!ret)
			return false;
	}

	return true;
}
static int sunxi_drm_connector_path_save_para(struct sunxi_drm_connector *conn,
					      struct display_state *state)
{
	if (conn->funcs->save_kernel_para)
		conn->funcs->save_kernel_para(conn, state);

	return 0;
}

int sunxi_drm_connector_save_para(struct display_state *state)
{
	struct sunxi_drm_connector *conn;

	conn = state->conn_state.connector;
	sunxi_drm_connector_path_save_para(conn, state);

	if (state->conn_state.secondary) {
		conn = state->conn_state.secondary;
		sunxi_drm_connector_path_save_para(conn, state);
	}

	return 0;
}

static int sunxi_drm_connector_path_pre_enable(struct sunxi_drm_connector *conn,
					      struct display_state *state)
{
	if (conn->funcs->prepare)
		conn->funcs->prepare(conn, state);

	if (conn->bridge)
		sunxi_drm_bridge_pre_enable(conn->bridge);

	if (conn->panel)
		sunxi_drm_panel_prepare(conn->panel);

	return 0;
}

int sunxi_drm_connector_pre_enable(struct display_state *state)
{
	struct sunxi_drm_connector *conn;

	conn = state->conn_state.connector;
	sunxi_drm_connector_path_pre_enable(conn, state);

	if (state->conn_state.secondary) {
		conn = state->conn_state.secondary;
		sunxi_drm_connector_path_pre_enable(conn, state);
	}

	return 0;
}

int sunxi_drm_connector_pre_init(struct display_state *state)
{
	int ret = 0;
	struct sunxi_drm_connector *conn;

	conn = state->conn_state.connector;
	if (conn->funcs->pre_init) {
		ret = conn->funcs->pre_init(conn, state);
		if (ret)
			return ret;
		if (state->conn_state.secondary) {
			conn = state->conn_state.connector;
			ret = conn->funcs->pre_init(conn, state);
			if (ret)
				return ret;
		}
	}

	return ret;
}


static int sunxi_drm_connector_path_init(struct sunxi_drm_connector *conn,
					struct display_state *state)
{
	int ret = 0;

	if (conn->panel)
		sunxi_drm_panel_init(conn->panel, conn, state);

	if (conn->bridge)
		sunxi_drm_bridge_init(conn->bridge, conn, state);

	if (conn->funcs->init) {
		ret = conn->funcs->init(conn, state);
		if (ret)
			return ret;
	}

	return ret;
}

int sunxi_drm_connector_init(struct display_state *state)
{
	int ret = 0;
	struct sunxi_drm_connector *conn;

	conn = state->conn_state.connector;
	ret = sunxi_drm_connector_path_init(conn, state);
	if (ret)
		return ret;
	if (state->conn_state.secondary) {
		conn = state->conn_state.secondary;
		ret = sunxi_drm_connector_path_init(conn, state);
		if (ret)
			return ret;
	}

	return ret;
}


int sunxi_drm_connector_get_timing(struct display_state *state)
{
	int ret = 0;
	struct sunxi_drm_connector *conn;

	conn = state->conn_state.connector;
	if (conn->funcs->get_timing) {
		ret = conn->funcs->get_timing(conn, state);
		if (ret)
			return ret;
		if (state->conn_state.secondary) {
			conn = state->conn_state.secondary;
			ret = conn->funcs->get_timing(conn, state);
			if (ret)
				return ret;
		}
	}

	return ret;
}

int sunxi_drm_connector_get_edid(struct display_state *state)
{
	int ret = 0;
	struct sunxi_drm_connector *conn;

	conn = state->conn_state.connector;
	if (conn->funcs->get_edid) {
		ret = conn->funcs->get_edid(conn, state);
		if (ret)
			return ret;
		if (state->conn_state.secondary) {
			conn = state->conn_state.secondary;
			ret = conn->funcs->get_edid(conn, state);
			if (ret)
				return ret;
		}
	}

	return ret;
}

struct sunxi_drm_connector *
get_sunxi_drm_connector_by_type(struct sunxi_drm_device *drm, unsigned int type)
{
	struct sunxi_drm_connector *conn;

	list_for_each_entry(conn, &drm->connector_list, head) {
		if (conn->type == type)
			return conn;
	}

	return NULL;
}

struct sunxi_drm_connector *
get_sunxi_drm_connector_by_device(struct sunxi_drm_device *drm,
				  struct udevice *dev)
{
	struct sunxi_drm_connector *conn;

	list_for_each_entry(conn, &drm->connector_list, head) {
		if (conn->dev == dev)
			return conn;
	}

	return NULL;
}


static int sunxi_drm_connector_path_post_disable(struct sunxi_drm_connector *conn,
						struct display_state *state)
{
	if (conn->panel)
		sunxi_drm_panel_unprepare(conn->panel);

	if (conn->bridge)
		sunxi_drm_bridge_post_disable(conn->bridge);

	if (conn->funcs->unprepare)
		conn->funcs->unprepare(conn, state);

	return 0;
}


int sunxi_drm_connector_post_disable(struct display_state *state)
{
	struct sunxi_drm_connector *conn;

	conn = state->conn_state.connector;
	sunxi_drm_connector_path_post_disable(conn, state);
	if (state->conn_state.secondary) {
		conn = state->conn_state.secondary;
		sunxi_drm_connector_path_post_disable(conn, state);
	}

	return 0;
}

static int sunxi_drm_panel_path_post_disable(struct sunxi_drm_connector *conn,
						struct display_state *state)
{
	if (conn->panel)
		sunxi_drm_panel_uboot_reset(conn->panel);

	return 0;
}

int sunxi_drm_panel_post_disable(struct display_state *state)
{
	struct sunxi_drm_connector *conn;

	conn = state->conn_state.connector;
	sunxi_drm_panel_path_post_disable(conn, state);
	if (state->conn_state.secondary) {
		conn = state->conn_state.secondary;
		sunxi_drm_panel_path_post_disable(conn, state);
	}

	return 0;
}

static int sunxi_drm_connector_path_enable(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	if (conn->funcs->enable)
		conn->funcs->enable(conn, state);

	if (conn->bridge)
		sunxi_drm_bridge_enable(conn->bridge);

	if (conn->panel && state->backlight)
		sunxi_drm_panel_enable(conn->panel);

	return 0;
}

int sunxi_drm_connector_enable(struct display_state *state)
{
	struct sunxi_drm_connector *conn;

	conn = state->conn_state.connector;
	sunxi_drm_connector_path_enable(conn, state);
	if (state->conn_state.secondary) {
		conn = state->conn_state.secondary;
		sunxi_drm_connector_path_enable(conn, state);
	}

	return 0;
}
static int sunxi_drm_connector_path_backlight(struct sunxi_drm_connector *conn,
					   struct display_state *state, bool flag)
{
	if (conn->panel) {
		if (flag)
			sunxi_drm_panel_enable(conn->panel);
		else
			sunxi_drm_panel_disable(conn->panel);
	}

	return 0;
}

int sunxi_drm_connector_backlight(struct display_state *state, bool flag)
{
	struct sunxi_drm_connector *conn;

	conn = state->conn_state.connector;
	sunxi_drm_connector_path_backlight(conn, state, flag);
	if (state->conn_state.secondary) {
		conn = state->conn_state.secondary;
		sunxi_drm_connector_path_backlight(conn, state, flag);
	}

	return 0;
}

static int sunxi_drm_connector_path_disable(struct sunxi_drm_connector *conn,
					   struct display_state *state)
{
	if (conn->panel)
		sunxi_drm_panel_disable(conn->panel);

	if (conn->bridge)
		sunxi_drm_bridge_disable(conn->bridge);

	if (conn->funcs->disable)
		conn->funcs->disable(conn, state);

	return 0;
}

int sunxi_drm_connector_disable(struct display_state *state)
{
	struct sunxi_drm_connector *conn;

	conn = state->conn_state.connector;
	sunxi_drm_connector_path_disable(conn, state);
	if (state->conn_state.secondary) {
		conn = state->conn_state.secondary;
		sunxi_drm_connector_path_disable(conn, state);
	}

	return 0;
}


int sunxi_drm_connector_deinit(struct display_state *state)
{
	struct sunxi_drm_connector *conn;

	conn = state->conn_state.connector;
	if (conn->funcs->deinit) {
		conn->funcs->deinit(conn, state);
		if (state->conn_state.secondary) {
			conn = state->conn_state.secondary;
			conn->funcs->deinit(conn, state);
		}
	}

	return 0;
}

static int sunxi_drm_connector_print(struct sunxi_drm_connector *conn,
					      struct display_state *state)
{
	if (conn->funcs->print_conn)
		conn->funcs->print_conn(conn, state);

	return 0;
}

int sunxi_connector_print_state(struct display_state *state)
{
	struct sunxi_drm_connector *conn;

	conn = state->conn_state.connector;
	sunxi_drm_connector_print(conn, state);

	if (state->conn_state.secondary) {
		conn = state->conn_state.secondary;
		sunxi_drm_connector_print(conn, state);
	}

	return 0;
}

int drm_mode_to_sunxi_video_timings(struct drm_display_mode *mode,
				    struct disp_video_timings *timings)
{
	if (!mode) {
		DRM_ERROR("drm mode invalid!\n");
		return -1;
	}

	if (!timings) {
		DRM_ERROR("sunxi video timings invalid!\n");
		return -1;
	}

	//TODO:implement drm_match_cea_mode?
	/*timings->vic = drm_match_cea_mode(mode);*/
	timings->vic = 0;
	timings->pixel_clk = mode->clock * 1000;
	if (mode->clock < 27000)
		timings->pixel_repeat = 1;
	else
		timings->pixel_repeat = (mode->flags & DRM_MODE_FLAG_DBLCLK) ? 0x1 : 0x0;

	timings->b_interlace = mode->flags & DRM_MODE_FLAG_INTERLACE;
	timings->x_res = mode->hdisplay;
	timings->y_res = mode->vdisplay;
	timings->hor_total_time = mode->htotal;
	timings->hor_back_porch = mode->htotal - mode->hsync_end;
	timings->hor_front_porch = mode->hsync_start - mode->hdisplay;
	timings->hor_sync_time = mode->hsync_end - mode->hsync_start;
	timings->hor_sync_polarity = (mode->flags & DRM_MODE_FLAG_PHSYNC) ? 1 : 0;

	timings->ver_total_time = mode->vtotal;
	timings->ver_back_porch = (mode->vtotal - mode->vsync_end)
		/ (timings->b_interlace + 1);
	timings->ver_front_porch = (mode->vsync_start - mode->vdisplay)
		/ (timings->b_interlace + 1);
	timings->ver_sync_time = (mode->vsync_end - mode->vsync_start)
		/ (timings->b_interlace + 1);
	timings->ver_sync_polarity = (mode->flags & DRM_MODE_FLAG_PVSYNC) ? 1 : 0;

	return 0;
}


void drm_mode_probed_add(struct sunxi_drm_connector *connector,
			 struct drm_display_mode *mode)
{
	list_add_tail(&mode->head, &connector->probed_modes);
}

//End of File
