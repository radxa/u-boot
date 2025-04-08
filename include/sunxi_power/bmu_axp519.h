/*
 * Copyright (C) 2016 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI AXP519  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __AXP519_H__
#define __AXP519_H__

#define AXP519_CHIP_VER			(0x3)
#define AXP2601_CHIP_VER		(0x18)
#define AXP2602_CHIP_VER		(0x1c)

#define AXP519_DEVICE_ADDR		(0x3A3)
#define AXP2601_DEVICE_ADDR		(0x3A3)

#ifdef CONFIG_AXP519_SUNXI_I2C_SLAVE
#define AXP519_RUNTIME_ADDR		CONFIG_AXP519_SUNXI_I2C_SLAVE
#else
#define AXP519_RUNTIME_ADDR		(0x3c)
#endif

#ifdef CONFIG_AXP2601_SUNXI_I2C_SLAVE
#define AXP2601_RUNTIME_ADDR		CONFIG_AXP2601_SUNXI_I2C_SLAVE
#else
#define AXP2601_RUNTIME_ADDR		(0x62)
#endif

#define AXP2601_FLAGE_REG SUNXI_RTC_BASE+0x104

/* List of registers for axp519 */
#define AXP519_CHIP_ID					0x01
#define AXP519_IRQ_EN0					0x02
#define AXP519_IRQ_EN1					0x03
#define AXP519_IRQ0					0x04
#define AXP519_IRQ1					0x05
#define AXP519_SYS_STATUS				0x06
#define AXP519_WORK_CFG					0x0d
#define AXP519_ADC_CFG					0x10
#define AXP519_ADC_H					0x11
#define AXP519_ADC_L					0x12
#define AXP519_CTRL						0x19
#define AXP519_DISCHG_SET1				0x20
#define AXP519_VBUSOUT_VOL_SET_H			0x23
#define AXP519_VBUSOUT_LIM_SET				0x25
#define AXP519_CHG_SET1					0x30
#define AXP519_CHG_SET3					0x32
#define AXP519_VTERM_CFG_H				0x34
#define AXP519_OVP_SET					0x38
#define AXP519_LINLIM_SET				0x39
#define AXP519_VBATLIM_SET				0x3a
#define AXP519_VSYS_SET					0x42
#define AXP519_NTC_SET1					0x43
#define AXP519_NTC_SET2					0x44
#define AXP519_END					0xff

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
int bmu_axp519_probe(void);
int bmu_axp519_get_vbus_status(void);
bool get_axp519(void);
#endif /* __AXP519_REGS_H__ */


