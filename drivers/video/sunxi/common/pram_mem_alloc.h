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
#ifndef _PRAM_MEM_ALLOC_H
#define _PRAM_MEM_ALLOC_H

int pram_memalign_init(void);

unsigned long pram_memalign(unsigned int memalign_val, unsigned int bytes);

#endif /*End of file*/
