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

#ifdef CONFIG_SUNXI_DUAL_STORAGE
#include <sunxi_dual_storage.h>
#endif

__attribute__((section(".data"))) static sunxi_flash_desc *current_flash;

__attribute__((section(".data"))) static sunxi_flash_desc *sprite_flash;

static int sunxi_flash_init_blk(void);


int sunxi_flash_read(uint start_block, uint nblock, void *buffer)
{
	return current_flash->read(start_block, nblock, buffer);
}

int sunxi_flash_write(uint start_block, uint nblock, void *buffer)
{
	return current_flash->write(start_block, nblock, buffer);
}

int sunxi_flash_flush(void)
{
	return current_flash->flush();
}

int sunxi_flash_erase(int erase, void *mbr_buffer)
{
	return current_flash->erase(erase, mbr_buffer);
}

int sunxi_flash_force_erase(void)
{
	return current_flash->force_erase();
}

int sunxi_flash_phyread(uint start_block, uint nblock, void *buffer)
{
	return current_flash->phyread(start_block, nblock, buffer);
}

int sunxi_flash_phywrite(uint start_block, uint nblock, void *buffer)
{
	return current_flash->phywrite(start_block, nblock, buffer);
}

uint sunxi_flash_size(void)
{
	return current_flash->size();
}

int sunxi_secstorage_read(int item, unsigned char *buf, unsigned int len)
{
	return current_flash->secstorage_read(item, buf, len);
}

int sunxi_secstorage_write(int item, unsigned char *buf, unsigned int len)
{
	if (current_flash->secstorage_fast_write)
		return current_flash->secstorage_fast_write(item, buf, len);
	else
		return current_flash->secstorage_write(item, buf, len);
}

int sunxi_secstorage_flush(void)
{
	if (current_flash->secstorage_flush)
		return current_flash->secstorage_flush();
	return 0;
}

int sunxi_flash_is_support_fast_write(int flash_type)
{
	int ret = false;

	switch (flash_type) {
#ifdef CONFIG_SUNXI_NAND
	case STORAGE_NAND:
		if (sunxi_nand_desc.secstorage_fast_write)
			ret = true;
		break;
#endif
	default:
		break;
	}

	return ret;
}
int sunxi_flash_download_spl(unsigned char *buf, int len, unsigned int ext)
{
	return current_flash->download_spl(buf, len, ext);
}

int sunxi_flash_download_boot_param(void)
{
	return current_flash->download_boot_param();
}

int sunxi_flash_download_toc(unsigned char *buf, int len, unsigned int ext)
{
	return current_flash->download_toc(buf, len, ext);
}

int sunxi_flash_init(void)
{
	int boot_mode = get_boot_work_mode() == WORK_MODE_BOOT ? 1 : 0;
	return current_flash->init(boot_mode, 0);
}

int sunxi_flash_exit(int force)
{
	return current_flash->exit(force);
}

int sunxi_flash_write_end(void)
{
	int ret = 0;

	if (current_flash->write_end != NULL)
		ret = current_flash->write_end();

	return ret;
}

int sunxi_flash_erase_area(uint start_block, uint nblock)
{
	int ret = 0;
	if (current_flash->erase_area != NULL)
		ret = current_flash->erase_area(start_block, nblock);
	return ret;
}

int sunxi_sprite_read(uint start_block, uint nblock, void *buffer)
{
	return sprite_flash->read(start_block, nblock, buffer);
}

int sunxi_sprite_write(uint start_block, uint nblock, void *buffer)
{
	return sprite_flash->write(start_block, nblock, buffer);
}

int sunxi_sprite_flush(void)
{
	return sprite_flash->flush();
}

int sunxi_sprite_erase(int erase, void *mbr_buffer)
{
	return sprite_flash->erase(erase, mbr_buffer);
}

int sunxi_sprite_force_erase(void)
{
	return sprite_flash->force_erase();
}

int sunxi_sprite_phyread(uint start_block, uint nblock, void *buffer)
{
	return sprite_flash->phyread(start_block, nblock, buffer);
}

