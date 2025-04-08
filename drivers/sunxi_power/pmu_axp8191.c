/*
 * Copyright (C) 2019 Allwinner.
 * lifueng <liufeng@allwinnertech.com>
 *
 * SUNXI AXP8191  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <sunxi_power/pmu_axp8191.h>
#include <sunxi_power/axp.h>
#include <asm/arch/pmic_bus.h>

/*#include <power/sunxi/pmu.h>*/

#ifdef PMU_DEBUG
#define axp_info(fmt...) tick_printf("[axp][info]: " fmt)
#define axp_err(fmt...) tick_printf("[axp][err]: " fmt)
#else
#define axp_info(fmt...)
#define axp_err(fmt...) tick_printf("[axp][err]: " fmt)
#endif

typedef struct _axp_contrl_info {
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

	u32 start_split3_val;
	u32 step3_val;
	u32 split3_val;
} axp_contrl_info;

__attribute__((section(".data"))) axp_contrl_info pmu_axp8191_ctrl_tbl[] = {
	/*name,    min,  max, reg,  mask, step0,split1_val, step1,ctrl_reg,ctrl_bit */
	{ "dcdc1", 1000, 3800, AXP8191_DC1OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_DCDC_POWER_ON_OFF_CTL1, 0 },
	{ "dcdc2", 500, 1540, AXP8191_DC2OUT_VOL, 0x7f, 10, 1200, 20,
	  AXP8191_DCDC_POWER_ON_OFF_CTL1, 1 },
	{ "dcdc3", 500, 1540, AXP8191_DC3OUT_VOL, 0x7f, 10, 1200, 20,
	  AXP8191_DCDC_POWER_ON_OFF_CTL1, 2 },
	{ "dcdc4", 500, 1540, AXP8191_DC4OUT_VOL, 0x7f, 10, 1200, 20,
	  AXP8191_DCDC_POWER_ON_OFF_CTL1, 3 },
	{ "dcdc5", 500, 1540, AXP8191_DC5OUT_VOL, 0x7f, 10, 1200, 20,
	  AXP8191_DCDC_POWER_ON_OFF_CTL1, 4 },
	{ "dcdc6", 500, 2760, AXP8191_DC6OUT_VOL, 0x7f, 10, 1200, 20,
	  AXP8191_DCDC_POWER_ON_OFF_CTL1, 5, 20, 1540, 1800, 40, 2400, },
	{ "dcdc7", 500, 1840, AXP8191_DC7OUT_VOL, 0x7f, 10, 1200, 20,
	  AXP8191_DCDC_POWER_ON_OFF_CTL1, 6, },
	{ "dcdc8", 500, 3400, AXP8191_DC8OUT_VOL, 0x7f, 10, 1200, 20,
	  AXP8191_DCDC_POWER_ON_OFF_CTL1, 7, 100, 1840, },
	{ "dcdc9", 500, 3400, AXP8191_DC9OUT_VOL, 0x7f, 10, 1200, 20,
	  AXP8191_DCDC_POWER_ON_OFF_CTL2, 0, 100, 1840, },

	{ "aldo1", 500, 3400, AXP8191_ALDO1OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL1, 0 },
	{ "aldo2", 500, 3400, AXP8191_ALDO2OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL1, 1 },
	{ "aldo3", 500, 3400, AXP8191_ALDO3OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL1, 2 },
	{ "aldo4", 500, 3400, AXP8191_ALDO4OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL1, 3 },
	{ "aldo5", 500, 3400, AXP8191_ALDO5OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL1, 4 },
	{ "aldo6", 500, 3400, AXP8191_ALDO6OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL1, 5 },

	{ "bldo1", 500, 3400, AXP8191_BLDO1OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL1, 6 },
	{ "bldo2", 500, 3400, AXP8191_BLDO2OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL1, 7 },
	{ "bldo3", 500, 3400, AXP8191_BLDO3OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL2, 0 },
	{ "bldo4", 500, 3400, AXP8191_BLDO4OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL2, 1 },
	{ "bldo5", 500, 3400, AXP8191_BLDO5OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL2, 2 },

	{ "cldo1", 500, 3400, AXP8191_CLDO1OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL2, 3 },
	{ "cldo2", 500, 3400, AXP8191_CLDO2OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL2, 4 },
	{ "cldo3", 500, 3400, AXP8191_CLDO3OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL2, 5 },
	{ "cldo4", 500, 3400, AXP8191_CLDO4OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL2, 6 },
	{ "cldo5", 500, 3400, AXP8191_CLDO5OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL2, 7 },

	{ "dldo1", 500, 3400, AXP8191_DLDO1OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL3, 0 },
	{ "dldo2", 500, 3400, AXP8191_DLDO2OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL3, 1 },
	{ "dldo3", 500, 3400, AXP8191_DLDO3OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL3, 2 },
	{ "dldo4", 500, 3400, AXP8191_DLDO4OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL3, 3 },
	{ "dldo5", 500, 3400, AXP8191_DLDO5OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL3, 4 },
	{ "dldo6", 500, 3400, AXP8191_DLDO6OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL3, 5 },

	{ "eldo1", 500, 1500, AXP8191_ELDO1OUT_VOL, 0x3f, 25, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL3, 6 },
	{ "eldo2", 500, 1500, AXP8191_ELDO2OUT_VOL, 0x3f, 25, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL3, 7 },
	{ "eldo3", 500, 1500, AXP8191_ELDO3OUT_VOL, 0x3f, 25, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL4, 0 },
	{ "eldo4", 500, 1500, AXP8191_ELDO4OUT_VOL, 0x3f, 25, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL4, 1 },
	{ "eldo5", 500, 1500, AXP8191_ELDO5OUT_VOL, 0x3f, 25, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL4, 2 },
	{ "eldo6", 500, 1500, AXP8191_ELDO6OUT_VOL, 0x3f, 25, 0, 0,
	  AXP8191_LDO_POWER_ON_OFF_CTL4, 3 },

	{ "dc1sw1", 1000, 3800, AXP8191_DC1OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_DCDC_POWER_ON_OFF_CTL2, 3 },
	{ "dc1sw2", 1000, 3800, AXP8191_DC1OUT_VOL, 0x1f, 100, 0, 0,
	  AXP8191_DCDC_POWER_ON_OFF_CTL2, 4 },
};

