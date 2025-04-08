/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI AXP  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <sunxi_power/bmu_axp2101.h>
#include <sunxi_power/axp.h>
#include <asm/arch/pmic_bus.h>
#include <sys_config.h>
#include <sunxi_power/power_manage.h>

int bmu_axp2101_battery_probe_check(void)
{
	u8 reg_value;
	int ret = 0, bat_exist = 0;

	ret = script_parser_fetch(FDT_PATH_POWER_SPLY, "battery_exist", &bat_exist, 1);
	if (ret < 0)
		bat_exist = 1;

	if (!bat_exist) {
		pmic_bus_clrbits(AXP2101_RUNTIME_ADDR, AXP2101_BAT_DET, BIT(0));
		pmic_bus_clrbits(AXP2101_RUNTIME_ADDR, AXP2101_FUEL_GAUGE_CTL, BIT(1));
		pmic_bus_clrbits(AXP2101_RUNTIME_ADDR, AXP2101_FUEL_GAUGE_CTL, BIT(3));
		if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_VBUS_VOL_SET, &reg_value))
			return -1;
		reg_value &= ~(0xf);
		if (pmic_bus_write(AXP2101_RUNTIME_ADDR, AXP2101_VBUS_VOL_SET, reg_value))
			return -1;
	}
	return 0;
}

static int bmu_axp2101_probe(void)
{
	u8 bmu_chip_id;
	if (pmic_bus_init(AXP2101_DEVICE_ADDR, AXP2101_RUNTIME_ADDR)) {
		tick_printf("%s pmic_bus_init fail\n", __func__);
		return -1;
	}
	if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_VERSION, &bmu_chip_id)) {
		tick_printf("%s pmic_bus_read fail\n", __func__);
		return -1;
	}
	bmu_chip_id &= 0XCF;
	if (bmu_chip_id == 0x47 || bmu_chip_id == 0x4a) {
		/*bmu type AXP21*/
		tick_printf("BMU: AXP21\n");
		bmu_axp2101_battery_probe_check();
		return 0;
	}
	return -1;
}

int bmu_axp2101_set_power_off(void)
{
	u8 reg_value;
	int set_vol = 3300;

	if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_VOFF_THLD, &reg_value)) {
		return -1;
	}
	reg_value &= 0xf8;
	if (set_vol >= 2600 && set_vol <= 3300) {
		reg_value |= (set_vol - 2600) / 100;
	} else if (set_vol <= 2600) {
		reg_value |= 0x00;
	} else {
		reg_value |= 0x07;
	}
	if (pmic_bus_write(AXP2101_RUNTIME_ADDR, AXP2101_VOFF_THLD, reg_value)) {
		return -1;
	}

	return 0;
}


int bmu_axp2101_get_battery_probe(void)
{
	u8 reg_value;
	int ret = 0, bat_exist = 0;

	ret = script_parser_fetch(FDT_PATH_POWER_SPLY, "battery_exist", &bat_exist, 1);
	if (ret < 0)
		bat_exist = 1;

	if (!bat_exist)
		return -1;

	if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_COMM_STATUS0, &reg_value)) {
		return -1;
	}

	if (reg_value & 0x08)
		return 1;

	return -1;
}

/*
	boot_source	0x20		help			return

	power low	BIT0		boot button		AXP_BOOT_SOURCE_BUTTON
	irq		BIT1		IRQ LOW			AXP_BOOT_SOURCE_IRQ_LOW
	usb		BIT2		VBUS	insert		AXP_BOOT_SOURCE_VBUS_USB
	charge		BIT3		charge to 3.3v		AXP_BOOT_SOURCE_CHARGER
	battery		BIT4		battary in		AXP_BOOT_SOURCE_BATTERY
*/
int bmu_axp2101_get_poweron_source(void)
{
	uchar reg_value;
	if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_PWRON_STATUS, &reg_value)) {
		return -1;
	}
	switch (reg_value) {
	case (1 << AXP_BOOT_SOURCE_BUTTON): return AXP_BOOT_SOURCE_BUTTON;
	case (1 << AXP_BOOT_SOURCE_IRQ_LOW): return AXP_BOOT_SOURCE_IRQ_LOW;
	case (1 << AXP_BOOT_SOURCE_VBUS_USB):
					     if (bmu_axp2101_get_battery_probe() > -1)
						     return AXP_BOOT_SOURCE_CHARGER;
					     else
						     return AXP_BOOT_SOURCE_VBUS_USB;
	case (1 << AXP_BOOT_SOURCE_CHARGER): return AXP_BOOT_SOURCE_CHARGER;
	case (1 << AXP_BOOT_SOURCE_BATTERY): return AXP_BOOT_SOURCE_BATTERY;
	default: return -1;
	}

}

