// SPDX-License-Identifier: GPL-2.0+
/*
 * sunxi ufs driver for uboot.
 *
 * Copyright (C) 2018
 * 2024.6.25 lixiang <lixiang@allwinnertech.com>
 */
#include <common.h>
#include <sys_config.h>
/*#include <sys_partition.h>*/
/*#include <boot_type.h>*/
#include "private_uboot.h"
#include <private_toc.h>
#include <private_boot0.h>
#include <sunxi_flash.h>
#include <sunxi_board.h>
#include <mmc.h>
#include <malloc.h>
#include <blk.h>
#include <asm/global_data.h>
#include <boot_param.h>
#include <sprite_download.h>
#include <memalign.h>
#include <hexdump.h>

#include "../flash_interface.h"
#include "../../ufs/ufs_main.h"

#define UFS_BLOCK_ALIGN_SECTOR  8

DECLARE_GLOBAL_DATA_PTR;
int sunxi_ufs_global_read(lbaint_t start,
			lbaint_t blkcnt, void *buffer);
int sunxi_ufs_global_write(lbaint_t start,
			lbaint_t blkcnt, void *buffer);
int sunxi_ufs_global_init(void);
u64 sunxi_ufs_global_ufs_size(void);
s32 sunxi_ufs_global_sync_cache(void);
s32 sunxi_ufs_global_format_unit(void);
s32 sunxi_ufs_global_unmap(u64 start_lba, u32 nr_blocks);
s32 sunxi_ufs_global_enable_erase(void);
s32 sunxi_ufs_global_enable_data_reliability(void);
s32 sunxi_ufs_global_logical_unit_config(void);
u64 sunxi_ufs_global_update_ufs_size(void);

static int  ufs_has_init;
static int sunxi_flash_ufs_init(int stage, int card_no);
int sunxi_ufs_init_for_sprite(int workmode, int card_no);
int sunxi_ufs_init_for_boot(int workmode, int card_no);
static int sunxi_sprite_ufs_read(unsigned int start_block, unsigned int nblock,
				void *buffer);
static int sunxi_sprite_ufs_write(unsigned int start_block, unsigned int nblock,
				void *buffer);
int sunxi_sprite_ufs_phyread(unsigned int start_block, unsigned int nblock,
			     void *buffer);
int sunxi_sprite_ufs_phywrite(unsigned int start_block, unsigned int nblock,
			      void *buffer);
static int sunxi_sprite_ufs_buffer_read(unsigned int start_block, unsigned int nblock,
				void *buffer);
static int sunxi_sprite_ufs_buffer_write(unsigned int start_block, unsigned int nblock,
				 void *buffer);
int sunxi_sprite_ufs_buffer_phywrite(unsigned int start_block, unsigned int nblock,
			      void *buffer);
int sunxi_sprite_ufs_buffer_phyread(unsigned int start_block, unsigned int nblock,
			      void *buffer);

static int sunxi_ufs_secure_storage_read_key(int item, unsigned char *buf,
				       unsigned int len);
static int sunxi_ufs_secure_storage_read_map(int item, unsigned char *buf,
				       unsigned int len);

static unsigned char _inner_buffer[4096 + 64]; /*align temp buffer*/

/*
 *
 * normal boot interface
 *
 */

int sunxi_flash_ufs_probe(void)
{
	return 0;
}

static int sunxi_flash_ufs_init(int stage, int card_no)
{
	return sunxi_ufs_init_for_boot(stage, card_no);
}

static int sunxi_flash_ufs_exit(int force)
{
	return sunxi_ufs_global_sync_cache();
}
/*
static int sunxi_flash_ufs_read(unsigned int start_block, unsigned int nblock,
				void *buffer)
{
	debug("ufsboot read: start 0x%x, sector 0x%x\n", start_block, nblock);

	return sunxi_ufs_global_read((start_block + sunxi_flashmap_logical_offset(FLASHMAP_UFS, LINUX_LOGIC_OFFSET))/8,
	    nblock/8, buffer);

}

static int sunxi_flash_ufs_write(unsigned int start_block, unsigned int nblock,
				 void *buffer)
{
	debug("mmcboot write: start 0x%x, sector 0x%x\n", start_block, nblock);

	return sunxi_ufs_global_write((start_block + sunxi_flashmap_logical_offset(FLASHMAP_UFS, LINUX_LOGIC_OFFSET))/8,
	    nblock/8, buffer);

}
*/
int card_verify_boot0(uint start_block, uint length);
static int sunxi_flash_ufs_download_spl(unsigned char *buf, int len,
					unsigned int ext)
{
	uint32_t i, fail_count;
	uint32_t write_offset[2] = { CONFIG_SUNXI_BOOT0_UFS_BACKUP_START_ADDR,
				     SUNXI_UFS_BOOT0_START_ADDRS };

	{
//		volatile int i = 1;
//		while(i);
	}

#ifdef CONFIG_SUNXI_OTA_TURNNING
	if (sunxi_get_active_boot0_id() != 0) {
		write_offset[0] = SUNXI_UFS_BOOT0_START_ADDRS;
		write_offset[1] = CONFIG_SUNXI_BOOT0_UFS_BACKUP_START_ADDR;
	}
#endif

	fail_count = 0;
	for (i = 0; i < 2; i++) {
		if (!sunxi_sprite_ufs_phywrite(write_offset[i], len / 512, buf)) {
			pr_force("%s: write %s failed\n", __func__,
				 write_offset[i] ==
						 SUNXI_UFS_BOOT0_START_ADDRS ?
					 "main spl" :
					 "back spl");
			fail_count++;
		} else {
			pr_force("%s: write %s done\n", __func__,
				 write_offset[i] ==
						 SUNXI_UFS_BOOT0_START_ADDRS ?
					 "main spl" :
					 "back spl");
		}
		if (card_verify_boot0(write_offset[i], len) < 0) {
			return -1;
		}
	}
	if (fail_count == 2) {
		return -1;
	}

	return 0;
}

static int sunxi_flash_ufs_download_boot_param(void)
{
	struct sunxi_boot_param_region *sunxi_boot_param = gd->boot_param;

	sunxi_boot_param->header.check_sum = sunxi_generate_checksum(
		sunxi_boot_param, sizeof(typedef_sunxi_boot_param), 1,
		sunxi_boot_param->header.check_sum);
	if (!sunxi_sprite_ufs_phywrite(sunxi_flashmap_offset(FLASHMAP_UFS, BOOT_PARAM_COMMON),
					       (BOOT_PARAM_SIZE + 511) / 512, sunxi_boot_param)) {
		pr_err("%s: write boot_param failed\n", __func__);
		return -1;
	}
	return 0;
}

static int
sunxi_flash_ufs_download_toc(unsigned char *buf, int len,  unsigned int ext)
{
	if (!sunxi_sprite_ufs_phywrite(sunxi_flashmap_offset(FLASHMAP_UFS, TOC1),
					       (len + 511)/512, buf)) {
		pr_err("%s: write main uboot failed\n", __func__);
		return -1;
	}

	if (!sunxi_sprite_ufs_phywrite(sunxi_flashmap_offset(FLASHMAP_UFS, TOC1_BAK),
		    (len + 511)/512, buf)) {
		pr_err("%s: write back uboot failed\n", __func__);
		return -1;
	}

	return 0;
}

static uint sunxi_flash_ufs_size(void)
{
	return sunxi_ufs_global_ufs_size()*UFS_BLOCK_ALIGN_SECTOR;
}

