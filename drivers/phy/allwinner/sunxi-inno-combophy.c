// SPDX-License-Identifier: GPL-2.0
/*
 * Allwinner PIPE USB3.0 PCIE Combo Phy driver
 *
 * Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved.
 *
 * sunxi-inno-combophy.c:	chenhuaqiang <chenhuaqiang@allwinnertech.com>
 */

#include <dm.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <iotrace.h>
#include <dm/device.h>
#include <dm/ofnode.h>
#include <dm/of_access.h>
#include <generic-phy.h>
#include <dt-bindings/phy/phy.h>
#include <clk/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/bitops.h>
#include <asm/arch/clock.h>

#define SUNXI_COMBOPHY_CTL_BASE	0x04f00000
#define SUNXI_COMBOPHY_CLK_BASE	0x04f80000

/* PCIE USB3 Sub-System Registers */
/* Sub-System Version Reset Register */
#define PCIE_USB3_SYS_VER		0x00

/* Sub-System PCIE Bus Gating Reset Register */
#define PCIE_COMBO_PHY_BGR		0x04
#define   PCIE_SLV_ACLK_EN		BIT(18)
#define   PCIE_ACLK_EN			BIT(17)
#define   PCIE_HCLK_EN			BIT(16)
#define   PCIE_PERSTN			BIT(1)
#define   PCIE_PW_UP_RSTN		BIT(0)

/* Sub-System USB3 Bus Gating Reset Register */
#define USB3_COMBO_PHY_BGR		0x08
#define   USB3_ACLK_EN			BIT(17)
#define   USB3_HCLK_EN			BIT(16)
#define   USB3_U2_PHY_RSTN		BIT(4)
#define   USB3_U2_PHY_MUX_EN		BIT(3)
#define   USB3_U2_PHY_MUX_SEL		BIT(0)
#define   USB3_RESETN			BIT(0)

/* Sub-System PCIE PHY Control Register */
#define PCIE_COMBO_PHY_CTL		0x10
#define   PHY_USE_SEL			BIT(31)	/* 0:PCIE; 1:USB3 */
#define   PHY_CLK_SEL			BIT(30) /* 0:internal clk; 1:external clk */
#define   PHY_BIST_EN			BIT(16)
#define   PHY_PIPE_SW			BIT(9)
#define   PHY_PIPE_SEL			BIT(8)  /* 0:rstn by PCIE or USB3; 1:rstn by PHY_PIPE_SW */
#define   PHY_PIPE_CLK_INVERT		BIT(4)
#define   PHY_FPGA_SYS_RSTN		BIT(1)  /* for FPGA  */
#define   PHY_RSTN			BIT(0)

/* Registers */
#define  COMBO_REG_SYSVER(comb_base_addr)	((comb_base_addr) \
							+ PCIE_USB3_SYS_VER)
#define  COMBO_REG_PCIEBGR(comb_base_addr)	((comb_base_addr) \
							+ PCIE_COMBO_PHY_BGR)
#define  COMBO_REG_USB3BGR(comb_base_addr)	((comb_base_addr) \
							+ USB3_COMBO_PHY_BGR)
#define  COMBO_REG_PHYCTRL(comb_base_addr)	((comb_base_addr) \
							+ PCIE_COMBO_PHY_CTL)

/* Sub-System Version Number */
#define  COMBO_VERSION_01				(0x10000)
#define  COMBO_VERSION_ANY				(0x0)

enum phy_use_sel {
	PHY_USE_BY_PCIE = 0, /* PHY used by PCIE */
	PHY_USE_BY_USB3, /* PHY used by USB3 */
	PHY_USE_BY_PCIE_USB3_U2,/* PHY used by PCIE & USB3_U2 */
};

enum phy_refclk_sel {
	INTER_SIG_REF_CLK = 0, /* PHY use internal single end reference clock */
	EXTER_DIF_REF_CLK, /* PHY use external single end reference clock */
};

struct sunxi_combophy_of_data {
	bool has_cfg_clk;
	bool has_slv_clk;
	bool has_phy_mbus_clk;
	bool has_phy_ahb_clk;
	bool has_pcie_axi_clk;
	bool has_u2_phy_mux;
	bool need_noppu_rst;
	bool has_u3_phy_data_quirk;
	bool need_optimize_jitter;
};

struct sunxi_combphy {
	struct device *dev;
	struct phy *phy;
	void __iomem *phy_ctl;  /* parse dts, control the phy mode, reset and power */
	void __iomem *phy_clk;  /* parse dts, set the phy clock */
	struct reset_control *reset;
	struct reset_control *noppu_reset;

