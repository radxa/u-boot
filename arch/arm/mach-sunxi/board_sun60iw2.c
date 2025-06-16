/*
 * Allwinner Sun60iw2 do poweroff in uboot with arisc
 *
 * (C) Copyright 2021  <xinouyang@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <sunxi_board.h>
#include <smc.h>
#include <asm/arch/usb.h>
#include <asm/arch/watchdog.h>
#include <sprite.h>

#ifdef CONFIG_SUNXI_POWER
#include <sunxi_power/power_manage.h>
#include <sunxi_power/pmu_axp8191.h>
#include <linux/delay.h>
#include <linux/libfdt.h>
#include <fdt_support.h>
#include <linux/bitops.h>
#endif

#include <asm/arch-sunxi/efuse.h>
#include <asm/arch/gpio.h>

#define SUNXI_SOC_VER_B		(0x1)
#define SUNXI_GPIO_VCCIO	(12)

int sunxi_platform_power_off(int status)
{
	int work_mode = get_boot_work_mode();
	/* imporve later */
	ulong cfg_base = 0;

	if (work_mode != WORK_MODE_BOOT)
		return 0;

	/* startup cpus before shutdown */
	arm_svc_arisc_startup(cfg_base);

	if (status)
		arm_svc_poweroff_charge();
	else
		arm_svc_poweroff();

	while (1) {
		asm volatile ("wfi");
	}
	return 0;
}

#ifdef CONFIG_SUNXI_PMU_EXT
int update_pmu_ext_info_to_kernel(void)
{
	int nodeoffset, pmu_ext_type, err, i;
	uint32_t phandle = 0;

	/* get pmu_ext type */
	pmu_ext_type = pmu_ext_get_type();
	if (pmu_ext_type < 0) {
		pr_err("Could not find pmu_ext type: %s: L%d\n", __func__, __LINE__);
		return -1;
	}

	/* get used pmu_ext node */
	nodeoffset = fdt_path_offset(working_fdt, pmu_ext_reg[pmu_ext_type]);
	if (nodeoffset < 0) {
		pr_err("Could not find nodeoffset for used ext pmu:%s\n", pmu_ext_reg[pmu_ext_type]);
		return -1;
	}
	/* get used pmu_ext phandle */
	phandle = fdt_get_phandle(working_fdt, nodeoffset);
	if (!phandle) {
		pr_err("Could not find phandle for used ext pmu:%s\n", pmu_ext_reg[pmu_ext_type]);
		return -1;
	}
	pr_debug("get ext power phandle %d\n", phandle);

	/* delete other pmu_ext node */
	for (i = 0; i < NR_PMU_EXT_VARIANTS; i++) {
		if (i == pmu_ext_type)
			continue;

		nodeoffset = fdt_path_offset(working_fdt, pmu_ext[i]);
		if (nodeoffset < 0) {
			pr_warn("Could not find nodeoffset for unused ext pmu:%s\n", pmu_ext[i]);
			continue;
		}

		err = fdt_del_node(working_fdt, nodeoffset);
		if (err < 0) {
			pr_err("WARNING: fdt_del_node can't delete %s from node %s: %s\n",
				"compatible", "status", fdt_strerror(err));
			return -1;
		}
	}

	/* get cpu@4 node */
	nodeoffset = fdt_path_offset(working_fdt, "cpu-ext");
	if (nodeoffset < 0) {
		pr_err("## error: %s: L%d\n", __func__, __LINE__);
		return -1;
	}

	/* Change cpu-supply to ext dcdc*/
	err = fdt_setprop_u32(working_fdt, nodeoffset,
				"cpu-supply", phandle);
	if (err < 0) {
		pr_warn("WARNING: fdt_setprop can't set %s from node %s: %s\n",
			"compatible", "status", fdt_strerror(err));
		return -1;
	}

	return 0;
}
#endif

#define SRAM_CTRL_REG2 (SUNXI_SYSCTRL_BASE + 0x8)
int sunxi_set_sramc_mode(void)
{
	u32 reg_val;

	/* SRAM:set sram to npu, default boot mode */
	reg_val = readl(SRAM_CTRL_REG2);
	reg_val &= ~(0x1 << 1);
	writel(reg_val, SRAM_CTRL_REG2);
	debug("set sram to npu\n");

	reg_val = readl(SRAM_CTRL_REG2);
	if (reg_val & (0x1 << 1))
		pr_err("set sram to npu fail!\n");
	return 0;
}

int sunxi_get_active_boot0_id(void)
{
	uint32_t val = *(uint32_t *)(SUNXI_RTC_BASE + 0x318);
	if (val & (1 << 15)) {
		return (val >> 12) & 0x7;
	} else {
		return (val >> 28) & 0x7;
	}
}

