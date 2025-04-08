// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2007-2031
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Aaron <leafy.myeh@allwinnertech.com>
 *
 * UFS driver for allwinner sunxi platform.
 */
#ifndef _LINUX_COMPAT_H_
#define _LINUX_COMPAT_H_

#define GFP_ATOMIC ((gfp_t) 0)
#define GFP_KERNEL ((gfp_t) 0)
#define GFP_NOFS ((gfp_t) 0)
#define GFP_USER ((gfp_t) 0)
#define __GFP_NOWARN ((gfp_t)0)
#define __GFP_ZERO	((__force gfp_t)0x8000u)	/* Return zeroed page on success */

void *kmalloc(size_t size, int flags);
static inline void kfree(const void *block)
{
	free((void *)block);
}

#endif
