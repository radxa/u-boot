/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2018 MediaTek Inc.
 */

#ifndef __MEDIATEK_MISC_H_
#define __MEDIATEK_MISC_H_

#include <linux/types.h>

#define VER_BASE		0x08000000
#define VER_SIZE		0x10

#define APHW_CODE		0x00
#define APHW_SUBCODE		0x04
#define APHW_VER		0x08
#define APSW_VER		0x0c

#define BOOT_ARGUMENT_LOCATION	(0x40000100)
#define BOOT_ARGUMENT_MAGIC	0x504c504c
#define BOOT_ARGUMENT		((struct boot_argument *)BOOT_ARGUMENT_LOCATION)

#define MBLOCK_RESERVED_NAME_SIZE 32
#define MBLOCK_RESERVED_NUM_MAX  32
#define MBLOCK_MAGIC 0x4D424C4F

struct reserved_t {
	u64 start;
	u64 size;
	u32 mapping;
	char name[MBLOCK_RESERVED_NAME_SIZE];
};

struct mblock_info_t {
	u32 magic_number;
	u32 reserved_num;
	struct reserved_t reserved[MBLOCK_RESERVED_NUM_MAX];
};

struct boot_argument {
	unsigned int magic_number;
	unsigned long long dram_size;
	struct mblock_info_t mblock_info;
};

void mediatek_capsule_update_board_setup(void);
void mtk_reserved_memory_init(void *blob, struct boot_argument *boot_arg);

#endif /* __MEDIATEK_MISC_H_ */
