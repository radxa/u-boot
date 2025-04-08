/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI AXP  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __BMU_EXT_H__
#define __BMU_EXT_H__

#include <common.h>
#include <linker_lists.h>

struct sunxi_bmu_ext_dev_t {
const char *bmu_ext_name;
int (*probe)(void); /* matches chipid*/
bool (*get_exist)(void); /*get bmu ext exist*/
int (*get_vbus_status)(void); /*get vbus status*/
int (*get_poweron_source)(int poweron_source); /*get poweron source*/
int (*get_battery_probe)(void); /*get battery status*/
int (*set_discharge)(void); /*get battery status*/
int (*set_charge)(void); /*get battery status*/
int (*get_charge)(void); /*get battery status*/
int (*get_ntc_temp)(int param[]); /*get battery temp*/
int (*set_ntc_onoff)(int onoff, int ntc_cur); /*get battery temp*/
int (*get_battery_vol)(void); /*Get the average battery voltage*/
int (*get_battery_capacity)(void); /*Get battery capacity*/
int (*reset_capacity)(void);
};

enum {
	ETA6973 = 0,
	AXP519,
	NR_BMU_EXT_VARIANTS,
};

#define U_BOOT_BMU_EXT_INIT(_name)                                             \
	ll_entry_declare(struct sunxi_bmu_ext_dev_t, _name, bmu_ext)

int bmu_ext_probe(void);
bool bmu_ext_get_exist(void);
int bmu_ext_get_vbus_status(void);
int bmu_ext_get_poweron_source(int poweron_source);
int bmu_ext_get_battery_probe(void);
int bmu_ext_get_type(void);
int bmu_ext_set_discharge(void);
int bmu_ext_set_charge(void);
int bmu_ext_get_charge(void);
int bmu_ext_get_ntc_temp(int param[]);
int bmu_ext_set_ntc_onoff(int onoff, int ntc_cur);
int bmu_ext_get_battery_vol(void);
int bmu_ext_get_battery_capacity(void);
int bmu_ext_reset_capacity(void);

extern const char *const bmu_ext[];


#endif /* __BMU_EXT_H__ */