int bmu_axp2101_set_coulombmeter_onoff(int onoff)
{
	u8 reg_value;

	if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_FUEL_GAUGE_CTL,
			  &reg_value)) {
		return -1;
	}
	if (!onoff)
		reg_value &= ~(0x01 << 3);
	else
		reg_value |= (0x01 << 3);

	if (pmic_bus_write(AXP2101_RUNTIME_ADDR, AXP2101_FUEL_GAUGE_CTL,
			   reg_value)) {
		return -1;
	}
	return 0;
}

int bmu_axp2101_get_axp_bus_exist(void)
{
	u8 reg_value;
	if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_COMM_STATUS0, &reg_value)) {
		return -1;
	}
	/*bit1: 0: vbus not power,  1: power good*/
	if (reg_value & 0x20) {
		return AXP_VBUS_EXIST;
	}
	return 0;
}

int bmu_axp2101_get_battery_vol(void)
{
	u8 reg_value_h = 0, reg_value_l = 0;
	int i, vtemp[3];

	for (i = 0; i < 3; i++) {
		if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_BAT_AVERVOL_H6,
				  &reg_value_h)) {
			return -1;
		}
		if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_BAT_AVERVOL_L8,
				  &reg_value_l)) {
			return -1;
		}
		/*step 1mv*/
		vtemp[i] = ((reg_value_h & 0x3F) << 8) | reg_value_l;
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

int bmu_axp2101_get_battery_capacity(void)
{
	u8 reg_value;

	if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_BAT_PERCEN_CAL,
			  &reg_value)) {
		return -1;
	}
	return reg_value;
}


int bmu_axp2101_set_vbus_current_limit(int current)
{
	u8 reg_value;
	if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_VBUS_CUR_SET, &reg_value)) {
		return -1;
	}
	reg_value &= 0xf8;

	if (current >= 2000) {
		/*limit to 2000mA */
		reg_value |= 0x05;
	} else if (current >= 1500) {
		/* limit to 1500mA */
		reg_value |= 0x04;
	} else if (current >= 1000) {
		/* limit to 1000mA */
		reg_value |= 0x03;
	} else if (current >= 900) {
		/*limit to 900mA */
		reg_value |= 0x02;
	} else if (current >= 500) {
		/*limit to 500mA */
		reg_value |= 0x01;
	} else if (current >= 100) {
		/*limit to 100mA */
		reg_value |= 0x0;
	} else
		reg_value |= 0x01;

	tick_printf("Input current:%d mA\n", current);
	if (pmic_bus_write(AXP2101_RUNTIME_ADDR, AXP2101_VBUS_CUR_SET, reg_value)) {
		return -1;
	}
	return 0;
}

int bmu_axp2101_get_vbus_current_limit(void)
{
	uchar reg_value;
	if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_VBUS_CUR_SET, &reg_value)) {
		return -1;
	}
	reg_value &= 0x07;
	if (reg_value == 0x05) {
		printf("limit to 2000mA \n");
		return 2000;
	} else if (reg_value == 0x04) {
		printf("limit to 1500mA \n");
		return 1500;
	} else if (reg_value == 0x03) {
		printf("limit to 1000mA \n");
		return 1000;
	} else if (reg_value == 0x02) {
		printf("limit to 900mA \n");
		return 900;
	} else if (reg_value == 0x01) {
		printf("limit to 500mA \n");
		return 500;
	} else if (reg_value == 0x00) {
		printf("limit to 100mA \n");
		return 100;
	} else {
		printf("do not limit current \n");
		return 0;
	}
}
int bmu_axp2101_set_charge_current_limit(int current)
{
	u8 reg_value;
	int step;

	if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_CHARGE1, &reg_value)) {
		return -1;
	}
	reg_value &= ~0x1f;
	if (current > 2000) {
		current = 2000;
	}
	if (current <= 200)
		step = current / 25;
	else
		step = (current / 100) + 6;

	reg_value |= (step & 0x1f);
	if (pmic_bus_write(AXP2101_RUNTIME_ADDR, AXP2101_CHARGE1, reg_value)) {
		return -1;
	}

	return 0;
}

