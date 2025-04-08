// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2022-2024 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : mmc_flashmap.h
 * Description :
 * History :
 *    Author  :  <huangrongcun@allwinnertech.com>
 *    Date    : 2022/04/07
 *    Comment : mmc flash map
 */

#include <sunxi_flashmap.h>
#include "../mmc/mmc_def.h"
#include "../ufs/sunxi_ufs_map.h"
#include <sunxi_board.h>
#include <fdt_support.h>
#include <boot_param.h>
/*
 * The working_fdt points to our working flattened device tree.
 */

#define FLASHMAP_DEBUG 0

#ifdef CONFIG_SPINOR_LOGICAL_OFFSET
#define DEFAULT_SPINOR_LOGICAL_OFFSET CONFIG_SPINOR_LOGICAL_OFFSET
#else
#define DEFAULT_SPINOR_LOGICAL_OFFSET (2016)
#endif

#ifdef CONFIG_SPINOR_LOGICAL_SECURE_OFFSET
#define DEFAULT_SPINOR_LOGICAL_SECURE_OFFSET CONFIG_SPINOR_LOGICAL_SECURE_OFFSET
#else
#define DEFAULT_SPINOR_LOGICAL_SECURE_OFFSET (2200)
#endif

#ifdef CONFIG_SPINOR_LOGICAL_OFFSET
#define DEFAULT_SPINOR_UBOOT_OFFSET CONFIG_SPINOR_UBOOT_OFFSET
#else
#define DEFAULT_SPINOR_UBOOT_OFFSET (128)
#endif

#ifdef CONFIG_MMC_LOGICAL_OFFSET
#define DEFAULT_MMC_LOGICAL_OFFSET CONFIG_MMC_LOGICAL_OFFSET
#else
#define DEFAULT_MMC_LOGICAL_OFFSET (40960)
#endif

#ifdef CONFIG_UFS_LOGICAL_OFFSET
#define DEFAULT_UFS_LOGICAL_OFFSET CONFIG_UFS_LOGICAL_OFFSET
#else
#define DEFAULT_UFS_LOGICAL_OFFSET (40960)
#endif


#ifdef CONFIG_SPINAND_LOGICAL_OFFSET
#define DEFAULT_SPINAND_UBOOT_OFFSET CONFIG_SPINAND_LOGICAL_OFFSET
#else
#define DEFAULT_SPINAND_UBOOT_OFFSET (0x100000)
#endif


#ifdef CONFIG_SUNXI_RTOS_LOGICAL_OFFSET
#define DEFAULT_SUNXI_RTOS_LOGICAL_OFFSET CONFIG_SUNXI_RTOS_LOGICAL_OFFSET
#else
#define DEFAULT_SUNXI_RTOS_LOGICAL_OFFSET (-1)
#endif

#define MMC_BOOT_PARAM_COMMON_OFFSET 8
#define DEFAULT_MMC_BOOTPARAM_COMMOM_OFFSET                                    \
	(SUNXI_SDMMC_PARAMETER_REGION_LBA_START - MMC_BOOT_PARAM_COMMON_OFFSET)
#define DEFAULT_MMC_BOOTPARAM_COMMOM_SIZE (BOOT_PARAM_SIZE / 512)

#define UFS_BOOT_PARAM_COMMON_OFFSET 8
#define DEFAULT_UFS_BOOTPARAM_COMMOM_OFFSET                                    \
	(SUNXI_UFS_PARAMETER_REGION_LBA_START - UFS_BOOT_PARAM_COMMON_OFFSET)
#define DEFAULT_UFS_BOOTPARAM_COMMOM_SIZE (BOOT_PARAM_SIZE / 512)


