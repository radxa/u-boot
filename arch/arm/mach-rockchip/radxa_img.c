/*
 * Copyright (c) 2024 Radxa Computer (Shenzhen) Co., Ltd.
 *
 * SPDX-License-Identifier:	 GPL-2.0+
 */

#include <asm/arch/radxa_img.h>
#include <fat.h>
#include <fs.h>
#include <boot_rkimg.h>
#include <malloc.h>
#include <stdlib.h>
#include <memalign.h>

#define MBR_SIGNATURE_OFFSET  0x1FE
#define MBR_SIGNATURE		 0xAA55
#define DOS_FS_TYPE_OFFSET	0x36
#define DOS_FS32_TYPE_OFFSET  0x52

#define VOLUME_LABEL16_OFFSET 0x2B
#define VOLUME_LABEL32_OFFSET 0x47

#define LABEL_LENGTH 11

int get_partition_count(struct blk_desc *dev_desc) {
	struct disk_partition info;
	int part = 1;
	int count = 0;

	while (part_get_info(dev_desc, part, &info) == 0) {
		count++;
		part++;
	}

	return count;
}

int get_fat_volume_label(struct blk_desc *dev_desc, disk_partition_t *part_info, char *vol_label) {
	void *buffer;
	int is_fat32 = 0;

	buffer = malloc_cache_aligned(dev_desc->blksz);
	if (!buffer) {
		printf("Error: Unable to allocate buffer\n");
		return -ENOMEM;
	}

	if (blk_dread(dev_desc, part_info->start, 1, buffer) != 1) {
		printf("Error: Unable to read boot sector\n");
		free(buffer);
		return -EIO;
	}
	unsigned char *buf = (unsigned char *)buffer;

	if (buf[MBR_SIGNATURE_OFFSET] != 0x55 || buf[MBR_SIGNATURE_OFFSET + 1] != 0xAA) {
		printf("Error: Invalid boot sector signature for partition.\n");
		free(buffer);
		return -EINVAL;
	}

	if (!memcmp(buffer + DOS_FS_TYPE_OFFSET, "FAT", 3)) {
		is_fat32 = 0;
	} else if (!memcmp(buffer + DOS_FS32_TYPE_OFFSET, "FAT32", 5)) {
		is_fat32 = 1;
	} else {
		printf("Unknown filesystem type on partition.\n");
		free(buffer);
		return -EINVAL;
	}

	int offset = is_fat32 ? VOLUME_LABEL32_OFFSET : VOLUME_LABEL16_OFFSET;
	memcpy(vol_label, buffer + offset, LABEL_LENGTH);
	vol_label[LABEL_LENGTH] = '\0';

	for (int i = LABEL_LENGTH - 1; i >= 0; i--) {
		if (vol_label[i] == ' ') {
			vol_label[i] = '\0';
		} else {
			break;
		}
	}

	free(buffer);
	return 0;
}

int radxa_read_bmp_file(void *buf, const char *name) {
	struct blk_desc *desc = rockchip_get_bootdev();
	disk_partition_t part_info;
	loff_t actread, len;
	int part_count, part_num;
	char volume_label[LABEL_LENGTH + 1];

	if (!desc) {
		printf("Error: No boot device found.\n");
		return -ENODEV;
	}

	part_count = get_partition_count(desc);
	if (part_count <= 0) {
		printf("No partitions found.\n");
		return -ENODEV;
	}

	for (part_num = 1; part_num <= part_count; part_num++) {
		if (part_get_info(desc, part_num, &part_info) != 0) {
			printf("Failed to get information for partition %d\n", part_num);
			continue;
		}

		if (get_fat_volume_label(desc, &part_info, volume_label) != 0) {
			continue;
		}

		if (strcasecmp(volume_label, "config") == 0 ) {
			if (fat_set_blk_dev(desc, &part_info) != 0) {
				printf("Failed to set block device for FAT on partition %d\n", part_num);
				return -ENODEV;
			}

			if (fat_exists(name) != 1) {
				printf("File %s does not exist on partition %d\n", name, part_num);
				return -ENOENT;
			}

			if (fat_size(name, &len) < 0) {
				printf("Failed to get file size for %s\n", name);
				return -ENOENT;
			}

			actread = file_fat_read(name, buf, len);
			if (actread != len) {
				printf("Failed to read file %s: read %lld, expected %lld\n", name, actread, len);
				return -EIO;
			}

			printf("File %s successfully read, size: %lld bytes\n", name, len);
			return len;
		}
	}

	printf("Partition with label 'CONFIG' not found.\n");

	return -ENOENT;
}