	struct clk *phyclk_ref;
	struct clk *refclk_par;
	struct clk *phyclk_cfg;
	struct clk *cfgclk_par;
	struct clk *phy_mclk;
	struct clk *phy_hclk;
	struct clk *phy_axi;
	struct clk *phy_axi_par;
	__u8 mode;
	__u32 vernum; /* version number */
	enum phy_use_sel user;
	enum phy_refclk_sel ref;
	const struct sunxi_combophy_of_data *drvdata;

	struct regulator *select3v3_supply;
	bool initialized;
};

static void sunxi_combphy_usb3_phy_set(struct sunxi_combphy *combphy, bool enable)
{
	u32 val, tmp = 0;

	val = readl(combphy->phy_clk + 0x1418);
	tmp = GENMASK(17, 16);
	if (enable) {
		val &= ~tmp;
		val |= BIT(25);
	} else {
		val |= tmp;
		val &= ~BIT(25);
	}
	writel(val, combphy->phy_clk + 0x1418);

	/* reg_rx_eq_bypass[3]=1, rx_ctle_res_cal_bypass */
	val = readl(combphy->phy_clk + 0x0674);
	if (enable)
		val |= BIT(3);
	else
		val &= ~BIT(3);
	writel(val, combphy->phy_clk + 0x0674);

	/* rx_ctle_res_cal=0xf, 0x4->0xf */
	val = readl(combphy->phy_clk + 0x0704);
	tmp = GENMASK(9, 8) | BIT(11);
	if (enable)
		val |= tmp;
	else
		val &= ~tmp;
	writel(val, combphy->phy_clk + 0x0704);

	/* CDR_div_fin_gain1 */
	val = readl(combphy->phy_clk + 0x0400);
	if (enable)
		val |= BIT(4);
	else
		val &= ~BIT(4);
	writel(val, combphy->phy_clk + 0x0400);

	/* CDR_div1_fin_gain1 */
	val = readl(combphy->phy_clk + 0x0404);
	tmp = GENMASK(3, 0) | BIT(5);
	if (enable)
		val |= tmp;
	else
		val &= ~tmp;
	writel(val, combphy->phy_clk + 0x0404);

	/* CDR_div3_fin_gain1 */
	val = readl(combphy->phy_clk + 0x0408);
	if (enable)
		val |= BIT(5);
	else
		val &= ~BIT(5);
	writel(val, combphy->phy_clk + 0x0408);

	val = readl(combphy->phy_clk + 0x109c);
	if (enable)
		val |= BIT(1);
	else
		val &= ~BIT(1);
	writel(val, combphy->phy_clk + 0x109c);

	/* balance parm configure */
	if (combphy->drvdata->has_u3_phy_data_quirk) {
		val = readl(combphy->phy_clk + 0x0804);
		if (enable)
			val |= (0x6<<4);
		else
			val &= ~(0xf<<4);
		writel(val, combphy->phy_clk + 0x0804);
	}

	/* SSC configure */
	val = readl(combphy->phy_clk + 0x107c);
	tmp = 0x3f << 12;
	val = val & (~tmp);
	val |= ((0x1 << 12) & tmp);              /* div_N */
	writel(val, combphy->phy_clk + 0x107c);

	val = readl(combphy->phy_clk + 0x1020);
	tmp = 0x1f << 0;
	val = val & (~tmp);
	val |= ((0x6 << 0) & tmp);               /* modulation freq div */
	writel(val, combphy->phy_clk + 0x1020);

	val = readl(combphy->phy_clk + 0x1034);
	tmp = 0x7f << 16;
	val = val & (~tmp);
	val |= ((0x9 << 16) & tmp);              /* spread[6:0], 400*9=4410ppm ssc */
	writel(val, combphy->phy_clk + 0x1034);

	val = readl(combphy->phy_clk + 0x101c);
	tmp = 0x1 << 27;
	val = val & (~tmp);
	val |= ((0x1 << 27) & tmp);              /* choose downspread */

	tmp = 0x1 << 28;
	val = val & (~tmp);
	if (enable)
		val |= ((0x0 << 28) & tmp);      /* don't disable ssc = 0 */
	else
		val |= ((0x1 << 28) & tmp);      /* don't enable ssc = 1 */
	writel(val, combphy->phy_clk + 0x101c);

#ifdef SUNXI_INNO_COMMBOPHY_DEBUG
	/* TX Eye configure bypass_en */
	val = readl(combphy->phy_clk + 0x0ddc);
	if (enable)
		val |= BIT(4);                   /*  0x0ddc[4]=1 */
	else
		val &= ~BIT(4);
	writel(val, combphy->phy_clk + 0x0ddc);

	/* Leg_cur[6:0] - 7'd84 */
	val = readl(combphy->phy_clk + 0x0ddc);
	val |= ((0x54 & BIT(6)) >> 3);           /* 0x0ddc[3] */
	writel(val, combphy->phy_clk + 0x0ddc);

	val = readl(combphy->phy_clk + 0x0de0);
	val |= ((0x54 & GENMASK(5, 0)) << 2);    /* 0x0de0[7:2] */
	writel(val, combphy->phy_clk + 0x0de0);

	/* Leg_curb[5:0] - 6'd18 */
	val = readl(combphy->phy_clk + 0x0de4);
	val |= ((0x12 & GENMASK(5, 1)) >> 1);    /* 0x0de4[4:0] */
	writel(val, combphy->phy_clk + 0x0de4);

	val = readl(combphy->phy_clk + 0x0de8);
	val |= ((0x12 & BIT(0)) << 7);           /* 0x0de8[7] */
	writel(val, combphy->phy_clk + 0x0de8);

	/* Exswing_isel */
	val = readl(combphy->phy_clk + 0x0028);
	val |= (0x4 << 28);                      /* 0x28[30:28] */
	writel(val, combphy->phy_clk + 0x0028);

	/* Exswing_en */
	val = readl(combphy->phy_clk + 0x0028);
	if (enable)
		val |= BIT(31);                  /* 0x28[31]=1 */
	else
		val &= ~BIT(31);
	writel(val, combphy->phy_clk + 0x0028);
#endif
}