static axp_contrl_info *get_ctrl_info_from_tbl(char *name)
{
	int i    = 0;
	int size = ARRAY_SIZE(pmu_axp8191_ctrl_tbl);
	axp_contrl_info *p;

	for (i = 0; i < size; i++) {
		if (!strncmp(name, pmu_axp8191_ctrl_tbl[i].name,
			     strlen(pmu_axp8191_ctrl_tbl[i].name))) {
			break;
		}
	}
	if (i >= size) {
		axp_err("can't find %s from table\n", name);
		return NULL;
	}
	p = pmu_axp8191_ctrl_tbl + i;
	return p;
}

static int pmu_axp8191_ap_reset_enable(void)
{
	u8 reg_value;

	if (pmic_bus_read(AXP8191_RUNTIME_ADDR, AXP8191_POWER_DISABLE_POWER_DOWN_SEQUENCE, &reg_value))
		return -1;

	reg_value |= 1 << 3;
	if (pmic_bus_write(AXP8191_RUNTIME_ADDR, AXP8191_POWER_DISABLE_POWER_DOWN_SEQUENCE, reg_value))
		return -1;

	return 0;
}

static void pmu_axp8191_set_necessary_reg(void)
{
	u8 reg_value;

	pmic_bus_write(AXP8191_RUNTIME_ADDR, AXP8191_WRITE_LOCK_F1, 0x06);
	pmic_bus_write(AXP8191_RUNTIME_ADDR, AXP8191_EFUSE_CTL, 0x04);
	pmic_bus_write(AXP8191_RUNTIME_ADDR, AXP8191_REG_ADD_EXT, 0x01);

	pmic_bus_read(AXP8191_RUNTIME_ADDR, AXP8191_EXT_CFG(AXP8191_COMMOM_CFG1), &reg_value);
	if (!(reg_value & BIT(6)))
		pmic_bus_setbits(AXP8191_RUNTIME_ADDR, AXP8191_EXT_CFG(AXP8191_COMMOM_CFG1), BIT(6));

	pmic_bus_write(AXP8191_RUNTIME_ADDR, AXP8191_REG_ADD_EXT, 0);
	pmic_bus_write(AXP8191_RUNTIME_ADDR, AXP8191_EFUSE_CTL, 0);
	pmic_bus_write(AXP8191_RUNTIME_ADDR, AXP8191_WRITE_LOCK_F1, 0);
}