unsigned char bmu_axp2101_get_reg_value(unsigned char reg_addr)
{
	u8 reg_value;
	if (pmic_bus_read(AXP2101_RUNTIME_ADDR, reg_addr, &reg_value)) {
		return -1;
	}
	return reg_value;
}

unsigned char bmu_axp2101_set_reg_value(unsigned char reg_addr, unsigned char reg_value)
{
	unsigned char reg;
	if (pmic_bus_write(AXP2101_RUNTIME_ADDR, reg_addr, reg_value)) {
		return -1;
	}
	if (pmic_bus_read(AXP2101_RUNTIME_ADDR, reg_addr, &reg)) {
		return -1;
	}
	return reg;
}

int bmu_axp2101_reset_capacity(void)
{
	if (pmic_bus_clrbits(AXP2101_RUNTIME_ADDR, AXP210X_REG_CONFIG, BIT(4)))
		return -1;

	return 1;
}


int bmu_axp2101_set_ntc_cur(int ntc_cur)
{
	unsigned char reg_value;
	if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_TS_CFG, &reg_value)) {
			return -1;
	}
	reg_value &= 0xFC;

	if (ntc_cur < 40)
		reg_value |= 0x00;
	else if (ntc_cur < 50)
		reg_value |= 0x01;
	else if (ntc_cur < 60)
		reg_value |= 0x02;
	else
		reg_value |= 0x03;

	if (pmic_bus_write(AXP2101_RUNTIME_ADDR, AXP2101_TS_CFG, reg_value)) {
			return -1;
	}

	return reg_value;
}

int bmu_axp2101_set_ntc_onff(int onoff, int ntc_cur)
{
	unsigned char reg_value;
	if (!onoff) {
		/* disable ts cfg*/
		pmic_bus_clrbits(AXP2101_RUNTIME_ADDR, AXP2101_ADC_CH_EN0, BIT(1));
		pmic_bus_setbits(AXP2101_RUNTIME_ADDR, AXP2101_TS_CFG, BIT(4));
		pmic_bus_clrbits(AXP2101_RUNTIME_ADDR, AXP2101_TS_CFG, BIT(3));

		/* set disable dbg */
		if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_BAT_AVERVOL_H6, &reg_value))
			return -1;

		reg_value &= ~(1 << 7);
		reg_value &= ~(1 << 6);
		if (pmic_bus_write(AXP2101_RUNTIME_ADDR, AXP2101_BAT_AVERVOL_H6, reg_value))
			return -1;
		pmic_bus_clrbits(AXP2101_RUNTIME_ADDR, AXP2101_TS_H, BIT(6));
	} else {
		/* set disable dbg */
		if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_BAT_AVERVOL_H6, &reg_value))
			return -1;

		reg_value &= ~(1 << 7);
		reg_value &= ~(1 << 6);
		if (pmic_bus_write(AXP2101_RUNTIME_ADDR, AXP2101_BAT_AVERVOL_H6, reg_value))
			return -1;

		pmic_bus_clrbits(AXP2101_RUNTIME_ADDR, AXP2101_TS_H, BIT(6));

		/* enable ts cfg*/
		pmic_bus_setbits(AXP2101_RUNTIME_ADDR, AXP2101_ADC_CH_EN0, BIT(1));
		pmic_bus_clrbits(AXP2101_RUNTIME_ADDR, AXP2101_TS_CFG, BIT(4));
		pmic_bus_setbits(AXP2101_RUNTIME_ADDR, AXP2101_TS_CFG, BIT(3));

		bmu_axp2101_set_ntc_cur(ntc_cur);
	}

	return 0;
}

