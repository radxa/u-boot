// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2007-2024 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
/*
 * Pointer to initial global data area
 *
 * Here we initialize it if needed.
 */
#ifdef XTRN_DECLARE_GLOBAL_DATA_PTR
#undef	XTRN_DECLARE_GLOBAL_DATA_PTR
#define XTRN_DECLARE_GLOBAL_DATA_PTR	/* empty = allocate here */
DECLARE_GLOBAL_DATA_PTR = (gd_t *)(CONFIG_SYS_INIT_GD_ADDR);
#else
DECLARE_GLOBAL_DATA_PTR;
#endif

struct AlignMemoryPool {
	bool inited;
	unsigned long start;
	unsigned long end;
	unsigned long cur_ptr;
};

static struct AlignMemoryPool memalign_pool;

int pram_memalign_init(void)
{
	ulong reg;

	if (memalign_pool.inited == false) {
		reg = env_get_ulong("pram", 10, CONFIG_PRAM);
		memalign_pool.start = gd->ram_top - (reg << 10);
		memalign_pool.end = gd->ram_top;
		memalign_pool.cur_ptr = memalign_pool.start;
		memalign_pool.inited = true;
		pr_err("gd->relocaddr = %lx %lu\n", memalign_pool.start, reg);
	} else {
		pr_info("pram mem has already init!\n");
	}
	return 1;
}

unsigned long pram_memalign(unsigned int memalign_val, unsigned int bytes)
{
	unsigned long ret_ptr;

	if (bytes == 0 || memalign_val == 0 || memalign_val & (memalign_val - 1)) {
		printf("pram_memalign param error!!!\n");
		return 0;
	}
	memalign_pool.cur_ptr = ALIGN(memalign_pool.cur_ptr, memalign_val);
	pr_info("current ptr = %lx\n", memalign_pool.cur_ptr);

	if (memalign_pool.cur_ptr + bytes > memalign_pool.end) {
		pr_err("space exhausted\n");
		return 0;
	}
	ret_ptr = memalign_pool.cur_ptr;
	memalign_pool.cur_ptr += bytes;
	return ret_ptr;
}