int sunxi_sprite_phywrite(uint start_block, uint nblock, void *buffer)
{
	return sprite_flash->phywrite(start_block, nblock, buffer);
}

int sunxi_sprite_phyerase(unsigned int start_block, unsigned int nblock, void *skip)
{
	return sprite_flash->phyerase(start_block, nblock, skip);
}

uint sunxi_sprite_size(void)
{
	return sprite_flash->size();
}

int sunxi_sprite_secstorage_read(int item, unsigned char *buf, unsigned int len)
{
	return sprite_flash->secstorage_read(item, buf, len);
}

int sunxi_sprite_secstorage_write(int item, unsigned char *buf,
				  unsigned int len)
{
	return sprite_flash->secstorage_write(item, buf, len);
}

int sunxi_sprite_download_spl(unsigned char *buf, int len, unsigned int ext)
{
	return sprite_flash->download_spl(buf, len, ext);
}

int sunxi_sprite_download_boot_param(void)
{
	return sprite_flash->download_boot_param();
}

int sunxi_sprite_download_toc(unsigned char *buf, int len, unsigned int ext)
{
	return sprite_flash->download_toc(buf, len, ext);
}

int sunxi_sprite_upload_toc(void *buf, uint len)
{
	return sprite_flash->upload_toc(buf, len);
}

int sunxi_sprite_init(int boot_mode)
{
	return sprite_flash->init(boot_mode, 2);
}

int sunxi_sprite_exit(int force)
{
	return sprite_flash->exit(force);
}

int sunxi_sprite_write_end(void)
{
	int ret = 0;

	if (sprite_flash->write_end != NULL)
		ret = sprite_flash->write_end();

	return ret;
}

int sunxi_sprite_erase_area(uint start_block, uint nblock)
{
	int ret = 0;

	if (sprite_flash->erase_area != NULL)
		ret = sprite_flash->erase_area(start_block, nblock);

	return ret;
}
/* sunxi_flash_hook_init apply to boot for burn key*/
int sunxi_flash_hook_init(void)
{
#ifdef CONFIG_SUNXI_NAND
	int storage_type = get_boot_storage_type();

	if (storage_type == STORAGE_NAND)
		current_flash = &sunxi_nand_desc;

	return 0;
#else
	return -1;
#endif
}

int sunxi_flash_boot_init(int storage_type, int workmode)
{
	int state;

	switch (storage_type) {
#ifdef CONFIG_SUNXI_NAND
	case STORAGE_NAND:
	case STORAGE_SPI_NAND: {
		int boot_mode = workmode == WORK_MODE_SPRITE_RECOVERY ? 0 : 1;
		current_flash = &sunxi_nand_desc;
		state	 = current_flash->init(boot_mode, 0);
#ifdef CONFIG_SUNXI_UBIFS
		if (nand_use_ubi()) {
			ubi_nand_probe_uboot();
			ubi_nand_attach_mtd();
		}
#endif
	} break;
#endif

#ifdef CONFIG_SUNXI_SDMMC
	case STORAGE_SD:
	case STORAGE_EMMC:
	case STORAGE_EMMC0:
	case STORAGE_EMMC3: {
		//sdmmc handle init
		int card_no = 0;
		if (storage_type == STORAGE_SD)
			card_no = 0;
		else if (storage_type == STORAGE_EMMC)
			card_no = 2;
		else if (storage_type == STORAGE_EMMC0)
			card_no = 0;
		else
			card_no = 3;
		if (workmode == WORK_MODE_CARD_PRODUCT)
			current_flash = &sunxi_sdmmcs_desc;
		else
			current_flash = &sunxi_sdmmc_desc;
		state		      = current_flash->init(workmode, card_no);

#ifdef CONFIG_SUNXI_RECOVERY_BOOT0_COPY0
#ifdef CONFIG_SUNXI_DUAL_STORAGE
		if (get_boot_storage_type() == storage_type)
#endif
		sunxi_flash_mmc_recover_boot0_copy0();
#endif
	} break;
#endif


#ifdef CONFIG_SUNXI_UFS
	case STORAGE_UFS: {
		if (workmode == WORK_MODE_CARD_PRODUCT)
			current_flash = &sunxi_ufss_desc;
		else
			current_flash = &sunxi_ufs_desc;
		state		      = current_flash->init(workmode, 0);

#ifdef CONFIG_SUNXI_RECOVERY_BOOT0_COPY0
		sunxi_flash_ufs_recover_boot0_copy0();
#endif
	} break;
#endif


#ifdef CONFIG_SUNXI_SPINOR
	case STORAGE_NOR: {
		current_flash = &sunxi_spinor_desc;
		state	 = current_flash->init(workmode, 0);
	} break;
#endif
	default: {
		pr_err("not support\n");
		state = -1;
	} break;
	}

	if (state != 0) {
		return -1;
	}

	sprite_flash = current_flash;
	tick_printf("sunxi flash init ok\n");

	if (current_flash->update_backup_boot0) {
		current_flash->update_backup_boot0();
	}
	return 0;
}

