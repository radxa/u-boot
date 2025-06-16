// SPDX-License-Identifier: GPL-2.0+
#ifndef __MMC_DEF__
#define __MMC_DEF__

#include <asm/types.h>
//#define  CONFIG_MMC_DEBUG_SUNXI
/*change address to iomem pointer*/
/*#define IOMEM_ADDR(addr) ((void __iomem *)((phys_addr_t)(addr)))*/
#define PT_TO_U32(p)   ((u32)((phys_addr_t)(p)))
#define WR_MB() 	wmb()
/*transfer pointer to unsigned type,if phy add is 64,trasfer to u64*/
#define PT_TO_PHU(p)   ((unsigned long)(p))
typedef volatile void __iomem *iom;

#ifdef CONFIG_MMC_DEBUG_SUNXI
#define MMCINFO(fmt, args...)	pr_err("[mmc]: "fmt, ##args)//err or info
#define MMCDBG(fmt, args...)	pr_err("[mmc]: "fmt, ##args)//dbg
#define MMCPRINT(fmt, args...)	pr_err(fmt, ##args)//data or register and so on
#else
#define MMCINFO(fmt, args...)	pr_force("[mmc]: "fmt, ##args)//err or info
#define MMCDBG(fmt...)
#define MMCPRINT(fmt...)
#endif

#define MMC_MSG_EN	(1U)
#define MMCMSG(d, fmt, args...) do {if ((d)->msglevel & MMC_MSG_EN)  pr_err("[mmc]: "fmt, ##args); } while (0)

#define DRIVER_VER  "uboot2018:2024-10-18 9:01:00 -- uboot2018:2025-03-28 10:03:00"

//secure storage relate
#define MAX_SECURE_STORAGE_MAX_ITEM             32
#define SDMMC_SECURE_STORAGE_START_ADD  (6*1024*1024/512)//6M
#define SDMMC_ITEM_SIZE                                 (4*1024/512)//4K

//BOOT0 AND UBOOT ADDR
#define SUNXI_MMC_BOOT0_START_ADDRS	(16)
#define SUNXI_MMC_TOC_START_ADDRS	(32800)
#define UBOOT_BACKUP_START_SECTOR_IN_SDMMC (24576)
//#define CONFIG_SUNXI_BOOT0_SDMMC_BACKUP_START_ADDR (512)

//mmc
#define SUNXI_SDMMC_PARAMETER_REGION_LBA_START 24504
#define SUNXI_SDMMC_PARAMETER_REGION_SIZE_BYTE 512

//#define TUNING_LEN		(1)//The address which store the tuninng pattern
//#define TUNING_ADD		(38192-TUNING_LEN)//The address which store the tuninng pattern
#define TUNING_LEN		(60)//The length of the tuninng pattern
#define TUNING_ADD		(UBOOT_BACKUP_START_SECTOR_IN_SDMMC-4-TUNING_LEN)//The address which store the tuninng pattern
#define REPEAT_TIMES		(30)
#define SAMPLE_MODE 		(2)

#endif
