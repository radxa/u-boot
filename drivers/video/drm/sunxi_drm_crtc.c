/*
 * drivers/video/drm/sunxi_drm_crtc/sunxi_drm_crtc.c
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

#include <common.h>
#include <clk/clk.h>
#include <reset.h>
#include <log.h>
#include <malloc.h>
#include <asm/arch/gic.h>
#include <asm/arch/clock.h>
#include <asm/io.h>
#include <memalign.h>
#include <irq_func.h>
#include <drm/drm_modes.h>
#include <linux/iopoll.h>
#include <drm/drm_connector.h>
#include <drm/drm_framebuffer.h>
#include "sunxi_drm_connector.h"
#include "sunxi_drm_drv.h"
#include "sunxi_drm_crtc.h"
#include <sunxi_de.h>


#define to_sunxi_plane(x)			container_of(x, struct sunxi_drm_plane, plane)


int sunxi_drm_crtc_get_hw_id(struct sunxi_drm_crtc *scrtc)
{
	if (!scrtc) {
		pr_err("crtc is NULL\n");
		return -EINVAL;
	}
	return scrtc->hw_id;
}

int sunxi_plane_disable_plane(struct drm_plane *plane)
{
	struct drm_plane_state *plane_state = plane->state;
	plane_state->fb = NULL;
	plane_state->crtc_x = 0;
	plane_state->crtc_y = 0;
	plane_state->crtc_w = 0;
	plane_state->crtc_h = 0;
	plane_state->src_x = 0;
	plane_state->src_y = 0;
	plane_state->src_w = 0;
	plane_state->src_h = 0;
	//tigger commit
	return 0;
}

static void sunxi_plane_atomic_update(struct drm_plane *plane)

{
	struct drm_plane_state *old_state = plane->old_state;
	struct display_channel_state *old_cstate = to_display_channel_state(old_state);

	struct drm_plane_state *new_state = plane->state;
	struct display_channel_state *new_cstate = to_display_channel_state(new_state);
	struct sunxi_drm_plane *sunxi_plane = to_sunxi_plane(plane);
	struct sunxi_drm_crtc *scrtc = sunxi_plane->crtc;
	struct sunxi_de_channel_update info;

	info.hdl = sunxi_plane->hdl;
	info.hwde = scrtc->sunxi_de;
	info.new_state = new_cstate;
	info.old_state = old_cstate;
	info.is_fbdev = false;
	info.fbdev_output = scrtc->fbdev_output;
	DRM_INFO("[SUNXI-DE] %s %d \n", __FUNCTION__, __LINE__);
	sunxi_de_channel_update(&info);

}

static void commit_new_wb_job(struct sunxi_drm_crtc *scrtc, struct sunxi_drm_wb *wb)
{
	int i;
	unsigned long flags;
	bool found = false;
	struct crtc_state *scrtc_state = NULL;
	struct display_state *s = NULL;

	s = display_state_get_by_crtc(scrtc);
	if (s) {
		scrtc_state = &s->crtc_state;
	}

	if (!scrtc_state) {
		DRM_ERROR("Fail to find crtc state!\n");
		return;
	}


	DRM_INFO("[SUNXI-DE] %s start\n", __FUNCTION__);
	/* find a free signal slot */
	spin_lock_irqsave(&wb->signal_lock, flags);
	for (i = 0; i < WB_SIGNAL_MAX; i++) {
		if (wb->signal[i].active == false) {
			DRM_DEBUG_DRIVER("[SUNXI-DE] set wb for crtc\n");
			wb->signal[i].active = true;
			found = true;
			break;
		}
	}
	spin_unlock_irqrestore(&wb->signal_lock, flags);

	if (!found) {
		DRM_ERROR("no free wb active signal slot\n");
	}

	/* add wb for isr to signal wb job */
	spin_lock_irqsave(&scrtc->wb_lock, flags);
	scrtc_state->wb = NULL;
	scrtc->wb = wb;
	spin_unlock_irqrestore(&scrtc->wb_lock, flags);

	if (wb->mode == SELF_GEN_TIMING_EINK) {
		sunxi_eink_write_back(scrtc->sunxi_de, wb, wb->fb);
	}

	if (wb->mode == TIMING_FROM_TCON) {
		sunxi_de_write_back(scrtc->sunxi_de, wb->hw_wb, wb->fb);
	}
}