int sunxi_flash_probe(void)
{
	int state = 0;

//try emmc, nand, spi-nor
	do {
#ifdef CONFIG_SUNXI_SDMMC
		current_flash = &sunxi_sdmmcs_desc;
		state	 = current_flash->probe();
		if (state == 0)
			break;
		printf("try emmc fail\n");
#endif

#ifdef CONFIG_SUNXI_UFS
		current_flash = &sunxi_ufss_desc;
		state	 = current_flash->probe();
		if (state == 0)
			break;
		printf("try UFS fail\n");
#endif



#ifdef CONFIG_SUNXI_NAND
		current_flash = &sunxi_nand_desc;
		state	 = current_flash->probe();
		if (state == 0)
			break;
		printf("try nand fail\n");
#endif

#ifdef CONFIG_SUNXI_SPINOR
		current_flash = &sunxi_spinor_desc;
		state	 = current_flash->probe();
		if (state == 0)
			break;

		printf("try spinor fail\n");
#endif

		if (state != 0) {
			return -1;
		}

	} while(0);
	sprite_flash = current_flash;
#ifdef CONFIG_SUNXI_SDMMC
	if (get_boot_work_mode() == WORK_MODE_CARD_PRODUCT) {
		current_flash = &sunxi_sdmmc_desc;
		if (current_flash->init(0, 0))
			return -1;
	}
#endif

	return 0;
}

char global_boardflash;
/*
bit 0 -- nand exist
	1 -- emmc exist
	2 -- nor exist
	[3,7]resvere
*/
#define BAORD_NAND_MASK 0x1
#define BAORD_EMMC_MASK 0x2
#define BAORD_NOR_MASK 0x4
#define BAORD_UFS_MASK 0x8

void set_board_flash_type(SUNXI_BOOT_STORAGE boardflash)
{
	switch (boardflash) {
	case STORAGE_NAND:
		global_boardflash |= BAORD_NAND_MASK;
		break;
	case STORAGE_EMMC:
		global_boardflash |= BAORD_EMMC_MASK;
		break;
	case STORAGE_NOR:
		global_boardflash |= BAORD_NOR_MASK;
		break;
	case STORAGE_UFS:
		global_boardflash |= BAORD_UFS_MASK;
		break;
	default:
		printf("no support board flash");
	}
}

int get_board_flash_type_exist(SUNXI_BOOT_STORAGE boardflash)
{
	switch (boardflash) {
	case STORAGE_NAND:
		return global_boardflash & BAORD_NAND_MASK;
	case STORAGE_EMMC:
		return global_boardflash & BAORD_EMMC_MASK;
	case STORAGE_NOR:
		return global_boardflash & BAORD_NOR_MASK;
	case STORAGE_UFS:
		return global_boardflash & BAORD_UFS_MASK;
	default:
		return 0;
	}
}