static int sunxi_combphy_usb3_init(struct sunxi_combphy *combphy)
{
	sunxi_combphy_usb3_phy_set(combphy, true);

	return 0;
}

// static int sunxi_combphy_usb3_exit(struct sunxi_combphy *combphy)
// {
// 	sunxi_combphy_usb3_phy_set(combphy, false);

// 	return 0;
// }

static void sunxi_combphy_pcie_phy_enable(struct sunxi_combphy *combphy)
{
	u32 val;

	/* set the phy:
	 * bit(18): slv aclk enable
	 * bit(17): aclk enable
	 * bit(16): hclk enbale
	 * bit(1) : pcie_presetn
	 * bit(0) : pcie_power_up_rstn
	 */
	val = readl(combphy->phy_ctl + PCIE_COMBO_PHY_BGR);
	val &= (~(0x03<<0));
	val &= (~(0x03<<16));
	val |= (0x03<<0);
	if (combphy->drvdata->has_slv_clk)
		val |= (0x07<<16);
	else
		val |= (0x03<<16);
	writel(val, combphy->phy_ctl + PCIE_COMBO_PHY_BGR);


	/* select phy mode, phy assert */
	val = readl(combphy->phy_ctl + PCIE_COMBO_PHY_CTL);
	val &= (~PHY_USE_SEL);
	val &= (~(0x03<<8));
	val &= (~PHY_FPGA_SYS_RSTN);
	val &= (~PHY_RSTN);
	writel(val, combphy->phy_ctl + PCIE_COMBO_PHY_CTL);

	 /* phy De-assert */
	val = readl(combphy->phy_ctl + PCIE_COMBO_PHY_CTL);
	val &= (~PHY_CLK_SEL);
	val &= (~(0x03<<8));
	val &= (~PHY_FPGA_SYS_RSTN);
	val &= (~PHY_RSTN);
	val |= PHY_RSTN;
	writel(val, combphy->phy_ctl + PCIE_COMBO_PHY_CTL);

	val = readl(combphy->phy_ctl + PCIE_COMBO_PHY_CTL);
	val &= (~PHY_CLK_SEL);
	val &= (~(0x03<<8));
	val &= (~PHY_FPGA_SYS_RSTN);
	val &= (~PHY_RSTN);
	val |= PHY_RSTN;
	val |= (PHY_FPGA_SYS_RSTN);
	writel(val, combphy->phy_ctl + PCIE_COMBO_PHY_CTL);

}