static void disable_and_reset_wb(struct sunxi_drm_crtc *scrtc)
{
	bool all_finish = true;
	struct sunxi_drm_wb *wb = scrtc->wb;
	unsigned long flags;
	int i;

	if (wb) {
		/* check if all jobs finsh */
		spin_lock_irqsave(&wb->signal_lock, flags);
		for (i = 0; i < WB_SIGNAL_MAX; i++) {
			if (wb->signal[i].active == true) {
				all_finish = false;
				break;
			}
		}
		spin_unlock_irqrestore(&wb->signal_lock, flags);

		/* disable wb if all jobs finish  */
		if (all_finish) {
			DRM_DEBUG_DRIVER("[SUNXI-DE] rm wb for crtc\n");
			spin_lock_irqsave(&scrtc->wb_lock, flags);
			scrtc->wb = NULL;
			spin_unlock_irqrestore(&scrtc->wb_lock, flags);
			if (wb->mode == SELF_GEN_TIMING_EINK)
				sunxi_eink_wb_stop(scrtc->sunxi_de, wb);
			else
				sunxi_de_write_back(scrtc->sunxi_de, wb->hw_wb, NULL);
		}
	}
}


static void sunxi_wb_commit(struct sunxi_drm_crtc *scrtc)
{
	struct sunxi_drm_wb *wb = NULL;
	struct display_state *s = NULL;
	struct crtc_state *scrtc_state = NULL;

	s = display_state_get_by_crtc(scrtc);
	if (s) {
		scrtc_state = &s->crtc_state;
		wb = scrtc_state->wb;
		if (scrtc_state->wb->fb)
			commit_new_wb_job(scrtc, wb);
		else
			disable_and_reset_wb(scrtc);
	}

}

static int sunxi_plane_update_plane(struct drm_plane *plane,
		    struct sunxi_drm_crtc *crtc, struct drm_framebuffer *fb,
		    int crtc_x, int crtc_y,
		    unsigned int crtc_w, unsigned int crtc_h,
		    uint32_t src_x, uint32_t src_y,
		    uint32_t src_w, uint32_t src_h)
{
	struct drm_plane_state *plane_state = plane->state;
	struct sunxi_drm_plane *sunxi_plane = to_sunxi_plane(plane);
	struct display_channel_state *old_ch_state = NULL;
	struct display_channel_state *ch_state = NULL;
	struct display_state *state;

	if (!plane_state) {
		plane->funcs->reset(plane);
		plane_state = plane->state;
		if (!plane_state) {
			return -ENOMEM;
		}
	}

	if (!plane_state) {
		return -ENOMEM;
	}

	if (sunxi_plane->crtc != crtc) {
		DRM_ERROR("Different crtc with plane!\n");
		return -EINVAL;
	}

	plane_state->fb = fb;
	plane_state->crtc_x = crtc_x;
	plane_state->crtc_y = crtc_y;
	plane_state->crtc_w = crtc_w;
	plane_state->crtc_h = crtc_h;
	plane_state->src_x = src_x;
	plane_state->src_y = src_y;
	plane_state->src_w = src_w;
	plane_state->src_h = src_h;
	plane_state->pixel_blend_mode = DRM_MODE_BLEND_PIXEL_NONE;

	//trigger plane commit
	sunxi_plane_atomic_update(plane);
	state = display_state_get_by_crtc(crtc);
	if (state) {
		crtc->funcs->flush(state);
	}


	ch_state = to_display_channel_state(plane_state);
	old_ch_state = to_display_channel_state(plane->old_state);
	memcpy(old_ch_state, ch_state, sizeof(struct display_channel_state));
	return 0;
}




