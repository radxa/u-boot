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


#define GENIO_700_EVK_FIT_IMAGE_GUID \
	EFI_GUID(0x40ecb7c9, 0x416e, 0x4524, 0xb4, 0x95, \
		 0xa9, 0x67, 0xe0, 0x69, 0x9c, 0xb1)

#define GENIO_700_EVK_FIP_IMAGE_GUID \
	EFI_GUID(0x551cc92d, 0x9a22, 0x4dc8, 0x88, 0x72, \
		 0xdb, 0x1b, 0xb8, 0xdc, 0xaf, 0xa0)

#define GENIO_700_EVK_BL2_IMAGE_GUID \
	EFI_GUID(0xa2c34f52, 0x9452, 0x4f6b, 0x83, 0xdb, \
		 0xfd, 0xe2, 0x18, 0x53, 0xe2, 0xa5)

#define GENIO_700_EVK_FW_IMAGE_GUID \
	EFI_GUID(0x87eb9b05, 0x582f, 0x4e68, 0xa5, 0xec, \
		 0x7c, 0x24, 0x54, 0xff, 0xc1, 0xf6)

#define GENIO_700_EVK_ENV_IMAGE_GUID \
	EFI_GUID(0x1480544a, 0x3e06, 0x4b0c, 0xba, 0x67, \
		 0x87, 0xe3, 0xaa, 0xd5, 0x8f, 0xda)

#define GENIO_700_EVK_QSPI_FIT_IMAGE_GUID \
	EFI_GUID(0xee2f059d, 0xbb3c, 0x4f2f, 0x92, 0xf5, \
		 0xc8, 0xea, 0x4c, 0x23, 0xe7, 0x6c)

#define GENIO_700_EVK_QSPI_FIP_IMAGE_GUID \
	EFI_GUID(0x2b8e5b72, 0xc6ea, 0x49fa, 0x98, 0xa9, \
		 0x5d, 0x9e, 0x0f, 0x4c, 0x74, 0xfa)

#define GENIO_700_EVK_QSPI_BL2_IMAGE_GUID \
	EFI_GUID(0x6acd6552, 0xf05d, 0x4d47, 0xbd, 0xe2, \
		 0xb8, 0x4c, 0x60, 0x12, 0x9a, 0xd0)

#define GENIO_700_EVK_QSPI_FW_IMAGE_GUID \
	EFI_GUID(0x0cb43ed1, 0x4a37, 0x4f00, 0x9d, 0x93, \
		 0x44, 0x99, 0xe9, 0x3b, 0xbf, 0xd2)

#define GENIO_700_EVK_QSPI_ENV_IMAGE_GUID \
	EFI_GUID(0x636ed10f, 0x528c, 0x4583, 0x92, 0xec, \
		 0xbc, 0x6d, 0xa3, 0x2d, 0xe9, 0x0a)

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
