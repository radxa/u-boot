/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI AXP  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <sunxi_power/bmu_axp515.h>
#include <sunxi_power/axp.h>
#include <asm/arch/pmic_bus.h>
#include <sys_config.h>
#include <sunxi_power/power_manage.h>

static int bmu_axp515_probe(void)
{
	u8 bmu_chip_id;
	u8 reg_val;

	if (pmic_bus_init(AXP515_DEVICE_ADDR, AXP515_RUNTIME_ADDR)) {
		tick_printf("%s pmic_bus_init f11ail\n", __func__);
		return -1;
	}
	if (pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_CHIP_ID,
			  &bmu_chip_id)) {
		tick_printf("%s pmic_bus_read fail\n", __func__);
		return -1;
	}
	bmu_chip_id &= 0XCF;
	if (bmu_chip_id == 0x46 || bmu_chip_id == 0x49) {
		/*bmu type AXP515*/
		tick_printf("BMU: AXP515\n");
		/*
		 * when use this bit to shutdown system, it must write 0
		 * when system power on
		 */
		pmic_bus_clrbits(AXP515_RUNTIME_ADDR, AXP515_ILIMIT, BIT(7));
		/* set input limit to 3A */
		pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_ILIMIT, &reg_val);
		reg_val &= ~(0x3f);
		reg_val |= 0x3A;
		pmic_bus_write(AXP515_RUNTIME_ADDR, AXP515_ILIMIT, reg_val);
		/* set vindpm to 4.6V */
		pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_RBFET_SET, &reg_val);
		reg_val &= ~(0xf);
		reg_val |= 0x9;
		pmic_bus_write(AXP515_RUNTIME_ADDR, AXP515_RBFET_SET, reg_val);
		/* dafault auto sets disabled */
		pmic_bus_clrbits(AXP515_RUNTIME_ADDR, AXP515_AUTO_SETS, BIT(5));
		/* disabled idpm */
		pmic_bus_setbits(AXP515_RUNTIME_ADDR, AXP515_DPM_LOOP_SET, BIT(6));
		pmic_bus_setbits(AXP515_RUNTIME_ADDR, AXP515_DPM_LOOP_SET, BIT(4));
		pmic_bus_setbits(AXP515_RUNTIME_ADDR, AXP515_DPM_LOOP_SET, BIT(3));

		return 0;
	}
	return -1;
}

static void bmu_axp515_clear_irq(void)
{
	u8 reg;

	for (reg = AXP515_INTEN1; reg <= AXP515_INTEN6; reg++) {
		pmic_bus_write(AXP515_RUNTIME_ADDR, reg, 0x00);
	}

	for (reg = AXP515_INTSTS1; reg <= AXP515_INTSTS6; reg++) {
		pmic_bus_write(AXP515_RUNTIME_ADDR, reg, 0xFF);
	}

	pmic_bus_write(AXP515_RUNTIME_ADDR, AXP515_INTSTS3, 0xc0);
	pmic_bus_write(AXP515_RUNTIME_ADDR, AXP515_INTEN4, 0xf0);

}

int bmu_axp515_set_power_off(void)
{
	u8 reg_value;

	bmu_axp515_clear_irq();

	pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_BATFET_DLY, &reg_value);
	reg_value &= ~(0x30);
	pmic_bus_write(AXP515_RUNTIME_ADDR, AXP515_BATFET_DLY, reg_value);

	pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_ILIMIT, &reg_value);
	reg_value |= 0x80;
	pmic_bus_write(AXP515_RUNTIME_ADDR, AXP515_ILIMIT, reg_value);

	return 0;
}

