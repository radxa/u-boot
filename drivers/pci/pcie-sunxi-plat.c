// SPDX-License-Identifier: GPL-2.0
/*
 * Allwinner PCI Express plat driver
 *
 * Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved.
 *
 * pcie-sunxi-plat.c:	chenhuaqiang <chenhuaqiang@allwinnertech.com>
 */

#include <common.h>
#include "pci.h"
#include "pcie-sunxi.h"
#include <linux/types.h>
#include <asm/arch/clock.h>
#include <sunxi_power/axp.h>
#include <dm.h>
#include <sys_config.h>

extern struct sunxi_pcie_port pcie_port;
struct sunxi_pcie pci;

/* Indexed by PCI_EXP_LNKCAP_SLS, PCI_EXP_LNKSTA_CLS */
const unsigned char pcie_link_speed[] = {
	PCI_SPEED_UNKNOWN,		/* 0 */
	PCIE_SPEED_2_5GT,		/* 1 */
	PCIE_SPEED_5_0GT,		/* 2 */
	PCIE_SPEED_8_0GT,		/* 3 */
	PCIE_SPEED_16_0GT,		/* 4 */
	PCIE_SPEED_32_0GT,		/* 5 */
};

int sunxi_pcie_cfg_write(void __iomem *addr, int size, ulong val)
{
	if ((uintptr_t)addr & (size - 1))
		return PCIBIOS_BAD_REGISTER_NUMBER;

	if (size == 4)
		writel(val, addr);
	else if (size == 2)
		writew(val, addr);
	else if (size == 1)
		writeb(val, addr);
	else
		return PCIBIOS_BAD_REGISTER_NUMBER;

	return PCIBIOS_SUCCESSFUL;
}

int sunxi_pcie_cfg_read(void __iomem *addr, int size, ulong *val)
{
	if ((uintptr_t)addr & (size - 1)) {
		*val = 0;
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}

	if (size == 4) {
		*val = readl(addr);
	} else if (size == 2) {
		*val = readw(addr);
	} else if (size == 1) {
		*val = readb(addr);
	} else {
		*val = 0;
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}

	return PCIBIOS_SUCCESSFUL;
}

void sunxi_pcie_writel(u32 val, struct sunxi_pcie *pcie, u32 offset)
{
	writel(val, pcie->app_base + offset);
}

u32 sunxi_pcie_readl(struct sunxi_pcie *pcie, u32 offset)
{
	return readl(pcie->app_base + offset);
}

static void sunxi_pcie_write_dbi(struct sunxi_pcie *pci, u32 reg, size_t size, u32 val)
{
	int ret;

	ret = sunxi_pcie_cfg_write(pci->dbi_base + reg, size, val);
	if (ret)
		printf("Write DBI address failed\n");
}

static ulong sunxi_pcie_read_dbi(struct sunxi_pcie *pci, u32 reg, size_t size)
{
	int ret;
	ulong val;

	ret = sunxi_pcie_cfg_read(pci->dbi_base + reg, size, &val);
	if (ret)
		printf("Read DBI address failed\n");

	return val;
}

void sunxi_pcie_writel_dbi(struct sunxi_pcie *pci, u32 reg, u32 val)
{
	sunxi_pcie_write_dbi(pci, reg, 0x4, val);
}

u32 sunxi_pcie_readl_dbi(struct sunxi_pcie *pci, u32 reg)
{
	return sunxi_pcie_read_dbi(pci, reg, 0x4);
}

void sunxi_pcie_writew_dbi(struct sunxi_pcie *pci, u32 reg, u16 val)
{
	sunxi_pcie_write_dbi(pci, reg, 0x2, val);
}

u16 sunxi_pcie_readw_dbi(struct sunxi_pcie *pci, u32 reg)
{
	return sunxi_pcie_read_dbi(pci, reg, 0x2);
}

void sunxi_pcie_writeb_dbi(struct sunxi_pcie *pci, u32 reg, u8 val)
{
	sunxi_pcie_write_dbi(pci, reg, 0x1, val);
}

