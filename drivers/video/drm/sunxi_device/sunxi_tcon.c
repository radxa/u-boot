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
#include <dm/of_access.h>
#include <clk/clk.h>
#include <reset.h>
#include <log.h>
#include <malloc.h>
#include <asm/arch/gic.h>
#include <asm/arch/clock.h>
#include <asm/io.h>
#include <memalign.h>
#include <linux/math64.h>
#include <drm/drm_print.h>


#include "sunxi_tcon.h"
#include "sunxi_tcon_top.h"
#include "../sunxi_drm_helper_funcs.h"

enum tcon_type {
	TCON_LCD = 0,
	TCON_TV = 1,
};

struct sunxi_tcon {
	int id;
	bool is_enabled;
	struct udevice *dev;
	struct udevice *tcon_top;
	struct phy *lvds_combo_phy0;
	struct phy *lvds_combo_phy1;
	struct tcon_device tcon_ctrl;
	struct sunxi_tcon_tv tcon_tv;
	struct sunxi_tcon_lcd tcon_lcd;

	uintptr_t reg_base;
	enum tcon_type type;

	/* clock resource */
	struct clk *ahb_clk; /* module clk */
	struct clk *mclk; /* module clk */
	struct clk *mclk_bus; /* module clk bus */
	struct clk *rst_bus_tcon; /* module clk rst */

	/* interrupt resource */
	unsigned int irq_no;
	/* judge_line for start delay, used to judge if there is enough time
	 *to update and sync DE register
	 */
	unsigned int judge_line;
	void *output_data;

	void *irq_data;
	interrupt_handler_t *irq_handler;
};

static void sunxi_tcon_dsi_calc_judge_line(struct sunxi_tcon *hwtcon,
					   struct disp_dsi_para *dsi_para);
static int sunxi_tcon_request_irq(struct sunxi_tcon *hwtcon);
static int sunxi_tcon_free_irq(struct sunxi_tcon *hwtcon);

static const struct udevice_id sunxi_tcon_match[] = {
	{ .compatible = "allwinner,tcon-lcd",},
	{ .compatible = "allwinner,tcon-tv",},
	{},
};

static enum tcon_type get_dev_tcon_type(ofnode node)
{
	if (ofnode_device_is_compatible(node, "allwinner,tcon-lcd"))
		return TCON_LCD;
	if (ofnode_device_is_compatible(node, "allwinner,tcon-tv"))
		return TCON_TV;

	DRM_ERROR("invalid tcon match compatible\n");
	return TCON_LCD;
}


//remember top put tcon_top
static int sunxi_tcon_get_tcon_top(struct sunxi_tcon *tcon)
{
	ofnode topnode;
	struct udevice *top_dev = NULL;
	u32 phandle = -1;

	ofnode_read_u32(dev_ofnode(tcon->dev), "top", &phandle);
	if (phandle < 0) {
		DRM_ERROR("%s:Get top phandle fail!\n", __func__);
		return -1;
	}
	topnode = ofnode_get_by_phandle(phandle);
	if (!ofnode_valid(topnode)) {
		DRM_ERROR("%s:Invalid top node\n", __func__);
		return -1;
	}

	uclass_get_device_by_ofnode(UCLASS_MISC,
					  topnode,
					  &top_dev);
	if (top_dev == NULL) {
		DRM_ERROR("%s:Get top udevice fail!\n", __func__);
		return -1;
	}

	tcon->tcon_top = top_dev;
	return 0;
}



static int sunxi_tcon_lcd_set_clk(struct sunxi_tcon *hwtcon, unsigned long pixel_clk)
{
	unsigned long tcon_rate, tcon_rate_set;
	struct clk *clks[] = {hwtcon->mclk, hwtcon->mclk_bus};

	if (!hwtcon->tcon_ctrl.cfg.sw_enable && pixel_clk) {
		tcon_rate = pixel_clk;
		clk_set_rate(hwtcon->mclk, tcon_rate);
		tcon_rate_set = clk_get_rate(hwtcon->mclk);
		if (tcon_rate_set != tcon_rate)
			DRM_INFO("tcon rate to be set:%luHz, real clk rate:%luHz\n", tcon_rate,
				 tcon_rate_set);
	}

	return sunxi_clk_enable(clks, ARRAY_SIZE(clks));
}


static int sunxi_tcon_lcd_prepare(struct sunxi_tcon *hwtcon, unsigned long pixel_clk)
{
	int ret = 0;
	if (hwtcon->tcon_top)
		sunxi_tcon_top_clk_enable(hwtcon->tcon_top);
	ret = sunxi_tcon_lcd_set_clk(hwtcon, pixel_clk);
	if (ret < 0) {
		DRM_ERROR("sunxi_tcon_lcd_set_clk failed\n");
		return -1;
	}

	return 0;
}

