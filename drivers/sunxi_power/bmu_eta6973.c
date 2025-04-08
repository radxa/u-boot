/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI ETA6973  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <sunxi_power/bmu_eta6973.h>
#include <sunxi_power/bmu_ext.h>
#include <sunxi_power/axp.h>
#include <asm/arch/pmic_bus.h>
#include <asm/io.h>
#include <spare_head.h>
#include <asm/arch/gpio.h>
#include <asm/gpio.h>
#include <sunxi_power/power_manage.h>
#include <sys_config.h>
#include <fdt_support.h>

#define ETA6973_BAT_GPIO SUNXI_GPL(7)
#define ETA6973_CHG_GPIO SUNXI_GPL(5)

static bool eta6973_exist;

int bmu_eta6973_probe(void)
{
	u8 bmu_chip_id;
	u8 reg_value = 0;

	eta6973_exist = false;
	pr_msg("EXT: EXT_probe\n");
	if (pmic_bus_init(ETA6973_DEVICE_ADDR, ETA6973_RUNTIME_ADDR)) {
		tick_printf("%s pmic_bus_init fail\n", __func__);
		return -1;
	}

	if (pmic_bus_read(ETA6973_RUNTIME_ADDR, ETA6973_REG_0B, &bmu_chip_id)) {
		tick_printf("%s pmic_bus_read fail\n", __func__);
		return -1;
	}

	bmu_chip_id &= 0x78;

	if (bmu_chip_id == ETA6973_CHIP_ID || bmu_chip_id == ETA6974_CHIP_ID) {
		tick_printf("EXT: ETA6973/ETA6974\n");
		/* set watchdog disable */
		pmic_bus_read(ETA6973_RUNTIME_ADDR, ETA6973_REG_05, &bmu_chip_id);
		bmu_chip_id &= ~(0x30);
		pmic_bus_write(ETA6973_RUNTIME_ADDR, ETA6973_REG_05, bmu_chip_id);
		pmic_bus_read(ETA6973_RUNTIME_ADDR, ETA6973_REG_05, &bmu_chip_id);

		/* set charge enable */
		gpio_request(ETA6973_CHG_GPIO, "quick_charge_en");
		gpio_direction_output(ETA6973_CHG_GPIO, 0);
		sunxi_gpio_set_pull(ETA6973_CHG_GPIO, SUNXI_GPIO_PULL_DOWN);

		pmic_bus_read(ETA6973_RUNTIME_ADDR, ETA6973_REG_01, &reg_value);
		reg_value |= 0x10;
		pmic_bus_write(ETA6973_RUNTIME_ADDR, ETA6973_REG_01, reg_value);

		/* set default chgcur to 2A */
		pmic_bus_read(ETA6973_RUNTIME_ADDR, ETA6973_REG_02, &reg_value);
		reg_value &= ~(0x3F);
		reg_value |= 0x22;
		pmic_bus_write(ETA6973_RUNTIME_ADDR, ETA6973_REG_02, reg_value);

		/* set default ovp to 10.5v */
		pmic_bus_read(ETA6973_RUNTIME_ADDR, ETA6973_REG_06, &reg_value);
		reg_value &= ~(0xC0);
		reg_value |= 0x80;
		pmic_bus_write(ETA6973_RUNTIME_ADDR, ETA6973_REG_06, reg_value);

		/* set default sys_vol to 3.7v */
		pmic_bus_read(ETA6973_RUNTIME_ADDR, ETA6973_REG_01, &reg_value);
		reg_value &= ~(0x0E);
		reg_value |= 0x0E;
		pmic_bus_write(ETA6973_RUNTIME_ADDR, ETA6973_REG_01, reg_value);

		/* clear batfet */
		pmic_bus_read(ETA6973_RUNTIME_ADDR, ETA6973_REG_07, &reg_value);
		reg_value &= ~(0x20);
		pmic_bus_write(ETA6973_RUNTIME_ADDR, ETA6973_REG_07, reg_value);

		eta6973_exist = true;
		return 0;
	}

	tick_printf("EXT: NO FOUND\n");

	return -1;
}

