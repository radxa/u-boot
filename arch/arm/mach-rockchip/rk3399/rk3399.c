/*
 * Copyright (c) 2016 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <display_options.h>
#include <dm.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <misc.h>
#include <asm/armv8/mmu.h>
#include <asm/arch/bootrom.h>
#include <asm/arch/grf_rk3399.h>
#include <asm/arch/cru_rk3399.h>
#include <asm/arch/hardware.h>
#include <asm/io.h>
#include <syscon.h>

DECLARE_GLOBAL_DATA_PTR;

#define GRF_EMMCCORE_CON11 0xff77f02c
#define PMU_GRF_SOC_CON0   0xff320180

static struct mm_region rk3399_mem_map[] = {
	{
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0xf8000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0xf8000000UL,
		.phys = 0xf8000000UL,
		.size = 0x08000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = rk3399_mem_map;

const char * const boot_devices[BROM_LAST_BOOTSOURCE + 1] = {
	[BROM_BOOTSOURCE_EMMC] = "/sdhci@fe330000",
	[BROM_BOOTSOURCE_SPINOR] = "/spi@ff1d0000",
	[BROM_BOOTSOURCE_SD] = "/dwmmc@fe320000",
};

#ifdef CONFIG_SPL_BUILD

#define TIMER_CHN10_BASE	0xff8680a0
#define TIMER_END_COUNT_L	0x00
#define TIMER_END_COUNT_H	0x04
#define TIMER_INIT_COUNT_L	0x10
#define TIMER_INIT_COUNT_H	0x14
#define TIMER_CONTROL_REG	0x1c

#define TIMER_EN	0x1
#define	TIMER_FMODE	(0 << 1)
#define	TIMER_RMODE	(1 << 1)

void rockchip_stimer_init(void)
{
	writel(0xffffffff, TIMER_CHN10_BASE + TIMER_END_COUNT_L);
	writel(0xffffffff, TIMER_CHN10_BASE + TIMER_END_COUNT_H);
	writel(0, TIMER_CHN10_BASE + TIMER_INIT_COUNT_L);
	writel(0, TIMER_CHN10_BASE + TIMER_INIT_COUNT_H);
	writel(TIMER_EN | TIMER_FMODE, TIMER_CHN10_BASE + TIMER_CONTROL_REG);
}
#endif

#define GRF_BASE	0xff770000
#define PMUGRF_BASE	0xff320000
#define PMUSGRF_BASE	0xff330000
#define PMUCRU_BASE	0xff750000
#define NIU_PERILP_NSP_ADDR	0xffad8188
#define QOS_PRIORITY_LEVEL(h, l)	((((h) & 3) << 8) | ((l) & 3))

#ifndef CONFIG_TPL_BUILD
int arch_cpu_init(void)
{
	struct rk3399_pmugrf_regs *pmugrf = (void *)PMUGRF_BASE;
	struct rk3399_grf_regs * const grf = (void *)GRF_BASE;

	/* We do some SoC one time setting here. */

#ifdef CONFIG_SPL_BUILD
	struct rk3399_pmusgrf_regs *sgrf = (void *)PMUSGRF_BASE;

	/*
	 * Disable DDR and SRAM security regions.
	 *
	 * As we are entered from the BootROM, the region from
	 * 0x0 through 0xfffff (i.e. the first MB of memory) will
	 * be protected. This will cause issues with the DW_MMC
	 * driver, which tries to DMA from/to the stack (likely)
	 * located in this range.
	 */
	rk_clrsetreg(&sgrf->ddr_rgn_con[16], 0x1ff, 0);
	rk_clrreg(&sgrf->slv_secure_con4, 0x2000);
#endif

	/* eMMC clock generator: disable the clock multipilier */
	rk_clrreg(&grf->emmccore_con[11], 0x0ff);

	/* PWM3 select pwm3a io */
	rk_clrreg(&pmugrf->soc_con0, 1 << 5);

#if defined(CONFIG_ROCKCHIP_RK3399PRO)
	struct rk3399_pmucru *pmucru = (void *)PMUCRU_BASE;

	/* set wifi_26M to 24M and disabled by default */
	writel(0x7f002000, &pmucru->pmucru_clksel[1]);
	writel(0x01000100, &pmucru->pmucru_clkgate_con[0]);
