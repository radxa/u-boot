/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2021 Rockchip Electronics Co., Ltd
 */

#ifndef __CONFIG_RK3568_COMMON_H
#define __CONFIG_RK3568_COMMON_H

#define CFG_CPUID_OFFSET		0xa

#include "rockchip-common.h"

#define CFG_IRAM_BASE			0xfdcc0000
#define CFG_SYS_SDRAM_BASE		0
#define SDRAM_MAX_SIZE			0xf0000000

/* Reserve low memory for secure firmware during SPL kernel boot. */
#define SPL_RESV_MEM_SIZE		SZ_2M

#define GICD_BASE			0xfd400000
#define GICR_BASE			0xfd460000
#define GICC_BASE			0xfd800000
#define CONFIG_LIB_HW_RAND
#define CONFIG_SYS_NONCACHED_MEMORY	(1 << 20)	/* 1 MiB */

/* secure otp */
#define OTP_UBOOT_ROLLBACK_OFFSET	0xe0
#define OTP_UBOOT_ROLLBACK_WORDS	2	/* 64 bits, 2 words */
#define OTP_ALL_ONES_NUM_BITS		32
#define OTP_SECURE_BOOT_ENABLE_ADDR	0x80
#define OTP_SECURE_BOOT_ENABLE_SIZE	2
#define OTP_RSA_HASH_ADDR		0x90
#define OTP_RSA_HASH_SIZE		32

#define CONFIG_ROCKUSB_G_DNL_PID	0x350a
#define ROCKUSB_FSG_BUFLEN		0x400000

#ifndef ROCKCHIP_DEVICE_SETTINGS
#define ROCKCHIP_DEVICE_SETTINGS
#endif

#define ENV_MEM_LAYOUT_SETTINGS		\
	"scriptaddr=0x00c00000\0"	\
	"script_offset_f=0xffe000\0"	\
	"script_size_f=0x2000\0"	\
	"pxefile_addr_r=0x00e00000\0"	\
	"kernel_addr_r=0x02000000\0"	\
	"kernel_comp_addr_r=0x0a000000\0"	\
	"fdt_addr_r=0x12000000\0"	\
	"fdtoverlay_addr_r=0x12100000\0"	\
	"ramdisk_addr_r=0x12180000\0"	\
	"kernel_comp_size=0x8000000\0"

#define CFG_EXTRA_ENV_SETTINGS		\
	"fdtfile=" CONFIG_DEFAULT_FDT_FILE "\0"	\
	"partitions=" PARTS_DEFAULT	\
	ENV_MEM_LAYOUT_SETTINGS		\
	ROCKCHIP_DEVICE_SETTINGS	\
	"boot_targets=" BOOT_TARGETS "\0"
#endif