int sunxi_flash_ufs_phyread(unsigned int start_block, unsigned int nblock,
			    void *buffer)
{
	return sunxi_sprite_ufs_phyread(start_block, nblock, buffer);
}

int sunxi_flash_ufs_phywrite(unsigned int start_block, unsigned int nblock,
			     void *buffer)
{
	return sunxi_sprite_ufs_phywrite(start_block, nblock, buffer);
}

int sunxi_flash_ufs_flush(void)
{
	return 0;
}

int sunxi_flash_ufs_phyerase(unsigned int start_block, unsigned int nblock,
				void *skip)
{
/*
	if (nblock == 0) {
		printf("%s: @nr is 0, erase from @from to end\n", __FUNCTION__);
		nblock = mmc_boot->block_dev.lba - start_block - 1;
	}
	return mmc_boot->block_dev.block_mmc_erase(&mmc_boot->block_dev,
		start_block, nblock, skip);
*/
	dev_err("%s: ufs unsupport now\n", __FUNCTION__);
	return -1;
}

/*
 *
 * normal sprite interface
 *
 */

int sunxi_sprite_ufs_probe(void)
{
//	{
//		volatile int i=1;
//		while(i);
//	}
	return sunxi_ufs_init_for_sprite(0, 0);
}

static int sunxi_sprite_ufs_init(int stage, int card_no)
{
	return sunxi_ufs_init_for_sprite(stage, card_no);
}

static int sunxi_sprite_ufs_exit(int force)
{
	return sunxi_ufs_global_sync_cache();
}

#define UFS_MAX_ALING_BUF_SIZE_BYTE (1*1024*1024)
DEFINE_CACHE_ALIGN_BUFFER(u8, g_align_buf, UFS_MAX_ALING_BUF_SIZE_BYTE);


static int sunxi_sprite_ufs_read(unsigned int start_block, unsigned int nblock,
				void *buffer)
{
	u32 start_sector = start_block;
	//u32 end_sector = start_sector + nblock - 1;
	u32 size_sector = nblock;
	u32 max_buf_len_sector = ((UFS_MAX_ALING_BUF_SIZE_BYTE - (UFS_BLOCK_ALIGN_SECTOR*512)*2)/512);
	u32 max_buf_len_byte = max_buf_len_sector * 512;
	int i = 0;
	int ret = 0;

	//printf("%s,ufs buffer read start %d nblock %d\n", __FUNCTION__, start_block, nblock);

	for (i = 0; i < (size_sector*512)/max_buf_len_byte; i++) {
		ret = sunxi_sprite_ufs_buffer_read(start_sector + i*max_buf_len_sector,  max_buf_len_sector, buffer + i*max_buf_len_byte);
		if (ret != max_buf_len_sector) {
			printf("%s,ufs buffer read %d failed,ret %d\n", __FUNCTION__, max_buf_len_sector, ret);
			return ret;
		}
		//printf("%s,ufs align buffer size read %d\n", __FUNCTION__, ret);
	}
	if (size_sector % max_buf_len_sector) {
		ret = sunxi_sprite_ufs_buffer_read(start_sector + i*max_buf_len_sector,  size_sector % max_buf_len_sector, buffer + i*max_buf_len_byte);
		if (ret !=  (size_sector % max_buf_len_sector)) {
			printf("%s,ufs buffer read %d failed,ret %d\n", __FUNCTION__, max_buf_len_sector, ret);
			return ret;
		}
		//printf("%s,ufs remain read %d\n", __FUNCTION__, size_sector % max_buf_len_sector);
	}
	//printf("%s,ufs buffer read end %d\n", __FUNCTION__, nblock);
	return nblock;
}

static int sunxi_sprite_ufs_buffer_read(unsigned int start_block, unsigned int nblock,
				void *buffer)
{
	debug("ufsboot read: start 0x%x, sector 0x%x\n", start_block, nblock);
	u32 start_sector = start_block + sunxi_flashmap_logical_offset(FLASHMAP_UFS, LINUX_LOGIC_OFFSET);
	u32 end_sector = start_sector + nblock - 1;
	u32 size_sector = nblock;
	u32 align_start = start_sector/UFS_BLOCK_ALIGN_SECTOR;
	u32 align_end = (end_sector + UFS_BLOCK_ALIGN_SECTOR -1)/UFS_BLOCK_ALIGN_SECTOR;
	u32 align_size = align_end - align_start + 1;
	int ret = 0;
//	u8 *align_buf = malloc(align_size*UFS_BLOCK_ALIGN_SECTOR*512);
//	u8 *align_buf = memalign(64, align_size*UFS_BLOCK_ALIGN_SECTOR*512*4);
#ifdef DYNAMIC_MEMALIGN
	u8 *align_buf = NULL;
#else
	u8 *align_buf = g_align_buf;
#endif
//ALLOC_CACHE_ALIGN_BUFFER(u8, align_buf, align_size*UFS_BLOCK_ALIGN_SECTOR*512);
//	printf("ufsboot read: start %d, nblock %d, start_sector %d, end_sector %d,size_sector %d,align_start %d,align_end %d, align_size %d\n",
//							start_block, nblock, start_sector, end_sector ,size_sector ,align_start ,align_end , align_size);

	{
		//volatile int i=1;
	//	while(i);
	}

	if ((start_sector%UFS_BLOCK_ALIGN_SECTOR) || ((end_sector +1)%UFS_BLOCK_ALIGN_SECTOR)) {

		if ((align_size* UFS_BLOCK_ALIGN_SECTOR*512) > UFS_MAX_ALING_BUF_SIZE_BYTE) {
			printf("align size is large than align buf\n");
			goto out;
		}

#ifdef DYNAMIC_MEMALIGN
		align_buf = memalign(64, align_size*UFS_BLOCK_ALIGN_SECTOR*512);
#endif
		//printf("align operlation read\n");
		if (align_buf == NULL) {
			printf("%s,%d, align_buf is null\n", __FUNCTION__, __LINE__);
			goto out;
		}

		ret = sunxi_ufs_global_read(align_start, align_size, align_buf);
		if (ret == align_size) {
			memcpy(buffer, align_buf + (start_sector%UFS_BLOCK_ALIGN_SECTOR)*512, size_sector*512);
		} else {
			printf("%s,%d,ufs write align read failed ret %d\n", __FUNCTION__, __LINE__, ret);
			goto out_free;
		}
	} else {
		ret = sunxi_ufs_global_read(start_sector/UFS_BLOCK_ALIGN_SECTOR, size_sector/UFS_BLOCK_ALIGN_SECTOR, buffer);
		return ret*UFS_BLOCK_ALIGN_SECTOR;
	}
	if (ret == align_size)
		ret = nblock;
	else
		printf("%s,%d,ufs read align read failed ret %d\n", __FUNCTION__, __LINE__, ret);

out_free:
#ifdef DYNAMIC_MEMALIGN
	if (align_buf)
		free(align_buf);
#endif
out:
	return ret;
}


