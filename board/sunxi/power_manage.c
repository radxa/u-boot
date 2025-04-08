/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <asm/io.h>
#include <sunxi_power/power_manage.h>
#include <sys_config.h>
#include <sunxi_board.h>
#include <spare_head.h>
#include <sunxi_display2.h>
#include <console.h>
#include <asm/arch-sunxi/gpio.h>

#if defined(CONFIG_AW_DRM)
#include <drm/drm_logo.h>
#endif
/*
 * Global data (for the gd->bd)
 */
DECLARE_GLOBAL_DATA_PTR;

int set_gpio_bias(void)
{
#if !defined(CONFIG_SUNXI_GPIO_POWER_VOL_MODE) && !defined(CONFIG_SUNXI_GPIO_NOT_CTRL_BY_POWER)
	char bias_name[32];
	int bias_vol;
	int val;
	int nodeoffset = -1, offset;
	const struct fdt_property *prop;
	const char *pname;
	const int *pdata;
	nodeoffset = fdt_path_offset(working_fdt, FDT_PATH_GPIO_BIAS);
	for (offset = fdt_first_property_offset(working_fdt, nodeoffset);
	     offset > 0; offset = fdt_next_property_offset(working_fdt, offset)) {
		prop  = fdt_get_property_by_offset(working_fdt, offset, NULL);
		pname = fdt_string(working_fdt, fdt32_to_cpu(prop->nameoff));
		pdata = (const int *)prop->data;
		bias_vol = fdt32_to_cpu(pdata[0]);
		memset(bias_name, 0, sizeof(bias_name));
		strcpy(bias_name, pname);
		if (strstr((const char *)bias_name, "bias") == NULL) {
			continue;
		}
		pr_msg("bias_name:%s\t bias_vol:%d\n", bias_name, bias_vol);
		if (bias_name[1] == 'l') {
			val = readl(SUNXI_R_PIO_BASE + 0x340);
			if (bias_vol <= 1800) {
				val |= (1 << 0);
			} else {
				val &= ~(1 << 0);
			}
			writel(val, SUNXI_R_PIO_BASE + 0x340);
		} else if ((bias_name[1] <= 'j') && (bias_name[1] >= 'a')) {
			val = readl(SUNXI_PIO_BASE + 0x340);
			if (bias_vol <= 1800) {
				val |= (1 << (bias_name[1] - 'a'));
			} else {
				val &= ~(1 << (bias_name[1] - 'a'));
			}
			writel(val, SUNXI_PIO_BASE + 0x340);
		}
	}
#endif
	return 0;
}

