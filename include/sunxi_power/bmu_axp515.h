/*
 * Copyright (C) 2016 Allwinner.
 * wangwei <wangwei@allwinnertech.com>
 *
 * SUNXI AXP1506  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __AXP515_H__
#define __AXP515_H__

#define AXP515_ADDR (0x56)

#define AXP515_DEVICE_ADDR (0x3a3)

#ifndef CONFIG_SYS_SUNXI_R_I2C0_SLAVE
#define AXP515_RUNTIME_ADDR                     (0x3a)
#else
#ifndef CONFIG_AXP515_SUNXI_I2C_SLAVE
#ifndef CONFIG_AXP858_SUNXI_I2C_SLAVE
#define AXP515_RUNTIME_ADDR                    CONFIG_SYS_SUNXI_R_I2C0_SLAVE
#else
#define AXP515_RUNTIME_ADDR                    CONFIG_AXP858_SUNXI_I2C_SLAVE
#endif
#else
#define AXP515_RUNTIME_ADDR                    CONFIG_AXP515_SUNXI_I2C_SLAVE
#endif
#endif
/* For BMU1760 */
#define BMU_CHG_STATUS (0x00)
#define BMU_BAT_STATUS (0x02)
#define AXP515_CHIP_ID (0x03)
#define AXP515_ONOFF_STATUS (0x06)
#define AXP515_ILIMIT (0x10)
#define AXP515_RBFET_SET (0x11)
#define BMU_BOOST_EN (0x12)
#define BMU_BOOST_CTL (0x13)
#define AXP515_BATFET_DLY (0x16)
#define PWR_ON_CTL (0x17)
#define AXP515_AUTO_SETS (0x23)
#define AXP515_INTEN1	(0x40)
#define AXP515_INTEN2	(0x41)
#define AXP515_INTEN3	(0x42)
#define AXP515_INTEN4	(0x43)
#define AXP515_INTEN5	(0x44)
#define AXP515_INTEN6	(0x45)
#define AXP515_INTSTS1	(0x48)
#define AXP515_INTSTS2	(0x49)
#define AXP515_INTSTS3	(0x4A)
#define AXP515_INTSTS4	(0x4B)
#define AXP515_INTSTS5	(0x4C)
#define AXP515_INTSTS6	(0x4D)
#define AXP515_DIE_TEMP_H (0x56)
#define AXP515_DIE_TEMP_L (0x57)
#define AXP515_VTS_RES_H (0x58)
#define AXP515_VTS_RES_L (0x59)
#define BMU_BAT_VOL_H (0x78)
#define BMU_BAT_VOL_L (0x79)
#define AXP515_ADC_CONTROL (0x80)
#define AXP515_TS_PIN_CONTROL (0x81)
#define AXP515_IPRECHG_CFG    (0x8A)
#define BMU_CHG_CUR_LIMIT (0x8b)
#define AXP515_TIMER2_SET (0x8E)
#define AXP515_DPM_LOOP_SET (0x8F)
#define BMU_BAT_PERCENTAGE (0xB9)
#define AXP515_BATCAP0 (0xE0)
#define AXP515_BATCAP1 (0xE1)
#define BMU_REG_LOCK (0xF2)
#define BMU_REG_EXTENSION_EN (0xF4)
#define BMU_ADDR_EXTENSION (0xFF)

#endif /* __AXP515_H__ */
