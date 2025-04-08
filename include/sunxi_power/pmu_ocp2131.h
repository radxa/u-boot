/*
 * Copyright (C) 2016 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI OCP2131  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __OCP2131_H__
#define __OCP2131_H__

#define OCP2131_CHIP_ID             (0x80)

#define OCP2131_DEVICE_ADDR			(0x3A3)
#ifdef CONFIG_OCP2131_SUNXI_I2C_SLAVE
#define OCP2131_RUNTIME_ADDR		CONFIG_OCP2131_SUNXI_I2C_SLAVE
#else
#define OCP2131_RUNTIME_ADDR        (0x3e)
#endif

/* List of registers for OCP2131 */
#define OCP2131_AVDD_VOL		0x00
#define OCP2131_AVEE_VOL		0x01
#define OCP2131_CTRL		    0x03
#define OCP2131_ID				0x04

#endif /* __OCP2131_REGS_H__ */