int axp_set_power_supply_output(void)
{
	int onoff, ret = 0;
	int power_delay = 0;
	char delay_name[32];
	char power_name[32];
	int power_vol;
	int power_vol_d;
	int nodeoffset = -1, nodeoffset2 = -1, offset;
	int twi_nodeoffset = -1;
	int pmu_nodeoffset = -1;
	const struct fdt_property *prop;
	const char *pname;
	const int *pdata;
	const char *twi_regu_name;
	char pmu_node_str[8] = {0};
	char twi_path[128] = {0};
	int work_mode = get_boot_work_mode();

#ifdef CONFIG_SUNXI_TRY_POWER_SPLY
	char axp_name[16] = {0}, chipid;
	char axp_sply_path[64] = {0};
	pmu_get_info(axp_name, (unsigned char *)&chipid);
	sprintf(axp_sply_path, "/soc/%s_power_sply", axp_name);
	nodeoffset = fdt_path_offset(working_fdt, axp_sply_path);
#endif
	if (nodeoffset < 0) {
		nodeoffset = fdt_path_offset(working_fdt, FDT_PATH_POWER_SPLY);
	}

	nodeoffset2 =  fdt_path_offset(working_fdt, FDT_PATH_GPIO_BIAS);
	if (nodeoffset2 < 0) {
		pr_msg("%s get gpio bias information fail!\n", __func__);
	}

#if !defined(CONFIG_SUNXI_GPIO_POWER_VOL_MODE) && !defined(CONFIG_SUNXI_GPIO_NOT_CTRL_BY_POWER)
	char sply_node[32], bias_node[32];
	int reg_base, pin_base;
	int i = 0, ret1, ret2, val;

	struct pin_bias_t {
		const char *pin_name;
		char *supply_name;
		int gpio_bias;
	} pin_bias[] = {
		{"pc"},
		{"pl"},
	};
#endif

	/* Get pmu node and its parent twi node*/
	sprintf(pmu_node_str, "pmu0");
	pmu_nodeoffset = fdt_path_offset(working_fdt, pmu_node_str);
	twi_nodeoffset = fdt_parent_offset(working_fdt, pmu_nodeoffset);
	fdt_get_path(working_fdt, twi_nodeoffset, twi_path, sizeof(twi_path));

	/* Get the regulator for pmu's twi, for changing pinctrl when change voltage*/
	twi_regu_name = fdt_get_regulator_name(twi_nodeoffset, "twi");

#if !defined(CONFIG_SUNXI_GPIO_POWER_VOL_MODE) && !defined(CONFIG_SUNXI_GPIO_NOT_CTRL_BY_POWER)
	/* For change GPIO[x] bias when gpio voltage is changed */
	for (i = 0; i < sizeof(pin_bias)/sizeof(pin_bias[0]); i++) {
		sprintf(sply_node, "%s_supply", pin_bias[i].pin_name);
		sprintf(bias_node, "%s_bias", pin_bias[i].pin_name);
		ret1 = fdt_getprop_string(working_fdt, nodeoffset2, sply_node, (char **)(&(pin_bias[i].supply_name)));
		ret2 = script_parser_fetch(FDT_PATH_GPIO_BIAS, bias_node, &(pin_bias[i].gpio_bias), 0);
		pr_msg("gpio_bias, %s: %4d, %s: %-9s\n",
				bias_node, ret2 < 0 ? -1  : pin_bias[i].gpio_bias,
				sply_node, ret1 < 0 ? "not set" : pin_bias[i].supply_name);
	};
#endif

	for (offset = fdt_first_property_offset(working_fdt, nodeoffset);
	     offset > 0; offset = fdt_next_property_offset(working_fdt, offset)) {
		prop  = fdt_get_property_by_offset(working_fdt, offset, NULL);
		pname = fdt_string(working_fdt, fdt32_to_cpu(prop->nameoff));
		pdata = (const int *)prop->data;
		power_vol = fdt32_to_cpu(pdata[0]);
		memset(power_name, 0, sizeof(power_name));
		strcpy(power_name, pname);
		if (strstr((const char *)power_name, "vol") == NULL) {
			continue;
		}
		if (strstr((const char *)power_name, "ext") != NULL) {
			continue;
		}

		onoff       = -1;
		power_vol_d = 0;

		if (power_vol > 1100000) {
			if (work_mode != WORK_MODE_BOOT) {
				onoff       = 0;
			} else {
				onoff       = 1;
			}
			power_vol_d = power_vol % 10000;
		} else if (power_vol > 10000) {
			onoff       = 1;
			power_vol_d = power_vol % 10000;

		} else if (power_vol >= 0) {
			onoff       = 0;
			power_vol_d = power_vol;
		}

		debug("%s = %d, onoff=%d\n", power_name, power_vol_d, onoff);

		if (pmu_set_voltage(power_name, power_vol_d, onoff)) {
			debug("axp set %s to %d failed\n", power_name,
			       power_vol_d);
		}

		/*set delay for each output*/
		sprintf(delay_name, "%s_delay", power_name);
		ret = script_parser_fetch(FDT_PATH_POWER_DELAY, delay_name, &power_delay, 0);
		if (ret < 0)
			power_delay = 0;
		if (power_delay != 0) {
			pr_msg("%s need to wait stable!\n", power_name);

			/* change twi pinctrl to shorten delay time */
			if (!strncmp(twi_regu_name, power_name, strlen(twi_regu_name))) {
				fdt_set_all_pin(twi_path, "pinctrl-1");
				pr_msg("%s has set pinctrl-1\n", twi_path);
			}

			mdelay(power_delay / 1000);

			if (!strncmp(twi_regu_name, power_name, strlen(twi_regu_name))) {
				fdt_set_all_pin(twi_path, "pinctrl-0");
				pr_msg("%s has set pinctrl-0\n", twi_path);
			}
		}

#if !defined(CONFIG_SUNXI_GPIO_POWER_VOL_MODE) && !defined(CONFIG_SUNXI_GPIO_NOT_CTRL_BY_POWER)
		for (i = 0; i < sizeof(pin_bias)/sizeof(pin_bias[0]); i++) {
			if (!strncmp(pin_bias[i].supply_name, power_name, sizeof(power_name))) {
				if (pin_bias[i].gpio_bias == 0)
					pin_bias[i].gpio_bias = power_vol_d;

				if (pin_bias[i].pin_name[1] >= 'l') {
					reg_base = SUNXI_R_PIO_BASE;
					pin_base = 'l';
				} else {
					reg_base = SUNXI_PIO_BASE;
					pin_base = 'a';
				}

				val = readl(reg_base + 0x340);
				if (pin_bias[i].gpio_bias <= 1800)
					val |=  (1 << (pin_bias[i].pin_name[1] - pin_base));
				else
					val &= ~(1 << (pin_bias[i].pin_name[1] - pin_base));

				writel(val, reg_base + 0x340);

				pr_msg("GPIO%c change bias done!\n", pin_bias[i].pin_name[1] - 'a' + 'A');
			}
		}
#endif

		pr_msg("%s = %d, onoff=%d\n", power_name, pmu_get_voltage(power_name), onoff);
	}

#ifndef CONFIG_GPIO_BIAS_SKIP
	set_gpio_bias();
#endif

	return 0;
}

