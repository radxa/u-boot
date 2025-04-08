/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI AXP  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <sunxi_power/bmu_axp2202.h>
#include <sunxi_power/axp.h>
#include <asm/arch/pmic_bus.h>
#include <asm/arch/rtc.h>
#include <sys_config.h>
#include <sunxi_power/power_manage.h>

int bmu_axp2202_get_battery_probe_check(void)
{
	u8 reg_value;
	int ret = 0, bat_exist = 0;

	ret = script_parser_fetch(FDT_PATH_POWER_SPLY, "battery_exist", &bat_exist, 1);
	if (ret < 0)
		bat_exist = 1;

	if (!bat_exist) {
		if (AXP2202_RUNTIME_ADDR == 0x34) {
			pmic_bus_clrbits(AXP2202_RUNTIME_ADDR, AXP2202_BAT_DET, BIT(0));
			pmic_bus_clrbits(AXP2202_RUNTIME_ADDR, AXP2202_MODULE_EN, BIT(1));
			pmic_bus_clrbits(AXP2202_RUNTIME_ADDR, AXP2202_CLK_EN, BIT(2));
			if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_VBUS_VOL_SET, &reg_value))
				return -1;
			reg_value &= ~(0xf);
			if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_VBUS_VOL_SET, reg_value))
				return -1;
		}
		return 0;
	}

	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_COMM_STATUS0, &reg_value)) {
		return -1;
	}

	if (reg_value & 0x08)
		return 0;

	pmic_bus_clrbits(AXP2202_RUNTIME_ADDR, AXP2202_ADC_CH_EN0, BIT(1));

	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_TS_CFG, &reg_value))
		return -1;

	reg_value |= (1 << 4);
	reg_value &= ~(3 << 2);
	if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_TS_CFG, reg_value))
		return -1;

	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_COMM_FAULT, &reg_value))
		return -1;

	reg_value |= 0x03;
	if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_COMM_FAULT, reg_value))
		return -1;

	if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_TS_CFG_DATA_H, 0x03))
		return -1;

	if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_TS_CFG_DATA_L, 0xEB))
		return -1;

	pmic_bus_clrbits(AXP2202_RUNTIME_ADDR, AXP2202_BAT_DET, BIT(0));
	mdelay(200);
	pmic_bus_setbits(AXP2202_RUNTIME_ADDR, AXP2202_BAT_DET, BIT(0));
	mdelay(2 * 1000);

	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_COMM_STATUS0, &reg_value)) {
		return -1;
	}

	tick_printf("battery probe reset :STATUS0:0x%x\n", reg_value);

	return 0;
}

static int bmu_axp2202_probe(void)
{
	u8 bmu_chip_id;
	if (pmic_bus_init(AXP2202_DEVICE_ADDR, AXP2202_RUNTIME_ADDR)) {
		tick_printf("%s pmic_bus_init fail\n", __func__);
		return -1;
	}
	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_CHIP_ID, &bmu_chip_id)) {
		tick_printf("%s pmic_bus_read fail\n", __func__);
		return -1;
	}
	bmu_chip_id &= 0Xff;

	if (bmu_chip_id == 0x01) {
		bmu_axp2202_get_battery_probe_check();
		/*bmu type AXP21*/
		tick_printf("BMU: AXP2202\n");

		if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_VERSION, &bmu_chip_id)) {
			tick_printf("%s pmic_bus_read fail\n", __func__);
			return -1;
		}

		bmu_chip_id &= 0Xff;

		if (bmu_chip_id == 0x00) {
			/*bmu type AXP21*/
			tick_printf("BMU: AXP2202 VER_A\n");
		}
		return 0;
	}

	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_CHIP_ID_EXT, &bmu_chip_id)) {
		tick_printf("%s pmic_bus_read fail\n", __func__);
		return -1;
	}

	if (bmu_chip_id == 0x02) {
		bmu_axp2202_get_battery_probe_check();
		tick_printf("BMU: AXP2202\n");
		return 0;
	}

	return -1;
}

