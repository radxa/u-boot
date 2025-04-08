/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __HWSPINLOCK_PLATFORM_H__
#define __HWSPINLOCK_PLATFORM_H__

#if defined(CONFIG_MACH_SUN50IW11)
#include "platform/hwspinlock-sun50iw11.h"
#endif

#if defined(CONFIG_MACH_SUN8IW20)
#include "platform/hwspinlock-sun8iw20.h"
#endif

#if defined(CONFIG_MACH_SUN20IW2)
#include "platform/hwspinlock-sun20iw2.h"
#endif

#if defined(CONFIG_MACH_SUN55IW3)
#include "platform/hwspinlock-sun55iw3.h"
#endif

#if defined(CONFIG_MACH_SUN60IW2)
#include "platform/hwspinlock-sun60iw2.h"
#endif
#endif /* __HWSPINLOCK_PLATFORM_H__ */