int sunxi_board_flash_probe(void)
{
	int record_workmode	= 0;
	int record_storage_type = 0;
	int state		= 0;
	sunxi_flash_desc *board_flash;

	record_workmode	    = get_boot_work_mode();
	record_storage_type = get_boot_storage_type();

	set_boot_work_mode(WORK_MODE_CARD_PRODUCT);

	do {
#ifdef CONFIG_SUNXI_SDMMC
		board_flash = &sunxi_sdmmcs_desc;
		state	    = board_flash->probe();
		if (state == 0) {
			set_board_flash_type(STORAGE_EMMC);
		}
		printf("try emmc fail\n");
#endif

#ifdef CONFIG_SUNXI_UFS
		board_flash = &sunxi_ufss_desc;
		state	    = board_flash->probe();
		if (state == 0) {
			set_board_flash_type(STORAGE_UFS);
		}
		printf("try UFS fail\n");
#endif



#ifdef CONFIG_SUNXI_NAND
		board_flash = &sunxi_nand_desc;
		state	    = board_flash->probe();
		if (state == 0) {
			set_board_flash_type(STORAGE_NAND);
		}
		printf("try nand fail\n");
#endif

#ifdef CONFIG_SUNXI_SPINOR
		board_flash = &sunxi_spinor_desc;
		state	    = board_flash->probe();
		if (state == 0) {
			set_board_flash_type(STORAGE_NOR);
		}

		printf("try spinor fail\n");
#endif

		if (state != 0) {
			printf("try flash fail\n");
		}

	} while (0);

	set_boot_work_mode(record_workmode);
	set_boot_storage_type(record_storage_type);
	return 0;
}

int sunxi_flash_sprite_init(int storage_type, int workmode)
{
	int state = 0;

	switch (storage_type) {
#ifdef CONFIG_SUNXI_NAND
	case STORAGE_NAND:
	case STORAGE_SPI_NAND: {
		current_flash = &sunxi_nand_desc;
		state	 = current_flash->probe();
	} break;
#endif

#ifdef CONFIG_SUNXI_SDMMC
	case STORAGE_SD:
	case STORAGE_EMMC:
	case STORAGE_EMMC0:
	case STORAGE_EMMC3: {
		current_flash = &sunxi_sdmmcs_desc;
		state	 = current_flash->probe();
	} break;
#endif

#ifdef CONFIG_SUNXI_SPINOR
	case STORAGE_NOR: {
		current_flash = &sunxi_spinor_desc;
		state	 = current_flash->probe();
	} break;
#endif
	default: {
		pr_err("not support\n");
		state = -1;
	} break;
	}

	if (state != 0) {
		return -1;
	}

	sprite_flash = current_flash;
#ifdef CONFIG_SUNXI_SDMMC
	if (workmode == WORK_MODE_CARD_PRODUCT) {
		current_flash = &sunxi_sdmmc_desc;
		if (current_flash->init(0, 0))
			return -1;
	}
#endif

	return 0;
}

int sunxi_flash_sprite_switch(int storage_type, int workmode)
{
	switch (storage_type) {
#ifdef CONFIG_SUNXI_NAND
	case STORAGE_NAND:
	case STORAGE_SPI_NAND: {
		current_flash = &sunxi_nand_desc;
	} break;
#endif

#ifdef CONFIG_SUNXI_SDMMC
	case STORAGE_SD:
	case STORAGE_EMMC:
	case STORAGE_EMMC0:
	case STORAGE_EMMC3: {
		current_flash = &sunxi_sdmmcs_desc;
	} break;
#endif

#ifdef CONFIG_SUNXI_SPINOR
	case STORAGE_NOR: {
		current_flash = &sunxi_spinor_desc;
	} break;
#endif
	default: {
		pr_err("not support\n");
		return -1;
	} break;
	}
	sprite_flash = current_flash;

#ifdef CONFIG_SUNXI_SDMMC
	if (workmode == WORK_MODE_CARD_PRODUCT) {
		current_flash = &sunxi_sdmmc_desc;
	}
#endif

	return 0;
}

