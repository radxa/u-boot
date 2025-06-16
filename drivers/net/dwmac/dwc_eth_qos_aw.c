// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Allwinner
 */
#include <common.h>
#include <clk/clk.h>
#include <dm.h>
#include <errno.h>
#include <log.h>
#include <malloc.h>
#include <miiphy.h>
#include <net.h>
#include <netdev.h>
#include <phy.h>
#include <reset.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <dm/pinctrl.h>
#include <dm/device.h>
#include <power/regulator.h>
#include <linux/bitfield.h>
#include <linux/delay.h>
#include <sys_config.h>
#include <asm-generic/gpio.h>
#include <sunxi_power/axp.h>

#include "dwc_eth_qos.h"

#define DWMAC_SUNXI_MODULE_VERSION	"0.1.1"

/* GMAC-200 Register */
#define SUNXI_DWMAC200_SYSCON_REG	(0x00)
	#define SUNXI_DWMAC200_SYSCON_BPS_EFUSE		GENMASK(31, 28)
	#define SUNXI_DWMAC200_SYSCON_XMII_SEL		BIT(27)
	#define SUNXI_DWMAC200_SYSCON_EPHY_MODE		GENMASK(26, 25)
	#define SUNXI_DWMAC200_SYSCON_PHY_ADDR		GENMASK(24, 20)
	#define SUNXI_DWMAC200_SYSCON_BIST_CLK_EN	BIT(19)
	#define SUNXI_DWMAC200_SYSCON_CLK_SEL		BIT(18)
	#define SUNXI_DWMAC200_SYSCON_LED_POL		BIT(17)
	#define SUNXI_DWMAC200_SYSCON_SHUTDOWN		BIT(16)
	#define SUNXI_DWMAC200_SYSCON_PHY_SEL		BIT(15)
	#define SUNXI_DWMAC200_SYSCON_ENDIAN_MODE	BIT(14)
	#define SUNXI_DWMAC200_SYSCON_RMII_EN		BIT(13)
	#define SUNXI_DWMAC200_SYSCON_ETXDC			GENMASK(12, 10)
	#define SUNXI_DWMAC200_SYSCON_ERXDC			GENMASK(9, 5)
	#define SUNXI_DWMAC200_SYSCON_ERXIE			BIT(4)
	#define SUNXI_DWMAC200_SYSCON_ETXIE			BIT(3)
	#define SUNXI_DWMAC200_SYSCON_EPIT			BIT(2)
	#define SUNXI_DWMAC200_SYSCON_ETCS			GENMASK(1, 0)

/* GMAC-210 Register */
#define SUNXI_DWMAC210_CFG_REG	(0x00)
	#define SUNXI_DWMAC210_CFG_ETXDC_H		GENMASK(17, 16)
	#define SUNXI_DWMAC210_CFG_PHY_SEL		BIT(15)
	#define SUNXI_DWMAC210_CFG_ENDIAN_MODE	BIT(14)
	#define SUNXI_DWMAC210_CFG_RMII_EN		BIT(13)
	#define SUNXI_DWMAC210_CFG_ETXDC_L		GENMASK(12, 10)
	#define SUNXI_DWMAC210_CFG_ERXDC		GENMASK(9, 5)
	#define SUNXI_DWMAC210_CFG_ERXIE		BIT(4)
	#define SUNXI_DWMAC210_CFG_ETXIE		BIT(3)
	#define SUNXI_DWMAC210_CFG_EPIT			BIT(2)
	#define SUNXI_DWMAC210_CFG_ETCS			GENMASK(1, 0)
#define SUNXI_DWMAC210_PTP_TIMESTAMP_L_REG	(0x40)
#define SUNXI_DWMAC210_PTP_TIMESTAMP_H_REG	(0x48)
#define SUNXI_DWMAC210_STAT_INT_REG		(0x4C)
	#define SUNXI_DWMAC210_STAT_PWR_DOWN_ACK	BIT(4)
	#define SUNXI_DWMAC210_STAT_SBD_TX_CLK_GATE	BIT(3)
	#define SUNXI_DWMAC210_STAT_LPI_INT			BIT(1)
	#define SUNXI_DWMAC210_STAT_PMT_INT			BIT(0)