int bmu_eta6973_set_charge(void)
{
	u8 reg_value = 0;

	/* set charge enable */
	gpio_request(ETA6973_CHG_GPIO, "quick_charge_en");
	gpio_direction_output(ETA6973_CHG_GPIO, 0);
	sunxi_gpio_set_pull(ETA6973_CHG_GPIO, SUNXI_GPIO_PULL_DOWN);

	pmic_bus_read(ETA6973_RUNTIME_ADDR, ETA6973_REG_01, &reg_value);
	reg_value |= 0x10;
	pmic_bus_write(ETA6973_RUNTIME_ADDR, ETA6973_REG_01, reg_value);

	tick_printf("EXT: %s\n", __func__);
	return 0;
}

int bmu_eta6973_set_discharge(void)
{
	u8 reg_value = 0;

	/* set charge disable */
	gpio_request(ETA6973_CHG_GPIO, "quick_charge_en");
	gpio_direction_output(ETA6973_CHG_GPIO, 1);
	sunxi_gpio_set_pull(ETA6973_CHG_GPIO, SUNXI_GPIO_PULL_UP);

	pmic_bus_read(ETA6973_RUNTIME_ADDR, ETA6973_REG_01, &reg_value);
	reg_value &= ~(0x10);
	pmic_bus_write(ETA6973_RUNTIME_ADDR, ETA6973_REG_01, reg_value);
	tick_printf("EXT: %s\n", __func__);
	return 0;
}

int bmu_eta6973_get_vbus_status(void)
{
	u8 reg_value = 0;

	if (pmic_bus_read(ETA6973_RUNTIME_ADDR, ETA6973_REG_0A, &reg_value))
		return -1;

	if ((reg_value & 0x80) == 0x80)
		return AXP_VBUS_EXIST;

	return 0;
}

int bmu_eta6973_get_poweron_source(int poweron_source)
{
	unsigned int reg_value = 0;

	switch (poweron_source) {
	case AXP_BOOT_SOURCE_IRQ_LOW:
		reg_value = readl(SUNXI_RTC_BASE+0x30);
		if (!reg_value) {
			if ((bmu_get_battery_probe() > -1) && bmu_ext_get_vbus_status()) {
				if (pmu_get_sys_mode() == SUNXI_CHARGING_FLAG) {
					pmu_set_sys_mode(0);
					poweron_source = AXP_BOOT_SOURCE_CHARGER;
				}
				if (bmu_get_power_on_flag() == 0) {
					poweron_source = AXP_BOOT_SOURCE_CHARGER;
				}
			}
		}
	default:
		bmu_set_power_on_flag(1);
		return poweron_source;
	}

	bmu_set_power_on_flag(1);
	return poweron_source;
}

int bmu_eta6973_get_battery_probe(void)
{
	int battery_status;

	gpio_request(ETA6973_BAT_GPIO, "quick_charge_en");
	gpio_direction_input(ETA6973_BAT_GPIO);
	sunxi_gpio_set_pull(ETA6973_BAT_GPIO, SUNXI_GPIO_PULL_DOWN);
	battery_status = gpio_get_value(ETA6973_BAT_GPIO);

	if (battery_status > 0)
		battery_status = -1;
	else
		battery_status = 1;

	return battery_status;
}

inline bool get_eta6973(void)
{
	return eta6973_exist;
}

int bmu_eta6973_get_charge(void)
{
	u8 reg_value = 0, charge_status;

	pmic_bus_read(ETA6973_RUNTIME_ADDR, ETA6973_REG_08, &reg_value);

	reg_value &= 0x18;

	/* chg_stat = bit[2:0] */
	switch (reg_value >> 3) {
	case 1:
	case 2:
		charge_status = 1;
		break;
	default:
		charge_status = 0;
		break;
	}

	return charge_status;
}

U_BOOT_BMU_EXT_INIT(bmu_eta6973) = {
	.bmu_ext_name	 	 = "bmu_eta6973",
	.get_exist	   		 = get_eta6973,
	.probe			  	 = bmu_eta6973_probe,
	.get_vbus_status     = bmu_eta6973_get_vbus_status,
	.get_poweron_source     = bmu_eta6973_get_poweron_source,
	.get_battery_probe     = bmu_eta6973_get_battery_probe,
	.set_discharge  = bmu_eta6973_set_discharge,
	.set_charge     = bmu_eta6973_set_charge,
	.get_charge     = bmu_eta6973_get_charge,
};
