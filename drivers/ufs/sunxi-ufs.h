// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2007-2031
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Aaron <leafy.myeh@allwinnertech.com>
 *
 * UFS driver for allwinner sunxi platform.
 */
#ifndef __SUNXI_UFS_H
#define __SUNXI_UFS_H

#define SUNXI_UFS_AXI_CLK_REG 		(SUNXI_CCMU_BASE + 0x0d80)
#define SUNXI_UFS_AXI_CLK_GATING_BIT 	(0x1U<<31)
#define SUNXI_UFS_CLK_SRC_SEL_MASK 	(0x7U<<24)
#define SUNXI_UFS_AXI_FACTOR_M_MASK 	(0x1F)
#define SUNXI_UFS_SRC_PERI0_300M 	(0 << 24)
#define SUNXI_UFS_SRC_PERI0_200M 	(1 << 24)


#define SUNXI_UFS_BGR_REG 		(SUNXI_CCMU_BASE + 0x0d8C)
#define SUNXI_UFS_CORE_RST_BIT 		(0x1U<<19)
#define SUNXI_UFS_PHY_RST_BIT 		(0x1U<<18)
#define SUNXI_UFS_AXI_RST_BIT 		(0x1U<<17)
#define SUNXI_UFS_RST_BIT 		(0x1U<<16)
#define SUNXI_UFS_GATING_BIT 		(0x1U<<0)

#define SUNXI_UFS_CFG_CLK_REG 		(SUNXI_CCMU_BASE + 0x0d84)
#define SUNXI_UFS_CFG_CLK_GATING_BIT 	(1U<<31)
#define SUNXI_UFS_CFG_CLK_SRC_SEL_MASK 	(0x7<<24)
#define SUNXI_UFS_CFG_FACTOR_M_MASK 	(0x1f)
#define SUNXI_UFS_CFG_SRC_PERI0_480M 	(0 << 24)


#define SUNXI_RTC 			(SUNXI_RTC_BASE)
#define SUNXI_XO_CTRL1_REG 		(SUNXI_RTC + 0x16c)
#define SUNXI_DCXO_UFS_GATING 		(0x1U)
#define SUNXI_XO_CTRL_REG 		(SUNXI_RTC_BASE + 0x160)
/*Be careful 1 is disable,0 is enalbe*/
#define SUNXI_CLK_REQ_EN 		(0x1U<<31)
#define SUNXI_XO_CTRL_WP_REG	 	(SUNXI_RTC_BASE + 0x015c)

#define SUNXI_UFS_EXT_REF_26M 		0x0
#define SUNXI_UFS_INT_REF_19_2M		0x1
#define SUNXI_UFS_INT_REF_26M		0x2
#define SUNXI_UFS_EXT_REF_19_2M 	0x3

#define SUNXI_SYSCFG 			(SUNXI_SYSCTRL_BASE)
#define SUNXI_UFS_LOCK_REG 		 (SUNXI_SYSCFG + 0x22C)
#define SUNXI_UFS_LOCK_BIT 		(0x1U)
#define SUNXI_U3U2_PHY_MUX_CTRL_REG 	(SUNXI_SYSCFG + 0x0228)
#define SUNXI_UFS_REF_CLK_EN_APP_BIT 	(0x1U<<8)

/*new reg in aw1903*/
#define SUNXI_UFS_REF_CLK_EN_REG 	(SUNXI_CCMU_BASE  + 0x0d90)
/*UFS_REF_CLK_EN*/
#define SUNXI_UFS_CCU_REF_CLK_EN_APP_BIT 	(0x1U)

#define SUNXI_UFS_REGS_BASE 			(0x4520000)

#define EFUSE_CFG_UFS_USE_CCU_EN_APP        (0x10000)    //0b0001 0000 0000 0000 0000
#endif