#endif

	/* Set perilp_nsp QOS priority to 3 for USB 3.0 */
	writel(QOS_PRIORITY_LEVEL(3, 3), NIU_PERILP_NSP_ADDR);

	return 0;
}
#endif

void board_debug_uart_init(void)
{
#define GRF_BASE	0xff770000
	struct rk3399_grf_regs * const grf = (void *)GRF_BASE;

#if defined(CONFIG_DEBUG_UART_BASE) && (CONFIG_DEBUG_UART_BASE == 0xff180000)
	/* Enable early UART0 on the RK3399 */
	rk_clrsetreg(&grf->gpio2c_iomux,
		     GRF_GPIO2C0_SEL_MASK,
		     GRF_UART0BT_SIN << GRF_GPIO2C0_SEL_SHIFT);
	rk_clrsetreg(&grf->gpio2c_iomux,
		     GRF_GPIO2C1_SEL_MASK,
		     GRF_UART0BT_SOUT << GRF_GPIO2C1_SEL_SHIFT);
#else
	/* Enable early UART2 channel on the RK3399/RK3399PRO */
	rk_clrsetreg(&grf->gpio4c_iomux,
		     GRF_GPIO4C3_SEL_MASK,
		     GRF_UART2DBGC_SIN << GRF_GPIO4C3_SEL_SHIFT);
	rk_clrsetreg(&grf->gpio4c_iomux,
		     GRF_GPIO4C4_SEL_MASK,
		     GRF_UART2DBGC_SOUT << GRF_GPIO4C4_SEL_SHIFT);
#if defined(CONFIG_ROCKCHIP_RK3399PRO)
	/* Set channel A as UART2 input */
	rk_clrsetreg(&grf->soc_con7,
		     GRF_UART_DBG_SEL_MASK,
		     GRF_UART_DBG_SEL_A << GRF_UART_DBG_SEL_SHIFT);
#else
	/* Set channel C as UART2 input */
	rk_clrsetreg(&grf->soc_con7,
		     GRF_UART_DBG_SEL_MASK,
		     GRF_UART_DBG_SEL_C << GRF_UART_DBG_SEL_SHIFT);
#endif
#endif
}

#if CONFIG_IS_ENABLED(ROCKCHIP_EFUSE) && !defined(CONFIG_SPL_BUILD)
#define RK3399_EFUSE_INFO_SIZE	0x23
#define RK3399_CPUID_OFFSET	0x07
#define RK3399_CPUID_SIZE	0x10
#define RK3399_SPEC_OFFSET	0x06
#define RK3399_CUSTOMER_OFFSET	0x22
#define RK3399_CPU_BIG_LEAKAGE_OFFSET	0x17
#define RK3399_GPU_LEAKAGE_OFFSET	0x18
#define RK3399_CENTER_LEAKAGE_OFFSET	0x19
#define RK3399_CPU_LITTLE_LEAKAGE_OFFSET	0x1a
#define RK3399_LOGIC_LEAKAGE_OFFSET	0x1b
#define RK3399_WAFER_INFO_OFFSET		0x1c
#define RK3399_STANDARD_LITTLE_MAX	1416000000ULL
#define RK3399_STANDARD_BIG_MAX		1800000000ULL
#define RK3399_OP1_LITTLE_MAX		1512000000ULL
#define RK3399_OP1_BIG_MAX		2016000000ULL
#define RK3399_GPU_HAS_297M		BIT(0)
#define RK3399_GPU_HAS_300M		BIT(1)
#define RK3399_GPU_HAS_500M		BIT(2)

struct rk3399_opp_entry {
	const char *name;
	u64 rate;
	u32 microvolt;
	u32 max_microvolt;
	bool latency;
	bool suspend;
};

struct rk3399_opp_profile {
	u64 little_max;
	u64 big_max;
	u32 gpu_required;
	u32 gpu_forbidden;
};

struct rk3399_opp_state {
	u64 little_max;
	u64 big_max;
	u32 gpu_mask;
};

struct rk3399_opp_table_desc {
	const char *device_path;
	const char *standard_first;
	const char *standard_last;
	const char *legacy_last;
	const struct rk3399_opp_entry *standard_opps;
	int standard_count;
	const struct rk3399_opp_entry *high_freq_delta;
	int high_freq_count;
};

enum rk3399_opp_format {
	RK3399_OPP_FORMAT_UNKNOWN,
	RK3399_OPP_FORMAT_STANDARD,
	RK3399_OPP_FORMAT_LEGACY,
	RK3399_OPP_FORMAT_MERGED,
};

