/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI PMU_GENERAL  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __PMU_GENERAL_H__
#define __PMU_GENERAL_H__

#include <common.h>
#include <linker_lists.h>
#include <linux/libfdt.h>
#include <fdt_support.h>
#include <sys_config.h>
#include <asm/arch/pmic_bus.h>
#include <sunxi_power/power_manage.h>
#include <sunxi_power/axp.h>
#include <sunxi_power/pmu_ext.h>
#include "sunxi_i2c.h"
#include <asm/arch/pmic_bus.h>
#include <i2c.h>

#ifdef PMU_DEBUG
#define pmu_info(fmt...) tick_printf("[pmu_general][info]: " fmt)
#define pmu_err(fmt...) tick_printf("[pmu_general][err]: " fmt)
#else
#define pmu_info(fmt...)
#define pmu_err(fmt...) tick_printf("[pmu_general][err]: " fmt)
#endif

struct sunxi_pmu_general_dev_t {
const char *pmu_general_name;
int (*probe)(void); /* matches chipid*/
bool (*get_exist)(void); /*get axp info*/
int (*set_dcdc_mode)(const char *name, int mode); /*force dcdc mode in pwm or not */
int (*set_voltage)(char *name, uint vol_value, uint onoff); /*Set a certain power, voltage value. */
int (*get_voltage)(char *name); /*Read a certain power, voltage value */
};

enum {
	PMU_NORMAL = 0,
	PMU_EXT,
	PMU_GENERAL_OCP2131,
	PMU_GENERAL_AW37501,
	NR_PMU_GENERAL_TYPE_MAX,
};

typedef struct _pmu_general_contrl_info {
	char name[16];

	u32 min_vol;
	u32 max_vol;
	u32 cfg_reg_addr;
	u32 cfg_reg_mask;

	u32 step0_val;
	u32 split1_val;
	u32 step1_val;
	u32 ctrl_reg_addr;

	u32 ctrl_bit_ofs;
	u32 step2_val;
	u32 split2_val;

} pmu_general_contrl_info;

#define U_BOOT_PMU_GENERAL_INIT(_name)                                             \
	ll_entry_declare(struct sunxi_pmu_general_dev_t, _name, pmu_general)

/* private function */
int pmic_general_bus_init(u16 device_addr, u16 runtime_addr);
int pmic_general_bus_read(unsigned int i2c_bus, u16 runtime_addr, u8 reg, u8 *data);
int pmic_general_bus_write(unsigned int i2c_bus, u16 runtime_addr, u8 reg, u8 data);
int pmic_general_bus_setbits(unsigned int i2c_bus, u16 runtime_addr, u8 reg, u8 bits);
int pmic_general_bus_clrbits(unsigned int i2c_bus, u16 runtime_addr, u8 reg, u8 bits);
/* public function */
int pmu_general_probe(void);
bool pmu_general_get_exist(void);
int pmu_general_get_voltage_by_phandle(const void *fdt, uint32_t phandle);
int pmu_general_get_voltage_by_full_name(char *name);
int pmu_general_set_voltage_by_phandle(const void *fdt, uint32_t phandle, uint vol_value, uint onoff);
int pmu_general_set_voltage_by_full_name(char *name, uint vol_value, uint onoff);

#endif /* __PMU_general_H__ */
