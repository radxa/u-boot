/*
 * (C) Copyright 2016 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <android_bootloader_message.h>
#include <config.h>
#include <common.h>
#include <command.h>
#include <exports.h>
#include <image.h>
#include <malloc.h>
#include <part.h>
#include <asm/cache.h>
#include <asm/io.h>
#include <asm/arch-rockchip/boot_mode.h>
#include <dm/device.h>
#include <dm/uclass.h>
#include <linux/printk.h>
#include <asm/arch-rockchip/common.h>

DECLARE_GLOBAL_DATA_PTR;

enum {
	PH = 0,	/* P: Priority, H: high, M: middle, L: low*/
	PM,
	PL,
};

static int bcb_read_message(u32 bcb_offset, struct android_bootloader_message *bmsg)
{
	struct android_bootloader_message *buffer;
	struct blk_desc *dev_desc;
	struct disk_partition part;
	ulong blkcnt;
	int ret = 0;

	dev_desc = plat_bootdev();
	if (!dev_desc) {
		printf("dev_desc is NULL!\n");
		return -ENODEV;
	}

	if (part_get_info_by_name(dev_desc, PART_MISC, &part) < 0) {
		printf("No misc partition\n");
		return -EINVAL;
	}

	blkcnt = DIV_ROUND_UP(sizeof(*bmsg), dev_desc->blksz);
	buffer = memalign(ARCH_DMA_MINALIGN, blkcnt * dev_desc->blksz);
	if (!buffer)
		return -ENOMEM;

	if (blk_dread(dev_desc, part.start + bcb_offset, blkcnt, buffer) != blkcnt)
		ret = -EIO;
	else
		memcpy(bmsg, buffer, sizeof(*bmsg));
	free(buffer);

	return ret;
}

int bcb_read_mode(int bcb_offset)
{
	struct android_bootloader_message bmsg;
	int bcb_mode = BCB_MODE_NONE;

	if (bcb_offset < 0) { /* auto get */
#ifdef CONFIG_ANDROID_BOOT_IMAGE
		bcb_offset = android_bcb_msg_sector_offset();
#else
		bcb_offset = BCB_MESSAGE_BLK_OFFSET;
#endif
	}

	if (bcb_read_message(bcb_offset, &bmsg))
		return bcb_mode;

	/*
	 * Don't change the priority/order.
	 */
	if (!strcmp(bmsg.command, "bootonce-bootloader"))
		bcb_mode = BCB_MODE_BOOTLOADER;
	else if (!strcmp(bmsg.recovery, "recovery\n--rk_fwupdate\n"))
		bcb_mode = BCB_MODE_RECOVERY_RK_FWUPDATE;
	else if (!strcmp(bmsg.recovery, "recovery\n--factory_mode=whole") ||
		 !strcmp(bmsg.recovery, "recovery\n--factory_mode=small"))
		bcb_mode = BCB_MODE_RECOVERY_PCBA;
	else if (!strcmp(bmsg.command, "boot-recovery") ||
		 !strcmp(bmsg.command, "boot-fastboot"))
		bcb_mode = BCB_MODE_RECOVERY;

	return bcb_mode;
}

static int bcb_clear_mode(u32 bcb_offset)
{
	struct android_bootloader_message *buffer;
	struct android_bootloader_message bmsg;
	struct blk_desc *dev_desc;
	struct disk_partition part;
	ulong blkcnt;
	int ret;

	dev_desc = plat_bootdev();
	if (!dev_desc)
		return -ENODEV;

	if (part_get_info_by_name(dev_desc, PART_MISC, &part) < 0)
		return -EINVAL;

	blkcnt = DIV_ROUND_UP(sizeof(bmsg), dev_desc->blksz);
	if (!blkcnt)
		return -EINVAL;

	ret = bcb_read_message(bcb_offset, &bmsg);
	if (ret)
		return ret;

	memset(bmsg.command, 0, sizeof(bmsg.command));
	buffer = memalign(ARCH_DMA_MINALIGN, blkcnt * dev_desc->blksz);
	if (!buffer)
		return -ENOMEM;

	memset(buffer, 0, blkcnt * dev_desc->blksz);
	memcpy(buffer, &bmsg, sizeof(bmsg));
	ret = blk_dwrite(dev_desc, part.start + bcb_offset,
			 blkcnt, buffer) == blkcnt ? 0 : -EIO;
	free(buffer);

	return ret;
}