static void sunxi_tcon_lcd_unprepare(struct sunxi_tcon *hwtcon)
{
	struct clk *clks[] = {hwtcon->mclk, hwtcon->mclk_bus};

	sunxi_clk_disable(clks, ARRAY_SIZE(clks));

	if (hwtcon->tcon_top)
		sunxi_tcon_top_clk_disable(hwtcon->tcon_top);
}

int sunxi_tcon_enable_output(struct udevice *dev)
{
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);
	struct disp_dsi_para *dsi_para = &hwtcon->tcon_ctrl.cfg.dsi_para;
	return tcon_dsi_open(&hwtcon->tcon_lcd, dsi_para);
}

static int sunxi_tcon_dsi_mode_init(struct udevice *dev)
{
	int ret = 0;
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);
	struct disp_dsi_para *dsi_para = &hwtcon->tcon_ctrl.cfg.dsi_para;
	bool sw_enable = hwtcon->tcon_ctrl.cfg.sw_enable;
	bool displl_clk = hwtcon->tcon_ctrl.cfg.displl_clk;
	unsigned int tcon_div = hwtcon->tcon_ctrl.cfg.tcon_lcd_div;

	DRM_INFO("[DSI] %s start\n", __FUNCTION__);
	if (displl_clk)
		ret = sunxi_tcon_lcd_prepare(hwtcon, 0);
	else
		ret = sunxi_tcon_lcd_prepare(hwtcon, dsi_para->timings.pixel_clk * tcon_div);
	if (ret < 0) {
		DRM_ERROR("sunxi_tcon_lcd_prepare failed\n");
		return ret;
	}
	if (!sw_enable) {
		tcon_lcd_init(&hwtcon->tcon_lcd);
		tcon_lcd_set_dclk_div(&hwtcon->tcon_lcd, tcon_div);
		if (displl_clk)
			tcon_lcd_dsi_clk_source(hwtcon->id, dsi_para->dual_dsi);
		if (tcon_dsi_cfg(&hwtcon->tcon_lcd, dsi_para) != 0) { /* the final param is rcq related */
			DRM_ERROR("lcd cfg fail!\n");
			return -1;
		}
//		tcon0_cfg_ext(&hwtcon->tcon_lcd, p_panel_ext);
		tcon_lcd_src_select(&hwtcon->tcon_lcd, LCD_SRC_DE);
		sunxi_tcon_dsi_calc_judge_line(hwtcon, dsi_para);
		/*tcon_dsi_open(&hwtcon->tcon_lcd, dsi_para);*/
	}

	if (hwtcon->tcon_ctrl.cfg.slave_dsi)
		sunxi_tcon_request_irq(hwtcon);

	hwtcon->is_enabled = true;

	return 0;
}

static int sunxi_tcon_dsi_mode_exit(struct udevice *dev)
{
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);

	if (hwtcon->tcon_ctrl.cfg.slave_dsi)
		sunxi_tcon_free_irq(hwtcon);

	hwtcon->is_enabled = false;
	hwtcon->judge_line = 0;

	tcon_dsi_close(&hwtcon->tcon_lcd);
	tcon_lcd_exit(&hwtcon->tcon_lcd);
	sunxi_tcon_lcd_unprepare(hwtcon);
	return 0;
}


int sunxi_lvds_enable_output(struct udevice *dev)
{
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);
	struct disp_lvds_para *lvds_para = &hwtcon->tcon_ctrl.cfg.lvds_para;

	tcon_lvds_open(&hwtcon->tcon_lcd);
	lvds_open(&hwtcon->tcon_lcd, lvds_para);
	return 0;
}

int sunxi_lvds_disable_output(struct udevice *dev)
{
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);

	tcon_lvds_close(&hwtcon->tcon_lcd);
	lvds_close(&hwtcon->tcon_lcd);

	return 0;
}

static int sunxi_tcon_lvds_mode_init(struct udevice *dev)
{
	int ret = 0;
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);
	struct tcon_device *tcon = &hwtcon->tcon_ctrl;
	struct disp_lvds_para *lvds_para = &tcon->cfg.lvds_para;
	unsigned int tcon_div = hwtcon->tcon_ctrl.cfg.tcon_lcd_div;
	bool displl_clk = hwtcon->tcon_ctrl.cfg.displl_clk;

	DRM_INFO("[LVDS] %s start\n", __FUNCTION__);
	if (displl_clk)
		ret = sunxi_tcon_lcd_prepare(hwtcon, 0);
	else
		ret = sunxi_tcon_lcd_prepare(hwtcon, lvds_para->timings.pixel_clk * tcon_div);
	if (ret < 0) {
		DRM_ERROR("sunxi_tcon_lcd_prepare failed\n");
		return ret;
	}
	if (!tcon->cfg.sw_enable) {
		tcon_lcd_init(&hwtcon->tcon_lcd);
		tcon_lcd_set_dclk_div(&hwtcon->tcon_lcd, tcon_div);
		if (displl_clk)
			tcon_lcd_dsi_clk_source(hwtcon->id, 0);
		if (tcon_lvds_cfg(&hwtcon->tcon_lcd, lvds_para) != 0) { /* the final param is rcq related */
			DRM_ERROR("lcd cfg fail!\n");
			return -1;
		}
		tcon_lcd_src_select(&hwtcon->tcon_lcd, LCD_SRC_DE);
	}

	sunxi_tcon_request_irq(hwtcon);

	hwtcon->is_enabled = true;

	return 0;
}