#if FLASHMAP_DEBUG
#define flashmap_debug(fmt, arg...)                                            \
	printf("%s()%d - " fmt, __func__, __LINE__, ##arg)
#else
#define flashmap_debug(fmt, arg...)
#endif

struct _flashmap_member {
	u32 start;
	u32 size;
};

struct _logicoffset_member {
	u32 logicoffset;
	u32 secure_logicoffset;
};

struct flashmap_info {
	struct _logicoffset_member logic[REGION_LOGICTYPE_COUNT];
	struct _flashmap_member maps[FLASHMAP_MEM_COUNT];
} global_flashmap[FLASHMAP_TYPE_COUNT];

enum _flashmap_member_type {
	FLASHMAP_MEM_START,
	FLASHMAP_MEM_SIZE,
	FLASHMAP_MAX_MEM
};

char *fdt_path[FLASHMAP_TYPE_COUNT] = {
	[FLASHMAP_SPI_NOR] = "/soc/sunxi_flashmap/nor_map",
	[FLASHMAP_SDMMC] = "/soc/sunxi_flashmap/sdmmc_map"
};

char *flash_name[FLASHMAP_TYPE_COUNT] = {
	[FLASHMAP_SPI_NOR] = "SPINOR",
	[FLASHMAP_SDMMC] = "SDMMC",
	[FLASHMAP_SPI_NAND] = "SPINAND"
};

char *fdt_logicoffset_name[REGION_LOGICTYPE_COUNT][FLASHMAP_LOGICTYPE_COUNT] = {
	[LINUX_LOGIC_OFFSET][FLASHMAP_LOGIC_OFFSET] = "logic_offset",
	[LINUX_LOGIC_OFFSET][FLASHMAP_SECLOGIC_OFFSET] = "secure_logic_offset",
	[RTOS_LOGIC_OFFSET][FLASHMAP_MEM_START] = "rtos_logic_offset",
	[RTOS_LOGIC_OFFSET][FLASHMAP_MEM_SIZE] = "rtos_secure_logic_offset",
};

char *fdt_member_name[FLASHMAP_MEM_COUNT][FLASHMAP_MAX_MEM] = {
	[BOOT_PARAM][FLASHMAP_MEM_START] = "boot_param_start",
	[BOOT_PARAM][FLASHMAP_MEM_SIZE] = "boot_param_size",
	[TOC1][FLASHMAP_MEM_START] = "uboot_start",
	[TOC1][FLASHMAP_MEM_SIZE] = "uboot_size",
	[TOC1_BAK][FLASHMAP_MEM_START] = "uboot_bak_start",
	[TOC1_BAK][FLASHMAP_MEM_SIZE] = "uboot_bak_size",
	[SEC_STORAGE][FLASHMAP_MEM_START] = "secure_storage_start",
	[SEC_STORAGE][FLASHMAP_MEM_SIZE] = "secure_storage_size",
	[MMC_TUNING][FLASHMAP_MEM_START] = "tuning_data_start",
	[MMC_TUNING][FLASHMAP_MEM_SIZE]  = "tuning_data_size",
};

struct flashmap_info global_flashmap[FLASHMAP_TYPE_COUNT] = {
	[FLASHMAP_SPI_NOR] = {
		.logic[LINUX_LOGIC_OFFSET] = {
			.logicoffset = DEFAULT_SPINOR_LOGICAL_OFFSET, /*normal*/
			.secure_logicoffset = DEFAULT_SPINOR_LOGICAL_SECURE_OFFSET,
		},
		.logic[RTOS_LOGIC_OFFSET] = {
			.logicoffset = DEFAULT_SUNXI_RTOS_LOGICAL_OFFSET, /*normal*/
			.secure_logicoffset = DEFAULT_SUNXI_RTOS_LOGICAL_OFFSET,
		},

		.maps[BOOT_PARAM_COMMON] = {
			.start = DEFAULT_SPINOR_UBOOT_OFFSET - (BOOT_PARAM_SIZE >> 9),
			.size = BOOT_PARAM_SIZE >> 9
		},
		.maps[BOOT_PARAM] = {
			.start = DEFAULT_SPINOR_UBOOT_OFFSET - (BOOT_PARAM_SIZE >> 9),
			.size = BOOT_PARAM_SIZE >> 9
		},
		.maps[TOC1] = {
			.start = DEFAULT_SPINOR_UBOOT_OFFSET,
			.size = 1888
		},
		.maps[TOC1_BAK] = {
			.start = -1,
			.size = -1
		},
		.maps[SEC_STORAGE] = {
			.start = -1,
			.size = -1
		},
		.maps[MMC_TUNING] = {
			.start = -1,
			.size = -1
		}
	},
	[FLASHMAP_SDMMC] = {
		.logic[LINUX_LOGIC_OFFSET] = {
			.logicoffset = DEFAULT_MMC_LOGICAL_OFFSET, /*normal*/
			.secure_logicoffset = DEFAULT_MMC_LOGICAL_OFFSET,
		},
		.logic[RTOS_LOGIC_OFFSET] = {
			.logicoffset = DEFAULT_SUNXI_RTOS_LOGICAL_OFFSET, /*normal*/
			.secure_logicoffset = DEFAULT_SUNXI_RTOS_LOGICAL_OFFSET,
		},

		.maps[BOOT_PARAM_COMMON] = {
			.start = DEFAULT_MMC_BOOTPARAM_COMMOM_OFFSET,
			.size = DEFAULT_MMC_BOOTPARAM_COMMOM_SIZE
		},
		.maps[BOOT_PARAM] = {
			.start = SUNXI_SDMMC_PARAMETER_REGION_LBA_START,
			.size = SUNXI_SDMMC_PARAMETER_REGION_SIZE_BYTE >> 9
		},
		.maps[TOC1] = {
			.start = SUNXI_MMC_TOC_START_ADDRS,
			.size = 4096
		},
		.maps[TOC1_BAK] = {
			.start = UBOOT_BACKUP_START_SECTOR_IN_SDMMC,
			.size = 4096
		},
		.maps[SEC_STORAGE] = {
			.start = SDMMC_SECURE_STORAGE_START_ADD,
			.size = 512
		},
		.maps[MMC_TUNING] = {
			.start = TUNING_ADD,
			.size = TUNING_LEN
		}
	},
	[FLASHMAP_SPI_NAND] = {
		.maps[TOC1] = {
			.start = DEFAULT_SPINAND_UBOOT_OFFSET,
			.size = 6144
		}
	},
#ifdef CONFIG_SUNXI_UFS
	[FLASHMAP_UFS] = {
		.logic[LINUX_LOGIC_OFFSET] = {
			.logicoffset = DEFAULT_UFS_LOGICAL_OFFSET, /*normal*/
			.secure_logicoffset = DEFAULT_UFS_LOGICAL_OFFSET,
		},
		.logic[RTOS_LOGIC_OFFSET] = {
			.logicoffset = DEFAULT_SUNXI_RTOS_LOGICAL_OFFSET, /*normal*/
			.secure_logicoffset = DEFAULT_SUNXI_RTOS_LOGICAL_OFFSET,
		},

		.maps[BOOT_PARAM_COMMON] = {
			.start = DEFAULT_UFS_BOOTPARAM_COMMOM_OFFSET,
			.size = DEFAULT_UFS_BOOTPARAM_COMMOM_SIZE
		},
		.maps[BOOT_PARAM] = {
			.start = SUNXI_UFS_PARAMETER_REGION_LBA_START,
			.size = SUNXI_UFS_PARAMETER_REGION_SIZE_BYTE >> 9
		},
		.maps[TOC1] = {
			.start = SUNXI_UFS_TOC_START_ADDRS,
			.size = 4096
		},
		.maps[TOC1_BAK] = {
			.start = UBOOT_BACKUP_START_SECTOR_IN_UFS,
			.size = 4096
		},
		.maps[SEC_STORAGE] = {
			.start = UFS_SECURE_STORAGE_START_ADD,
			.size = 512
		},
	},
#endif
};


int sunxi_flashmap_init(void)
{

	pr_msg("sunxi flash map init\n");
	int flash_name = 0, region_name = 0;
	for (flash_name = 0; flash_name < FLASHMAP_TYPE_COUNT; flash_name++) {

		int nodeoffset = 0;
		flashmap_debug("%s\n", fdt_path[flash_name]);
		/* Update parameters from fdt */
		nodeoffset = fdt_path_offset(working_fdt, fdt_path[flash_name]);
		if (nodeoffset < 0) {
			continue;
		}
		if (!fdtdec_get_is_enabled(working_fdt, nodeoffset)) {
			printf("FDT sunxi flash disable\n");
			continue;
		}

		for (region_name = 0; region_name < REGION_LOGICTYPE_COUNT; region_name++) {
			fdt_getprop_u32(working_fdt, nodeoffset,
					fdt_logicoffset_name[region_name][FLASHMAP_LOGIC_OFFSET],
					&global_flashmap[flash_name].logic[region_name].logicoffset);
			fdt_getprop_u32(working_fdt, nodeoffset,
					fdt_logicoffset_name[region_name][FLASHMAP_SECLOGIC_OFFSET],
					&global_flashmap[flash_name].logic[region_name].secure_logicoffset);
		}
		for (region_name = 0; region_name < FLASHMAP_MEM_COUNT; region_name++) {
			fdt_getprop_u32(working_fdt, nodeoffset,
					fdt_member_name[region_name][FLASHMAP_MEM_START],
					&global_flashmap[flash_name].maps[region_name].start);
			fdt_getprop_u32(working_fdt, nodeoffset,
					fdt_member_name[region_name][FLASHMAP_MEM_SIZE],
					&global_flashmap[flash_name].maps[region_name].size);
		}
	}
#if FLASHMAP_DEBUG
	sunxi_flashmap_info_dump();
#endif
	return 0;
}

#if FLASHMAP_DEBUG
void sunxi_flashmap_info_dump(void)
{
	int flash_type = 0, member = 0;
	printf("======================================================\n");

	for (flash_type = 0; flash_type < FLASHMAP_TYPE_COUNT; flash_type++) {
		// printf("flash_type:%s, logical:%d\n", flash_name[flash_type],
		// 	sunxi_flashmap_logical_offset(flash_type));
		for (member = 0; member < REGION_LOGICTYPE_COUNT; member++) {
			printf("%s: %d, %s: %d\n", fdt_logicoffset_name[member][FLASHMAP_LOGIC_OFFSET],
				global_flashmap[flash_type].logic[member].logicoffset,
				fdt_logicoffset_name[member][FLASHMAP_SECLOGIC_OFFSET],
				global_flashmap[flash_type].logic[member].secure_logicoffset);
		}
		for (member = 0; member < FLASHMAP_MEM_COUNT; member++) {
			printf("%s: %d, %s: %d\n", fdt_member_name[member][FLASHMAP_MEM_START],
				global_flashmap[flash_type].maps[member].start,
				fdt_member_name[member][FLASHMAP_MEM_SIZE],
				global_flashmap[flash_type].maps[member].size);
		}
	printf("======================================================\n");
	}
}
#endif

int sunxi_flashmap_logical_offset(enum flash_type flash_name, enum region_logicoffset_type region_name)
{
	if (sunxi_get_secureboard())
		return global_flashmap[flash_name].logic[region_name].secure_logicoffset;
	return global_flashmap[flash_name].logic[region_name].logicoffset;
}

int sunxi_flashmap_offset(enum flash_type flash_name, enum region_flash_type region_name)
{
	return global_flashmap[flash_name].maps[region_name].start;
}

int sunxi_flashmap_size(enum flash_type flash_name, enum region_flash_type region_name)
{
	return global_flashmap[flash_name].maps[region_name].size;
}
