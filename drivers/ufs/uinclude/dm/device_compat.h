    /* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2020 Sean Anderson <seanga2@gmail.com>
 * Copyright (c) 2013 Google, Inc
 *
 * (C) Copyright 2012
 * Pavel Herrmann <morpheus.ibis@gmail.com>
 * Marek Vasut <marex@denx.de>
 */

#ifndef _DM_DEVICE_COMPAT_H
#define _DM_DEVICE_COMPAT_H

//#if defined SYSTEM_SIMULATION || defined FPGA_SIMULATION
//#ifndef  __FPGA__I#if (BROM_MODE == BROM_MODE_ALL_SIMU)
//#if (BROM_MODE == BROM_MODE_ALL_SIMU)
/*fpga no use printk to sram,because fpga has uart print,no need printk to sram
 *if print to sram and printk to sram use together,no enough rom and compiler will failed
 * */
#if 1
extern void msg_sim(const char *fmt, ...);
void sunxi_dump(void *addr, unsigned int size);
//extern void printf(const /*u8*/char *fmt, ...);

#ifdef UFS_SIM_DEG
#define __dev_printk msg_sim
#else
#define __dev_printk printf
#endif

#define dev_emerg(dev, fmt, ...) \
	__dev_printk("[ufs]emerg:"fmt, ##__VA_ARGS__)
#define dev_alert(dev, fmt, ...) \
	__dev_printk("[ufs]alert:"fmt, ##__VA_ARGS__)
#define dev_crit(dev, fmt, ...) \
	__dev_printk("[ufs]crit:"fmt, ##__VA_ARGS__)
#define dev_err(dev, fmt, ...) \
	__dev_printk("[ufs]err:"fmt, ##__VA_ARGS__)
#define dev_warn(dev, fmt, ...) \
	__dev_printk("[ufs]warn:"fmt, ##__VA_ARGS__)
#define dev_notice(dev, fmt, ...) \
	__dev_printk("[ufs]:"fmt, ##__VA_ARGS__)
#define dev_info(dev, fmt, ...) \
	__dev_printk("[ufs]info:"fmt, ##__VA_ARGS__)

#if 0
#define dev_dbg(dev, fmt, ...) \
	__dev_printk("[ufs]dbg:"fmt, ##__VA_ARGS__)
#else
#define dev_dbg(dev, fmt, ...)
#endif

#define dev_vdbg(dev, fmt, ...) \
	__dev_printk(fmt, ##__VA_ARGS__)

#define dev_dump(dev, a, s)\
{\
    dev_err(dev, "dump:");\
    sunxi_dump(a, s);\
}
/*
#define dev_emerg(dev, fmt, ...) \
	__dev_printk(LOGL_EMERG, dev, fmt, ##__VA_ARGS__)
#define dev_alert(dev, fmt, ...) \
	__dev_printk(LOGL_ALERT, dev, fmt, ##__VA_ARGS__)
#define dev_crit(dev, fmt, ...) \
	__dev_printk(LOGL_CRIT, dev, fmt, ##__VA_ARGS__)
#define dev_err(dev, fmt, ...) \
	__dev_printk(LOGL_ERR, dev, fmt, ##__VA_ARGS__)
#define dev_warn(dev, fmt, ...) \
	__dev_printk(LOGL_WARNING, dev, fmt, ##__VA_ARGS__)
#define dev_notice(dev, fmt, ...) \
	__dev_printk(LOGL_NOTICE, dev, fmt, ##__VA_ARGS__)
#define dev_info(dev, fmt, ...) \
	__dev_printk(LOGL_INFO, dev, fmt, ##__VA_ARGS__)
#define dev_dbg(dev, fmt, ...) \
	__dev_printk(LOGL_DEBUG, dev, fmt, ##__VA_ARGS__)
#define dev_vdbg(dev, fmt, ...) \
	__dev_printk(LOGL_DEBUG_CONTENT, dev, fmt, ##__VA_ARGS__)
*/
#else
#define dev_emerg(dev, fmt, ...)
#define dev_alert(dev, fmt, ...)
#define dev_crit(dev, fmt, ...)
#define dev_err(dev, fmt, ...)
#define dev_warn(dev, fmt, ...)
#define dev_notice(dev, fmt, ...)
#define dev_info(dev, fmt, ...)
#define dev_dbg(dev, fmt, ...)
#define dev_vdbg(dev, fmt, ...)

#endif

#endif
