/*
 * Copyright (c) 2024 Radxa Computer (Shenzhen) Co., Ltd.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __RADXA_IMG_H_
#define __RADXA_IMG_H_

/*
 * radxa_read_bmp_file() - read file from resource.
 *
 * @buf: destination buf to store file data
 * @name: file name
 *
 * return the length of read data.
 */

int radxa_read_bmp_file(void *buf, const char *name);

#endif