static int pmu_axp8191_probe(void)
{
	u8 pmu_chip_id;

	if (pmic_bus_init(AXP8191_DEVICE_ADDR, AXP8191_RUNTIME_ADDR)) {
		tick_printf("%s pmic_bus_init fail\n", __func__);
		return -1;
	}

	if (pmic_bus_read(AXP8191_RUNTIME_ADDR, AXP8191_CHIP_ID, &pmu_chip_id)) {
		tick_printf("%s pmic_bus_read fail\n", __func__);
		return -1;
	}
	/* AXP8191 Version-A-ID:0x03 */
	pmu_chip_id &= 0XFF;
	if (pmu_chip_id == 0x03) {
		/*pmu type AXP21*/
		pmu_axp8191_set_necessary_reg();
		pmu_axp8191_ap_reset_enable();
		tick_printf("PMU: AXP8191\n");

		if (pmic_bus_read(AXP8191_RUNTIME_ADDR, AXP8191_CHIP_VER, &pmu_chip_id)) {
			tick_printf("%s pmic_bus_read fail\n", __func__);
			return -1;
		}

		pmu_chip_id &= 0Xff;

		if (pmu_chip_id == 0x00) {
			/*bmu type AXP21*/
			tick_printf("PMU: AXP8191 VER_A\n");
		}
		return 0;
	}

	return -1;
}

