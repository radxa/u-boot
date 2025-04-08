// SPDX-License-Identifier: GPL-2.0+
/*
 * Generic LVDS panel driver
 *
 * Copyright (C) 2016 Laurent Pinchart
 * Copyright (C) 2016 Renesas Electronics Corporation
 *
 * Contact: Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 */


#include <dm.h>
#include <linux/delay.h>
#include <drm/drm_print.h>
#include <drm/drm_modes.h>
#include <backlight.h>
#include "../sunxi_drm_panel.h"
#include "../sunxi_drm_helper_funcs.h"

#define POWER_MAX 3
#define GPIO_MAX  3

struct general_panel {
	struct sunxi_drm_panel panel;
	struct udevice *backlight;
	struct udevice *dev;
#ifdef DRM_USE_DM_POWER
	struct udevice *supply[POWER_MAX];
#else
	uint32_t supply[POWER_MAX];
#endif
#ifdef DRM_USE_DM_GPIO
	struct gpio_desc *enable_gpio[GPIO_MAX];
	struct gpio_desc *reset_gpio;
#else
	ulong enable_gpio[GPIO_MAX];
	ulong reset_gpio;
#endif
	unsigned int power_delay_ms;
	unsigned int disable_delay_ms;
};

static void general_panel_sleep(int msec)
{
	mdelay(msec);
}

static inline struct general_panel *to_general_panel(struct sunxi_drm_panel *panel)
{
	return container_of(panel, struct general_panel, panel);
}

static int general_panel_unprepare(struct sunxi_drm_panel *panel)
{
	struct general_panel *edp_panel = to_general_panel(panel);
	int i;

	for (i = GPIO_MAX; i > 0; i--) {
		if (edp_panel->enable_gpio[i - 1])
#ifdef DRM_USE_DM_GPIO
			dm_gpio_set_value(edp_panel->enable_gpio[i - 1], 0);
#else
			sunxi_drm_gpio_set_value(edp_panel->enable_gpio[i - 1], 0);
#endif
	}

	for (i = POWER_MAX; i > 0; i--) {
		if (edp_panel->supply[i - 1]) {
#ifdef DRM_USE_DM_POWER
			regulator_set_enable(edp_panel->supply[i - 1], false);
#else
			sunxi_drm_power_disable(edp_panel->supply[i - 1]);
#endif
		}
	}

	general_panel_sleep(edp_panel->disable_delay_ms);

	return 0;
}

static int general_panel_prepare(struct sunxi_drm_panel *panel)
{
	struct general_panel *edp_panel = to_general_panel(panel);
	int i;
	int err = 0;

	for (i = 0; i < POWER_MAX; i++) {
		if (edp_panel->supply[i]) {
#ifdef DRM_USE_DM_POWER
			err = regulator_set_enable(edp_panel->supply[i], true);
#else
			err = sunxi_drm_power_enable(edp_panel->supply[i], 0);
#endif
			if (err < 0) {
				dev_err(edp_panel->dev, "failed to enable supply%d: %d\n",
					i, err);
				return err;
			}
		}
	}

	for (i = 0; i < GPIO_MAX; i++) {
		if (edp_panel->enable_gpio[i])
#ifdef DRM_USE_DM_GPIO
			dm_gpio_set_value(edp_panel->enable_gpio[i], 1);
#else
			sunxi_drm_gpio_set_value(edp_panel->enable_gpio[i], 1);
#endif
	}

	general_panel_sleep(edp_panel->power_delay_ms);

	return 0;
}

static int general_panel_disable(struct sunxi_drm_panel *panel)
{
	struct general_panel *edp_panel = to_general_panel(panel);

	if (edp_panel->backlight)
		backlight_disable(edp_panel->backlight);

	return 0;
}

static int general_panel_enable(struct sunxi_drm_panel *panel)
{
	struct general_panel *edp_panel = to_general_panel(panel);

	if (edp_panel->backlight)
		backlight_enable(edp_panel->backlight);

	return 0;
}

static const struct sunxi_drm_panel_funcs general_panel_funcs = {
	.unprepare = general_panel_unprepare,
	.prepare = general_panel_prepare,
	.disable = general_panel_disable,
	.enable = general_panel_enable,
};

