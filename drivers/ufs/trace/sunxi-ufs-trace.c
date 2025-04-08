// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2007-2031
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Aaron <leafy.myeh@allwinnertech.com>
 *
 * UFS driver for allwinner sunxi platform.
 */
#ifdef SUNXI_UFS_TRACE
u32 trace_info;
#endif

#if 0
#define SUNXI_UFS_SOFT_SHIFT	(0)
#define SUNXI_UFS_HARD_SHIFT	(8)
#define SUNXI_UFS_STAGE			(16)

u32 trace_info;

inline void sunxi_ufs_trace(u32 stage, u32 soft, u32 hard)
{
	u32 debug_reg = BROM_UFS_DBG_REG;
	trace_info = (stage << SUNXI_UFS_STAGE)|(soft <<  SUNXI_UFS_SOFT_SHIFT) | (hard << SUNXI_UFS_HARD_SHIFT);
	writel(trace_info, debug_reg);
}

inline void sunxi_ufs_trace_stage_start(u32 stage)
{
	trace_info = stage;
}

inline void sunxi_ufs_trace_soft(u32 soft)
{
	trace_info |= soft << SUNXI_UFS_SOFT_SHIFT;
}

inline void sunxi_ufs_trace_hard(u32 hard)
{
	trace_info |= hard << SUNXI_UFS_HARD_SHIFT;
}


inline void sunxi_ufs_trace_stage_end(void)
{
	u32 debug_reg = BROM_UFS_DBG_REG;
	writel(trace_info, debug_reg);
}

#endif