static int pmu_axp8191_set_voltage(char *name, uint set_vol, uint onoff)
{
	u8 reg_value;
	axp_contrl_info *p_item = NULL;
	u8 base_step		= 0;

	p_item = get_ctrl_info_from_tbl(name);
	if (!p_item) {
		return -1;
	}

	axp_info(
		"name %s, min_vol %dmv, max_vol %d, cfg_reg 0x%x, cfg_mask 0x%x \
		step0_val %d, split1_val %d, step1_val %d, ctrl_reg_addr 0x%x, ctrl_bit_ofs %d\n",
		p_item->name, p_item->min_vol, p_item->max_vol,
		p_item->cfg_reg_addr, p_item->cfg_reg_mask, p_item->step0_val,
		p_item->split1_val, p_item->step1_val, p_item->ctrl_reg_addr,
		p_item->ctrl_bit_ofs);

	if ((set_vol > 0) && (p_item->min_vol)) {
		if (set_vol < p_item->min_vol) {
			set_vol = p_item->min_vol;
		} else if (set_vol > p_item->max_vol) {
			set_vol = p_item->max_vol;
		}
		if (pmic_bus_read(AXP8191_RUNTIME_ADDR, p_item->cfg_reg_addr,
				  &reg_value)) {
			return -1;
		}
		reg_value &= ~p_item->cfg_reg_mask;
		if (p_item->split3_val && (set_vol > p_item->split3_val)) {
			base_step = (p_item->split3_val - p_item->start_split3_val) /
				    p_item->step2_val +1;
			base_step += (p_item->split2_val - p_item->split1_val) /
				     p_item->step1_val;
			base_step += (p_item->split1_val - p_item->min_vol) /
				     p_item->step0_val;
			reg_value |= (base_step +
				      (set_vol - p_item->split3_val/p_item->step3_val*p_item->step3_val) /
					      p_item->step3_val);
		} else if (p_item->split3_val && (set_vol >= p_item->start_split3_val)) {
			base_step = (p_item->split2_val - p_item->split1_val) /
				    p_item->step1_val;

			base_step += (p_item->split1_val - p_item->min_vol) /
				     p_item->step0_val;
			reg_value |= (base_step +
				      (set_vol - p_item->start_split3_val) /
					      p_item->step2_val + 1);
		} else if (p_item->split2_val && (set_vol > p_item->split2_val)) {
			base_step = (p_item->split2_val - p_item->split1_val) /
				    p_item->step1_val;

			base_step += (p_item->split1_val - p_item->min_vol) /
				     p_item->step0_val;
			reg_value |= (base_step +
				      (set_vol - p_item->split2_val/p_item->step2_val*p_item->step2_val) /
					      p_item->step2_val);
		} else if (p_item->split1_val &&
			   (set_vol > p_item->split1_val)) {
			if (p_item->split1_val < p_item->min_vol) {
				axp_err("bad split val(%d) for %s\n",
					p_item->split1_val, name);
			}

			base_step = (p_item->split1_val - p_item->min_vol) /
				    p_item->step0_val;
			reg_value |= (base_step +
				      (set_vol - p_item->split1_val) /
					      p_item->step1_val);
		} else {
			reg_value |=
				(set_vol - p_item->min_vol) / p_item->step0_val;
		}
		if (pmic_bus_write(AXP8191_RUNTIME_ADDR, p_item->cfg_reg_addr,
				   reg_value)) {
			axp_err("unable to set %s\n", name);
			return -1;
		}
	}

	if (onoff < 0) {
		return 0;
	}
	if (pmic_bus_read(AXP8191_RUNTIME_ADDR, p_item->ctrl_reg_addr,
			  &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << p_item->ctrl_bit_ofs);
	} else {
		reg_value |= (1 << p_item->ctrl_bit_ofs);
	}
	if (pmic_bus_write(AXP8191_RUNTIME_ADDR, p_item->ctrl_reg_addr,
			   reg_value)) {
		axp_err("unable to onoff %s\n", name);
		return -1;
	}
	return 0;
}

static int pmu_axp8191_get_voltage(char *name)
{
	u8 reg_value;
	axp_contrl_info *p_item = NULL;
	u8 base_step;
	int vol;

	p_item = get_ctrl_info_from_tbl(name);
	if (!p_item) {
		return -1;
	}

	if (pmic_bus_read(AXP8191_RUNTIME_ADDR, p_item->ctrl_reg_addr,
			  &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << p_item->ctrl_bit_ofs))) {
		return 0;
	}
	if (pmic_bus_read(AXP8191_RUNTIME_ADDR, p_item->cfg_reg_addr,
			  &reg_value)) {
		return -1;
	}
	reg_value &= p_item->cfg_reg_mask;

	if (p_item->split3_val) {
		u32 base_step2;
		u32 base_step3;
		base_step = (p_item->split1_val - p_item->min_vol) /
				     p_item->step0_val;

		base_step2 = base_step + (p_item->split2_val - p_item->split1_val) /
			    p_item->step1_val;

		base_step3 = base_step2 + (p_item->split3_val - p_item->start_split3_val) /
			    p_item->step2_val + 1;

		/* only dcdc6 have base_step3 */
		if (reg_value >= base_step3) {
			vol = ALIGN(p_item->split3_val, p_item->step3_val) +
			      p_item->step3_val * (reg_value - base_step3);
		} else if (reg_value >= base_step2) {
			vol = p_item->start_split3_val + p_item->step2_val * (reg_value - base_step2 - 1);
		} else if (reg_value >= base_step) {
			vol = p_item->split1_val +
			      p_item->step1_val * (reg_value - base_step);
		} else {
			vol = p_item->min_vol + p_item->step0_val * reg_value;
		}
	} else if (p_item->split2_val) {
		u32 base_step2;
		base_step = (p_item->split1_val - p_item->min_vol) /
				     p_item->step0_val;

		base_step2 = base_step + (p_item->split2_val - p_item->split1_val) /
			    p_item->step1_val;

		if (reg_value >= base_step2) {
			vol = ALIGN(p_item->split2_val, p_item->step2_val) +
			      p_item->step2_val * (reg_value - base_step2);
		} else if (reg_value >= base_step) {
			vol = p_item->split1_val +
			      p_item->step1_val * (reg_value - base_step);
		} else {
			vol = p_item->min_vol + p_item->step0_val * reg_value;
		}
	} else if (p_item->split1_val) {
		base_step = (p_item->split1_val - p_item->min_vol) /
			    p_item->step0_val;
		if (reg_value > base_step) {
			vol = p_item->split1_val +
			      p_item->step1_val * (reg_value - base_step);
		} else {
			vol = p_item->min_vol + p_item->step0_val * reg_value;
		}
	} else {
		vol = p_item->min_vol + p_item->step0_val * reg_value;
	}
	return vol;
}

