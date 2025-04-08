/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI AXP  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <sunxi_power/bmu_axp517.h>
#include <sunxi_power/axp.h>
#include <asm/arch/pmic_bus.h>
#include <sys_config.h>
#include <sunxi_power/power_manage.h>


static inline int axp517_vbat_to_mV(u32 reg)
{
	return (int)(reg & 0x3FFF);
}

static inline int axp517_vts_to_mV(u32 reg)
{
	return (int)(reg & 0x3FFF) / 2;
}

static void bmu_axp517_none_battery_sets(void)
{
	pmic_bus_write(AXP517_RUNTIME_ADDR, AXP517_IPRECHG_CFG, 0x7f);
	pmic_bus_write(AXP517_RUNTIME_ADDR, AXP517_ICC_CFG, 0xff);
	pmic_bus_write(AXP517_RUNTIME_ADDR, AXP517_ITERM_CFG, 0x05);
	/* set vindpm to 3.6v */
	pmic_bus_write(AXP517_RUNTIME_ADDR, AXP517_VINDPM_CFG, 0);
	/* set cv to 3.8v */
	pmic_bus_write(AXP517_RUNTIME_ADDR, AXP517_VTERM_CFG, 0x5);
	/* disable bat det */
	pmic_bus_clrbits(AXP517_RUNTIME_ADDR, AXP517_BAT_DET, BIT(0));
}

static int bmu_axp517_set_charge(int status)
{
	if (!status) {
		pmic_bus_clrbits(AXP517_RUNTIME_ADDR, AXP517_MODULE_EN, BIT(1));
	} else {
		pmic_bus_setbits(AXP517_RUNTIME_ADDR, AXP517_MODULE_EN, BIT(1));
	}

	return 0;
}

static inline int axp517_bat_temp_mv(void)
{
	unsigned char reg_value[2];
	int bat_temp_mv;
	u32 bat_temp_adc;

	if (pmic_bus_read(AXP517_RUNTIME_ADDR, AXP517_TS_H, &reg_value[0])) {
			return -1;
	}

	if (pmic_bus_read(AXP517_RUNTIME_ADDR, AXP517_TS_L, &reg_value[1])) {
			return -1;
	}

	reg_value[0] &= GENMASK(5, 0);
	bat_temp_adc = (reg_value[0] << 8) | reg_value[1];
	bat_temp_mv = axp517_vts_to_mV(bat_temp_adc);

	return bat_temp_mv;
}

static void bmu_axp517_set_necessary_reg(void)
{
	u8 reg_value;
	/*
	 * when use this bit to shutdown system, it must write 0
	 * when system power on
	 */
	pmic_bus_clrbits(AXP517_RUNTIME_ADDR, AXP517_BATFET_CTRL, BIT(3));
	/* set input limit to 3A */
	pmic_bus_write(AXP517_RUNTIME_ADDR, AXP517_IIN_LIM, 0xE8);
	/* set cc clock enable */
	pmic_bus_setbits(AXP517_RUNTIME_ADDR, AXP517_CLK_EN, BIT(3));
	/* set vbus_ov to 11v */
	pmic_bus_read(AXP517_RUNTIME_ADDR, AXP517_VBUS_OV_SET, &reg_value);
	reg_value &= ~(0xC0);
	reg_value |= 0x80;
	pmic_bus_write(AXP517_RUNTIME_ADDR, AXP517_VBUS_OV_SET, reg_value);
	/* set bc/cc changes disable */
	pmic_bus_setbits(AXP517_RUNTIME_ADDR, AXP517_BST_CFG1, BIT(7));
}

static int bmu_axp517_probe(void)
{
	u8 bmu_chip_id;

	if (pmic_bus_init(AXP517_DEVICE_ADDR, AXP517_RUNTIME_ADDR)) {
		tick_printf("%s pmic_bus_init f11ail\n", __func__);
		return -1;
	}
	if (pmic_bus_read(AXP517_RUNTIME_ADDR, AXP517_CHIP_ID_EXT,
			  &bmu_chip_id)) {
		tick_printf("%s pmic_bus_read fail\n", __func__);
		return -1;
	}
	if (bmu_chip_id == 0x4) {
		/*bmu type AXP517*/
		tick_printf("BMU: AXP517\n");
		bmu_axp517_set_necessary_reg();
		return 0;
	}
	return -1;
}

