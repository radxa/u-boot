// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd
 */

#include <dm.h>
#include <errno.h>
#include <linux/err.h>
#include <syscon.h>
#include <asm/arch-rockchip/clock.h>
#include <asm/arch-rockchip/cru_rv1103b.h>

int rockchip_get_clk(struct udevice **devp)
{
	return uclass_get_device_by_driver(UCLASS_CLK,
			DM_DRIVER_GET(rockchip_rv1103b_cru), devp);
}

void *rockchip_get_cru(void)
{
	struct rv1103b_clk_priv *priv;
	struct udevice *dev;
	int ret;

	ret = rockchip_get_clk(&dev);
	if (ret)
		return ERR_PTR(ret);

	priv = dev_get_priv(dev);

	return priv->cru;
}