/*
	boot_source	0x4A		help			return
	(irq flag)

	usb		BIT7		VBUS	insert		AXP_BOOT_SOURCE_VBUS_USB
	battery		BIT5		battary in		AXP_BOOT_SOURCE_BATTERY

	boot_source	0x4B		help			return
	power low	0xf0		boot button		AXP_BOOT_SOURCE_BUTTON
*/
int bmu_axp515_get_poweron_source(void)
{
	uchar reg_value[2];
	int reboot_mode;

	reboot_mode = pmu_get_sys_mode();
	tick_printf("[AXP515] charge/reboot status:0x%x\n", reboot_mode);
	/* if have reboot flag, then return -1*/
	if (reboot_mode == SUNXI_REBOOT_FLAG) {
		pmu_set_sys_mode(0);
		return -1;
	}

	if (pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_INTSTS3,
			  &reg_value[0])) {
		return -1;
	}

	if (pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_INTSTS4,
			  &reg_value[1])) {
		return -1;
	}

	/*bit7 : vbus, bit6: battery inster*/
	if (reg_value[0] & (1 << 7) || reg_value[0] & (1 << 6)) {
		reg_value[0] &= 0xC0;
		pmic_bus_write(AXP515_RUNTIME_ADDR, AXP515_INTSTS3, reg_value[0]);
		if (axp_get_battery_exist() > 0) {
			return AXP_BOOT_SOURCE_CHARGER;
		} else {
			return AXP_BOOT_SOURCE_VBUS_USB;
		}
	} else if (reg_value[0] & (1 << 5)) {
		reg_value[0] &= 0x20;
		pmic_bus_write(AXP515_RUNTIME_ADDR, AXP515_INTSTS3, reg_value[0]);
		return AXP_BOOT_SOURCE_BATTERY;
	} else if (reg_value[1] & 0xF0) {
		reg_value[1] &= 0xF0;
		pmic_bus_write(AXP515_RUNTIME_ADDR, AXP515_INTSTS4, reg_value[1]);
		return AXP_BOOT_SOURCE_BUTTON;
	}

	return -1;
}

int bmu_axp515_get_axp_bus_exist(void)
{
	u8 reg_value;
	if (pmic_bus_read(AXP515_RUNTIME_ADDR, BMU_CHG_STATUS, &reg_value)) {
		return -1;
	}
	/*bit1: 0: vbus not power,  1: power good*/
	if (reg_value & 0x2) {
		return AXP_VBUS_EXIST;
	}
	return 0;
}

int bmu_axp515_get_battery_vol(void)
{
	u8 reg_value_h = 0, reg_value_l = 0;
	int tmp_value = 0;
	int bat_vol;

	if (pmic_bus_read(AXP515_RUNTIME_ADDR, BMU_BAT_VOL_H, &reg_value_h)) {
		return -1;
	}
	if (pmic_bus_read(AXP515_RUNTIME_ADDR, BMU_BAT_VOL_L, &reg_value_l)) {
		return -1;
	}

	tmp_value = (reg_value_h << 4) | reg_value_l;
	bat_vol   = tmp_value * 12 / 10;

	return bat_vol;
}

int bmu_axp515_get_battery_capacity(void)
{
	u8 reg_value;

	if (pmic_bus_read(AXP515_RUNTIME_ADDR, BMU_BAT_PERCENTAGE,
			  &reg_value)) {
		return -1;
	}

	//bit7-- 1:valid 0:invalid
	if (reg_value & (1 << 7)) {
		reg_value = reg_value & 0x7f;
	} else {
		reg_value = 0;
	}

	return reg_value;
}

int _bmu_axp515_get_battery_probe_with_bat_temp(void)
{
	unsigned char reg_value[2];
	int vbus_exist;
	u16 temp;

	vbus_exist = bmu_axp515_get_axp_bus_exist();

	if (vbus_exist != AXP_VBUS_EXIST)
		return BATTERY_IS_EXIST;

	/* get ts vol*/
	bmu_set_ntc_onoff(1, 20);
	if (pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_VTS_RES_H, &reg_value[0])) {
			return -1;
	}
	if (pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_VTS_RES_L, &reg_value[1])) {
			return -1;
	}

	temp = ((reg_value[0] << 0x04) | (reg_value[1] & 0xf));
	pr_msg("[AXP515] battery temp:0x%x\n", temp);

	if (temp > 0xBB8) {
		/* get die vol*/
		if (pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_DIE_TEMP_H, &reg_value[0])) {
				return -1;
		}
		if (pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_DIE_TEMP_L, &reg_value[1])) {
				return -1;
		}
		temp = ((reg_value[0] << 0x04) | (reg_value[1] & 0xf));
		pr_msg("[AXP515] die temp:0x%x\n", temp);

		if (temp > 0x9D7)
			return BATTERY_NONE;
	}

	return BATTERY_IS_EXIST;
}