static void bmu_axp517_clear_irq(void)
{
	u8 reg;

	for (reg = AXP517_IRQ_EN0; reg <= AXP517_IRQ_EN4; reg++) {
		pmic_bus_write(AXP517_RUNTIME_ADDR, reg, 0x00);
	}

	for (reg = AXP517_IRQ0; reg <= AXP517_IRQ4; reg++) {
		pmic_bus_write(AXP517_RUNTIME_ADDR, reg, 0xFF);
	}

	pmic_bus_write(AXP517_RUNTIME_ADDR, AXP517_IRQ_EN1, 0xc3);

}

int bmu_axp517_set_power_off(void)
{
	u8 reg_value;

	bmu_axp517_clear_irq();

	pmic_bus_read(AXP517_RUNTIME_ADDR, AXP517_BATFET_CTRL, &reg_value);
	reg_value &= ~(0x30);
	reg_value |= 0x10;
	pmic_bus_write(AXP517_RUNTIME_ADDR, AXP517_BATFET_CTRL, reg_value);

	pmic_bus_read(AXP517_RUNTIME_ADDR, AXP517_BATFET_CTRL, &reg_value);
	reg_value |= 0x08;
	pmic_bus_write(AXP517_RUNTIME_ADDR, AXP517_BATFET_CTRL, reg_value);

	return 0;
}

/*
	boot_source	0x49		help			return

	usb			BIT6/7		VBUS insert		AXP_BOOT_SOURCE_VBUS_USB
	battery		BIT5		battary in		AXP_BOOT_SOURCE_BATTERY
	power low	BIT2/3		boot button		AXP_BOOT_SOURCE_BUTTON
*/
int bmu_axp517_get_poweron_source(void)
{
	uchar reg_value;
	int reboot_mode;

	reboot_mode = pmu_get_sys_mode();
	tick_printf("[AXP517] charge/reboot status:0x%x\n", reboot_mode);
	/* if have reboot flag, then return -1*/
	if (reboot_mode == SUNXI_REBOOT_FLAG) {
		pmu_set_sys_mode(0);
		return -1;
	}

	if (pmic_bus_read(AXP517_RUNTIME_ADDR, AXP517_IRQ1, &reg_value)) {
		return -1;
	}

	/*bit7 : vbus, bit6: battery inster*/
	if (reg_value & (1 << 7) || reg_value & (1 << 6)) {
		reg_value &= 0xC0;
		pmic_bus_write(AXP517_RUNTIME_ADDR, AXP517_IRQ1, reg_value);
		if (axp_get_battery_exist() == BATTERY_IS_EXIST) {
			return AXP_BOOT_SOURCE_CHARGER;
		} else {
			return AXP_BOOT_SOURCE_VBUS_USB;
		}
	} else if (reg_value & (1 << 5)) {
		reg_value &= 0x20;
		pmic_bus_write(AXP517_RUNTIME_ADDR, AXP517_IRQ1, reg_value);
		return AXP_BOOT_SOURCE_BATTERY;
	} else if (reg_value & 0x0C) {
		reg_value &= 0x0C;
		pmic_bus_write(AXP517_RUNTIME_ADDR, AXP517_IRQ1, reg_value);
		return AXP_BOOT_SOURCE_BUTTON;
	}

	return -1;
}

int bmu_axp517_get_axp_bus_exist(void)
{
	u8 reg_value;
	if (pmic_bus_read(AXP517_RUNTIME_ADDR, AXP517_STATUS0, &reg_value)) {
		return -1;
	}
	/*bit5: 0: vbus not power,  1: power good*/
	if (reg_value & BIT(5)) {
		return AXP_VBUS_EXIST;
	}
	return 0;
}

int bmu_axp517_get_battery_vol(void)
{
	unsigned char reg_value[2];
	int bat_vol;
	u32 bat_vol_adc;

	if (pmic_bus_read(AXP517_RUNTIME_ADDR, AXP517_VBAT_H, &reg_value[0])) {
			return -1;
	}

	if (pmic_bus_read(AXP517_RUNTIME_ADDR, AXP517_VBAT_L, &reg_value[1])) {
			return -1;
	}

	reg_value[0] &= GENMASK(5, 0);
	bat_vol_adc = (reg_value[0] << 8) | reg_value[1];

	bat_vol = axp517_vbat_to_mV(bat_vol_adc);

	return bat_vol;
}

int bmu_axp517_get_battery_capacity(void)
{
	u8 reg_value;

	if (pmic_bus_read(AXP517_RUNTIME_ADDR, AXP517_GAUGE_SOC, &reg_value)) {
		return -1;
	}

	return (int)(reg_value & 0x7F);
}

