/*
 * sunxi_drm_lvds/sunxi_drm_lvds.c
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


#include <common.h>
#include <linux/compat.h>
#include <dm.h>
#include <dm/ofnode.h>
#include <dm/pinctrl.h>
#include <sys_config.h>
#include <drm/drm_print.h>
#include <media_bus_format.h>
#include <generic-phy.h>

#include "sunxi_device/sunxi_tcon.h"
#include "sunxi_drm_helper_funcs.h"
#include "sunxi_drm_connector.h"
#include "sunxi_drm_phy.h"
#include "sunxi_drm_crtc.h"
#include "sunxi_drm_drv.h"

#if IS_ENABLED(CONFIG_MACH_SUN55IW6) || IS_ENABLED(CONFIG_MACH_SUN60IW2) \
	|| IS_ENABLED(CONFIG_MACH_SUN65IW1) || IS_ENABLED(CONFIG_MACH_SUN55IW3)
#define LVDS_DISPLL_CLK
#endif
struct lvds_data {
	int id;
};
struct sunxi_drm_lvds {
	struct sunxi_drm_connector connector;
	struct udevice *tcon_dev;
	struct udevice *dev;
	struct drm_display_mode mode;
	struct disp_lvds_para lvds_para;
	unsigned int tcon_id;
	unsigned int tcon_top_id;
	//struct drm_panel *panel;
	//struct drm_bridge *bridge;
	bool bound;
	//bool allow_sw_enable;
	//bool sw_enable;
	struct phy *phy0;
	struct phy *phy1;
	struct sunxi_drm_phy_cfg phy_opts;

	const struct lvds_data *lvds_data;
	u32 lvds_id;

	struct clk *mclk;
	unsigned long mode_flags;

};


static const struct lvds_data lvds0_data = {
	.id = 0,
};

static const struct lvds_data lvds1_data = {
	.id = 1,
};

static const struct udevice_id sunxi_drm_lvds_match[] = {
	{ .compatible = "allwinner,lvds0", .data = (ulong)&lvds0_data },
	{ .compatible = "allwinner,lvds1", .data = (ulong)&lvds1_data },
	{},
};



static int sunxi_lvds_clk_config_enable(struct sunxi_drm_lvds *lvds,
					const struct disp_lvds_para *para)
{
	int ret = 0;
	struct clk *clks[] = {lvds->mclk};

	ret = sunxi_clk_enable(clks, ARRAY_SIZE(clks));

	return ret;
}

static int sunxi_lvds_clk_config_disable(struct sunxi_drm_lvds *lvds)
{
	int ret = 0;
	struct clk *clks[] = {lvds->mclk};

	ret = sunxi_clk_disable(clks, ARRAY_SIZE(clks));

	return ret;
}

static int sunxi_lvds_connector_enable(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	int ret = -1;
	struct sunxi_drm_lvds *lvds = dev_get_priv(conn->dev);
	struct crtc_state *scrtc_state = &state->crtc_state;
	struct disp_output_config disp_cfg;
	struct sunxi_drm_phy_cfg *phy_cfg = &lvds->phy_opts;
	char tmp[128] = {0};

	DRM_INFO("[LVDS] %s start\n", __FUNCTION__);

	memset(&disp_cfg, 0, sizeof(struct disp_output_config));
	memset(phy_cfg, 0, sizeof(struct sunxi_drm_phy_cfg));

	disp_cfg.type = INTERFACE_LVDS;
	disp_cfg.de_id = sunxi_drm_crtc_get_hw_id(scrtc_state->crtc);
	disp_cfg.irq_handler = scrtc_state->crtc_irq_handler;
	disp_cfg.irq_data = state;
	disp_cfg.tcon_lcd_div = 7;
#ifdef LVDS_DISPLL_CLK
	disp_cfg.displl_clk = true;
#else
	disp_cfg.displl_clk = false;
#endif
	if (lvds->lvds_id == 1)
		disp_cfg.displl_clk = false;
	memcpy(&disp_cfg.lvds_para, &lvds->lvds_para,
	       sizeof(struct disp_lvds_para));

	sunxi_tcon_mode_init(lvds->tcon_dev, &disp_cfg);

	ret = sunxi_lvds_clk_config_enable(lvds, &lvds->lvds_para);
	if (ret) {
		DRM_ERROR("lvds clk enable failed\n");
		return -1;
	}

	ret = fdt_get_path(working_fdt, ofnode_to_offset(dev_ofnode(lvds->dev)), tmp, 128);
	ret = fdt_set_all_pin(tmp, "pinctrl-0");
	if (ret < 0) {
		DRM_ERROR("%s:pinctrl_select_state fail!:%d\n", __func__, ret);
	}

	phy_cfg->mode = PHY_MODE_LVDS;
	phy_cfg->submode = 1;
	phy_cfg->mipi_dphy.hs_clk_rate = lvds->lvds_para.timings.pixel_clk * disp_cfg.tcon_lcd_div;

	if (lvds->phy0) {
		generic_phy_power_on(lvds->phy0);
		generic_phy_configure(lvds->phy0, (void *)phy_cfg);
	}
	if (lvds->phy1) {
		generic_phy_power_on(lvds->phy1);
		generic_phy_configure(lvds->phy1, (void *)phy_cfg);
	}

	/*drm_panel_prepare(lvds->panel);*/
	/*drm_panel_enable(lvds->panel);*/

	ret = sunxi_lvds_enable_output(lvds->tcon_dev);
	if (ret < 0)
		DRM_ERROR("failed to enable lvds ouput\n");
	DRM_INFO("[LVDS] %s finish\n", __FUNCTION__);

	return ret;
}