static void sunxi_plane_destroy_state(struct drm_plane *plane,
					    struct drm_plane_state *state)
{
	struct display_channel_state *cstate = to_display_channel_state(state);

	//TODO:free state fb?
	kfree(cstate);
}

//create new channel state and plane state
static void sunxi_plane_reset(struct drm_plane *plane)
{
	struct display_channel_state *ch_state = NULL;
	struct display_channel_state *old_ch_state = NULL;
	int i;

	//destroy state if exist
	if (plane->state) {
		ch_state = to_display_channel_state(plane->state);
		sunxi_plane_destroy_state(plane, plane->state);
	}

	ch_state = kzalloc(sizeof(*ch_state), GFP_KERNEL);
	if (!ch_state) {
		pr_err("%s fail, no mem\n", __FUNCTION__);
		return;
	}

	ch_state->base.plane = plane;
	ch_state->base.rotation = DRM_MODE_ROTATE_0;
	ch_state->base.alpha = DRM_BLEND_ALPHA_OPAQUE;
	ch_state->base.pixel_blend_mode = DRM_MODE_BLEND_PREMULTI;
	plane->state = &ch_state->base;

	for (i = 0; i < MAX_LAYER_NUM_PER_CHN - 1; i++) {
		ch_state->alpha[i] = DRM_BLEND_ALPHA_OPAQUE;
		ch_state->pixel_blend_mode[i] = DRM_MODE_BLEND_PREMULTI;
	}
	ch_state->eotf = DE_EOTF_BT709;
	ch_state->color_space = DE_COLOR_SPACE_BT709;
	ch_state->color_range = DE_COLOR_RANGE_0_255;
	plane->state = &ch_state->base;

	old_ch_state = kzalloc(sizeof(*old_ch_state), GFP_KERNEL);
	if (!old_ch_state) {
		pr_err("%s fail, no mem\n", __FUNCTION__);
		return;
	}
	memcpy(old_ch_state, ch_state, sizeof(struct display_channel_state));
	plane->old_state = &old_ch_state->base;
}


void sunxi_plane_print_state(struct drm_plane *plane, struct drm_printer *p)
{
	struct sunxi_drm_plane *sunxi_plane = container_of(plane, struct sunxi_drm_plane, plane);
	struct sunxi_drm_crtc *scrtc = sunxi_plane->crtc;
	struct display_channel_state *cstate = to_display_channel_state(plane->state);
	sunxi_de_dump_channel_state(p, scrtc->sunxi_de, sunxi_plane->hdl, cstate, false);
}


static bool sunxi_plane_format_mod_supported(struct drm_plane *plane, u32 format, u64 modifier)
{
	struct sunxi_drm_plane *sunxi_plane = to_sunxi_plane(plane);
	struct sunxi_drm_crtc *scrtc = sunxi_plane->crtc;
	return sunxi_de_format_mod_supported(scrtc->sunxi_de, sunxi_plane->hdl, format, modifier);
}

static const struct drm_plane_funcs sunxi_plane_funcs = {
	.update_plane = sunxi_plane_update_plane,
	.disable_plane = sunxi_plane_disable_plane,
	.destroy = drm_plane_cleanup,
	.reset = sunxi_plane_reset,
	.atomic_destroy_state = sunxi_plane_destroy_state,
	.format_mod_supported = sunxi_plane_format_mod_supported,
	.print_state = sunxi_plane_print_state,
};