#define SUNXI_DWMAC210_CLK_GATE_CFG_REG	(0x80)
	#define SUNXI_DWMAC210_CLK_GATE_CFG_RX		BIT(7)
	#define SUNXI_DWMAC210_CLK_GATE_CFG_PTP_REF	BIT(6)
	#define SUNXI_DWMAC210_CLK_GATE_CFG_CSR		BIT(5)
	#define SUNXI_DWMAC210_CLK_GATE_CFG_TX		BIT(4)
	#define SUNXI_DWMAC210_CLK_GATE_CFG_APP		BIT(3)

#define SUNXI_DWMAC_ETCS_MII		0x0
#define SUNXI_DWMAC_ETCS_EXT_GMII	0x1
#define SUNXI_DWMAC_ETCS_INT_GMII	0x2

/* MAC flags defined */
#define SUNXI_DWMAC_NSI_CLK_GATE	BIT(0)

struct sunxi_dwmac;

enum sunxi_dwmac_delaychain_dir {
	SUNXI_DWMAC_DELAYCHAIN_TX,
	SUNXI_DWMAC_DELAYCHAIN_RX,
};

struct sunxi_dwmac_variant {
	u32 flags;
	u32 interface;
	u32 rx_delay_max;
	u32 tx_delay_max;
	int (*set_syscon)(struct sunxi_dwmac *chip);
	int (*set_delaychain)(struct sunxi_dwmac *chip, enum sunxi_dwmac_delaychain_dir dir, u32 delay);
	u32 (*get_delaychain)(struct sunxi_dwmac *chip, enum sunxi_dwmac_delaychain_dir dir);
};

struct sunxi_dwmac {
    struct udevice *dev;
    const struct sunxi_dwmac_variant *variant;
	void *syscfg_base;
	const char *dwmac_supply;
	const char *phy_supply;
	struct clk *pclk;
	struct clk *axi_clk;
	struct clk *ahb_clk;
	struct clk *phy_clk;
	struct clk *nsi_clk;
	struct reset_ctl *mac_rst;
	struct reset_ctl *ahb_rst;
    bool rgmii_clk_ext;
	bool soc_phy_clk_en;
	int bus_num;
    int interface;
	u32 tx_delay;
	u32 rx_delay;
};

static inline int dev_seq(const struct udevice *dev)
{
	return dev->seq;
}

static int sunxi_dwmac200_set_syscon(struct sunxi_dwmac *chip)
{
	u32 reg_val = 0;

	/* Clear interface mode bits */
	reg_val &= ~(SUNXI_DWMAC200_SYSCON_ETCS | SUNXI_DWMAC200_SYSCON_EPIT);
	if (chip->variant->interface & PHY_INTERFACE_MODE_RMII)
		reg_val &= ~SUNXI_DWMAC200_SYSCON_RMII_EN;

	switch (chip->interface) {
	case PHY_INTERFACE_MODE_MII:
		/* default */
		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		reg_val |= SUNXI_DWMAC200_SYSCON_EPIT;
		reg_val |= FIELD_PREP(SUNXI_DWMAC200_SYSCON_ETCS,
					chip->rgmii_clk_ext ? SUNXI_DWMAC_ETCS_EXT_GMII : SUNXI_DWMAC_ETCS_INT_GMII);
		break;
	case PHY_INTERFACE_MODE_RMII:
		reg_val |= SUNXI_DWMAC200_SYSCON_RMII_EN;
		reg_val &= ~SUNXI_DWMAC200_SYSCON_ETCS;
		break;
	default:
		dev_err(chip->dev, "Unsupported interface mode: %s\n", phy_string_for_interface(chip->interface));
		return -EINVAL;
	}

	writel(reg_val, chip->syscfg_base + SUNXI_DWMAC200_SYSCON_REG);
	return 0;
}

