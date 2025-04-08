/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2013 Google, Inc
 */

#ifndef __DM_UTIL_H
#define __DM_UTIL_H


#ifdef DM_WARN
#define dm_warn(fmt...) log(LOGC_DM, LOGL_WARNING, ##fmt)
#else
static inline void dm_warn(const char *fmt, ...)
{
}
#endif



//#if CONFIG_IS_ENABLED(OF_PLATDATA_INST) && CONFIG_IS_ENABLED(READ_ONLY)
#if 0
void *dm_priv_to_rw(void *priv);
#else
static inline void *dm_priv_to_rw(void *priv)
{
	return priv;
}
#endif

#endif