int _bmu_axp517_get_battery_probe_with_bat_temp(void)
{
	int temp_mv;

	/* get ts vol*/
	bmu_set_ntc_onoff(1, 50);
	temp_mv = axp517_bat_temp_mv();
	tick_printf("[AXP517] battery temp_mv:%d\n", temp_mv);

	if (!temp_mv)
		return BATTERY_NONE;

	return BATTERY_IS_EXIST;
}

int _bmu_axp517_get_battery_probe_with_bat_vol(void)
{
	int bat_vol;

	bmu_axp517_set_charge(0);

	mdelay(2 * 1000);
	bat_vol = bmu_axp517_get_battery_vol();
	tick_printf("[AXP517] bat_vol when discharge:%d\n", bat_vol);
	bmu_axp517_set_charge(1);

	if (bat_vol < 3000)
		return BATTERY_NONE;

	return BATTERY_IS_EXIST;
}

int _bmu_axp517_get_battery_probe(void)
{
	int ts_bat_detect, bat_exist;
	int ret, vbus_exist;

	vbus_exist = bmu_axp517_get_axp_bus_exist();

	if (vbus_exist != AXP_VBUS_EXIST)
		return BATTERY_IS_EXIST;

	ret = script_parser_fetch(FDT_PATH_POWER_SPLY, "ts_bat_detect", &ts_bat_detect, 1);
	if (ret < 0)
		ts_bat_detect = 1;

	if (ts_bat_detect)
		bat_exist = _bmu_axp517_get_battery_probe_with_bat_temp();
	else
		bat_exist = _bmu_axp517_get_battery_probe_with_bat_vol();

	return bat_exist;
}

int bmu_axp517_get_battery_probe(void)
{
	int work_mode = get_boot_work_mode();
	static int bat_exist, check_count;
	int ret;

	if (work_mode != WORK_MODE_BOOT)
		return BATTERY_NONE;

	if (check_count)
		return bat_exist;

	ret = script_parser_fetch(FDT_PATH_POWER_SPLY, "battery_exist", &bat_exist, 1);
	if (ret < 0)
		bat_exist = 1;

	if (bat_exist) {
		bat_exist = _bmu_axp517_get_battery_probe();
	}

	if (bat_exist != BATTERY_IS_EXIST) {
		bmu_axp517_none_battery_sets();
	}

	tick_printf("[AXP517] battery check exist:%d\n", bat_exist);
	check_count = 1;

	return bat_exist;
}