/*
 * There are three ways to get boot-mode:
 *
 * No1. Android BCB which is defined in misc.img (0KB or 16KB offset)
 * No2. CONFIG_ROCKCHIP_BOOT_MODE_REG that supports "reboot xxx" commands
 * No3. Env variable "reboot_mode" which is added by U-Boot
 *
 * Recovery mode from:
 *	- Android BCB in misc.img
 *	- "reboot recovery" command
 *	- recovery key pressed without usb attach
 */
int plat_boot_mode(void)
{
	static int boot_mode[] =		/* static */
		{ -EINVAL, -EINVAL, -EINVAL };
	static int bcb_offset = -EINVAL;	/* static */
	uint32_t reg_boot_mode;
	char *env_reboot_mode;
	int ret, clear_boot_reg = 0;
	int bcb_mode = 0;
#ifdef CONFIG_ANDROID_BOOT_IMAGE
	u32 offset = android_bcb_msg_sector_offset();
#else
	u32 offset = BCB_MESSAGE_BLK_OFFSET;
#endif

	/*
	 * Env variable "reboot_mode" which is added by U-Boot, reading ever time.
	 */
	env_reboot_mode = env_get("reboot_mode");
	if (env_reboot_mode) {
		if (!strcmp(env_reboot_mode, "recovery-key")) {
			printf("boot mode: recovery (key)\n");
			return BOOT_MODE_RECOVERY;
		} else if (!strcmp(env_reboot_mode, "recovery-usb")) {
			printf("boot mode: recovery (usb)\n");
			return BOOT_MODE_RECOVERY;
		} else if (!strcmp(env_reboot_mode, "recovery")) {
			printf("boot mode: recovery (env)\n");
			return BOOT_MODE_RECOVERY;
		} else if (!strcmp(env_reboot_mode, "fastboot")) {
			printf("boot mode: fastboot\n");
			return BOOT_MODE_BOOTLOADER;
		} else if (!strcmp(env_reboot_mode, "normal")) {
			printf("boot mode: normal(env)\n");
			return BOOT_MODE_NORMAL;
		}
	}

	/*
	 * Android BCB special handle:
	 *    Once the Android BCB offset changed, reinitalize "boot_mode[PM]".
	 *
	 * Background:
	 *    1. there are two Android BCB at the 0KB(google) and 16KB(rk)
	 *       offset in misc.img
	 *    2. Android image: return 0KB offset if image version >= 10,
	 *	 otherwise 16KB
	 *    3. Not Android image: return 16KB offset, eg: FIT image.
	 *
	 * To handle the cases of 16KB and 0KB, we reinitial boot_mode[PM] once
	 * Android BCB is changed.
	 *
	 * PH and PL is from boot mode register and reading once.
	 * PM is from misc.img and should be updated if BCB offset is changed.
	 * Return the boot mode according to priority: PH > PM > PL.
	 */
	if (bcb_offset != offset) {
		boot_mode[PM] = -EINVAL;
		bcb_offset = offset;
	}

	/* directly return if there is already valid mode */
	if (boot_mode[PH] != -EINVAL)
		return boot_mode[PH];
	else if (boot_mode[PM] != -EINVAL)
		return boot_mode[PM];
	else if (boot_mode[PL] != -EINVAL)
		return boot_mode[PL];

	/*
	 * Boot mode priority
	 *
	 * Anyway, we should set download boot mode as the highest priority, so:
	 * reboot loader/bootloader/fastboot > misc partition "recovery" > reboot xxx.
	 */
	reg_boot_mode = readl((void *)CONFIG_ROCKCHIP_BOOT_MODE_REG);
	if (reg_boot_mode == BOOT_LOADER) {
		printf("boot mode: loader\n");
		boot_mode[PH] = BOOT_MODE_LOADER;
		clear_boot_reg = 1;
	} else if (reg_boot_mode == BOOT_DFU) {
		printf("boot mode: dfu\n");
		boot_mode[PH] = BOOT_MODE_DFU;
		clear_boot_reg = 1;
	} else if (reg_boot_mode == BOOT_FASTBOOT) {
		printf("boot mode: bootloader\n");
		boot_mode[PH] = BOOT_MODE_BOOTLOADER;
		clear_boot_reg = 1;
		/* clear bcb */
		if (bcb_read_mode(bcb_offset) == BCB_MODE_BOOTLOADER) {
			ret = bcb_clear_mode(bcb_offset);
			if (ret)
				printf("failed to clear BCB: %d\n", ret);
		}
	} else if ((bcb_mode = bcb_read_mode(bcb_offset)) &&
		   (bcb_mode == BCB_MODE_BOOTLOADER)) {
		printf("boot mode: bootloader (misc)\n");
		boot_mode[PM] = BOOT_MODE_BOOTLOADER;
		/* bootonce-bootloader is consumed only once */
		ret = bcb_clear_mode(bcb_offset);
		if (ret)
			printf("failed to clear BCB: %d\n", ret);
	} else if (bcb_mode == BCB_MODE_RECOVERY ||
		   bcb_mode == BCB_MODE_RECOVERY_PCBA ||
		   bcb_mode == BCB_MODE_RECOVERY_RK_FWUPDATE) {
		printf("boot mode: recovery (misc)\n");
		boot_mode[PM] = BOOT_MODE_RECOVERY;
	} else {
		switch (reg_boot_mode) {
		case BOOT_NORMAL:
			printf("boot mode: normal\n");
			boot_mode[PL] = BOOT_MODE_NORMAL;
			clear_boot_reg = 1;
			break;
		case BOOT_RECOVERY:
			printf("boot mode: recovery (cmd)\n");
			boot_mode[PL] = BOOT_MODE_RECOVERY;
			clear_boot_reg = 1;
			break;
		case BOOT_UMS:
			printf("boot mode: ums\n");
			boot_mode[PL] = BOOT_MODE_UMS;
			clear_boot_reg = 1;
			break;
		case BOOT_CHARGING:
			printf("boot mode: charging\n");
			boot_mode[PL] = BOOT_MODE_CHARGING;
			clear_boot_reg = 1;
			break;
		case BOOT_PANIC:
			printf("boot mode: panic\n");
			boot_mode[PL] = BOOT_MODE_PANIC;
			break;
		case BOOT_WATCHDOG:
			printf("boot mode: watchdog\n");
			boot_mode[PL] = BOOT_MODE_WATCHDOG;
			break;
		case BOOT_QUIESCENT:
			printf("boot mode: quiescent\n");
			boot_mode[PL] = BOOT_MODE_QUIESCENT;
			break;
		default:
			printf("boot mode: None\n");
			boot_mode[PL] = BOOT_MODE_UNDEFINE;
		}
	}

	/*
	 * We don't clear boot mode reg when its value stands for the reboot
	 * reason or others(in the future), the kernel will need and clear it.
	 */
	if (clear_boot_reg)
		writel(BOOT_NORMAL, (void *)CONFIG_ROCKCHIP_BOOT_MODE_REG);

	if (boot_mode[PH] != -EINVAL)
		return boot_mode[PH];
	else if (boot_mode[PM] != -EINVAL)
		return boot_mode[PM];
	else
		return boot_mode[PL];
}