static int sunxi_drm_plane_init(struct sunxi_drm_device *dev,
				struct sunxi_drm_crtc *scrtc,
				uint32_t possible_crtc,
				struct sunxi_drm_plane *plane, int type,
				unsigned int de_id, const struct sunxi_plane_info *info)
{
	pr_info("[SUNXI-DE] %s %d \n", __FUNCTION__, __LINE__);
	plane->crtc = scrtc;
	plane->hdl = info->hdl;
	plane->index = info->index;
	plane->layer_cnt = info->layer_cnt;
	char name[40] = {0};

	snprintf(name, 40, "plane-%d-%s(%d)", plane->index, info->name, de_id);

	if (drm_universal_plane_init(dev, &plane->plane, possible_crtc,
				     &sunxi_plane_funcs, info->formats, info->format_count,
				     info->format_modifiers, type, name)) {
		pr_err("drm_universal_plane_init failed\n");
		return -1;
	}

	return 0;
}

struct sunxi_drm_crtc *sunxi_drm_find_crtc_by_port(struct udevice *dev, ofnode node)
{
	int id = -1;

	if (!ofnode_valid(node) || !dev) {
		return NULL;
	}

	if (ofnode_read_s32(node, "reg", &id)) {
		pr_err("%s:fail to read reg node!\n", __func__);
		return NULL;
	}

	return sunxi_de_get_crtc_by_id(dev, id);
}


