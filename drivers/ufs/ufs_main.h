// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2007-2012
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * Description: UFS driver for General UFS operations
 * Author: lixiang <lixiang@allwinnertech.com>
 * Date: 2022/10/21 16:22
 */
#ifndef __LOAD_BOOT0_FROM_UFS
#define  __LOAD_BOOT0_FROM_UFS

//#include "Brom_def_cfg.h"

//#define EMMC_BOOT_START_BLOCK (0)
//#define SD_BOOT_START_BLOCK (16)
/*ufs 4k per block*/
//#define UFS_BOOT_START_BLOCK_BACKUP_0 (16/8)
//#define UFS_BOOT_START_BLOCK_BACKUP_1 ((2064)/8) //change to 2064 for lpddr5 change

/*ufs 4k per block*/
#define UFS_BOOT_HEAD_SIZE_BLOCK   (1)
//#define MAX_BOOT0_SIZE		(32U << 10)
//#define MAX_BOOT0_BLOCKS	(MAX_BOOT0_SIZE >> 9)

#define PT_TO_U32(p)   ((u32)((phys_addr_t)(p)))
#define WR_MB() 	wmb()
/*transfer pointer to unsigned type,if phy add is 64,trasfer to u64*/
#define PT_TO_PHU(p)   ((unsigned long)(p))

typedef volatile void __iomem *iom;

/*sector address*/
#define SUNXI_UFS_BOOT0_START_ADDRS (16)

/*sector address*/
#ifndef CONFIG_SUNXI_BOOT0_UFS_BACKUP_START_ADDR
#define CONFIG_SUNXI_BOOT0_UFS_BACKUP_START_ADDR (2064)
#endif

//secure storage relate
#define SUNXI_UFS_MAX_SECURE_STORAGE_MAX_ITEM             32
#define SUNXI_UFS_SECURE_STORAGE_START_ADD  (6*1024*1024/512)//6M
#define SUNXI_UFS_ITEM_SIZE                                 (4*1024/512)//4K

//mmc
#define SUNXI_UFS_PARAMETER_REGION_LBA_START 24504
#define SUNXI_UFS_PARAMETER_REGION_SIZE_BYTE 512



#endif  /*__LOAD_BOOT0_FROM_UFS*/