/* set dcdc pwm mode */
int axp_set_dcdc_mode(void)
{
	const struct fdt_property *prop;
	int nodeoffset = -1, offset, mode;
	const char *pname;
	const int *pdata;

	if (nodeoffset < 0) {
		nodeoffset = fdt_path_offset(working_fdt, FDT_PATH_POWER_SPLY);
	}

	for (offset = fdt_first_property_offset(working_fdt, nodeoffset);
	     offset > 0; offset = fdt_next_property_offset(working_fdt, offset)) {
		prop  = fdt_get_property_by_offset(working_fdt, offset, NULL);
		pname = fdt_string(working_fdt, fdt32_to_cpu(prop->nameoff));
		pdata = (const int *)prop->data;
		mode = fdt32_to_cpu(pdata[0]);
		if (strstr(pname, "mode") == NULL) {
			continue;
		}

		if (strstr(pname, "ext") != NULL) {
			continue;
		}

		if (pmu_set_dcdc_mode(pname, mode) < 0) {
			debug("set %s fail!\n", pname);
		}
	}

	return 0;
}

#ifdef CONFIG_SUNXI_PMU_EXT
int pmu_ext_set_dcdc_mode(void)
{
	const struct fdt_property *prop;
	int nodeoffset = -1, offset, mode;
	const char *pname;
	const int *pdata;

	if (nodeoffset < 0) {
		nodeoffset = fdt_path_offset(working_fdt, FDT_PATH_POWER_SPLY);
	}

	for (offset = fdt_first_property_offset(working_fdt, nodeoffset);
	     offset > 0; offset = fdt_next_property_offset(working_fdt, offset)) {
		prop  = fdt_get_property_by_offset(working_fdt, offset, NULL);
		pname = fdt_string(working_fdt, fdt32_to_cpu(prop->nameoff));
		pdata = (const int *)prop->data;
		mode = fdt32_to_cpu(pdata[0]);
		if (strstr(pname, "mode") == NULL) {
			continue;
		}

		if (strstr(pname, "ext") == NULL) {
			continue;
		}

		if (pmu_set_dcdc_mode_ext(pname, mode) < 0) {
			debug("set %s fail!\n", pname);
		}
	}

	return 0;
}