int bmu_axp2202_set_power_off(void)
{
	u8 reg_value;
	int set_vol = 3200;

	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_VOFF_THLD, &reg_value)) {
		return -1;
	}
	reg_value &= 0x8f;
	if (set_vol >= 2600 && set_vol <= 3200) {
		reg_value |= (set_vol - 2600) / 100;
	} else if (set_vol <= 2600) {
		reg_value |= 0x00;
	} else {
		reg_value |= 0x70;
	}
	if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_VOFF_THLD, reg_value)) {
		return -1;
	}

	return 0;
}

int bmu_axp2202_reset_capacity(void)
{
	if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_GAUGE_CONFIG, 0x00))
		return -1;

	return 1;
}

int bmu_axp2202_get_sys_mode(void)
{
	int work_mode = get_boot_work_mode();
#ifndef CONFIG_BOOTCMD_SKIP_RTC
	u8 bootmode_flag = rtc_get_bootmode_flag();
#else
	u8 bootmode_flag = 0;
#endif
	u8 reg_value;

	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_DATA_BUFFER3, &reg_value)) {
		return -1;
	}

	reg_value &= 0x01;

	if ((work_mode == WORK_MODE_BOOT) && (bootmode_flag != SUNXI_EFEX_CMD_FLAG))
		pmic_bus_clrbits(AXP2202_RUNTIME_ADDR, AXP2202_DATA_BUFFER3, BIT(0));

	return reg_value;
}

static int bmu_axp2202_get_power_on_flag(void)
{

	u8 reg_value;
	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_BTN_CHG_CFG, &reg_value)) {
		return -1;
	}

	if (reg_value == 0x01) {
		reg_value = 1;
	} else {
		reg_value = 0;
	}

	return reg_value;
}

static int bmu_axp2202_set_power_on_flag(int status)
{
	if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_BTN_CHG_CFG,
			   (u8)status)) {
		return -1;
	}

	return 0;
}

/*
	boot_source	0x20		help			return

	power low	BIT0		boot button		AXP_BOOT_SOURCE_BUTTON
	irq		BIT1		IRQ LOW			AXP_BOOT_SOURCE_IRQ_LOW
	usb		BIT2		VBUS	insert		AXP_BOOT_SOURCE_VBUS_USB
	charge		BIT3		charge to 3.3v		AXP_BOOT_SOURCE_CHARGER
	battery		BIT4		battary in		AXP_BOOT_SOURCE_BATTERY
*/
int bmu_axp2202_get_poweron_source(void)
{
	uchar reg_value;

	/* if have reboot flag, then return -1*/
	if (bmu_axp2202_get_sys_mode())
		return -1;

	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_PWRON_STATUS, &reg_value)) {
		return -1;
	}
	switch (reg_value) {
	case (1 << AXP_BOOT_SOURCE_BUTTON): return AXP_BOOT_SOURCE_BUTTON;
	case (1 << AXP_BOOT_SOURCE_IRQ_LOW): return AXP_BOOT_SOURCE_IRQ_LOW;
	case (1 << AXP_BOOT_SOURCE_VBUS_USB): {
		if (bmu_get_battery_probe() > -1) {
			return AXP_BOOT_SOURCE_CHARGER;
		} else {
			return AXP_BOOT_SOURCE_VBUS_USB;
		}
	}
	case (1 << AXP_BOOT_SOURCE_CHARGER): return AXP_BOOT_SOURCE_CHARGER;
	case (1 << AXP_BOOT_SOURCE_BATTERY): return AXP_BOOT_SOURCE_BATTERY;
	default: return -1;
	}

}

int bmu_axp2202_set_coulombmeter_onoff(int onoff)
{
	u8 reg_value;

	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_FUEL_GAUGE_CTL,
			  &reg_value)) {
		return -1;
	}
	if (!onoff)
		reg_value &= ~(0x01 << 2);
	else
		reg_value |= (0x01 << 2);

	if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_FUEL_GAUGE_CTL,
			   reg_value)) {
		return -1;
	}
	return 0;
}

int bmu_axp2202_get_axp_bus_exist(void)
{
	u8 reg_value;
	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_COMM_STATUS0, &reg_value)) {
		return -1;
	}

	/*bit1: 0: vbus not power,  1: power good*/
	if (reg_value & 0x20) {
		return AXP_VBUS_EXIST;
	}
	return 0;
}