enum rk3399_opp_table_id {
	RK3399_OPP_LITTLE,
	RK3399_OPP_BIG,
	RK3399_OPP_GPU,
	RK3399_OPP_TABLE_COUNT,
};

static const u64 rk3399_gpu_signature_rates[] = {
	297000000,
	300000000,
	500000000,
};

static const struct rk3399_opp_profile rk3399_standard_profile = {
	.little_max = RK3399_STANDARD_LITTLE_MAX,
	.big_max = RK3399_STANDARD_BIG_MAX,
	.gpu_required = RK3399_GPU_HAS_300M,
	.gpu_forbidden = RK3399_GPU_HAS_297M | RK3399_GPU_HAS_500M,
};

static const struct rk3399_opp_profile rk3399_high_freq_profile = {
	.little_max = RK3399_OP1_LITTLE_MAX,
	.big_max = RK3399_OP1_BIG_MAX,
	.gpu_required = RK3399_GPU_HAS_297M | RK3399_GPU_HAS_500M,
};

/*
 * rk2410 includes rk3399-opp.dtsi first and rk3399-op1-opp.dtsi second.
 * Preserve the complete standard tables (including leakage/PVTM and
 * opp-microvolt-L* data) and add only the frequencies unique to OP1.  This
 * produces the same usable frequency set without duplicate-frequency OPPs.
 */
static const struct rk3399_opp_entry rk3399_op1_little_delta[] = {
	{ "opp-1512000000", 1512000000, 1150000, 0, false, false },
};

static const struct rk3399_opp_entry rk3399_op1_big_delta[] = {
	{ "opp-2016000000", 2016000000, 1250000, 0, false, false },
};

static const struct rk3399_opp_entry rk3399_op1_gpu_delta[] = {
	{ "opp-297000000", 297000000, 800000, 0, false, false },
	{ "opp-500000000", 500000000, 850000, 0, false, false },
};

/* Safe nominal standard tables for converting a pure OP1 input DTB. */
static const struct rk3399_opp_entry rk3399_standard_little_opps[] = {
	{ "opp-408000000",  408000000,  825000, 1250000, true, false },
	{ "opp-600000000",  600000000,  825000, 1250000, true, false },
	{ "opp-816000000",  816000000,  850000, 1250000, true, true },
	{ "opp-1008000000", 1008000000,  925000, 1250000, true, false },
	{ "opp-1200000000", 1200000000, 1000000, 1250000, true, false },
	{ "opp-1416000000", 1416000000, 1125000, 1250000, true, false },
};

static const struct rk3399_opp_entry rk3399_standard_big_opps[] = {
	{ "opp-408000000",  408000000,  825000, 1250000, true, false },
	{ "opp-600000000",  600000000,  825000, 1250000, true, false },
	{ "opp-816000000",  816000000,  825000, 1250000, true, true },
	{ "opp-1008000000", 1008000000,  875000, 1250000, true, false },
	{ "opp-1200000000", 1200000000,  950000, 1250000, true, false },
	{ "opp-1416000000", 1416000000, 1025000, 1250000, true, false },
	{ "opp-1608000000", 1608000000, 1100000, 1250000, true, false },
	{ "opp-1800000000", 1800000000, 1200000, 1250000, true, false },
};

static const struct rk3399_opp_entry rk3399_standard_gpu_opps[] = {
	{ "opp-200000000", 200000000,  825000, 1150000, false, false },
	{ "opp-300000000", 300000000,  825000, 1150000, false, false },
	{ "opp-400000000", 400000000,  825000, 1150000, false, false },
	{ "opp-600000000", 600000000,  925000, 1150000, false, false },
	{ "opp-800000000", 800000000, 1100000, 1150000, false, false },
};