static int pmu_axp8191_set_power_off(void)
{
	u8 reg_value;
	if (pmic_bus_read(AXP8191_RUNTIME_ADDR, AXP8191_POWER_DISABLE_POWER_DOWN_SEQUENCE, &reg_value)) {
		return -1;
	}
	reg_value |= (1 << 7);
	if (pmic_bus_write(AXP8191_RUNTIME_ADDR, AXP8191_POWER_DISABLE_POWER_DOWN_SEQUENCE, reg_value)) {
		return -1;
	}
	return 0;
}

static int pmu_axp8191_set_sys_mode(int status)
{

	if (pmic_bus_write(AXP8191_RUNTIME_ADDR, AXP8191_BUFFER0,
			   (u8)status)) {
		return -1;
	}

	return 0;
}

static int pmu_axp8191_get_sys_mode(void)
{

	u8 reg_value;
	if (pmic_bus_read(AXP8191_RUNTIME_ADDR, AXP8191_BUFFER0, &reg_value)) {
		return -1;
	}

	tick_printf("[AXP8191] charge/reboot status:0x%x\n", reg_value);

	if (reg_value & 0x01) {
		reg_value = SUNXI_REBOOT_FLAG;
	} else if (reg_value & 0x08) {
		reg_value = SUNXI_CHARGING_FLAG;
	}

	return reg_value;
}

static int pmu_axp8191_get_key_irq(void)
{
	u8 reg_value;
	if (pmic_bus_read(AXP8191_RUNTIME_ADDR, AXP8191_IRQ_STATUS2, &reg_value)) {
		return -1;
	}
	reg_value &= (0x03 << 3);
	if (reg_value) {
		if (pmic_bus_write(AXP8191_RUNTIME_ADDR, AXP8191_IRQ_STATUS2,
				   reg_value)) {
			return -1;
		}
	}
	return (reg_value >> 3) & 3;
}

static int pmu_axp8191_set_dcdc_mode(const char *name, int mode)
{
	u8 reg_value = 0;
	int mask = -1;

	if (!strncmp(name, "dcdc1_mode", sizeof("dcdc1_mode")))
		mask = AXP8191_DCDC1_PWM_BIT;

	if (!strncmp(name, "dcdc9_mode", sizeof("dcdc2_mode")))
		mask = AXP8191_DCDC9_PWM_BIT;

	if (mask == AXP8191_DCDC1_PWM_BIT) {
		if (pmic_bus_read(AXP8191_RUNTIME_ADDR, AXP8191_DC1OUT_VOL, &reg_value))
			return -1;

		reg_value &= ~(1 << mask);
		reg_value |= (mode << mask);

		if (pmic_bus_write(AXP8191_RUNTIME_ADDR, AXP8191_DC1OUT_VOL, reg_value))
			return -1;
		pr_msg("[AXP8191] %s:%d\n", name, mode);
	} else {
		if (pmic_bus_read(AXP8191_RUNTIME_ADDR, AXP8191_DCDC_MODE_CTL1, &reg_value))
			return -1;

		reg_value &= ~(1 << mask);
		reg_value |= (mode << mask);

		if (pmic_bus_write(AXP8191_RUNTIME_ADDR, AXP8191_DCDC_MODE_CTL1, reg_value))
			return -1;
	}

	pr_msg("[AXP8191] %s:%d\n", name, mode);

	return 0;
}

