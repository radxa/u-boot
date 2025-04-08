/*
 * panels/panel-lvds/panel-lvds.c
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
#include <backlight.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <drm/drm_modes.h>
#include <media_bus_format.h>
#include "../sunxi_drm_panel.h"
#include "../sunxi_drm_helper_funcs.h"

#define POWER_MAX 3
#define GPIO_MAX 3

struct panel_lvds {
	struct sunxi_drm_panel panel;
	struct udevice *dev;

	const char *label;
	unsigned int width;
	unsigned int height;
	struct videomode video_mode;
	bool data_mirror;
	struct {
		unsigned int power;
		unsigned int enable;
		unsigned int reset;
	} delay;

	struct udevice *backlight;
	uint32_t supply[POWER_MAX];

	ulong enable_gpio[GPIO_MAX];
	ulong reset_gpio;
};

static void panel_lvds_sleep(unsigned int msec)
{
	mdelay(msec);
}

static inline struct panel_lvds *to_panel_lvds(struct sunxi_drm_panel *panel)
{
	return container_of(panel, struct panel_lvds, panel);
}


static int panel_lvds_prepare(struct sunxi_drm_panel *panel)
{
	struct panel_lvds *lvds = to_panel_lvds(panel);
	int err, i;

	DRM_INFO("%s\n", __func__);
	for (i = 0; i < POWER_MAX; i++) {
		if (lvds->supply[i]) {
			err = sunxi_drm_power_enable(lvds->supply[i], 0);
			if (err < 0) {
				DRM_ERROR("failed to enable supply%d: %d\n",
					i, err);
				return err;
			}
			if (lvds->delay.power)
				mdelay(lvds->delay.power);
		}
	}

	for (i = 0; i < GPIO_MAX; i++) {
		if (lvds->enable_gpio[i]) {
			sunxi_drm_gpio_set_value(lvds->enable_gpio[i], 1);

			if (lvds->delay.enable)
				mdelay(lvds->delay.enable);
		}
	}

	if (lvds->reset_gpio)
		sunxi_drm_gpio_set_value(lvds->reset_gpio, 1);
	if (lvds->delay.reset)
		mdelay(lvds->delay.reset);

	return 0;
}

static int panel_lvds_enable(struct sunxi_drm_panel *panel)
{
	struct panel_lvds *lvds = to_panel_lvds(panel);

	DRM_INFO("%s\n", __func__);
	if (lvds->backlight) {
		/*backlight_set_brightness(lvds->backlight)*/
		backlight_enable(lvds->backlight);
	}

	return 0;
}


static int panel_lvds_disable(struct sunxi_drm_panel *panel)
{
	struct panel_lvds *lvds = to_panel_lvds(panel);

	if (lvds->backlight)
		backlight_disable(lvds->backlight);

	return 0;
}

static int panel_lvds_unprepare(struct sunxi_drm_panel *panel)
{
	struct panel_lvds *lvds = to_panel_lvds(panel);
	int i;


	for (i = GPIO_MAX; i > 0; i--) {
		if (lvds->enable_gpio[i - 1]) {
			sunxi_drm_gpio_set_value(lvds->enable_gpio[i - 1], 0);
			if (lvds->delay.enable)
				panel_lvds_sleep(lvds->delay.enable);
		}
	}

	if (lvds->reset_gpio)
		sunxi_drm_gpio_set_value(lvds->reset_gpio, 0);
	if (lvds->delay.reset)
		panel_lvds_sleep(lvds->delay.reset);

	for (i = POWER_MAX; i > 0; i--) {
		if (lvds->supply[i - 1]) {
			sunxi_drm_power_disable(lvds->supply[i - 1]);
			if (lvds->delay.power)
				mdelay(lvds->delay.power);
		}
	}

	return 0;
}

static const struct sunxi_drm_panel_funcs panel_lvds_funcs = {
	.disable = panel_lvds_disable,
	.unprepare = panel_lvds_unprepare,
	.prepare = panel_lvds_prepare,
	.enable = panel_lvds_enable,
	/*.get_modes = panel_rgb_get_modes,*/
};


static int panel_lvds_parse_dt(struct panel_lvds *lvds)
{
	char power_name[40] = {0}, gpio_name[40] = {0};
	int i = 0;
	ulong ret;

	ret = sunxi_of_get_panel_orientation(lvds->dev, &lvds->panel.orientation);

	for (i = 0; i < POWER_MAX; i++) {
		lvds->supply[i] = 0;
		snprintf(power_name, 40, "power%d-supply", i);
		if (dev_read_bool(lvds->dev, power_name)) {
			ret = dev_read_u32(lvds->dev, power_name, &lvds->supply[i]);
			if (ret) {
				pr_err("failed to request regulator(%s): %d\n", power_name, ret);
			}
		}
	}
	dev_read_u32(lvds->dev, "power-delay-ms", &lvds->delay.power);
	dev_read_u32(lvds->dev, "enable-delay-ms", &lvds->delay.enable);
	dev_read_u32(lvds->dev, "reset-delay-ms", &lvds->delay.reset);

	/* Get GPIOs and backlight controller. */
	for (i = 0; i < GPIO_MAX; i++) {
		snprintf(gpio_name, 40, "enable%d-gpios", i);

		if (dev_read_bool(lvds->dev, gpio_name)) {
			ret = sunxi_drm_gpio_request(lvds->dev, gpio_name);
			if (ret < 0) {
				pr_err("failed to request %s GPIO: %d\n", gpio_name, ret);
			} else
				lvds->enable_gpio[i] = ret;
		}
	}

	if (dev_read_bool(lvds->dev, "reset-gpios")) {
		ret = sunxi_drm_gpio_request(lvds->dev, "reset-gpios");
		if (ret < 0) {
			pr_err("failed to request %s GPIO: %d\n", "reset", ret);
		} else
			lvds->reset_gpio = ret;
	}

	lvds->panel.bus_format = dev_read_u32_default(lvds->dev, "bus-format", MEDIA_BUS_FMT_RGB888_1X7X4_JEIDA);

	lvds->data_mirror = dev_read_bool(lvds->dev, "data-mirror");

	return 0;

}

static int panel_lvds_probe(struct udevice *dev)
{
	struct panel_lvds *lvds = dev_get_priv(dev);
	struct sunxi_drm_panel *panel = &lvds->panel;
	int ret = 0;

	lvds->dev = dev;

	ret = panel_lvds_parse_dt(lvds);
	if (ret < 0)
		return ret;

	ret = uclass_get_device_by_phandle(UCLASS_PANEL_BACKLIGHT, dev,
					   "backlight", &lvds->backlight);
	if (ret) {
		pr_err("%s: Cannot get backlight: %d\n", __func__, ret);
	}


	dev->driver_data = (ulong)panel;
	panel->dev = dev;
	panel->funcs = &panel_lvds_funcs;

	return 0;
}

static const struct udevice_id panel_lvds_of_table[] = {
	{ .compatible = "sunxi-lvds", },
	{ /* Sentinel */ },
};

U_BOOT_DRIVER(sunxi_lvds) = {
	.name	= "sunxi_lvds",
	.id	= UCLASS_PANEL,
	.of_match = panel_lvds_of_table,
	.probe	= panel_lvds_probe,
	.priv_auto_alloc_size = sizeof(struct panel_lvds),
};

//End of File
