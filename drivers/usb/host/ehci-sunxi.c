/*
 * SPDX-License-Identifier: GPL-2.0+
 * (C) Copyright 20016-2020
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangwei <wangwei@allwinnertech.com>
 *
 */

#include <common.h>
#include <asm/arch/clock.h>
//#include <asm/arch/usb_phy.h>
#include <asm/io.h>
#include <dm.h>
#include <asm/arch/gpio.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include "ehci.h"
#include <sunxi_power/power_manage.h>

#if IS_ENABLED(CONFIG_MACH_SUN300IW1)
#define  USB_Readb(reg)                            (*(volatile unsigned char *)(reg))
#define  USB_Readw(reg)                            (*(volatile unsigned short *)(reg))
#define  USB_Readl(reg)                            (*(volatile unsigned long *)(reg))

#define  USB_Writeb(value, reg)			   (*(volatile unsigned char *)(reg) = (value))
#define  USB_Writew(value, reg)                    (*(volatile unsigned short *)(reg) = (value))
#define  USB_Writel(value, reg)                    (*(volatile unsigned long *)(reg) = (value))
#endif

#define SUNXI_USB_CTRL		0x800
#define SUNXI_USB_PHY_CTRL	0x810
#define HOST_MAXNUM		5
#define USB_REG_o_PHYCTL	0x410
#define USB_REG_o_PHYSTATUS	0x0424

static uintptr_t usb_vbase = SUNXI_EHCI0_BASE;
unsigned int usb_drv_vbus_gpio = -1;

typedef struct _ehci_config
{
	uintptr_t ehci_base;
	u32 bus_soft_reset_ofs;
	u32 bus_clk_gating_ofs;
	u32 phy_reset_ofs;
	u32 phy_slk_gatimg_ofs;
	u32 usb0_support;
	char name[32];
}ehci_config_t;


static ehci_config_t ehci_cfg[HOST_MAXNUM] = {
	{SUNXI_EHCI0_BASE, USBEHCI0_RST_BIT, USBEHCI0_GATIING_BIT, USBPHY0_RST_BIT, USBPHY0_SCLK_GATING_BIT, 1, "USB-DRD"},
#ifdef SUNXI_EHCI1_BASE
	{SUNXI_EHCI1_BASE, USBEHCI1_RST_BIT, USBEHCI1_GATIING_BIT, USBPHY1_RST_BIT, USBPHY1_SCLK_GATING_BIT, 0, "USB1-Host"},
#endif
#ifdef SUNXI_EHCI2_BASE
	{SUNXI_EHCI2_BASE, USBEHCI2_RST_BIT, USBEHCI2_GATIING_BIT, USBPHY2_RST_BIT, USBPHY2_SCLK_GATING_BIT, 0, "USB2-Host"},
#endif
#ifdef SUNXI_EHCI3_BASE
	{SUNXI_EHCI3_BASE, USBEHCI3_RST_BIT, USBEHCI3_GATIING_BIT, USBPHY3_RST_BIT, USBPHY3_SCLK_GATING_BIT, 0, "USB3-Host"},
#endif
};

/* This soc platform needs to operate phy pll,
 * so an operation interface needs to be added here. */
#if IS_ENABLED(CONFIG_MACH_SUN300IW1)
__u32 usb_new_phyx_tp_read(__u32 regs, __u32 addr, __u32 len)
{
	int temp, i, j, ret = 0;

	temp = USB_Readb(regs + USB_REG_o_PHYCTL);
	temp |= (0x1 << 1);
	USB_Writeb(temp, regs + USB_REG_o_PHYCTL);

	for (j = len; j > 0; j--) {
		USB_Writeb((addr + j - 1), regs + USB_REG_o_PHYCTL + 1);

		for (i = 0; i < 0x4; i++)
			;

		temp = USB_Readb(regs + USB_REG_o_PHYSTATUS);
		ret <<= 1;
		ret |= (temp & 0x1);
	}

	temp = USB_Readb(regs + USB_REG_o_PHYCTL);
	temp &= ~(0x1 << 1);
	USB_Writeb(temp, regs + USB_REG_o_PHYCTL);

	return ret;
}