static int sunxi_tcon_lvds_mode_exit(struct udevice *dev)
{
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);

	hwtcon->is_enabled = false;
	hwtcon->judge_line = 0;

	tcon_lcd_exit(&hwtcon->tcon_lcd);
	sunxi_tcon_lcd_unprepare(hwtcon);

	return 0;
}


int sunxi_rgb_enable_output(struct udevice *tcon_dev)
{
	struct sunxi_tcon *hwtcon = dev_get_priv(tcon_dev);

	tcon_rgb_open(&hwtcon->tcon_lcd);

	return 0;
}

int sunxi_rgb_disable_output(struct udevice *tcon_dev)
{
	struct sunxi_tcon *hwtcon = dev_get_priv(tcon_dev);

	tcon_rgb_close(&hwtcon->tcon_lcd);

	return 0;
}

static int sunxi_tcon_rgb_mode_init(struct udevice *dev)
{
	int ret = 0;
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);
	struct tcon_device *tcon = &hwtcon->tcon_ctrl;
	struct disp_rgb_para *rgb_para = &tcon->cfg.rgb_para;
	unsigned int tcon_div = hwtcon->tcon_ctrl.cfg.tcon_lcd_div;
	bool displl_clk = hwtcon->tcon_ctrl.cfg.displl_clk;

	DRM_INFO("[RGB] %s start\n", __FUNCTION__);
	if (displl_clk)
		ret = sunxi_tcon_lcd_prepare(hwtcon, 0);
	else
		ret = sunxi_tcon_lcd_prepare(hwtcon, rgb_para->timings.pixel_clk * tcon_div);
	if (ret < 0) {
		DRM_ERROR("sunxi_tcon_lcd_prepare failed\n");
		return ret;
	}
	if (!tcon->cfg.sw_enable) {
		tcon_lcd_init(&hwtcon->tcon_lcd);
		tcon_lcd_set_dclk_div(&hwtcon->tcon_lcd, tcon_div);
		if (displl_clk)
			tcon_lcd_dsi_clk_source(hwtcon->id, 0);
		if (tcon_rgb_cfg(&hwtcon->tcon_lcd, rgb_para) != 0) {
			DRM_ERROR("lcd-rgb cfg fail!\n");
			return -1;
		}
		tcon_lcd_src_select(&hwtcon->tcon_lcd, LCD_SRC_DE);
	}

	sunxi_tcon_request_irq(hwtcon);

	hwtcon->is_enabled = true;

	return 0;
}

static int sunxi_tcon_rgb_mode_exit(struct udevice *dev)
{
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);

	hwtcon->is_enabled = false;
	hwtcon->judge_line = 0;

	sunxi_tcon_free_irq(hwtcon);

	tcon_lcd_exit(&hwtcon->tcon_lcd);
	sunxi_tcon_lcd_unprepare(hwtcon);

	return 0;
}

static void sunxi_tcon_dsi_calc_judge_line(struct sunxi_tcon *hwtcon,
					   struct disp_dsi_para *dsi_para)
{
	unsigned int usec_per_line, start_delay;
	unsigned int usec_start_delay, usec_judge_point;

	usec_per_line = dsi_para->timings.hor_total_time / (dsi_para->timings.pixel_clk / 1000000);
	start_delay = tcon_lcd_get_start_delay(&hwtcon->tcon_lcd);
	usec_start_delay = start_delay * usec_per_line;

	if (usec_start_delay <= 200)
		usec_judge_point = usec_start_delay * 3 / 7;
	else if (usec_start_delay <= 400)
		usec_judge_point = usec_start_delay / 2;
	else
		usec_judge_point = 200;

	hwtcon->judge_line = usec_judge_point / usec_per_line;
}

static int sunxi_tcon_device_query_irq(struct sunxi_tcon *hwtcon)
{
	int ret = -1;

	if (hwtcon->type == TCON_LCD)
		ret = tcon_lcd_irq_query(&hwtcon->tcon_lcd, LCD_IRQ_TCON0_VBLK);
	else
		ret = tcon_tv_irq_query(&hwtcon->tcon_tv, LCD_IRQ_TCON1_VBLK);

	return ret;
}

static void sunxi_tcon_irq_event_proc(void *parg)
{
	struct sunxi_tcon *hwtcon = parg;
	sunxi_tcon_device_query_irq(hwtcon);

	return hwtcon->irq_handler(hwtcon->irq_data);
}

