/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI AXP519  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <sunxi_power/bmu_axp2601.h>
#include <sunxi_power/axp.h>
#include <asm/arch/pmic_bus.h>
#include <asm/io.h>
#include <spare_head.h>
#include <asm/arch/gpio.h>
#include <asm/gpio.h>
#include <sunxi_power/power_manage.h>
#include <sys_config.h>
#include <fdt_support.h>

#define AXP2601_VBUS_GPIO SUNXI_GPL(3)

int bmu_axp2601_probe(void)
{
	u8 bmu_chip_id;
	int axp_bus_num;

	script_parser_fetch(FDT_PATH_POWER_SPLY, "axp_bus_num", &axp_bus_num, AXP2601_DEVICE_ADDR);

	if (pmic_bus_init(axp_bus_num, AXP2601_RUNTIME_ADDR)) {
		tick_printf("axp2601 pmic_bus_init fail\n", __func__);
		return -1;
	}

	if (pmic_bus_read(AXP2601_RUNTIME_ADDR, AXP2601_CHIP_ID, &bmu_chip_id)) {
		tick_printf("axp2601 pmic_bus_read fail\n", __func__);
		return -1;
	}

	bmu_chip_id &= 0x1F;

	if (bmu_chip_id != AXP2601_CHIP_VER) {
		pmic_bus_clrbits(AXP2601_RUNTIME_ADDR, AXP2601_RESET_CFG, BIT(0));
		mdelay(500);
		if (pmic_bus_read(AXP2601_RUNTIME_ADDR, AXP2601_CHIP_ID, &bmu_chip_id)) {
			tick_printf("axp2601 pmic_bus_read fail\n", __func__);
			return -1;
		}
		bmu_chip_id &= 0x1F;
	}


	if (bmu_chip_id == AXP2601_CHIP_VER) {
		tick_printf("BMU: AXP2601\n");
		return 0;
	}

	tick_printf("BMU: NO FOUND\n");

	return -1;
}

int bmu_axp2601_get_battery_vol(void)
{
	u8 reg_value_h = 0, reg_value_l = 0;
	int i, vtemp[3];

	for (i = 0; i < 3; i++) {
		if (pmic_bus_read(AXP2601_RUNTIME_ADDR, AXP2601_VBAT_H,
				  &reg_value_h)) {
			return -1;
		}
		if (pmic_bus_read(AXP2601_RUNTIME_ADDR, AXP2601_VBAT_L,
				  &reg_value_l)) {
			return -1;
		}
		/*step 1mv*/
		vtemp[i] = ((reg_value_h << 8) | reg_value_l);
	}
	if (vtemp[0] > vtemp[1]) {
		vtemp[0] = vtemp[0] ^ vtemp[1];
		vtemp[1] = vtemp[0] ^ vtemp[1];
		vtemp[0] = vtemp[0] ^ vtemp[1];
	}
	if (vtemp[1] > vtemp[2]) {
		vtemp[1] = vtemp[2] ^ vtemp[1];
		vtemp[2] = vtemp[2] ^ vtemp[1];
		vtemp[1] = vtemp[2] ^ vtemp[1];
	}
	if (vtemp[0] > vtemp[1]) {
		vtemp[0] = vtemp[0] ^ vtemp[1];
		vtemp[1] = vtemp[0] ^ vtemp[1];
		vtemp[0] = vtemp[0] ^ vtemp[1];
	}
	return vtemp[1];
}

int bmu_axp2601_get_battery_probe(void)
{
	int bat_vol;
	bat_vol = bmu_axp2601_get_battery_vol();

	if (bat_vol < 3000)
		return BATTERY_NONE;
	else
		return BATTERY_IS_EXIST;
}

int bmu_axp2601_get_vbus_status(void)
{
	int vbus_status;

	gpio_request(AXP2601_VBUS_GPIO, "vbus_det");
	gpio_direction_input(AXP2601_VBUS_GPIO);
	sunxi_gpio_set_pull(AXP2601_VBUS_GPIO, SUNXI_GPIO_PULL_DOWN);
	vbus_status = gpio_get_value(AXP2601_VBUS_GPIO);

	if (vbus_status > 0)
		vbus_status = -1;
	else
		vbus_status = 1;

	return vbus_status;
}


