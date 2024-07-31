/*
 * SPDX-License-Identifier:     GPL-2.0+
 *
 * (C) Copyright 2020 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <dwc3-uboot.h>
#include <usb.h>
#include <linux/usb/phy-rockchip-naneng-combphy.h>
#include <asm/io.h>
#include <rockusb.h>
#include <adc.h>
#include <dm.h>

#define SARADC_ADDR	"saradc@ffae0000"
#define HW_ID_CHANNEL	2
#define BOM_ID_CHANNEL	3

#define countof(x) (sizeof(x) / sizeof(x[0]))

struct variant_def {
	char *compatible;
	unsigned int hw_id_lower_bound;
	unsigned int hw_id_upper_bound;
	unsigned int bom_id_lower_bound;
	unsigned int bom_id_upper_bound;
	char *fdtfile;
};

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_USB_DWC3
#define CRU_BASE		0xff4a0000
#define CRU_SOFTRST_CON33	0x0a84
#define U3PHY_BASE		0xffdc0000

static struct dwc3_device dwc3_device_data = {
	.maximum_speed = USB_SPEED_SUPER,
	.base = 0xfe500000,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 0,
	.dis_u2_susphy_quirk = 1,
	.dis_u1u2_quirk = 1,
	.usb2_phyif_utmi_width = 16,
};

int usb_gadget_handle_interrupts(int index)
{
	dwc3_uboot_handle_interrupt(0);
	return 0;
}

bool rkusb_usb3_capable(void)
{
	return true;
}

static void usb_reset_otg_controller(void)
{
	writel(0x00020002, CRU_BASE + CRU_SOFTRST_CON33);
	mdelay(1);
	writel(0x00020000, CRU_BASE + CRU_SOFTRST_CON33);
	mdelay(1);
}

int board_usb_init(int index, enum usb_init_type init)
{
	u32 ret = 0;

	usb_reset_otg_controller();

#if defined(CONFIG_SUPPORT_USBPLUG)
	dwc3_device_data.maximum_speed = USB_SPEED_HIGH;

	if (rkusb_switch_usb3_enabled()) {
		dwc3_device_data.maximum_speed = USB_SPEED_SUPER;
		ret = rockchip_combphy_usb3_uboot_init(U3PHY_BASE);
		if (ret) {
			rkusb_force_to_usb2(true);
			dwc3_device_data.maximum_speed = USB_SPEED_HIGH;
		}
	}
#else
	ret = rockchip_combphy_usb3_uboot_init(U3PHY_BASE);
	if (ret) {
		rkusb_force_to_usb2(true);
		dwc3_device_data.maximum_speed = USB_SPEED_HIGH;
	}
#endif

	return dwc3_uboot_init(&dwc3_device_data);
}

#if defined(CONFIG_SUPPORT_USBPLUG)
int board_usb_cleanup(int index, enum usb_init_type init)
{
	dwc3_uboot_exit(index);
	return 0;
}
#endif

#endif

#ifdef CONFIG_ID_EEPROM
static struct variant_def variants[] = {
	{"radxa,rock-2a",   80, 250, 0, -1, "rockchip/rk3528-rock-2a.dtb"},
	{"radxa,rock-2a",   320, 380, 0, -1, "rockchip/rk3528-radxa-e20c.dtb"},
	{"radxa,rock-2a",   430, 490, 0, -1, "rockchip/rk3528-medge-io.dtb"},
	{"radxa,rock-2a",   530, 710, 0, -1, "rockchip/rk3528-rock-2f.dtb"},
};

static void set_fdtfile(void)
{
	int i, ret;
	unsigned int hw_id, bom_id;
	struct variant_def *v;

	ret = adc_channel_single_shot(SARADC_ADDR, HW_ID_CHANNEL, &hw_id);
	if (ret) {
		pr_err("%s: adc_channel_single_shot fail for HW_ID: %i!\n", __func__, ret);
		return;
	}
	ret = adc_channel_single_shot(SARADC_ADDR, BOM_ID_CHANNEL, &bom_id);
	if (ret) {
		pr_err("%s: adc_channel_single_shot fail for BOM_ID: %i!\n", __func__, ret);
		return;
	}
	for(i = 0; i < countof(variants); i++) {
		v = &variants[i];
		if (of_machine_is_compatible(v->compatible) &&
		    hw_id >= v->hw_id_lower_bound &&
		    hw_id <= v->hw_id_upper_bound &&
		    bom_id >= v->bom_id_lower_bound &&
		    bom_id <= v->bom_id_upper_bound) {
			printf("Override default fdtfile to %s\n", v->fdtfile);
			env_set("fdtfile", v->fdtfile);
			break;
		}
	}
}

/**
 * mac_read_from_eeprom() - read the MAC address & the serial number in EEPROM
 *
 * This function reads the MAC address and the serial number from EEPROM and
 * sets the appropriate environment variables for each one read.
 *
 * The environment variables are only set if they haven't been set already.
 * This ensures that any user-saved variables are never overwritten.
 *
 * If CONFIG_ID_EEPROM is enabled, this function will be called in
 * "static init_fnc_t init_sequence_r[]" of u-boot/common/board_r.c.
 */
int mac_read_from_eeprom(void)
{
	set_fdtfile();
	return 0;
}

int do_mac(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("This device does not support user programmable EEPROM.\n");
	return -1;
}
#endif