u8 sunxi_pcie_readb_dbi(struct sunxi_pcie *pci, u32 reg)
{
	return sunxi_pcie_read_dbi(pci, reg, 0x1);
}

void sunxi_pcie_dbi_ro_wr_en(struct sunxi_pcie *pci)
{
	u32 val;

	val = sunxi_pcie_readl_dbi(pci, PCIE_MISC_CONTROL_1_CFG);
	val |= (0x1 << 0);
	sunxi_pcie_writel_dbi(pci, PCIE_MISC_CONTROL_1_CFG, val);
}

void sunxi_pcie_dbi_ro_wr_dis(struct sunxi_pcie *pci)
{
	u32 val;

	val = sunxi_pcie_readl_dbi(pci, PCIE_MISC_CONTROL_1_CFG);
	val &= ~(0x1 << 0);
	sunxi_pcie_writel_dbi(pci, PCIE_MISC_CONTROL_1_CFG, val);
}

void sunxi_pcie_plat_ltssm_enable(struct sunxi_pcie *pcie)
{
	u32 val;

	val = sunxi_pcie_readl(pcie, PCIE_LTSSM_CTRL);
	val |= PCIE_LINK_TRAINING;
	sunxi_pcie_writel(val, pcie, PCIE_LTSSM_CTRL);
}

void sunxi_pcie_plat_ltssm_disable(struct sunxi_pcie *pcie)
{
	u32 val;

	val = sunxi_pcie_readl(pcie, PCIE_LTSSM_CTRL);
	val &= ~PCIE_LINK_TRAINING;
	sunxi_pcie_writel(val, pcie, PCIE_LTSSM_CTRL);
}

static u8 __sunxi_pcie_find_next_cap(struct sunxi_pcie *pci, u8 cap_ptr,
						u8 cap)
{
	u8 cap_id, next_cap_ptr;
	u16 reg;

	if (!cap_ptr)
		return 0;

	reg = sunxi_pcie_readw_dbi(pci, cap_ptr);
	cap_id = (reg & CAP_ID_MASK);

	if (cap_id > PCI_CAP_ID_MAX)
		return 0;

	if (cap_id == cap)
		return cap_ptr;

	next_cap_ptr = (reg & NEXT_CAP_PTR_MASK) >> 8;
	return __sunxi_pcie_find_next_cap(pci, next_cap_ptr, cap);
}

static u8 sunxi_pcie_plat_find_capability(struct sunxi_pcie *pci, u8 cap)
{
	u8 next_cap_ptr;
	u16 reg;

	reg = sunxi_pcie_readw_dbi(pci, PCI_CAPABILITY_LIST);
	next_cap_ptr = (reg & CAP_ID_MASK);

	return __sunxi_pcie_find_next_cap(pci, next_cap_ptr, cap);
}

static void sunxi_pcie_plat_set_link_cap(struct sunxi_pcie *pci, u32 link_gen)
{
	u32 cap, ctrl2, link_speed = 0;

	u8 offset = sunxi_pcie_plat_find_capability(pci, PCI_CAP_ID_EXP);

	cap = sunxi_pcie_readl_dbi(pci, offset + PCI_EXP_LNKCAP);
	ctrl2 = sunxi_pcie_readl_dbi(pci, offset + PCI_EXP_LNKCTL2);
	ctrl2 &= ~PCI_EXP_LNKCTL2_TLS;

	switch (pcie_link_speed[link_gen]) {
	case PCIE_SPEED_2_5GT:
		link_speed = PCI_EXP_LNKCTL2_TLS_2_5GT;
		break;
	case PCIE_SPEED_5_0GT:
		link_speed = PCI_EXP_LNKCTL2_TLS_5_0GT;
		break;
	case PCIE_SPEED_8_0GT:
		link_speed = PCI_EXP_LNKCTL2_TLS_8_0GT;
		break;
	case PCIE_SPEED_16_0GT:
		link_speed = PCI_EXP_LNKCTL2_TLS_16_0GT;
		break;
	default:
		/* Use hardware capability */
		// link_speed = FIELD_GET(PCI_EXP_LNKCAP_SLS, cap);
		// ctrl2 &= ~PCI_EXP_LNKCTL2_HASD;
		break;
	}

	sunxi_pcie_writel_dbi(pci, offset + PCI_EXP_LNKCTL2, ctrl2 | link_speed);

	cap &= ~((u32)PCI_EXP_LNKCAP_SLS);
	sunxi_pcie_writel_dbi(pci, offset + PCI_EXP_LNKCAP, cap | link_speed);
}