static const struct rk3399_opp_table_desc
rk3399_opp_tables[RK3399_OPP_TABLE_COUNT] = {
	[RK3399_OPP_LITTLE] = {
		.device_path = "/cpus/cpu@0",
		.standard_first = "opp-408000000",
		.standard_last = "opp-1416000000",
		.legacy_last = "opp06",
		.standard_opps = rk3399_standard_little_opps,
		.standard_count = ARRAY_SIZE(rk3399_standard_little_opps),
		.high_freq_delta = rk3399_op1_little_delta,
		.high_freq_count = ARRAY_SIZE(rk3399_op1_little_delta),
	},
	[RK3399_OPP_BIG] = {
		.device_path = "/cpus/cpu@100",
		.standard_first = "opp-408000000",
		.standard_last = "opp-1800000000",
		.legacy_last = "opp08",
		.standard_opps = rk3399_standard_big_opps,
		.standard_count = ARRAY_SIZE(rk3399_standard_big_opps),
		.high_freq_delta = rk3399_op1_big_delta,
		.high_freq_count = ARRAY_SIZE(rk3399_op1_big_delta),
	},
	[RK3399_OPP_GPU] = {
		.device_path = "/gpu@ff9a0000",
		.standard_first = "opp-200000000",
		.standard_last = "opp-800000000",
		.legacy_last = "opp05",
		.standard_opps = rk3399_standard_gpu_opps,
		.standard_count = ARRAY_SIZE(rk3399_standard_gpu_opps),
		.high_freq_delta = rk3399_op1_gpu_delta,
		.high_freq_count = ARRAY_SIZE(rk3399_op1_gpu_delta),
	},
};

static int rk3399_read_efuse_info(u8 *efuse)
{
	struct udevice *dev;
	int ret;

	ret = uclass_get_device_by_driver(UCLASS_MISC,
					  DM_GET_DRIVER(rockchip_efuse), &dev);
	if (ret)
		return ret;

	return misc_read(dev, 0, efuse, RK3399_EFUSE_INFO_SIZE);
}

static int rk3399_efuse_bin(const u8 *efuse, u8 *specification,
			    u8 *customer_demand)
{
	u8 customer = (efuse[RK3399_CUSTOMER_OFFSET] >> 4) & 0x0f;
	u8 spec = efuse[RK3399_SPEC_OFFSET] & 0x1f;

	if (specification)
		*specification = spec;
	if (customer_demand)
		*customer_demand = customer;

	/* Keep this in sync with Rockchip's rk3399_get_soc_info(). */
	if (spec == 0x0b)
		return 0;
	if (spec == 0x01)
		return customer == 0 ? 0 : 1;
	if (spec == 0x10)
		return 1;

	return -EINVAL;
}

static int rk3399_fdt_get_opp_table(const void *blob,
				    const struct rk3399_opp_table_desc *desc)
{
	int node;

	node = fdt_path_offset(blob, desc->device_path);
	if (node < 0)
		return node;

	return fdtdec_lookup_phandle(blob, node, "operating-points-v2");
}

static int rk3399_fdt_remove_legacy_op1_nodes(void *blob, int table)
{
	char name[] = "opp00";
	int i, node, ret;

	/*
	 * Old 4B+ DTBs merge rk3399-op1-opp.dtsi into the standard table.
	 * Its nodes are named opp00..opp08, while standard nodes use
	 * opp-<rate>.
	 */
	for (i = 0; i <= 8; i++) {
		name[3] = '0' + i / 10;
		name[4] = '0' + i % 10;
		node = fdt_subnode_offset(blob, table, name);
		if (node == -FDT_ERR_NOTFOUND)
			continue;
		if (node < 0)
			return node;
		ret = fdt_del_node(blob, node);
		if (ret)
			return ret;
	}

	return 0;
}

static int rk3399_fdt_upsert_opp_entries(void *blob, int table,
					 const struct rk3399_opp_entry *opps,
					 int count)
{
	fdt32_t microvolt[3];
	int i, node, ret;

	for (i = 0; i < count; i++) {
		node = fdt_subnode_offset(blob, table, opps[i].name);
		if (node == -FDT_ERR_NOTFOUND)
			node = fdt_add_subnode(blob, table, opps[i].name);
		if (node < 0)
			return node;
		ret = fdt_setprop_u64(blob, node, "opp-hz", opps[i].rate);
		if (ret)
			return ret;
		if (opps[i].max_microvolt) {
			microvolt[0] = cpu_to_fdt32(opps[i].microvolt);
			microvolt[1] = microvolt[0];
			microvolt[2] = cpu_to_fdt32(opps[i].max_microvolt);
			ret = fdt_setprop(blob, node, "opp-microvolt",
					  microvolt, sizeof(microvolt));
		} else {
			ret = fdt_setprop_u32(blob, node, "opp-microvolt",
					      opps[i].microvolt);
		}
		if (ret)
			return ret;
		ret = fdt_delprop(blob, node, "status");
		if (ret && ret != -FDT_ERR_NOTFOUND)
			return ret;
		if (opps[i].latency) {
			ret = fdt_setprop_u32(blob, node, "clock-latency-ns",
					      40000);
			if (ret)
				return ret;
		}
		if (opps[i].suspend) {
			ret = fdt_setprop(blob, node, "opp-suspend", NULL, 0);
			if (ret)
				return ret;
		}
	}

	return 0;
}

