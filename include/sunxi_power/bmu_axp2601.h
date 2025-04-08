/*
 * Copyright (C) 2016 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI AXP2601  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __AXP2601_H__
#define __AXP2601_H__

#define AXP2601_CHIP_VER		(0x18)

#define AXP2601_DEVICE_ADDR		(0x3A3)

#ifdef CONFIG_AXP2601_SUNXI_I2C_SLAVE
#define AXP2601_RUNTIME_ADDR		CONFIG_AXP2601_SUNXI_I2C_SLAVE
#else
#define AXP2601_RUNTIME_ADDR		(0x62)
#endif

#if IS_ENABLED(CONFIG_MACH_SUN300IW1)
#define AXP2601_FLAGE_REG (volatile void *)(SUNXI_RTC_BASE+0x204)
#else
#define AXP2601_FLAGE_REG SUNXI_RTC_BASE+0x104
#endif


#define AXP2601_CHIP_ID					0x00
#define AXP2601_GAUGE_BROM				0x01
#define AXP2601_RESET_CFG				0x02
#define AXP2601_GAUGE_CONFIG				0x03
#define AXP2601_VBAT_H					0x04
#define AXP2601_VBAT_L					0x05
#define AXP2601_TM					0x06
#define AXP2601_GAUGE_SOC				0x08
#define AXP2601_T2E					0x0A
#define AXP2601_T2F					0x0C
#define AXP2601_LOWSOC					0x0E
#define AXP2601_IRQ					0x20
#define AXP2601_IRQ_EN					0x21
#define AXP2601_FWVER					0xC0
#define AXP2601_TRIM_EFUSE				0xC8
#define AXP2601_GAUGE_FG_ADDR				0xCD
#define AXP2601_GAUGE_FG_DATA_H				0xCE
#define AXP2601_END					0xFF

#endif /* __AXP2601_REGS_H__ */