void sunxi_pcie_plat_set_rate(struct sunxi_pcie *pci)
{
	u32 val;

	sunxi_pcie_plat_set_link_cap(pci, pci->link_gen);
	/* set the number of lanes */
	val = sunxi_pcie_readl_dbi(pci, PCIE_PORT_LINK_CONTROL);
	val &= ~PORT_LINK_MODE_MASK;
	switch (pci->lanes) {
	case 1:
		val |= PORT_LINK_MODE_1_LANES;
		break;
	case 2:
		val |= PORT_LINK_MODE_2_LANES;
		break;
	case 4:
		val |= PORT_LINK_MODE_4_LANES;
		break;
	default:
		printf("num-lanes %u: invalid value\n", pci->lanes);
		return;
	}
	sunxi_pcie_writel_dbi(pci, PCIE_PORT_LINK_CONTROL, val);

	/* set link width speed control register */
	val = sunxi_pcie_readl_dbi(pci, PCIE_LINK_WIDTH_SPEED_CONTROL);
	val &= ~PORT_LOGIC_LINK_WIDTH_MASK;
	switch (pci->lanes) {
	case 1:
		val |= PORT_LOGIC_LINK_WIDTH_1_LANES;
		break;
	case 2:
		val |= PORT_LOGIC_LINK_WIDTH_2_LANES;
		break;
	case 4:
		val |= PORT_LOGIC_LINK_WIDTH_4_LANES;
		break;
	}
	sunxi_pcie_writel_dbi(pci, PCIE_LINK_WIDTH_SPEED_CONTROL, val);
}

static int sunxi_pcie_plat_power_on(struct sunxi_pcie *pci)
{
	if (pci->pcie1v8_supply)
		pmu_set_voltage((char *)pci->pcie1v8_supply, 1800, true); // 1.8V phy
	if (pci->pcie3v3_supply)
		pmu_set_voltage((char *)pci->pcie3v3_supply, 3300, true); // 3.3V slot

	return 0;
}

static void sunxi_pcie_plat_power_off(struct sunxi_pcie *pci)
{
	if (pci->pcie3v3_supply)
		pmu_set_voltage((char *)pci->pcie3v3_supply, 3300, false); // 3.3V slot
	if (pci->pcie1v8_supply)
		pmu_set_voltage((char *)pci->pcie1v8_supply, 1800, false); // 1.8V phy
}

static int sunxi_pcie_plat_clk_setup(struct sunxi_pcie *pci)
{
	unsigned long reg_value = 0;
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	if (pci->drvdata->need_pcie_rst) {
		reg_value = readl(&ccm->pcie_bgr_reg);
		reg_value |= BIT(PCIE_BRG_REG_RST) | BIT(PCIE_BRG_REG_PWRUP_RST);
		writel(reg_value, &ccm->pcie_bgr_reg);
	}

	reg_value = readl(&ccm->pcie_aux_clk_reg);
	reg_value |= BIT(PCIE_AUX_CLK_GATING_BIT);
	writel(reg_value, &ccm->pcie_aux_clk_reg);

#ifndef CONFIG_MACH_SUN55IW3
	if (pci->drvdata->has_pcie_slv_clk) {
		reg_value = readl(&ccm->pcie_axi_slv_clk_reg);
		if (pci->drvdata->pcie_slv_clk_400m)
			reg_value |= (2 << 24);
		reg_value |= BIT(PCIE_AXI_SLV_GATING_BIT);
		writel(reg_value, &ccm->pcie_axi_slv_clk_reg);
	}

	if (pci->drvdata->has_pcie_its_clk) {
		reg_value = readl(&ccm->its_bgr_reg);
		reg_value |= BIT(ITS_PCIE_RST) | BIT(ITS_PCIE_CLK_GATING_BIT);
		writel(reg_value, &ccm->its_bgr_reg);
	}
#endif

	return 0;
}