int sunxi_flash_init_ext(void)
{
	int workmode     = 0;
	int storage_type = 0;
	int state	= 0;
	uint32_t uboot_dragon_board_test = 0;
	workmode     = get_boot_work_mode();
	storage_type = get_boot_storage_type();

	tick_printf("workmode = %d,storage type = %d\n", workmode, storage_type);

	if (workmode == WORK_MODE_USB_DEBUG) {
		return 0;
	} else if (workmode == WORK_MODE_BOOT ||
		   workmode == WORK_MODE_SPRITE_RECOVERY) {
#ifdef CONFIG_SUNXI_DUAL_STORAGE
		state = sunxi_dual_storage_handle(SUNXI_DUAL_STORAGE_BOOT, workmode);
		if (state == 0)
			goto out;
#endif
		state = sunxi_flash_boot_init(storage_type, workmode);
		if (storage_type == STORAGE_SD) {
			extern int get_dragonboard_test(
				struct fdt_header *point_fdt,
				uint32_t *dragon_board_test);
			get_dragonboard_test(working_fdt,
					     &uboot_dragon_board_test);
			if (uboot_dragon_board_test == 1)
				sunxi_board_flash_probe();
		}
	} else if ((workmode & WORK_MODE_PRODUCT) || (workmode == 0x30)) {
#ifdef CONFIG_SUNXI_DUAL_STORAGE
		state = sunxi_dual_storage_handle(SUNXI_DUAL_STORAGE_SPRITE, workmode);
		if (state == 0)
			goto out;
#endif
		state = sunxi_flash_probe();
	} else if (workmode & WORK_MODE_UPDATE) {
	} else {
	}
#ifdef CONFIG_SUNXI_DUAL_STORAGE
out:
#endif
	//init blk dev
	sunxi_flash_init_blk();

	return state;
}


int sunxi_flash_upload_boot0(char *buffer, int size, int backup_id)
{
	int storage_type = 0;
	int ret = 0;
	storage_type = get_boot_storage_type();
	switch (storage_type) {
#ifdef CONFIG_SUNXI_NAND
	case STORAGE_NAND:
		ret = nand_read_boot0(buffer, size);
		break;
#endif
#ifdef CONFIG_SUNXI_SDMMC
	case STORAGE_EMMC:
	case STORAGE_EMMC3:
		ret = card_read_boot0(buffer, size, backup_id);
		break;
#endif
#ifdef CONFIG_SUNXI_UFS
	case STORAGE_UFS:
		ret = card_read_boot0(buffer, size, backup_id);
		break;
#endif


	default:
		pr_debug("%s:not support storage type %d\n", __func__, storage_type);
		ret = -1;
		break;
}

return ret;
}

int sunxi_flash_get_boot1_size(void)
{

	int ret = 0;
	int length = nand_get_uboot_total_len();
	unsigned char *boot1 = malloc_align(length, 64);
	sbrom_toc1_head_info_t *toc1 = (sbrom_toc1_head_info_t *)boot1;


	memset(boot1, 0x00, length);
	ret = nand_read_uboot_data(boot1, length);
	if (ret != 0) {
		printf("read uboot fail\n");
		return -1;
	}

	length = toc1->valid_len;
	printf("ubootsize: %d\n", length);

	return length;
}

int sunxi_flash_get_boot0_size(int backup_id)
{
	/* note:page align for nand */
	char *boot_buffer = NULL;
	int size;
	int ret = 0;

	boot_buffer = memalign(CONFIG_SYS_CACHELINE_SIZE, 32 * 1024);
	if (boot_buffer == NULL) {
		pr_err("%s:alloc memory fail\n", __func__);
		return -1;
	}
	/* read boot head */
	ret = sunxi_flash_upload_boot0(boot_buffer, 32 * 1024, backup_id);
	if (ret) {
		pr_debug("%s: get boot0 head fail\n", __func__);
		free(boot_buffer);
		return -1;
	}
	/* get boot size */
	if (SUNXI_NORMAL_MODE == sunxi_get_securemode()) {
		boot0_file_head_t *boot0 = (boot0_file_head_t *)boot_buffer;
		size			 = boot0->boot_head.length;
	} else {
		toc0_private_head_t *toc0 = (toc0_private_head_t *)boot_buffer;
		size			  = toc0->length;
	}
	free(boot_buffer);
	return size;
}


