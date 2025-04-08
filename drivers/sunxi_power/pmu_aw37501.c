/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI aw37501  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <sunxi_power/pmu_aw37501.h>
#include <sunxi_power/pmu_general.h>
#include <asm/arch/pmic_bus.h>

unsigned int aw37501_bus_num;
unsigned int aw37501_reg;

static pmu_general_contrl_info pmu_general_aw37501_ctrl_tbl[] = {
	/*name,    min,  max, reg,  mask, step0,split1_val, step1,ctrl_reg,ctrl_bit */
	{ "voutp", 4000, 6000, AW37501_VOUTP_VOL, 0x1f, 100, 0, 0, AW37501_CTRL, 3},
	{ "voutn", 4000, 6000, AW37501_VOUTN_VOL, 0x1f, 100, 0, 0, AW37501_CTRL, 4},
};

static pmu_general_contrl_info *get_ctrl_info_from_tbl(char *name)
{
	int i    = 0;
	int size = ARRAY_SIZE(pmu_general_aw37501_ctrl_tbl);
	pmu_general_contrl_info *p;

	for (i = 0; i < size; i++) {
		if (!strncmp(name, pmu_general_aw37501_ctrl_tbl[i].name,
			     strlen(pmu_general_aw37501_ctrl_tbl[i].name))) {
			break;
		}
	}
	if (i >= size) {
		pmu_err("can't find %s from table\n", name);
		return NULL;
	}
	p = pmu_general_aw37501_ctrl_tbl + i;
	return p;
}

int pmu_aw37501_get_reg_from_dts(void)
{
	int reg, nodeoffset;

	/* get used aw37501 node */
	nodeoffset = fdt_path_offset(working_fdt, "aw37501");
	if (nodeoffset < 0) {
		pr_err("Could not find nodeoffset\n");
		return -1;
	}

	fdt_getprop_u32(working_fdt, nodeoffset, "reg", (u32 *)&reg);

	return reg;
}

int pmu_aw37501_probe(void)
{
	u8 pmu_chip_id;
	int reg;

	reg = pmu_aw37501_get_reg_from_dts();
	if (reg < 0)
		aw37501_reg = AW37501_RUNTIME_ADDR;
	else
		aw37501_reg = reg;

	aw37501_bus_num = sunxi_get_i2c_bus_num_from_device_addr(reg);

	if (pmic_general_bus_init(aw37501_bus_num, aw37501_reg)) {
		tick_printf("%s pmic_bus_init fail\n", __func__);
		return -1;
	}

	if (pmic_general_bus_read(aw37501_bus_num, aw37501_reg, AW37501_ID, &pmu_chip_id)) {
		tick_printf("%s pmic_general_bus_read fail\n", __func__);
		return -1;
	}

	pmu_chip_id &= 0x3;

	if (pmu_chip_id == 0x01) {
		tick_printf("PMU_GENERAL: AW37501\n");
		return 0;
	}

	tick_printf("PMU_GENERAL: NO FOUND\n");

	return -1;
}

static int pmu_aw37501_set_voltage(char *name, uint set_vol, uint onoff)
{
	u8 reg_value;
	pmu_general_contrl_info *p_item = NULL;
	u8 base_step		= 0;

	p_item = get_ctrl_info_from_tbl(name);
	if (!p_item) {
		return -1;
	}

	pmu_info(
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
		if (pmic_general_bus_read(aw37501_bus_num, aw37501_reg, p_item->cfg_reg_addr,
				  &reg_value)) {
			return -1;
		}

		reg_value &= ~p_item->cfg_reg_mask;
		if (p_item->split2_val && (set_vol > p_item->split2_val)) {
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
				pmu_err("bad split val(%d) for %s\n",
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
		if (pmic_general_bus_write(aw37501_bus_num, aw37501_reg, p_item->cfg_reg_addr,
				   reg_value)) {
			pmu_err("unable to set %s\n", name);
			return -1;
		}
	}

	if (onoff < 0) {
		return 0;
	}
	if (pmic_general_bus_write(aw37501_bus_num, aw37501_reg, AW37501_WPRTEN, 0x4c)) {
		return -1;
	}
	if (pmic_general_bus_read(aw37501_bus_num, aw37501_reg, p_item->ctrl_reg_addr,
			  &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << p_item->ctrl_bit_ofs);
	} else {
		reg_value |= (1 << p_item->ctrl_bit_ofs);
	}
	if (pmic_general_bus_write(aw37501_bus_num, aw37501_reg, p_item->ctrl_reg_addr,
			   reg_value)) {
		pmu_err("unable to onoff %s\n", name);
		if (pmic_general_bus_write(aw37501_bus_num, AW37501_WPRTEN, 0x4c, reg_value)) {
			return -1;
		}
		return -1;
	}
	if (pmic_general_bus_write(aw37501_bus_num, aw37501_reg, AW37501_WPRTEN, 0x00)) {
		return -1;
	}
	return 0;
}

static int pmu_aw37501_get_voltage(char *name)
{
	u8 reg_value;
	pmu_general_contrl_info *p_item = NULL;
	u8 base_step;
	int vol;

	p_item = get_ctrl_info_from_tbl(name);
	if (!p_item) {
		return -1;
	}

	if (pmic_general_bus_read(aw37501_bus_num, aw37501_reg, p_item->ctrl_reg_addr,
			  &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << p_item->ctrl_bit_ofs))) {
		return 0;
	}

	if (pmic_general_bus_read(aw37501_bus_num, aw37501_reg, p_item->cfg_reg_addr,
			  &reg_value)) {
		return -1;
	}
	reg_value &= p_item->cfg_reg_mask;
	if (p_item->split2_val) {
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

U_BOOT_PMU_GENERAL_INIT(pmu_aw37501) = {
	.pmu_general_name	  = "pmu_aw37501",
	.probe			  = pmu_aw37501_probe,
	.set_voltage      = pmu_aw37501_set_voltage,
	.get_voltage      = pmu_aw37501_get_voltage,
};