static int sunxi_tcon_request_irq(struct sunxi_tcon *hwtcon)
{
	hwtcon->irq_handler = hwtcon->tcon_ctrl.cfg.irq_handler;
	hwtcon->irq_data = hwtcon->tcon_ctrl.cfg.irq_data;

	irq_install_handler(hwtcon->irq_no, sunxi_tcon_irq_event_proc, (void *)hwtcon);
	irq_enable(hwtcon->irq_no);

	return 0;
}

static int sunxi_tcon_free_irq(struct sunxi_tcon *hwtcon)
{
	if (hwtcon->irq_data != hwtcon->tcon_ctrl.cfg.irq_data) {
		DRM_ERROR("Couldn't free the IRQ for tcon\n");
		return -EINVAL;
	}
	irq_free_handler(hwtcon->irq_no);
	return 0;
}

/*******************************************************************************
 * @desc: suxni tcon tv for hdmi api, referred from sunxi display2
 ******************************************************************************/
static void _sunxi_tcon_hdmi_calc_judge_line(struct sunxi_tcon *hwtcon,
		struct disp_video_timings *p_timgs)
{
	unsigned long long usec_per_line, start_delay;
	unsigned int usec_start_delay, usec_judge_point;

	usec_per_line = p_timgs->hor_total_time * 1000000ull;
	do_div(usec_per_line, p_timgs->pixel_clk);

	start_delay = tcon_tv_get_start_delay(&hwtcon->tcon_tv);
	usec_start_delay = start_delay * usec_per_line;

	if (usec_start_delay <= 200)
		usec_judge_point = usec_start_delay * 3 / 7;
	else if (usec_start_delay <= 400)
		usec_judge_point = usec_start_delay / 2;
	else
		usec_judge_point = 200;

	hwtcon->judge_line = usec_judge_point / usec_per_line;

	DRM_INFO("[SUNXI-TCON-HDMI]tcon%d judge_line:%u\n",
			hwtcon->id, hwtcon->judge_line);
}

static int _sunxi_tcon_hdmi_clk_prepare(struct sunxi_tcon *hwtcon,
		unsigned long pclk)
{
	long rate_diff = 0;

	if (hwtcon->tcon_top)
		sunxi_tcon_top_clk_enable(hwtcon->tcon_top);

	if (!IS_ERR_OR_NULL(hwtcon->rst_bus_tcon))
		clk_prepare_enable(hwtcon->rst_bus_tcon);

	/* set rate */
	clk_set_rate(hwtcon->mclk, pclk);
	rate_diff = (clk_round_rate(hwtcon->mclk, pclk) - pclk);
	if (rate_diff != 0)
		DRM_WARN("suxni tcon hdmi set rate: %ldHz and get diff: %ldHz\n",
			pclk, rate_diff);

	/* enable tcon bus clock */
	if (!IS_ERR_OR_NULL(hwtcon->mclk_bus))
		clk_prepare_enable(hwtcon->mclk_bus);

	if (!IS_ERR_OR_NULL(hwtcon->mclk))
		clk_prepare_enable(hwtcon->mclk);

	return 0;
}

static int _sunxi_tcon_hdmi_clk_unprepare(struct sunxi_tcon *hwtcon)
{
	if (hwtcon->tcon_top)
		sunxi_tcon_top_clk_disable(hwtcon->tcon_top);

	if (!IS_ERR_OR_NULL(hwtcon->mclk))
		clk_disable(hwtcon->mclk);

	if (!IS_ERR_OR_NULL(hwtcon->mclk_bus))
		clk_disable(hwtcon->mclk_bus);

	if (!IS_ERR_OR_NULL(hwtcon->rst_bus_tcon))
		clk_disable(hwtcon->rst_bus_tcon);

	return 0;
}

static int _sunxi_tcon_hdmi_config(struct sunxi_tcon *hwtcon, unsigned int de_id,
		struct disp_video_timings *p_timing, enum disp_csc_type format)
{
	struct disp_video_timings *timings = NULL;

	if (!p_timing) {
		DRM_ERROR("point p_timing is null\n");
		return -1;
	}

	timings = kmalloc(sizeof(struct disp_video_timings), GFP_KERNEL | __GFP_ZERO);
	if (timings) {
		memcpy(timings, p_timing, sizeof(struct disp_video_timings));
		if (format == DISP_CSC_TYPE_YUV420) {
			timings->x_res /= 2;
			timings->hor_total_time /= 2;
			timings->hor_back_porch /= 2;
			timings->hor_front_porch /= 2;
			timings->hor_sync_time /= 2;
		}
	}

