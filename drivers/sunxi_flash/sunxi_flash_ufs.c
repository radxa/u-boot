/*
 *SPDX-License-Identifier: GPL-2.0+
 *
 * (C) Copyright 2018
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangwei <wangwei@allwinnertech.com>
 *
 */

#include <common.h>
#include <sunxi_flash.h>
#include <malloc.h>
#include <private_toc.h>
#include <private_boot0.h>
#include <private_uboot.h>
#include <sunxi_board.h>
#include <blk.h>
#include <linux/libfdt.h>
#include <sys_partition.h>
#include "flash_interface.h"
#include "sprite_download.h"
#include "sprite_verify.h"

__attribute__((section(".data"))) static sunxi_flash_desc *current_flash;


static int sunxi_flash_ufs_blk_init(void);


int sunxi_flash_ufs_blk_read(uint start_block, uint nblock, void *buffer)
{
	return current_flash->read(start_block, nblock, buffer);
}

int sunxi_flash_ufs_blk_write(uint start_block, uint nblock, void *buffer)
{
	return current_flash->write(start_block, nblock, buffer);
}

int sunxi_flash_ufs_blk_flush(void)
{
	return current_flash->flush();
}

int sunxi_flash_ufs_blk_phyread(uint start_block, uint nblock, void *buffer)
{
	return current_flash->phyread(start_block, nblock, buffer);
}

int sunxi_flash_ufs_blk_phywrite(uint start_block, uint nblock, void *buffer)
{
	return current_flash->phywrite(start_block, nblock, buffer);
}

uint sunxi_flash_ufs_blk_size(void)
{
	return current_flash->size();
}


int sunxi_flash_ufs_blk_exit(int force)
{
	return current_flash->exit(force);
}

int sunxi_flash_ufs_blk_write_end(void)
{
	int ret = 0;

	if (current_flash->write_end != NULL)
		ret = current_flash->write_end();

	return ret;
}


int sunxi_flash_ufs_blk_init_ext(void)
{
	current_flash = &sunxi_ufs_blk_desc;
	int state  = current_flash->init(WORK_MODE_BOOT, 0);
	//init blk dev
	sunxi_flash_ufs_blk_init();

	return state;
}


static struct blk_desc sunxi_flash_ufs_blk_dev;

static unsigned long sunxi_block_read(struct blk_desc *block_dev,
				      lbaint_t start, lbaint_t blkcnt,
				      void *buffer)
{
		return sunxi_flash_ufs_blk_phyread((uint)start, (uint)blkcnt, (void *)buffer);
}

static unsigned long sunxi_block_write(struct blk_desc *block_dev,
				       lbaint_t start, lbaint_t blkcnt,
				       const void *buffer)
{
		return sunxi_flash_ufs_blk_phywrite((uint)start, (uint)blkcnt, (void *)buffer);
}

static int sunxi_flash_ufs_blk_init(void)
{
	debug("sunxi flash init uboot\n");
	sunxi_flash_ufs_blk_dev.if_type   = IF_TYPE_SUNXI_FLASH_UFS;
	sunxi_flash_ufs_blk_dev.part_type = PART_TYPE_EFI;
	sunxi_flash_ufs_blk_dev.devnum    = 0;
	sunxi_flash_ufs_blk_dev.lun       = 0;
	sunxi_flash_ufs_blk_dev.type      = 0;

	/* FIXME fill in the correct size (is set to 32MByte) */
	sunxi_flash_ufs_blk_dev.blksz       = 4096;
	sunxi_flash_ufs_blk_dev.lba		= 0;
	sunxi_flash_ufs_blk_dev.removable   = 0;
	sunxi_flash_ufs_blk_dev.block_read  = sunxi_block_read;
	sunxi_flash_ufs_blk_dev.block_write = sunxi_block_write;

	return 0;
}

static int sunxi_flash_ufs_blk_get_dev(int dev, struct blk_desc **descp)
{
	sunxi_flash_ufs_blk_dev.devnum = dev;
	sunxi_flash_ufs_blk_dev.lba    = sunxi_flash_ufs_blk_size();

	*descp = &sunxi_flash_ufs_blk_dev;

	return 0;
}

U_BOOT_LEGACY_BLK(sunxi_flash_ufs) = {
	.if_typename   = "sunxi_flash_ufs",
	.if_type       = IF_TYPE_SUNXI_FLASH_UFS,
	.max_devs      = -1,
	.get_dev       = sunxi_flash_ufs_blk_get_dev,
	.select_hwpart = NULL,
};
