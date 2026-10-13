/*
 * (C) Copyright 2024 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __EVB_RV1103B_H
#define __EVB_RV1103B_H

#include <configs/rv1103b_common.h>

#define CONFIG_SYS_MMC_ENV_DEV 0

#ifndef ROCKCHIP_DEVICE_SETTINGS
#define ROCKCHIP_DEVICE_SETTINGS \
			"stdout=serial,vidconsole\0" \
			"stderr=serial,vidconsole\0"
#endif

#ifdef CONFIG_SPL_KERNEL_BOOT
/*
 * We made a deal: Not allow U-Boot to bring up thunder-boot kernel.
 *
 * Because the thunder-boot feature may require special memory layout
 * or other appointments, U-Boot can't handle all that. Let's go back
 * to SPL to bring up kernel.
 *
 * Note: bootcmd is only called in normal boot sequence, that means
 * we allow user to boot what they want in U-Boot shell mode.
 */
#undef CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND "reset"
#endif

#endif /* __EVB_RV1103B_H */
