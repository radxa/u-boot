// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 MediaTek Inc.
 * Author: Chris-QJ Chen <chris-qj.chen@mediatek.com>
 */

#include <common.h>
#include <dm.h>
#include <efi_loader.h>
#include <env.h>
#include <env_internal.h>
#include <fdt_support.h>
#include <iot_ab.h>
#include <net.h>
#include <asm/io.h>
#include <log.h>
#include <asm/arch/misc.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <linux/arm-smccc.h>

#define MT8391_UPDATABLE_IMAGES	5

#if CONFIG_IS_ENABLED(EFI_HAVE_CAPSULE_SUPPORT)
static struct efi_fw_image fw_images[MT8391_UPDATABLE_IMAGES] = {0};

struct efi_capsule_update_info update_info = {
#if IS_ENABLED(CONFIG_MEDIATEK_IOT_AB_BOOT_SUPPORT)
#if (IS_ENABLED(CONFIG_DFU_MTD))
	.dfu_string = "mtd nor0=bl2.img part 1;"
			"fip.bin part %d;firmware.a.ubi part %d;u-boot-env.bin part 9",
#elif (IS_ENABLED(CONFIG_UFS_MEDIATEK))
	.dfu_string = "ufs 0=bl2.img raw 0x0 0x400 dev 0;"
			"fip.bin part 2 %d;firmware.vfat part 2 %d;u-boot-env.bin raw 0x0 0x400 dev 1",
#else
	.dfu_string = "mmc 0=bl2.img raw 0x0 0x2000 mmcpart 1;"
			"fip.bin part 0 %d;firmware.vfat part 0 %d;u-boot-env.bin raw 0x0 0x2000 mmcpart 2",
#endif
#else
#if (IS_ENABLED(CONFIG_DFU_MTD))
	.dfu_string = "mtd nor0=bl2.img part 1;"
			"fip.bin part 2;firmware.a.ubi part 4;u-boot-env.bin part 9",
#elif (IS_ENABLED(CONFIG_UFS_MEDIATEK))
	.dfu_string = "ufs 0=bl2.img raw 0x0 0x400 dev 0;"
			"fip.bin part 2 1;firmware.vfat part 2 3;u-boot-env.bin raw 0x0 0x400 dev 1",
#else
	.dfu_string = "mmc 0=bl2.img raw 0x0 0x2000 mmcpart 1;"
			"fip.bin part 0 1;firmware.vfat part 0 3;u-boot-env.bin raw 0x0 0x2000 mmcpart 2",
#endif
#endif
	.images = fw_images,
};

u8 num_image_type_guids = MT8391_UPDATABLE_IMAGES;
#endif

#if defined(CONFIG_EFI_HAVE_CAPSULE_SUPPORT) && defined(CONFIG_EFI_PARTITION)
enum mt8390_updatable_images {
	MT8391_BL2_IMAGE = 1,
	MT8391_FIP_IMAGE,
	MT8391_FW_IMAGE,
	MT8391_ENV_IMAGE,
	MT8391_FIT_IMAGE,
};

static bool board_is_genio_720_evk(void)
{
	return CONFIG_IS_ENABLED(TARGET_MT8391) &&
		of_machine_is_compatible("mediatek,genio-720-evk");
}

static bool board_is_genio_720_evk_ufs(void)
{
	return CONFIG_IS_ENABLED(TARGET_MT8391) &&
		of_machine_is_compatible("mediatek,genio-720-evk-ufs");
}

static bool board_is_genio_720_evk_ufs_qspi(void)
{
	return CONFIG_IS_ENABLED(TARGET_MT8391) &&
		of_machine_is_compatible("mediatek,genio-720-evk-ufs-qspi");
}

