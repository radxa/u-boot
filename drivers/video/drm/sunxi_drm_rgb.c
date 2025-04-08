/*
 * drivers/video/drm/sunxi_drm_rgb/sunxi_drm_rgb.c
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
#include <dm.h>
#include <dm/pinctrl.h>
#include <asm/arch/gic.h>
#include <drm/drm_modes.h>
#include <sys_config.h>
#include <tcon_lcd.h>
#include <sunxi_device/sunxi_tcon.h>
#include <drm/drm_print.h>
#include <generic-phy.h>
#include "sunxi_drm_phy.h"

#include "sunxi_drm_crtc.h"
#include "sunxi_drm_connector.h"
#include "sunxi_drm_drv.h"

#if IS_ENABLED(CONFIG_MACH_SUN55IW6) || IS_ENABLED(CONFIG_MACH_SUN60IW2)
#define RGB_DISPLL_CLK
#endif
struct rgb_data {
	int id;
};

struct sunxi_drm_rgb {
	struct udevice *dev;
	struct sunxi_drm_connector connector;
	struct udevice *tcon_dev;
	struct drm_display_mode mode;
	struct disp_rgb_para rgb_para;
	unsigned int tcon_id;
	unsigned int tcon_top_id;
	bool bound;
	const struct rgb_data *rgb_data;
	u32 rgb_id;

	struct phy *phy;
	struct sunxi_drm_phy_cfg phy_opts;
	unsigned long mode_flags;
};

static const struct rgb_data rgb0_data = {
	.id = 0,
};

static const struct rgb_data rgb1_data = {
	.id = 1,
};

static const struct udevice_id sunxi_drm_rgb_match[] = {
	{ .compatible = "allwinner,rgb0", .data = (ulong)&rgb0_data },
	{ .compatible = "allwinner,rgb1", .data = (ulong)&rgb1_data },
	{},
};

static bool sunxi_rgb_fifo_check(void *data)
{
	struct sunxi_drm_rgb *rgb = (struct sunxi_drm_rgb *)data;
	return sunxi_tcon_check_fifo_status(rgb->tcon_dev);
}

static void sunxi_rgb_enable_vblank(bool enable, void *data)
{
	struct sunxi_drm_rgb *rgb = (struct sunxi_drm_rgb *)data;

	sunxi_tcon_enable_vblank(rgb->tcon_dev, enable);
}


static int sunxi_rgb_connector_init(struct sunxi_drm_connector *conn, struct display_state *state)
{
	struct sunxi_drm_rgb *rgb = dev_get_priv(conn->dev);
	struct crtc_state *scrtc_state = &state->crtc_state;


	scrtc_state->tcon_id = rgb->tcon_id;
	scrtc_state->tcon_top_id = rgb->tcon_top_id;
	scrtc_state->enable_vblank = sunxi_rgb_enable_vblank;
	scrtc_state->check_status = sunxi_rgb_fifo_check;
	scrtc_state->vblank_enable_data = rgb;

	return 0;
}


static int sunxi_rgb_connector_prepare(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	int ret = -1;
	char tmp[128] = {0};
	struct sunxi_drm_rgb *rgb = dev_get_priv(conn->dev);
	struct crtc_state *scrtc_state = &state->crtc_state;
	struct disp_output_config disp_cfg;
	struct connector_state *conn_state = &state->conn_state;

	memcpy(&rgb->mode, &conn_state->mode, sizeof(struct drm_display_mode));
	memset(&disp_cfg, 0, sizeof(struct disp_output_config));
	disp_cfg.type = INTERFACE_RGB;
	disp_cfg.de_id = sunxi_drm_crtc_get_hw_id(scrtc_state->crtc);
	disp_cfg.irq_handler = scrtc_state->crtc_irq_handler;
	disp_cfg.irq_data = state;
	disp_cfg.tcon_lcd_div = 7;
#ifdef RGB_DISPLL_CLK
	disp_cfg.displl_clk = true;
#else
	disp_cfg.displl_clk = false;
#endif
	if (rgb->rgb_id == 1) {
		disp_cfg.displl_clk = false;
	}
	drm_mode_to_sunxi_video_timings(&rgb->mode, &rgb->rgb_para.timings);
	memcpy(&disp_cfg.rgb_para, &rgb->rgb_para,
		sizeof(rgb->rgb_para));

	sunxi_tcon_mode_init(rgb->tcon_dev, &disp_cfg);
	rgb->phy_opts.mipi_dphy.hs_clk_rate = rgb->rgb_para.timings.pixel_clk * disp_cfg.tcon_lcd_div;

	ret = fdt_get_path(working_fdt, ofnode_to_offset(dev_ofnode(rgb->dev)), tmp, sizeof(tmp));
	ret = fdt_set_all_pin(tmp, "pinctrl-0");
	if (ret < 0) {
		DRM_ERROR("%s:%d:fdt_set_all_pin fail!:%d\n", __func__, __LINE__, ret);
	}

	return 0;
}


static int sunxi_rgb_connector_enable(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	struct sunxi_drm_rgb *rgb = dev_get_priv(conn->dev);
	int ret = -1;

	DRM_INFO("%s\n", __func__);
	if (rgb->phy) {
		generic_phy_power_on(rgb->phy);
		generic_phy_configure(rgb->phy, &rgb->phy_opts);
	}

	ret = sunxi_rgb_enable_output(rgb->tcon_dev);
	if (ret < 0)
		DRM_ERROR("failed to enable rgb ouput\n");

	return ret;
}


static const struct sunxi_drm_connector_funcs rgb_connector_funcs = {
	.init = sunxi_rgb_connector_init,
	.prepare = sunxi_rgb_connector_prepare,
	/*.unprepare = sunxi_rgb_connector_unprepare,*/
	.enable = sunxi_rgb_connector_enable,
	/*.disable = sunxi_rgb_connector_disable,*/
	/*.check = rgb_connector_check,*/
};