static int sunxi_sprite_ufs_write(unsigned int start_block, unsigned int nblock,
				void *buffer)
{
	u32 start_sector = start_block;
	//u32 end_sector = start_sector + nblock - 1;
	u32 size_sector = nblock;
	u32 max_buf_len_sector = ((UFS_MAX_ALING_BUF_SIZE_BYTE - (UFS_BLOCK_ALIGN_SECTOR*512)*2)/512);
	u32 max_buf_len_byte = max_buf_len_sector * 512;
	int i = 0;
	int ret = 0;

	//dev_dbg("%s,ufs buffer write start %d nblock %d\n", __FUNCTION__, start_block, nblock);

	for (i = 0; i < (size_sector*512)/max_buf_len_byte; i++) {
		ret = sunxi_sprite_ufs_buffer_write(start_sector + i*max_buf_len_sector,  max_buf_len_sector, buffer + i*max_buf_len_byte);
		if (ret != max_buf_len_sector) {
			printf("%s,ufs buffer write %d failed,ret %d\n", __FUNCTION__, max_buf_len_sector, ret);
			return ret;
		}
		//dev_dbg("%s,ufs align buffer size write %d\n", __FUNCTION__, ret);
	}
	if (size_sector % max_buf_len_sector) {
		ret = sunxi_sprite_ufs_buffer_write(start_sector + i*max_buf_len_sector,  size_sector % max_buf_len_sector, buffer + i*max_buf_len_byte);
		if (ret !=  (size_sector % max_buf_len_sector)) {
			printf("%s,ufs buffer write %d failed,ret %d\n", __FUNCTION__, max_buf_len_sector, ret);
			return ret;
		}
		//dev_dbg("%s,ufs remain write %d\n", __FUNCTION__, size_sector % max_buf_len_sector);
	}
	//dev_dbg("%s,ufs buffer write end %d\n", __FUNCTION__, nblock);
	return nblock;
}


static int sunxi_sprite_ufs_buffer_write(unsigned int start_block, unsigned int nblock,
				 void *buffer)
{
	debug("ufsboot write: start 0x%x, sector 0x%x\n", start_block, nblock);
	u32 start_sector = start_block + sunxi_flashmap_logical_offset(FLASHMAP_UFS, LINUX_LOGIC_OFFSET);
	u32 end_sector = start_sector + nblock - 1;
	u32 size_sector = nblock;
	u32 align_start = start_sector/UFS_BLOCK_ALIGN_SECTOR;
	u32 align_end = (end_sector + UFS_BLOCK_ALIGN_SECTOR -1)/UFS_BLOCK_ALIGN_SECTOR;
	u32 align_size = align_end - align_start + 1;
	int ret = 0;
//	u8 *align_buf = malloc(align_size*UFS_BLOCK_ALIGN_SECTOR*512);
//	u8 *align_buf = memalign(64, align_size*ufs_block_align_sector*512*4);
#ifdef DYNAMIC_MEMALIGN
	u8 *align_buf = NULL;
#else
	u8 *align_buf = g_align_buf;
#endif
// printf("ufsboot read: start %d, nblock %d, start_sector %d, end_sector %d,size_sector %d,align_start %d,align_end %d, align_size %d\n",
//							start_block, nblock, start_sector, end_sector ,size_sector ,align_start ,align_end , align_size);

	//ALLOC_CACHE_ALIGN_BUFFER(u8, align_buf, align_size*UFS_BLOCK_ALIGN_SECTOR*512);
	if ((start_sector%UFS_BLOCK_ALIGN_SECTOR) || ((end_sector +1)%UFS_BLOCK_ALIGN_SECTOR)) {
		if ((align_size* UFS_BLOCK_ALIGN_SECTOR*512) > UFS_MAX_ALING_BUF_SIZE_BYTE) {
			printf("align size is large than align buf\n");
			goto out;
		}
#ifdef DYNAMIC_MEMALIGN
		align_buf = memalign(64, align_size*UFS_BLOCK_ALIGN_SECTOR*512);
#endif
		//printf("align operlation write\n");
		if (align_buf == NULL) {
			printf("%s,%d, align_buf is null\n", __FUNCTION__, __LINE__);
			goto out;
		}

		ret = sunxi_ufs_global_read(align_start, align_size, align_buf);
		if (ret == align_size) {
			memcpy(align_buf + (start_sector%UFS_BLOCK_ALIGN_SECTOR)*512, buffer, size_sector*512);
		} else {
			printf("%s,%d,ufs write align read failed ret %d\n", __FUNCTION__, __LINE__, ret);
			goto out_free;
		}
		ret = sunxi_ufs_global_write(align_start, align_size, align_buf);
	} else {
		ret = sunxi_ufs_global_write(start_sector/UFS_BLOCK_ALIGN_SECTOR, size_sector/UFS_BLOCK_ALIGN_SECTOR, buffer);
		return ret*UFS_BLOCK_ALIGN_SECTOR;
	}

	if (ret == align_size)
		ret = nblock;
	else
		printf("%s,%d,ufs write align read failed ret %d\n", __FUNCTION__, __LINE__, ret);

out_free:
#ifdef DYNAMIC_MEMALIGN
	if (align_buf)
		free(align_buf);
#endif
out:
	return ret;
}

static int sunxi_sprite_ufs_erase(int erase, void *mbr_buffer)
{
	return card_erase(erase, mbr_buffer);
}

int sunxi_sprite_ufs_flush(void)
{
	return 0;
}

static u32 sunxi_sprite_ufs_size(void)
{
	/*To do:get ufs max size**/
	//return mmc_sprite->block_dev.lba;
//	printf("%s: ufs unsupport now\n", __FUNCTION__);
	//return UINT_MAX;
//	return (128U*1024*1024*1024*9/10)/512;
	return sunxi_ufs_global_ufs_size()*4096/512;
}

int sunxi_sprite_ufs_phyread(unsigned int start_block, unsigned int nblock,
				void *buffer)
{
	u32 start_sector = start_block;
	//u32 end_sector = start_sector + nblock - 1;
	u32 size_sector = nblock;
	u32 max_buf_len_sector = ((UFS_MAX_ALING_BUF_SIZE_BYTE - (UFS_BLOCK_ALIGN_SECTOR*512)*2)/512);
	u32 max_buf_len_byte = max_buf_len_sector * 512;
	int i = 0;
	int ret = 0;

	//printf("%s,ufs buffer read start %d nblock %d\n", __FUNCTION__, start_block, nblock);

	for (i = 0; i < (size_sector*512)/max_buf_len_byte; i++) {
		ret = sunxi_sprite_ufs_buffer_phyread(start_sector + i*max_buf_len_sector,  max_buf_len_sector, buffer + i*max_buf_len_byte);
		if (ret != max_buf_len_sector) {
			printf("%s,ufs buffer read %d failed,ret %d\n", __FUNCTION__, max_buf_len_sector, ret);
			return ret;
		}
		//printf("%s,ufs align buffer size read %d\n", __FUNCTION__, ret);
	}
	if (size_sector % max_buf_len_sector) {
		ret = sunxi_sprite_ufs_buffer_phyread(start_sector + i*max_buf_len_sector,  size_sector % max_buf_len_sector, buffer + i*max_buf_len_byte);
		if (ret !=  (size_sector % max_buf_len_sector)) {
			printf("%s,ufs buffer read %d failed,ret %d\n", __FUNCTION__, max_buf_len_sector, ret);
			return ret;
		}
		//printf("%s,ufs remain read %d\n", __FUNCTION__, size_sector % max_buf_len_sector);
	}
	//printf("%s,ufs buffer read end %d\n", __FUNCTION__, nblock);
	return nblock;
}



