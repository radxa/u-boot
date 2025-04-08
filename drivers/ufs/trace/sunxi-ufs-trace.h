// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2007-2012
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * Description: UFS driver for General UFS operations
 * Author: lixiang <lixiang@allwinnertech.com>
 * Date: 2022/10/21 16:22
 */
#ifndef __UFS_TRRACE
#define  __UFS_TRRACE
#include <device_compat.h>
extern u32 trace_info;
extern void msg_sim(const char *fmt, ...) ;
#define REG_BIT(__b)  (1U<<__b)

/*bit 0,1 use to record ref clk*/
/*UIC Error/UTP Error */
#define SUNXI_UFS_UIC_ERR 			REG_BIT(2)
#define SUNXI_UFS_DEV_FATAL_ERR 			REG_BIT(3)
#define SUNXI_UFS_CTR_FATAL_ERR 			REG_BIT(4)
#define SUNXI_UFS_BUS_FATAL_ERR 			REG_BIT(5)


#define SUNXI_UFS_HOST_EN_ERR			REG_BIT(6)
#define SUNXI_UFS_RAMINIT_ERR 			REG_BIT(7)
#define SUNXI_UFS_PHYINIT_ERR 			REG_BIT(8)
#define SUNXI_UFS_LINK_ERR 			REG_BIT(9)
#define SUNXI_UFS_LINK_NOT_UP			REG_BIT(10)


#define SUNXI_UFS_UNIPRO_INIT_ERR 		REG_BIT(11)
#define SUNXI_UFS_OCS_ERR			REG_BIT(12)
#define SUNXI_UFS_NOP_ERR 			REG_BIT(13)
#define SUNXI_UFS_DEV_BUSY_ERR 			REG_BIT(14)
/*scsi inquiry error*/
#define SUNXI_UFS_INQ_ERR 			REG_BIT(15)
#define SUNXI_UFS_TESTREADY_ERR 		REG_BIT(16)
#define SUNXI_UFS_SCI_ST_ERR			REG_BIT(17)
#define SUNXI_UFS_BT0_HEAD_ERR 			REG_BIT(18)
#define SUNXI_UFS_BT0_MAGIC_ERR 		REG_BIT(19)
#define SUNXI_UFS_BT0_READALL_ERR 		REG_BIT(20)
#define SUNXI_UFS_BT0_CHK_SUM_ERR 		REG_BIT(21)
#define SUNXI_UFS_BT0_LEN_ERR 			REG_BIT(22)
#define SUNXI_UFS_BT0_BAK 			REG_BIT(23)

/*General timeout error*/
#define SUNXI_UFS_TOUT 				REG_BIT(24)





//#define SUNXI_UFS_TRACE

#ifdef SUNXI_UFS_TRACE
inline void sunxi_ufs_trace_point(u32 point)
{
	trace_info |= point;
	writel(trace_info, BROM_UFS_DBG_REG);
}

inline void sunxi_ufs_trace_is_point(u32 err)
{
    if (err & DEVICE_FATAL_ERROR)
	sunxi_ufs_trace_point(SUNXI_UFS_DEV_FATAL_ERR);
    if (err & CONTROLLER_FATAL_ERROR)
	sunxi_ufs_trace_point(SUNXI_UFS_CTR_FATAL_ERR);
    if (err & SYSTEM_BUS_FATAL_ERROR)
	sunxi_ufs_trace_point(SUNXI_UFS_BUS_FATAL_ERR);
    if (err & UIC_ERROR)
	sunxi_ufs_trace_point(SUNXI_UFS_UIC_ERR);
}

inline void sunxi_ufs_trace_clear_point(u32 point)
{
	trace_info &= ~point;
}

inline void sunxi_ufs_trace_stage_end(void)
{
	u32 debug_reg = BROM_UFS_DBG_REG;
	writel(trace_info, debug_reg);
	dev_dbg(NULL, "trace info %x", readl(debug_reg));
}

#else
inline void sunxi_ufs_trace_point(u32 point)
{
}

inline void sunxi_ufs_trace_is_point(u32 err)
{
}

inline void sunxi_ufs_trace_clear_point(u32 point)
{
}

inline void sunxi_ufs_trace_stage_end(void)
{
}


#endif


#endif  /*__UFS_TRRACE*/