static int sunxi_dwmac200_set_delaychain(struct sunxi_dwmac *chip, enum sunxi_dwmac_delaychain_dir dir, u32 delay)
{
	u32 reg_val = readl(chip->syscfg_base + SUNXI_DWMAC200_SYSCON_REG);
	int ret = -EINVAL;

	switch (dir) {
	case SUNXI_DWMAC_DELAYCHAIN_TX:
		if (delay <= chip->variant->tx_delay_max) {
			reg_val &= ~SUNXI_DWMAC200_SYSCON_ETXDC;
			reg_val |= FIELD_PREP(SUNXI_DWMAC200_SYSCON_ETXDC, delay);
			ret = 0;
		}
		break;
	case SUNXI_DWMAC_DELAYCHAIN_RX:
		if (delay <= chip->variant->rx_delay_max) {
			reg_val &= ~SUNXI_DWMAC200_SYSCON_ERXDC;
			reg_val |= FIELD_PREP(SUNXI_DWMAC200_SYSCON_ERXDC, delay);
			ret = 0;
		}
		break;
	}

	if (!ret)
		writel(reg_val, chip->syscfg_base + SUNXI_DWMAC200_SYSCON_REG);

	return ret;
}

static u32 sunxi_dwmac200_get_delaychain(struct sunxi_dwmac *chip, enum sunxi_dwmac_delaychain_dir dir)
{
	u32 delay = 0;
	u32 reg_val = readl(chip->syscfg_base + SUNXI_DWMAC200_SYSCON_REG);

	switch (dir) {
	case SUNXI_DWMAC_DELAYCHAIN_TX:
		delay = FIELD_GET(SUNXI_DWMAC200_SYSCON_ETXDC, reg_val);
		break;
	case SUNXI_DWMAC_DELAYCHAIN_RX:
		delay = FIELD_GET(SUNXI_DWMAC200_SYSCON_ERXDC, reg_val);
		break;
	default:
		dev_err(chip->dev, "Unknow delaychain dir %d\n", dir);
	}

	return delay;
}

static int sunxi_dwmac210_set_delaychain(struct sunxi_dwmac *chip, enum sunxi_dwmac_delaychain_dir dir, u32 delay)
{
	u32 reg_val = readl(chip->syscfg_base + SUNXI_DWMAC210_CFG_REG);
	int ret = -EINVAL;

	switch (dir) {
	case SUNXI_DWMAC_DELAYCHAIN_TX:
		if (delay <= chip->variant->tx_delay_max) {
			reg_val &= ~(SUNXI_DWMAC210_CFG_ETXDC_H | SUNXI_DWMAC210_CFG_ETXDC_L);
			reg_val |= FIELD_PREP(SUNXI_DWMAC210_CFG_ETXDC_H, delay >> 3);
			reg_val |= FIELD_PREP(SUNXI_DWMAC210_CFG_ETXDC_L, delay);
			ret = 0;
		}
		break;
	case SUNXI_DWMAC_DELAYCHAIN_RX:
		if (delay <= chip->variant->rx_delay_max) {
			reg_val &= ~SUNXI_DWMAC210_CFG_ERXDC;
			reg_val |= FIELD_PREP(SUNXI_DWMAC210_CFG_ERXDC, delay);
			ret = 0;
		}
		break;
	}

	if (!ret)
		writel(reg_val, chip->syscfg_base + SUNXI_DWMAC210_CFG_REG);

	return ret;
}

static u32 sunxi_dwmac210_get_delaychain(struct sunxi_dwmac *chip, enum sunxi_dwmac_delaychain_dir dir)
{
	u32 delay = 0;
	u32 tx_l, tx_h;
	u32 reg_val = readl(chip->syscfg_base + SUNXI_DWMAC210_CFG_REG);

	switch (dir) {
	case SUNXI_DWMAC_DELAYCHAIN_TX:
		tx_h = FIELD_GET(SUNXI_DWMAC210_CFG_ETXDC_H, reg_val);
		tx_l = FIELD_GET(SUNXI_DWMAC210_CFG_ETXDC_L, reg_val);
		delay = (tx_h << 3 | tx_l);
		break;
	case SUNXI_DWMAC_DELAYCHAIN_RX:
		delay = FIELD_GET(SUNXI_DWMAC210_CFG_ERXDC, reg_val);
		break;
	}

	return delay;
}

