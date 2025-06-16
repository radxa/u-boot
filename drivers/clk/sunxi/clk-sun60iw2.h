/*
 * Copyright (C) 2013 Allwinnertech, huangshuosheng <huangshuosheng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable factor-based clock implementation
 */
#ifndef __MACH_SUNXI_CLK_SUN60IW2_H
#define __MACH_SUNXI_CLK_SUN60IW2_H

#include "clk_factor.h"


/* CCMU Register List */
#define PLL_PERIPH0         0x00A0
#define PLL_PERIPH1         0x00C0
#define PLL_VIDEO0          0x0120
#define PLL_VIDEO1          0x0140
#define PLL_VIDEO2          0x0160
#define PLL_DE              0x02E0

/* gmac */
#define GMAC_MBUS           0x05E4
#define GMAC_PTP            0x1400
#define GMAC0_PHY           0x1410
#define GMAC0_BUS_GATE_RST  0x141C
#define GMAC1_PHY           0x1420
#define GMAC1_BUS_GATE_RST  0x142C

/* Accelerator */
#define DE0_CFG             0x0A00
#define DE_GATE             0x0A04
#define DE_SYS_GATE         0x0A74

/* net */
#define EINK_CFG            0x0A60
#define EINK_GATE           0x0A64
#define EINK_PANEL_CFG      0x0A6C

/* Storage Medium */
#define SMHC0_CFG           0x0D00
#define SMHC0_GATE          0x0D0C
#define SMHC1_CFG           0x0D10
#define SMHC1_GATE          0x0D1C
#define SMHC2_CFG           0x0D20
#define SMHC2_GATE          0x0D2C
#define SMHC3_CFG           0x0D30
#define SMHC3_GATE          0x0D3C

/* serdes */
#define SERDES_PHY_CFG      0x13C0
#define SERDES_GATE         0x13C4

/* Display Interface */
#define TCON_LCD_CFG0       0x1500
#define TCON_LCD_GATE0      0x1504
#define TCON_LCD_CFG1       0x1508
#define TCON_LCD_GATE1      0x150C
#define TCON_LCD_CFG2       0x1510
#define TCON_LCD_GATE2      0x1514
#define LVDS_GATE0          0x1544
#define LVDS_GATE1          0x154C
#define DSI_CFG0   	    0x1580
#define DSI_CFG1   	    0x1588
#define DSI_GATE0           0x1584
#define DSI_GATE1           0x158C
#define COMBPHY0_CFG        0x15C0
#define COMBPHY1_CFG        0x15C4
#define TCONTV_GATE0	    0x1604
#define TCONTV_GATE1	    0x160C
#define EDP_CFG             0x1640
#define EDP_GATE            0x164C
#define HDMI_TV_CFG         0x1684
#define HDMI_GATE           0x168C
#define HDMI_SFR_CFG        0x1690
#define DPSS_TOP0_GATE      0x16C4
#define DPSS_TOP1_GATE      0x16CC
#define VIDEO_OUT0_GATE     0x16E4
#define VIDEO_OUT1_GATE     0x16EC
#define DSC_GATE	    0x1744

#define SUNXI_CLK_MAX_REG   0x1FF0
#define CPUS_CLK_MAX_REG    0x03F0

/* RTC-CCMU Register List */
#define DCXO_SERDES_GATE    0x016C

#define F_N8X8_M0X2_P16x2(nv, mv, pv)      (FACTOR_ALL(nv, 8, 8, 0, 0, 0, mv, 0, 2, pv, 16, 2, 0, 0, 0, 0, 0, 0))
#define F_N8X8_D1V1X1_D2V0X1(nv, d1v, d2v) (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, d1v, 1, 1, d2v, 16, 3))
#define F_N8X8_D1V1X1_D2V20X3(nv, d1v, d2v) (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, d1v, 1, 1, d2v, 20, 3))
#define F_N8X8_D1V1X1_D2V0X0(nv, d1v, d2v)	    (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, d1v, 1, 1, d2v, 0, 1))
#define F_N8X8_D1V4X2_D2V0X2(nv, d1v, d2v) (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, d1v, 4, 2, d2v, 0, 2))
#define F_N8X8_P16X6_D1V1X1_D2V0X1(nv, pv, d1v, d2v) (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, pv, 16, 6, d1v, 1, 1, d2v, 0, 1))

#define PLL_PERIPH0_2X(n, d1, d2, freq)  {F_N8X8_D1V1X1_D2V0X1(n, d1, d2), freq}
#define PLL_PERIPH1_2X(n, d1, d2, freq)  {F_N8X8_D1V1X1_D2V0X1(n, d1, d2), freq}
#define PLL_PERIPH1_800M(n, d1, d2, freq)  {F_N8X8_D1V1X1_D2V20X3(n, d1, d2), freq}
#define PLLVIDEO0(n, d1, d2, freq)       {F_N8X8_D1V1X1_D2V0X0(n, d1, d2), freq}
#define PLLVIDEO3(n, d1, d2, freq)       {F_N8X8_D1V1X1_D2V0X0(n, d1, d2), freq}

#endif
