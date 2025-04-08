// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2016 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 */

#include <common.h>
#include <dm.h>
#include <backlight.h>
#include <pwm.h>
#include <asm/gpio.h>
#include <power/regulator.h>
#ifdef CONFIG_AW_DRM
#include "drm/sunxi_drm_helper_funcs.h"
#endif

#define GPIO_NUM_MAX 3
#define POWER_NUM_MAX 3

struct pwm_backlight_priv {
	u32 regulator;
	ulong enable_gpio[GPIO_NUM_MAX];
	int sunxi_pwm;

	struct udevice *reg;
	struct gpio_desc enable;
	struct udevice *pwm;

	uint channel;
	uint period_ns;
	uint polarity;
	uint default_level;
	uint min_level;
	uint max_level;
};

static int pwm_backlight_enable(struct udevice *dev)
{
	struct pwm_backlight_priv *priv = dev_get_priv(dev);
	uint duty_cycle;
	int ret, i;
#ifndef CONFIG_AW_DRM
	struct dm_regulator_uclass_platdata *plat;
	if (priv->reg) {
		plat = dev_get_uclass_platdata(priv->reg);
		debug("%s: Enable '%s', regulator '%s'/'%s'\n", __func__,
		      dev->name, priv->reg->name, plat->name);
		ret = regulator_set_enable(priv->reg, true);
		if (ret) {
			debug("%s: Cannot enable regulator for PWM '%s'\n",
			      __func__, dev->name);
			return ret;
		}
		mdelay(120);
	}

	duty_cycle = priv->period_ns * (priv->default_level - priv->min_level) /
		(priv->max_level - priv->min_level + 1);
	ret = pwm_set_config(priv->pwm, priv->channel, priv->period_ns,
			     duty_cycle);
	if (ret)
		return ret;
	ret = pwm_set_enable(priv->pwm, priv->channel, true);
	if (ret)
		return ret;
	mdelay(10);
	dm_gpio_set_value(&priv->enable, 1);
#else
	if (priv->regulator) {
		sunxi_drm_power_enable(priv->regulator, 0);
		mdelay(120);
	}
	duty_cycle = priv->period_ns * (priv->default_level - priv->min_level) /
		(priv->max_level - priv->min_level + 1);
	ret = pwm_config(priv->sunxi_pwm, duty_cycle, priv->period_ns);
	if (ret)
		return ret;
	ret = pwm_set_polarity(priv->sunxi_pwm, priv->polarity);
	if (ret)
		return ret;

	ret = pwm_enable(priv->sunxi_pwm);
	if (ret)
		return ret;
	mdelay(10);

	for (i = 0; i < GPIO_NUM_MAX; i++) {
		sunxi_drm_gpio_set_value(priv->enable_gpio[i], 1);
	}

#endif
	return 0;
}
static int pwm_backlight_disable(struct udevice *dev)
{
	struct pwm_backlight_priv *priv = dev_get_priv(dev);
	int ret, i;
#ifdef CONFIG_AW_DRM
	if (priv->regulator)
		sunxi_drm_power_disable(priv->regulator);

	ret = pwm_disable(priv->sunxi_pwm);
	if (ret)
		return ret;
	mdelay(10);

	for (i = 0; i < GPIO_NUM_MAX; i++) {
		sunxi_drm_gpio_set_value(priv->enable_gpio[i], 0);
	}

#endif
	return 0;
}

static int pwm_backlight_ofdata_to_platdata(struct udevice *dev)
{
	struct pwm_backlight_priv *priv = dev_get_priv(dev);
	struct ofnode_phandle_args args;
	int index, ret, count, len, i;
	const u32 *cell;
	char gpio_name[32] = {0};

	debug("%s: start\n", __func__);
#ifdef CONFIG_AW_DRM
	ret = dev_read_u32(dev, "power-supply", &priv->regulator);
	if (ret)
		debug("%s: Cannot get power supply: ret=%d\n", __func__, ret);

	priv->enable_gpio[0] = sunxi_drm_gpio_request(dev, "enable-gpios");

	for (i = 1; i < GPIO_NUM_MAX; i++) {
		snprintf(gpio_name, 32, "enable%d-gpios", i);
		priv->enable_gpio[i] = sunxi_drm_gpio_request(dev, gpio_name);
	}

	ret = dev_read_phandle_with_args(dev, "pwms", "#pwm-cells", -1, 0,
					 &args);
	if (ret) {
		debug("%s: Cannot get PWM phandle: ret=%d\n", __func__, ret);
		return ret;
	}
	priv->channel = args.args[0];
	priv->period_ns = args.args[1];
	priv->polarity = args.args[2];

	ret = pwm_request(priv->channel, "pwm_backlight");
	if (ret < 0) {
		pr_err("failed to request pwm\n");
		return ret;
	} else
		priv->sunxi_pwm = ret;

#else
	ret = uclass_get_device_by_phandle(UCLASS_REGULATOR, dev,
					   "power-supply", &priv->reg);
	if (ret)
		debug("%s: Cannot get power supply: ret=%d\n", __func__, ret);
	ret = gpio_request_by_name(dev, "enable-gpios", 0, &priv->enable,
				   GPIOD_IS_OUT);
	if (ret) {
		debug("%s: Warning: cannot get enable GPIO: ret=%d\n",
		      __func__, ret);
		if (ret != -ENOENT)
			return ret;
	}
	ret = dev_read_phandle_with_args(dev, "pwms", "#pwm-cells", 0, 0,
					 &args);
	if (ret) {
		debug("%s: Cannot get PWM phandle: ret=%d\n", __func__, ret);
		return ret;
	}

	ret = uclass_get_device_by_ofnode(UCLASS_PWM, args.node, &priv->pwm);
	if (ret) {
		debug("%s: Cannot get PWM: ret=%d\n", __func__, ret);
		return ret;
	}
	priv->channel = args.args[0];
	priv->period_ns = args.args[1];
#endif

	index = dev_read_u32_default(dev, "default-brightness-level", 255);
	cell = dev_read_prop(dev, "brightness-levels", &len);
	count = len / sizeof(u32);
	if (cell && count > index) {
		priv->default_level = fdt32_to_cpu(cell[index]);
		priv->max_level = fdt32_to_cpu(cell[count - 1]);
	} else {
		priv->default_level = index;
		priv->max_level = 255;
	}
	debug("%s: done\n", __func__);


	return 0;
}

static int pwm_backlight_probe(struct udevice *dev)
{
	return 0;
}

static const struct backlight_ops pwm_backlight_ops = {
	.enable	= pwm_backlight_enable,
	.disable = pwm_backlight_disable,
};

static const struct udevice_id pwm_backlight_ids[] = {
	{ .compatible = "pwm-backlight" },
	{ }
};

U_BOOT_DRIVER(pwm_backlight) = {
	.name	= "pwm_backlight",
	.id	= UCLASS_PANEL_BACKLIGHT,
	.of_match = pwm_backlight_ids,
	.ops	= &pwm_backlight_ops,
	.ofdata_to_platdata	= pwm_backlight_ofdata_to_platdata,
	.probe		= pwm_backlight_probe,
	.priv_auto_alloc_size	= sizeof(struct pwm_backlight_priv),
};