void mediatek_capsule_update_board_setup(void)
{
	fw_images[0].image_index = MT8391_FIT_IMAGE;
	fw_images[1].image_index = MT8391_FIP_IMAGE;
	fw_images[2].image_index = MT8391_BL2_IMAGE;
	fw_images[3].image_index = MT8391_FW_IMAGE;
	fw_images[4].image_index = MT8391_ENV_IMAGE;

	if (board_is_genio_720_evk()) {
		efi_guid_t image_type_guid = GENIO_720_EVK_FIT_IMAGE_GUID;
		efi_guid_t uboot_image_type_guid = GENIO_720_EVK_FIP_IMAGE_GUID;
		efi_guid_t bl2_image_type_guid = GENIO_720_EVK_BL2_IMAGE_GUID;
		efi_guid_t fw_image_type_guid = GENIO_720_EVK_FW_IMAGE_GUID;
		efi_guid_t env_image_type_guid = GENIO_720_EVK_ENV_IMAGE_GUID;

		guidcpy(&fw_images[0].image_type_id, &image_type_guid);
		guidcpy(&fw_images[1].image_type_id, &uboot_image_type_guid);
		guidcpy(&fw_images[2].image_type_id, &bl2_image_type_guid);
		guidcpy(&fw_images[3].image_type_id, &fw_image_type_guid);
		guidcpy(&fw_images[4].image_type_id, &env_image_type_guid);

		fw_images[0].fw_name = u"GENIO-720-EVK-FIT";
		fw_images[1].fw_name = u"GENIO-720-EVK-FIP";
		fw_images[2].fw_name = u"GENIO-720-EVK-BL2";
		fw_images[3].fw_name = u"GENIO-720-EVK-FW";
		fw_images[4].fw_name = u"GENIO-720-EVK-ENV";
	} else if (board_is_genio_720_evk_ufs()) {
		efi_guid_t image_type_guid = GENIO_720_EVK_UFS_FIT_IMAGE_GUID;
		efi_guid_t uboot_image_type_guid = GENIO_720_EVK_UFS_FIP_IMAGE_GUID;
		efi_guid_t bl2_image_type_guid = GENIO_720_EVK_UFS_BL2_IMAGE_GUID;
		efi_guid_t fw_image_type_guid = GENIO_720_EVK_UFS_FW_IMAGE_GUID;
		efi_guid_t env_image_type_guid = GENIO_720_EVK_UFS_ENV_IMAGE_GUID;

		guidcpy(&fw_images[0].image_type_id, &image_type_guid);
		guidcpy(&fw_images[1].image_type_id, &uboot_image_type_guid);
		guidcpy(&fw_images[2].image_type_id, &bl2_image_type_guid);
		guidcpy(&fw_images[3].image_type_id, &fw_image_type_guid);
		guidcpy(&fw_images[4].image_type_id, &env_image_type_guid);

		fw_images[0].fw_name = u"GENIO-720-EVK-UFS-FIT";
		fw_images[1].fw_name = u"GENIO-720-EVK-UFS-FIP";
		fw_images[2].fw_name = u"GENIO-720-EVK-UFS-BL2";
		fw_images[3].fw_name = u"GENIO-720-EVK-UFS-FW";
		fw_images[4].fw_name = u"GENIO-720-EVK-UFS-ENV";
	} else if (board_is_genio_720_evk_ufs_qspi()) {
		efi_guid_t image_type_guid = GENIO_720_EVK_UFS_QSPI_FIT_IMAGE_GUID;
		efi_guid_t uboot_image_type_guid = GENIO_720_EVK_UFS_QSPI_FIP_IMAGE_GUID;
		efi_guid_t bl2_image_type_guid = GENIO_720_EVK_UFS_QSPI_BL2_IMAGE_GUID;
		efi_guid_t fw_image_type_guid = GENIO_720_EVK_UFS_QSPI_FW_IMAGE_GUID;
		efi_guid_t env_image_type_guid = GENIO_720_EVK_UFS_QSPI_ENV_IMAGE_GUID;

		guidcpy(&fw_images[0].image_type_id, &image_type_guid);
		guidcpy(&fw_images[1].image_type_id, &uboot_image_type_guid);
		guidcpy(&fw_images[2].image_type_id, &bl2_image_type_guid);
		guidcpy(&fw_images[3].image_type_id, &fw_image_type_guid);
		guidcpy(&fw_images[4].image_type_id, &env_image_type_guid);

		fw_images[0].fw_name = u"GENIO-720-EVK-UFS-QSPI-FIT";
		fw_images[1].fw_name = u"GENIO-720-EVK-UFS-QSPI-FIP";
		fw_images[2].fw_name = u"GENIO-720-EVK-UFS-QSPI-BL2";
		fw_images[3].fw_name = u"GENIO-720-EVK-UFS-QSPI-FW";
		fw_images[4].fw_name = u"GENIO-720-EVK-UFS-QSPI-ENV";
	}
}