#ifdef CONFIG_SUNXI_OTA_TURNNING
/* boot0 buffer malloced in this api, caller should release it when it's done'*/
static int sunxi_try_boot0_backup(char **load_buffer, int backup_id)
{
	toc0_private_head_t *toc0;
	boot0_file_head_t *boot0;
	int ret;
	int size	  = 0;
	char *boot_buffer = NULL;
	size		  = sunxi_flash_get_boot0_size(backup_id);
	if (size <= 0) {
		pr_err("%s: get boot size error\n", __func__);
		return -1;
	}
	pr_err("boot0 size:%d\n", size);
	boot_buffer = memalign(CONFIG_SYS_CACHELINE_SIZE, size);
	if (!boot_buffer) {
		pr_err("%s:alloc memory fail\n", __func__);
		return -1;
	}
	/* read total boot data */
	ret = sunxi_flash_upload_boot0(boot_buffer, size, backup_id);
	if (ret) {
		pr_err("%s:upload boot0 fail\n", __func__);
		return -1;
	}
	if (sunxi_get_secureboard()) {
		toc0 = (toc0_private_head_t *)boot_buffer;
		printf("size %d\n", toc0->length);
		if (sunxi_sprite_verify_checksum(boot_buffer, toc0->length,
						 toc0->check_sum)) {
			pr_err("%s:verify backup %d fail\n", __func__,
			       backup_id);
			return -1;
		}
	} else {
		boot0 = (boot0_file_head_t *)boot_buffer;
		printf("size %d\n", boot0->boot_head.length);
		if (sunxi_sprite_verify_checksum(boot_buffer,
						 boot0->boot_head.length,
						 boot0->boot_head.check_sum)) {
			pr_err("%s:verify backup %d fail\n", __func__,
			       backup_id);
			return -1;
		}
	}
	*load_buffer = boot_buffer;
	return 0;
}

int sunxi_flash_update_boot0(void)
{
	int storage_type  = 0;
	char *boot_buffer = NULL;
	int ret		  = 0;
	int backup_count  = 1;
	int backup_id;

	storage_type = get_boot_storage_type();
	if ((storage_type == STORAGE_EMMC)) {
		backup_count = 2;
	} else {
		backup_count = 1;
	}

	if (sunxi_try_boot0_backup(&boot_buffer, sunxi_get_active_boot0_id())) {
		/*somehow active boot0 curruptted, try all back up to find a valid one*/
		for (backup_id = 0; backup_id < backup_count; backup_id++) {
			if (sunxi_try_boot0_backup(&boot_buffer, backup_id))
				continue;
			else
				break;
		}
		if (backup_id == backup_count) {
			goto _UPDATE_ERROR_;
		}
	}

	ret = sunxi_sprite_download_boot0(boot_buffer, storage_type);
	if (ret) {
		pr_err("%s:update boot0 for ota fail\n", __func__);
	}
	free(boot_buffer);
	/* sunxi_flash_flush(); */

	return ret;
_UPDATE_ERROR_:
	if (boot_buffer)
		free(boot_buffer);
	return -1;
}
#endif

