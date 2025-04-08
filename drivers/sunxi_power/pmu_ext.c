/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI PMU_EXT  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <sunxi_power/pmu_ext.h>
#include <asm/arch/pmic_bus.h>


#ifdef AXP_DEBUG
#define axp_err(fmt...) tick_printf("[pmu_ext][err]: " fmt)
#else
#define axp_err(fmt...)
#endif

__attribute__((section(".data"))) static struct sunxi_pmu_ext_dev_t *sunxi_pmu_ext_dev =
	NULL;

const char *const pmu_ext_reg[] = {
	[TCS4838] = "reg-tcs0",
	[SY8827G] = "reg-sy0",
	[AXP1530] = "reg-axp1530",
};

const char *const pmu_ext[] = {
	[TCS4838] = "tcs0",
	[SY8827G] = "sy0",
	[AXP1530] = "axp1530",
};

/* traverse the u-boot segment to find the pmu offset*/
static struct sunxi_pmu_ext_dev_t *pmu_ext_get_dev_t(void)
{
	struct sunxi_pmu_ext_dev_t *sunxi_pmu_ext_dev_temp;
	struct sunxi_pmu_ext_dev_t *sunxi_pmu_ext_dev_start =
		ll_entry_start(struct sunxi_pmu_ext_dev_t, pmu_ext);
	int max = ll_entry_count(struct sunxi_pmu_ext_dev_t, pmu_ext);
	for (sunxi_pmu_ext_dev_temp = sunxi_pmu_ext_dev_start;
	     sunxi_pmu_ext_dev_temp != sunxi_pmu_ext_dev_start + max;
	     sunxi_pmu_ext_dev_temp++) {
		if (!strncmp("pmu", sunxi_pmu_ext_dev_temp->pmu_ext_name, 3)) {
			if (!sunxi_pmu_ext_dev_temp->probe()) {
				pr_msg("PMU_EXT: %s found\n",
				       sunxi_pmu_ext_dev_temp->pmu_ext_name);
				return sunxi_pmu_ext_dev_temp;
			}
		}
	}
	pr_msg("PMU: no found\n");
	return NULL;
}

/* matches chipid*/
int pmu_ext_probe(void)
{
	sunxi_pmu_ext_dev = pmu_ext_get_dev_t();
	if (sunxi_pmu_ext_dev == NULL)
		return -1;
	return 0;
}

/*get pmu_ext exist*/
bool pmu_ext_get_exist(void)
{
	if ((sunxi_pmu_ext_dev) && (sunxi_pmu_ext_dev->get_exist()))
		return sunxi_pmu_ext_dev->get_exist();
	axp_err("not exist pmu_ext:%s\n", __func__);
	return false;
}

/*Force DCDC in pwm mode or not */
int pmu_set_dcdc_mode_ext(const char *name, int mode)
{
	if ((sunxi_pmu_ext_dev) && (sunxi_pmu_ext_dev->set_dcdc_mode))
		return sunxi_pmu_ext_dev->set_dcdc_mode(name, mode);
	axp_err("not imple:%s\n", __func__);
	return -1;
}

/*Set a certain power, voltage value. */
int pmu_ext_set_voltage(char *name, uint vol_value, uint onoff)
{
	if ((sunxi_pmu_ext_dev) && (sunxi_pmu_ext_dev->set_voltage))
		return sunxi_pmu_ext_dev->set_voltage(name, vol_value, onoff);
	axp_err("not imple:%s\n", __func__);
	return -1;
}

/*Read a certain power, voltage value */
int pmu_ext_get_voltage(char *name)
{
	if ((sunxi_pmu_ext_dev) && (sunxi_pmu_ext_dev->get_voltage))
		return sunxi_pmu_ext_dev->get_voltage(name);
	axp_err("not imple:%s\n", __func__);
	return -1;
}
/*get pmu_ext type*/
int pmu_ext_get_type(void)
{
	if (sunxi_pmu_ext_dev == NULL)
		return -1;

	if (!strncmp(sunxi_pmu_ext_dev->pmu_ext_name, "pmu_tcs4838", sizeof("pmu_tcs4838")))
		return TCS4838;

	if (!strncmp(sunxi_pmu_ext_dev->pmu_ext_name, "pmu_sy8827g", sizeof("pmu_sy8827g")))
		return SY8827G;

	if (!strncmp(sunxi_pmu_ext_dev->pmu_ext_name, "pmu_axp1530", sizeof("pmu_axp1530")))
		return AXP1530;
	axp_err("error pmu_ext type:%s\n", __func__);
	return -1;
}

/*get pmu name*/
const char *pmu_ext_get_name(void)
{
	if (!pmu_ext_get_exist())
		return NULL;

	if ((sunxi_pmu_ext_dev) && (sunxi_pmu_ext_dev->pmu_ext_name))
		return sunxi_pmu_ext_dev->pmu_ext_name;

	return NULL;
}