int sunxi_sprite_ufs_buffer_phyread(unsigned int start_block, unsigned int nblock,
			     void *buffer)
{
//	return sunxi_ufs_global_read(start_block/8, nblock/8, buffer);
	debug("ufsboot read: start 0x%x, sector 0x%x\n", start_block, nblock);
	u32 start_sector = start_block;
	u32 end_sector = start_sector + nblock - 1;
	u32 size_sector = nblock;
	u32 align_start = start_sector/UFS_BLOCK_ALIGN_SECTOR;
	u32 align_end = (end_sector + UFS_BLOCK_ALIGN_SECTOR -1)/UFS_BLOCK_ALIGN_SECTOR;
	u32 align_size = align_end - align_start + 1;
	int ret = 0;
	//	u8 *align_buf = malloc(align_size*UFS_BLOCK_ALIGN_SECTOR*512);
	//u8 *align_buf = memalign(64, align_size*UFS_BLOCK_ALIGN_SECTOR*512*4);
#ifdef DYNAMIC_MEMALIGN
	u8 *align_buf = NULL;
#else
	u8 *align_buf = g_align_buf;
#endif
//	printf("ufsboot read: start %d, nblock %d, start_sector %d, end_sector %d,size_sector %d,align_start %d,align_end %d, align_size %d\n",
//							start_block, nblock, start_sector, end_sector ,size_sector ,align_start ,align_end , align_size);
	//ALLOC_CACHE_ALIGN_BUFFER(u8, align_buf, align_size*UFS_BLOCK_ALIGN_SECTOR*512);
	if ((start_sector%UFS_BLOCK_ALIGN_SECTOR) || ((end_sector +1)%UFS_BLOCK_ALIGN_SECTOR)) {
		if ((align_size* UFS_BLOCK_ALIGN_SECTOR*512) > UFS_MAX_ALING_BUF_SIZE_BYTE) {
			printf("align size is large than align buf\n");
			goto out;
		}

		//printf("align operlation phyread\n");
#ifdef DYNAMIC_MEMALIGN
		align_buf = memalign(64, align_size*UFS_BLOCK_ALIGN_SECTOR*512);
#endif
		if (align_buf == NULL) {
			printf("%s,%d, align_buf is null\n", __FUNCTION__, __LINE__);
			goto out;
		}

		ret = sunxi_ufs_global_read(align_start, align_size, align_buf);
		if (ret == align_size) {
			memcpy(buffer, align_buf + (start_sector%UFS_BLOCK_ALIGN_SECTOR)*512, size_sector*512);
		} else {
			printf("%s,%d,ufs write align read failed ret %d\n", __FUNCTION__, __LINE__, ret);
			goto out_free;
		}
	} else {
		ret = sunxi_ufs_global_read(start_sector/UFS_BLOCK_ALIGN_SECTOR, size_sector/UFS_BLOCK_ALIGN_SECTOR,  buffer);
		return ret*UFS_BLOCK_ALIGN_SECTOR;
	}
	if (ret == align_size)
		ret = nblock;
	else
		printf("%s,%d,ufs read align read failed ret %d\n", __FUNCTION__, __LINE__, ret);

out_free:
#ifdef DYNAMIC_MEMALIGN
	if (align_buf)
		free(align_buf);
#endif
out:
	return ret;
}

int sunxi_sprite_ufs_phywrite(unsigned int start_block, unsigned int nblock,
				void *buffer)
{
	u32 start_sector = start_block;
	//u32 end_sector = start_sector + nblock - 1;
	u32 size_sector = nblock;
	u32 max_buf_len_sector = ((UFS_MAX_ALING_BUF_SIZE_BYTE - (UFS_BLOCK_ALIGN_SECTOR*512)*2)/512);
	u32 max_buf_len_byte = max_buf_len_sector * 512;
	int i = 0;
	int ret = 0;

	//printf("%s,ufs buffer read start %d nblock %d\n", __FUNCTION__, start_block, nblock);

	for (i = 0; i < (size_sector*512)/max_buf_len_byte; i++) {
		ret = sunxi_sprite_ufs_buffer_phywrite(start_sector + i*max_buf_len_sector,  max_buf_len_sector, buffer + i*max_buf_len_byte);
		if (ret != max_buf_len_sector) {
			printf("%s,ufs buffer read %d failed,ret %d\n", __FUNCTION__, max_buf_len_sector, ret);
			return ret;
		}
		//printf("%s,ufs align buffer size read %d\n", __FUNCTION__, ret);
	}
	if (size_sector % max_buf_len_sector) {
		ret = sunxi_sprite_ufs_buffer_phywrite(start_sector + i*max_buf_len_sector,  size_sector % max_buf_len_sector, buffer + i*max_buf_len_byte);
		if (ret !=  (size_sector % max_buf_len_sector)) {
			printf("%s,ufs buffer read %d failed,ret %d\n", __FUNCTION__, max_buf_len_sector, ret);
			return ret;
		}
		//printf("%s,ufs remain read %d\n", __FUNCTION__, size_sector % max_buf_len_sector);
	}
	//printf("%s,ufs buffer read end %d\n", __FUNCTION__, nblock);
	return nblock;
}



int sunxi_sprite_ufs_buffer_phywrite(unsigned int start_block, unsigned int nblock,
			      void *buffer)
{
//	return sunxi_ufs_global_write(start_block, nblock, buffer);
	u32 start_sector = start_block;
	u32 end_sector = start_sector + nblock - 1;
	u32 size_sector = nblock;
	u32 align_start = start_sector/UFS_BLOCK_ALIGN_SECTOR;
	u32 align_end = (end_sector + UFS_BLOCK_ALIGN_SECTOR -1)/UFS_BLOCK_ALIGN_SECTOR;
	u32 align_size = align_end - align_start + 1;
	int ret = 0;

	//	u8 *align_buf = malloc(align_size*UFS_BLOCK_ALIGN_SECTOR*512);
	//u8 *align_buf = memalign(64, align_size*UFS_BLOCK_ALIGN_SECTOR*512*4);
#ifdef DYNAMIC_MEMALIGN
	u8 *align_buf = NULL;
#else
	u8 *align_buf = g_align_buf;
#endif
//	printf("ufsboot read: start %d, nblock %d, start_sector %d, end_sector %d,size_sector %d,align_start %d,align_end %d, align_size %d\n",
//							start_block, nblock, start_sector, end_sector ,size_sector ,align_start ,align_end , align_size);
	//ALLOC_CACHE_ALIGN_BUFFER(u8, align_buf, align_size*UFS_BLOCK_ALIGN_SECTOR*512);
	if ((start_sector%UFS_BLOCK_ALIGN_SECTOR) || ((end_sector +1)%UFS_BLOCK_ALIGN_SECTOR)) {
		if ((align_size* UFS_BLOCK_ALIGN_SECTOR*512) >  UFS_MAX_ALING_BUF_SIZE_BYTE) {
			printf("align size is large than align buf\n");
			goto out;
		}

#ifdef DYNAMIC_MEMALIGN
		align_buf = memalign(64, align_size*UFS_BLOCK_ALIGN_SECTOR*512);
#endif
		//printf("align operlation phywrite\n");
		if (align_buf == NULL) {
			printf("%s,%d, align_buf is null\n", __FUNCTION__, __LINE__);
			goto out;
		}

		ret = sunxi_ufs_global_read(align_start, align_size, align_buf);
		if (ret == align_size) {
			memcpy(align_buf + (start_sector%UFS_BLOCK_ALIGN_SECTOR)*512, buffer, size_sector*512);
		} else {
			printf("%s,%d,ufs write align read failed ret %d\n", __FUNCTION__, __LINE__, ret);
			goto out_free;
		}
		ret = sunxi_ufs_global_write(align_start, align_size, align_buf);
	} else {
		ret = sunxi_ufs_global_write(start_sector/UFS_BLOCK_ALIGN_SECTOR, size_sector/UFS_BLOCK_ALIGN_SECTOR,  buffer);
		return ret*UFS_BLOCK_ALIGN_SECTOR;
	}
	if (ret == align_size)
		ret = nblock;
	else
		printf("%s,%d,ufs write align read failed ret %d\n", __FUNCTION__, __LINE__, ret);

out_free:
#ifdef DYNAMIC_MEMALIGN
		if (align_buf)
			free(align_buf);
#endif
out:
	return ret;

}

