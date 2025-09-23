// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025 MediaTek Inc. All Rights Reserved.
 * Author: Bo-Chen Chen <rex-bc.chen@mediatek.com>
 */

#include <dm.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <power/regulator.h>
#include <power/mt6359.h>
#include <power/mt6359p.h>
#include <power/pmic.h>

#include <dm/device.h>
#include <dm/device_compat.h>

enum mt6359_regulator_type {
	MT6359_REG_TYPE_LINEAR = 0,
	MT6359_REG_TYPE_TABLE,
	MT6359_REG_TYPE_FIXED,
	MT6359_REG_TYPE_VEMC,
};

struct regulator_desc {
	const char *name;
	const char *of_match;
	enum mt6359_regulator_type type;
	int id;
	unsigned int uV_step;
	unsigned int n_voltages;
	const unsigned int *volt_table;
	unsigned int min_uV;
	unsigned int vsel_reg;
	unsigned int vsel_mask;
	unsigned int enable_reg;
	unsigned int enable_mask;
	unsigned int fixed_uV;
};

/*
 * MT6359 regulators' information
 *
 * @desc: standard fields of regulator description.
 * @status_reg: for query status of regulators.
 * @qi: Mask for query enable signal status of regulators.
 * @modeset_reg: for operating AUTO/PWM mode register.
 * @modeset_mask: MASK for operating modeset register.
 */
struct mt6359_regulator_info {
	struct regulator_desc desc;
	u32 status_reg;
	u32 qi;
	u32 modeset_reg;
	u32 modeset_mask;
	u32 lp_mode_reg;
	u32 lp_mode_mask;
};

#define MT6359_BUCK(match, _name, _min, _max, _step,		\
	_enable_reg, _status_reg,				\
	_vsel_reg, _vsel_mask,					\
	_lp_mode_reg, _lp_mode_shift,				\
	_modeset_reg, _modeset_shift)				\
[MT6359_ID_##_name] = {						\
	.desc = {						\
		.name = #_name,					\
		.of_match = of_match_ptr(match),		\
		.type = MT6359_REG_TYPE_LINEAR,			\
		.id = MT6359_ID_##_name,			\
		.uV_step = (_step),				\
		.n_voltages = ((_max) - (_min)) / (_step) + 1,	\
		.min_uV = (_min),				\
		.vsel_reg = _vsel_reg,				\
		.vsel_mask = _vsel_mask,			\
		.enable_reg = _enable_reg,			\
		.enable_mask = BIT(0),				\
	},							\
	.status_reg = _status_reg,				\
	.qi = BIT(0),						\
	.lp_mode_reg = _lp_mode_reg,				\
	.lp_mode_mask = BIT(_lp_mode_shift),			\
	.modeset_reg = _modeset_reg,				\
	.modeset_mask = BIT(_modeset_shift),			\
}

#define MT6359_LDO_LINEAR(match, _name, _min, _max, _step,	\
	_enable_reg, _status_reg, _vsel_reg, _vsel_mask)	\
[MT6359_ID_##_name] = {						\
	.desc = {						\
		.name = #_name,					\
		.of_match = of_match_ptr(match),		\
		.type = MT6359_REG_TYPE_LINEAR,			\
		.id = MT6359_ID_##_name,			\
		.uV_step = (_step),				\
		.n_voltages = ((_max) - (_min)) / (_step) + 1,	\
		.min_uV = (_min),				\
		.vsel_reg = _vsel_reg,				\
		.vsel_mask = _vsel_mask,			\
		.enable_reg = _enable_reg,			\
		.enable_mask = BIT(0),				\
	},							\
	.status_reg = _status_reg,				\
	.qi = BIT(0),						\
}

#define MT6359_LDO(match, _name, _tmp_volt_table,		\
	_enable_reg, _enable_mask, _status_reg,			\
	_vsel_reg, _vsel_mask, _en_delay)			\
[MT6359_ID_##_name] = {						\
	.desc = {						\
		.name = #_name,					\
		.of_match = of_match_ptr(match),		\
		.type = MT6359_REG_TYPE_TABLE,			\
		.id = MT6359_ID_##_name,			\
		.n_voltages = ARRAY_SIZE(_tmp_volt_table),	\
		.volt_table = _tmp_volt_table,			\
		.vsel_reg = _vsel_reg,				\
		.vsel_mask = _vsel_mask,			\
		.enable_reg = _enable_reg,			\
		.enable_mask = BIT(_enable_mask),		\
	},							\
	.status_reg = _status_reg,				\
	.qi = BIT(0),						\
}

#define MT6359_REG_FIXED(match, _name, _enable_reg,	\
	_status_reg, _fixed_volt)			\
[MT6359_ID_##_name] = {					\
	.desc = {					\
		.name = #_name,				\
		.of_match = of_match_ptr(match),	\
		.type = MT6359_REG_TYPE_FIXED,		\
		.id = MT6359_ID_##_name,		\
		.n_voltages = 1,			\
		.enable_reg = _enable_reg,		\
		.enable_mask = BIT(0),			\
		.fixed_uV = (_fixed_volt),		\
	},						\
	.status_reg = _status_reg,			\
	.qi = BIT(0),					\
}

#define MT6359P_LDO1(match, _name, _type, _tmp_volt_table,	\
	_enable_reg, _enable_mask, _status_reg,			\
	_vsel_reg, _vsel_mask)					\
[MT6359_ID_##_name] = {						\
	.desc = {						\
		.name = #_name,					\
		.of_match = of_match_ptr(match),		\
		.type = _type,					\
		.id = MT6359_ID_##_name,			\
		.n_voltages = ARRAY_SIZE(_tmp_volt_table),	\
		.volt_table = _tmp_volt_table,			\
		.vsel_reg = _vsel_reg,				\
		.vsel_mask = _vsel_mask,			\
		.enable_reg = _enable_reg,			\
		.enable_mask = BIT(_enable_mask),		\
	},							\
	.status_reg = _status_reg,				\
	.qi = BIT(0),						\
}

static const unsigned int vsim1_voltages[] = {
	0, 0, 0, 1700000, 1800000, 0, 0, 0, 2700000, 0, 0, 3000000, 3100000,
};

static const unsigned int vibr_voltages[] = {
	1200000, 1300000, 1500000, 0, 1800000, 2000000, 0, 0, 2700000, 2800000,
	0, 3000000, 0, 3300000,
};

static const unsigned int vrf12_voltages[] = {
	0, 0, 1100000, 1200000,	1300000,
};

static const unsigned int volt18_voltages[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1700000, 1800000, 1900000,
};

static const unsigned int vcn13_voltages[] = {
	900000, 1000000, 0, 1200000, 1300000,
};

static const unsigned int vcn33_voltages[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 2800000, 0, 0, 0, 3300000, 3400000, 3500000,
};

static const unsigned int vefuse_voltages[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1700000, 1800000, 1900000, 2000000,
};

static const unsigned int vxo22_voltages[] = {
	1800000, 0, 0, 0, 2200000,
};

static const unsigned int vrfck_voltages[] = {
	0, 0, 1500000, 0, 0, 0, 0, 1600000, 0, 0, 0, 0, 1700000,
};

static const unsigned int vrfck_voltages_1[] = {
	1240000, 1600000,
};

static const unsigned int vio28_voltages[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 2800000, 2900000, 3000000, 3100000, 3300000,
};

static const unsigned int vemc_voltages[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2900000, 3000000, 0, 3300000,
};

static const unsigned int vemc_voltages_1[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 2500000, 2800000, 2900000, 3000000, 3100000,
	3300000,
};