int _bmu_axp515_get_battery_probe_with_bat_vol(void)
{
	int vbus_exist, bat_vol;

	vbus_exist = bmu_axp515_get_axp_bus_exist();

	if (vbus_exist != AXP_VBUS_EXIST)
		return BATTERY_IS_EXIST;

	bmu_set_ntc_onoff(0, 0);
	pmic_bus_clrbits(AXP515_RUNTIME_ADDR, AXP515_IPRECHG_CFG, BIT(7));

	mdelay(2 * 1000);
	bat_vol = bmu_axp515_get_battery_vol();
	tick_printf("%s bat_vol:%d\n", __func__, bat_vol);
	if (bat_vol < 3000)
		return BATTERY_NONE;

	pmic_bus_setbits(AXP515_RUNTIME_ADDR, AXP515_IPRECHG_CFG, BIT(7));

	return BATTERY_IS_EXIST;
}

int bmu_axp515_get_battery_probe(void)
{
	int work_mode = get_boot_work_mode();
	static int bat_exist, check_count;
	int ret, ts_bat_detect;

	if (work_mode != WORK_MODE_BOOT)
		return BATTERY_NONE;

	if (check_count)
		return bat_exist;

	ret = script_parser_fetch(FDT_PATH_POWER_SPLY, "battery_exist", &bat_exist, 1);
	if (ret < 0)
		bat_exist = 1;

	ret = script_parser_fetch(FDT_PATH_POWER_SPLY, "ts_bat_detect", &ts_bat_detect, 1);
	if (ret < 0)
		ts_bat_detect = 1;

	if (bat_exist) {
		if (ts_bat_detect)
			bat_exist = _bmu_axp515_get_battery_probe_with_bat_temp();
		else
			bat_exist = _bmu_axp515_get_battery_probe_with_bat_vol();
	}

	tick_printf("[AXP515] battery exist:%d\n", bat_exist);
	check_count = 1;

	return bat_exist;
}

int bmu_axp515_set_vbus_current_limit(int current)
{
	u8 reg_value;
	u8 temp;
	if (pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_ILIMIT, &reg_value)) {
		return -1;
	}
	if (current) {
		if (current > 3250) {
			temp = 0x3F;
		} else if (current >= 100) {
			temp = (current - 100) / 50;
		} else {
			temp = 0x00;
		}
	} else {
		/*default was 2500ma*/
		temp = 0x30;
	}
	reg_value &= 0xB0;
	reg_value |= temp;
	tick_printf("[AXP515]Input current:%d mA\n", current);
	if (pmic_bus_write(AXP515_RUNTIME_ADDR, AXP515_ILIMIT, reg_value)) {
		return -1;
	}

	if (pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_DPM_LOOP_SET, &reg_value)) {
		return -1;
	}
	if ((current < 3000) && (reg_value & BIT(4)))
		pmic_bus_clrbits(AXP515_RUNTIME_ADDR, AXP515_DPM_LOOP_SET, BIT(4));
	else if  ((current >= 3000) && (!(reg_value & BIT(4))))
		pmic_bus_setbits(AXP515_RUNTIME_ADDR, AXP515_DPM_LOOP_SET, BIT(4));

	return 0;
}

unsigned char bmu_axp515_get_reg_value(unsigned char reg_addr)
{
	u8 reg_value;
	if (pmic_bus_read(AXP515_RUNTIME_ADDR, reg_addr, &reg_value)) {
		return -1;
	}
	return reg_value;
}

unsigned char bmu_axp515_set_reg_value(unsigned char reg_addr, unsigned char reg_value)
{
	unsigned char reg;
	if (pmic_bus_write(AXP515_RUNTIME_ADDR, reg_addr, reg_value)) {
		return -1;
	}
	if (pmic_bus_read(AXP515_RUNTIME_ADDR, reg_addr, &reg)) {
		return -1;
	}
	return reg;
}

int bmu_axp515_set_ntc_cur(int ntc_cur)
{
	unsigned char reg_value;

	if (pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_TS_PIN_CONTROL, &reg_value)) {
			return -1;
	}
	reg_value &= 0xF9;

	if (ntc_cur < 40)
		reg_value |= 0x00 << 1;
	else if (ntc_cur < 60)
		reg_value |= 0x01 << 1;
	else if (ntc_cur < 80)
		reg_value |= 0x02 << 1;
	else
		reg_value |= 0x03 << 1;

	if (pmic_bus_write(AXP515_RUNTIME_ADDR, AXP515_TS_PIN_CONTROL, reg_value)) {
			return -1;
	}
	return reg_value;
}

