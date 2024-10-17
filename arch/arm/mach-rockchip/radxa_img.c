/*
 * Copyright (c) 2024 Radxa Computer (Shenzhen) Co., Ltd.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <fat.h>
#include <fs.h>
#include <boot_rkimg.h>
#include <malloc.h>
#include <asm/arch/radxa_img.h>

int radxa_read_bmp_file(void *buf, const char *name) {
	struct blk_desc *desc = rockchip_get_bootdev();
	disk_partition_t part;
	loff_t actread, len;

	if (!desc)
		return -ENODEV;

	if (part_get_info_by_name(desc, "config", &part) < 0) {
		printf("Failed to get info for config partition\n");
		return -ENODEV;
	}

	if (fat_set_blk_dev(desc, &part) != 0) {
		printf("Failed to set block device for FAT\n");
		return -ENODEV;
	}

	if (fat_exists(name) != 1) {
		printf("File %s does not exist\n", name);
		return -ENOENT;
	}

	if (fat_size(name, &len) < 0) {
		printf("Failed to get file size for %s\n", name);
		return -ENOENT;
	}

	if (!buf) {
		printf("Failed to allocate memory for file\n");
		free(buf);
		return -ENOMEM;
	}

	actread = file_fat_read(name, buf, len);
	if (actread != len) {
		printf("Failed to read file %s: read %lld, expected %lld\n", name, actread, len);
		free(buf);
		return -EIO;
	}

	return len;
}