static void sunxi_lvds_enable_vblank(bool enable, void *data)
{
	struct sunxi_drm_lvds *lvds = (struct sunxi_drm_lvds *)data;

	sunxi_tcon_enable_vblank(lvds->tcon_dev, enable);
}

static int sunxi_lvds_connector_disable(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	struct sunxi_drm_lvds *lvds = dev_get_priv(conn->dev);
	/*struct crtc_state *scrtc_state = &state->crtc_state;*/

	/*drm_panel_disable(lvds->panel);*/
	/*drm_panel_unprepare(lvds->panel);*/

	if (lvds->phy0) {
		generic_phy_power_off(lvds->phy0);
	}
	if (lvds->phy1) {
		generic_phy_power_off(lvds->phy1);
	}
	sunxi_lvds_clk_config_disable(lvds);

	pinctrl_select_state(lvds->dev, "sleep");
	sunxi_lvds_disable_output(lvds->tcon_dev);
	sunxi_tcon_mode_exit(lvds->dev);
	DRM_INFO("%s finish\n", __FUNCTION__);
	return 0;
}

static bool sunxi_lvds_fifo_check(void *data)
{
	struct sunxi_drm_lvds *lvds = (struct sunxi_drm_lvds *)data;
	return sunxi_tcon_check_fifo_status(lvds->tcon_dev);
}

static int sunxi_lvds_connector_init(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	struct sunxi_drm_lvds *lvds = dev_get_priv(conn->dev);
	struct crtc_state *scrtc_state = &state->crtc_state;

	scrtc_state->tcon_id = lvds->tcon_id;
	scrtc_state->tcon_top_id = lvds->tcon_top_id;
	scrtc_state->enable_vblank = sunxi_lvds_enable_vblank;
	scrtc_state->check_status = sunxi_lvds_fifo_check;
	scrtc_state->vblank_enable_data = lvds;
	return 0;
}
static int sunxi_lvds_connector_prepare(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_info *info = &conn_state->info;
	struct sunxi_drm_lvds *lvds = dev_get_priv(conn->dev);
	struct disp_lvds_para *lvds_para = &lvds->lvds_para;
	u32 bus_format = MEDIA_BUS_FMT_RGB888_1X7X4_SPWG;

	memcpy(&lvds->mode, &conn_state->mode, sizeof(struct drm_display_mode));

	drm_mode_to_sunxi_video_timings(&lvds->mode, &lvds_para->timings);

	if (info->num_bus_formats)
		bus_format = info->bus_formats[0];
	switch (bus_format) {
	case MEDIA_BUS_FMT_RGB888_1X7X4_JEIDA:  // jeida-24
		lvds_para->lvds_data_mode = 1;
		lvds_para->lvds_colordepth = 0;
		break;
	case MEDIA_BUS_FMT_RGB666_1X7X3_SPWG:   // vesa-18
		lvds_para->lvds_data_mode = 0;
		lvds_para->lvds_colordepth = 1;
		break;
	case MEDIA_BUS_FMT_RGB888_1X7X4_SPWG:   // vesa-24
		lvds_para->lvds_data_mode = 0;
		lvds_para->lvds_colordepth = 0;
		break;
	default:
		;
	}

	return 0;
}