	tcon_tv_init(&hwtcon->tcon_tv);
	tcon_tv_set_timming(&hwtcon->tcon_tv, timings ? timings : p_timing);
	tcon_tv_src_select(&hwtcon->tcon_tv, LCD_SRC_DE, de_id);
	tcon_tv_black_src(&hwtcon->tcon_tv, 0x0, format);

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
	tcon_tv_volume_force(&hwtcon->tcon_tv, 0x00040014);
#endif /* CONFIG_ARCH_SUN55IW3 */

	tcon1_hdmi_clk_enable(hwtcon->id, 1);

	tcon_tv_open(&hwtcon->tcon_tv);

	kfree(timings);

	return 0;
}

static int _sunxi_tcon_hdmi_disconfig(struct sunxi_tcon *hwtcon)
{

	tcon_tv_close(&hwtcon->tcon_tv);
	tcon_tv_exit(&hwtcon->tcon_tv);

	tcon1_hdmi_clk_enable(hwtcon->id, 0);

	return 0;
}

int sunxi_tcon_hdmi_open(struct udevice *dev)
{
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);

	if (hwtcon == NULL) {
		DRM_ERROR("hwtcon is error\n");
		return -1;
	}

	if (hwtcon->tcon_top != NULL)
		sunxi_tcon_top_clk_enable(hwtcon->tcon_top);

	if (hwtcon->rst_bus_tcon)
		clk_prepare_enable(hwtcon->rst_bus_tcon);

	tcon1_hdmi_clk_enable(hwtcon->id, 1);
	return 0;
}

int sunxi_tcon_hdmi_src(struct udevice *dev, u8 src)
{
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);

	if (hwtcon == NULL) {
		DRM_ERROR("hwtcon is error\n");
		return -1;
	}
	tcon_hdmi_clk_src_sel(hwtcon->id, src);
	return 0;
}

static int sunxi_tcon_hdmi_mode_init(struct udevice *dev)
{
	int ret = 0;
	unsigned long pclk;
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);
	unsigned int de_id = hwtcon->tcon_ctrl.cfg.de_id;
	struct disp_video_timings *p_timing = &hwtcon->tcon_ctrl.cfg.timing;
	enum disp_csc_type format = hwtcon->tcon_ctrl.cfg.format;

	if (hwtcon->is_enabled) {
		DRM_WARN("tcon hdmi has been enable");
		return 0;
	}

	/* calculate actual pixel clock */
	pclk = p_timing->pixel_clk * (p_timing->pixel_repeat + 1);
	if (format == DISP_CSC_TYPE_YUV420)
		pclk /= 2;


	ret = _sunxi_tcon_hdmi_clk_prepare(hwtcon, pclk);
	if (ret != 0) {
		DRM_ERROR("tcon hdmi prepare failed\n");
		return ret;
	}

	ret = _sunxi_tcon_hdmi_config(hwtcon, de_id, p_timing, format);
	if (ret != 0) {
		DRM_ERROR("tcon hdmi config failed\n");
		return ret;
	}

	_sunxi_tcon_hdmi_calc_judge_line(hwtcon, p_timing);

	sunxi_tcon_request_irq(hwtcon);
	hwtcon->is_enabled = true;
	return 0;
}

static int sunxi_tcon_hdmi_mode_exit(struct udevice *dev)
{
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);
	int ret = 0;

	if (!hwtcon->is_enabled) {
		DRM_WARN("tcon hdmi has been disable");
		return 0;
	}

	sunxi_tcon_free_irq(hwtcon);
	ret = _sunxi_tcon_hdmi_disconfig(hwtcon);
	if (ret != 0) {
		DRM_ERROR("_sunxi_tcon_hdmi_disconfig failed\n");
		return -1;
	}

	_sunxi_tcon_hdmi_clk_unprepare(hwtcon);

	hwtcon->judge_line = 0;
	hwtcon->is_enabled = false;

	return 0;
}
static int edp_tcon_clk_enable(struct sunxi_tcon *hwtcon)
{
	int ret = 0;

	if (hwtcon->tcon_top)
		sunxi_tcon_top_clk_enable(hwtcon->tcon_top);

	if (hwtcon->mclk_bus) {
		ret = clk_prepare_enable(hwtcon->mclk_bus);
		if (ret != 0) {
			DRM_WARN("fail enable edp's bus clock!\n");
			return -1;
		}
	}

	if (hwtcon->mclk) {
		if (clk_prepare_enable(hwtcon->mclk)) {
			DRM_WARN("fail to enable edp clk\n");
			return -1;
		}
	}

	return 0;
}

static int edp_tcon_clk_disable(struct sunxi_tcon *hwtcon)
{
	if (hwtcon->mclk)
		clk_disable(hwtcon->mclk);

	if (hwtcon->mclk_bus)
		clk_disable(hwtcon->mclk_bus);

	if (hwtcon->tcon_top)
		sunxi_tcon_top_clk_disable(hwtcon->tcon_top);

	return 0;
}