int bmu_axp2601_battery_check(int ratio)
{
	int bat_vol, dcin_exist;
	u32 reg;

	bat_vol = bmu_get_battery_vol();
	dcin_exist = bmu_get_axp_bus_exist();

	if (!ratio) {
		if ((dcin_exist != AXP_VBUS_EXIST) && (bat_vol > 3700)) {
			pmic_bus_setbits(AXP2601_RUNTIME_ADDR, AXP2601_RESET_CFG, BIT(2));
			pmic_bus_clrbits(AXP2601_RUNTIME_ADDR, AXP2601_RESET_CFG, BIT(2));
			tick_printf("%s only battery reset gauge: soc = 0\n", __func__);
			return -1;
		}
		if (dcin_exist == AXP_VBUS_EXIST) {
			mdelay(500);
			pmic_bus_setbits(AXP2601_RUNTIME_ADDR, AXP2601_RESET_CFG, BIT(2));
			pmic_bus_clrbits(AXP2601_RUNTIME_ADDR, AXP2601_RESET_CFG, BIT(2));
			mdelay(500);
			tick_printf("%s adapt reset gauge: soc = 0\n", __func__);
			reg = readl(AXP2601_FLAGE_REG);
			reg &= ~(0xFF);
			reg |= 0x81;
			writel(reg, AXP2601_FLAGE_REG);
			reg = readl(AXP2601_FLAGE_REG);
			tick_printf("AXP2601_CURVE_CHECK:%x\n\n", reg);
			return -1;
		}
	}

	if (ratio > 60) {
		if (bat_vol < 3500) {
			mdelay(500);
			pmic_bus_setbits(AXP2601_RUNTIME_ADDR, AXP2601_RESET_CFG, BIT(2));
			pmic_bus_clrbits(AXP2601_RUNTIME_ADDR, AXP2601_RESET_CFG, BIT(2));
			mdelay(500);
			tick_printf("%s adapt reset gauge: soc > 60% , bat_vol < 3500\n", __func__);
			return -1;
		}
	}

	tick_printf("battery_check pass:radio:%d, vol:%d\n", ratio, bat_vol);
	return 0;
}

int bmu_axp2601_get_battery_capacity(void)
{
	u8 reg_value;
	u32 reg;
	int check;
	static int check_count = 1;

	if (axp_get_battery_exist() < 1)
		return 0;

	if (pmic_bus_read(AXP2601_RUNTIME_ADDR, AXP2601_GAUGE_SOC, &reg_value)) {
		return -1;
	}
	if (check_count == 1) {
		check = bmu_axp2601_battery_check(reg_value);
		if (check != 0) {
			if (pmic_bus_read(AXP2601_RUNTIME_ADDR, AXP2601_GAUGE_SOC,
					  &reg_value)) {
				return -1;
			}
			if (!reg_value) {
				reg = readl(AXP2601_FLAGE_REG);
				reg &= ~(0xFF);
				writel(reg, AXP2601_FLAGE_REG);
			}
		}
		check_count = 0;
	}
	return reg_value;
}

int bmu_axp2601_reset_capacity(void)
{
	if (pmic_bus_write(AXP2601_RUNTIME_ADDR, AXP2601_GAUGE_CONFIG, 0x00))
		return -1;

	return 1;
}

U_BOOT_AXP_BMU_INIT(bmu_axp2601) = {
	.bmu_name		  = "bmu_axp2601",
	.probe			  	 = bmu_axp2601_probe,
	.reset_capacity    = bmu_axp2601_reset_capacity,
	.get_battery_vol	  = bmu_axp2601_get_battery_vol,
	.get_battery_probe     = bmu_axp2601_get_battery_probe,
	.get_battery_capacity     = bmu_axp2601_get_battery_capacity,
	.get_axp_bus_exist     = bmu_axp2601_get_vbus_status,
};