int pmu_ext_set_power_supply_output(void)
{
	char power_name[32];
	int power_vol, power_vol_d, onoff;
	int nodeoffset = -1, offset;
	const struct fdt_property *prop;
	const char *pname;
	const int *pdata;
	int work_mode = get_boot_work_mode();

	if (nodeoffset < 0) {
		nodeoffset = fdt_path_offset(working_fdt, FDT_PATH_POWER_SPLY);
	}

	for (offset = fdt_first_property_offset(working_fdt, nodeoffset);
	     offset > 0; offset = fdt_next_property_offset(working_fdt, offset)) {
		prop  = fdt_get_property_by_offset(working_fdt, offset, NULL);
		pname = fdt_string(working_fdt, fdt32_to_cpu(prop->nameoff));
		pdata = (const int *)prop->data;
		power_vol = fdt32_to_cpu(pdata[0]);
		memset(power_name, 0, sizeof(power_name));
		strcpy(power_name, pname);
		if (strstr((const char *)power_name, "vol") == NULL) {
			continue;
		}

		if (strstr((const char *)power_name, "ext") == NULL) {
			continue;
		}

		onoff       = -1;
		power_vol_d = 0;

		if (power_vol > 1100000) {
			if (work_mode != WORK_MODE_BOOT) {
				onoff       = 0;
			} else {
				onoff       = 1;
			}
			power_vol_d = power_vol % 10000;
		} else if (power_vol > 10000) {
			onoff       = 1;
			power_vol_d = power_vol % 10000;

		} else if (power_vol >= 0) {
			onoff       = 0;
			power_vol_d = power_vol;
		}

		debug("%s = %d, onoff=%d\n", power_name, power_vol_d, onoff);

		if (pmu_ext_set_voltage(power_name, power_vol_d, onoff)) {
			debug("axp set %s to %d failed\n", power_name,
			       power_vol_d);
		}

		pr_msg("%s = %d, onoff=%d\n", power_name, pmu_ext_get_voltage(power_name), onoff);
	}

	return 0;
}

#endif

void axp_show_bmp(char *name, int onoff)
{
#if defined(CONFIG_EINK200_SUNXI) || defined(CONFIG_CMD_SUNXI_BMP)
	sunxi_bmp_display(name);
#endif
#if defined(CONFIG_AW_DRM)
	if (!onoff)
		sunxi_show_bmp_and_backlight(name, "off");
	else
		sunxi_show_bmp(name);
#endif
}

void axp_black_light_control(int onoff)
{
	__maybe_unused unsigned long arg[3] = {0};
#ifdef CONFIG_DISP2_SUNXI
	disp_ioctl(NULL, onoff, (void *)arg);
#endif
	if (onoff == DISP_LCD_BACKLIGHT_ENABLE) {
		pr_notice("onoff:DISP_LCD_BACKLIGHT_ENABLE\n");
#if defined(CONFIG_AW_DRM)
		sunxi_backlight_ctrl("on");
#endif
	} else if (onoff == DISP_LCD_BACKLIGHT_DISABLE) {
		pr_notice("onoff:DISP_LCD_BACKLIGHT_DISABLE\n");
#if defined(CONFIG_AW_DRM)
		sunxi_backlight_ctrl("off");
#endif
	}
}

/* reset bat capacity when system is writing firmware*/
int axp_reset_capacity(void)
{
	return bmu_reset_capacity();
}

int axp_set_charge_vol_limit(char *dev)
{
	int limit_vol = 0;
	if (strstr(dev, "vol") == NULL) {
		debug("Illegal string");
		return -1;
	}
	if (script_parser_fetch(FDT_PATH_CHARGER0, dev, &limit_vol, 1)) {
		return -1;
	}
	pmu_set_bus_vol_limit(limit_vol);
	return 0;
}

int axp_set_current_limit(char *dev)
{
	int limit_cur = 0;
	if (strstr(dev, "cur") == NULL) {
		debug("Illegal string");
		return -1;
	}
	if (script_parser_fetch(FDT_PATH_CHARGER0, dev, &limit_cur, 1)) {
		return -1;
	}
	if (!strncmp(dev, "pmu_runtime_chgcur", sizeof("pmu_runtime_chgcur")) ||
	    !strncmp(dev, "pmu_suspend_chgcur", sizeof("pmu_suspend_chgcur"))) {
		bmu_set_charge_current_limit(limit_cur);
	} else {
		bmu_set_vbus_current_limit(limit_cur);
	}
	return 0;
}

