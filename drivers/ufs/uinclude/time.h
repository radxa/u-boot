/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef _TIME_H
#define _TIME_H

#include <common.h>

/* Returns time in milliseconds */
inline unsigned long get_timer(unsigned long base)
{
    return get_sys_ticks() - base;
}


#endif /* _TIME_H */