int bmu_axp515_set_ntc_onff(int onoff, int ntc_cur)
{
	unsigned char reg_value;
	if (!onoff) {
		if (pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_TS_PIN_CONTROL, &reg_value))
			return -1;

		reg_value |= 1 << 7;

		if (pmic_bus_write(AXP515_RUNTIME_ADDR, AXP515_TS_PIN_CONTROL, reg_value))
			return -1;
	} else {

		if (pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_TS_PIN_CONTROL, &reg_value))
			return -1;

		reg_value |= 0x18;
		reg_value &= ~(1 << 7);

		if (pmic_bus_write(AXP515_RUNTIME_ADDR, AXP515_TS_PIN_CONTROL, reg_value))
			return -1;

		bmu_axp515_set_ntc_cur(ntc_cur);
		mdelay(10);
	}
	return 0;
}

static int axp_vts_to_temp(int data, int param[16])
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

int bmu_axp515_get_ntc_temp(int param[16])
{
	unsigned char reg_value[2];
	int temp, tmp;

	if (pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_VTS_RES_H, &reg_value[0])) {
			return -1;
	}

	if (pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_VTS_RES_L, &reg_value[1])) {
			return -1;
	}

	temp = ((reg_value[0] << 0x04) | (reg_value[1] & 0xf));
	tmp = temp * 800 / 1000;
	temp = axp_vts_to_temp(tmp, (int *)param);

	return temp;
}

int bmu_axp515_reg_debug(void)
{
	u8 reg_value[2];

	bmu_axp515_get_battery_probe();

	if (pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_INTSTS3, &reg_value[0])) {
		return -1;
	}
	if (pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_INTSTS4, &reg_value[1])) {
		return -1;
	}
	tick_printf("[AXP515] poweron irq: 0x%x:0x%x, 0x%x:0x%x\n", AXP515_INTSTS3, reg_value[0], AXP515_INTSTS4, reg_value[1]);

	if (pmic_bus_read(AXP515_RUNTIME_ADDR, AXP515_ONOFF_STATUS, &reg_value[0])) {
		return -1;
	}
	tick_printf("[AXP515] on/off status 0x%x:0x%x\n", AXP515_ONOFF_STATUS, reg_value[0]);
	return 0;
}

int bmu_axp515_reset_capacity(void)
{
	pmic_bus_write(AXP515_RUNTIME_ADDR, AXP515_BATCAP0, 0);
	pmic_bus_write(AXP515_RUNTIME_ADDR, AXP515_BATCAP1, 0);
	pmic_bus_clrbits(AXP515_RUNTIME_ADDR, AXP515_BATFET_DLY, BIT(3));

	return 1;
}

U_BOOT_AXP_BMU_INIT(bmu_axp515) = {
	.bmu_name	   = "bmu_axp515",
	.probe		    = bmu_axp515_probe,
	.set_power_off      = bmu_axp515_set_power_off,
	.get_poweron_source = bmu_axp515_get_poweron_source,
	.get_axp_bus_exist  = bmu_axp515_get_axp_bus_exist,
	/*.set_coulombmeter_onoff	= bmu_axp515_set_coulombmeter_onoff,*/
	.get_battery_vol	= bmu_axp515_get_battery_vol,
	.get_battery_capacity   = bmu_axp515_get_battery_capacity,
	.get_battery_probe      = bmu_axp515_get_battery_probe,
	.set_vbus_current_limit = bmu_axp515_set_vbus_current_limit,
	/*.get_vbus_current_limit		= bmu_axp515_get_vbus_current_limit,*/
	/*.set_charge_current_limit	= bmu_axp515_set_charge_current_limit,*/
	.get_reg_value = bmu_axp515_get_reg_value,
	.set_reg_value = bmu_axp515_set_reg_value,
	.set_ntc_onoff     = bmu_axp515_set_ntc_onff,
	.get_ntc_temp      = bmu_axp515_get_ntc_temp,
	.reg_debug         = bmu_axp515_reg_debug,
	.reset_capacity    = bmu_axp515_reset_capacity,
};
