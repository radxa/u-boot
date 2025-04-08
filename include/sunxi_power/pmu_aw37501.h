/*
 * Copyright (C) 2016 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI AW37501  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __AW37501_H__
#define __AW37501_H__

#define AW37501_CHIP_ID             (0x80)

#define AW37501_DEVICE_ADDR			(0x3A3)
#ifdef CONFIG_AW37501_SUNXI_I2C_SLAVE
#define AW37501_RUNTIME_ADDR		CONFIG_AW37501_SUNXI_I2C_SLAVE
#else
#define AW37501_RUNTIME_ADDR        (0x3e)
#endif

/* List of registers for AW37501 */
#define AW37501_VOUTP_VOL		0x00
#define AW37501_VOUTN_VOL		0x01
#define AW37501_CTRL		    0x03
#define AW37501_ID				0x04
#define AW37501_WPRTEN			0x21

#endif /* __AW37501_REGS_H__ */