static int general_panel_parse_dts(struct general_panel *edp_panel)
{
	struct udevice *dev = edp_panel->dev;
	char power_name[32] = {0};
	char gpio_name[32] = {0};
	int ret;
	ulong gpio;
	int i = 0;

	ret = sunxi_of_get_panel_orientation(dev, &edp_panel->panel.orientation);
	if (ret < 0) {
		edp_panel->panel.orientation = DRM_MODE_PANEL_ORIENTATION_NORMAL;
	}

	for (i = 0; i < POWER_MAX; i++) {
		snprintf(power_name, 32, "power%d-supply", i);
		if (dev_read_bool(dev, power_name)) {
#ifdef DRM_USE_DM_POWER
			ret = uclass_get_device_by_phandle(UCLASS_REGULATOR, dev,
							   power_name, &edp_panel->supply[i]);
			if (IS_ERR(edp_panel->supply[i]) || ret) {
				DRM_ERROR("failed to request regulator(%s) for edp panel, ret:%d\n",
					power_name, ret);
				edp_panel->supply[i] = NULL;
			}
#else
			ret = dev_read_u32(dev, power_name, &edp_panel->supply[i]);
			if (!edp_panel->supply[i] || ret) {
				DRM_ERROR("failed to request regulator(%s) for edp panel, ret:%d\n",
					power_name, ret);
				edp_panel->supply[i] = 0;
			}
#endif
		}
	}

#ifdef DRM_USE_DM_GPIO
	/* Get GPIOs and backlight controller. */
	for (i = 0; i < GPIO_MAX; i++) {
		edp_panel->enable_gpio[i] = kmalloc(sizeof(struct gpio_desc), __GFP_ZERO);
		snprintf(gpio_name, 32, "enable%d-gpios", i);
		if (dev_read_bool(dev, gpio_name)) {
			ret = gpio_request_by_name(dev, gpio_name, 0,
						   edp_panel->enable_gpio[i], GPIOD_IS_OUT);
			if (IS_ERR(edp_panel->enable_gpio[i]) || ret) {
				DRM_ERROR("failed to request %s GPIO for edp panel, ret:%d\n",
					gpio_name, ret);
			}
		}
	}

	edp_panel->reset_gpio = kmalloc(sizeof(struct gpio_desc), __GFP_ZERO);
	if (dev_read_bool(dev, "reset-gpios")) {
		ret = gpio_request_by_name(dev, "reset-gpios", 0,
					   edp_panel->reset_gpio, GPIOD_IS_OUT);
		if (IS_ERR(edp_panel->reset_gpio) || ret) {
			dev_err(dev, "failed to request %s GPIO: %d\n", "reset", ret);
		}
	}
#else
	for (i = 0; i < GPIO_MAX; i++) {
		snprintf(gpio_name, 32, "enable%d-gpios", i);
		if (dev_read_bool(dev, gpio_name)) {
			gpio = sunxi_drm_gpio_request(dev, gpio_name);
			if (gpio < 0) {
				DRM_ERROR("failed to request %s GPIO for edp panel, ret:%d\n",
					gpio_name, ret);
			} else {
				edp_panel->enable_gpio[i] = gpio;
			}
		}
	}

	ret = dev_read_u32(dev, "power-delay-ms", &edp_panel->power_delay_ms);
	if (ret)
		edp_panel->power_delay_ms = 10;
	ret = dev_read_u32(dev, "disable-delay-ms", &edp_panel->disable_delay_ms);
	if (ret)
		edp_panel->disable_delay_ms = 10;

	if (dev_read_bool(dev, "reset-gpios")) {
		gpio = sunxi_drm_gpio_request(dev, "reset-gpios");
		if (gpio < 0) {
			DRM_ERROR("failed to request %s GPIO for edp panel, ret:%d\n",
				"reset", ret);
		} else {
			edp_panel->reset_gpio = gpio;
		}
	}
#endif

	return 0;
}

static int general_panel_probe(struct udevice *dev)
{
	struct general_panel *edp_panel = dev_get_priv(dev);
	struct sunxi_drm_panel *panel = &edp_panel->panel;
	int ret;

	edp_panel->dev = dev;

	ret = general_panel_parse_dts(edp_panel);
	if (ret < 0) {
		DRM_WARN("general edp panel timings parse from dts fail!\n");
	}

	ret = uclass_get_device_by_phandle(UCLASS_PANEL_BACKLIGHT, dev,
					   "backlight", &edp_panel->backlight);
	if (ret) {
		DRM_ERROR("%s: Cannot get backlight for edp panel, ret:%d\n", __func__, ret);
		edp_panel->backlight = NULL;
	}

	dev->driver_data = (ulong)panel;
	panel->dev = dev;
	panel->funcs = &general_panel_funcs;

	return 0;
}

static const struct udevice_id general_panel_of_table[] = {
	{
		.compatible = "edp-general-panel",
	},
	{ /* Sentinel */ },
};

U_BOOT_DRIVER(edp_general_panel) = {
	.name	= "edp_general_panel",
	.id	= UCLASS_PANEL,
	.of_match = general_panel_of_table,
	.probe	= general_panel_probe,
	.priv_auto_alloc_size = sizeof(struct general_panel),
};