#ifdef CONFIG_SUNXI_FDT_SAVE
int read_boot_package(int storage_type, void *package_buf)
{

	int total_length = 0;
	int read_len = uboot_spare_head.boot_data.boot_package_size;
	int ret = 0;
	sbrom_toc1_head_info_t *toc1_head = NULL;

#ifndef CONFIG_SUNXI_UBIFS
	extern  int nand_read_uboot_data(unsigned char *buf, unsigned int len);
#endif
	debug("boot package size: 0x%x\n", read_len);

	switch (storage_type) {
#ifdef CONFIG_SUNXI_NAND
	case STORAGE_NAND:
#ifdef CONFIG_SUNXI_UBIFS
		ret = sunxi_sprite_upload_uboot(package_buf, read_len);
#else
		ret = nand_read_uboot_data(package_buf, read_len);
#endif
		break;
#endif
#ifdef CONFIG_SUNXI_SDMMC
	case STORAGE_EMMC:
	case STORAGE_EMMC0:
	case STORAGE_SD:
	case STORAGE_EMMC3:
		ret = sunxi_flash_phyread(sunxi_flashmap_offset(FLASHMAP_SDMMC, TOC1), read_len/512, package_buf);
		break;
#endif

#ifdef CONFIG_SUNXI_UFS
	case STORAGE_UFS:
		ret = sunxi_flash_phyread(sunxi_flashmap_offset(FLASHMAP_UFS, TOC1)/8, read_len/4096, package_buf);
		break;
#endif


#ifdef CONFIG_SUNXI_SPINOR
	case STORAGE_NOR:
		ret = sunxi_flash_phyread(sunxi_flashmap_offset(FLASHMAP_SPI_NOR, TOC1), read_len/512, package_buf);
		break;
#endif
	default:
		pr_error("%s:not support storage type %d\n", __func__, storage_type);
		ret = -1;
		break;
	}
	toc1_head = (struct sbrom_toc1_head_info *)package_buf;
	if (toc1_head->magic != TOC_MAIN_INFO_MAGIC) {
		pr_error("toc1 magic error\n");
		return -1;
	}
	total_length = toc1_head->valid_len;

	debug("read uboot from flash: ret %d\n", ret);
	return total_length;
}

int save_partition_fdt_to_flash(void *fdt_buf, size_t fdt_size)
{
	int ret;
	disk_partition_t info = { 0 };

	ret = sunxi_partition_get_info_byname("dtb", (uint *)&info.start, (uint *)&info.size);
	if (ret) {
		pr_err("fail to get part info\n");
		return -1;
	}

	sunxi_flash_write(info.start, info.size, (void *)fdt_buf);
	sunxi_flash_flush();

	return 0;

}

int save_fdt_to_flash(void *fdt_buf, size_t fdt_size)
{
	int package_size;
	int i = 0;
	char *package_buf = NULL;
	int   package_buf_size;
	int   find_flag = 0;
	u32   dtb_base = 0;
	int   storage_type = 0;
	int   ret = -1;

	struct sbrom_toc1_head_info  *toc1_head = NULL;
	struct sbrom_toc1_item_info  *item_head = NULL;
	struct sbrom_toc1_item_info  *toc1_item = NULL;

	storage_type = get_boot_storage_type();

	/*1M buffer*/
	if (storage_type == STORAGE_NOR || storage_type == STORAGE_SPI_NAND) {
		package_buf_size = 1 << 20;
	} else {
		/*10M buffer*/
		package_buf_size = 10 << 20;
	}
	package_buf = (char *)memalign(CONFIG_SYS_CACHELINE_SIZE, package_buf_size);
	if (package_buf == NULL)
		return -1;

	memset(package_buf, 0, package_buf_size);
	package_size = read_boot_package(storage_type, package_buf);
	if (package_size <= 0) {
		goto _UPDATE_END;
	}

	toc1_head = (struct sbrom_toc1_head_info *)package_buf;
	item_head = (struct sbrom_toc1_item_info *)(package_buf + sizeof(struct sbrom_toc1_head_info));
	toc1_item = item_head;
	for (i = 0; i < toc1_head->items_nr; i++, toc1_item++) {
		debug("Entry_name		  = %s\n",	 toc1_item->name);
		if (strncmp(toc1_item->name, ITEM_DTB_NAME, sizeof(ITEM_DTB_NAME)) == 0) {
			find_flag = 1;
			break;
		}
	}
	if (!find_flag) {
		pr_error("error:can't find dtb\n");
		goto _UPDATE_END;
	}

	dtb_base = (u32)(package_buf + toc1_item->data_offset);
	if (fdt_check_header((void *)dtb_base)) {
		pr_error("%s: fdt header is error\n", __func__);
		goto _UPDATE_END;
	}

	if (fdt_size > CONFIG_SUNXI_DTB_RESERVE_SIZE) {
		pr_error("fdt size is too large\n");
		goto _UPDATE_END;
	}

	memcpy((void *)dtb_base, fdt_buf, fdt_size);

	ret = sunxi_sprite_download_uboot(package_buf, storage_type, 1);

_UPDATE_END:
	if (package_buf) {
		free(package_buf);
	}
	return ret;

}
#endif
static struct blk_desc sunxi_flash_blk_dev;