static void sunxi_tcon_edp_calc_judge_line(struct sunxi_tcon *hwtcon,
					   struct disp_video_timings *timings)
{
	unsigned long long usec_per_line, start_delay;
	unsigned int usec_start_delay, usec_judge_point;

	usec_per_line =
		    timings->hor_total_time * 1000000ull / timings->pixel_clk;
	start_delay = tcon_tv_get_start_delay(&hwtcon->tcon_tv);
	usec_start_delay = start_delay * usec_per_line;

	if (usec_start_delay <= 200)
		usec_judge_point = usec_start_delay * 3 / 7;
	else if (usec_start_delay <= 400)
		usec_judge_point = usec_start_delay / 2;
	else
		usec_judge_point = 200;

	hwtcon->judge_line = usec_judge_point / usec_per_line;
}

int sunxi_tcon_show_pattern(struct udevice *dev, int pattern)
{
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);

	if (hwtcon->type == TCON_LCD)
		tcon_lcd_show_builtin_patten(&hwtcon->tcon_lcd, pattern);
	else
		tcon_tv_show_builtin_patten(&hwtcon->tcon_tv, pattern);

	return 0;
}

int sunxi_tcon_pattern_get(struct udevice *dev)
{
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);

	if (hwtcon->type == TCON_LCD)
		return tcon_lcd_src_get(&hwtcon->tcon_lcd);
	else
		return tcon_tv_src_get(&hwtcon->tcon_tv);
}

static int sunxi_tcon_edp_mode_init(struct udevice *dev)
{
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);
	struct disp_video_timings *timings = &hwtcon->tcon_ctrl.cfg.timing;
	unsigned int de_id = hwtcon->tcon_ctrl.cfg.de_id;
	bool sw_enable = hwtcon->tcon_ctrl.cfg.sw_enable;
	unsigned int pixel_mode = hwtcon->tcon_ctrl.cfg.pixel_mode;

	if (hwtcon->is_enabled) {
		DRM_WARN("tcon edp has been enable!\n");
		return 0;
	}

	edp_tcon_clk_enable(hwtcon);
	if (hwtcon->mclk)
		clk_set_rate(hwtcon->mclk, timings->pixel_clk / pixel_mode);

	if (!sw_enable) {
		tcon_tv_init(&hwtcon->tcon_tv);
		tcon_tv_set_timming(&hwtcon->tcon_tv, timings);
		tcon_tv_set_pixel_mode(&hwtcon->tcon_tv, pixel_mode);

		tcon_tv_src_select(&hwtcon->tcon_tv, LCD_SRC_DE, de_id);

		tcon1_edp_clk_enable(hwtcon->id, 1);
		tcon_tv_open(&hwtcon->tcon_tv);

		sunxi_tcon_edp_calc_judge_line(hwtcon, timings);
	}

	sunxi_tcon_request_irq(hwtcon);

	hwtcon->is_enabled = true;

	return 0;
}

static int sunxi_tcon_edp_mode_exit(struct udevice *dev)
{
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);

	if (!hwtcon->is_enabled) {
		DRM_WARN("tcon edp has been disable!\n");
		return 0;
	}

	sunxi_tcon_free_irq(hwtcon);
	tcon_tv_close(&hwtcon->tcon_tv);
	tcon_tv_exit(&hwtcon->tcon_tv);
	tcon1_edp_clk_enable(hwtcon->id, 0);

	edp_tcon_clk_disable(hwtcon);

	hwtcon->is_enabled = false;
	hwtcon->judge_line = 0;

	return 0;
}

/*
 * referred from sunxi display
 */
//call by de, not use now
#define TODO_TEMP_MASK 0
#if TODO_TEMP_MASK
bool sunxi_tcon_sync_time_is_enough(unsigned int nr)
{
	int cur_line, judge_line, start_delay;
	unsigned int tcon_type;
	struct sunxi_tcon *hwtcon = sunxi_tcon_get_tcon(nr);

	tcon_type = hwtcon->type;
	judge_line = hwtcon->judge_line;

	if (tcon_type == TCON_LCD) {
		cur_line = tcon_lcd_get_cur_line(&hwtcon->tcon_lcd);
		start_delay = tcon_lcd_get_start_delay(&hwtcon->tcon_lcd);
	} else {
		cur_line = tcon_tv_get_cur_line(&hwtcon->tcon_tv);
		start_delay = tcon_tv_get_start_delay(&hwtcon->tcon_tv);
	}

	/*
	DRM_INFO("cur_line:%d start_delay:%d judge_line:%d\n",
			cur_line, start_delay, judge_line);
	 */

	if (cur_line <= (start_delay - judge_line))
		return true;

	return false;
}

#endif


