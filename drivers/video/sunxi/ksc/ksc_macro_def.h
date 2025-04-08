// SPDX-License-Identifier: GPL-2.0
/*
 * ksc_macro_def.h
 *
 * Copyright (c) 2007-2024 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
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
#ifndef _KSC_MACRO_DEF_H
#define _KSC_MACRO_DEF_H

#ifdef __cplusplus
extern "C" {
#endif
#include "../../../../include/sunxi_ksc.h"


#define KSC_MEMALIGN 4096
#define MAX_CLK_NUM 2
#define KSC_DBG(fmt, ...)			pr_debug("[KSC_DBG]: "fmt, ##__VA_ARGS__)
#define KSC_ERR(fmt, ...)			pr_err("[KSC_ERR]: "fmt, ##__VA_ARGS__)
#define KSC_WRN(fmt, ...)			pr_warn("[KSC_WRN]: "fmt, ##__VA_ARGS__)
#define KSC_INFO(fmt, ...)			pr_info("[KSC_INFO]: "fmt, ##__VA_ARGS__)
#define KSC_DEV_ERR(dev, fmt, ...)	dev_err(dev, "[KSC_DEV_ERR]: "fmt, ##__VA_ARGS__)


#define read_poll_timeout(op, val, cond, sleep_us, timeout_us, args...)	\
({ \
	unsigned long timeout = timer_get_us() + timeout_us; \
	for (;;) { \
		(val) = op(args); \
		if (cond) \
			break; \
		if (timeout_us && time_after(timer_get_us(), timeout)) { \
			(val) = op(args); \
			break; \
		} \
		if (sleep_us) \
			udelay(sleep_us); \
	} \
	(cond) ? 0 : -ETIMEDOUT; \
})

#define HW_ACC   1

//KSC_LUT_SAMPLE_RATIO = 2^KSC_LUT_SAMPLE_RATIO_BITS
#define KSC_LUT_SAMPLE_RATIO      (8)
#define KSC_LUT_SAMPLE_RATIO_BITS (3)
#define KSC_LUT_COORDS_FIX_BITS   (4)
#define KSC_LUT_HIGHBITS_SHIFT    (16)

//1<<KSC_LUT_COORDS_FIX_BITS
#define KSC_LUT_COORDS_FIX_VAL    (16)

#define KSC_IN_W_ALIGN_RAT       (2)
#define KSC_IN_H_ALIGN_RAT       (2)
#define KSC_OU_W_ALIGN_RAT       (2)
#define KSC_OU_H_ALIGN_RAT       (2)

#define KSC_PAD_NUM    (32)

#define KSC_PAD_NUM_SHIFT   (KSC_PAD_NUM << KSC_LUT_COORDS_FIX_BITS)

#define KSC_BURST_LENGTH_BYTE    (64)

#define KSC_LUT_MAX_INTERVAL_X   (128)
#define KSC_LUT_MAX_INTERVAL_Y   (128)

#define SVD_MAT_DIM   (9)
#define SVD_MAT_NUM   (81)

#define KSC_BLK_MAX_H   (16)
#define KSC_BLK_MAX_V   (16)

#define KSC_BLOCK_SIZE   (KSC_BLK_MAX_H*KSC_BLK_MAX_V)

#define KSC_MB_LEN_ONLINE    (8)
#define KSC_MB_LEN_OFFLINE   (16)

#define KSC_SCAN_ORDER_H_V (0)
#define KSC_SCAN_ORDER_V_H (1)

#define KSC_BLK_SCAN_ORDER  (KSC_SCAN_ORDER_V_H)

#define RES_MAX        (128)
#define PIX_DIFF_MAX   (80)

#define ANTIALIASING_MAX        (64)
#define ANTIALIASING_DEPTH_HALF (32)
#define ANTIALIASING_BITS_SHIFT (6)
#define ANTIALIASING_QUANT_REF  (4)

#define WEIGHT_LUT_NUM         (192)
#define WEIGHT_LUT_NUM_SUB1    (188)
#define WEIGHT_QUANT_BITS      (7)
#define WEIGHT_QUANT_VAL       (128)
#define WEIGHT_QUANT_VAL_HALF  (64)

#define KSC_16BITS_MAXVAL        (65535)


#define LUT_NUM_0      (0)
#define LUT_NUM_1      (1)
#define LUT_NUM_2      (2)


#define LUT_INVALID   (0)
#define LUT_BORDER    (1)
#define LUT_VALID     (2)


//for Online mode-FE scaler down
#define MAX_FRM_WTH     (2048)

#define SD_QUANT_DEPTH  (12)
#define SD_QUANT_NUM    (1<<SD_QUANT_DEPTH)
#define SD_QUANT_NUM_HALF (SD_QUANT_NUM>>1)

#define SCALE_RATIO_SHIFT  (SD_QUANT_DEPTH - KSC_LUT_COORDS_FIX_BITS)

//#define PHASE_OFFSET_MAX (256)
//#define FILTER_WIN_MAX (128)
#define FILTER_PHASE_DEPTH (4)
//#define FILTER_QUANT_DEPTH (8)


#define PHASE_OFFSET_MAX (128)
//#define FILTER_WIN_MAX (64)
#define FILTER_WIN_MAX (4)
#define FILTER_QUANT_DEPTH (7)

#define MAX_BUFFER_LINE (2)
#define LINE_QUAN_DEPTH  (FILTER_QUANT_DEPTH)
//#define PHASE_OFFSET (128)



#define CSC_QUANT_BITS  (10)
#define CSC_QUANT_VAL  (1<<CSC_QUANT_BITS)
//1<<CSC_QUANT_BITS

#define CSC_MULT_SHIFT0  (6)
#define CSC_MULT_SHIFT1  (CSC_QUANT_BITS-CSC_MULT_SHIFT0)

#define MAX_VAL_9BITS  (511)
#define MAX_VAL_17BITS (131071)


#define MIN_DNS_RATIO (0.1f)
#define BE_MIN_DNS_RATIO (0.29f)


#define HW_VERIFY_TEST   1

#define GET_HIGH_PART(val, bits) ((val) >> (bits))
#define GET_LOW_PART(val, bits) ((val) & ((1 << (bits)) - 1))

#define SET_LOW_BITS_ZERO(val, bits) (((val) >> (bits)) << (bits))


#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

// #ifndef MIN
// #define MIN(a, b) ((a) < (b) ? (a) : (b))
// #endif

#ifndef CLIP
#define CLIP(val, min, max)                                                    \
	((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))
#endif

#ifndef ABS_SUB
#define ABS_SUB(A, B) (((A) > (B)) ? ((A) - (B)) : ((B) - (A)))
#endif

// #ifndef ALIGN
// #define ALIGN(x, a) (((x) + (a)-1) & (~((a)-1)))
// #endif

#ifndef ALIGN_FLOOR
#define ALIGN_FLOOR(x, a) ((x) & (~((a)-1)))
#endif

#ifndef ABS
#define ABS(a) ((a) > 0 ? (a) : -(a))
#endif

#ifndef PI
#define PI  (3.1415926f)
#endif

#ifndef DEG2RAD
#define DEG2RAD(angle) ((angle)*PI / 180.0f)
#endif

#define LUT_MEM_SIZE (1024 * 1024)


__maybe_unused static int IsYUV420(enum ksc_pix_fmt fmt)
{
	return (fmt == YUV420P || fmt == YVU420P || fmt == YUV420SP || fmt == YVU420SP);
}
__maybe_unused static int IsYUV422(enum ksc_pix_fmt fmt)
{
	return (fmt == YUV422P || fmt == YVU422P || fmt == YUV422SP || fmt == YVU422SP);
}
__maybe_unused static int IsYUV444(enum ksc_pix_fmt fmt)
{
	return (fmt == YUV444P || fmt == YVU444P || fmt == YUV444SP || fmt == YVU444SP);
}
__maybe_unused static int IsRGB888(enum ksc_pix_fmt fmt)
{
	return (fmt == RGB888 || fmt == BGR888);
}
__maybe_unused static int IsARGB8888(enum ksc_pix_fmt fmt)
{
	return (fmt == ARGB8888 || fmt == ABGR8888 || fmt == RGBA8888 || fmt == BGRA8888);
}
__maybe_unused static int IsARGB4444(enum ksc_pix_fmt fmt)
{
	return (fmt == ARGB4444 || fmt == ABGR4444 || fmt == RGBA4444 || fmt == BGRA4444);
}
__maybe_unused static int IsARGB1555(enum ksc_pix_fmt fmt)
{
	return (fmt == ARGB1555 || fmt == ABGR1555 || fmt == RGBA1555 || fmt == BGRA1555);
}
__maybe_unused static int IsARGB(enum ksc_pix_fmt fmt)
{
	return (IsARGB8888(fmt) || IsARGB4444(fmt) || IsARGB1555(fmt));
}
__maybe_unused static int IsAYUV(enum ksc_pix_fmt fmt)
{
	return (fmt == AYUV);
}


#ifdef __cplusplus
}
#endif

#endif /*End of file*/
