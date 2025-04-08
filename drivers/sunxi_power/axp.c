/*
 * Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
*/

#include <common.h>
#include <sys_config.h>
#include <sunxi_power/axp.h>
#include <sunxi_power/power_manage.h>

DECLARE_GLOBAL_DATA_PTR;

int do_poweroff(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
#ifdef CONFIG_SUNXI_PMU
	pmu_set_power_off();
#else
	#error "NO FOUND PMU"
#endif
#ifdef CONFIG_SUNXI_BMU
	bmu_set_power_off();
#endif

	while (1)
	;

	return 0;
}

int axp_reg_debug(void)
{
	int ret = 0;
#ifdef CONFIG_SUNXI_PMU
	ret = pmu_reg_debug();
#endif
#ifdef CONFIG_SUNXI_BMU
	ret = bmu_reg_debug();
#endif
	return ret;
}

int axp_probe(void)
{
	int ret = 0;
	int __maybe_unused work_mode = get_boot_work_mode();

#ifdef CONFIG_SUNXI_PMU
	if (!pmu_probe()) {
		axp_set_dcdc_mode();
		axp_set_power_supply_output();
	}
#else
	#error "NO FOUND PMU"
#endif

#ifdef CONFIG_SUNXI_BMU
#ifdef CONFIG_SUNXI_BMU_EXT
	if ((!bmu_probe()) || (bmu_ext_get_exist() == true)) {
#else
	if (!bmu_probe()) {
#endif /* CONFIG_SUNXI_BMU_EXT */
#if !defined CONFIG_AXP_LATE_INFO
		gd->pmu_saved_status = bmu_get_poweron_source();
		gd->pmu_runtime_chgcur = axp_get_battery_status();

		if (work_mode != WORK_MODE_BOOT) {
			ret = axp_reset_capacity();
			if (!ret) {
				pr_msg("axp reset capacity fail!\n");
			}
		}
#else
		gd->pmu_saved_status = -1;
		gd->pmu_runtime_chgcur = BATTERY_IS_NOT_EXIST;
#endif /* !CONFIG_AXP_LATE_INFO */
	}
#else
	gd->pmu_saved_status = -1;
	gd->pmu_runtime_chgcur = BATTERY_IS_NOT_EXIST;
#endif /* CONFIG_SUNXI_BMU */

	axp_reg_debug();
	return ret;
}