static int rk3399_fdt_remove_opp_entries(void *blob, int table,
					 const struct rk3399_opp_entry *opps,
					 int count)
{
	int i, node, ret;

	for (i = 0; i < count; i++) {
		node = fdt_subnode_offset(blob, table, opps[i].name);
		if (node == -FDT_ERR_NOTFOUND)
			continue;
		if (node < 0)
			return node;
		ret = fdt_del_node(blob, node);
		if (ret)
			return ret;
	}

	return 0;
}

static int rk3399_fdt_get_max_opp_rate(
		const void *blob, const struct rk3399_opp_table_desc *desc,
		u64 *max_rate)
{
	const fdt32_t *rate_prop;
	const char *status;
	u64 rate, max = 0;
	int table, node, len;

	table = rk3399_fdt_get_opp_table(blob, desc);
	if (table < 0)
		return table;

	fdt_for_each_subnode(node, blob, table) {
		status = fdt_getprop(blob, node, "status", NULL);
		if (status && strcmp(status, "okay") && strcmp(status, "ok"))
			continue;

		rate_prop = fdt_getprop(blob, node, "opp-hz", &len);
		if (!rate_prop || len != sizeof(fdt64_t))
			continue;
		rate = fdt_read_number(rate_prop, 2);
		if (rate > max)
			max = rate;
	}

	if (!max)
		return -FDT_ERR_NOTFOUND;
	*max_rate = max;

	return 0;
}

static int rk3399_fdt_get_opp_rate_mask(
		const void *blob, const struct rk3399_opp_table_desc *desc,
		const u64 *rates, int count, u32 *mask)
{
	const fdt32_t *rate_prop;
	const char *status;
	u64 rate;
	int table, node, len, i;

	*mask = 0;
	table = rk3399_fdt_get_opp_table(blob, desc);
	if (table < 0)
		return table;

	fdt_for_each_subnode(node, blob, table) {
		status = fdt_getprop(blob, node, "status", NULL);
		if (status && strcmp(status, "okay") && strcmp(status, "ok"))
			continue;

		rate_prop = fdt_getprop(blob, node, "opp-hz", &len);
		if (!rate_prop || len != sizeof(fdt64_t))
			continue;

		rate = fdt_read_number(rate_prop, 2);
		for (i = 0; i < count; i++)
			if (rate == rates[i])
				*mask |= BIT(i);
	}

	return 0;
}

static int rk3399_fdt_get_opp_state(const void *blob,
				    struct rk3399_opp_state *state)
{
	int ret;

	ret = rk3399_fdt_get_max_opp_rate(
		blob, &rk3399_opp_tables[RK3399_OPP_LITTLE],
		&state->little_max);
	if (ret)
		return ret;
	ret = rk3399_fdt_get_max_opp_rate(
		blob, &rk3399_opp_tables[RK3399_OPP_BIG], &state->big_max);
	if (ret)
		return ret;

	return rk3399_fdt_get_opp_rate_mask(
		blob, &rk3399_opp_tables[RK3399_OPP_GPU],
		rk3399_gpu_signature_rates,
		ARRAY_SIZE(rk3399_gpu_signature_rates), &state->gpu_mask);
}

static bool rk3399_opp_state_matches(const struct rk3399_opp_state *state,
				     const struct rk3399_opp_profile *profile)
{
	if (state->little_max != profile->little_max ||
	    state->big_max != profile->big_max)
		return false;
	if ((state->gpu_mask & profile->gpu_required) != profile->gpu_required)
		return false;

	return !(state->gpu_mask & profile->gpu_forbidden);
}

static int rk3399_fdt_detect_opp_format(const void *blob,
					enum rk3399_opp_format *format)
{
	const struct rk3399_opp_table_desc *desc;
	bool standard_format = true;
	bool legacy_format = true;
	int i, table;