static int sunxi_tcon_parse_dts(struct udevice *dev)
{
	struct sunxi_tcon *hwtcon = dev_get_priv(dev);

	hwtcon->reg_base = (uintptr_t)dev_read_addr_ptr(dev);
	if (!hwtcon->reg_base) {
		DRM_ERROR("unable to map io for tcon\n");
		return -EINVAL;
	}

	hwtcon->irq_no = sunxi_of_get_irq_number(dev, 0);
	if (!hwtcon->irq_no) {
		DRM_ERROR("get irq no of tcon failed\n");
		return -EINVAL;
	}

	hwtcon->mclk = clk_get_by_name(dev, "clk_tcon");
	if (IS_ERR_OR_NULL(hwtcon->mclk)) {
		DRM_INFO("fail to get clk for tcon \n");
	}

	hwtcon->ahb_clk = clk_get_by_name(dev, "clk_ahb_tcon");
	if (IS_ERR_OR_NULL(hwtcon->ahb_clk)) {
		DRM_INFO("fail to get ahb clk for tcon \n");
	}

	hwtcon->mclk_bus = clk_get_by_name(dev, "clk_bus_tcon");
	if (IS_ERR_OR_NULL(hwtcon->mclk_bus)) {
		DRM_INFO("fail to get clk bus for tcon\n");
	}

	hwtcon->rst_bus_tcon = clk_get_by_name(dev, "rst_bus_tcon");
	if (IS_ERR_OR_NULL(hwtcon->rst_bus_tcon)) {
		DRM_INFO("fail to get clk rst bus for tcon\n");
	}

	return 0;
}

struct udevice *sunxi_tcon_of_get_tcon_dev(struct udevice *remote_dev)
{
	struct device_node *node = NULL;
	ofnode port_node;
	struct device_node *rep_parent = NULL;
	struct udevice *tcon_udev = NULL;
	int ret = 0;

	node = sunxi_of_graph_get_remote_node(dev_ofnode(remote_dev), PORT_DIR_IN, 0);
	if (!node) {
		DRM_ERROR("sunxi_of_graph_get_remote_node fail!\n");
		goto OUT;
	}

	port_node = ofnode_get_parent(np_to_ofnode(node));
	if (!ofnode_valid(port_node)) {
		DRM_ERROR("Get remote node's parent fail\n");
		goto OUT;
	}

	rep_parent = sunxi_of_graph_get_port_parent(port_node);
	if (!rep_parent) {
		DRM_ERROR("sunxi_of_graph_get_port_parent fail!\n");
		goto OUT;
	}
	DRM_INFO("Get tcon node :%s\n", ofnode_get_name(np_to_ofnode(rep_parent)));

	ret = uclass_get_device_by_ofnode(UCLASS_MISC, np_to_ofnode(rep_parent), &tcon_udev);
	if (ret) {
		DRM_ERROR("Get uclass_get_device_by_ofnode fail!\n");
	}

OUT:
	return tcon_udev;
}

bool sunxi_tcon_check_fifo_status(struct udevice *tcon_dev)
{
	int status = 0;
	struct sunxi_tcon *tcon = dev_get_priv(tcon_dev);

	if (tcon->type == TCON_TV) {
		status = tcon_tv_get_status(&tcon->tcon_tv);
	}
	if (tcon->type == TCON_LCD) {
		status = tcon_lcd_get_status(&tcon->tcon_lcd);
	}
	return status ? true : false;
}

void sunxi_tcon_enable_vblank(struct udevice *tcon_dev, bool enable)
{
	struct sunxi_tcon *tcon = dev_get_priv(tcon_dev);

	if (tcon->type == TCON_TV)
		tcon_tv_enable_vblank(&tcon->tcon_tv, enable);
	if (tcon->type == TCON_LCD)
		tcon_lcd_enable_vblank(&tcon->tcon_lcd, enable);
}

int sunxi_tcon_of_get_id(struct udevice *tcon_dev)
{
	struct device_node *disp0_output_ep;
	struct device_node *tcon_in_disp0_ep;
	u32 id;

	tcon_in_disp0_ep = sunxi_of_graph_get_port_by_id(dev_ofnode(tcon_dev), PORT_DIR_IN);
	if (!tcon_in_disp0_ep) {
		DRM_ERROR("endpoint tcon_in_disp0_ep not fount\n");
		return -EINVAL;
	}
	disp0_output_ep = sunxi_of_graph_get_remote_endpoint(tcon_in_disp0_ep);
	if (!disp0_output_ep) {
		DRM_ERROR("endpoint disp0_output_ep not fount\n");
		return -EINVAL;
	}

	if (ofnode_read_u32(np_to_ofnode(disp0_output_ep), "reg", &id)) {
		DRM_ERROR("%s:Get reg fail!\n", __func__);
		goto OUT;
	}

	DRM_INFO("[SUNXI-TCON] %s %d\n", __FUNCTION__, id);
OUT:
	of_node_put(tcon_in_disp0_ep);
	of_node_put(disp0_output_ep);
	return id;
}