static const unsigned int va12_voltages[] = {
	0, 0, 0, 0, 0, 0, 1200000, 1300000,
};

static const unsigned int va09_voltages[] = {
	0, 0, 800000, 900000, 0, 0, 1200000,
};

static const unsigned int vrf18_voltages[] = {
	0, 0, 0, 0, 0, 1700000, 1800000, 1810000,
};

static const unsigned int vbbck_voltages[] = {
	0, 0, 0, 0, 1100000, 0, 0, 0, 1150000, 0, 0, 0, 1200000,
};

static const unsigned int vsim2_voltages[] = {
	0, 0, 0, 1700000, 1800000, 0, 0, 0, 2700000, 0, 0, 3000000, 3100000,
};

static int regulator_set_voltage_sel_regmap(struct udevice *dev,
					    struct mt6359_regulator_info *info, unsigned int sel)
{
	sel <<= ffs(info->desc.vsel_mask) - 1;

	return pmic_clrsetbits(dev->parent, info->desc.vsel_reg,
			       info->desc.vsel_mask, sel);
}

static int mt6359p_vemc_set_voltage_sel(struct udevice *dev,
					struct mt6359_regulator_info *info, unsigned int sel)
{
	int ret;

	sel <<= ffs(info->desc.vsel_mask) - 1;
	ret = pmic_reg_write(dev->parent, MT6359P_TMA_KEY_ADDR, TMA_KEY);
	if (ret)
		return ret;

	switch (pmic_reg_read(dev->parent, MT6359P_VM_MODE_ADDR)) {
	case 0:
		/* If HW trapping is 0, use VEMC_VOSEL_0 */
		ret = pmic_clrsetbits(dev->parent, info->desc.vsel_reg,
				      info->desc.vsel_mask, sel);
		break;
	case 1:
		/* If HW trapping is 1, use VEMC_VOSEL_1 */
		ret = pmic_clrsetbits(dev->parent, info->desc.vsel_reg + 0x2,
				      info->desc.vsel_mask, sel);
		break;
	default:
		return -EINVAL;
	}

	if (ret)
		return ret;

	return pmic_reg_write(dev->parent, MT6359P_TMA_KEY_ADDR, 0);
}

static int regulator_get_voltage_sel(struct udevice *dev, struct mt6359_regulator_info *info)
{
	int selector;

	selector = pmic_reg_read(dev->parent, info->desc.vsel_reg);
	if (selector < 0)
		return selector;

	selector &= info->desc.vsel_mask;
	selector >>= ffs(info->desc.vsel_mask) - 1;

	return selector;
}

static int mt6359p_vemc_get_voltage_sel(struct udevice *dev, struct mt6359_regulator_info *info)
{
	int selector;

	switch (pmic_reg_read(dev->parent, MT6359P_VM_MODE_ADDR)) {
	case 0:
		/* If HW trapping is 0, use VEMC_VOSEL_0 */
		selector = pmic_reg_read(dev->parent, info->desc.vsel_reg);
		break;
	case 1:
		/* If HW trapping is 1, use VEMC_VOSEL_1 */
		selector = pmic_reg_read(dev->parent, info->desc.vsel_reg + 0x2);
		break;
	default:
		return -EINVAL;
	}
	if (selector < 0)
		return selector;

	selector &= info->desc.vsel_mask;
	selector >>= ffs(info->desc.vsel_mask) - 1;

	return selector;
}

static int mt6359_get_enable(struct udevice *dev)
{
	struct mt6359_regulator_info *info = dev_get_priv(dev);
	int ret;

	ret = pmic_reg_read(dev->parent, info->desc.enable_reg);
	if (ret < 0)
		return ret;

	return ret & info->desc.enable_mask ? true : false;
}

static int mt6359_set_enable(struct udevice *dev, bool enable)
{
	struct mt6359_regulator_info *info = dev_get_priv(dev);

	return pmic_clrsetbits(dev->parent, info->desc.enable_reg,
			       info->desc.enable_mask,
			       enable ? info->desc.enable_mask : 0);
}

static int mt6359_get_value(struct udevice *dev)
{
	struct mt6359_regulator_info *info = dev_get_priv(dev);
	unsigned int val_uV = -EINVAL;
	int selector;

	switch (info->desc.type) {
	case MT6359_REG_TYPE_LINEAR:
		/* Get selection */
		selector = regulator_get_voltage_sel(dev, info);
		if (selector < 0)
			return -EINVAL;

		/* Get voltage value */
		if (selector >= info->desc.n_voltages)
			return -EINVAL;

		val_uV = info->desc.min_uV + (info->desc.uV_step * selector);
		break;
	case MT6359_REG_TYPE_TABLE:
		/* Get selection */
		selector = regulator_get_voltage_sel(dev, info);
		if (selector < 0)
			return -EINVAL;

		/* Get voltage value */
		if (!info->desc.volt_table) {
			dev_err(dev, "invalid voltage table for %s\n", info->desc.name);
			return -EINVAL;
		}

		if (selector >= info->desc.n_voltages)
			return -EINVAL;

		val_uV = info->desc.volt_table[selector];
		break;
	case MT6359_REG_TYPE_FIXED:
		val_uV = info->desc.fixed_uV;
		break;
	case MT6359_REG_TYPE_VEMC:
		/* Get selection */
		selector = mt6359p_vemc_get_voltage_sel(dev, info);
		if (selector < 0)
			return -EINVAL;

		/* Get voltage value */
		if (!info->desc.volt_table) {
			dev_err(dev, "invalid voltage table for %s\n", info->desc.name);
			return -EINVAL;
		}

		if (selector >= info->desc.n_voltages)
			return -EINVAL;

		val_uV = info->desc.volt_table[selector];
		break;
	default:
		val_uV = -EINVAL;
		break;
	}

	return val_uV;
}

static int mt6359_set_value(struct udevice *dev, int uvolt)
{
	struct mt6359_regulator_info *info = dev_get_priv(dev);
	int selector = -EINVAL;
	int ret = -EINVAL;
	int i;

	switch (info->desc.type) {
	case MT6359_REG_TYPE_LINEAR:
		/* Find selection */
		if (uvolt < info->desc.min_uV)
			return -EINVAL;
		selector = DIV_ROUND_UP(uvolt - info->desc.min_uV, info->desc.uV_step);
		if (selector < 0)
			return -EINVAL;

		/* Set selection */
		ret = regulator_set_voltage_sel_regmap(dev, info, selector);
		if (ret < 0)
			return -EINVAL;
		break;
	case MT6359_REG_TYPE_TABLE:
		/* Find selection */
		for (i = 0; i < info->desc.n_voltages; i++) {
			if (info->desc.volt_table[i] == uvolt) {
				selector = i;
				break;
			}
		}
		if (selector < 0)
			return -EINVAL;
		/* Set selection */
		ret = regulator_set_voltage_sel_regmap(dev, info, selector);
		if (ret < 0)
			return -EINVAL;
		break;
	case MT6359_REG_TYPE_VEMC:
		/* Find selection */
		for (i = 0; i < info->desc.n_voltages; i++) {
			if (info->desc.volt_table[i] == uvolt) {
				selector = i;
				break;
			}
		}
		if (selector < 0)
			return -EINVAL;
		/* Set selection */
		ret = mt6359p_vemc_set_voltage_sel(dev, info, selector);
		break;
	default:
		break;
	}

	return ret;
}