unsigned char pmu_axp8191_get_reg_value(unsigned char reg_addr)
{
	u8 reg_value;
	if (pmic_bus_read(AXP8191_RUNTIME_ADDR, reg_addr, &reg_value)) {
		return -1;
	}
	return reg_value;
}

unsigned char pmu_axp8191_set_reg_value(unsigned char reg_addr, unsigned char reg_value)
{
	unsigned char reg;
	if (pmic_bus_write(AXP8191_RUNTIME_ADDR, reg_addr, reg_value)) {
		return -1;
	}
	if (pmic_bus_read(AXP8191_RUNTIME_ADDR, reg_addr, &reg)) {
		return -1;
	}
	return reg;
}

int pmu_axp8191_reg_debug(void)
{
	u8 reg_value[2];

	if (pmic_bus_read(AXP8191_RUNTIME_ADDR, AXP8191_POWER_ON_OFF_SOURCE_INDIVATION, &reg_value[0])) {
		return -1;
	}
	if (pmic_bus_read(AXP8191_RUNTIME_ADDR, AXP8191_POWER_OFF_SOURCE_INDIVATION, &reg_value[1])) {
		return -1;
	}
	tick_printf("[AXP8191] onoff status: 0x%x = 0x%x, 0x%x = 0x%x\n", AXP8191_POWER_ON_OFF_SOURCE_INDIVATION, reg_value[0], AXP8191_POWER_OFF_SOURCE_INDIVATION, reg_value[1]);

	if (pmic_bus_read(AXP8191_RUNTIME_ADDR, AXP8191_BUFFER0, &reg_value[1])) {
		return -1;
	}
	tick_printf("[AXP8191] charge status: 0x%x = 0x%x\n", AXP8191_BUFFER0, reg_value[1]);

	return 0;
}


int pmu_axp8191_get_poweron_source(void)
{
	uchar reg_value;
	int reboot_mode;

	reboot_mode = pmu_get_sys_mode();

	if (reboot_mode == SUNXI_REBOOT_FLAG) {
		pmu_set_sys_mode(0);
		return -1;
	}

	if (reboot_mode == SUNXI_CHARGING_FLAG) {
		pmu_set_sys_mode(0);
		return AXP_BOOT_SOURCE_CHARGER;
	}

	if (pmic_bus_read(AXP8191_RUNTIME_ADDR, AXP8191_POWER_ON_OFF_SOURCE_INDIVATION, &reg_value)) {
		return -1;
	}

	reg_value &= 0x07;

	switch (reg_value) {
	case (1 << 0): return AXP_BOOT_SOURCE_PS_L_TO_H;
	case (1 << 1): return AXP_BOOT_SOURCE_BUTTON;
	case (1 << 2): return AXP_BOOT_SOURCE_IRQ_LOW;
	default: return -1;
	}

}

U_BOOT_AXP_PMU_INIT(pmu_axp8191) = {
	.pmu_name	  = "pmu_axp8191",
	.probe		   = pmu_axp8191_probe,
	.set_voltage       = pmu_axp8191_set_voltage,
	.get_voltage       = pmu_axp8191_get_voltage,
	.set_power_off     = pmu_axp8191_set_power_off,
	.set_sys_mode      = pmu_axp8191_set_sys_mode,
	.get_sys_mode      = pmu_axp8191_get_sys_mode,
	.get_key_irq       = pmu_axp8191_get_key_irq,
	/*.set_bus_vol_limit = pmu_axp8191_set_bus_vol_limit,*/
	.set_dcdc_mode     = pmu_axp8191_set_dcdc_mode,
	.get_reg_value	   = pmu_axp8191_get_reg_value,
	.set_reg_value	   = pmu_axp8191_set_reg_value,
	.reg_debug = pmu_axp8191_reg_debug,
	.get_poweron_source     = pmu_axp8191_get_poweron_source,
};
