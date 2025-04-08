// SPDX-License-Identifier: GPL-2.0+
/*
 * sunxi flash map
 *
 * Copyright (C) 2022
 * 2022.04.27 huangrongcun <huangrongcun@allwinnertech.com>
 */

#ifndef __SUNXI_FLASHMAP_H__
#define __SUNXI_FLASHMAP_H__
#include <common.h>
#include <sys_config.h>
#include <mmc.h>
#include <sunxi_flash.h>
#include "fdtdec.h"

enum flash_type{
	FLASHMAP_SPI_NOR,
	FLASHMAP_SDMMC,
	FLASHMAP_SPI_NAND,
#ifdef CONFIG_SUNXI_UFS
	FLASHMAP_UFS,
#endif
	FLASHMAP_TYPE_COUNT
};

enum region_flash_type {
	BOOT_PARAM,
	TOC1,
	TOC1_BAK,
	SEC_STORAGE,
	MMC_TUNING,
	BOOT_PARAM_COMMON,
	FLASHMAP_MEM_COUNT
};


enum logicoffset_type {
	FLASHMAP_LOGIC_OFFSET,
	FLASHMAP_SECLOGIC_OFFSET,
	FLASHMAP_LOGICTYPE_COUNT,
};

enum region_logicoffset_type {
	LINUX_LOGIC_OFFSET,
	RTOS_LOGIC_OFFSET,
	REGION_LOGICTYPE_COUNT,
};

int sunxi_flashmap_init(void);
extern void sunxi_flashmap_info_dump(void);

int sunxi_flashmap_logical_offset(enum flash_type flash_name, enum region_logicoffset_type region_name);
extern int sunxi_flashmap_offset(enum flash_type flash_name, enum region_flash_type region_name);
extern int sunxi_flashmap_size(enum flash_type flash_name, enum region_flash_type region_name);

#endif