/* The array is indexed by id(MT6359_ID_XXX) */
static struct mt6359_regulator_info mt6359_regulators[] = {
	MT6359_BUCK("buck-vs1", VS1, 800000, 2200000, 12500,
		    MT6359_RG_BUCK_VS1_EN_ADDR,
		    MT6359_DA_VS1_EN_ADDR, MT6359_RG_BUCK_VS1_VOSEL_ADDR,
		    MT6359_RG_BUCK_VS1_VOSEL_MASK <<
		    MT6359_RG_BUCK_VS1_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VS1_LP_ADDR, MT6359_RG_BUCK_VS1_LP_SHIFT,
		    MT6359_RG_VS1_FPWM_ADDR, MT6359_RG_VS1_FPWM_SHIFT),
	MT6359_BUCK("buck-vgpu11", VGPU11, 400000, 1193750, 6250,
		    MT6359_RG_BUCK_VGPU11_EN_ADDR,
		    MT6359_DA_VGPU11_EN_ADDR, MT6359_RG_BUCK_VGPU11_VOSEL_ADDR,
		    MT6359_RG_BUCK_VGPU11_VOSEL_MASK <<
		    MT6359_RG_BUCK_VGPU11_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VGPU11_LP_ADDR,
		    MT6359_RG_BUCK_VGPU11_LP_SHIFT,
		    MT6359_RG_VGPU11_FCCM_ADDR, MT6359_RG_VGPU11_FCCM_SHIFT),
	MT6359_BUCK("buck-vmodem", VMODEM, 400000, 1100000, 6250,
		    MT6359_RG_BUCK_VMODEM_EN_ADDR,
		    MT6359_DA_VMODEM_EN_ADDR, MT6359_RG_BUCK_VMODEM_VOSEL_ADDR,
		    MT6359_RG_BUCK_VMODEM_VOSEL_MASK <<
		    MT6359_RG_BUCK_VMODEM_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VMODEM_LP_ADDR,
		    MT6359_RG_BUCK_VMODEM_LP_SHIFT,
		    MT6359_RG_VMODEM_FCCM_ADDR, MT6359_RG_VMODEM_FCCM_SHIFT),
	MT6359_BUCK("buck-vpu", VPU, 400000, 1193750, 6250,
		    MT6359_RG_BUCK_VPU_EN_ADDR,
		    MT6359_DA_VPU_EN_ADDR, MT6359_RG_BUCK_VPU_VOSEL_ADDR,
		    MT6359_RG_BUCK_VPU_VOSEL_MASK <<
		    MT6359_RG_BUCK_VPU_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VPU_LP_ADDR, MT6359_RG_BUCK_VPU_LP_SHIFT,
		    MT6359_RG_VPU_FCCM_ADDR, MT6359_RG_VPU_FCCM_SHIFT),
	MT6359_BUCK("buck-vcore", VCORE, 400000, 1193750, 6250,
		    MT6359_RG_BUCK_VCORE_EN_ADDR,
		    MT6359_DA_VCORE_EN_ADDR, MT6359_RG_BUCK_VCORE_VOSEL_ADDR,
		    MT6359_RG_BUCK_VCORE_VOSEL_MASK <<
		    MT6359_RG_BUCK_VCORE_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VCORE_LP_ADDR, MT6359_RG_BUCK_VCORE_LP_SHIFT,
		    MT6359_RG_VCORE_FCCM_ADDR, MT6359_RG_VCORE_FCCM_SHIFT),
	MT6359_BUCK("buck-vs2", VS2, 800000, 1600000, 12500,
		    MT6359_RG_BUCK_VS2_EN_ADDR,
		    MT6359_DA_VS2_EN_ADDR, MT6359_RG_BUCK_VS2_VOSEL_ADDR,
		    MT6359_RG_BUCK_VS2_VOSEL_MASK <<
		    MT6359_RG_BUCK_VS2_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VS2_LP_ADDR, MT6359_RG_BUCK_VS2_LP_SHIFT,
		    MT6359_RG_VS2_FPWM_ADDR, MT6359_RG_VS2_FPWM_SHIFT),
	MT6359_BUCK("buck-vpa", VPA, 500000, 3650000, 50000,
		    MT6359_RG_BUCK_VPA_EN_ADDR,
		    MT6359_DA_VPA_EN_ADDR, MT6359_RG_BUCK_VPA_VOSEL_ADDR,
		    MT6359_RG_BUCK_VPA_VOSEL_MASK <<
		    MT6359_RG_BUCK_VPA_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VPA_LP_ADDR, MT6359_RG_BUCK_VPA_LP_SHIFT,
		    MT6359_RG_VPA_MODESET_ADDR, MT6359_RG_VPA_MODESET_SHIFT),
	MT6359_BUCK("buck-vproc2", VPROC2, 400000, 1193750, 6250,
		    MT6359_RG_BUCK_VPROC2_EN_ADDR,
		    MT6359_DA_VPROC2_EN_ADDR, MT6359_RG_BUCK_VPROC2_VOSEL_ADDR,
		    MT6359_RG_BUCK_VPROC2_VOSEL_MASK <<
		    MT6359_RG_BUCK_VPROC2_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VPROC2_LP_ADDR,
		    MT6359_RG_BUCK_VPROC2_LP_SHIFT,
		    MT6359_RG_VPROC2_FCCM_ADDR, MT6359_RG_VPROC2_FCCM_SHIFT),
	MT6359_BUCK("buck-vproc1", VPROC1, 400000, 1193750, 6250,
		    MT6359_RG_BUCK_VPROC1_EN_ADDR,
		    MT6359_DA_VPROC1_EN_ADDR, MT6359_RG_BUCK_VPROC1_VOSEL_ADDR,
		    MT6359_RG_BUCK_VPROC1_VOSEL_MASK <<
		    MT6359_RG_BUCK_VPROC1_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VPROC1_LP_ADDR,
		    MT6359_RG_BUCK_VPROC1_LP_SHIFT,
		    MT6359_RG_VPROC1_FCCM_ADDR, MT6359_RG_VPROC1_FCCM_SHIFT),
	MT6359_BUCK("buck-vcore-sshub", VCORE_SSHUB, 400000, 1193750, 6250,
		    MT6359_RG_BUCK_VCORE_SSHUB_EN_ADDR,
		    MT6359_DA_VCORE_EN_ADDR,
		    MT6359_RG_BUCK_VCORE_SSHUB_VOSEL_ADDR,
		    MT6359_RG_BUCK_VCORE_SSHUB_VOSEL_MASK <<
		    MT6359_RG_BUCK_VCORE_SSHUB_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VCORE_LP_ADDR, MT6359_RG_BUCK_VCORE_LP_SHIFT,
		    MT6359_RG_VCORE_FCCM_ADDR, MT6359_RG_VCORE_FCCM_SHIFT),
	MT6359_REG_FIXED("ldo-vaud18", VAUD18, MT6359_RG_LDO_VAUD18_EN_ADDR,
			 MT6359_DA_VAUD18_B_EN_ADDR, 1800000),
	MT6359_LDO("ldo-vsim1", VSIM1, vsim1_voltages,
		   MT6359_RG_LDO_VSIM1_EN_ADDR, MT6359_RG_LDO_VSIM1_EN_SHIFT,
		   MT6359_DA_VSIM1_B_EN_ADDR, MT6359_RG_VSIM1_VOSEL_ADDR,
		   MT6359_RG_VSIM1_VOSEL_MASK << MT6359_RG_VSIM1_VOSEL_SHIFT,
		   480),
	MT6359_LDO("ldo-vibr", VIBR, vibr_voltages,
		   MT6359_RG_LDO_VIBR_EN_ADDR, MT6359_RG_LDO_VIBR_EN_SHIFT,
		   MT6359_DA_VIBR_B_EN_ADDR, MT6359_RG_VIBR_VOSEL_ADDR,
		   MT6359_RG_VIBR_VOSEL_MASK << MT6359_RG_VIBR_VOSEL_SHIFT,
		   240),
	MT6359_LDO("ldo-vrf12", VRF12, vrf12_voltages,
		   MT6359_RG_LDO_VRF12_EN_ADDR, MT6359_RG_LDO_VRF12_EN_SHIFT,
		   MT6359_DA_VRF12_B_EN_ADDR, MT6359_RG_VRF12_VOSEL_ADDR,
		   MT6359_RG_VRF12_VOSEL_MASK << MT6359_RG_VRF12_VOSEL_SHIFT,
		   120),
	MT6359_REG_FIXED("ldo-vusb", VUSB, MT6359_RG_LDO_VUSB_EN_0_ADDR,
			 MT6359_DA_VUSB_B_EN_ADDR, 3000000),
	MT6359_LDO_LINEAR("ldo-vsram-proc2", VSRAM_PROC2, 500000, 1293750, 6250,
			  MT6359_RG_LDO_VSRAM_PROC2_EN_ADDR,
			  MT6359_DA_VSRAM_PROC2_B_EN_ADDR,
			  MT6359_RG_LDO_VSRAM_PROC2_VOSEL_ADDR,
			  MT6359_RG_LDO_VSRAM_PROC2_VOSEL_MASK <<
			  MT6359_RG_LDO_VSRAM_PROC2_VOSEL_SHIFT),
	MT6359_LDO("ldo-vio18", VIO18, volt18_voltages,
		   MT6359_RG_LDO_VIO18_EN_ADDR, MT6359_RG_LDO_VIO18_EN_SHIFT,
		   MT6359_DA_VIO18_B_EN_ADDR, MT6359_RG_VIO18_VOSEL_ADDR,
		   MT6359_RG_VIO18_VOSEL_MASK << MT6359_RG_VIO18_VOSEL_SHIFT,
		   960),
	MT6359_LDO("ldo-vcamio", VCAMIO, volt18_voltages,
		   MT6359_RG_LDO_VCAMIO_EN_ADDR, MT6359_RG_LDO_VCAMIO_EN_SHIFT,
		   MT6359_DA_VCAMIO_B_EN_ADDR, MT6359_RG_VCAMIO_VOSEL_ADDR,
		   MT6359_RG_VCAMIO_VOSEL_MASK << MT6359_RG_VCAMIO_VOSEL_SHIFT,
		   1290),
	MT6359_REG_FIXED("ldo-vcn18", VCN18, MT6359_RG_LDO_VCN18_EN_ADDR,
			 MT6359_DA_VCN18_B_EN_ADDR, 1800000),
	MT6359_REG_FIXED("ldo-vfe28", VFE28, MT6359_RG_LDO_VFE28_EN_ADDR,
			 MT6359_DA_VFE28_B_EN_ADDR, 2800000),
	MT6359_LDO("ldo-vcn13", VCN13, vcn13_voltages,
		   MT6359_RG_LDO_VCN13_EN_ADDR, MT6359_RG_LDO_VCN13_EN_SHIFT,
		   MT6359_DA_VCN13_B_EN_ADDR, MT6359_RG_VCN13_VOSEL_ADDR,
		   MT6359_RG_VCN13_VOSEL_MASK << MT6359_RG_VCN13_VOSEL_SHIFT,
		   240),
	MT6359_LDO("ldo-vcn33-1-bt", VCN33_1_BT, vcn33_voltages,
		   MT6359_RG_LDO_VCN33_1_EN_0_ADDR,
		   MT6359_RG_LDO_VCN33_1_EN_0_SHIFT,
		   MT6359_DA_VCN33_1_B_EN_ADDR, MT6359_RG_VCN33_1_VOSEL_ADDR,
		   MT6359_RG_VCN33_1_VOSEL_MASK <<
		   MT6359_RG_VCN33_1_VOSEL_SHIFT, 240),
	MT6359_LDO("ldo-vcn33-1-wifi", VCN33_1_WIFI, vcn33_voltages,
		   MT6359_RG_LDO_VCN33_1_EN_1_ADDR,
		   MT6359_RG_LDO_VCN33_1_EN_1_SHIFT,
		   MT6359_DA_VCN33_1_B_EN_ADDR, MT6359_RG_VCN33_1_VOSEL_ADDR,
		   MT6359_RG_VCN33_1_VOSEL_MASK <<
		   MT6359_RG_VCN33_1_VOSEL_SHIFT, 240),
	MT6359_REG_FIXED("ldo-vaux18", VAUX18, MT6359_RG_LDO_VAUX18_EN_ADDR,
			 MT6359_DA_VAUX18_B_EN_ADDR, 1800000),
	MT6359_LDO_LINEAR("ldo-vsram-others", VSRAM_OTHERS, 500000, 1293750,
			  6250,
			  MT6359_RG_LDO_VSRAM_OTHERS_EN_ADDR,
			  MT6359_DA_VSRAM_OTHERS_B_EN_ADDR,
			  MT6359_RG_LDO_VSRAM_OTHERS_VOSEL_ADDR,
			  MT6359_RG_LDO_VSRAM_OTHERS_VOSEL_MASK <<
			  MT6359_RG_LDO_VSRAM_OTHERS_VOSEL_SHIFT),
	MT6359_LDO("ldo-vefuse", VEFUSE, vefuse_voltages,
		   MT6359_RG_LDO_VEFUSE_EN_ADDR, MT6359_RG_LDO_VEFUSE_EN_SHIFT,
		   MT6359_DA_VEFUSE_B_EN_ADDR, MT6359_RG_VEFUSE_VOSEL_ADDR,
		   MT6359_RG_VEFUSE_VOSEL_MASK << MT6359_RG_VEFUSE_VOSEL_SHIFT,
		   240),
	MT6359_LDO("ldo-vxo22", VXO22, vxo22_voltages,
		   MT6359_RG_LDO_VXO22_EN_ADDR, MT6359_RG_LDO_VXO22_EN_SHIFT,
		   MT6359_DA_VXO22_B_EN_ADDR, MT6359_RG_VXO22_VOSEL_ADDR,
		   MT6359_RG_VXO22_VOSEL_MASK << MT6359_RG_VXO22_VOSEL_SHIFT,
		   120),
	MT6359_LDO("ldo-vrfck", VRFCK, vrfck_voltages,
		   MT6359_RG_LDO_VRFCK_EN_ADDR, MT6359_RG_LDO_VRFCK_EN_SHIFT,
		   MT6359_DA_VRFCK_B_EN_ADDR, MT6359_RG_VRFCK_VOSEL_ADDR,
		   MT6359_RG_VRFCK_VOSEL_MASK << MT6359_RG_VRFCK_VOSEL_SHIFT,
		   480),
	MT6359_REG_FIXED("ldo-vbif28", VBIF28, MT6359_RG_LDO_VBIF28_EN_ADDR,
			 MT6359_DA_VBIF28_B_EN_ADDR, 2800000),
	MT6359_LDO("ldo-vio28", VIO28, vio28_voltages,
		   MT6359_RG_LDO_VIO28_EN_ADDR, MT6359_RG_LDO_VIO28_EN_SHIFT,
		   MT6359_DA_VIO28_B_EN_ADDR, MT6359_RG_VIO28_VOSEL_ADDR,
		   MT6359_RG_VIO28_VOSEL_MASK << MT6359_RG_VIO28_VOSEL_SHIFT,
		   240),
	MT6359_LDO("ldo-vemc", VEMC, vemc_voltages,
		   MT6359_RG_LDO_VEMC_EN_ADDR, MT6359_RG_LDO_VEMC_EN_SHIFT,
		   MT6359_DA_VEMC_B_EN_ADDR, MT6359_RG_VEMC_VOSEL_ADDR,
		   MT6359_RG_VEMC_VOSEL_MASK << MT6359_RG_VEMC_VOSEL_SHIFT,
		   240),
	MT6359_LDO("ldo-vcn33-2-bt", VCN33_2_BT, vcn33_voltages,
		   MT6359_RG_LDO_VCN33_2_EN_0_ADDR,
		   MT6359_RG_LDO_VCN33_2_EN_0_SHIFT,
		   MT6359_DA_VCN33_2_B_EN_ADDR, MT6359_RG_VCN33_2_VOSEL_ADDR,
		   MT6359_RG_VCN33_2_VOSEL_MASK <<
		   MT6359_RG_VCN33_2_VOSEL_SHIFT, 240),
	MT6359_LDO("ldo-vcn33-2-wifi", VCN33_2_WIFI, vcn33_voltages,
		   MT6359_RG_LDO_VCN33_2_EN_1_ADDR,
		   MT6359_RG_LDO_VCN33_2_EN_1_SHIFT,
		   MT6359_DA_VCN33_2_B_EN_ADDR, MT6359_RG_VCN33_2_VOSEL_ADDR,
		   MT6359_RG_VCN33_2_VOSEL_MASK <<
		   MT6359_RG_VCN33_2_VOSEL_SHIFT, 240),
	MT6359_LDO("ldo-va12", VA12, va12_voltages,
		   MT6359_RG_LDO_VA12_EN_ADDR, MT6359_RG_LDO_VA12_EN_SHIFT,
		   MT6359_DA_VA12_B_EN_ADDR, MT6359_RG_VA12_VOSEL_ADDR,
		   MT6359_RG_VA12_VOSEL_MASK << MT6359_RG_VA12_VOSEL_SHIFT,
		   240),
	MT6359_LDO("ldo-va09", VA09, va09_voltages,
		   MT6359_RG_LDO_VA09_EN_ADDR, MT6359_RG_LDO_VA09_EN_SHIFT,
		   MT6359_DA_VA09_B_EN_ADDR, MT6359_RG_VA09_VOSEL_ADDR,
		   MT6359_RG_VA09_VOSEL_MASK << MT6359_RG_VA09_VOSEL_SHIFT,
		   240),
	MT6359_LDO("ldo-vrf18", VRF18, vrf18_voltages,
		   MT6359_RG_LDO_VRF18_EN_ADDR, MT6359_RG_LDO_VRF18_EN_SHIFT,
		   MT6359_DA_VRF18_B_EN_ADDR, MT6359_RG_VRF18_VOSEL_ADDR,
		   MT6359_RG_VRF18_VOSEL_MASK << MT6359_RG_VRF18_VOSEL_SHIFT,
		   120),
	MT6359_LDO_LINEAR("ldo-vsram-md", VSRAM_MD, 500000, 1100000, 6250,
			  MT6359_RG_LDO_VSRAM_MD_EN_ADDR,
			  MT6359_DA_VSRAM_MD_B_EN_ADDR,
			  MT6359_RG_LDO_VSRAM_MD_VOSEL_ADDR,
			  MT6359_RG_LDO_VSRAM_MD_VOSEL_MASK <<
			  MT6359_RG_LDO_VSRAM_MD_VOSEL_SHIFT),
	MT6359_LDO("ldo-vufs", VUFS, volt18_voltages,
		   MT6359_RG_LDO_VUFS_EN_ADDR, MT6359_RG_LDO_VUFS_EN_SHIFT,
		   MT6359_DA_VUFS_B_EN_ADDR, MT6359_RG_VUFS_VOSEL_ADDR,
		   MT6359_RG_VUFS_VOSEL_MASK << MT6359_RG_VUFS_VOSEL_SHIFT,
		   1920),
	MT6359_LDO("ldo-vm18", VM18, volt18_voltages,
		   MT6359_RG_LDO_VM18_EN_ADDR, MT6359_RG_LDO_VM18_EN_SHIFT,
		   MT6359_DA_VM18_B_EN_ADDR, MT6359_RG_VM18_VOSEL_ADDR,
		   MT6359_RG_VM18_VOSEL_MASK << MT6359_RG_VM18_VOSEL_SHIFT,
		   1920),
	MT6359_LDO("ldo-vbbck", VBBCK, vbbck_voltages,
		   MT6359_RG_LDO_VBBCK_EN_ADDR, MT6359_RG_LDO_VBBCK_EN_SHIFT,
		   MT6359_DA_VBBCK_B_EN_ADDR, MT6359_RG_VBBCK_VOSEL_ADDR,
		   MT6359_RG_VBBCK_VOSEL_MASK << MT6359_RG_VBBCK_VOSEL_SHIFT,
		   240),
	MT6359_LDO_LINEAR("ldo-vsram-proc1", VSRAM_PROC1, 500000, 1293750, 6250,
			  MT6359_RG_LDO_VSRAM_PROC1_EN_ADDR,
			  MT6359_DA_VSRAM_PROC1_B_EN_ADDR,
			  MT6359_RG_LDO_VSRAM_PROC1_VOSEL_ADDR,
			  MT6359_RG_LDO_VSRAM_PROC1_VOSEL_MASK <<
			  MT6359_RG_LDO_VSRAM_PROC1_VOSEL_SHIFT),
	MT6359_LDO("ldo-vsim2", VSIM2, vsim2_voltages,
		   MT6359_RG_LDO_VSIM2_EN_ADDR, MT6359_RG_LDO_VSIM2_EN_SHIFT,
		   MT6359_DA_VSIM2_B_EN_ADDR, MT6359_RG_VSIM2_VOSEL_ADDR,
		   MT6359_RG_VSIM2_VOSEL_MASK << MT6359_RG_VSIM2_VOSEL_SHIFT,
		   480),
	MT6359_LDO_LINEAR("ldo-vsram-others-sshub", VSRAM_OTHERS_SSHUB,
			  500000, 1293750, 6250,
			  MT6359_RG_LDO_VSRAM_OTHERS_SSHUB_EN_ADDR,
			  MT6359_DA_VSRAM_OTHERS_B_EN_ADDR,
			  MT6359_RG_LDO_VSRAM_OTHERS_SSHUB_VOSEL_ADDR,
			  MT6359_RG_LDO_VSRAM_OTHERS_SSHUB_VOSEL_MASK <<
			  MT6359_RG_LDO_VSRAM_OTHERS_SSHUB_VOSEL_SHIFT),
};