static void sunxi_combphy_pcie_phy_100M(struct sunxi_combphy *combphy)
{
	u32 val;

	val = readl(combphy->phy_clk + 0x1004);
	val &= ~(0x3<<3);
	val &= ~(0x1<<0);
	val |= (0x1<<0);
	val |= (0x1<<2);
	val |= (0x1<<4);
	writel(val, combphy->phy_clk + 0x1004);

	val = readl(combphy->phy_clk + 0x1018);
	val &= ~(0x3<<4);
	val |= (0x3<<4);
	writel(val, combphy->phy_clk + 0x1018);

	val = readl(combphy->phy_clk + 0x101c);
	val &= ~(0x0fffffff);
	writel(val, combphy->phy_clk + 0x101c);

	/* if need optimize jitter parm*/
	if (combphy->drvdata->need_optimize_jitter) {
		val = readl(combphy->phy_clk + 0x107c);
		val &= ~(0x3ffff);
		val |= (0x4<<12);
		val |= 0x64;
		writel(val, combphy->phy_clk + 0x107c);

		val = readl(combphy->phy_clk + 0x1030);
		val &= ~(0x3<<20);
		writel(val, combphy->phy_clk + 0x1030);

		val = readl(combphy->phy_clk + 0x1050);
		val &= ~(0x7<<0);
		val &= ~(0x7<<5);
		val &= ~(0x3<<3);
		val |= (0x3<<3);
		writel(val, combphy->phy_clk + 0x1050);
	} else {
		val = readl(combphy->phy_clk + 0x107c);
		val &= ~(0x3ffff);
		val |= (0x2<<12);
		val |= 0x32;
		writel(val, combphy->phy_clk + 0x107c);

		val = readl(combphy->phy_clk + 0x1030);
		val &= ~(0x3<<20);
		writel(val, combphy->phy_clk + 0x1030);

		val = readl(combphy->phy_clk + 0x1050);
		val &= ~(0x7<<5);
		val |= (0x1<<5);
		writel(val, combphy->phy_clk + 0x1050);
	}

	val = readl(combphy->phy_clk + 0x1054);
	val &= ~(0x7<<5);
	val |= (0x1<<5);
	writel(val, combphy->phy_clk + 0x1054);

	val = readl(combphy->phy_clk + 0x0804);
	val &= ~(0xf<<4);
	val |= (0xc<<4);
	writel(val, combphy->phy_clk + 0x0804);

	val = readl(combphy->phy_clk + 0x109c);
	val &= ~(0x3<<8);
	val |= (0x1<<1);
	writel(val, combphy->phy_clk + 0x109c);

	writel(0x80540a0a, combphy->phy_clk + 0x1418);
}

static int sunxi_combphy_pcie_init(struct sunxi_combphy *combphy)
{
	sunxi_combphy_pcie_phy_100M(combphy);

	sunxi_combphy_pcie_phy_enable(combphy);

	return 0;
}