#if IS_ENABLED(CONFIG_MEDIATEK_IOT_AB_BOOT_SUPPORT)
void set_dfu_alt_info(char *interface, char *devstr)
{
	char alt[BOOTCTRL_DFU_ALT_LEN] = { 0 };
	const char *s = env_get(BOOTCTRL_ENV);

	if (s) {
		if (!strcmp(s, "a")) {
			if (sprintf(alt, update_info.dfu_string,
				    BOOTCTRL_FIP_NUM + PART_BOOT_B,
				    BOOTCTRL_FW_NUM + PART_BOOT_B) < 0)
				return;
		} else if (!strcmp(s, "b")) {
			if (sprintf(alt, update_info.dfu_string,
				    BOOTCTRL_FIP_NUM, BOOTCTRL_FW_NUM) < 0)
				return;
		}
		env_set("dfu_alt_info", alt);
	}
}
#endif
#endif /* CONFIG_EFI_HAVE_CAPSULE_SUPPORT && CONFIG_EFI_PARTITION */

static const char *board_full_name;

#if (IS_ENABLED(CONFIG_BOARD_LATE_INIT))

static int env_append_bootargs(const char *append_args)
{
	/* MAX_CMDLINE_SIZE taken from boot/bootm.c */
	const int MAX_CMDLINE_SIZE = SZ_4K;
	const char *old_args;
	char *buf;
	int newlen;
	int ret;

	old_args = env_get("bootargs");
	if (!old_args)
		return env_set("bootargs", append_args);

	/* Note that we add spaces between the old and new arguments */
	newlen = strlen(old_args) + strlen(append_args) + 2;
	if (newlen >= MAX_CMDLINE_SIZE)
		return -E2BIG;
	buf = malloc(MAX_CMDLINE_SIZE);
	if (!buf)
		return -ENOMEM;
	snprintf(buf, MAX_CMDLINE_SIZE, "%s %s", old_args, append_args);
	ret = env_set("bootargs", buf);

	free(buf);
	return ret;
}

int board_late_init(void)
{
	/* Construct full board name from SoC part name and boot method,
	 * and set to env variables "hostname" and "bootargs".
	 */
	u32 part = 0;
	const char *board_name = NULL;
	const char *boot_suffix = NULL;
	char hostname_buf[64] = {0};
	char cmdline_buf[64] = {0};
	enum env_location loc = ENVL_MMC;

	part = mediatek_sip_part_name();
	switch (part) {
	case 0x8371:
		board_name = "genio-520-evk";
		board_full_name = "MediaTek Genio 520 EVK";
		break;
	case 0x8391:
	default:
		board_name = "genio-720-evk";
		board_full_name = "MediaTek Genio 720 EVK";
		break;
	}

	loc = env_get_location(ENVOP_LOAD, 0);
	log_info("u-boot env location detected: %d\n", loc);
	switch (loc) {
	case ENVL_SCSI:
		boot_suffix = "-ufs";
		break;
	case ENVL_SPI_FLASH:
		boot_suffix = "-nor";
		break;
	case ENVL_MMC:
		break;
	default:
		break;
	}

	if (board_name) {
		if (boot_suffix)
			snprintf(hostname_buf, sizeof(hostname_buf), "%s%s",
				 board_name, boot_suffix);
		else
			snprintf(hostname_buf, sizeof(hostname_buf), "%s", board_name);
		env_set("hostname", hostname_buf);
		snprintf(cmdline_buf, sizeof(cmdline_buf), "systemd.hostname=%s", hostname_buf);
		env_append_bootargs(cmdline_buf);
	}

	return 0;
}
#endif

#if (IS_ENABLED(CONFIG_OF_BOARD_SETUP))
int ft_board_setup(void *blob, struct bd_info *bd)
{
	mtk_reserved_memory_init(blob, BOOT_ARGUMENT);

	if (board_full_name) {
		log_info("%s board model:%s\n", __func__, board_full_name);
		do_fixup_by_path_string(blob, "/", "model",
					board_full_name);
	}

	return 0;
}
#endif

int board_init(void)
{
	struct udevice *dev;
	int ret;

	if (CONFIG_IS_ENABLED(USB_GADGET)) {
		ret = uclass_get_device(UCLASS_USB_GADGET_GENERIC, 0, &dev);
		if (ret) {
			pr_err("%s: Cannot find USB device\n", __func__);
			return ret;
		}
	}

	if (CONFIG_IS_ENABLED(USB_ETHER))
		usb_ether_init();

	if (CONFIG_IS_ENABLED(MMC_MTK)) {
		static const char * const mmc1_regulators[] = {
			"vpa",
			"vsim1",
			NULL,
		};

		ret = regulator_list_autoset(mmc1_regulators, NULL, false);
		if (ret) {
			pr_err("%s: Unable to init all mmc1 regulators\n", __func__);
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_EFI_HAVE_CAPSULE_SUPPORT) &&
	    IS_ENABLED(CONFIG_EFI_PARTITION))
		mediatek_capsule_update_board_setup();

	return 0;
}
