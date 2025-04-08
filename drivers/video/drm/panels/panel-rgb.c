/*
 * panels/panel-rgb/panel-rgb.c
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
#include <power/regulator.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <drm/drm_modes.h>
#include "../sunxi_drm_panel.h"
#include "../sunxi_drm_helper_funcs.h"

#define POWER_MAX 3
#define GPIO_MAX 3
struct panel_rgb {
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


static inline struct panel_rgb *to_panel_rgb(struct sunxi_drm_panel *panel)
{
	return container_of(panel, struct panel_rgb, panel);
}


static int panel_rgb_prepare(struct sunxi_drm_panel *panel)
{
	struct panel_rgb *rgb = to_panel_rgb(panel);
	int err, i;

	for (i = 0; i < POWER_MAX; i++) {
		if (rgb->supply[i]) {
			err = sunxi_drm_power_enable(rgb->supply[i], 0);
			if (err < 0) {
				DRM_ERROR("failed to enable supply%d: %d\n",
					i, err);
				return err;
			}
			if (rgb->delay.power)
				mdelay(rgb->delay.power);
		}
	}

	for (i = 0; i < GPIO_MAX; i++) {
		if (rgb->enable_gpio[i]) {
			sunxi_drm_gpio_set_value(rgb->enable_gpio[i], 1);

			if (rgb->delay.enable)
				mdelay(rgb->delay.enable);
		}
	}

	if (rgb->reset_gpio)
		sunxi_drm_gpio_set_value(rgb->reset_gpio, 1);
	if (rgb->delay.reset)
		mdelay(rgb->delay.reset);

	return 0;
}

static int panel_rgb_enable(struct sunxi_drm_panel *panel)
{
	struct panel_rgb *rgb = to_panel_rgb(panel);

	DRM_INFO("%s\n", __func__);
	if (rgb->backlight) {
		/*backlight_set_brightness(rgb->backlight)*/
		backlight_enable(rgb->backlight);
	}

	return 0;
}


static int panel_rgb_disable(struct sunxi_drm_panel *panel)
{

	struct panel_rgb *rgb = to_panel_rgb(panel);

	if (rgb->backlight)
		backlight_disable(rgb->backlight);

	return 0;
}

static int panel_rgb_unprepare(struct sunxi_drm_panel *panel)
{
	struct panel_rgb *rgb = to_panel_rgb(panel);
	int i;

	for (i = GPIO_MAX; i > 0; i--) {
		if (rgb->enable_gpio[i - 1]) {
			sunxi_drm_gpio_set_value(rgb->enable_gpio[i - 1], 0);
			if (rgb->delay.enable)
				mdelay(rgb->delay.enable);
		}
	}

	if (rgb->reset_gpio)
		sunxi_drm_gpio_set_value(rgb->reset_gpio, 0);
	if (rgb->delay.reset)
		mdelay(rgb->delay.reset);

	for (i = POWER_MAX; i > 0; i--) {
		if (rgb->supply[i - 1]) {
			sunxi_drm_power_disable(rgb->supply[i - 1]);
			if (rgb->delay.power)
				mdelay(rgb->delay.power);
		}
	}

	return 0;
}

static const struct sunxi_drm_panel_funcs panel_rgb_funcs = {
	.disable = panel_rgb_disable,
	.unprepare = panel_rgb_unprepare,
	.prepare = panel_rgb_prepare,
	.enable = panel_rgb_enable,
	/*.get_modes = panel_rgb_get_modes,*/
};


static int panel_rgb_parse_dt(struct panel_rgb *rgb)
{
	char power_name[40] = {0}, gpio_name[40] = {0};
	int i = 0;
	ulong ret;

	ret = sunxi_of_get_panel_orientation(rgb->dev, &rgb->panel.orientation);

	for (i = 0; i < POWER_MAX; i++) {
		rgb->supply[i] = 0;
		snprintf(power_name, 40, "power%d-supply", i);

		ret = dev_read_u32(rgb->dev, power_name, &rgb->supply[i]);
		if (ret) {
			pr_err("failed to request regulator(%s): %d\n", power_name, ret);
		}
	}

	dev_read_u32(rgb->dev, "power-delay-ms", &rgb->delay.power);
	dev_read_u32(rgb->dev, "enable-delay-ms", &rgb->delay.enable);
	dev_read_u32(rgb->dev, "reset-delay-ms", &rgb->delay.reset);
	/* Get GPIOs and backlight controller. */
	for (i = 0; i < GPIO_MAX; i++) {
		snprintf(gpio_name, 40, "enable%d-gpios", i);

		ret = sunxi_drm_gpio_request(rgb->dev, gpio_name);
		if (ret < 0) {
			pr_err("failed to request %s GPIO: %d\n", gpio_name, ret);
		} else
			rgb->enable_gpio[i] = ret;
	}

	snprintf(gpio_name, 40, "reset-gpios");
	ret = sunxi_drm_gpio_request(rgb->dev, gpio_name);
	if (ret < 0) {
		pr_err("failed to request %s GPIO: %d\n", "reset", ret);
	} else
		rgb->reset_gpio = ret;

	return 0;

}

static int panel_rgb_probe(struct udevice *dev)
{
	struct panel_rgb *rgb = dev_get_priv(dev);
	struct sunxi_drm_panel *panel = &rgb->panel;
	int ret = 0;

	rgb->dev = dev;

	ret = panel_rgb_parse_dt(rgb);
	if (ret < 0)
		return ret;

	ret = uclass_get_device_by_phandle(UCLASS_PANEL_BACKLIGHT, dev,
					   "backlight", &rgb->backlight);
	if (ret) {
		DRM_INFO("%s: Cannot get backlight: %d\n", __func__, ret);
	}

	dev->driver_data = (ulong)panel;
	panel->dev = dev;
	panel->funcs = &panel_rgb_funcs;
	return 0;
}

static const struct udevice_id panel_rgb_of_table[] = {
	{ .compatible = "sunxi-rgb", },
	{ /* Sentinel */ },
};

U_BOOT_DRIVER(sunxi_rgb) = {
	.name	= "sunxi_rgb",
	.id	= UCLASS_PANEL,
	.of_match = panel_rgb_of_table,
	.probe	= panel_rgb_probe,
	.priv_auto_alloc_size = sizeof(struct panel_rgb),
};

//End of File