static const struct sunxi_drm_connector_funcs lvds_connector_funcs = {
	.init = sunxi_lvds_connector_init,
	.prepare = sunxi_lvds_connector_prepare,
	.enable = sunxi_lvds_connector_enable,
	.disable = sunxi_lvds_connector_disable,
	/*.check = sunxi_lvds_connector_check,*/
};


s32 sunxi_lvds_parse_dt(struct udevice *dev)
{
	s32 ret = -1;
	u32 value = 0;
	struct sunxi_drm_lvds *lvds = dev_get_priv(dev);
	struct disp_lvds_para *lvds_para = &lvds->lvds_para;
	ofnode of_node = dev_ofnode(dev);

	if (!lvds->lvds_id) {
		ret = generic_phy_get_by_name(dev, "combophy0", lvds->phy0);
		if (ret)
			DRM_INFO("lvds%d's combophy0 not setting, maybe not used!\n", lvds->lvds_id);
		else
			generic_phy_init(lvds->phy0);
	}
	ret = ofnode_read_u32(of_node, "dual-channel", &value);
	if (!ret) {
		lvds_para->dual_lvds = value;
	}

	if (lvds_para->dual_lvds && !lvds->lvds_id) {
		ret = generic_phy_get_by_name(dev, "combophy1", lvds->phy1);
		if (ret)
			DRM_INFO("lvds%d's combophy1 not setting, maybe not used!\n", lvds->lvds_id);
		else
			generic_phy_init(lvds->phy1);
	}

	lvds->mclk = clk_get_by_name(dev, "clk_lvds");
	if (IS_ERR_OR_NULL(lvds->mclk)) {
		DRM_ERROR("fail to get clk for lvds \n");
	}

	return 0;
}

static int sunxi_drm_lvds_probe(struct udevice *dev)
{
	struct sunxi_drm_lvds *lvds = dev_get_priv(dev);
	int ret = -1;

	lvds->dev = dev;
	lvds->lvds_data = (struct lvds_data *)dev_get_driver_data(dev);
	lvds->lvds_id = lvds->lvds_data->id;
	lvds->tcon_dev = sunxi_tcon_of_get_tcon_dev(dev);
	if (!lvds->tcon_dev) {
		pr_err("%s:Get tcon dev fail!\n", __func__);
		return -1;
	}
	lvds->tcon_id = sunxi_tcon_of_get_id(lvds->tcon_dev);
	lvds->tcon_top_id = sunxi_tcon_of_get_top_id(lvds->tcon_dev);
	lvds->phy0 = kmalloc(sizeof(struct phy), __GFP_ZERO);
	lvds->phy1 = kmalloc(sizeof(struct phy), __GFP_ZERO);
	if (!lvds->phy0 || !lvds->phy1) {
		return -ENOMEM;
	}

	ret = sunxi_lvds_parse_dt(dev);
	if (ret) {
		DRM_ERROR("sunxi_tcon_parse_dts failed\n");
	}

	sunxi_drm_connector_bind(&lvds->connector, dev, lvds->lvds_id, &lvds_connector_funcs,
				NULL, DRM_MODE_CONNECTOR_LVDS);

	of_periph_clk_config_setup(ofnode_to_offset(dev_ofnode(dev)));
	lvds->bound = true;

	return 0;
}

U_BOOT_DRIVER(sunxi_drm_lvds) = {
	.name = "sunxi_drm_lvds",
	.id = UCLASS_DISPLAY,
	.of_match = sunxi_drm_lvds_match,
	.probe = sunxi_drm_lvds_probe,
	.priv_auto_alloc_size = sizeof(struct sunxi_drm_lvds),
};

//End of File
