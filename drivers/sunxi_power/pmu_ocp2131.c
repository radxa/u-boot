/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI ocp2131  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <sunxi_power/pmu_ocp2131.h>
#include <sunxi_power/pmu_general.h>
#include <asm/arch/pmic_bus.h>

unsigned int ocp2131_bus_num;
unsigned int ocp2131_reg;

static pmu_general_contrl_info pmu_general_ocp2131_ctrl_tbl[] = {
	/*name,    min,  max, reg,  mask, step0,split1_val, step1,ctrl_reg,ctrl_bit */
	{ "avdd", 4000, 6000, OCP2131_AVDD_VOL, 0x1f, 100, 0, 0},
	{ "avee", 4000, 6000, OCP2131_AVEE_VOL, 0x1f, 100, 0, 0},
};

static pmu_general_contrl_info *get_ctrl_info_from_tbl(char *name)
{
	int i    = 0;
	int size = ARRAY_SIZE(pmu_general_ocp2131_ctrl_tbl);
	pmu_general_contrl_info *p;

	for (i = 0; i < size; i++) {
		if (!strncmp(name, pmu_general_ocp2131_ctrl_tbl[i].name,
			     strlen(pmu_general_ocp2131_ctrl_tbl[i].name))) {
			break;
		}
	}
	if (i >= size) {
		pmu_err("can't find %s from table\n", name);
		return NULL;
	}
	p = pmu_general_ocp2131_ctrl_tbl + i;
	return p;
}

int pmu_ocp2131_get_reg_from_dts(void)
{
	int reg, nodeoffset;

	/* get used ocp2131 node */
	nodeoffset = fdt_path_offset(working_fdt, "ocp2131");
	if (nodeoffset < 0) {
		pr_err("Could not find nodeoffset\n");
		return -1;
	}

	fdt_getprop_u32(working_fdt, nodeoffset, "reg", (u32 *)&reg);

	return reg;
}

int pmu_ocp2131_probe(void)
{
	u8 pmu_chip_id;
	int reg;

	reg = pmu_ocp2131_get_reg_from_dts();
	if (reg < 0)
		ocp2131_reg = OCP2131_RUNTIME_ADDR;
	else
		ocp2131_reg = reg;

	ocp2131_bus_num = sunxi_get_i2c_bus_num_from_device_addr(reg);

	if (pmic_general_bus_init(ocp2131_bus_num, ocp2131_reg)) {
		tick_printf("%s pmic_bus_init fail\n", __func__);
		return -1;
	}

	if (pmic_general_bus_read(ocp2131_bus_num, ocp2131_reg, OCP2131_ID, &pmu_chip_id)) {
		tick_printf("%s pmic_general_bus_read fail\n", __func__);
		return -1;
	}

	if (pmu_chip_id == 0x0D) {
		tick_printf("PMU_GENERAL: OCP2131\n");
		return 0;
	}

	tick_printf("PMU_GENERAL: NO FOUND\n");

	return -1;
}

static int pmu_ocp2131_set_voltage(char *name, uint set_vol, uint onoff)
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
		if (pmic_general_bus_read(ocp2131_bus_num, ocp2131_reg, p_item->cfg_reg_addr,
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
		if (pmic_general_bus_write(ocp2131_bus_num, ocp2131_reg, p_item->cfg_reg_addr,
				   reg_value)) {
			pmu_err("unable to set %s\n", name);
			return -1;
		}
	}

	return 0;
}

static int pmu_ocp2131_get_voltage(char *name)
{
	u8 reg_value;
	pmu_general_contrl_info *p_item = NULL;
	u8 base_step;
	int vol;

	p_item = get_ctrl_info_from_tbl(name);
	if (!p_item) {
		return -1;
	}

	if (pmic_general_bus_read(ocp2131_bus_num, ocp2131_reg, p_item->ctrl_reg_addr,
			  &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << p_item->ctrl_bit_ofs))) {
		return 0;
	}

	if (pmic_general_bus_read(ocp2131_bus_num, ocp2131_reg, p_item->cfg_reg_addr,
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

U_BOOT_PMU_GENERAL_INIT(pmu_ocp2131) = {
	.pmu_general_name	  = "pmu_ocp2131",
	.probe			  = pmu_ocp2131_probe,
	.set_voltage      = pmu_ocp2131_set_voltage,
	.get_voltage      = pmu_ocp2131_get_voltage,
};