static const struct sunxi_dwmac_variant dwmac200_variant = {
	.interface = PHY_INTERFACE_MODE_RMII | PHY_INTERFACE_MODE_RGMII,
	.rx_delay_max = 31,
	.tx_delay_max = 7,
	.set_syscon = sunxi_dwmac200_set_syscon,
	.set_delaychain = sunxi_dwmac200_set_delaychain,
	.get_delaychain = sunxi_dwmac200_get_delaychain,
};

static const struct sunxi_dwmac_variant dwmac210_variant = {
	.interface = PHY_INTERFACE_MODE_RMII | PHY_INTERFACE_MODE_RGMII,
	.rx_delay_max = 31,
	.tx_delay_max = 31,
	.set_syscon = sunxi_dwmac200_set_syscon,
	.set_delaychain = sunxi_dwmac210_set_delaychain,
	.get_delaychain = sunxi_dwmac210_get_delaychain,
};

static const struct sunxi_dwmac_variant dwmac220_variant = {
	.interface = PHY_INTERFACE_MODE_RMII | PHY_INTERFACE_MODE_RGMII,
	.flags = SUNXI_DWMAC_NSI_CLK_GATE,
	.rx_delay_max = 31,
	.tx_delay_max = 31,
	.set_syscon = sunxi_dwmac200_set_syscon,
	.set_delaychain = sunxi_dwmac210_set_delaychain,
	.get_delaychain = sunxi_dwmac210_get_delaychain,
};

static int sunxi_dwmac_power_on(struct sunxi_dwmac *chip)
{
	if (chip->dwmac_supply)
		pmu_set_voltage((char *)chip->dwmac_supply, 3300, true);
	if (chip->phy_supply)
		pmu_set_voltage((char *)chip->phy_supply, 3300, true);

	return 0;
}

static void sunxi_dwmac_power_off(struct sunxi_dwmac *chip)
{
	if (chip->dwmac_supply)
		pmu_set_voltage((char *)chip->dwmac_supply, 3300, false);
	if (chip->phy_supply)
		pmu_set_voltage((char *)chip->phy_supply, 3300, false);
}

static int sunxi_dwmac_hw_init(struct sunxi_dwmac *chip)
{
	int ret;
	char eth_name[32];

	sunxi_dwmac_power_on(chip);

	sprintf(eth_name, "ethernet%d", chip->bus_num);
	fdt_set_all_pin(eth_name, "pinctrl-0");

	ret = chip->variant->set_syscon(chip);
	if (ret < 0) {
		dev_err(chip->dev, "Set syscon failed\n");
		goto err;
	}

	ret = chip->variant->set_delaychain(chip, SUNXI_DWMAC_DELAYCHAIN_TX, chip->tx_delay);
	if (ret < 0) {
		dev_err(chip->dev, "Invalid TX clock delay: %d\n", chip->tx_delay);
		goto err;
	}

	ret = chip->variant->set_delaychain(chip, SUNXI_DWMAC_DELAYCHAIN_RX, chip->rx_delay);
	if (ret < 0) {
		dev_err(chip->dev, "Invalid RX clock delay: %d\n", chip->rx_delay);
		goto err;
	}

	return 0;
err:
	sunxi_dwmac_power_off(chip);
	return ret;
}

static void sunxi_dwmac_hw_exit(struct sunxi_dwmac *chip)
{
	sunxi_dwmac_power_off(chip);
}