int axp_vts_to_temp(int data, int param[16])
{
	int temp;

	if (data < param[15])
		return 800;
	else if (data <= param[14]) {
		temp = 700 + (param[14]-data)*100/
		(param[14]-param[15]);
	} else if (data <= param[13]) {
		temp = 600 + (param[13]-data)*100/
		(param[13]-param[14]);
	} else if (data <= param[12]) {
		temp = 550 + (param[12]-data)*50/
		(param[12]-param[13]);
	} else if (data <= param[11]) {
		temp = 500 + (param[11]-data)*50/
		(param[11]-param[12]);
	} else if (data <= param[10]) {
		temp = 450 + (param[10]-data)*50/
		(param[10]-param[11]);
	} else if (data <= param[9]) {
		temp = 400 + (param[9]-data)*50/
		(param[9]-param[10]);
	} else if (data <= param[8]) {
		temp = 300 + (param[8]-data)*100/
		(param[8]-param[9]);
	} else if (data <= param[7]) {
		temp = 200 + (param[7]-data)*100/
		(param[7]-param[8]);
	} else if (data <= param[6]) {
		temp = 100 + (param[6]-data)*100/
		(param[6]-param[7]);
	} else if (data <= param[5]) {
		temp = 50 + (param[5]-data)*50/
		(param[5]-param[6]);
	} else if (data <= param[4]) {
		temp = 0 + (param[4]-data)*50/
		(param[4]-param[5]);
	} else if (data <= param[3]) {
		temp = -50 + (param[3]-data)*50/
		(param[3] - param[4]);
	} else if (data <= param[2]) {
		temp = -100 + (param[2]-data)*50/
		(param[2] - param[3]);
	} else if (data <= param[1]) {
		temp = -150 + (param[1]-data)*50/
		(param[1] - param[2]);
	} else if (data <= param[0]) {
		temp = -250 + (param[0]-data)*100/
		(param[0] - param[1]);
	} else
		temp = -250;
	return temp;
}

int bmu_axp2101_get_ntc_temp(int param[16])
{
	unsigned char reg_value[2];
	int temp, tmp;

	if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_TS_H, &reg_value[0])) {
			return -1;
		}
	if (pmic_bus_read(AXP2101_RUNTIME_ADDR, AXP2101_TS_L, &reg_value[1])) {
			return -1;
	}

	temp = (((reg_value[0] & GENMASK(5, 0)) << 0x08) | (reg_value[1]));
	tmp = temp * 500 / 1000;
	temp = axp_vts_to_temp(tmp, (int *)param);

	return temp;
}

U_BOOT_AXP_BMU_INIT(bmu_axp2101) = {
	.bmu_name		  = "bmu_axp2101",
	.probe			  = bmu_axp2101_probe,
	.set_power_off		  = bmu_axp2101_set_power_off,
	.get_poweron_source       = bmu_axp2101_get_poweron_source,
	.get_axp_bus_exist	= bmu_axp2101_get_axp_bus_exist,
	.set_coulombmeter_onoff   = bmu_axp2101_set_coulombmeter_onoff,
	.get_battery_vol	  = bmu_axp2101_get_battery_vol,
	.get_battery_capacity     = bmu_axp2101_get_battery_capacity,
	.get_battery_probe	= bmu_axp2101_get_battery_probe,
	.set_vbus_current_limit   = bmu_axp2101_set_vbus_current_limit,
	.get_vbus_current_limit   = bmu_axp2101_get_vbus_current_limit,
	.set_charge_current_limit = bmu_axp2101_set_charge_current_limit,
	.get_reg_value	   = bmu_axp2101_get_reg_value,
	.set_reg_value	   = bmu_axp2101_set_reg_value,
	.reset_capacity    = bmu_axp2101_reset_capacity,
	.set_ntc_onoff     = bmu_axp2101_set_ntc_onff,
	.get_ntc_temp      = bmu_axp2101_get_ntc_temp,
};
