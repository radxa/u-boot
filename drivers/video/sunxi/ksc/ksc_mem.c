// SPDX-License-Identifier: GPL-2.0
/*
 * ksc/ksc.c
 *
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

#include "ksc_mem.h"
#include <pram_mem_alloc.h>

uintptr_t ksc_mem_alloc(unsigned int memalign_val, unsigned int num_bytes)
{
#if IS_ENABLED(CONFIG_SUNXI_PRAM_MEM)
	return (uintptr_t)pram_memalign(memalign_val, num_bytes);
#endif
	return (uintptr_t)memalign(memalign_val, num_bytes);

}

//End of File