/* get battery exist:                  */
/*                    BATTERY_NONE     */
/*                    BATTERY_IS_EXIST */
int axp_get_battery_exist(void)
{
	static int bat_exist, check_count;

	if (check_count)
		return bat_exist;

	check_count = 1;
	script_parser_fetch(FDT_PATH_POWER_SPLY, "battery_exist", &bat_exist, 1);

	if (!bat_exist) {
		bat_exist = BATTERY_NONE;
		return bat_exist;
	}

#ifdef CONFIG_SUNXI_BMU
	bat_exist = bmu_get_battery_probe();
#else
	bat_exist = BATTERY_NONE;
#endif

	return bat_exist;
}

/* get battery status :                             */
/*                     BATTERY_IS_NOT_EXIST         */
/*                     BATTERY_RATIO_VOL_NOT_ENOUGH */
/*                     BATTERY_RATIO_VOL_IS_ENOUGH  */
int axp_get_battery_status(void)
{
	int dcin_exist, bat_exist;
	int bat_vol, ratio;
	int safe_vol = 0, safe_ratio = 1;
	int ntc_status = -1, ntc_cur = 0;

	bat_exist	= axp_get_battery_exist();

	if (bat_exist != BATTERY_IS_EXIST) {
		pr_msg("bat not exist.\n");
		bmu_set_ntc_onoff(0, 0);
		return BATTERY_IS_NOT_EXIST;
	}

	script_parser_fetch(FDT_PATH_CHARGER0, "pmu_safe_vol", &safe_vol, -1);
	script_parser_fetch(FDT_PATH_CHARGER0, "pmu_safe_ratio", &safe_ratio, 1);
	script_parser_fetch(FDT_PATH_CHARGER0, "ntc_cur", &ntc_cur, 0);
	script_parser_fetch(FDT_PATH_POWER_SPLY, "ntc_status", &ntc_status, 0);

	dcin_exist	= bmu_get_axp_bus_exist();
	bat_vol		= bmu_get_battery_vol();
	ratio		= bmu_get_battery_capacity();

	pr_msg("battery_exist=%d, dcin_exist=%d\n", bat_exist, dcin_exist);
	pr_msg("bat_vol=%d, ratio=%d\n", bat_vol, ratio);
	pr_msg("safe_vol=%d, safe_ratio=%d\n", safe_vol, safe_ratio);
	pr_msg("ntc_status=%d, ntc_cur=%dmA\n", ntc_status, ntc_cur);

	bmu_set_ntc_onoff(ntc_status, ntc_cur);

	if ((ratio < safe_ratio) || (bat_vol < safe_vol))
		return BATTERY_RATIO_VOL_NOT_ENOUGH;

	return BATTERY_RATIO_VOL_IS_ENOUGH;
}