void otg_phy_config(void)
{
	u32 reg_val;
	reg_val = readl((const volatile void __iomem *)(SUNXI_USBOTG_BASE +
							USBC_REG_o_PHYCTL));
	reg_val &= ~(0x01 << USBC_PHY_CTL_SIDDQ);
	reg_val |= 0x01 << USBC_PHY_CTL_VBUSVLDEXT;
	writel(reg_val, (volatile void __iomem *)(SUNXI_USBOTG_BASE +
						  USBC_REG_o_PHYCTL));
}

void sunxi_board_reset_cpu(ulong addr)
{
	static const struct sunxi_wdog *wdog =
		(struct sunxi_wdog *)SUNXI_WDT_BASE;

	writel(((WDT_CFG_KEY << 16) | 0x02), &wdog->ocfg);
	/* Set the watchdog for its shortest interval (.5s) and wait */
	writel(((WDT_CFG_KEY << 16) | WDT_MODE_EN), &wdog->srst);
	while (1) { }

}

#ifdef CONFIG_SUNXI_POWER
int update_bmu_info_to_kernel(void)
{
	int bat_exist;
	int nodeoffset;

	bat_exist	= axp_get_battery_exist();
	if (bat_exist != BATTERY_IS_EXIST) {
		tick_printf("no battery, disabled battery functons\n");

		nodeoffset = fdt_path_offset(working_fdt, "bat_supply");
		if (nodeoffset < 0) {
			pr_err("Could not find nodeoffset for bat_supply\n");
			return -1;
		}
		fdt_set_node_status(working_fdt, nodeoffset, FDT_STATUS_DISABLED, -1);

		nodeoffset = fdt_path_offset(working_fdt, "pmu0");
		if (nodeoffset < 0) {
			pr_err("Could not find nodeoffset for pmu0\n");
			return -1;
		}
		fdt_setprop_u32(working_fdt, nodeoffset, "pmu-charging-poweroff", 1);
	}

	return 0;
}
#endif

#ifdef CONFIG_SUNXI_SET_EFUSE_POWER
int set_efuse_voltage(int status)
{
	int nodeoffset, len;
	int vol;
	const char *power_supply;
	const char *vol_value = "voltage";
	const char *power_name = "power_supply";

	nodeoffset = fdt_path_offset(working_fdt, "/soc/sid");
	if (nodeoffset < 0) {
		printf("libfdt fdt_path_offset() returned %s\n",
				fdt_strerror(nodeoffset));
		return -1;
	}
	vol = fdt_getprop_u32_default_node(working_fdt, nodeoffset, 0, vol_value, -1);
	power_supply = fdt_getprop(working_fdt, nodeoffset, power_name, &len);

	if (status) {
		pmu_set_voltage((char *)power_supply, vol, 1);
		mdelay(20);
	} else {
		mdelay(20);
		pmu_set_voltage((char *)power_supply, 0, 0);
	}

	return 0;
}
#endif

int get_group_bit_offset(enum pin_e port_group)
{
	switch (port_group) {
	case GPIO_GROUP_B:
		return SUNXI_GPIO_VCCIO;
		break;
	case GPIO_GROUP_C:
	case GPIO_GROUP_D:
	case GPIO_GROUP_E:
	case GPIO_GROUP_F:
	case GPIO_GROUP_G:
	case GPIO_GROUP_H:
	case GPIO_GROUP_I:
	case GPIO_GROUP_J:
	case GPIO_GROUP_K:
		return port_group;
		break;
	default:
		return -1;
	}
	return -1;
}

enum io_pow_mode_e io_get_volt_val(enum pin_e port_group)
{
	uint32_t reg;
	uint8_t group_bit_offset = get_group_bit_offset(port_group);
	int ver;

	if ((group_bit_offset < 0) || ((group_bit_offset * 2) >= 32)) {
		pr_err("get port %d volt error:port error\n", port_group);
		return IO_MODE_DEFAULT;
	}

	reg = readl(PIOC_REG_POW_VAL);
	ver = sunxi_efuse_get_soc_ver();
	if ((ver == SUNXI_SOC_VER_B)
			&& (port_group != GPIO_GROUP_C)
			&& (port_group != GPIO_GROUP_E)
			&& (port_group != GPIO_GROUP_K))
		/* (except bank C/E/K): 0 -- 3.3v  1 -- 1.8v */
		return (!(reg & (1 << (group_bit_offset * 2))) != 0) ?
			IO_MODE_3_3_V :
			IO_MODE_1_8_V;
	else
		/* (only bank C/E/K): 0 -- 1.8v  1 -- 3.3v */
		return ((reg & (1 << (group_bit_offset * 2))) != 0) ?
			IO_MODE_3_3_V :
			IO_MODE_1_8_V;
}
