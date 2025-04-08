/*
 * sunxi_logo_display/sunxi_logo_display.h
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _DRM_LOGO_H
#define _DRM_LOGO_H

#ifdef __cplusplus
extern "C" {
#endif

extern int sunxi_bmp_display(char *name);
int sunxi_show_bmp(char *name);
int sunxi_show_bmp_and_backlight(char *name, char *reg);
int sunxi_backlight_ctrl(char *reg);
int sunxi_drm_disable(void);
#ifdef __cplusplus
}
#endif

#endif /*End of file*/