int _bmu_axp2202_get_battery_vol(void)
{
	u8 reg_value_h = 0, reg_value_l = 0;
	int i, vtemp[3];

	for (i = 0; i < 3; i++) {
		if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_BAT_AVERVOL_H6,
				  &reg_value_h)) {
			return -1;
		}
		if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_BAT_AVERVOL_L8,
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

int bmu_axp2202_get_battery_vol(void)
{
	int bat_vol;

	bat_vol = _bmu_axp2202_get_battery_vol();

	if (bat_vol < 3300) {
		printk("bat_vol(%d) is too low, need delay 1s to check.\n", bat_vol);
		mdelay(1000);
		bat_vol = _bmu_axp2202_get_battery_vol();
	}
	return bat_vol;
}

int bmu_axp2202_get_charge_status(void)
{
	u8 reg_value, charge_status;

#ifdef CONFIG_SUNXI_BMU_EXT
	return bmu_ext_get_charge();
#endif

	pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_MODE_CHGSTATUS, &reg_value);
	reg_value &= 0x7;

	/* chg_stat = bit[2:0] */
	switch (reg_value) {
	case 1:
	case 2:
	case 3:
		charge_status = 1;
		break;
	default:
		charge_status = 0;
		break;
	}

	return charge_status;

}
int bmu_axp2202_battery_check(int ratio)
{
	int bat_vol, dcin_exist;
	u8 reg_value;

	bat_vol = bmu_get_battery_vol();
	dcin_exist = bmu_get_axp_bus_exist();

	if (!ratio) {
		if (bmu_axp2202_get_charge_status() > 0) {
#ifdef CONFIG_SUNXI_BMU_EXT
			bmu_ext_set_discharge();
#endif
			pmic_bus_clrbits(AXP2202_RUNTIME_ADDR, AXP2202_MODULE_EN, BIT(1));
			mdelay(500);
			pmic_bus_setbits(AXP2202_RUNTIME_ADDR, AXP2202_RESET_CFG, BIT(2));
			pmic_bus_clrbits(AXP2202_RUNTIME_ADDR, AXP2202_RESET_CFG, BIT(2));
			mdelay(500);
			pmic_bus_setbits(AXP2202_RUNTIME_ADDR, AXP2202_MODULE_EN, BIT(1));
#ifdef CONFIG_SUNXI_BMU_EXT
			bmu_ext_set_charge();
#endif
			tick_printf("%s adapt reset gauge: soc = 0\n", __func__);
			pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_CURVE_CHECK, 0x81);
			pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_CURVE_CHECK, &reg_value);
			tick_printf("AXP2202_CURVE_CHECK:%x\n\n", reg_value);
			return -1;
		}
	}

	if ((ratio > 60) && (bat_vol < 3500)) {
		if (dcin_exist == AXP_VBUS_EXIST) {
#ifdef CONFIG_SUNXI_BMU_EXT
			bmu_ext_set_discharge();
#endif
			pmic_bus_clrbits(AXP2202_RUNTIME_ADDR, AXP2202_MODULE_EN, BIT(1));
			mdelay(500);
			pmic_bus_setbits(AXP2202_RUNTIME_ADDR, AXP2202_RESET_CFG, BIT(2));
			pmic_bus_clrbits(AXP2202_RUNTIME_ADDR, AXP2202_RESET_CFG, BIT(2));
			mdelay(500);
			pmic_bus_setbits(AXP2202_RUNTIME_ADDR, AXP2202_MODULE_EN, BIT(1));
#ifdef CONFIG_SUNXI_BMU_EXT
			bmu_ext_set_charge();
#endif
			tick_printf("%s adapt reset gauge: soc > 60% , bat_vol < 3500\n", __func__);
			pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_CURVE_CHECK, 0x81);
			pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_CURVE_CHECK, &reg_value);
			tick_printf("AXP2202_CURVE_CHECK:%x\n\n", reg_value);
			return -1;
		}
	}

	tick_printf("battery_check pass:radio:%d, vol:%d\n", ratio, bat_vol);
	return 0;
}