	for (i = 0; i < ARRAY_SIZE(rk3399_opp_tables); i++) {
		desc = &rk3399_opp_tables[i];

		table = rk3399_fdt_get_opp_table(blob, desc);
		if (table < 0)
			return table;
		if (!desc->standard_first)
			continue;
		if (fdt_subnode_offset(blob, table, desc->standard_first) < 0 ||
		    fdt_subnode_offset(blob, table, desc->standard_last) < 0)
			standard_format = false;
		if (fdt_subnode_offset(blob, table, "opp00") < 0 ||
		    fdt_subnode_offset(blob, table, desc->legacy_last) < 0)
			legacy_format = false;
	}

	if (standard_format && legacy_format)
		*format = RK3399_OPP_FORMAT_MERGED;
	else if (standard_format)
		*format = RK3399_OPP_FORMAT_STANDARD;
	else if (legacy_format)
		*format = RK3399_OPP_FORMAT_LEGACY;
	else
		*format = RK3399_OPP_FORMAT_UNKNOWN;

	return *format == RK3399_OPP_FORMAT_UNKNOWN ? -FDT_ERR_NOTFOUND : 0;
}

static int rk3399_fdt_apply_opp_table(void *blob,
				      const struct rk3399_opp_table_desc *desc,
				      enum rk3399_opp_format format,
				      bool high_freq)
{
	int table, ret;

	table = rk3399_fdt_get_opp_table(blob, desc);
	if (table < 0)
		return table;

	/* A pure OP1 CPU/GPU table is already in its native form. */
	if (format == RK3399_OPP_FORMAT_LEGACY && high_freq)
		return 0;

	ret = rk3399_fdt_remove_legacy_op1_nodes(blob, table);
	if (ret)
		return ret;

	/* Node edits move offsets, so resolve the table again. */
	table = rk3399_fdt_get_opp_table(blob, desc);
	if (table < 0)
		return table;

	if (format == RK3399_OPP_FORMAT_LEGACY) {
		return rk3399_fdt_upsert_opp_entries(blob, table,
						     desc->standard_opps,
						     desc->standard_count);
	}

	if (high_freq)
		return rk3399_fdt_upsert_opp_entries(blob, table,
						     desc->high_freq_delta,
						     desc->high_freq_count);

	return rk3399_fdt_remove_opp_entries(blob, table,
					     desc->high_freq_delta,
					     desc->high_freq_count);
}

static int rk3399_fdt_normalize_opps_inplace(void *blob, bool high_freq)
{
	enum rk3399_opp_format format;
	int i, ret;

	ret = rk3399_fdt_detect_opp_format(blob, &format);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(rk3399_opp_tables); i++) {
		ret = rk3399_fdt_apply_opp_table(blob, &rk3399_opp_tables[i],
						 format, high_freq);
		if (ret)
			return ret;
	}

	return 0;
}

static int rk3399_fdt_normalize_opps(void *blob, bool high_freq)
{
	const struct rk3399_opp_profile *profile;
	struct rk3399_opp_state state;
	int size = fdt_totalsize(blob);
	void *work;
	int ret;

	/* Keep the original kernel FDT intact unless every fixup succeeds. */
	work = malloc(size);
	if (!work)
		return -ENOMEM;
	memcpy(work, blob, size);
	profile = high_freq ? &rk3399_high_freq_profile :
			      &rk3399_standard_profile;

	ret = rk3399_fdt_normalize_opps_inplace(work, high_freq);
	if (!ret)
		ret = rk3399_fdt_get_opp_state(work, &state);
	if (!ret && !rk3399_opp_state_matches(&state, profile))
		ret = -EINVAL;
	if (!ret)
		memcpy(blob, work, size);
	free(work);

	return ret;
}