int sunxi_tcon_of_get_top_id(struct udevice *tcon_dev)
{
	struct sunxi_tcon *tcon = dev_get_priv(tcon_dev);
	return sunxi_tcon_top_get_id(tcon->tcon_top);
}

int sunxi_tcon_mode_init(struct udevice *tcon_dev, struct disp_output_config *disp_cfg)
{
	struct sunxi_tcon *tcon = dev_get_priv(tcon_dev);
	struct tcon_device *ctrl = &tcon->tcon_ctrl;

	memcpy(&ctrl->cfg, disp_cfg, sizeof(struct disp_output_config));

	switch (ctrl->cfg.type) {
	case INTERFACE_EDP:
		return sunxi_tcon_edp_mode_init(tcon_dev);
	case INTERFACE_HDMI:
		return sunxi_tcon_hdmi_mode_init(tcon_dev);
	case INTERFACE_DSI:
		return sunxi_tcon_dsi_mode_init(tcon_dev);
	case INTERFACE_LVDS:
		return sunxi_tcon_lvds_mode_init(tcon_dev);
	case INTERFACE_RGB:
		return sunxi_tcon_rgb_mode_init(tcon_dev);
	default:
		break;
	}

	return -1;
}

int sunxi_tcon_mode_exit(struct udevice *tcon_dev)
{
	struct sunxi_tcon *tcon = dev_get_priv(tcon_dev);
	struct tcon_device *ctrl = &tcon->tcon_ctrl;

	switch (ctrl->cfg.type) {
	case INTERFACE_EDP:
		return sunxi_tcon_edp_mode_exit(tcon_dev);
	case INTERFACE_HDMI:
		return sunxi_tcon_hdmi_mode_exit(tcon_dev);
	case INTERFACE_DSI:
		return sunxi_tcon_dsi_mode_exit(tcon_dev);
	case INTERFACE_LVDS:
		return sunxi_tcon_lvds_mode_exit(tcon_dev);
	case INTERFACE_RGB:
		return sunxi_tcon_rgb_mode_exit(tcon_dev);
	default:
		break;
	}


	return 0;
}

int sunxi_tcon_print(struct udevice *tcon_dev)
{
	struct sunxi_tcon *tcon = dev_get_priv(tcon_dev);

	print_buffer((ulong)tcon->reg_base, (void *)tcon->reg_base, 4, 94, 4);

	print_buffer((ulong)tcon->reg_base + 0x220, (void *)tcon->reg_base + 0x220, 4, 12, 4);

	return 0;
}

static int sunxi_tcon_probe(struct udevice *dev)
{
	int ret;
	struct sunxi_tcon *tcon = dev_get_priv(dev);
	struct tcon_device *ctrl = &tcon->tcon_ctrl;

	DRM_INFO("[TCON] sunxi_tcon_probe start\n");

	if (!tcon) {
		DRM_ERROR("can NOT allocate memory for tcon_drv\n");
		ret = -ENOMEM;
		goto out;
	}
	tcon->dev = dev;
	tcon->id = sunxi_tcon_of_get_id(dev);
	tcon->type = get_dev_tcon_type(dev_ofnode(dev));
	ret = sunxi_tcon_parse_dts(dev);
	if (ret) {
		DRM_ERROR("sunxi_tcon_parse_dts failed\n");
		goto out;
	}


	if (tcon->type == TCON_TV) {
		tcon->tcon_tv.tcon_index = tcon->id;
		tcon_tv_set_reg_base(&tcon->tcon_tv, tcon->reg_base);
	}

	if (tcon->type == TCON_LCD) {
		tcon->tcon_lcd.tcon_index = tcon->id;
		tcon_lcd_set_reg_base(&tcon->tcon_lcd, tcon->reg_base);
	}


	ret = sunxi_tcon_get_tcon_top(tcon);
	if (ret)
		goto out;

	ctrl->dev = dev;
	ctrl->hw_id = tcon->id;
	of_periph_clk_config_setup(ofnode_to_offset(dev_ofnode(dev)));
/*
	tcon->lvds_combo_phy0 = devm_phy_optional_get(dev, "lvds_combo_phy0");
	if (IS_ERR(tcon->lvds_combo_phy0)) {
		DRM_ERROR("lvds_combo_phy0 get fail failed\n");
		goto out;
	}

	tcon->lvds_combo_phy1 = devm_phy_optional_get(dev, "lvds_combo_phy1");
	if (IS_ERR(tcon->lvds_combo_phy1)) {
		DRM_ERROR("lvds_combo_phy1 get fail failed\n");
		goto out;
	}
*/
out:
	DRM_INFO("[TCON] sunxi_tcon_probe ret = %d\n", ret);
	return ret;
}

U_BOOT_DRIVER(tcon) = {
	.name = "tcon",
	.id = UCLASS_MISC,
	.of_match = sunxi_tcon_match,
	.probe = sunxi_tcon_probe,
	.priv_auto_alloc_size = sizeof(struct sunxi_tcon),
};
