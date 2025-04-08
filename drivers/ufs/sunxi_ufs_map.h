// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2007-2031
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Aaron <leafy.myeh@allwinnertech.com>
 *
 * UFS driver for allwinner sunxi platform.
 */
#ifndef SUNXI_UFS_MAP
#define SUNXI_UFS_MAP

#define SUNXI_UFS_PARAMETER_REGION_LBA_START 24504
#define SUNXI_UFS_PARAMETER_REGION_SIZE_BYTE 512

#define SUNXI_UFS_TOC_START_ADDRS	(32800)
#define UBOOT_BACKUP_START_SECTOR_IN_UFS (24576)

//secure storage relate
#define MAX_SECURE_STORAGE_MAX_ITEM             32
#define UFS_SECURE_STORAGE_START_ADD  (6*1024*1024/512)//6M
#define UFS_ITEM_SIZE                                 (4*1024/512)//4K


#endif