int sunxi_bat_low_vol_handle(void)
{
	int i = 0, safe_vol = 0, safe_ratio = 1, bmp_type = 0;
	int onoff;
	__maybe_unused unsigned long arg[3] = {0};
	int bat_vol      = bmu_get_battery_vol();
	int dcin_exist   = bmu_get_axp_bus_exist();
	int bat_ratio    = bmu_get_battery_capacity();
	script_parser_fetch(FDT_PATH_CHARGER0, "pmu_safe_vol", &safe_vol, -1);
	script_parser_fetch(FDT_PATH_CHARGER0, "pmu_safe_ratio", &safe_ratio, 1);
	script_parser_fetch(FDT_PATH_CHARGER0, "bat_bmp_type", &bmp_type, 0);

	if (bmp_type) {
		onoff = DISP_LCD_BACKLIGHT_ENABLE;
	} else {
		onoff = DISP_LCD_BACKLIGHT_DISABLE;
	}

	axp_black_light_control(onoff);
	axp_show_bmp("bat\\bat0.bmp", bmp_type);

	pr_force("bat_ratio:%d\tsafe_ratio:%d\tbat_vol:%dmV\tsafe_vol:%dmV\n",
			bat_ratio, safe_ratio, bat_vol, safe_vol);
	while (bat_vol < safe_vol || bat_ratio < safe_ratio) {
		bat_vol = bmu_get_battery_vol();
		bat_ratio = bmu_get_battery_capacity();
		dcin_exist = bmu_get_axp_bus_exist();
		if (onoff == DISP_LCD_BACKLIGHT_ENABLE) {
			if (i++ >= 500) {
				i = 0;
				onoff = DISP_LCD_BACKLIGHT_DISABLE;
				pr_force("bat_ratio:%d\tsafe_ratio:%d\tbat_vol:%dmV\tsafe_vol:%dmV\n",
						bat_ratio, safe_ratio, bat_vol, safe_vol);
				axp_black_light_control(onoff);
			}
		} else {
			if (pmu_get_key_irq() > 0) {
				i = 0;
				onoff = DISP_LCD_BACKLIGHT_ENABLE;
				pr_force("bat_ratio:%d\tsafe_ratio:%d\tbat_vol:%dmV\tsafe_vol:%dmV\n",
						bat_ratio, safe_ratio, bat_vol, safe_vol);
				axp_black_light_control(onoff);
			}
		}
		if (!dcin_exist) {
			onoff = DISP_LCD_BACKLIGHT_ENABLE;
			axp_black_light_control(onoff);
			pr_force("bat_ratio:%d\tsafe_ratio:%d\tbat_vol:%dmV\tsafe_vol:%dmV\n",
					bat_ratio, safe_ratio, bat_vol, safe_vol);
			tick_printf("battery ratio is low without dcin,to be shutdown\n");
			mdelay(3000);
			sunxi_board_shutdown_charge();
		}
		if (ctrlc())
			break;
		mdelay(10);
	}
	axp_show_bmp("bat\\bat_blank.bmp", 1);
	return 0;
}

int sunxi_bat_key_handle(void)
{
	int dcin_exist   = bmu_get_axp_bus_exist();
	__maybe_unused unsigned long arg[3] = {0};

	tick_printf("press power_on to start up\n");

	while (pmu_get_key_irq() <= 0) {
		dcin_exist = bmu_get_axp_bus_exist();
		if (!dcin_exist) {
			tick_printf("dcin is out,to be shutdown\n");
			mdelay(3000);
			sunxi_board_shutdown_charge();
		}
		if (ctrlc())
			break;
		mdelay(10);
	}
	return 0;
}

int sunxi_bat_temp_handle(void)
{
	__maybe_unused int ret, temp = 300, i, j;
	int safe_temp[2], para[16];
	char para_name[32];
	int onoff = DISP_LCD_BACKLIGHT_ENABLE;
	int dcin_exist   = bmu_get_axp_bus_exist();

	for (i = 0; i < 16; i++) {
		j = i + 1;
		sprintf(para_name, "pmu_bat_temp_para%d", j);
		ret = script_parser_fetch(FDT_PATH_CHARGER0, para_name, &para[i], -1);
	}
	temp = bmu_get_ntc_temp((int *)para);
	ret = script_parser_fetch(FDT_PATH_CHARGER0, "safe_temp_H", &safe_temp[0], 600);
	ret = script_parser_fetch(FDT_PATH_CHARGER0, "safe_temp_L", &safe_temp[1], 0);
	pr_force("safe_temp_H:%d\tsafe_temp_L:%d\ttemp:%d\n", safe_temp[0], safe_temp[1], temp);
	while (temp < safe_temp[1] || safe_temp[0] < temp) {
		dcin_exist = bmu_get_axp_bus_exist();
		temp = bmu_get_ntc_temp((int *)para);
		if (onoff == DISP_LCD_BACKLIGHT_ENABLE) {
			if (i++ >= 200) {
				i = 0;
				onoff = DISP_LCD_BACKLIGHT_DISABLE;
				pr_force("safe_temp_H:%d\tsafe_temp_L:%d\ttemp:%d\n", safe_temp[0], safe_temp[1], temp);
				axp_black_light_control(onoff);
			}
		} else {
			if (pmu_get_key_irq() > 0) {
				i = 0;
				onoff = DISP_LCD_BACKLIGHT_ENABLE;
				pr_force("safe_temp_H:%d\tsafe_temp_L:%d\temp:%d\n", safe_temp[0], safe_temp[1], temp);
				axp_black_light_control(onoff);
			}
		}
		if (!dcin_exist) {
			onoff = DISP_LCD_BACKLIGHT_ENABLE;
			pr_force("safe_temp_H:%d\tsafe_temp_L:%d\temp:%d\n", safe_temp[0], safe_temp[1], temp);
			tick_printf("battery without dcin,to be shutdown\n");
			axp_black_light_control(onoff);
			mdelay(3000);
			sunxi_board_shutdown_charge();
		}

		if (ctrlc())
			break;
		mdelay(10);
	}
	axp_show_bmp("bat\\bat_blank.bmp", 1);
	return 0;
}

