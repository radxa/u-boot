/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Logging support
 *
 * Copyright (c) 2017 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 */

#ifndef __LOG_H
#define __LOG_H

//#if (BROM_MODE == BROM_MODE_ALL_SIMU)
#if 0
#define debug(fmt, ...) msg_sim(fmt, ##__VA_ARGS__)
#else
#define debug(fmt, args...)
#endif

#endif
