/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration for MT8188 based boards
 *
 * Copyright (C) 2022 MediaTek Inc.
 * Author: Macpaul Lin <macpaul.lin@mediatek.com>
 */

#ifndef __MT8188_H
#define __MT8188_H

#include <linux/sizes.h>

#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE	-4
#define CONFIG_SYS_NS16550_MEM32
#define CONFIG_SYS_NS16550_COM1		0x11002000
#define CONFIG_SYS_NS16550_CLK		26000000

#define GENIO_510_EVK_FIT_IMAGE_GUID \
	EFI_GUID(0x458ae454, 0xb228, 0x49eb, 0x80, 0xfc, \
		 0x5c, 0x68, 0x7f, 0x96, 0xc7, 0xc8)
#define GENIO_510_EVK_FIP_IMAGE_GUID \
	EFI_GUID(0x0cb9a4dd, 0x8692, 0x425d, 0xa1, 0xdc, \
		 0xa3, 0x3b, 0xcf, 0x52, 0xad, 0x05)
#define GENIO_510_EVK_BL2_IMAGE_GUID \
	EFI_GUID(0x546af9d8, 0x91e7, 0x4de0, 0xa8, 0xa3, \
		 0x35, 0x1b, 0xf1, 0x56, 0x9d, 0x64)
#define GENIO_510_EVK_FW_IMAGE_GUID \
	EFI_GUID(0x5be6e67f, 0x8cec, 0x43f7, 0xb4, 0xe3, \
		 0x01, 0x33, 0x49, 0xdc, 0x49, 0xad)
#define GENIO_510_EVK_ENV_IMAGE_GUID \
	EFI_GUID(0xf967dfc1, 0xbb75, 0x4439, 0x98, 0x8f, \
		 0xab, 0xe6, 0xc0, 0xa2, 0xd0, 0x19)

#define GENIO_510_EVK_QSPI_FIT_IMAGE_GUID \
	EFI_GUID(0xd7d70c1b, 0x3d55, 0x45f9, 0xa1, 0xca, \
		 0xcf, 0xc8, 0x57, 0xe2, 0xe2, 0x8d)
#define GENIO_510_EVK_QSPI_FIP_IMAGE_GUID \
	EFI_GUID(0x333cffd3, 0x68f7, 0x4ce0, 0xbd, 0xfb, \
		 0xe1, 0x87, 0xbe, 0x42, 0xee, 0xdd)
#define GENIO_510_EVK_QSPI_BL2_IMAGE_GUID \
	EFI_GUID(0x4889d6c8, 0x7986, 0x469f, 0xbd, 0xcb, \
		 0xe6, 0xd3, 0x03, 0xb5, 0x6c, 0xbd)
#define GENIO_510_EVK_QSPI_FW_IMAGE_GUID \
	EFI_GUID(0x865e7ce9, 0x21ab, 0x4e24, 0x96, 0x37, \
		 0xa5, 0x84, 0x64, 0xde, 0x79, 0xfc)
#define GENIO_510_EVK_QSPI_ENV_IMAGE_GUID \
	EFI_GUID(0x602d83ac, 0xcb13, 0x43ce, 0xb7, 0xbe, \
		 0x4e, 0xab, 0x07, 0x5c, 0x98, 0xd1)

/* Environment settings */
#include <config_distro_bootcmd.h>

#ifdef CONFIG_CMD_MMC
#define BOOT_TARGET_MMC(func) \
	func(MMC, mmc, 0) \
	func(MMC, mmc, 1)
#else
#define BOOT_TARGET_MMC(func)
#endif

#ifdef CONFIG_CMD_USB
#define BOOT_TARGET_USB(func) func(USB, usb, 0)
#else
#define BOOT_TARGET_USB(func)
#endif

#ifdef CONFIG_CMD_SCSI
#define BOOT_TARGET_SCSI(func) func(SCSI, scsi, 2)
#else
#define BOOT_TARGET_SCSI(func)
#endif

#define BOOT_TARGET_DEVICES(func) \
	BOOT_TARGET_MMC(func) \
	BOOT_TARGET_USB(func) \
	BOOT_TARGET_SCSI(func)

#if !defined(CONFIG_EXTRA_ENV_SETTINGS)
#if !IS_ENABLED(CONFIG_SPI_FLASH)
#define CONFIG_EXTRA_ENV_SETTINGS \
	"scriptaddr=0x40000000\0" \
	"fdt_addr_r=0x44000000\0" \
	"fdtoverlay_addr_r=0x44c00000\0" \
	"fdt_resize=0x3000\0" \
	"kernel_addr_r=0x45000000\0" \
	"ramdisk_addr_r=0x49000000\0" \
	"fdtfile=" CONFIG_DEFAULT_FDT_FILE ".dtb\0" \
	"splashimage=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" \
	"splashsource=mmc_fs\0" \
	"splashfile=logo.bmp\0" \
	"splashdevpart=0#bootassets\0" \
	"splashpos=m,m\0" \
	BOOTENV
#else
#define CONFIG_EXTRA_ENV_SETTINGS \
	"scriptaddr=0x40000000\0" \
	"fdt_addr_r=0x44000000\0" \
	"fdtoverlay_addr_r=0x44c00000\0" \
	"fdt_resize=0x3000\0" \
	"kernel_addr_r=0x45000000\0" \
	"ramdisk_addr_r=0x49000000\0" \
	"fdtfile=" CONFIG_DEFAULT_FDT_FILE ".dtb\0" \
	"splashimage=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" \
	"splashsource=sf\0" \
	"splashpos=m,m\0" \
	BOOTENV
#endif
#endif

#ifdef CONFIG_ARM64
#define MTK_SIP_PLAT_BINFO		0xC2000529
#endif

#endif