int sunxi_rgb_parse_dt(struct udevice *dev)
{
	struct sunxi_drm_rgb *rgb = dev_get_priv(dev);

	generic_phy_get_by_name(dev, "combophy0", rgb->phy);
	if (IS_ERR_OR_NULL(rgb->phy)) {
		DRM_INFO("rgb%d's combophy0 not setting, maybe not used!\n", rgb->rgb_id);
		rgb->phy = NULL;
		goto NO_PHY;
	}

NO_PHY:
	return 0;
}

static int sunxi_drm_rgb_probe(struct udevice *dev)
{
	struct sunxi_drm_rgb *rgb = dev_get_priv(dev);

	rgb->dev = dev;
	rgb->rgb_data = (struct rgb_data *)dev_get_driver_data(dev);
	rgb->rgb_id = rgb->rgb_data->id;
	rgb->tcon_dev = sunxi_tcon_of_get_tcon_dev(dev);
	if (!rgb->tcon_dev) {
		pr_err("%s:Get tcon dev fail!\n", __func__);
		return -1;
	}
	rgb->tcon_id = sunxi_tcon_of_get_id(rgb->tcon_dev);
	rgb->tcon_top_id = sunxi_tcon_of_get_top_id(rgb->tcon_dev);
	rgb->phy = kmalloc(sizeof(struct phy), __GFP_ZERO);
	sunxi_rgb_parse_dt(rgb->dev);

	sunxi_drm_connector_bind(&rgb->connector, dev, rgb->rgb_id, &rgb_connector_funcs,
				NULL, DRM_MODE_CONNECTOR_DPI);

	of_periph_clk_config_setup(ofnode_to_offset(dev_ofnode(dev)));
	rgb->bound = true;

	return 0;
}

U_BOOT_DRIVER(sunxi_drm_rgb) = {
	.name = "sunxi_drm_rgb",
	.id = UCLASS_DISPLAY,
	.of_match = sunxi_drm_rgb_match,
	.probe = sunxi_drm_rgb_probe,
	.priv_auto_alloc_size = sizeof(struct sunxi_drm_rgb),
};
//End of File