void axp_battery_temp_check(int battery_status)
{
	int ret = 0, ntc_status = -1;
	int temp = 300, safe_temp[2], i, j;
	int para[16];
	char para_name[32];

	if (battery_status == BATTERY_IS_NOT_EXIST)
		return;

	ret = script_parser_fetch(FDT_PATH_POWER_SPLY, "ntc_status", &ntc_status, 0);
	if (ret < 0)
		ntc_status = 0;

	if (!ntc_status)
		return;

	for (i = 0; i < 16; i++) {
		j = i + 1;
		sprintf(para_name, "pmu_bat_temp_para%d", j);
		ret = script_parser_fetch(FDT_PATH_CHARGER0, para_name, &para[i], -1);
	}
	temp = bmu_get_ntc_temp((int *)para);
	ret = script_parser_fetch(FDT_PATH_CHARGER0, "safe_temp_H", &safe_temp[0], 600);
	ret = script_parser_fetch(FDT_PATH_CHARGER0, "safe_temp_L", &safe_temp[1], 0);
	tick_printf("battery temp is %d\n", temp);
	if (temp >= safe_temp[0]) {
		axp_show_bmp("bat\\bat_htmp.bmp", 1);
		tick_printf("battery temp (%d) is too high", temp);
		if (ntc_status == 1) {
			tick_printf(" , to be shutdown\n");
			mdelay(3000);
			sunxi_board_shutdown();
		} else {
			tick_printf(" , waitting for lower temp\n");
			sunxi_bat_temp_handle();
		}
	} else if (temp <= safe_temp[1]) {
		axp_show_bmp("bat\\bat_ltmp.bmp", 1);
		tick_printf("battery temp (%d) is too low", temp);
		if (ntc_status == 1) {
			tick_printf(" , to be shutdown\n");
			mdelay(3000);
			sunxi_board_shutdown();
		} else {
			tick_printf(" , waitting for higher temp\n");
			sunxi_bat_temp_handle();
		}
	}
}