int bmu_axp2202_get_battery_capacity(void)
{
	u8 reg_value;
	int check;
	static int check_count = 1;

	if (bmu_get_battery_probe() < 0)
		return 0;

	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_BAT_PERCEN_CAL,
			  &reg_value)) {
		return -1;
	}

	if (check_count == 1) {
		check = bmu_axp2202_battery_check(reg_value);
		if (check != 0) {
			if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_BAT_PERCEN_CAL,
					  &reg_value)) {
				return -1;
			}
			if (!reg_value)
				pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_CURVE_CHECK, 0);
		}
		check_count = 0;
	}

	return reg_value;
}


int bmu_axp2202_get_battery_probe(void)
{
	u8 reg_value;
	int ret = 0, bat_exist = 0;
	int work_mode = get_boot_work_mode();
#ifndef CONFIG_BOOTCMD_SKIP_RTC
	u8 bootmode_flag = rtc_get_bootmode_flag();
#else
	u8 bootmode_flag = 0;
#endif

	if ((work_mode != WORK_MODE_BOOT) || (bootmode_flag == SUNXI_EFEX_CMD_FLAG))
		return BATTERY_NONE;

	ret = script_parser_fetch(FDT_PATH_POWER_SPLY, "battery_exist", &bat_exist, 1);
	if (ret < 0)
		bat_exist = 1;

	if (!bat_exist)
		return -1;

	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_COMM_STATUS0, &reg_value)) {
		return -1;
	}

	if (reg_value & 0x08)
		return 1;

	return -1;
}

int bmu_axp2202_set_vbus_current_limit(int current)
{
	u8 reg_value;
	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_VBUS_CUR_SET, &reg_value)) {
		return -1;
	}
	reg_value &= 0xc0;
	if ((current >= 100) && (current <= 3250))
		reg_value |= (current - 100) / 50;
	else if (current < 100)
		reg_value |= 0x00;
	else if (current > 3250)
		reg_value |= 0x3f;

	tick_printf("Input current:%d mA\n", current);
	if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_VBUS_CUR_SET, reg_value)) {
		return -1;
	}
	return 0;
}

int bmu_axp2202_get_vbus_current_limit(void)
{
	uchar reg_value;
	int current;
	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_VBUS_CUR_SET, &reg_value)) {
		return -1;
	}
	reg_value &= 0x3f;
	current =  (reg_value * 50) + 100;
	printf("limit to %d mA \n", current);
	return current;
}
int bmu_axp2202_set_charge_current_limit(int current)
{
	u8 reg_value;
	int step;

	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_CHARGE1, &reg_value)) {
		return -1;
	}
	reg_value &= ~0x3f;
	if (current > 3072) {
		current = 3072;
	}
	step = (current - 64) / 64;

	reg_value |= (step & 0x3f);
	if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_CHARGE1, reg_value)) {
		return -1;
	}

	return 0;
}

unsigned char bmu_axp2202_get_reg_value(unsigned char reg_addr)
{
	u8 reg_value;
	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, reg_addr, &reg_value)) {
		return -1;
	}
	return reg_value;
}

unsigned char bmu_axp2202_set_reg_value(unsigned char reg_addr, unsigned char reg_value)
{
	unsigned char reg;
	if (pmic_bus_write(AXP2202_RUNTIME_ADDR, reg_addr, reg_value)) {
		return -1;
	}
	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, reg_addr, &reg)) {
		return -1;
	}
	return reg;
}

int bmu_axp2202_set_ntc_cur(int ntc_cur)
{
	unsigned char reg_value;
	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_TS_CFG, &reg_value)) {
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

	if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_TS_CFG, reg_value)) {
			return -1;
	}

	return reg_value;
}