static unsigned long sunxi_block_read(struct blk_desc *block_dev,
				      lbaint_t start, lbaint_t blkcnt,
				      void *buffer)
{
	int storage_type = get_boot_storage_type();
	/*debug("addr = %d, len = %d\n", (uint)start, (uint)blkcnt);*/
	if (get_boot_work_mode() == WORK_MODE_CARD_PRODUCT ||
		(storage_type == STORAGE_SD) || (storage_type == STORAGE_EMMC)
			|| (storage_type == STORAGE_EMMC0)
			|| (storage_type == STORAGE_UFS))
		return sunxi_flash_phyread((uint)start, (uint)blkcnt, (void *)buffer);
	else
		return sunxi_flash_read((uint)start, (uint)blkcnt, (void *)buffer);
}

static unsigned long sunxi_block_write(struct blk_desc *block_dev,
				       lbaint_t start, lbaint_t blkcnt,
				       const void *buffer)
{
	int storage_type = get_boot_storage_type();
	/*debug("addr = %d, len = %d\n", (uint)start, (uint)blkcnt);*/
	if (get_boot_work_mode() == WORK_MODE_CARD_PRODUCT ||
		(storage_type == STORAGE_SD) || (storage_type == STORAGE_EMMC)
			|| (storage_type == STORAGE_EMMC0)
			|| (storage_type == STORAGE_UFS))
		return sunxi_flash_phywrite((uint)start, (uint)blkcnt, (void *)buffer);
	else
		return sunxi_flash_write((uint)start, (uint)blkcnt, (void *)buffer);
}

static int sunxi_flash_init_blk(void)
{
	debug("sunxi flash init uboot\n");
	sunxi_flash_blk_dev.if_type   = IF_TYPE_SUNXI_FLASH;
	sunxi_flash_blk_dev.part_type = PART_TYPE_EFI;
	sunxi_flash_blk_dev.devnum    = 0;
	sunxi_flash_blk_dev.lun       = 0;
	sunxi_flash_blk_dev.type      = 0;

	/* FIXME fill in the correct size (is set to 32MByte) */
	sunxi_flash_blk_dev.blksz       = 512;
	sunxi_flash_blk_dev.lba		= 0;
	sunxi_flash_blk_dev.removable   = 0;
	sunxi_flash_blk_dev.block_read  = sunxi_block_read;
	sunxi_flash_blk_dev.block_write = sunxi_block_write;

	/* Setup the universal parts of the block interface just once */

	return 0;
}

static int sunxi_flash_get_dev(int dev, struct blk_desc **descp)
{
	sunxi_flash_blk_dev.devnum = dev;
	sunxi_flash_blk_dev.lba    = sunxi_flash_size();

	*descp = &sunxi_flash_blk_dev;

	return 0;
}

U_BOOT_LEGACY_BLK(sunxi_flash) = {
	.if_typename   = "sunxi_flash",
	.if_type       = IF_TYPE_SUNXI_FLASH,
	.max_devs      = -1,
	.get_dev       = sunxi_flash_get_dev,
	.select_hwpart = NULL,
};
