// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 MediaTek Inc.
 */

#include <common.h>
#include <cpu_func.h>
#include <dm.h>
#include <init.h>
#include <wdt.h>
#include <dm/uclass-internal.h>
#include <linux/arm-smccc.h>
#include <asm/global_data.h>
#include <asm/arch/misc.h>

int arch_cpu_init(void)
{
	icache_enable();

	/* Allow console to be disabled through CONFIG_DISABLE_CONSOLE */
	gd->flags |= GD_FLG_DISABLE_CONSOLE;

	return 0;
}

void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}

#ifdef MTK_SIP_PLAT_BINFO
/**
 * mediatek_sip_part_name - get the part name
 *
 * Retrieve the part name of platform description.
 * This only applicable to SoCs that support SIP plat binfo
 * SMC call.
 *
 * Return:
 * * > 0 - the part name invoked
 * * 0   - error or no part name invoked
 */
u32 mediatek_sip_part_name(void)
{
	struct arm_smccc_res res __maybe_unused;
	u32 ret = 0;

	arm_smccc_smc(MTK_SIP_PLAT_BINFO, 0, 0, 0, 0, 0, 0, 0, &res);
	ret = res.a1;

	if (res.a0)
		return 0;
	else
		return ret;
}

/**
 * mediatek_sip_segm_name - get the segment name
 *
 * Retrieve the segment name of platform description.
 * This only applicable to SoCs that support SIP plat binfo
 * SMC call.
 *
 * Return:
 * * > 0 - the segment name invoked
 * * 0   - error or no segment name invoked
 */
u32 mediatek_sip_segm_name(void)
{
	struct arm_smccc_res res __maybe_unused;
	u32 ret = 0;

	arm_smccc_smc(MTK_SIP_PLAT_BINFO, 1, 0, 0, 0, 0, 0, 0, &res);
	ret = res.a1;
	if (res.a0)
		return 0;
	else
		return ret;
}
#else
u32 mediatek_sip_part_name(void)
{
	return 0;
}

u32 mediatek_sip_segm_name(void)
{
	return 0;
}
#endif

/**
 * mtk_reserved_memory_init - Registers reserved memory regions in the device tree
 * @blob: Pointer to the FDT (device tree) blob
 * @boot_arg: Pointer to boot argument structure with reserved memory info
 *
 *
 * Checks boot argument validity, iterates over all reserved memory regions
 * from the bootloader, and adds each region to the device tree using
 * fdtdec_add_reserved_memory(). This allows the OS to recognize and protect
 * those regions from normal memory allocation.
 */
void mtk_reserved_memory_init(void *blob, struct boot_argument *boot_arg)
{
	struct fdt_memory gic_rd_tables;

	debug("%s: mblock_magic: %x, reserved_num: %d\n", __func__,
	      boot_arg->mblock_info.magic_number,
	      boot_arg->mblock_info.reserved_num);
	if (boot_arg->magic_number == BOOT_ARGUMENT_MAGIC &&
	    boot_arg->mblock_info.magic_number == MBLOCK_MAGIC) {
		struct mblock_info_t *mblock_info = &boot_arg->mblock_info;

		for (int i = 0; i < mblock_info->reserved_num; i++) {
			gic_rd_tables.start = mblock_info->reserved[i].start;
			gic_rd_tables.end = gic_rd_tables.start +
					    mblock_info->reserved[i].size - 1;

			fdtdec_add_reserved_memory(blob, mblock_info->reserved[i].name,
						   &gic_rd_tables, NULL, 0, NULL,
						   FDTDEC_RESERVED_MEMORY_NO_MAP);
			debug("%s: %d, start = 0x%llx, size = 0x%llx, mapping = %d, name = %s\n",
			      __func__, i,
			      mblock_info->reserved[i].start,
			      mblock_info->reserved[i].size,
			      mblock_info->reserved[i].mapping,
			      mblock_info->reserved[i].name);
		}
	}
}