int bmu_axp517_set_vbus_current_limit(int current)
{
	u8 reg_value;
	u8 temp;
	if (pmic_bus_read(AXP517_RUNTIME_ADDR, AXP517_IIN_LIM, &reg_value)) {
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
	reg_value = (temp << 2);
	tick_printf("[AXP517]Input current:%d mA\n", current);
	if (pmic_bus_write(AXP517_RUNTIME_ADDR, AXP517_IIN_LIM, reg_value)) {
		return -1;
	}

	return 0;
}

unsigned char bmu_axp517_get_reg_value(unsigned char reg_addr)
{
	u8 reg_value;
	if (pmic_bus_read(AXP517_RUNTIME_ADDR, reg_addr, &reg_value)) {
		return -1;
	}
	return reg_value;
}

unsigned char bmu_axp517_set_reg_value(unsigned char reg_addr, unsigned char reg_value)
{
	unsigned char reg;
	if (pmic_bus_write(AXP517_RUNTIME_ADDR, reg_addr, reg_value)) {
		return -1;
	}
	if (pmic_bus_read(AXP517_RUNTIME_ADDR, reg_addr, &reg)) {
		return -1;
	}
	return reg;
}

int bmu_axp517_set_ntc_cur(int ntc_cur)
{
	unsigned char reg_value;
	if (pmic_bus_read(AXP517_RUNTIME_ADDR, AXP517_TS_CFG, &reg_value)) {
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

	if (pmic_bus_write(AXP517_RUNTIME_ADDR, AXP517_TS_CFG, reg_value)) {
			return -1;
	}

	mdelay(10);
	return 0;
}

int bmu_axp517_set_ntc_onff(int onoff, int ntc_cur)
{
	if (!onoff) {
		pmic_bus_setbits(AXP517_RUNTIME_ADDR, AXP517_TS_CFG, BIT(4));
		pmic_bus_clrbits(AXP517_RUNTIME_ADDR, AXP517_ADC_CH_EN0, BIT(1));
	} else {
		pmic_bus_clrbits(AXP517_RUNTIME_ADDR, AXP517_TS_CFG, BIT(4));
		pmic_bus_setbits(AXP517_RUNTIME_ADDR, AXP517_ADC_CH_EN0, BIT(1));
		bmu_axp517_set_ntc_cur(ntc_cur);
	}
	return 0;
}

static int axp_vts_to_temp(int data, int param[16])
{
	int temp;

	if (data < param[15])
		return 800;
	else if (data <= param[14]) {
		temp = 700 + (param[14]-data) * 100 /
		(param[14]-param[15]);
	} else if (data <= param[13]) {
		temp = 600 + (param[13]-data) * 100 /
		(param[13]-param[14]);
	} else if (data <= param[12]) {
		temp = 550 + (param[12]-data) * 50 /
		(param[12]-param[13]);
	} else if (data <= param[11]) {
		temp = 500 + (param[11]-data) * 50 /
		(param[11]-param[12]);
	} else if (data <= param[10]) {
		temp = 450 + (param[10]-data) * 50 /
		(param[10]-param[11]);
	} else if (data <= param[9]) {
		temp = 400 + (param[9]-data) * 50 /
		(param[9]-param[10]);
	} else if (data <= param[8]) {
		temp = 300 + (param[8]-data) * 100 /
		(param[8]-param[9]);
	} else if (data <= param[7]) {
		temp = 200 + (param[7]-data) * 100 /
		(param[7]-param[8]);
	} else if (data <= param[6]) {
		temp = 100 + (param[6]-data) * 100 /
		(param[6]-param[7]);
	} else if (data <= param[5]) {
		temp = 50 + (param[5]-data) * 50 /
		(param[5]-param[6]);
	} else if (data <= param[4]) {
		temp = 0 + (param[4]-data) * 50 /
		(param[4]-param[5]);
	} else if (data <= param[3]) {
		temp = -50 + (param[3]-data) * 50 /
		(param[3] - param[4]);
	} else if (data <= param[2]) {
		temp = -100 + (param[2]-data) * 50 /
		(param[2] - param[3]);
	} else if (data <= param[1]) {
		temp = -150 + (param[1]-data) * 50 /
		(param[1] - param[2]);
	} else if (data <= param[0]) {
		temp = -250 + (param[0]-data) * 100 /
		(param[0] - param[1]);
	} else
		temp = -250;
	return temp;
}

int bmu_axp517_get_ntc_temp(int param[16])
{
	int bat_temp_mv, temp;

	bat_temp_mv = axp517_bat_temp_mv();

	temp = axp_vts_to_temp(bat_temp_mv, (int *)param);

	return temp;
}

int bmu_axp517_reg_debug(void)
{
	u8 reg_value[2];

	bmu_axp517_get_battery_probe();

	if (pmic_bus_read(AXP517_RUNTIME_ADDR, AXP517_IRQ1, &reg_value[0])) {
		return -1;
	}
	tick_printf("[AXP517] poweron irq: 0x%x:0x%x\n", AXP517_IRQ1, reg_value[0]);
	return 0;
}

int bmu_axp517_reset_capacity(void)
{
	pmic_bus_write(AXP517_RUNTIME_ADDR, AXP517_GAUGE_CONFIG, 0);

	return 1;
}

U_BOOT_AXP_BMU_INIT(bmu_axp517) = {
	.bmu_name	   = "bmu_axp517",
	.probe		    = bmu_axp517_probe,
	.set_power_off      = bmu_axp517_set_power_off,
	.get_poweron_source = bmu_axp517_get_poweron_source,
	.get_axp_bus_exist  = bmu_axp517_get_axp_bus_exist,
	.get_battery_vol	= bmu_axp517_get_battery_vol,
	.get_battery_capacity   = bmu_axp517_get_battery_capacity,
	.get_battery_probe      = bmu_axp517_get_battery_probe,
	.set_vbus_current_limit = bmu_axp517_set_vbus_current_limit,
	.get_reg_value = bmu_axp517_get_reg_value,
	.set_reg_value = bmu_axp517_set_reg_value,
	.set_ntc_onoff     = bmu_axp517_set_ntc_onff,
	.get_ntc_temp      = bmu_axp517_get_ntc_temp,
	.reg_debug         = bmu_axp517_reg_debug,
	.reset_capacity    = bmu_axp517_reset_capacity,
};