int sunxi_sprite_ufs_phyerase(unsigned int start_block, unsigned int nblock,
				void *skip)
{
/*
	if (nblock == 0) {
		printf("%s: @nr is 0, erase from @from to end\n", __FUNCTION__);
		nblock = mmc_sprite->block_dev.lba - start_block - 1;
	}
	return mmc_sprite->block_dev.block_mmc_erase(&mmc_sprite->block_dev,
		start_block, nblock, skip);
*/
	dev_err("%s: ufs unsupport now\n", __FUNCTION__);
	return -1;
}
/*
int sunxi_sprite_mmc_phywipe(unsigned int start_block, unsigned int nblock,
			     void *skip)
{
	if (nblock == 0) {
		printf("%s: @nr is 0, wipe from @from to end\n", __FUNCTION__);
		nblock = mmc_sprite->block_dev.lba - start_block - 1;
	}
	return mmc_sprite->block_dev.block_mmc_secure_wipe(&mmc_sprite->block_dev,
			start_block, nblock, skip);
}
*/
int sunxi_sprite_ufs_force_erase(void)
{

#if 0
	int ret = sunxi_ufs_global_logical_unit_config();
	if (ret) {
		dev_err(NULL, "erase: force logical unit config failed\n");
		return ret;
	}
	sunxi_ufs_global_update_ufs_size();
#else
	int ret = 0;
	ret = sunxi_ufs_global_enable_erase();
	if (ret) {
		dev_err(NULL, "enable erase failed");
		return ret;
	}

	ret = sunxi_ufs_global_enable_data_reliability();
	if (ret) {
		dev_err(NULL, "enable data reliability failed");
		return ret;
	}
#endif
	return  sunxi_ufs_global_format_unit();
//	return sunxi_ufs_global_unmap(0, sunxi_ufs_global_ufs_size());
}


static int sunxi_sprite_ufs_download_spl(unsigned char *buf, int len,
					 unsigned int ext)
{
#ifdef CONFIG_SUNXI_BOOT_PARAM
	typedef_sunxi_boot_param *sunxi_boot_param = gd->boot_param;
	int boot0_checksum			   = 0;
	int ret					   = -1;

	if (!sunxi_bootparam_format(sunxi_boot_param)) {
		ret = sunxi_sprite_get_boot0_cheksum(buf, &boot0_checksum);
		if (ret < 0) {
			return -1;
		}
		sunxi_bootparam_set_boot0_checksum(sunxi_boot_param,
						   boot0_checksum);
		sunxi_boot_param->header.check_sum = sunxi_generate_checksum(
		sunxi_boot_param, sizeof(typedef_sunxi_boot_param), 1,
		sunxi_boot_param->header.check_sum);

		if (!sunxi_sprite_ufs_phywrite(
			    sunxi_flashmap_offset(FLASHMAP_UFS,
						  BOOT_PARAM_COMMON),
			    (sunxi_flashmap_size(FLASHMAP_UFS,
						BOOT_PARAM_COMMON)),
			    sunxi_boot_param)) {
			pr_err("%s: write boot_param failed\n", __func__);
			return -1;
		}
	}
#endif



	dev_err(NULL, "write ufs back up addr %d ,len %d, align addr %d, align len %d\n", \
			 CONFIG_SUNXI_BOOT0_UFS_BACKUP_START_ADDR, len, CONFIG_SUNXI_BOOT0_UFS_BACKUP_START_ADDR/8, \
			 (len + UFS_BLOCK_ALIGN_SECTOR*512 -1)/(UFS_BLOCK_ALIGN_SECTOR*512));
	print_hex_dump("src toc0: ", DUMP_PREFIX_ADDRESS, 16, 1, buf, 16, true);
	if (!sunxi_ufs_global_write(CONFIG_SUNXI_BOOT0_UFS_BACKUP_START_ADDR/UFS_BLOCK_ALIGN_SECTOR,
					      (len + UFS_BLOCK_ALIGN_SECTOR*512 -1)/(UFS_BLOCK_ALIGN_SECTOR*512), buf)) {
		pr_err("%s: write backup spl failed\n", __func__);
		return -1;
	}

	dev_err(NULL, "write ufs addr %d ,len %d, align addr %d, align len %d\n", \
			 SUNXI_UFS_BOOT0_START_ADDRS, len, SUNXI_UFS_BOOT0_START_ADDRS/UFS_BLOCK_ALIGN_SECTOR, \
			 (len + UFS_BLOCK_ALIGN_SECTOR*512 -1)/(UFS_BLOCK_ALIGN_SECTOR*512));
	print_hex_dump("src toc0: ", DUMP_PREFIX_ADDRESS, 16, 1, buf, 16, true);

	if (!sunxi_ufs_global_write(SUNXI_UFS_BOOT0_START_ADDRS/UFS_BLOCK_ALIGN_SECTOR,
					      (len + UFS_BLOCK_ALIGN_SECTOR*512 -1)/(UFS_BLOCK_ALIGN_SECTOR*512), buf)) {
		pr_err("%s: write main spl failed\n", __func__);
		return -1;
	}

#if 0
	{
		u8 *tbuf = g_align_mirror_buffer;
		memset(tbuf, 0xef, 4096);
		sunxi_ufs_global_read(256/8, 1, tbuf);
		print_hex_dump("read 256 toc0: ", DUMP_PREFIX_ADDRESS, 16, 1, tbuf, 16, true);
		sunxi_ufs_global_read(2064/8, 1, tbuf);
		print_hex_dump("read 2064 toc0: ", DUMP_PREFIX_ADDRESS, 16, 1, tbuf, 16, true);
	}
#endif

	return 0;
}

static int sunxi_sprite_ufs_download_boot_param(void)
{
	struct sunxi_boot_param_region *sunxi_boot_param = gd->boot_param;

	sunxi_boot_param->header.check_sum = sunxi_generate_checksum(
		sunxi_boot_param, sizeof(typedef_sunxi_boot_param), 1,
		sunxi_boot_param->header.check_sum);

	if (!sunxi_sprite_ufs_phywrite(sunxi_flashmap_offset(FLASHMAP_UFS, BOOT_PARAM_COMMON),
					       (BOOT_PARAM_SIZE + 511 / 512), sunxi_boot_param)) {
		pr_err("%s: write boot_param failed\n", __func__);
		return -1;
	}
	return 0;
}

static int sunxi_sprite_ufs_download_toc(unsigned char *buf, int len,
					 unsigned int ext)
{
	if (!sunxi_sprite_ufs_phywrite(sunxi_flashmap_offset(FLASHMAP_UFS, TOC1),
					       (len + 511)/512, buf)) {
		pr_err("%s: write main uboot failed\n", __func__);
		return -1;
	}

	if (!sunxi_sprite_ufs_phywrite(sunxi_flashmap_offset(FLASHMAP_UFS, TOC1_BAK),
		    (len + 511)/512, buf)) {
		pr_err("%s: write back uboot failed\n", __func__);
		return -1;
	}

	return 0;
}

/*
 *
 * secure boot interface
 *
 */

