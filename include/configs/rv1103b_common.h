/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2024 Rockchip Electronics Co., Ltd
 */

#ifndef __CONFIG_RV1103B_COMMON_H
#define __CONFIG_RV1103B_COMMON_H

#define CFG_CPUID_OFFSET		0xa

#include "rockchip-common.h"

#define CFG_IRAM_BASE			0x210f6000
#define CFG_SYS_SDRAM_BASE		0
#define SDRAM_MAX_SIZE			0x20000000

#define GICD_BASE			0x20411000
#define GICC_BASE			0x20412000

#define CFG_SYS_NONCACHED_MEMORY	(1 << 20)	/* 1 MiB */

/* SPL */
#define CFG_SPL_MAX_SIZE		0x40000
#define CFG_SPL_BSS_START_ADDR		0x03de0000
#define CFG_SPL_BSS_MAX_SIZE		0x20000
#define CFG_SPL_STACK			0x03de0000

/* secure otp */
#define OTP_UBOOT_ROLLBACK_OFFSET	0x350
#define OTP_UBOOT_ROLLBACK_WORDS	2	/* 64 bits, 2 words */
#define OTP_ALL_ONES_NUM_BITS		32
#define OTP_SECURE_BOOT_ENABLE_ADDR	0x20
#define OTP_SECURE_BOOT_ENABLE_SIZE	1
#define OTP_RSA_HASH_ADDR		0x180
#define OTP_RSA_HASH_SIZE		32

/* rockusb */
#define CONFIG_ROCKUSB_G_DNL_PID	0x110e

#ifndef ROCKCHIP_DEVICE_SETTINGS
#define ROCKCHIP_DEVICE_SETTINGS
#endif

#ifdef CONFIG_ENV_MEM_LAYOUT
#define ENV_MEM_LAYOUT_SETTINGS		CONFIG_ENV_MEM_LAYOUT_SETTINGS
#else
/*
 *   Image:  0 - 8M
 *  zImage:  8 - 12M
 *     fdt: 12 - 13M
 * ramdisk: 14 ...
 */
#define ENV_MEM_LAYOUT_SETTINGS		\
	"scriptaddr=0x00b00000\0"	\
	"pxefile_addr_r=0x00c00000\0"	\
	"fdt_addr_r=0x00c00000\0"	\
	"kernel_comp_addr_r=0x00808000\0"	\
	"kernel_addr_r=0x00008000\0"	\
	"ramdisk_addr_r=0x000e00000\0"
#endif

#ifndef CONFIG_XPL_BUILD
#define CFG_EXTRA_ENV_SETTINGS		\
	"fdtfile=" CONFIG_DEFAULT_FDT_FILE "\0"	\
	"partitions=" PARTS_DEFAULT	\
	ENV_MEM_LAYOUT_SETTINGS		\
	ROCKCHIP_DEVICE_SETTINGS	\
	"boot_targets=" BOOT_TARGETS "\0"
#endif

#endif /* __CONFIG_RV1103B_COMMON_H */