static int sunxi_crtc_enable(struct display_state *state)
{
	struct crtc_state *scrtc_state = &state->crtc_state;
	struct sunxi_drm_crtc *scrtc = scrtc_state->crtc;
	struct drm_display_mode *modeinfo = &scrtc->active_mode;

	struct sunxi_de_out_cfg cfg;

	if (scrtc->enabled) {
		pr_info("crtc has been enable, no need to enable again\n");
		return 0;
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.sw_enable = false;
	cfg.hwdev_index = scrtc_state->tcon_id;
	cfg.width = modeinfo->hdisplay;
	cfg.height = modeinfo->vdisplay;
	cfg.device_fps = modeinfo->vrefresh;
	cfg.px_fmt_space = scrtc_state->px_fmt_space;
	cfg.yuv_sampling = scrtc_state->yuv_sampling;
	cfg.eotf = scrtc_state->eotf;
	cfg.color_space = scrtc_state->color_space;
	cfg.color_range = scrtc_state->color_range;
	cfg.data_bits = scrtc_state->data_bits;
	if ((scrtc_state->pixel_mode != 0) && (scrtc_state->pixel_mode != 1)
	    && (scrtc_state->pixel_mode != 2) && (scrtc_state->pixel_mode != 4)
	    && (scrtc_state->pixel_mode != 8)) {
		DRM_ERROR("pixel_mode set for crtc is not support:%d, use default 1 pixel mode\n",
			  scrtc_state->pixel_mode);
		cfg.pixel_mode = 1;
	} else {
		cfg.pixel_mode = scrtc_state->pixel_mode;
	}

	if (sunxi_de_enable(scrtc->sunxi_de, &cfg) < 0)
		pr_err("sunxi_de_enable failed\n");

	scrtc->enabled = true;
	return 0;
}


static int sunxi_crtc_disable(struct display_state *state)

{
	unsigned long flags;
	struct crtc_state *scrtc_state = &state->crtc_state;
	struct sunxi_drm_crtc *scrtc = scrtc_state->crtc;
	struct sunxi_drm_wb *wb = NULL;

	if (!scrtc->enabled) {
		pr_err("%s: crtc has been disabled\n", __func__);
		return 0;
	}

	/* remove not finish wb  */
	spin_lock_irqsave(&scrtc->wb_lock, flags);
	wb = scrtc->wb ? scrtc->wb : NULL;
	scrtc->wb = NULL;
	spin_unlock_irqrestore(&scrtc->wb_lock, flags);

	if (wb) {
		if (wb->mode == SELF_GEN_TIMING_EINK)
			sunxi_eink_wb_stop(scrtc->sunxi_de, wb);
		else
			sunxi_de_write_back(scrtc->sunxi_de, wb->hw_wb, NULL);
	}

	scrtc->enabled = false;
	sunxi_de_disable(scrtc->sunxi_de);
	return 0;
}


static void wb_finish_proc(struct sunxi_drm_crtc *scrtc)
{
	int i;
	struct sunxi_drm_wb *wb;
	unsigned long flags;
	struct wb_signal_wait *wait;
	bool signal = false;

	spin_lock_irqsave(&scrtc->wb_lock, flags);
	wb = scrtc->wb;
	spin_unlock_irqrestore(&scrtc->wb_lock, flags);
	if (!wb) {
		return;
	}

	spin_lock_irqsave(&wb->signal_lock, flags);
	for (i = 0; i < WB_SIGNAL_MAX; i++) {
		wait = &wb->signal[i];
		if (wait->active) {
			wait->vsync_cnt++;
			if (wait->vsync_cnt == 2) {
				wait->active = 0;
				wait->vsync_cnt = 0;
				signal = true;
			}
		}
	}

	if (signal == true) {
		DRM_INFO("wb finish !%d\n", signal);
	}

	spin_unlock_irqrestore(&wb->signal_lock, flags);
}

int sunxi_drm_get_fps(struct sunxi_drm_crtc *scrtc)
{
	u32 pre_time_index, cur_time_index;
	u32 pre_time, cur_time;

	pre_time_index = scrtc->health_info.sync_time_index;
	cur_time_index = (pre_time_index == 0) ? (DEBUG_TIME_SIZE - 1) : (pre_time_index - 1);

	pre_time = scrtc->health_info.sync_time[pre_time_index];
	cur_time = scrtc->health_info.sync_time[cur_time_index];

	if (pre_time != cur_time) {
		scrtc->fps = 10000 * DEBUG_TIME_SIZE / (cur_time - pre_time);
	}

	return 0;

}

static void sunxi_drm_sync_checkin(struct sunxi_drm_crtc *scrtc)
{
	u32 index = scrtc->health_info.sync_time_index;
	scrtc->health_info.sync_time[index] = get_timer_masked();/*msecond*/
	index++;
	index = (index >= DEBUG_TIME_SIZE) ? 0 : index;
	scrtc->health_info.sync_time_index = index;
}

static void sunxi_crtc_event_proc(void *parg)
{
	int ret = 0;
	struct display_state *state = (struct display_state *)parg;
	struct crtc_state *scrtc_state = &state->crtc_state;
	struct sunxi_drm_crtc *scrtc = scrtc_state->crtc;

	scrtc->irqcnt++;
	sunxi_drm_sync_checkin(scrtc);
	if (scrtc_state->check_status(scrtc_state->vblank_enable_data))
		scrtc->fifo_err++;

	ret = sunxi_de_event_proc(scrtc->sunxi_de);
	if (ret < 0) {
		pr_err("sunxi_de_event_proc FAILED!\n");
		goto out;
	}

out:
	wb_finish_proc(scrtc);
	//sunxi_crtc_finish_page_flip(crtc->dev, scrtc);
	return;
}

static int sunxi_drm_crtc_check(struct display_state *state)
{
	struct crtc_state *scrtc_state = &state->crtc_state;
	/*struct sunxi_drm_crtc *scrtc = scrtc_state->crtc;*/

	scrtc_state->crtc_irq_handler = sunxi_crtc_event_proc;
	return 0;
}

static int sunxi_crtc_prepare(struct display_state *dstate)
{
	struct crtc_state *state = &dstate->crtc_state;
	struct sunxi_drm_crtc *scrtc = state->crtc;

	state->px_fmt_space = DE_FORMAT_SPACE_RGB;
	state->yuv_sampling = DE_YUV444;
	state->eotf = DE_EOTF_BT709;
	state->color_space = DE_COLOR_SPACE_BT709;
	state->color_range = DE_COLOR_RANGE_0_255;
	state->data_bits = DE_DATA_8BITS;
	state->clk_freq = scrtc->clk_freq;
	state->excfg.brightness = 50;
	state->excfg.contrast = 50;
	state->excfg.saturation = 50;
	state->excfg.hue = 50;
	return 0;
}


static void sunxi_crtc_flush(struct display_state *state)
{
	struct crtc_state *scrtc_state = &state->crtc_state;
	struct sunxi_drm_crtc *scrtc = scrtc_state->crtc;
	struct sunxi_de_flush_cfg cfg;
	bool all_dirty = true;

	memset(&cfg, 0, sizeof(cfg));

	sunxi_wb_commit(scrtc);

	if (all_dirty || scrtc_state->bcsh_changed) {
		cfg.brightness = scrtc_state->excfg.brightness;
		cfg.contrast = scrtc_state->excfg.contrast;
		cfg.saturation = scrtc_state->excfg.saturation;
		cfg.hue = scrtc_state->excfg.hue;
		cfg.bcsh_dirty = true;
		cfg.gamma_lut = scrtc_state->excfg.gamma_lut;
		cfg.gamma_dirty = true;
	}
	sunxi_de_atomic_flush(scrtc->sunxi_de, &cfg);
}


static int sunxi_crtc_print_state(struct display_state *state, struct drm_printer *p)
{
	unsigned long flags;
	struct sunxi_drm_wb *wb;
	struct crtc_state *cstate = &state->crtc_state;
	struct connector_state *conn_state = &state->conn_state;
	struct sunxi_drm_crtc *scrtc = (struct sunxi_drm_crtc *)cstate->crtc;
	int w = conn_state->mode.hdisplay;
	int h = conn_state->mode.vdisplay;
//	int fps = drm_mode_vrefresh(&conn_state->mode);

	drm_printf(p, "\n\t%s: ", scrtc->enabled ? "on" : "off\n");
	if (scrtc->enabled) {
		sunxi_drm_get_fps(scrtc);
		drm_printf(p, "%dx%d@%d.%d&%dMhz->tcon%d irqcnt=%d err=%d\n", w, h, scrtc->fps / 10, scrtc->fps % 10,
			    (int)(scrtc->clk_freq / 1000000), cstate->tcon_id, scrtc->irqcnt, scrtc->fifo_err);
		drm_printf(p, "\t    format_space: %d yuv_sampling: %d eotf:%d cs: %d"
			    " color_range: %d data_bits: %d\n", cstate->px_fmt_space,
			    cstate->yuv_sampling, cstate->eotf, cstate->color_space,
			    cstate->color_range, cstate->data_bits);

		spin_lock_irqsave(&scrtc->wb_lock, flags);
		wb = scrtc->wb;
		if (!wb) {
			drm_printf(p, "\twb off\n");
		} else {
			drm_printf(p, "\twb on:\n\t\t[0]: %s %d\n\t\t[1]: %s %d\n",
				    wb->signal[0].active ? "waiting" : "finish", wb->signal[0].vsync_cnt,
				    wb->signal[1].active ? "waiting" : "finish", wb->signal[1].vsync_cnt);
		}
		spin_unlock_irqrestore(&scrtc->wb_lock, flags);
	}
	return 0;
}


static int sunxi_drm_crtc_enable_vblank(struct display_state *state)
{
	struct crtc_state *scrtc_state = &state->crtc_state;

	DRM_DEBUG_DRIVER("%s\n", __func__);
	if (scrtc_state->enable_vblank == NULL) {
		DRM_ERROR("enable vblank is not registerd!\n");
		return -1;
	}
	scrtc_state->enable_vblank(true, scrtc_state->vblank_enable_data);
	return 0;
}

static void sunxi_drm_crtc_disable_vblank(struct display_state *state)
{
	struct crtc_state *scrtc_state = &state->crtc_state;

	DRM_DEBUG_DRIVER("%s\n", __func__);
	if (scrtc_state->enable_vblank == NULL) {
		DRM_ERROR("enable vblank is not registerd!\n");
		return;
	}
	scrtc_state->enable_vblank(false, scrtc_state->vblank_enable_data);
}

static const struct sunxi_drm_crtc_funcs sunxi_crtc_helper_funcs = {
	.enable = sunxi_crtc_enable,
	.disable = sunxi_crtc_disable,
	.check = sunxi_drm_crtc_check,
	.flush = sunxi_crtc_flush,
	.prepare = sunxi_crtc_prepare,
	.print_state = sunxi_crtc_print_state,
	.enable_vblank = sunxi_drm_crtc_enable_vblank,
	.disable_vblank = sunxi_drm_crtc_disable_vblank,
};


/**
 * drm_crtc_index - find the index of a registered CRTC
 * @crtc: CRTC to find index for
 *
 * Given a registered CRTC, return the index of that CRTC within a DRM
 * device's list of CRTCs.
 */
static inline u32 sunxi_drm_crtc_index(const struct sunxi_drm_crtc *scrtc)
{
	return scrtc->hw_id;
}


static inline u32 sunxi_drm_crtc_mask(const struct sunxi_drm_crtc *scrtc)
{
	return 1 << sunxi_drm_crtc_index(scrtc);
}


static int sunxi_wb_connector_check(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	struct crtc_state *scrtc_state = &state->crtc_state;
	struct sunxi_drm_wb *wb =
		container_of(conn,
			     struct sunxi_drm_wb, wb_connector);
	int i = 0, ret = 0;

	unsigned long flags;
	if (!scrtc_state->crtc->enabled) {
		pr_err("[SUNXI-DE] wb check fail, crtc is not enabled %s %d \n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	spin_lock_irqsave(&wb->signal_lock, flags);
	for (i = 0; i < WB_SIGNAL_MAX; i++) {
		if (wb->signal[i].active) {
			ret = -EBUSY;
			pr_err("[SUNXI-DE] wb check fail, pending wb not finish %s %d \n", __FUNCTION__, __LINE__);
		}
	}
	spin_unlock_irqrestore(&wb->signal_lock, flags);

	scrtc_state->wb = wb;
	//check if any display has been enabled
	return ret;
}

int sunxi_drm_set_wb_fb_id(struct display_state *state, int fb_id)
{
	struct sunxi_drm_connector *conn = NULL;
	struct sunxi_drm_wb *wb = NULL;
	struct crtc_state *crtc_state = &state->crtc_state;
	const struct sunxi_drm_crtc_funcs *crtc_funcs = crtc_state->crtc->funcs;
	int finish_flag = 0;
	bool timeout = false;


	conn = get_sunxi_drm_connector_by_type(state->drm, DRM_MODE_CONNECTOR_VIRTUAL);
	if (!conn) {
		DRM_ERROR("Get wb connector fail!\n");
		return -1;
	}

	wb = container_of(conn, struct sunxi_drm_wb, wb_connector);
	if (!wb) {
		DRM_ERROR("Get wb fail!\n");
		return -3;
	}

	if (fb_id == -1 && wb->fb) {
		drm_framebuffer_free(state->drm, wb->fb);
	} else {
		//TODO:free fb or store fb to last fb
		wb->fb = drm_framebuffer_lookup(state->drm, fb_id);
		if (!wb->fb) {
			DRM_ERROR("Find fb %d fail!\n", fb_id);
			return -2;
		}
		//only enable writeback need to be flush
		if (crtc_funcs->flush) {
			crtc_funcs->flush(state);
		}

		timeout =
			read_poll_timeout(sunxi_de_get_wb_status, finish_flag,
					  !finish_flag, 20000, 100000, wb);
		if (timeout) {
			DRM_ERROR("Writeback timeout!\n");
		}
	}


	return 0;
}

static const struct sunxi_drm_connector_funcs sunxi_wb_connector_funcs = {
	.check = sunxi_wb_connector_check,
};


struct sunxi_drm_wb *sunxi_drm_wb_init_one(struct sunxi_de_wb_info *wb_info)
{
	int ret;
	struct sunxi_drm_wb *wb = kmalloc(sizeof(struct sunxi_drm_wb), GFP_KERNEL | __GFP_ZERO);
	/*static const u32 formats_wb[] = {*/
		/*DRM_FORMAT_ARGB8888,//TODO add more format base on hw feat*/
	/*};*/

	if (!wb) {
		DRM_ERROR("allocate memory for drm_wb fail\n");
		return ERR_PTR(-ENOMEM);
	}
	wb->hw_wb = wb_info->wb;
	spin_lock_init(&wb->signal_lock);

	ret = sunxi_drm_connector_bind(&wb->wb_connector, wb_info->dev, 0, &sunxi_wb_connector_funcs,
				 NULL, DRM_MODE_CONNECTOR_VIRTUAL);

	if (ret) {
		DRM_ERROR("writeback connector init failed\n");
		return NULL;
	} else
		return wb;
}

struct sunxi_drm_crtc *sunxi_drm_crtc_init_one(struct sunxi_de_info *info,
			struct sunxi_de_wb_info *wb_info)
{
	struct sunxi_drm_crtc *scrtc;
	const struct sunxi_plane_info *plane = NULL;
	int i, ret;
	int primary_cnt = 0;

	pr_info("[SUNXI-DE] %s %d xxxxxx\n", __FUNCTION__, __LINE__);
	scrtc = kmalloc(sizeof(*scrtc), GFP_KERNEL | __GFP_ZERO);
	if (!scrtc) {
		pr_err("allocate memory for sunxi_crtc fail\n");
		return ERR_PTR(-ENOMEM);
	}
	scrtc->sunxi_de = info->de_out;
	scrtc->hw_id = info->hw_id;
	scrtc->plane_cnt = info->plane_cnt;//channel count
	scrtc->port = info->port;
	scrtc->clk_freq = info->clk_freq;
	scrtc->sunxi_wb = wb_info->wb;
	scrtc->drm = sunxi_drm_device_get();
	scrtc->plane =
		kmalloc(sizeof(*scrtc->plane) * info->plane_cnt,
			     GFP_KERNEL | __GFP_ZERO);
	if (!scrtc->plane) {
		pr_err("allocate mem for planes fail\n");
		return ERR_PTR(-ENOMEM);
	}

	for (i = 0; i < info->plane_cnt; i++) {
		if (info->planes[i].is_primary) {
			plane = &info->planes[i];
			scrtc->primary_index = i;
			primary_cnt++;
		}
	}

	if (!plane || primary_cnt > 1) {
		pr_err("primary plane for de %d cfg err cnt %d\n", info->hw_id, primary_cnt);
		goto err_out;
	}

	/* create primary plane for crtc */
	ret = sunxi_drm_plane_init(scrtc->drm, scrtc, 0, &scrtc->plane[scrtc->primary_index],
				   DRM_PLANE_TYPE_PRIMARY, info->hw_id,
				   plane);
	if (ret) {
		pr_err("plane init fail for de %d\n", info->hw_id);
		goto err_out;
	}

	/* create overlay planes with remain channels for the specified crtc */
	for (i = 0; i < info->plane_cnt; i++) {
		plane = &info->planes[i];
		if (plane->is_primary)
			continue;
		ret = sunxi_drm_plane_init(scrtc->drm, scrtc, sunxi_drm_crtc_mask(scrtc),
				     &scrtc->plane[i], DRM_PLANE_TYPE_OVERLAY,
				     info->hw_id, plane);
		if (ret) {
			pr_err("sunxi plane init for %d fail\n", i);
			goto err_out;
		}
	}
	scrtc->funcs = &sunxi_crtc_helper_funcs;

	return scrtc;

err_out:
	for (i = 0; i < scrtc->plane_cnt; i++) {
		if (scrtc->plane[i].plane.drm)
			drm_plane_cleanup(&scrtc->plane[i].plane);
	}
	return ERR_PTR(-EINVAL);
}


//End of File
