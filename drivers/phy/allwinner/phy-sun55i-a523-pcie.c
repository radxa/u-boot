// SPDX-License-Identifier: GPL-2.0
/*
 * Allwinner A523 INNO PCIe PHY.
 * Based on Allwinner's sunxi-inno-combophy.c (Copyright 2020-2024).
 */

#include <dm.h>
#include <generic-phy.h>
#include <reset.h>
#include <asm/arch/cpu.h>
#include <asm/io.h>
#include <dt-bindings/phy/phy.h>
#include <linux/bitops.h>

#define A523_USB3_REF_CLK	(SUNXI_CCM_BASE + 0xa84)
#define COMBO_PCIE_BGR		0x04
#define COMBO_PHY_CTL		0x10

struct a523_pcie_phy {
	void __iomem *ctl;
	void __iomem *regs;
	struct reset_ctl reset;
};

static void a523_pcie_phy_configure(struct a523_pcie_phy *priv)
{
	void __iomem *regs = priv->regs;

	/* Preserve the vendor's 100 MHz PCIe PLL and analogue calibration. */
	clrsetbits_le32(regs + 0x1004, GENMASK(4, 3) | BIT(0),
			BIT(4) | BIT(2) | BIT(0));
	setbits_le32(regs + 0x1018, GENMASK(5, 4));
	clrbits_le32(regs + 0x101c, GENMASK(27, 0));
	clrsetbits_le32(regs + 0x107c, GENMASK(17, 0), BIT(13) | 0x32);
	clrbits_le32(regs + 0x1030, GENMASK(21, 20));
	clrsetbits_le32(regs + 0x1050, GENMASK(7, 5), BIT(5));
	clrsetbits_le32(regs + 0x1054, GENMASK(7, 5), BIT(5));
	clrsetbits_le32(regs + 0x0804, GENMASK(7, 4), 0xc << 4);
	clrsetbits_le32(regs + 0x109c, GENMASK(9, 8), BIT(1));
	writel(0x80540a0a, regs + 0x1418);

	setbits_le32(priv->ctl + COMBO_PCIE_BGR,
		     BIT(17) | BIT(16) | BIT(1) | BIT(0));
	clrbits_le32(priv->ctl + COMBO_PHY_CTL,
		     BIT(31) | BIT(30) | GENMASK(9, 8) | BIT(1) | BIT(0));
	setbits_le32(priv->ctl + COMBO_PHY_CTL, BIT(0));
	setbits_le32(priv->ctl + COMBO_PHY_CTL, BIT(1));
}

static int a523_pcie_phy_init(struct phy *phy)
{
	struct a523_pcie_phy *priv = dev_get_priv(phy->dev);
	int ret;

	ret = reset_deassert(&priv->reset);
	if (ret)
		return ret;

	/* Select PLL_PERIPH0_200M / 2 for the shared PHY reference clock. */
	writel(BIT(31) | BIT(24) | 1, A523_USB3_REF_CLK);
	a523_pcie_phy_configure(priv);

	return 0;
}

static int a523_pcie_phy_exit(struct phy *phy)
{
	struct a523_pcie_phy *priv = dev_get_priv(phy->dev);

	clrbits_le32(priv->ctl + COMBO_PHY_CTL, BIT(0));
	clrbits_le32(priv->ctl + COMBO_PCIE_BGR,
		     BIT(17) | BIT(16) | BIT(1) | BIT(0));
	clrbits_le32(A523_USB3_REF_CLK, BIT(31));

	return reset_assert(&priv->reset);
}

static int a523_pcie_phy_xlate(struct phy *phy,
			       struct ofnode_phandle_args *args)
{
	if (args->args_count != 1 || args->args[0] != PHY_TYPE_PCIE)
		return -EINVAL;

	return 0;
}

static int a523_pcie_phy_probe(struct udevice *dev)
{
	struct a523_pcie_phy *priv = dev_get_priv(dev);

	priv->ctl = dev_remap_addr_name(dev, "phy-ctl");
	priv->regs = dev_remap_addr_name(dev, "phy-clk");
	if (!priv->ctl || !priv->regs)
		return -EINVAL;

	return reset_get_by_index(dev, 0, &priv->reset);
}

static const struct phy_ops a523_pcie_phy_ops = {
	.init = a523_pcie_phy_init,
	.exit = a523_pcie_phy_exit,
	.of_xlate = a523_pcie_phy_xlate,
};

static const struct udevice_id a523_pcie_phy_ids[] = {
	{ .compatible = "allwinner,sun55i-a523-pcie-phy" },
	{ }
};

U_BOOT_DRIVER(a523_pcie_phy) = {
	.name = "a523_pcie_phy",
	.id = UCLASS_PHY,
	.of_match = a523_pcie_phy_ids,
	.probe = a523_pcie_phy_probe,
	.ops = &a523_pcie_phy_ops,
	.priv_auto = sizeof(struct a523_pcie_phy),
};