int usb_new_phyx_tp_write(__u32 regs, int addr, int data, int len)
{
	int temp, j, dtmp = 0;

	/* device: 0x410(phy_ctl) */
	dtmp = data;

	for (j = 0; j < len; j++) {

		temp = USB_Readb(regs + USB_REG_o_PHYCTL);
		temp |= (0x1 << 1);
		USB_Writeb(temp, regs + USB_REG_o_PHYCTL);

		USB_Writeb(addr + j, regs + USB_REG_o_PHYCTL + 1);

		temp = USB_Readb(regs + USB_REG_o_PHYCTL);
		temp &= ~(0x1 << 0);
		USB_Writeb(temp, regs + USB_REG_o_PHYCTL);

		temp = USB_Readb(regs + USB_REG_o_PHYCTL);
		temp &= ~(0x1 << 7);
		temp |= (dtmp & 0x1) << 7;
		USB_Writeb(temp, regs + USB_REG_o_PHYCTL);

		temp |= (0x1 << 0);
		USB_Writeb(temp, regs + USB_REG_o_PHYCTL);

		temp &= ~(0x1 << 0);
		USB_Writeb(temp, regs + USB_REG_o_PHYCTL);

		temp = USB_Readb(regs + USB_REG_o_PHYCTL);
		temp &= ~(0x1 << 1);
		USB_Writeb(temp, regs + USB_REG_o_PHYCTL);

		dtmp >>= 1;
	}

	return 0;
}
#endif

/*Port:port+num<fun><pull><drv><data>*/
ulong config_usb_pin(int index)
{

	if (index == 0)
		usb_drv_vbus_gpio = sunxi_name_to_gpio(CONFIG_USB0_VBUS_PIN);
	else if (index == 1)
		usb_drv_vbus_gpio = sunxi_name_to_gpio(CONFIG_USB1_VBUS_PIN);
	else if (index == 2)
		usb_drv_vbus_gpio = sunxi_name_to_gpio(CONFIG_USB2_VBUS_PIN);
	else
		usb_drv_vbus_gpio = sunxi_name_to_gpio(CONFIG_USB3_VBUS_PIN);

	if (usb_drv_vbus_gpio == -1)
		return 0;
	/*usbc0 port:PB0<0><1><default><default>*/
	/*usbc1 port:PB1<0><1><default><default>*/
	sunxi_gpio_set_cfgpin(usb_drv_vbus_gpio, 0);
	sunxi_gpio_set_pull(usb_drv_vbus_gpio, SUNXI_GPIO_PULL_UP);

	/* set cfg, ouput */
	sunxi_gpio_set_cfgpin(usb_drv_vbus_gpio, 1);

	/* reserved is pull down */
	sunxi_gpio_set_pull(usb_drv_vbus_gpio, 2);

	return 0;
}

void sunxi_set_vbus(int onoff)
{
	if (usb_drv_vbus_gpio == -1)
		return;

	gpio_set_value(usb_drv_vbus_gpio, onoff);
	return;
}

s32 __attribute__((weak)) axp_usb_vbus_output(void){ return 0;}

int alloc_pin(int index)
{
	if (axp_usb_vbus_output())
		return 0;
	config_usb_pin(index);
	return 0;
}

void free_pin(void)
{
	return;
}