int rk_board_dm_fdt_fixup(void *blob)
{
	const struct rk3399_opp_profile *profile;
	struct rk3399_opp_state state;
	u8 efuse[RK3399_EFUSE_INFO_SIZE];
	bool high_freq;
	int bin, ret;

	ret = rk3399_read_efuse_info(efuse);
	if (ret) {
		printf("FDT: eFuse read failed (%d), OPP unchanged\n", ret);
		return 0;
	}

	bin = rk3399_efuse_bin(efuse, NULL, NULL);
	if (bin < 0) {
		printf("FDT: unknown bin (%d), keeping original OPP\n", bin);
		return 0;
	}

	high_freq = bin == 0;
	profile = high_freq ? &rk3399_high_freq_profile :
			      &rk3399_standard_profile;
	ret = rk3399_fdt_get_opp_state(blob, &state);
	if (ret) {
		printf("FDT: CPU OPP read failed (%d), unchanged\n", ret);
		return 0;
	}

	if (rk3399_opp_state_matches(&state, profile)) {
		if (high_freq)
			printf("FDT: high-frequency OPP already matched\n");
		else
			printf("FDT: standard OPP already matched\n");
		return 0;
	}

	printf("FDT: OPP mismatch: CPU %llu/%llu MHz\n",
	       state.little_max / 1000000, state.big_max / 1000000);
	printf("FDT: converting to %s\n",
	       high_freq ? "high-frequency" : "standard-frequency");
	ret = rk3399_fdt_normalize_opps(blob, high_freq);
	if (ret) {
		printf("FDT: OPP conversion failed (%d), unchanged\n", ret);
		return 0;
	}

	printf("FDT: converted OPP tables to %s\n",
	       high_freq ? "high-frequency" : "standard-frequency");
	return 0;
}

static int do_rk3399_socinfo(cmd_tbl_t *cmdtp, int flag, int argc,
			     char * const argv[])
{
	u8 efuse[RK3399_EFUSE_INFO_SIZE];
	u8 customer_demand, specification;
	int bin;
	int i, ret;

	ret = rk3399_read_efuse_info(efuse);
	if (ret < 0) {
		printf("Cannot read RK3399 eFuse: %d\n", ret);
		return CMD_RET_FAILURE;
	}

	printf("SoC family:       RK3399\n");
	printf("CPUID:            ");
	for (i = 0; i < RK3399_CPUID_SIZE; i++)
		printf("%02x", efuse[RK3399_CPUID_OFFSET + i]);
	printf("\n");
	printf("CPU big leakage:  0x%02x (%u)\n",
	       efuse[RK3399_CPU_BIG_LEAKAGE_OFFSET],
	       efuse[RK3399_CPU_BIG_LEAKAGE_OFFSET]);
	printf("GPU leakage:      0x%02x (%u)\n",
	       efuse[RK3399_GPU_LEAKAGE_OFFSET],
	       efuse[RK3399_GPU_LEAKAGE_OFFSET]);
	printf("Center leakage:   0x%02x (%u)\n",
	       efuse[RK3399_CENTER_LEAKAGE_OFFSET],
	       efuse[RK3399_CENTER_LEAKAGE_OFFSET]);
	printf("CPU little leak.: 0x%02x (%u)\n",
	       efuse[RK3399_CPU_LITTLE_LEAKAGE_OFFSET],
	       efuse[RK3399_CPU_LITTLE_LEAKAGE_OFFSET]);
	printf("Logic leakage:    0x%02x (%u)\n",
	       efuse[RK3399_LOGIC_LEAKAGE_OFFSET],
	       efuse[RK3399_LOGIC_LEAKAGE_OFFSET]);
	printf("Wafer info:       0x%02x (%u)\n",
	       efuse[RK3399_WAFER_INFO_OFFSET],
	       efuse[RK3399_WAFER_INFO_OFFSET]);

	bin = rk3399_efuse_bin(efuse, &specification, &customer_demand);
	printf("Specification:    0x%02x\n", specification);
	printf("Customer demand:  0x%02x\n", customer_demand);

	if (bin == 0) {
		printf("DVFS bin:         0 (A72 2016, A53 1512 MHz)\n");
		printf("Variant class:    high-frequency / OP1-compatible\n");
	} else if (bin == 1) {
		printf("DVFS bin:         1 (A72 1800, A53 1416 MHz)\n");
		printf("Variant class:    standard RK3399\n");
	} else {
		printf("DVFS bin:         unknown\n");
		printf("Variant class:    unknown\n");
	}
	printf("RK3399-T:         not encoded by this bin; "
	       "select by board/SKU\n");

	if (argc == 2 && !strcmp(argv[1], "raw")) {
		printf("Non-secure eFuse bytes 0x00..0x22:\n");
		print_buffer(0, efuse, 1, sizeof(efuse), 16);
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	socinfo, 2, 1, do_rk3399_socinfo,
	"show RK3399 non-secure eFuse information",
	"[raw]\n"
	"    - show CPUID, leakage, wafer and DVFS bin; raw dumps bytes 0x00..0x22"
);
#endif
