/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration for MT8391 based boards
 *
 * Copyright (C) 2025 MediaTek Inc.
 * Author: Chris-QJ Chen <chris-qj.chen@mediatek.com>
 */

#ifndef __MT8391_H
#define __MT8391_H

#include <linux/sizes.h>

#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE	-4
#define CONFIG_SYS_NS16550_MEM32
#define CONFIG_SYS_NS16550_COM1		0x11002000
#define CONFIG_SYS_NS16550_CLK		26000000

#define GENIO_720_EVK_FIT_IMAGE_GUID \
	EFI_GUID(0x085dde70, 0x7863, 0x40f7, 0x86, 0x4f, \
		 0x7e, 0xd4, 0xca, 0x53, 0x21, 0x78)
#define GENIO_720_EVK_FIP_IMAGE_GUID \
	EFI_GUID(0x3dcbf27f, 0x9e63, 0x49fc, 0x93, 0x74, \
		 0x5a, 0x62, 0x47, 0x21, 0xad, 0x88)
#define GENIO_720_EVK_BL2_IMAGE_GUID \
	EFI_GUID(0x9ec54f9f, 0x077a, 0x4688, 0xbd, 0xbb, \
		 0xb0, 0xc9, 0xd6, 0x59, 0x47, 0x78)
#define GENIO_720_EVK_FW_IMAGE_GUID \
	EFI_GUID(0x94089703, 0xe6c2, 0x4526, 0xb3, 0xe3, \
		 0x2d, 0x76, 0xa6, 0x5d, 0x0e, 0xc9)
#define GENIO_720_EVK_ENV_IMAGE_GUID \
	EFI_GUID(0x1c56492e, 0xbe13, 0x4e8f, 0x83, 0xac, \
		 0x45, 0x0a, 0x0b, 0xba, 0xb5, 0x48)

#define GENIO_720_EVK_UFS_FIT_IMAGE_GUID \
	EFI_GUID(0x1fb6aa93, 0xe883, 0x47fb, 0xa1, 0x0e, \
		 0xd0, 0x9e, 0x81, 0xbf, 0x93, 0x54)
#define GENIO_720_EVK_UFS_FIP_IMAGE_GUID \
	EFI_GUID(0xb873ea46, 0x345f, 0x46ec, 0xac, 0x13, \
		 0xef, 0x44, 0x2c, 0xca, 0x1f, 0xfe)
#define GENIO_720_EVK_UFS_BL2_IMAGE_GUID \
	EFI_GUID(0xb89f1a70, 0x400d, 0x4b49, 0x82, 0x9f, \
		 0x90, 0x83, 0xa1, 0x47, 0xc1, 0xfd)
#define GENIO_720_EVK_UFS_FW_IMAGE_GUID \
	EFI_GUID(0x81d8203c, 0x582b, 0x4bac, 0xb8, 0x08, \
		 0x16, 0x94, 0xa8, 0x95, 0x82, 0xff)
#define GENIO_720_EVK_UFS_ENV_IMAGE_GUID \
	EFI_GUID(0x9d129b09, 0x80b8, 0x477f, 0x98, 0x45, \
		 0xd5, 0x5b, 0xd1, 0x53, 0x05, 0x3c)

#define GENIO_720_EVK_UFS_QSPI_FIT_IMAGE_GUID \
	EFI_GUID(0xf5bf4687, 0xefe8, 0x4054, 0xb4, 0x77, \
		 0x27, 0x9a, 0x0c, 0xdf, 0xa4, 0xee)
#define GENIO_720_EVK_UFS_QSPI_FIP_IMAGE_GUID \
	EFI_GUID(0xa6f74c75, 0xd3ad, 0x40a6, 0xb2, 0x95, \
		 0xec, 0x8d, 0xda, 0x8f, 0xdc, 0x51)
#define GENIO_720_EVK_UFS_QSPI_BL2_IMAGE_GUID \
	EFI_GUID(0xe6a5f66c, 0x4bec, 0x41d1, 0xbe, 0x27, \
		 0x35, 0x0d, 0xa2, 0xd1, 0xae, 0x6b)
#define GENIO_720_EVK_UFS_QSPI_FW_IMAGE_GUID \
	EFI_GUID(0x9e2abdfe, 0xf3ee, 0x41a1, 0x96, 0x83, \
		 0xb8, 0xdb, 0x50, 0x9e, 0x9e, 0x6a)
#define GENIO_720_EVK_UFS_QSPI_ENV_IMAGE_GUID \
	EFI_GUID(0x9539d35a, 0xb4bf, 0x4479, 0x80, 0xec, \
		 0x13, 0x53, 0x4f, 0x1e, 0x30, 0xe6)

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
#endif

#ifdef CONFIG_ARM64
#define MTK_SIP_PLAT_BINFO		0xC2000529
#endif

/* DRAM */
#define CONFIG_SYS_SDRAM_BASE		0x40000000

#endif