static void sunxi_pcie_plat_clk_exit(struct sunxi_pcie *pci)
{
	unsigned long reg_value = 0;
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

#ifndef CONFIG_MACH_SUN55IW3
	if (pci->drvdata->has_pcie_its_clk) {
		reg_value = readl(&ccm->its_bgr_reg);
		reg_value &= ~(BIT(ITS_PCIE_RST) | BIT(ITS_PCIE_CLK_GATING_BIT));
		writel(reg_value, &ccm->its_bgr_reg);
	}

	if (pci->drvdata->has_pcie_slv_clk) {
		reg_value = readl(&ccm->pcie_axi_slv_clk_reg);
		reg_value &= ~BIT(PCIE_AXI_SLV_GATING_BIT);
		writel(reg_value, &ccm->pcie_axi_slv_clk_reg);
	}
#endif

	reg_value = readl(&ccm->pcie_aux_clk_reg);
	reg_value &= ~BIT(PCIE_AUX_CLK_GATING_BIT);
	writel(reg_value, &ccm->pcie_aux_clk_reg);

	if (pci->drvdata->need_pcie_rst) {
		reg_value = readl(&ccm->pcie_bgr_reg);
		reg_value &= ~(BIT(PCIE_BRG_REG_RST) | BIT(PCIE_BRG_REG_PWRUP_RST));
		writel(reg_value, &ccm->pcie_bgr_reg);
	}
}

static int sunxi_pcie_plat_combo_phy_init(struct sunxi_pcie *pci)
{
	int ret;

	ret = generic_phy_init(pci->phy);
	if (ret) {
		printf("fail to init phy, err %d\n", ret);
		return ret;
	}

	return 0;
}

#if 0
static void sunxi_pcie_plat_combo_phy_deinit(struct sunxi_pcie *pci)
{
	generic_phy_exit(pci->phy);
}
#endif

int sunxi_pcie_plat_hw_init(struct udevice *dev)
{
	struct sunxi_pcie *pci = dev_get_priv(dev);
	int ret;

	ret = sunxi_pcie_plat_power_on(pci);
	if (ret)
		return ret;

	ret = sunxi_pcie_plat_clk_setup(pci);
	if (ret)
		goto err0;

	ret = sunxi_pcie_plat_combo_phy_init(pci);
	if (ret)
		goto err1;

	return 0;

err1:
	sunxi_pcie_plat_clk_exit(pci);
err0:
	sunxi_pcie_plat_power_off(pci);

	return ret;
}

void sunxi_pcie_plat_init(struct udevice *dev)
{
	struct sunxi_pcie *pci = dev_get_priv(dev);
	pci->dbi_base = (void *)SUNXI_PCIE_DBI_ADDR;

	pci->app_base = pci->dbi_base + PCIE_USER_DEFINED_REGISTER;

	pci->lanes = 1;

	pci->pcie_port.dbi_base = pci->dbi_base;
	pci->pcie_port.cfg0_base = SUNXI_PCIE_CFG_ADDR;
	pci->pcie_port.cfg0_size = SUNXI_PCIE_CFG_SIZE;
	pci->pcie_port.io_base = SUNXI_PCIE_IO_ADDR;
	pci->pcie_port.io_size = SUNXI_PCIE_IO_SIZE;
	pci->pcie_port.mem_base = SUNXI_PCIE_MEM_ADDR;
	pci->pcie_port.mem_size = SUNXI_PCIE_MEM_SIZE;
	pci->pcie_port.va_cfg0_base = (void *)pci->pcie_port.cfg0_base;
	pci->pcie_port.cpu_pcie_addr_quirk = pci->drvdata->cpu_pcie_addr_quirk;
	if (pci->pcie_port.cpu_pcie_addr_quirk) {
		pci->pcie_port.cfg0_base -= PCIE_CPU_BASE;
		pci->pcie_port.io_base   -= PCIE_CPU_BASE;
	}
}