int sunxi_flash_ufs_secread(int item, unsigned char *buf, unsigned int nblock)
{
	int ret = 0;
	if (buf == NULL) {
		printf("input buf is NULL\n");
		ret = -1;
		goto OUT;
	}

	if (item > SUNXI_UFS_MAX_SECURE_STORAGE_MAX_ITEM) {
		printf("item exceed %d\n", SUNXI_UFS_MAX_SECURE_STORAGE_MAX_ITEM);
		ret = -1;
		goto OUT;
	}

	if (nblock > SUNXI_UFS_ITEM_SIZE) {
		printf("block count exceed %d\n", SUNXI_UFS_ITEM_SIZE);
		ret = -1;
		goto OUT;
	}
	if (sunxi_sprite_ufs_phyread(sunxi_flashmap_offset(FLASHMAP_UFS, SEC_STORAGE) + SUNXI_UFS_ITEM_SIZE * 2 * item,
		nblock, buf) != nblock) {
		printf("read first backup failed in fun %s line %d\n", __FUNCTION__, __LINE__);
		ret = -1;
		goto OUT;
	}
OUT:
	return ret;
}

int sunxi_flash_ufs_secread_backup(int item, unsigned char *buf,
				   unsigned int nblock)
{
	int ret = 0;
	if (buf == NULL) {
		printf("input buf is NULL\n");
		ret = -1;
		goto OUT;
	}

	if (item > SUNXI_UFS_MAX_SECURE_STORAGE_MAX_ITEM) {
		printf("item exceed %d\n", SUNXI_UFS_MAX_SECURE_STORAGE_MAX_ITEM);
		ret = -1;
		goto OUT;
	}

	if (nblock > SUNXI_UFS_ITEM_SIZE) {
		printf("block count exceed %d\n", SUNXI_UFS_ITEM_SIZE);
		ret = -1;
		goto OUT;
	}

	if (sunxi_sprite_ufs_phyread(sunxi_flashmap_offset(FLASHMAP_UFS, SEC_STORAGE) + SUNXI_UFS_ITEM_SIZE * 2 * item + SUNXI_UFS_ITEM_SIZE,
		nblock, buf) != nblock) {
		printf("read second backup failed in fun %s line %d\n", __FUNCTION__, __LINE__);
		ret = -1;
		goto OUT;
	}
OUT:
	return ret;
}

int sunxi_flash_ufs_secwrite(int item, unsigned char *buf, unsigned int nblock)
{
	int ret = 0;
	if (buf == NULL) {
		printf("input buf is NULL\n");
		ret = -1;
		goto OUT;
	}

	if (item > SUNXI_UFS_MAX_SECURE_STORAGE_MAX_ITEM) {
		printf("item exceed %d\n", SUNXI_UFS_MAX_SECURE_STORAGE_MAX_ITEM);
		ret = -1;
		goto OUT;
	}

	if (nblock > SUNXI_UFS_ITEM_SIZE) {
		printf("block count exceed %d\n", SUNXI_UFS_ITEM_SIZE);
		ret = -1;
		goto OUT;
	}

	if (sunxi_sprite_ufs_phywrite(sunxi_flashmap_offset(FLASHMAP_UFS, SEC_STORAGE) + SUNXI_UFS_ITEM_SIZE * 2 * item,
		nblock, buf) != nblock) {
		printf("write first backup failed in fun %s line %d\n", __FUNCTION__, __LINE__);
		ret = -1;
		goto OUT;
	}

	if (sunxi_sprite_ufs_phywrite(sunxi_flashmap_offset(FLASHMAP_UFS, SEC_STORAGE) + SUNXI_UFS_ITEM_SIZE * 2 * item + SUNXI_UFS_ITEM_SIZE,
		nblock, buf) != nblock) {
		printf("write second backup failed in fun %s line %d\n", __FUNCTION__, __LINE__);
		ret = -1;
		goto OUT;
	}
OUT:
	return ret;
}

/*
 *
 * secure sprite interface
 *
 */

int sunxi_sprite_ufs_secread(int item, unsigned char *buf, unsigned int nblock)
{
	return sunxi_flash_ufs_secread(item, buf, nblock);
}

int sunxi_sprite_ufs_secread_backup(int item, unsigned char *buf,
				    unsigned int nblock)
{
	return sunxi_flash_ufs_secread_backup(item, buf, nblock);
}

int sunxi_sprite_ufs_secwrite(int item, unsigned char *buf, unsigned int nblock)
{
	return sunxi_flash_ufs_secwrite(item, buf, nblock);
}

int sunxi_ufs_secure_storage_read(int item, unsigned char *buf, unsigned int len)
{

	if (item == 0)
		return sunxi_ufs_secure_storage_read_map(item, buf, len);
	else
		return sunxi_ufs_secure_storage_read_key(item, buf, len);
	return -1;
}


int sunxi_ufs_secure_storage_write(int item, unsigned char *buf, unsigned int len)
{
	unsigned char *align;
	unsigned int blkcnt;
	int workmode;

	if (PT_TO_PHU(buf) % 32) { // input buf not align
		align = (unsigned char *)((PT_TO_PHU(_inner_buffer) + 0x20) &
					  (~0x1f));
		memcpy(align, buf, len);
	} else
		align = buf;

	blkcnt = (len + 511) / 512;
	workmode = uboot_spare_head.boot_data.work_mode;
	if (workmode == WORK_MODE_BOOT ||
	    workmode == WORK_MODE_SPRITE_RECOVERY) {
		return sunxi_flash_ufs_secwrite(item, align, blkcnt);
	} else if ((workmode & WORK_MODE_PRODUCT) || (workmode == 0x30)) {
		return sunxi_sprite_ufs_secwrite(item, align, blkcnt);
	} else {
		printf("workmode=%d is err\n", workmode);
		return -1;
	}
}
static int sunxi_ufs_mirror_boo0_copy(uint32_t src, uint32_t dst)
{
	int ret, size;
	u8 buffer[UFS_BLOCK_ALIGN_SECTOR*512];
	u8 *verify_buffer = NULL;
	u8 *boot_buffer	  = NULL;
	uint32_t check_sum;
	int i;
	const int write_retry_cnt = 3;
	ret			  = sunxi_flash_ufs_phyread(src, UFS_BLOCK_ALIGN_SECTOR, buffer);
	if (!ret) {
		pr_error("sunxi_flash_mmc_phyread fail\n");
		return -1;
	}

	if (sunxi_get_secureboard()) {
		toc0_private_head_t *toc0 = NULL;
		toc0			  = (toc0_private_head_t *)buffer;
		if (strncmp((const char *)toc0->name, TOC0_MAGIC, MAGIC_SIZE)) {
			pr_error("src toc0 magic is bad\n");
			print_hex_dump("src toc0: ", DUMP_PREFIX_ADDRESS, 16, 1, buffer, 16, true);
			return -1;
		}
		check_sum = toc0->check_sum;
		size	  = toc0->length;
	} else {
		boot0_file_head_t *boot0;
		boot0 = (boot0_file_head_t *)buffer;
		if (strncmp((const char *)boot0->boot_head.magic, BOOT0_MAGIC,
			    MAGIC_SIZE)) {
			pr_error("src boot0 magic is bad\n");
			return -1;
		}
		check_sum = boot0->boot_head.check_sum;
		size	  = boot0->boot_head.length;
	}

	boot_buffer = (u8 *)malloc(size);
	if (!boot_buffer) {
		pr_error("malloc buf fail\n");
		return -1;
	}
	verify_buffer = (u8 *)malloc(size);
	if (!verify_buffer) {
		pr_error("malloc buf fail\n");
		goto MIRROR_FAILED_;
	}

	ret = sunxi_flash_ufs_phyread(src, size / 512, boot_buffer);
	if (!ret) {
		pr_error("sunxi_flash_mmc_phyread fail\n");
		goto MIRROR_FAILED_;
	}

	if (sunxi_verify_checksum(boot_buffer, size, check_sum)) {
		pr_error("boot0 checksum is error\n");
		goto MIRROR_FAILED_;
	}

	for (i = 0; i < write_retry_cnt; i++) {
		ret = sunxi_flash_ufs_phywrite(dst, size / 512, boot_buffer);
		if (!ret) {
			pr_error("sunxi_flash_mmc_phywrite backup fail\n");
			continue;
		}

		memset(verify_buffer, 0, size);
		ret = sunxi_flash_ufs_phyread(dst, size / 512, verify_buffer);
		if (!ret) {
			pr_error("sunxi_flash_mmc_phyread backup fail\n");
			continue;
		}

		if (sunxi_verify_checksum(verify_buffer, size, check_sum)) {
			pr_error("recovery boot0 checksum is error\n");
			continue;
		}
	}
	if (boot_buffer)
		free(boot_buffer);
	if (verify_buffer)
		free(verify_buffer);
	return 0;

MIRROR_FAILED_:
	if (boot_buffer)
		free(boot_buffer);
	if (verify_buffer)
		free(verify_buffer);
	return -1;
}

