/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <asm/global_data.h>
#include <linux/libfdt.h>
#include <fdt_support.h>
#include <sys_config.h>
#include <sunxi_power/axp.h>
#include <sunxi_power/pmu_ext.h>
#include <sunxi_power/pmu_general.h>
#include <sunxi_power/bmu_ext.h>
#include <sunxi_board.h>

#define PMU_FASTBOOT_MODE		(0x0c)
#define PMU_FACTORY_MODE		(0x0d)
#define PMU_SYS_MODE			(0x0e)
#define PMU_CHARGE_MODE			(0x0f)
#define PMU_RECOVERY_MODE		(0x10)
#define PMU_EFEX_MODE			(0x11)
#define PMU_UBOOT_MODE			(0x02)
#define PMU_BOOT_MODE			(0x02)
#define FDT_PATH_CHARGER0                "/soc/charger0"
#define FDT_PATH_REGU                   "/soc/regulator0"
#define FDT_PATH_POWER_DELAY            "/soc/power_delay"


int axp_set_power_supply_output(void);
int axp_set_charge_vol_limit(char *dev);
int axp_set_current_limit(char *dev);
int axp_get_battery_status(void);
int axp_battery_status_handle(void);
int axp_set_vol(char *name, uint onoff);
int sunxi_update_axp_info(void);
int axp_reset_capacity(void);
int axp_set_dcdc_mode(void);
int axp_get_battery_exist(void);

#ifdef CONFIG_SUNXI_PMU_EXT
int pmu_ext_probe(void);
int pmu_ext_set_dcdc_mode(void);
int pmu_ext_set_power_supply_output(void);
bool pmu_ext_get_exist(void);
int pmu_ext_get_type(void);
#endif

#ifdef CONFIG_SUNXI_BMU_EXT
extern int bmu_ext_probe(void);
extern int bmu_ext_get_vbus_status(void);
extern bool bmu_ext_get_exist(void);
extern int bmu_ext_get_poweron_source(int poweron_source);
extern int bmu_ext_get_battery_probe(void);
extern int bmu_ext_get_type(void);
extern int bmu_ext_set_discharge(void);
extern int bmu_ext_set_charge(void);
extern int bmu_ext_get_charge(void);
#endif

#ifdef CONFIG_SUNXI_PMU_GENERAL
extern int pmu_general_probe(void);
extern bool pmu_general_get_exist(void);
extern int pmu_general_get_voltage_by_phandle(const void *fdt, uint32_t phandle);
extern int pmu_general_get_voltage_by_full_name(char *name);
extern int pmu_general_set_voltage_by_phandle(const void *fdt, uint32_t phandle, uint vol_value, uint onoff);
extern int pmu_general_set_voltage_by_full_name(char *name, uint vol_value, uint onoff);
#endif