int setup_boot_mode(void)
{
	char env_preboot[256] = {0};

	switch (plat_boot_mode()) {
	case BOOT_MODE_BOOTLOADER:
		printf("enter fastboot!\n");
#if defined(CONFIG_FASTBOOT_FLASH_MMC_DEV)
		snprintf(env_preboot, 256,
				"setenv preboot; mmc dev %x; fastboot usb 0; ",
				CONFIG_FASTBOOT_FLASH_MMC_DEV);
#elif defined(CONFIG_FASTBOOT_FLASH_NAND_DEV)
		snprintf(env_preboot, 256,
				"setenv preboot; fastboot usb 0; ");
#endif
		env_set("preboot", env_preboot);
		run_command("fastboot usb 0", 0);
		break;
	case BOOT_MODE_UMS:
		printf("enter UMS!\n");
		env_set("preboot", "setenv preboot; ums mmc 0");
		break;
#if defined(CONFIG_CMD_DFU)
	case BOOT_MODE_DFU:
		printf("enter DFU!\n");
		env_set("preboot", "setenv preboot; dfu 0 ${devtype} ${devnum}; rbrom");
		break;
#endif
	case BOOT_MODE_LOADER:
		printf("enter Rockusb!\n");
		env_set("preboot", "setenv preboot; download");
		run_command("download", 0);
		break;
	case BOOT_MODE_CHARGING:
		printf("enter charging!\n");
		env_set("preboot", "setenv preboot; charge");
		break;
	}

	return 0;
}
