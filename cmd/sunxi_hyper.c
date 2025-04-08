/*
 * (C) Copyright 2023 allwinnertech  <qinguangzhi@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 * qinguangzhi@allwinnertech.com
 */

#include <common.h>
#include <command.h>
#include <mapmem.h>
#include <bootm.h>
#include <smc.h>
#include <malloc.h>
#include <private_uboot.h>
#include <fdt_support.h>
#include <sys_partition.h>
#include <asm/global_data.h>
#include <linux/arm-smccc.h>
#include <dm.h>
#include <dm/root.h>

#include <sunxi_image_verifier.h>

DECLARE_GLOBAL_DATA_PTR;

#define HYPER_IMG_ZONE	"hyper"
#define HYPER_DTB_ZONE	"hyper-dtb"
#define	HYPER_DOM_ZONE	"hyper-dom"

#define ARM_SVC_HYPER	(0x8000ff40)
#define ARM_SVC_RUNNSOS (0x8000ff04)


#define HYPER_IMG_BASE	(0x60000000)
#define	HYPER_DTB_BASE	(0x65000000)
#define	HYPER_DOM_BASE	(0x41000000)


#define KERNEL_IMG_ZONE	"kernel"
#define KERNEL_IMG_BASE	(0x60000000)

/*
 * This fuction can load some image like xen
 */

int load_hyper_img(void *load_entry)
{
	struct {
		uint64_t jump_ins;
		uint64_t offset;
		uint64_t size;
		uint64_t flag;
		uint64_t res0;
		uint64_t res1;
		uint64_t res2;
		uint32_t magic;    /* ARM\x64*/
		uint64_t offset2pe;
	} xen_header;

	uint hyper_offset = 0, hyper_len = 0;
	sunxi_partition_get_info_byname(HYPER_IMG_ZONE, &hyper_offset, &hyper_len);
	if (!hyper_offset) {
		pr_error("cant find part named %s\n", HYPER_IMG_ZONE);
		return -1;
	} else
		pr_error("find part named %s\n", HYPER_IMG_ZONE);

	sunxi_flash_read(hyper_offset, hyper_len, load_entry);

	memcpy(&xen_header, load_entry, sizeof(xen_header));

	printf("hyper image magic = %x \n", xen_header.magic);

	return 0;
}

/* TODO: We use kernel Image, No need check header.
 * if need zimage, fix up this function by self.
 */
int kernel_image64_check(void *addr, ulong size)
{
    return 0;
}

int load_kernel_img(void *load_base)
{
	uint kernel_offset = 0, kernel_len = 0;
	sunxi_partition_get_info_byname(KERNEL_IMG_ZONE, &kernel_offset, &kernel_len);
	if (!kernel_offset) {
		pr_error("cant find part named %s\n", KERNEL_IMG_ZONE);
		return -1;
	} else
		pr_error("find part named %s\n", KERNEL_IMG_ZONE);

	printf("kernel len %x \n", kernel_len);

	sunxi_flash_read(kernel_offset, 0x50000, load_base);

	return 0;
}

/* this function can load android boot.img */
int load_linux_boot_img(void)
{
	int ret = 0;
	uint kernel_offset = 0, kernel_size = 0;
	ulong kernel_data = 0, kernel_len = 0;
	ulong ramdisk_data = 0, ramdisk_len = 0;
	ulong dtb_data = 0, dtb_len = 0;

	sunxi_partition_get_info_byname("boot", &kernel_offset, &kernel_size);

	if (!kernel_offset) {
		pr_error("can`t find part named %s\n", "boot");
		return -1;
	}

	sunxi_flash_read(kernel_offset, kernel_size, (void *)HYPER_DOM_BASE);

	android_image_get_kernel((const struct andr_img_hdr *)HYPER_DOM_BASE, 0, &kernel_data, &kernel_len);
	android_image_get_ramdisk((const struct andr_img_hdr *)HYPER_DOM_BASE, &ramdisk_data, &ramdisk_len);
	android_image_get_dtb((const struct andr_img_hdr *)HYPER_DOM_BASE, &dtb_data, &dtb_len);
	// update kernel addr and size for domu
	// hyper_dtb_update(working_fdt, DOM1_KERNEL_NODE, kernel_data, kernel_len);
	// hyper_dtb_update(working_fdt, DOM1_RAMDISK_NODE, ramdisk_data, ramdisk_len);
	// hyper_dtb_update(working_fdt, DOM1_TREE_NODE, dtb_data, dtb_len);


	ret = fdt_check_header((int *)dtb_data);
	if (ret < 0) {
		printf("domu fdt check fail: %s\n", fdt_strerror(ret));
		return 1;
	} else {
		printf("domu fdt check ok\n");
	}

	return ret;
}


/* load dtb in memory, but use this dtb, dont't use */
int load_dtb(const char *name, void *dtb_base)
{
	int ret = 0;
	uint dtb_offset = 0, dtb_size = 0;

	sunxi_partition_get_info_byname(name, &dtb_offset, &dtb_size);
	if (!dtb_offset) {
		pr_error("can't find part named %s\n", name);
		return -1;
	} else
		pr_error("find part named %s \n", name);

	sunxi_flash_read(dtb_offset, dtb_size, dtb_base);

	ret = fdt_check_header((int *)dtb_base);
	if (ret) {
		printf("%s fdt check fail: %s\n", name, fdt_strerror(ret));
		return -1;
	}

	set_working_fdt_addr((ulong)dtb_base);

	return 0;
}

int do_sunxi_boot_hyper(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{

	u32 arg1 = simple_strtoul(argv[1], NULL, 16);
	void *load_entry = (void *)arg1;

	u32 type = simple_strtoul(argv[2], NULL, 16);

	if (type == 0) { /* Test xen image */
		if (load_hyper_img(load_entry)) {
			printf("load hyper image fail\n");
			return -1;
		}
	} else { /* Test ARM64 bIamge image */
		ulong img_len = load_kernel_img(load_entry);
		if (img_len < 0) {
			printf("load kernel image fali\n");
			return -1;
		}
		if (kernel_image64_check(load_entry, 0x100)) {
			printf("check kernel image fali\n");
			return -1;
		}
	}

	printf("jump to hyper!\n");

	// prepare for booting hyper
	board_quiesce_devices();
	dm_remove_devices_flags(DM_REMOVE_ACTIVE_ALL);
	cleanup_before_linux();

	/* we will update some node in uboot, so ugly */
	sunxi_smc_call_atf(ARM_SVC_HYPER, (ulong)load_entry, (ulong)working_fdt,  1);
	return 0;
}

U_BOOT_CMD(
	boot_hyper,	3,	1,	do_sunxi_boot_hyper,
	"boot hyper",
	"boot_hyper load_addr os_type"
);
