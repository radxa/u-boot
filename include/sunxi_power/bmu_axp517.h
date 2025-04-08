/*
 * Copyright (C) 2016 Allwinner.
 * wangwei <wangwei@allwinnertech.com>
 *
 * SUNXI AXP1506  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __AXP517_H__
#define __AXP517_H__

#define AXP517_DEVICE_ADDR			(0x3A3)

#ifndef CONFIG_SYS_SUNXI_R_I2C0_SLAVE
#define AXP517_RUNTIME_ADDR			(0x34)
#else
#ifndef CONFIG_AXP517_SUNXI_I2C_SLAVE
#define AXP517_RUNTIME_ADDR			CONFIG_SYS_SUNXI_R_I2C0_SLAVE
#else
#define AXP517_RUNTIME_ADDR			CONFIG_AXP517_SUNXI_I2C_SLAVE
#endif
#endif

#define AXP517_STATUS0				(0x00)
#define AXP517_STATUS1				(0x01)
#define AXP517_CHIP_ID				(0x03)
#define AXP517_DATA_BUFF			(0x04)
#define AXP517_ILIM_TYPE			(0x06)
#define AXP517_CLK_EN				(0x0B)
#define AXP517_CHIP_ID_EXT			(0x0E)

#define AXP517_COMM_CFG            (0x10)
#define AXP517_BATFET_CTRL         (0x12)
#define AXP517_RBFET_CTRL          (0x13)
#define AXP517_DIE_TEMP_CFG        (0x14)
#define AXP517_VSYS_MIN            (0x15)
#define AXP517_VINDPM_CFG			(0x16)
#define AXP517_IIN_LIM				(0x17)
#define AXP517_RESET_CFG           (0x18)
#define AXP517_MODULE_EN           (0x19)
#define AXP517_WATCHDOG_CFG        (0x1a)
#define AXP517_GAUGE_THLD          (0x1b)
#define AXP517_PWRON_SET           (0x1c)
#define AXP517_VBUS_OV_SET         (0x1d)
#define AXP517_BST_CFG0            (0x1e)
#define AXP517_BST_CFG1            (0x1f)

#define AXP517_CHGLED_CFG          (0x30)

#define AXP517_IRQ_EN0				(0x40)
#define AXP517_IRQ_EN1				(0x41)
#define AXP517_IRQ_EN2				(0x42)
#define AXP517_IRQ_EN3				(0x43)
#define AXP517_IRQ_EN4				(0x44)
#define AXP517_IRQ0					(0x48)
#define AXP517_IRQ1					(0x49)
#define AXP517_IRQ2					(0x4a)
#define AXP517_IRQ3					(0x4b)
#define AXP517_IRQ4					(0x4c)

#define AXP517_TS_CFG              (0x50)
#define AXP517_TS_HYSL2H           (0x52)
#define AXP517_TS_HYSH2L           (0x53)
#define AXP517_VLTF_CHG            (0x54)
#define AXP517_VHTF_CHG            (0x55)
#define AXP517_VLTF_WORK           (0x56)
#define AXP517_VHTF_WORK           (0x57)
#define AXP517_JEITA_CFG           (0x58)
#define AXP517_JEITA_CV_CFG        (0x59)
#define AXP517_JEITA_COOL          (0x5a)
#define AXP517_JEITA_WARM          (0x5b)
#define AXP517_TS_CFG_DATA_H       (0x5c)
#define AXP517_TS_CFG_DATA_L       (0x5d)

#define AXP517_RECHG_CFG           (0x60)
#define AXP517_IPRECHG_CFG         (0x61)
#define AXP517_ICC_CFG             (0x62)
#define AXP517_ITERM_CFG           (0x63)
#define AXP517_VTERM_CFG           (0x64)
#define AXP517_TREGU_THLD          (0x65)
#define AXP517_CHG_FREQ            (0x66)
#define AXP517_CHG_TMR_CFG         (0x67)
#define AXP517_BAT_DET             (0x68)
#define AXP517_IR_COMP             (0x69)

#define AXP517_GAUGE_BROM          (0x70)
#define AXP517_GAUGE_CONFIG        (0x71)
#define AXP517_GAUGE_SOC			(0x74)
#define AXP517_GAUGE_TIME2EMPTY_H  (0x76)
#define AXP517_GAUGE_TIME2EMPTY_L  (0x77)
#define AXP517_GAUGE_TIME2FULL_H   (0x78)
#define AXP517_GAUGE_TIME2FULL_L   (0x79)
#define AXP517_CYCLE_H			(0x7B)
#define AXP517_CYCLE_L			(0x7C)

#define AXP517_ADC_CH_EN0          (0x90)
#define AXP517_VBAT_H              (0x91)
#define AXP517_VBAT_L              (0x92)
#define AXP517_IBAT_H              (0x93)
#define AXP517_IBAT_L              (0x94)
#define AXP517_TS_H              (0x95)
#define AXP517_TS_L              (0x96)
#define AXP517_IBUS_H              (0x97)
#define AXP517_IBUS_L              (0x98)
#define AXP517_VBUS_H              (0x99)
#define AXP517_VBUS_L              (0x9A)
#define AXP517_ADC_CONTROL			(0x9B)
#define AXP517_ADC_RES				(0x9C)

#define AXP517_TWI_ADDR_EXT      (0xFF)

#endif /* __AXP517_H__ */