static int eqos_probe_resources_aw(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	struct sunxi_dwmac *chip;
	const char *variant;
	int reset_flags = GPIOD_IS_OUT;
	int ret = 0;

	chip = malloc(sizeof(*chip));
	if (!chip)
		return -ENOMEM;
	variant = dev_read_string(dev, "aw,gmac-version");
	if (!strcmp(variant, "200"))
		chip->variant = &dwmac200_variant;
	else if (!strcmp(variant, "210") || !strcmp(variant, "211"))
		chip->variant = &dwmac210_variant;
	else if (!strcmp(variant, "220"))
		chip->variant = &dwmac220_variant;
	else
		goto err;
	chip->dev = dev;
	chip->bus_num = dev_seq(chip->dev);
	chip->syscfg_base = (void *)(uintptr_t)dev_read_addr_index(dev, 1);
	chip->interface = eqos->config->interface(dev);
	chip->rgmii_clk_ext = dev_read_bool(dev, "aw,rgmii-clk-ext");
	chip->soc_phy_clk_en = dev_read_bool(dev, "aw,soc-phy-clk-en");
	ret = dev_read_u32(dev, "tx-delay", &chip->tx_delay);
	if (ret)
		chip->tx_delay = 0;
	ret = dev_read_u32(dev, "rx-delay", &chip->rx_delay);
	if (ret)
		chip->rx_delay = 0;
	chip->dwmac_supply = dev_read_string(dev, "dwmac_supply");
	chip->phy_supply = dev_read_string(dev, "phy_supply");
	eqos->bsp_priv = chip;

	chip->pclk = clk_get_by_name(dev, "pclk");
	if (IS_ERR(chip->pclk)) {
		ret = PTR_ERR(chip->pclk);
		dev_err(dev, "get pclk failed: %d\n", ret);
		goto err;
	}

	chip->axi_clk = clk_get_by_name(dev, "axi");
	if (IS_ERR(chip->axi_clk)) {
		ret = PTR_ERR(chip->axi_clk);
		dev_err(dev, "get axi_clk failed: %d\n", ret);
		goto err;
	}

	chip->ahb_clk = clk_get_by_name(dev, "ahb");
	if (IS_ERR(chip->ahb_clk)) {
		ret = PTR_ERR(chip->ahb_clk);
		dev_err(dev, "get ahb_clk failed: %d\n", ret);
		goto err;
	}

	chip->phy_clk = clk_get_by_name(dev, "phy");
	if (IS_ERR(chip->phy_clk)) {
		ret = PTR_ERR(chip->phy_clk);
		dev_err(dev, "get phy_clk failed: %d\n", ret);
		goto err;
	}

	if (dev_read_bool(dev, "snps,reset-active-low"))
		reset_flags |= GPIOD_ACTIVE_LOW;
	eqos->phy_reset_gpio = sunxi_name_to_gpio(dev_read_string(dev, "snps,reset-gpio"));
	if (eqos->phy_reset_gpio) {
		sunxi_gpio_set_cfgpin(eqos->phy_reset_gpio, 1);
		sunxi_gpio_set_pull(eqos->phy_reset_gpio, SUNXI_GPIO_PULL_UP);
		gpio_set_value(eqos->phy_reset_gpio, 1);
	}
	dev_read_u32_array(dev, "snps,reset-delays-us", eqos->reset_delays, 3);

	debug("gmac%s eth%d interface %s with speed %d\n", variant, chip->bus_num, phy_string_for_interface(chip->interface), eqos->max_speed);
	debug("delaychain tx:%d rx:%d\n", chip->tx_delay, chip->rx_delay);
	debug("clk dir %s with phy25m %s\n", chip->rgmii_clk_ext ? "external" : "internal", chip->soc_phy_clk_en ? "soc" : "osc");
	debug("rst gpio %d [%d %d %d]\n", eqos->phy_reset_gpio, eqos->reset_delays[0], eqos->reset_delays[1], eqos->reset_delays[2]);
	debug("regulator dwmac:%s phy:%s\n", chip->dwmac_supply, chip->phy_supply);

	return 0;
err:
	if (chip)
		free(chip);
	return ret;
}

static int eqos_remove_resources_aw(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	struct sunxi_dwmac *chip = eqos->bsp_priv;

	if (chip)
		free(chip);

	return 0;
}

static int eqos_start_resets_aw(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	int ret;

	if (eqos->phy)
		return 0;

	ret = gpio_set_value(eqos->phy_reset_gpio, 1);
	if (ret < 0) {
		pr_err("set phy rst stage0 failed %d\n", ret);
		return ret;
	}

	udelay(eqos->reset_delays[0]);

	ret = gpio_set_value(eqos->phy_reset_gpio, 0);
	if (ret < 0) {
		pr_err("set phy rst stage1 failed %d\n", ret);
		return ret;
	}

	udelay(eqos->reset_delays[1]);

	ret = gpio_set_value(eqos->phy_reset_gpio, 1);
	if (ret < 0) {
		pr_err("set phy rst stage2 failed %d\n", ret);
		return ret;
	}

	udelay(eqos->reset_delays[2]);

	return 0;
}