int axp_battery_status_handle(void)
{
	int battery_status, charge_mode_sets, dcin_exist;
	int charge_mode_update;

	script_parser_fetch(FDT_PATH_POWER_SPLY, "charge_mode", &charge_mode_sets, 1);

#ifdef CONFIG_AXP_LATE_INFO
	battery_status = axp_get_battery_status();
	gd->pmu_runtime_chgcur = battery_status;
#else
	battery_status = gd->pmu_runtime_chgcur;
#endif

	dcin_exist = bmu_get_axp_bus_exist();
	axp_battery_temp_check(battery_status);

	switch (battery_status) {
	case BATTERY_IS_NOT_EXIST:
		if (charge_mode_sets == STARTUP_POWERKEY)
			sunxi_bat_key_handle();
		break;
	case BATTERY_RATIO_VOL_NOT_ENOUGH:
		if (dcin_exist) {
			sunxi_bat_low_vol_handle();
		} else {
			axp_show_bmp("bat\\bat0.bmp", 1);
			tick_printf("battery ratio or vol is low ,to be shutdown\n");
			mdelay(3000);
			sunxi_board_shutdown_charge();
		}
	case BATTERY_RATIO_VOL_IS_ENOUGH:
		switch (charge_mode_sets) {
		case STARTUP_IMMEDIATE_BOOT:
			pr_msg("start up directly\n");
			break;
		case STARTUP_POWERKEY:
			sunxi_bat_key_handle();
			break;
		case STARTUP_CHARGER:
			if (gd->chargemode == 1) {
				pr_msg("start up to charge mode\n");
				axp_show_bmp("bat\\battery_charge.bmp", 1);
				charge_mode_update = 1;
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	if (!charge_mode_update)
		gd->chargemode = 0;

	return 0;
}

int axp_set_vol(char *name, uint onoff)
{
	return pmu_set_voltage(name, 0, onoff);
}

/* get bootreason */
int sunxi_update_axp_info(void)
{
	int val = -1;
	char bootreason[16] = {0};
	int bat_exist;

	bat_exist = axp_get_battery_exist();

#ifdef CONFIG_SUNXI_BMU
#ifdef CONFIG_AXP_LATE_INFO
	val = bmu_get_poweron_source();
#else
	val = gd->pmu_saved_status;
#endif
#endif
	if ((val == -1 || val == AXP_BOOT_SOURCE_IRQ_LOW) && (pmu_get_sys_mode() == SUNXI_CHARGING_FLAG)) {
		val = AXP_BOOT_SOURCE_CHARGER;
		pmu_set_sys_mode(0);
	}
	switch (val) {
	case AXP_BOOT_SOURCE_BUTTON:
		strncpy(bootreason, "button", sizeof("button"));
		break;
	case AXP_BOOT_SOURCE_IRQ_LOW:
		strncpy(bootreason, "irq", sizeof("irq"));
		break;
	case AXP_BOOT_SOURCE_VBUS_USB:
		strncpy(bootreason, "usb", sizeof("usb"));
		break;
	case AXP_BOOT_SOURCE_CHARGER:
		strncpy(bootreason, "charger", sizeof("charger"));
		if (bat_exist == BATTERY_IS_EXIST)
			gd->chargemode = 1;
		break;
	case AXP_BOOT_SOURCE_BATTERY:
		strncpy(bootreason, "battery", sizeof("battery"));
		break;
	default:
		strncpy(bootreason, "unknow", sizeof("unknow"));
		break;
	}

	gd->pmu_saved_status = val;

	env_set("bootreason", bootreason);
	return 0;
}

int do_sunxi_axp(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	u8 reg_addr;
	u8 reg_value;
	if (argc < 4)
		return -1;
	reg_addr = (u8)simple_strtoul(argv[3], NULL, 16);
	if (!strncmp(argv[1], "pmu", 3)) {
#ifdef CONFIG_SUNXI_PMU
		if (!strncmp(argv[2], "read", 4)) {
			printf("pmu_value:0x%x\n", pmu_get_reg_value(reg_addr));
		} else if (!strncmp(argv[2], "write", 5)) {
			reg_value = (u8)simple_strtoul(argv[4], NULL, 16);
			printf("pmu_value:0x%x\n", pmu_set_reg_value(reg_addr, reg_value));
		} else {
			printf("input error\n");
			return -1;
		}
#endif
	} else if (!strncmp(argv[1], "bmu", 3)) {
#ifdef CONFIG_SUNXI_BMU
		if (!strncmp(argv[2], "read", 4)) {
			printf("bmu_value:0x%x\n", bmu_get_reg_value(reg_addr));
		} else if (!strncmp(argv[2], "write", 5)) {
			reg_value = (u8)simple_strtoul(argv[4], NULL, 16);
			printf("bmu_value:0x%x\n", bmu_set_reg_value(reg_addr, reg_value));
		} else {
			printf("input error\n");
			return -1;
		}
#endif
	} else {
		printf("input error\n");
		return -1;
	}
	return 0;
}

U_BOOT_CMD(sunxi_axp, 6, 1, do_sunxi_axp, "sunxi_axp sub-system",
	"sunxi_axp <pmu/bmu> <read> <reg_addr>\n"
	"sunxi_axp <pmu/bmu> <write> <reg_addr> <reg_value>\n");


