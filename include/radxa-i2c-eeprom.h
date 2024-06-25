/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __RADXA_I2C_EEPROM_H
#define __RADXA_I2C_EEPROM_H

#include <linux/errno.h>

#if CONFIG_IS_ENABLED(RADXA_ID_EEPROM)
/* Return zero if a valid primary MAC was found, or a negative error. */
int radxa_mac_read_from_eeprom(void);
#else
static inline int radxa_mac_read_from_eeprom(void)
{
	return -ENOSYS;
}
#endif

#endif
