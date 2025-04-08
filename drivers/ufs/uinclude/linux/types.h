// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2007-2031
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Aaron <leafy.myeh@allwinnertech.com>
 *
 * UFS driver for allwinner sunxi platform.
 */
#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#include <common.h>
#include <asm/types.h>

#ifdef __CHECK_ENDIAN__
#define __bitwise __bitwise__
#else
#define __bitwise
#endif

typedef         __u8            uint8_t;
typedef __u16 __bitwise __le16;
typedef __u16 __bitwise __be16;
typedef __u32 __bitwise __le32;
typedef __u32 __bitwise __be32;
//typedef u64 __bitwise __le64;
//typedef u64 __bitwise __be64;
typedef __u16 __bitwise __sum16;
typedef __u32 __bitwise __wsum;
typedef unsigned __bitwise gfp_t;

typedef         __u64           uint64_t;
typedef unsigned long           ulong;
typedef __u64 u64;
typedef __s64 s64;

/*
typedef enum
{
    true=1, false=0
}bool;
*/

#ifdef CONFIG_ARM64
#define BITS_PER_LONG 64
#else   /* CONFIG_ARM64 */
#define BITS_PER_LONG 32
#endif  /* CONFIG_ARM64 */

#ifdef CONFIG_PHYS_64BIT
typedef unsigned long long phys_addr_t;
typedef unsigned long long phys_size_t;
#else
/* DMA addresses are 32-bits wide */
typedef unsigned long phys_addr_t;
typedef unsigned long phys_size_t;
#endif

/*
 *  * A dma_addr_t can hold any valid DMA address, i.e., any address returned
 *   * by the DMA API.
 *    *
 *     * If the DMA API only uses 32-bit addresses, dma_addr_t need only be 32
 *      * bits wide.  Bus addresses, e.g., PCI BARs, may be wider than 32 bits,
 *       * but drivers do memory-mapped I/O to ioremapped kernel virtual addresses,
 *        * so they don't care about the size of the actual bus addresses.
 *         */
#ifdef CONFIG_DMA_ADDR_T_64BIT
typedef unsigned long long dma_addr_t;
#else
typedef u32 dma_addr_t;
#endif




#endif /* _LINUX_TYPES_H */
