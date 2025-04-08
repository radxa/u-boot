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
#ifndef _KSC_MEM_H
#define _KSC_MEM_H
#include <common.h>
#include <clk/clk.h>
#include <memalign.h>
#include <linux/list.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include "sys_config.h"
#include <fdt_support.h>
#include <linux/compat.h>
#include <linux/errno.h>
#include <fs.h>
#include <search.h>
#include <asm/atomic.h>
#include "ksc.h"
#include <mapmem.h>
#include <compiler.h>

int my_memalign_init(void);
uintptr_t ksc_mem_alloc(unsigned int memalign_val, unsigned int num_bytes);

#endif//End of File