int bmu_axp2202_set_ntc_onff(int onoff, int ntc_cur)
{
	unsigned char reg_value;
	if (!onoff) {
		if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_ADC_CH_EN0, &reg_value))
			return -1;

		reg_value &= ~(1 << 1);
		if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_ADC_CH_EN0, reg_value))
			return -1;

		if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_TS_CFG, &reg_value))
			return -1;

		reg_value |= (1 << 4);
		reg_value &= ~(3 << 2);
		if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_TS_CFG, reg_value))
			return -1;

		if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_COMM_FAULT, &reg_value))
			return -1;

		reg_value |= 0x03;
		if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_COMM_FAULT, reg_value))
			return -1;

		if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_TS_CFG_DATA_H, 0x03))
			return -1;

		if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_TS_CFG_DATA_L, 0xEB))
			return -1;


	} else {
		if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_ADC_CH_EN0, &reg_value))
			return -1;

		reg_value |= (1 << 1);
		if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_ADC_CH_EN0, reg_value))
			return -1;

		if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_TS_CFG, &reg_value))
			return -1;

		reg_value &= ~(1 << 4);
		reg_value |= (3 << 2);
		if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_TS_CFG, reg_value))
			return -1;

		if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_COMM_FAULT, &reg_value))
			return -1;

		reg_value |= 0x03;
		if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_COMM_FAULT, reg_value))
			return -1;

		bmu_axp2202_set_ntc_cur(ntc_cur);

		if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_ADC_DATA_SEL, &reg_value)) {
			return -1;
		}

		reg_value &= ~(3 << 0);

		if (pmic_bus_write(AXP2202_RUNTIME_ADDR, AXP2202_ADC_DATA_SEL, reg_value))
			return -1;

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

static int _bmu_axp2202_get_ntc_adc(void)
{
	unsigned char reg_value[2];

	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_ADC_DATA_H, &reg_value[0])) {
			return -1;
		}
	if (pmic_bus_read(AXP2202_RUNTIME_ADDR, AXP2202_ADC_DATA_L, &reg_value[1])) {
			return -1;
	}

	return (((reg_value[0] & GENMASK(5, 0)) << 8) | (reg_value[1]));
}

static int bmu_axp2202_get_ntc_adc(void)
{
	int i, temp;

	temp = _bmu_axp2202_get_ntc_adc();

	if ((temp > 0) && (temp < 0x1FFF))
		return temp;

	for (i = 0; i < 3; i++) {
		mdelay(100);
		temp = _bmu_axp2202_get_ntc_adc();
		if ((temp > 0) && (temp < 0x1FFF))
			break;
	}

	return temp;
}

int bmu_axp2202_get_ntc_temp(int param[16])
{
	int temp, tmp;

	temp = bmu_axp2202_get_ntc_adc();
	tmp = temp * 500 / 1000;
	temp = axp_vts_to_temp(tmp, (int *)param);

	return temp;
}

U_BOOT_AXP_BMU_INIT(bmu_axp2202) = {
	.bmu_name		  = "bmu_axp2202",
	.probe			  = bmu_axp2202_probe,
	.set_power_off		  = bmu_axp2202_set_power_off,
	.get_poweron_source       = bmu_axp2202_get_poweron_source,
	.get_axp_bus_exist	= bmu_axp2202_get_axp_bus_exist,
	.set_coulombmeter_onoff   = bmu_axp2202_set_coulombmeter_onoff,
	.get_battery_vol	  = bmu_axp2202_get_battery_vol,
	.get_battery_capacity     = bmu_axp2202_get_battery_capacity,
	.get_battery_probe	= bmu_axp2202_get_battery_probe,
	.set_vbus_current_limit   = bmu_axp2202_set_vbus_current_limit,
	.get_vbus_current_limit   = bmu_axp2202_get_vbus_current_limit,
	.set_charge_current_limit = bmu_axp2202_set_charge_current_limit,
	.get_reg_value	   = bmu_axp2202_get_reg_value,
	.set_reg_value	   = bmu_axp2202_set_reg_value,
	.reset_capacity    = bmu_axp2202_reset_capacity,
	.set_ntc_onoff     = bmu_axp2202_set_ntc_onff,
	.get_ntc_temp      = bmu_axp2202_get_ntc_temp,
	.get_power_on_flag      = bmu_axp2202_get_power_on_flag,
	.set_power_on_flag      = bmu_axp2202_set_power_on_flag,
};