#ifdef CONFIG_SUNXI_RECOVERY_BOOT0_COPY0
int sunxi_flash_ufs_recover_boot0_copy0(void)
{
	if (sunxi_get_active_boot0_id() != 0) {
		int ret;
		ret = snxi_ufs_mirror_boo0_copy(
			CONFIG_SUNXI_BOOT0_UFS_BACKUP_START_ADDR,
			SUNXI_UFS_BOOT0_START_ADDRS);
		if (0 == ret) {
			pr_force("recover boot0 copy0 ok\n");
		} else {
			pr_force("recover boot0 copy0 failed\n");
		}
		return ret;
	}
	pr_debug("boot0 copy0 ok, do not need recover");
	return 0;
}
#endif

int sunxi_ufs_update_backup_boot0(void)
{
	int ret;
	u8 buffer[UFS_BLOCK_ALIGN_SECTOR*512];
	toc0_private_head_t *toc0 = NULL;

	if (sunxi_get_securemode() == SUNXI_NORMAL_MODE) {
		pr_msg("non secure, do not need update backup boot0 to toc0\n");
		return 0;
	}

	/*check if backup already a toc0*/
	memset(buffer, 0x0, 512);
	ret = sunxi_flash_ufs_phyread(
		CONFIG_SUNXI_BOOT0_UFS_BACKUP_START_ADDR, UFS_BLOCK_ALIGN_SECTOR, buffer);
	if (!ret) {
		pr_error("sunxi_flash_mmc_phyread backup fail\n");
		return -1;
	}

	toc0 = (toc0_private_head_t *)buffer;
	if (strncmp((const char *)toc0->name, TOC0_MAGIC, MAGIC_SIZE) == 0) {
		pr_msg("toc0 magic is ok\n");
		return 0;
	}

	printf("update ufs backup boot0 start\n");
	if (0 ==
	    sunxi_ufs_mirror_boo0_copy(SUNXI_UFS_BOOT0_START_ADDRS,
				 CONFIG_SUNXI_BOOT0_UFS_BACKUP_START_ADDR)) {
		printf("update ufs backup boot0 ok\n");
		return 0;
	} else {
		printf("update ufs backup boot0 failed\n");
		return -1;
	}
}



int sunxi_ufs_blk_phyread(unsigned int start_block, unsigned int nblock,
			     void *buffer)
{
	debug("ufs blk read: start 0x%x, sector 0x%x\n", start_block, nblock);
	int ret = sunxi_ufs_global_read(start_block, nblock,  buffer);
	return ret;
}



int sunxi_ufs_blk_phywrite(unsigned int start_block, unsigned int nblock,
			      void *buffer)
{
	int ret = sunxi_ufs_global_write(start_block, nblock,  buffer);
	return ret;

}

static uint sunxi_ufs_blk_size(void)
{
	return sunxi_ufs_global_ufs_size();
}



sunxi_flash_desc sunxi_ufs_desc = {
    .probe = sunxi_flash_ufs_probe,
    .init = sunxi_flash_ufs_init,
    .exit = sunxi_flash_ufs_exit,
    .read = sunxi_sprite_ufs_read,
    .write = sunxi_sprite_ufs_write,
    .erase = sunxi_sprite_ufs_erase,
    .flush = sunxi_flash_ufs_flush,
    .size = sunxi_flash_ufs_size,
#ifdef CONFIG_SUNXI_SECURE_STORAGE
    .secstorage_read = sunxi_ufs_secure_storage_read,
    .secstorage_write = sunxi_ufs_secure_storage_write,
#endif
    .phyread = sunxi_sprite_ufs_phyread,
    .phywrite = sunxi_sprite_ufs_phywrite,
    .phyerase = sunxi_flash_ufs_phyerase,
    .download_spl = sunxi_flash_ufs_download_spl,
    .download_boot_param = sunxi_flash_ufs_download_boot_param,
    .download_toc = sunxi_flash_ufs_download_toc,
    .update_backup_boot0 = sunxi_ufs_update_backup_boot0,
};

sunxi_flash_desc sunxi_ufss_desc = {
    .probe = sunxi_sprite_ufs_probe,
    .init = sunxi_sprite_ufs_init,
    .exit = sunxi_sprite_ufs_exit,
    .read = sunxi_sprite_ufs_read,
    .write = sunxi_sprite_ufs_write,
    .erase = sunxi_sprite_ufs_erase,
    .force_erase = sunxi_sprite_ufs_force_erase,
    .flush = sunxi_sprite_ufs_flush,
    .size = sunxi_sprite_ufs_size,
#ifdef CONFIG_SUNXI_SECURE_STORAGE
    .secstorage_read = sunxi_ufs_secure_storage_read,
    .secstorage_write = sunxi_ufs_secure_storage_write,
#endif
    .phyread = sunxi_sprite_ufs_phyread,
    .phywrite = sunxi_sprite_ufs_phywrite,
    .phyerase = sunxi_sprite_ufs_phyerase,
    .download_spl = sunxi_sprite_ufs_download_spl,
    .download_boot_param = sunxi_sprite_ufs_download_boot_param,
    .download_toc = sunxi_sprite_ufs_download_toc,
};


sunxi_flash_desc sunxi_ufs_blk_desc = {
    .probe = sunxi_flash_ufs_probe,
    .init = sunxi_flash_ufs_init,
    .exit = sunxi_flash_ufs_exit,
    .erase = sunxi_sprite_ufs_erase,
    .flush = sunxi_flash_ufs_flush,
    .size = sunxi_ufs_blk_size,
    .phyread = sunxi_ufs_blk_phyread,
    .phywrite = sunxi_ufs_blk_phywrite,
};



//-----------------------------------end
//------------------------------------------------------

int sunxi_ufs_init_for_boot(int workmode, int card_no)
{
	debug("try to init ufs\n");
	int ret = 0;
	if (!ufs_has_init) {
		ufs_has_init = 1;
		ret = sunxi_ufs_global_init();
	} else {
		puts("ufs has init\n");
		return 0;
	}

	if (ret) {
		printf("fail to init ufs\n");
		return -1;
	}
	debug("ufs %d init ok\n", card_no);
	return 0;
}

int sunxi_ufs_init_for_sprite(int workmode, int card_no)
{
	int ret = sunxi_ufs_init_for_boot(workmode, card_no);
	if (ret)
		return ret;
	set_boot_storage_type(STORAGE_UFS);
	debug("sunxi sprite has installed UFS function\n");
	return 0;
}

