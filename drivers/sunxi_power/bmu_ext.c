/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI BMU_EXT  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <sunxi_power/bmu_ext.h>
#include <asm/arch/pmic_bus.h>


#ifdef AXP_DEBUG
#define axp_err(fmt...) tick_printf("[bmu_ext][err]: " fmt)
#else
#define axp_err(fmt...)
#endif

__attribute__((section(".data"))) static struct sunxi_bmu_ext_dev_t *sunxi_bmu_ext_dev =
	NULL;

const char *const bmu_ext[] = {
	[ETA6973] = "qc-charger",
};

/* traverse the u-boot segment to find the pmu offset*/
static struct sunxi_bmu_ext_dev_t *bmu_ext_get_dev_t(void)
{
	struct sunxi_bmu_ext_dev_t *sunxi_bmu_ext_dev_temp;
	struct sunxi_bmu_ext_dev_t *sunxi_bmu_ext_dev_start =
		ll_entry_start(struct sunxi_bmu_ext_dev_t, bmu_ext);
	int max = ll_entry_count(struct sunxi_bmu_ext_dev_t, bmu_ext);
	for (sunxi_bmu_ext_dev_temp = sunxi_bmu_ext_dev_start;
	     sunxi_bmu_ext_dev_temp != sunxi_bmu_ext_dev_start + max;
	     sunxi_bmu_ext_dev_temp++) {
		if (!strncmp("bmu", sunxi_bmu_ext_dev_temp->bmu_ext_name, 3)) {
			if (!sunxi_bmu_ext_dev_temp->probe()) {
				pr_msg("BMU_EXT: %s found\n",
				       sunxi_bmu_ext_dev_temp->bmu_ext_name);
				return sunxi_bmu_ext_dev_temp;
			}
		}
	}
	pr_msg("BMU_EXT: no found\n");
	return NULL;
}

/* matches chipid*/
int bmu_ext_probe(void)
{
	sunxi_bmu_ext_dev = bmu_ext_get_dev_t();
	if (sunxi_bmu_ext_dev == NULL)
		return -1;
	return 0;
}

/*get bmu_ext exist*/
bool bmu_ext_get_exist(void)
{
	if ((sunxi_bmu_ext_dev) && (sunxi_bmu_ext_dev->get_exist()))
		return sunxi_bmu_ext_dev->get_exist();
	axp_err("not exist bmu_ext:%s\n", __func__);
	return false;
}

/*get bmu_ext type*/
int bmu_ext_get_vbus_status(void)
{
	if  ((sunxi_bmu_ext_dev) && (sunxi_bmu_ext_dev->get_vbus_status))
		return sunxi_bmu_ext_dev->get_vbus_status();
	axp_err("not imple:%s\n", __func__);
	return -1;
}

/*get bmu_ext poweron sourc*/
int bmu_ext_get_poweron_source(int poweron_source)
{
	if  ((sunxi_bmu_ext_dev) && (sunxi_bmu_ext_dev->get_poweron_source))
		return sunxi_bmu_ext_dev->get_poweron_source(poweron_source);

	axp_err("not imple:%s\n", __func__);
	return -1;
}

/*get bmu_ext poweron sourc*/
int bmu_ext_get_battery_probe(void)
{
	if  ((sunxi_bmu_ext_dev) && (sunxi_bmu_ext_dev->get_battery_probe))
		return sunxi_bmu_ext_dev->get_battery_probe();
	axp_err("not imple:%s\n", __func__);
	return -1;
}

/*get bmu_ext poweron sourc*/
int bmu_ext_set_discharge(void)
{
	if  ((sunxi_bmu_ext_dev) && (sunxi_bmu_ext_dev->set_discharge))
		return sunxi_bmu_ext_dev->set_discharge();
	axp_err("not imple:%s\n", __func__);
	return -1;
}

/*get bmu_ext poweron sourc*/
int bmu_ext_set_charge(void)
{
	if  ((sunxi_bmu_ext_dev) && (sunxi_bmu_ext_dev->set_charge))
		return sunxi_bmu_ext_dev->set_charge();
	axp_err("not imple:%s\n", __func__);
	return -1;
}

/*get bmu_ext poweron sourc*/
int bmu_ext_get_charge(void)
{
	if  ((sunxi_bmu_ext_dev) && (sunxi_bmu_ext_dev->get_charge))
		return sunxi_bmu_ext_dev->get_charge();
	axp_err("not imple:%s\n", __func__);
	return -1;
}

/*get bmu_ext type*/
int bmu_ext_get_type(void)
{
	if (sunxi_bmu_ext_dev == NULL)
		return -1;

	if (!strncmp(sunxi_bmu_ext_dev->bmu_ext_name, "bmu_eta6973", sizeof("bmu_eta6973")))
		return ETA6973;

	if (!strncmp(sunxi_bmu_ext_dev->bmu_ext_name, "bmu_axp519", sizeof("bmu_axp519")))
		return AXP519;

	axp_err("error bmu_ext type:%s\n", __func__);
	return -1;
}

/*get bmu_ext_get_ntc_temp*/
int bmu_ext_get_ntc_temp(int param[16])
{
	if  ((sunxi_bmu_ext_dev) && (sunxi_bmu_ext_dev->get_ntc_temp))
		return sunxi_bmu_ext_dev->get_ntc_temp((int *)param);
	axp_err("not imple:%s\n", __func__);
	return -251;
}

/*Set ntc onoff*/
int bmu_ext_set_ntc_onoff(int onoff, int ntc_cur)
{
	if ((sunxi_bmu_ext_dev) && (sunxi_bmu_ext_dev->set_ntc_onoff))
		return sunxi_bmu_ext_dev->set_ntc_onoff(onoff, ntc_cur);
	axp_err("not imple:%s\n", __func__);
	return -1;
}

/*Get the average battery voltage*/
int bmu_ext_get_battery_vol(void)
{
	if ((sunxi_bmu_ext_dev) && (sunxi_bmu_ext_dev->get_battery_vol))
		return sunxi_bmu_ext_dev->get_battery_vol();
	axp_err("not imple:%s\n", __func__);
	return -1;
}

/*Get battery capacity*/
int bmu_ext_get_battery_capacity(void)
{
	if ((sunxi_bmu_ext_dev) && (sunxi_bmu_ext_dev->get_battery_capacity))
		return sunxi_bmu_ext_dev->get_battery_capacity();
	axp_err("not imple:%s\n", __func__);
	return -1;
}

/*reset battery capacity to zero */
int bmu_ext_reset_capacity(void)
{
	if  ((sunxi_bmu_ext_dev) && (sunxi_bmu_ext_dev->reset_capacity))
		return sunxi_bmu_ext_dev->reset_capacity();
	axp_err("not imple:%s\n", __func__);
	return -1;
}