static struct mt6359_regulator_info mt6359p_regulators[] = {
	MT6359_BUCK("buck-vs1", VS1, 800000, 2200000, 12500,
		    MT6359_RG_BUCK_VS1_EN_ADDR,
		    MT6359_DA_VS1_EN_ADDR, MT6359_RG_BUCK_VS1_VOSEL_ADDR,
		    MT6359_RG_BUCK_VS1_VOSEL_MASK <<
		    MT6359_RG_BUCK_VS1_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VS1_LP_ADDR, MT6359_RG_BUCK_VS1_LP_SHIFT,
		    MT6359_RG_VS1_FPWM_ADDR, MT6359_RG_VS1_FPWM_SHIFT),
	MT6359_BUCK("buck-vgpu11", VGPU11, 400000, 1193750, 6250,
		    MT6359_RG_BUCK_VGPU11_EN_ADDR,
		    MT6359_DA_VGPU11_EN_ADDR, MT6359P_RG_BUCK_VGPU11_VOSEL_ADDR,
		    MT6359_RG_BUCK_VGPU11_VOSEL_MASK <<
		    MT6359_RG_BUCK_VGPU11_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VGPU11_LP_ADDR,
		    MT6359_RG_BUCK_VGPU11_LP_SHIFT,
		    MT6359_RG_VGPU11_FCCM_ADDR, MT6359_RG_VGPU11_FCCM_SHIFT),
	MT6359_BUCK("buck-vmodem", VMODEM, 400000, 1100000, 6250,
		    MT6359_RG_BUCK_VMODEM_EN_ADDR,
		    MT6359_DA_VMODEM_EN_ADDR, MT6359_RG_BUCK_VMODEM_VOSEL_ADDR,
		    MT6359_RG_BUCK_VMODEM_VOSEL_MASK <<
		    MT6359_RG_BUCK_VMODEM_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VMODEM_LP_ADDR,
		    MT6359_RG_BUCK_VMODEM_LP_SHIFT,
		    MT6359_RG_VMODEM_FCCM_ADDR, MT6359_RG_VMODEM_FCCM_SHIFT),
	MT6359_BUCK("buck-vpu", VPU, 400000, 1193750, 6250,
		    MT6359_RG_BUCK_VPU_EN_ADDR,
		    MT6359_DA_VPU_EN_ADDR, MT6359_RG_BUCK_VPU_VOSEL_ADDR,
		    MT6359_RG_BUCK_VPU_VOSEL_MASK <<
		    MT6359_RG_BUCK_VPU_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VPU_LP_ADDR, MT6359_RG_BUCK_VPU_LP_SHIFT,
		    MT6359_RG_VPU_FCCM_ADDR, MT6359_RG_VPU_FCCM_SHIFT),
	MT6359_BUCK("buck-vcore", VCORE, 506250, 1300000, 6250,
		    MT6359_RG_BUCK_VCORE_EN_ADDR,
		    MT6359_DA_VCORE_EN_ADDR, MT6359P_RG_BUCK_VCORE_VOSEL_ADDR,
		    MT6359_RG_BUCK_VCORE_VOSEL_MASK <<
		    MT6359_RG_BUCK_VCORE_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VCORE_LP_ADDR, MT6359_RG_BUCK_VCORE_LP_SHIFT,
		    MT6359_RG_VCORE_FCCM_ADDR, MT6359_RG_VCORE_FCCM_SHIFT),
	MT6359_BUCK("buck-vs2", VS2, 800000, 1600000, 12500,
		    MT6359_RG_BUCK_VS2_EN_ADDR,
		    MT6359_DA_VS2_EN_ADDR, MT6359_RG_BUCK_VS2_VOSEL_ADDR,
		    MT6359_RG_BUCK_VS2_VOSEL_MASK <<
		    MT6359_RG_BUCK_VS2_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VS2_LP_ADDR, MT6359_RG_BUCK_VS2_LP_SHIFT,
		    MT6359_RG_VS2_FPWM_ADDR, MT6359_RG_VS2_FPWM_SHIFT),
	MT6359_BUCK("buck-vpa", VPA, 500000, 3650000, 50000,
		    MT6359_RG_BUCK_VPA_EN_ADDR,
		    MT6359_DA_VPA_EN_ADDR, MT6359_RG_BUCK_VPA_VOSEL_ADDR,
		    MT6359_RG_BUCK_VPA_VOSEL_MASK <<
		    MT6359_RG_BUCK_VPA_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VPA_LP_ADDR, MT6359_RG_BUCK_VPA_LP_SHIFT,
		    MT6359_RG_VPA_MODESET_ADDR, MT6359_RG_VPA_MODESET_SHIFT),
	MT6359_BUCK("buck-vproc2", VPROC2, 400000, 1193750, 6250,
		    MT6359_RG_BUCK_VPROC2_EN_ADDR,
		    MT6359_DA_VPROC2_EN_ADDR, MT6359_RG_BUCK_VPROC2_VOSEL_ADDR,
		    MT6359_RG_BUCK_VPROC2_VOSEL_MASK <<
		    MT6359_RG_BUCK_VPROC2_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VPROC2_LP_ADDR,
		    MT6359_RG_BUCK_VPROC2_LP_SHIFT,
		    MT6359_RG_VPROC2_FCCM_ADDR, MT6359_RG_VPROC2_FCCM_SHIFT),
	MT6359_BUCK("buck-vproc1", VPROC1, 400000, 1193750, 6250,
		    MT6359_RG_BUCK_VPROC1_EN_ADDR,
		    MT6359_DA_VPROC1_EN_ADDR, MT6359_RG_BUCK_VPROC1_VOSEL_ADDR,
		    MT6359_RG_BUCK_VPROC1_VOSEL_MASK <<
		    MT6359_RG_BUCK_VPROC1_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VPROC1_LP_ADDR,
		    MT6359_RG_BUCK_VPROC1_LP_SHIFT,
		    MT6359_RG_VPROC1_FCCM_ADDR, MT6359_RG_VPROC1_FCCM_SHIFT),
	MT6359_BUCK("buck-vgpu11-sshub", VGPU11_SSHUB, 400000, 1193750, 6250,
		    MT6359P_RG_BUCK_VGPU11_SSHUB_EN_ADDR,
		    MT6359_DA_VGPU11_EN_ADDR,
		    MT6359P_RG_BUCK_VGPU11_SSHUB_VOSEL_ADDR,
		    MT6359P_RG_BUCK_VGPU11_SSHUB_VOSEL_MASK <<
		    MT6359P_RG_BUCK_VGPU11_SSHUB_VOSEL_SHIFT,
		    MT6359_RG_BUCK_VGPU11_LP_ADDR,
		    MT6359_RG_BUCK_VGPU11_LP_SHIFT,
		    MT6359_RG_VGPU11_FCCM_ADDR, MT6359_RG_VGPU11_FCCM_SHIFT),
	MT6359_REG_FIXED("ldo-vaud18", VAUD18, MT6359P_RG_LDO_VAUD18_EN_ADDR,
			 MT6359P_DA_VAUD18_B_EN_ADDR, 1800000),
	MT6359_LDO("ldo-vsim1", VSIM1, vsim1_voltages,
		   MT6359P_RG_LDO_VSIM1_EN_ADDR, MT6359P_RG_LDO_VSIM1_EN_SHIFT,
		   MT6359P_DA_VSIM1_B_EN_ADDR, MT6359P_RG_VSIM1_VOSEL_ADDR,
		   MT6359_RG_VSIM1_VOSEL_MASK << MT6359_RG_VSIM1_VOSEL_SHIFT,
		   480),
	MT6359_LDO("ldo-vibr", VIBR, vibr_voltages,
		   MT6359P_RG_LDO_VIBR_EN_ADDR, MT6359P_RG_LDO_VIBR_EN_SHIFT,
		   MT6359P_DA_VIBR_B_EN_ADDR, MT6359P_RG_VIBR_VOSEL_ADDR,
		   MT6359_RG_VIBR_VOSEL_MASK << MT6359_RG_VIBR_VOSEL_SHIFT,
		   240),
	MT6359_LDO("ldo-vrf12", VRF12, vrf12_voltages,
		   MT6359P_RG_LDO_VRF12_EN_ADDR, MT6359P_RG_LDO_VRF12_EN_SHIFT,
		   MT6359P_DA_VRF12_B_EN_ADDR, MT6359P_RG_VRF12_VOSEL_ADDR,
		   MT6359_RG_VRF12_VOSEL_MASK << MT6359_RG_VRF12_VOSEL_SHIFT,
		   480),
	MT6359_REG_FIXED("ldo-vusb", VUSB, MT6359P_RG_LDO_VUSB_EN_0_ADDR,
			 MT6359P_DA_VUSB_B_EN_ADDR, 3000000),
	MT6359_LDO_LINEAR("ldo-vsram-proc2", VSRAM_PROC2, 500000, 1293750, 6250,
			  MT6359P_RG_LDO_VSRAM_PROC2_EN_ADDR,
			  MT6359P_DA_VSRAM_PROC2_B_EN_ADDR,
			  MT6359P_RG_LDO_VSRAM_PROC2_VOSEL_ADDR,
			  MT6359_RG_LDO_VSRAM_PROC2_VOSEL_MASK <<
			  MT6359_RG_LDO_VSRAM_PROC2_VOSEL_SHIFT),
	MT6359_LDO("ldo-vio18", VIO18, volt18_voltages,
		   MT6359P_RG_LDO_VIO18_EN_ADDR, MT6359P_RG_LDO_VIO18_EN_SHIFT,
		   MT6359P_DA_VIO18_B_EN_ADDR, MT6359P_RG_VIO18_VOSEL_ADDR,
		   MT6359_RG_VIO18_VOSEL_MASK << MT6359_RG_VIO18_VOSEL_SHIFT,
		   960),
	MT6359_LDO("ldo-vcamio", VCAMIO, volt18_voltages,
		   MT6359P_RG_LDO_VCAMIO_EN_ADDR,
		   MT6359P_RG_LDO_VCAMIO_EN_SHIFT,
		   MT6359P_DA_VCAMIO_B_EN_ADDR, MT6359P_RG_VCAMIO_VOSEL_ADDR,
		   MT6359_RG_VCAMIO_VOSEL_MASK << MT6359_RG_VCAMIO_VOSEL_SHIFT,
		   1290),
	MT6359_REG_FIXED("ldo-vcn18", VCN18, MT6359P_RG_LDO_VCN18_EN_ADDR,
			 MT6359P_DA_VCN18_B_EN_ADDR, 1800000),
	MT6359_REG_FIXED("ldo-vfe28", VFE28, MT6359P_RG_LDO_VFE28_EN_ADDR,
			 MT6359P_DA_VFE28_B_EN_ADDR, 2800000),
	MT6359_LDO("ldo-vcn13", VCN13, vcn13_voltages,
		   MT6359P_RG_LDO_VCN13_EN_ADDR, MT6359P_RG_LDO_VCN13_EN_SHIFT,
		   MT6359P_DA_VCN13_B_EN_ADDR, MT6359P_RG_VCN13_VOSEL_ADDR,
		   MT6359_RG_VCN13_VOSEL_MASK << MT6359_RG_VCN13_VOSEL_SHIFT,
		   240),
	MT6359_LDO("ldo-vcn33-1-bt", VCN33_1_BT, vcn33_voltages,
		   MT6359P_RG_LDO_VCN33_1_EN_0_ADDR,
		   MT6359_RG_LDO_VCN33_1_EN_0_SHIFT,
		   MT6359P_DA_VCN33_1_B_EN_ADDR, MT6359P_RG_VCN33_1_VOSEL_ADDR,
		   MT6359_RG_VCN33_1_VOSEL_MASK <<
		   MT6359_RG_VCN33_1_VOSEL_SHIFT, 240),
	MT6359_LDO("ldo-vcn33-1-wifi", VCN33_1_WIFI, vcn33_voltages,
		   MT6359P_RG_LDO_VCN33_1_EN_1_ADDR,
		   MT6359P_RG_LDO_VCN33_1_EN_1_SHIFT,
		   MT6359P_DA_VCN33_1_B_EN_ADDR, MT6359P_RG_VCN33_1_VOSEL_ADDR,
		   MT6359_RG_VCN33_1_VOSEL_MASK <<
		   MT6359_RG_VCN33_1_VOSEL_SHIFT, 240),
	MT6359_REG_FIXED("ldo-vaux18", VAUX18, MT6359P_RG_LDO_VAUX18_EN_ADDR,
			 MT6359P_DA_VAUX18_B_EN_ADDR, 1800000),
	MT6359_LDO_LINEAR("ldo-vsram-others", VSRAM_OTHERS, 500000, 1293750,
			  6250,
			  MT6359P_RG_LDO_VSRAM_OTHERS_EN_ADDR,
			  MT6359P_DA_VSRAM_OTHERS_B_EN_ADDR,
			  MT6359P_RG_LDO_VSRAM_OTHERS_VOSEL_ADDR,
			  MT6359_RG_LDO_VSRAM_OTHERS_VOSEL_MASK <<
			  MT6359_RG_LDO_VSRAM_OTHERS_VOSEL_SHIFT),
	MT6359_LDO("ldo-vefuse", VEFUSE, vefuse_voltages,
		   MT6359P_RG_LDO_VEFUSE_EN_ADDR,
		   MT6359P_RG_LDO_VEFUSE_EN_SHIFT,
		   MT6359P_DA_VEFUSE_B_EN_ADDR, MT6359P_RG_VEFUSE_VOSEL_ADDR,
		   MT6359_RG_VEFUSE_VOSEL_MASK << MT6359_RG_VEFUSE_VOSEL_SHIFT,
		   240),
	MT6359_LDO("ldo-vxo22", VXO22, vxo22_voltages,
		   MT6359P_RG_LDO_VXO22_EN_ADDR, MT6359P_RG_LDO_VXO22_EN_SHIFT,
		   MT6359P_DA_VXO22_B_EN_ADDR, MT6359P_RG_VXO22_VOSEL_ADDR,
		   MT6359_RG_VXO22_VOSEL_MASK << MT6359_RG_VXO22_VOSEL_SHIFT,
		   480),
	MT6359_LDO("ldo-vrfck-1", VRFCK, vrfck_voltages_1,
		   MT6359P_RG_LDO_VRFCK_EN_ADDR, MT6359P_RG_LDO_VRFCK_EN_SHIFT,
		   MT6359P_DA_VRFCK_B_EN_ADDR, MT6359P_RG_VRFCK_VOSEL_ADDR,
		   MT6359_RG_VRFCK_VOSEL_MASK << MT6359_RG_VRFCK_VOSEL_SHIFT,
		   480),
	MT6359_REG_FIXED("ldo-vbif28", VBIF28, MT6359P_RG_LDO_VBIF28_EN_ADDR,
			 MT6359P_DA_VBIF28_B_EN_ADDR, 2800000),
	MT6359_LDO("ldo-vio28", VIO28, vio28_voltages,
		   MT6359P_RG_LDO_VIO28_EN_ADDR, MT6359P_RG_LDO_VIO28_EN_SHIFT,
		   MT6359P_DA_VIO28_B_EN_ADDR, MT6359P_RG_VIO28_VOSEL_ADDR,
		   MT6359_RG_VIO28_VOSEL_MASK << MT6359_RG_VIO28_VOSEL_SHIFT,
		   1920),
	MT6359P_LDO1("ldo-vemc-1", VEMC, MT6359_REG_TYPE_VEMC, vemc_voltages_1,
		     MT6359P_RG_LDO_VEMC_EN_ADDR, MT6359P_RG_LDO_VEMC_EN_SHIFT,
		     MT6359P_DA_VEMC_B_EN_ADDR,
		     MT6359P_RG_LDO_VEMC_VOSEL_0_ADDR,
		     MT6359P_RG_LDO_VEMC_VOSEL_0_MASK <<
		     MT6359P_RG_LDO_VEMC_VOSEL_0_SHIFT),
	MT6359_LDO("ldo-vcn33-2-bt", VCN33_2_BT, vcn33_voltages,
		   MT6359P_RG_LDO_VCN33_2_EN_0_ADDR,
		   MT6359P_RG_LDO_VCN33_2_EN_0_SHIFT,
		   MT6359P_DA_VCN33_2_B_EN_ADDR, MT6359P_RG_VCN33_2_VOSEL_ADDR,
		   MT6359_RG_VCN33_2_VOSEL_MASK <<
		   MT6359_RG_VCN33_2_VOSEL_SHIFT, 240),
	MT6359_LDO("ldo-vcn33-2-wifi", VCN33_2_WIFI, vcn33_voltages,
		   MT6359P_RG_LDO_VCN33_2_EN_1_ADDR,
		   MT6359_RG_LDO_VCN33_2_EN_1_SHIFT,
		   MT6359P_DA_VCN33_2_B_EN_ADDR, MT6359P_RG_VCN33_2_VOSEL_ADDR,
		   MT6359_RG_VCN33_2_VOSEL_MASK <<
		   MT6359_RG_VCN33_2_VOSEL_SHIFT, 240),
	MT6359_LDO("ldo-va12", VA12, va12_voltages,
		   MT6359P_RG_LDO_VA12_EN_ADDR, MT6359P_RG_LDO_VA12_EN_SHIFT,
		   MT6359P_DA_VA12_B_EN_ADDR, MT6359P_RG_VA12_VOSEL_ADDR,
		   MT6359_RG_VA12_VOSEL_MASK << MT6359_RG_VA12_VOSEL_SHIFT,
		   960),
	MT6359_LDO("ldo-va09", VA09, va09_voltages,
		   MT6359P_RG_LDO_VA09_EN_ADDR, MT6359P_RG_LDO_VA09_EN_SHIFT,
		   MT6359P_DA_VA09_B_EN_ADDR, MT6359P_RG_VA09_VOSEL_ADDR,
		   MT6359_RG_VA09_VOSEL_MASK << MT6359_RG_VA09_VOSEL_SHIFT,
		   960),
	MT6359_LDO("ldo-vrf18", VRF18, vrf18_voltages,
		   MT6359P_RG_LDO_VRF18_EN_ADDR, MT6359P_RG_LDO_VRF18_EN_SHIFT,
		   MT6359P_DA_VRF18_B_EN_ADDR, MT6359P_RG_VRF18_VOSEL_ADDR,
		   MT6359_RG_VRF18_VOSEL_MASK << MT6359_RG_VRF18_VOSEL_SHIFT,
		   240),
	MT6359_LDO_LINEAR("ldo-vsram-md", VSRAM_MD, 500000, 1293750, 6250,
			  MT6359P_RG_LDO_VSRAM_MD_EN_ADDR,
			  MT6359P_DA_VSRAM_MD_B_EN_ADDR,
			  MT6359P_RG_LDO_VSRAM_MD_VOSEL_ADDR,
			  MT6359_RG_LDO_VSRAM_MD_VOSEL_MASK <<
			  MT6359_RG_LDO_VSRAM_MD_VOSEL_SHIFT),
	MT6359_LDO("ldo-vufs", VUFS, volt18_voltages,
		   MT6359P_RG_LDO_VUFS_EN_ADDR, MT6359P_RG_LDO_VUFS_EN_SHIFT,
		   MT6359P_DA_VUFS_B_EN_ADDR, MT6359P_RG_VUFS_VOSEL_ADDR,
		   MT6359_RG_VUFS_VOSEL_MASK << MT6359_RG_VUFS_VOSEL_SHIFT,
		   1920),
	MT6359_LDO("ldo-vm18", VM18, volt18_voltages,
		   MT6359P_RG_LDO_VM18_EN_ADDR, MT6359P_RG_LDO_VM18_EN_SHIFT,
		   MT6359P_DA_VM18_B_EN_ADDR, MT6359P_RG_VM18_VOSEL_ADDR,
		   MT6359_RG_VM18_VOSEL_MASK << MT6359_RG_VM18_VOSEL_SHIFT,
		   1920),
	MT6359_LDO("ldo-vbbck", VBBCK, vbbck_voltages,
		   MT6359P_RG_LDO_VBBCK_EN_ADDR, MT6359P_RG_LDO_VBBCK_EN_SHIFT,
		   MT6359P_DA_VBBCK_B_EN_ADDR, MT6359P_RG_VBBCK_VOSEL_ADDR,
		   MT6359P_RG_VBBCK_VOSEL_MASK << MT6359P_RG_VBBCK_VOSEL_SHIFT,
		   480),
	MT6359_LDO_LINEAR("ldo-vsram-proc1", VSRAM_PROC1, 500000, 1293750, 6250,
			  MT6359P_RG_LDO_VSRAM_PROC1_EN_ADDR,
			  MT6359P_DA_VSRAM_PROC1_B_EN_ADDR,
			  MT6359P_RG_LDO_VSRAM_PROC1_VOSEL_ADDR,
			  MT6359_RG_LDO_VSRAM_PROC1_VOSEL_MASK <<
			  MT6359_RG_LDO_VSRAM_PROC1_VOSEL_SHIFT),
	MT6359_LDO("ldo-vsim2", VSIM2, vsim2_voltages,
		   MT6359P_RG_LDO_VSIM2_EN_ADDR, MT6359P_RG_LDO_VSIM2_EN_SHIFT,
		   MT6359P_DA_VSIM2_B_EN_ADDR, MT6359P_RG_VSIM2_VOSEL_ADDR,
		   MT6359_RG_VSIM2_VOSEL_MASK << MT6359_RG_VSIM2_VOSEL_SHIFT,
		   480),
	MT6359_LDO_LINEAR("ldo-vsram-others-sshub", VSRAM_OTHERS_SSHUB,
			  500000, 1293750, 6250,
			  MT6359P_RG_LDO_VSRAM_OTHERS_SSHUB_EN_ADDR,
			  MT6359P_DA_VSRAM_OTHERS_B_EN_ADDR,
			  MT6359P_RG_LDO_VSRAM_OTHERS_SSHUB_VOSEL_ADDR,
			  MT6359_RG_LDO_VSRAM_OTHERS_SSHUB_VOSEL_MASK <<
			  MT6359_RG_LDO_VSRAM_OTHERS_SSHUB_VOSEL_SHIFT),
};

static int mt6359_regulator_probe(struct udevice *dev)
{
	struct mt6359_regulator_info *priv = dev_get_priv(dev);
	int i, hw_ver;

	hw_ver = pmic_reg_read(dev->parent, MT6359P_HWCID);
	if (hw_ver < MT6359P_CHIP_VER) {
		dev_err(dev, "mt6359 is not supported. Only support mt6359p, hw_ver(%d)\n",
			hw_ver);
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(mt6359_regulators); i++) {
		if (!strcmp(dev->name, mt6359_regulators[i].desc.of_match)) {
			*priv = mt6359_regulators[i];
			return 0;
		}
	}

	return -ENOENT;
}

static const struct dm_regulator_ops mt6359_regulator_ops = {
	.get_value  = mt6359_get_value,
	.set_value  = mt6359_set_value,
	.get_enable = mt6359_get_enable,
	.set_enable = mt6359_set_enable,
};

U_BOOT_DRIVER(mt6359_regulator) = {
	.name	   = MT6359_REGULATOR_DRIVER,
	.id	       = UCLASS_REGULATOR,
	.ops	   = &mt6359_regulator_ops,
	.probe	   = mt6359_regulator_probe,
	.priv_auto = sizeof(struct mt6359_regulator_info),
};