/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
/*
int ufs_read_boot0(void *buffer, uint length, int backup_id)
{
	int ret;
	int storage_type;
	storage_type = get_boot_storage_type();
	if ((STORAGE_EMMC == storage_type) || (STORAGE_EMMC0 == storage_type)) {
		ret = sunxi_sprite_phyread(
			backup_id == 0 ?
				BOOT0_SDMMC_START_ADDR :
				CONFIG_SUNXI_BOOT0_SDMMC_BACKUP_START_ADDR,
			(length + 511) / 512, buffer);
	} else {
		ret = sunxi_sprite_phyread(BOOT0_EMMC3_BACKUP_START_ADDR,
					   (length + 511) / 512, buffer);
	}

	if (!ret) {
		printf("%s: call fail\n", __func__);
		return -1;
	}
	return 0;
}
*/
#if 1
static int check_secure_storage_key(unsigned char *buffer)
{
	store_object_t *obj = (store_object_t *)buffer;

	if (obj->magic != STORE_OBJECT_MAGIC) {
		printf("Input object magic fail [0x%x]\n", obj->magic);
		return -1;
	}

	if (obj->crc != crc32(0, (void *)obj, sizeof(*obj) - 4)) {
		printf("Input object crc fail [0x%x]\n", obj->crc);
		return -1;
	}
	return 0;
}

static int sunxi_ufs_secure_storage_read_key(int item, unsigned char *buf,
				       unsigned int len)
{
	unsigned char *align;
	unsigned int blkcnt;
	int ret, workmode;

	if (PT_TO_PHU(buf) % 32) {
		align = (unsigned char *)((PT_TO_PHU(_inner_buffer) + 0x20) &
					  (~0x1f));
		memset(align, 0, 4096);
	} else {
		align = buf;
	}

	blkcnt = (len + 511) / 512;

	workmode = uboot_spare_head.boot_data.work_mode;
	if (workmode == WORK_MODE_BOOT ||
	    workmode == WORK_MODE_SPRITE_RECOVERY) {
		ret = sunxi_flash_ufs_secread(item, align, blkcnt);
	} else if ((workmode & WORK_MODE_PRODUCT) || (workmode == 0x30)) {
		ret = sunxi_sprite_ufs_secread(item, align, blkcnt);
	} else {
		pr_error("workmode=%d is err\n", workmode);
		return -1;
	}
	if (!ret) {
		/*check copy 0 */
		if (!check_secure_storage_key(align)) {
			pr_debug("the secure storage item%d copy0 is good\n",
			       item);
			goto ok; /*copy 0 pass*/
		}
		pr_force("the secure storage item%d copy0 is bad\n", item);
	}

	// read backup
	memset(align, 0x0, len);
	pr_debug("read item%d copy1\n", item);
	if (workmode == WORK_MODE_BOOT ||
	    workmode == WORK_MODE_SPRITE_RECOVERY) {
		ret = (sunxi_flash_ufs_secread_backup(item, align, blkcnt) ==
		       blkcnt)
			  ? 0
			  : -1;
	} else if ((workmode & WORK_MODE_PRODUCT) || (workmode == 0x30)) {
		ret = sunxi_sprite_ufs_secread_backup(item, align, blkcnt);
	} else {
		pr_error("workmode=%d is err\n", workmode);
		return -1;
	}
	if (!ret) {
		/*check copy 1 */
		if (!check_secure_storage_key(align)) {
			pr_debug("the secure storage item%d copy1 is good\n",
			       item);
			goto ok; /*copy 1 pass*/
		}
		pr_force("the secure storage item%d copy1 is bad\n", item);
	}

	pr_error("sunxi_secstorage_read fail\n");
	return -1;

ok:
	if (PT_TO_PHU(buf) % 32)
		memcpy(buf, align, len);
	return 0;
}

static int sunxi_ufs_secure_storage_read_map(int item, unsigned char *buf,
				       unsigned int len)
{
	unsigned char *align;
	unsigned int blkcnt;
	int ret, workmode;

	if (PT_TO_PHU(buf) % 32) {
		align = (unsigned char *)((PT_TO_PHU(_inner_buffer) + 0x20) &
					  (~0x1f));
		memset(align, 0, 4096);
	} else {
		align = buf;
	}

	blkcnt = (len + 511) / 512;

	pr_msg("read item%d copy0\n", item);
	workmode = uboot_spare_head.boot_data.work_mode;
	if (workmode == WORK_MODE_BOOT ||
	    workmode == WORK_MODE_SPRITE_RECOVERY) {
		ret = sunxi_flash_ufs_secread(item, align, blkcnt);
	} else if ((workmode & WORK_MODE_PRODUCT) || (workmode == 0x30)) {
		ret = sunxi_sprite_ufs_secread(item, align, blkcnt);
	} else {
		pr_error("workmode=%d is err\n", workmode);
		return -1;
	}
	if (!ret) {
		/*read ok*/
		ret = check_secure_storage_map(align);
		if (ret == 0) {
			pr_msg("the secure storage item0 copy0 is good\n");
			goto ok; /*copy 0 pass*/
		} else {
			pr_force("the secure storage item0 copy0 %s is bad\n",
				(ret == 2 ? "magic" : "crc"));
		}
	}

	// read backup
	memset(align, 0x0, len);
	pr_debug("read item%d copy1\n", item);
	if (workmode == WORK_MODE_BOOT ||
	    workmode == WORK_MODE_SPRITE_RECOVERY) {
		ret = sunxi_flash_ufs_secread(item, align, blkcnt);
	} else if ((workmode & WORK_MODE_PRODUCT) || (workmode == 0x30)) {
		ret = sunxi_sprite_ufs_secread(item, align, blkcnt);
	} else {
		pr_debug("workmode=%d is err\n", workmode);
		return -1;
	}
	if (!ret) {
		ret = check_secure_storage_map(align);
		if (ret == 0) {
			pr_debug("the secure storage item0 copy1 is good\n");
		} else {
			pr_force("the secure storage item0 copy1 %s is bad\n",
				(ret == 2 ? "magic" : "crc"));
			memset(align, 0x0, len);
		}
		goto ok;
	}
	pr_error("unknown error happen in item 0 read\n");
	return -1;

ok:
	if (PT_TO_PHU(buf) % 32)
		memcpy(buf, align, len);
	return 0;
}
#endif
#if defined(CONFIG_SUPPORT_EMMC_RPMB)
int sunxi_mmc_rpmb_burn_key(void *key)
{
	char original_part;
	int ret = -1;
#define RPMB_SZ_DATA 256
	char read_buf[RPMB_SZ_DATA];
	if (mmc_boot == NULL)
		return -4;
#ifndef CONFIG_BLK
	original_part = mmc_boot->block_dev.hwpart;
#else
	original_part = mmc_get_blk_desc(mmc_boot)->hwpart;
#endif
	if (blk_select_hwpart_devnum(IF_TYPE_MMC, board_mmc_get_num(),
				     MMC_PART_RPMB) != 0)
		return -1;

	if (mmc_rpmb_set_key(mmc_boot, key)) {
		if (1 == (mmc_rpmb_read(mmc_boot, read_buf, 0, 1, key))) {
			ret = -3;
		} else {
			ret = -2;
		}
	}

	/* Return to original partition */
	blk_select_hwpart_devnum(IF_TYPE_MMC, board_mmc_get_num(),
				 original_part);
	return ret;
}
#endif