static int eqos_start_clks_aw(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	struct sunxi_dwmac *chip = eqos->bsp_priv;
	int ret;

	clk_disable(chip->axi_clk);
	clk_disable(chip->pclk);
	udelay(2);

	ret = clk_prepare_enable(chip->pclk);
	if (ret < 0) {
		dev_err(dev, "clk_prepare_enable(pclk) failed: %d\n", ret);
		goto err_pclk;
	}

	ret = clk_prepare_enable(chip->axi_clk);
	if (ret < 0) {
		dev_err(dev, "clk_prepare_enable(axi_clk) failed: %d\n", ret);
		goto err_axi_clk;
	}

	if (chip->ahb_clk) {
		ret = clk_prepare_enable(chip->ahb_clk);
		if (ret < 0) {
			dev_err(dev, "clk_prepare_enable(ahb_clk) failed: %d\n", ret);
			goto err_ahb_clk;
		}
	}

	if (chip->soc_phy_clk_en) {
		ret = clk_prepare_enable(chip->phy_clk);
		if (ret < 0) {
			dev_err(dev, "clk_prepare_enable(phy_clk) failed: %d\n", ret);
			goto err_phy_clk;
		}
	}
	of_periph_clk_config_setup(ofnode_to_offset(dev_ofnode(dev)));

	ret = sunxi_dwmac_hw_init(chip);
	if (ret < 0) {
		dev_err(dev, "sunxi_dwmac_hw_init failed: %d\n", ret);
		goto err_hw_init;
	}

	return 0;
err_hw_init:
	if (chip->soc_phy_clk_en)
		clk_disable(chip->phy_clk);
err_phy_clk:
	clk_disable(chip->axi_clk);
err_axi_clk:
	if (chip->ahb_clk)
		clk_disable(chip->ahb_clk);
err_ahb_clk:
	clk_disable(chip->pclk);
err_pclk:
	return ret;
}

static int eqos_stop_clks_aw(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	struct sunxi_dwmac *chip = eqos->bsp_priv;

	sunxi_dwmac_hw_exit(chip);

	if (chip->soc_phy_clk_en)
		clk_disable(chip->phy_clk);
	clk_disable(chip->axi_clk);
	if (chip->ahb_clk)
		clk_disable(chip->ahb_clk);
	clk_disable(chip->pclk);

	return 0;
}

static struct eqos_ops eqos_aw_ops = {
	.eqos_inval_desc = eqos_inval_desc_generic,
	.eqos_flush_desc = eqos_flush_desc_generic,
	.eqos_inval_buffer = eqos_inval_buffer_generic,
	.eqos_flush_buffer = eqos_flush_buffer_generic,
	.eqos_probe_resources = eqos_probe_resources_aw,
	.eqos_remove_resources = eqos_remove_resources_aw,
	.eqos_stop_resets = eqos_null_ops,
	.eqos_start_resets = eqos_start_resets_aw,
	.eqos_stop_clks = eqos_stop_clks_aw,
	.eqos_start_clks = eqos_start_clks_aw,
	.eqos_calibrate_pads = eqos_null_ops,
	.eqos_disable_calibration = eqos_null_ops,
	.eqos_set_tx_clk_speed = eqos_null_ops,
	.eqos_get_enetaddr = eqos_null_ops,
};

struct eqos_config __maybe_unused eqos_aw_config = {
	.reg_access_always_ok = false,
	.mdio_wait = 10000,
	.swr_wait = 5000,
	.config_mac = EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB,
	.config_mac_mdio = EQOS_MAC_MDIO_ADDRESS_CR_150_250,
	.axi_bus_width = EQOS_AXI_WIDTH_64,
	.interface = dev_read_phy_mode,
	.ops = &eqos_aw_ops
};