static int sunxi_combphy_set_mode(struct sunxi_combphy *combphy)
{
	switch (combphy->mode) {
	case PHY_TYPE_PCIE:
		sunxi_combphy_pcie_init(combphy);
		break;
	case PHY_TYPE_USB3:
		if (combphy->user == PHY_USE_BY_PCIE_USB3_U2) {
			sunxi_combphy_pcie_init(combphy);
		} else if (combphy->user == PHY_USE_BY_USB3) {
			sunxi_combphy_usb3_init(combphy);
		}
		break;
	default:
		printf("incompatible PHY type\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_inno_phy_init(struct phy *phy)
{
	struct sunxi_combphy *combphy = dev_get_priv(phy->dev);
	int ret;

	ret = sunxi_combphy_set_mode(combphy);
	if (ret) {
		printf("invalid number of arguments\n");
		return ret;
	}

	return ret;
}

/*  PCIE USB3 Sub-system Application */
static void combo_pcie_clk_set(struct sunxi_combphy *combphy, bool enable)
{
	u32 val, tmp = 0;

	val = readl(COMBO_REG_PCIEBGR(combphy->phy_ctl));
	if (combphy->drvdata->has_slv_clk)
		tmp = PCIE_SLV_ACLK_EN | PCIE_ACLK_EN | PCIE_HCLK_EN | PCIE_PERSTN | PCIE_PW_UP_RSTN;
	else
		tmp = PCIE_ACLK_EN | PCIE_HCLK_EN | PCIE_PERSTN | PCIE_PW_UP_RSTN;
	if (enable)
		val |= tmp;
	else
		val &= ~tmp;
	writel(val, COMBO_REG_PCIEBGR(combphy->phy_ctl));
}

static void combo_usb3_clk_set(struct sunxi_combphy *combphy, bool enable)
{
	u32 val, tmp = 0;

	val = readl(COMBO_REG_USB3BGR(combphy->phy_ctl));
	if (combphy->drvdata->has_u2_phy_mux)
		tmp = USB3_ACLK_EN | USB3_HCLK_EN | USB3_U2_PHY_MUX_SEL | USB3_U2_PHY_RSTN | USB3_U2_PHY_MUX_EN;
	else
		tmp = USB3_ACLK_EN | USB3_HCLK_EN | USB3_RESETN;
	if (enable)
		val |= tmp;
	else
		val &= ~tmp;
	writel(val, COMBO_REG_USB3BGR(combphy->phy_ctl));
}

static void combo_phy_mode_set(struct sunxi_combphy *combphy, bool enable)
{
	u32 val, tmp = 0;

	val = readl(COMBO_REG_PHYCTRL(combphy->phy_ctl));

	if (combphy->user == PHY_USE_BY_PCIE)
		tmp &= ~PHY_USE_SEL;
	else if (combphy->user == PHY_USE_BY_USB3)
		tmp |= PHY_USE_SEL;
	else if (combphy->user == PHY_USE_BY_PCIE_USB3_U2)
		tmp &= ~PHY_USE_SEL;

	if (combphy->ref == INTER_SIG_REF_CLK)
		tmp &= ~PHY_CLK_SEL;
	else if (combphy->ref == EXTER_DIF_REF_CLK)
		tmp |= PHY_CLK_SEL;

	if (enable) {
		tmp |= PHY_RSTN;
		val |= tmp;
	} else {
		tmp &= ~PHY_RSTN;
		val &= ~tmp;
	}
	writel(val, COMBO_REG_PHYCTRL(combphy->phy_ctl));
}

static u32 combo_sysver_get(struct sunxi_combphy *combphy)
{
	u32 reg;

	reg = readl(COMBO_REG_SYSVER(combphy->phy_ctl));

	return reg;
}

static void pcie_usb3_sub_system_enable(struct sunxi_combphy *combphy)
{
	combo_phy_mode_set(combphy, true);

	if (combphy->user == PHY_USE_BY_PCIE)
		combo_pcie_clk_set(combphy, true);
	else if (combphy->user == PHY_USE_BY_USB3)
		combo_usb3_clk_set(combphy, true);
	else if (combphy->user == PHY_USE_BY_PCIE_USB3_U2) {
		combo_pcie_clk_set(combphy, true);
		combo_usb3_clk_set(combphy, true);
	}

	combphy->vernum = combo_sysver_get(combphy);
}

static int pcie_usb3_sub_system_init(struct sunxi_combphy *combphy)
{
	unsigned long reg_value = 0;
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	if (combphy->initialized)
		return 0;

	//0xaac pcie_bgr_reg
	reg_value = readl(&ccm->pcie_bgr_reg);
	reg_value |= (1 << PCIE_BRG_REG_RST);
	writel(reg_value, &ccm->pcie_bgr_reg);

	//0xa84 usb2_ref_clk_reg
	// reg_value = readl(&ccm->usb2_ref_clk_reg);
	reg_value = readl(0x2001a84);
	reg_value |= (1 << PCIE_REF_CLK_GATING);
	reg_value = 0x81000001;
	writel(reg_value, 0x2001a84);
	// writel(reg_value, &ccm->usb2_ref_clk_reg);

	pcie_usb3_sub_system_enable(combphy);

	combphy->initialized = true;

	return 0;
}

static int sunxi_inno_phy_probe(struct udevice *dev)
{
	struct sunxi_combphy *combphy = dev_get_priv(dev);
	int ret;

	memset(combphy, 0, sizeof(*combphy));
	combphy->drvdata = (struct sunxi_combophy_of_data *)dev_get_driver_data(dev);
	combphy->user = PHY_USE_BY_PCIE;	/* 0:PCIE; 1:USB3 */
	combphy->mode = PHY_TYPE_PCIE;
	combphy->phy_ctl = (void *)SUNXI_COMBOPHY_CTL_BASE;
	combphy->phy_clk = (void *)SUNXI_COMBOPHY_CLK_BASE;
	combphy->ref = EXTER_DIF_REF_CLK;

	ret = pcie_usb3_sub_system_init(combphy);
	if (ret)
		printf("failed to init sub system\n");

	return 0;
}

static const struct sunxi_combophy_of_data sunxi_inno_v1_of_data = {
	.has_cfg_clk = false,
};

static const struct phy_ops sunxi_inno_phy_ops = {
	.init		= sunxi_inno_phy_init,
};

static const struct udevice_id sunxi_inno_phy_of_match_table[] = {
	{
		.compatible = "allwinner,inno-combphy",
		.data = (ulong)&sunxi_inno_v1_of_data,
	},
};

U_BOOT_DRIVER(sunxi_inno_combophy) = {
	.name	= "sunxi_inno_combophy",
	.id	= UCLASS_PHY,
	.of_match = sunxi_inno_phy_of_match_table,
	.ops = &sunxi_inno_phy_ops,
	.probe = sunxi_inno_phy_probe,
	.priv_auto_alloc_size = sizeof(struct sunxi_combphy),
};