u32 open_usb_clock(int index)
{
	u32 reg_value = 0;

	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	/* Bus reset and gating for ehci */
#if IS_ENABLED(CONFIG_MACH_SUN60IW2)
	if (index == 0) {
		reg_value = readl(&ccm->usb0_bgr_reg);
		reg_value |= (1 << ehci_cfg[index].bus_soft_reset_ofs);
		reg_value |= (1 << ehci_cfg[index].bus_clk_gating_ofs);
		writel(reg_value, &ccm->usb0_bgr_reg);
	} else {
		reg_value = readl(&ccm->usb1_bgr_reg);
		reg_value |= (1 << ehci_cfg[index].bus_soft_reset_ofs);
		reg_value |= (1 << ehci_cfg[index].bus_clk_gating_ofs);
		writel(reg_value, &ccm->usb1_bgr_reg);
	}
#elif IS_ENABLED(CONFIG_MACH_SUN55IW3)
	reg_value = readl(&ccm->usb_bgr_reg);
	reg_value |= (1 << ehci_cfg[index].bus_soft_reset_ofs);
	reg_value |= (1 << ehci_cfg[index].bus_clk_gating_ofs);
	writel(reg_value, &ccm->usb_bgr_reg);
#elif IS_ENABLED(CONFIG_MACH_SUN8IW11)
	reg_value = readl(&ccm->ahb_reset0_cfg);
	reg_value |= (1 << ehci_cfg[index].bus_soft_reset_ofs);
	writel(reg_value, &ccm->ahb_reset0_cfg);

	reg_value = readl(&ccm->ahb_gate0);
	reg_value |= (1 << ehci_cfg[index].bus_clk_gating_ofs);
	writel(reg_value, &ccm->ahb_gate0);
#elif defined(CONFIG_MACH_SUN300IW1)
	reg_value = readl(&ccm->ccu_app_clk_reg);
	reg_value |= (1 << 3);
	writel(reg_value, &ccm->ccu_app_clk_reg);

	reg_value = readl(&ccm->bus_clk_gating0_reg);
	reg_value |= (1 << 21);
	reg_value |= (1 << 19);
	writel(reg_value, &ccm->bus_clk_gating0_reg);

	reg_value = readl(&ccm->bus_clk_gating1_reg);
	reg_value |= (1 << 14);
	writel(reg_value, &ccm->bus_clk_gating1_reg);

	reg_value = readl(&ccm->bus_reset0_reg);
	reg_value |= (1 << 21);
	reg_value |= (1 << 19);
	writel(reg_value, &ccm->bus_reset0_reg);
#else
	reg_value = readl(&ccm->usb_gate_reset);
	reg_value |= (1 << ehci_cfg[index].bus_soft_reset_ofs);
	reg_value |= (1 << ehci_cfg[index].bus_clk_gating_ofs);
	writel(reg_value, &ccm->usb_gate_reset);
#endif


#if defined(CONFIG_MACH_SUN50IW9)
	reg_value = readl(&ccm->usb_gate_reset);
	reg_value |= (1 << ehci_cfg[2].bus_soft_reset_ofs);
	reg_value |= (1 << ehci_cfg[2].bus_clk_gating_ofs);
	writel(reg_value, &ccm->usb_gate_reset);

	reg_value = readl(&ccm->usb2_clk_cfg);
	reg_value |= (1 << ehci_cfg[2].phy_slk_gatimg_ofs);
	reg_value |= (1 << ehci_cfg[2].phy_reset_ofs);
	writel(reg_value, &ccm->usb2_clk_cfg);

	reg_value = readl(ehci_cfg[2].ehci_base + SUNXI_USB_PHY_CTRL);
	reg_value &= ~(0x01 << 3);
	writel(reg_value, (ehci_cfg[2].ehci_base + SUNXI_USB_PHY_CTRL));
#endif

#if defined(CONFIG_MACH_SUN50IW10)
	reg_value = readl(&ccm->usb_gate_reset);
	reg_value |= (1 << ehci_cfg[1].bus_soft_reset_ofs);
	reg_value |= (1 << ehci_cfg[1].bus_clk_gating_ofs);
	writel(reg_value, &ccm->usb_gate_reset);

	reg_value = readl(&ccm->usb1_clk_cfg);
	reg_value |= (1 << ehci_cfg[1].phy_slk_gatimg_ofs);
	reg_value |= (1 << ehci_cfg[1].phy_reset_ofs);
	writel(reg_value, &ccm->usb1_clk_cfg);

	reg_value = readl(ehci_cfg[1].ehci_base + SUNXI_USB_PHY_CTRL);
	reg_value &= ~(0x01 << 3);
	writel(reg_value, (ehci_cfg[1].ehci_base + SUNXI_USB_PHY_CTRL));
#endif
	/* open clk for usb phy */
	if (index == 0) {
#if IS_ENABLED(CONFIG_MACH_SUN55IW3) || IS_ENABLED(CONFIG_MACH_SUN60IW2)
		reg_value = readl(&ccm->usb0_clk_reg);
		reg_value |= (1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value |= (1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb0_clk_reg);
#elif IS_ENABLED(CONFIG_MACH_SUN8IW11)
		reg_value = readl(&ccm->usb_clk_cfg);
		reg_value |= (1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value |= (1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb_clk_cfg);
#elif defined(CONFIG_MACH_SUN300IW1)
		reg_value = readl(&ccm->bus_reset0_reg);
		reg_value |= (1 << 23);
		writel(reg_value, &ccm->bus_reset0_reg);
#else
		reg_value = readl(&ccm->usb0_clk_cfg);
		reg_value |= (1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value |= (1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb0_clk_cfg);
#endif

	} else if (index == 1) {
#if IS_ENABLED(CONFIG_MACH_SUN55IW3) || IS_ENABLED(CONFIG_MACH_SUN60IW2)
		reg_value = readl(&ccm->usb1_clk_reg);
		reg_value |= (1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value |= (1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb1_clk_reg);
#elif IS_ENABLED(CONFIG_MACH_SUN8IW11)
		reg_value = readl(&ccm->usb_clk_cfg);
		reg_value |= (1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value |= (1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb_clk_cfg);
#else
#if !IS_ENABLED(CONFIG_MACH_SUN300IW1)
		reg_value = readl(&ccm->usb1_clk_cfg);
		reg_value |= (1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value |= (1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb1_clk_cfg);
#endif
#endif
	} else if (index == 2) {
#if defined(CONFIG_MACH_SUN50IW9)
		/*noting to do*/
#else
#ifdef SUNXI_EHCI2_BASE
#if IS_ENABLED(CONFIG_MACH_SUN8IW11)
		reg_value = readl(&ccm->usb_clk_cfg);
		reg_value |= (1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value |= (1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb_clk_cfg);

#else
		reg_value = readl(&ccm->usb2_clk_cfg);
		reg_value |= (1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value |= (1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb2_clk_cfg);
#endif
#endif
#endif
	} else {
#ifdef SUNXI_EHCI3_BASE
#if IS_ENABLED(CONFIG_MACH_SUN8IW11)
		reg_value = readl(&ccm->usb_clk_cfg);
		reg_value |= (1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value |= (1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb_clk_cfg);

#else
		reg_value = readl(&ccm->usb3_clk_cfg);
		reg_value |= (1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value |= (1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb3_clk_cfg);;
#endif
#endif
	}
	printf("config usb clk ok\n");

	/* For 24Mhz crystal oscillator, you need to modify the phy pll
	 * configuration, please refer to the spec.
	 */
#if IS_ENABLED(CONFIG_MACH_SUN300IW1)
	if (24 == aw_get_hosc_freq())
		usb_new_phyx_tp_write(SUNXI_USBOTG_BASE, 0xb, 20, 0x8);
#endif

	return 0;
}

u32 close_usb_clock(int index)
{
	u32 reg_value = 0;
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	/* Bus reset and gating for ehci */
#if IS_ENABLED(CONFIG_MACH_SUN60IW2)
	if (index == 0) {
		reg_value = readl(&ccm->usb0_bgr_reg);
		reg_value &= ~(1 << ehci_cfg[index].bus_soft_reset_ofs);
		reg_value &= ~(1 << ehci_cfg[index].bus_clk_gating_ofs);
		writel(reg_value, &ccm->usb0_bgr_reg);
	} else {
		reg_value = readl(&ccm->usb1_bgr_reg);
		reg_value &= ~(1 << ehci_cfg[index].bus_soft_reset_ofs);
		reg_value &= ~(1 << ehci_cfg[index].bus_clk_gating_ofs);
		writel(reg_value, &ccm->usb1_bgr_reg);
	}
#elif IS_ENABLED(CONFIG_MACH_SUN55IW3)
	reg_value = readl(&ccm->usb_bgr_reg);
	reg_value &= ~(1 << ehci_cfg[index].bus_soft_reset_ofs);
	reg_value &= ~(1 << ehci_cfg[index].bus_clk_gating_ofs);
	writel(reg_value, &ccm->usb_bgr_reg);
#elif IS_ENABLED(CONFIG_MACH_SUN8IW11)
	reg_value = readl(&ccm->ahb_reset0_cfg);
	reg_value &= ~(1 << ehci_cfg[index].bus_soft_reset_ofs);
	writel(reg_value, &ccm->ahb_reset0_cfg);

	reg_value = readl(&ccm->ahb_gate0);
	reg_value &= ~(1 << ehci_cfg[index].bus_clk_gating_ofs);
	writel(reg_value, &ccm->ahb_gate0);
#elif defined(CONFIG_MACH_SUN300IW1)
	reg_value = readl(&ccm->ccu_app_clk_reg);
	reg_value &= ~(1 << 3);
	writel(reg_value, &ccm->ccu_app_clk_reg);

	reg_value = readl(&ccm->bus_clk_gating0_reg);
	reg_value &= ~(1 << 21);
	reg_value &= ~(1 << 19);
	writel(reg_value, &ccm->bus_clk_gating0_reg);

	reg_value = readl(&ccm->bus_clk_gating1_reg);
	reg_value &= ~(1 << 14);
	writel(reg_value, &ccm->bus_clk_gating1_reg);

	reg_value = readl(&ccm->bus_reset0_reg);
	reg_value &= ~(1 << 21);
	reg_value &= ~(1 << 19);
	writel(reg_value, &ccm->bus_reset0_reg);
#else
	reg_value = readl(&ccm->usb_gate_reset);
	reg_value &= ~(1 << ehci_cfg[index].bus_soft_reset_ofs);
	reg_value &= ~(1 << ehci_cfg[index].bus_clk_gating_ofs);
	writel(reg_value, &ccm->usb_gate_reset);
#endif

	/* close clk for usb phy */
	if (index == 0) {
#if IS_ENABLED(CONFIG_MACH_SUN55IW3) || IS_ENABLED(CONFIG_MACH_SUN60IW2)
		reg_value = readl(&ccm->usb0_clk_reg);
		reg_value &= ~(1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value &= ~(1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb0_clk_reg);
#elif IS_ENABLED(CONFIG_MACH_SUN8IW11)
		reg_value = readl(&ccm->usb_clk_cfg);
		reg_value &= ~(1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value &= ~(1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb_clk_cfg);
#elif defined(CONFIG_MACH_SUN300IW1)
		reg_value = readl(&ccm->bus_reset0_reg);
		reg_value &= ~(1 << 23);
		writel(reg_value, &ccm->bus_reset0_reg);
#else
		reg_value = readl(&ccm->usb0_clk_cfg);
		reg_value &= ~(1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value &= ~(1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb0_clk_cfg);
#endif
	} else if (index == 1) {
#if IS_ENABLED(CONFIG_MACH_SUN55IW3) || IS_ENABLED(CONFIG_MACH_SUN60IW2)
		reg_value = readl(&ccm->usb1_clk_reg);
		reg_value &= ~(1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value &= ~(1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb1_clk_reg);
#elif IS_ENABLED(CONFIG_MACH_SUN8IW11)
		reg_value = readl(&ccm->usb_clk_cfg);
		reg_value &= ~(1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value &= ~(1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb_clk_cfg);
#else
#if !IS_ENABLED(CONFIG_MACH_SUN300IW1)
		reg_value = readl(&ccm->usb1_clk_cfg);
		reg_value &= ~(1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value &= ~(1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb1_clk_cfg);
#endif
#endif
	} else if (index == 2) {
#if defined(CONFIG_MACH_SUN50IW9)
		/*noting to do*/
#else
#ifdef SUNXI_EHCI2_BASE
#if IS_ENABLED(CONFIG_MACH_SUN8IW11)
		reg_value = readl(&ccm->usb_clk_cfg);
		reg_value &= ~(1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value &= ~(1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb_clk_cfg);
#else
		reg_value = readl(&ccm->usb2_clk_cfg);
		reg_value &= ~(1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value &= ~(1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb2_clk_cfg);
#endif
#endif
#endif
	} else {
#ifdef SUNXI_EHCI3_BASE
#if IS_ENABLED(CONFIG_MACH_SUN8IW11)
		reg_value = readl(&ccm->usb_clk_cfg);
		reg_value &= ~(1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value &= ~(1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb_clk_cfg);

#else
		reg_value = readl(&ccm->usb3_clk_cfg);
		reg_value &= ~(1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value &= ~(1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb3_clk_cfg);
#endif
#endif
	}

	return 0;
}


void usb_passby(int index, u32 enable)
{
	unsigned long reg_value = 0;
	uintptr_t ehci_vbase = ehci_cfg[index].ehci_base;

	if(ehci_cfg[index].usb0_support)
	{
		/* the default mode of usb0 is OTG,so change it here. */
		reg_value = readl((void *)(SUNXI_USBOTG_BASE + 0x420));
		reg_value &= ~(0x01);
		writel(reg_value, (void *)(SUNXI_USBOTG_BASE + 0x420));
	}
#if defined(CONFIG_MACH_SUN50IW11)
	if (index == 1) {
		reg_value = readl((void *)(SUNXI_USB1_BASE + 0x420));
		reg_value &= ~(0x01);
		writel(reg_value, (void *)(SUNXI_USB1_BASE + 0x420));
	}
#endif

	reg_value = readl((void *)(ehci_vbase + SUNXI_USB_PHY_CTRL));
#if IS_ENABLED(CONFIG_MACH_SUN8IW11)
	reg_value &= ~(0x01 << 1);
#else
	reg_value &= ~(0x01 << 3);
#endif
	writel(reg_value, (void *)(ehci_vbase + SUNXI_USB_PHY_CTRL));

	reg_value = readl((void *)(ehci_vbase + SUNXI_USB_CTRL));
	if (enable) {
		reg_value |= (1 << 10);		/* AHB Master interface INCR8 enable */
		reg_value |= (1 << 9);     	/* AHB Master interface burst type INCR4 enable */
		reg_value |= (1 << 8);     	/* AHB Master interface INCRX align enable */
		reg_value |= (1 << 0);     	/* ULPI bypass enable */
	} else if(!enable) {
		reg_value &= ~(1 << 10);	/* AHB Master interface INCR8 disable */
		reg_value &= ~(1 << 9);     /* AHB Master interface burst type INCR4 disable */
		reg_value &= ~(1 << 8);     /* AHB Master interface INCRX align disable */
		reg_value &= ~(1 << 0);     /* ULPI bypass disable */
	}
	writel(reg_value, (void *)(ehci_vbase + SUNXI_USB_CTRL));

	return;
}


int sunxi_start_ehci(int index)
{
#if IS_ENABLED(CONFIG_MACH_SUN55IW3) || IS_ENABLED(CONFIG_MACH_SUN60IW2)
	unsigned long reg_value = 0;
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
#endif

	if(alloc_pin(index))
		return -1;
	open_usb_clock(index);
#if IS_ENABLED(CONFIG_MACH_SUN55IW3) || IS_ENABLED(CONFIG_MACH_SUN60IW2)
#ifdef CONFIG_AIOT_DISP_PARAM_UPDATE
	pmu_set_reg_value(0x80, 0xf);
#endif

	if (index == 0) {
		reg_value = readl(&ccm->usb0_clk_reg);
		reg_value |= (1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value &= ~(1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb0_clk_reg);
		mdelay(1);
		reg_value = readl(&ccm->usb0_clk_reg);
		reg_value |= (1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb0_clk_reg);
	} else if (index == 1) {
		reg_value = readl(&ccm->usb1_clk_reg);
		reg_value |= (1 << ehci_cfg[index].phy_slk_gatimg_ofs);
		reg_value &= ~(1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb1_clk_reg);
		mdelay(1);
		reg_value = readl(&ccm->usb1_clk_reg);
		reg_value |= (1 << ehci_cfg[index].phy_reset_ofs);
		writel(reg_value, &ccm->usb1_clk_reg);
	}
#endif

	usb_passby(index, 1);
	sunxi_set_vbus(1);
	mdelay(800);
	return 0;
}

void sunxi_stop_ehci(int index)
{
	sunxi_set_vbus(0);
	usb_passby(index, 0);
	close_usb_clock(index);
	free_pin();
	return;
}

/*
 * Create the appropriate control structures to manage
 * a new EHCI host controller.
 */
int ehci_hcd_init(int index, enum usb_init_type init,
		struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
	printf("start sunxi  %s...\n", ehci_cfg[index].name);
	if(index > ARRAY_SIZE(ehci_cfg))
	{
		printf("the index is too large\n");
		return -1;
	}
	usb_vbase = ehci_cfg[index].ehci_base;
	if(sunxi_start_ehci(index))
	{
		return -1;
	}
	*hccr = (struct ehci_hccr *)usb_vbase;
	*hcor = (struct ehci_hcor *)((uintptr_t)(*hccr) +
		HC_LENGTH(ehci_readl(&((*hccr)->cr_capbase))));

	printf("sunxi %s init ok...\n", ehci_cfg[index].name);
	return 0;
}

/*
 * Destroy the appropriate control structures corresponding
 * the the EHCI host controller.
 */
int ehci_hcd_stop(int index)
{
	sunxi_stop_ehci(index);
	printf("stop sunxi %s ok...\n", ehci_cfg[index].name);
	return 0;
}
