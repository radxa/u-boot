/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <dm.h>
#include <fdt_support.h>
#include <fdtdec.h>
#include <spl.h>
#include <wait_bit.h>
#include <dm/uclass-id.h>
#include <asm/io.h>
#include <asm/arch-rockchip/cpu.h>
#include <asm/arch-rockchip/boot_mode.h>
#include <asm/arch-rockchip/bootrom.h>
#include <asm/arch-rockchip/hardware.h>
#include <asm/arch-rockchip/grf_rv1103b.h>
#include <asm/arch-rockchip/ioc_rv1103b.h>

DECLARE_GLOBAL_DATA_PTR;

const char * const boot_devices[BROM_LAST_BOOTSOURCE + 1] = {
	[BROM_BOOTSOURCE_EMMC] = "/mmc@20d30000",
	[BROM_BOOTSOURCE_SPINOR] = "/spi@20d40000/flash@1",
	[BROM_BOOTSOURCE_SPINAND] = "/spi@20d40000/flash@0",
	[BROM_BOOTSOURCE_SD] = "/mmc@20d20000",
};

#define PERI_CRU_BASE			0x20000000
#define PERICRU_PERISOFTRST_CON09	0x0a24
#define PERICRU_PERISOFTRST_CON10	0x0a28

#define TOP_CRU_BASE			0x20060000
#define TOPCRU_CRU_GLBRST_ST		0x0c04
#define GLB_WDTn_RST_ST			GENMASK(9, 6)
#define WDT_SYS_RST_ST			GENMASK(15, 10)
#define PMU0_CRU_BASE			0x20070000
#define PMUCRU_PMUSOFTRST_CON02		0x0a08

#define GRF_SYS_BASE			0x20150000
#define GRF_SYS_MACPHY_CON0		0x00b0
#define GRF_SYS_HPMCU_CACHE_MISC	0x0214

#define GPIO0_IOC_BASE			0x201B0000
#define GPIO0A_IOMUX_SEL_H		0x04
#define GPIO0_BASE			0x20520000
#define GPIO_SWPORT_DR_L		0x00
#define GPIO_SWPORT_DDR_L		0x08

#define GPIO1_IOC_BASE			0x20170000
#define GPIO1A_IOMUX_SEL_0		0x20
#define GPIO1A_IOMUX_SEL_1_0		0x24
#define GPIO1A_IOMUX_SEL_1_1		0x10024
#define GPIO1B_IOMUX_SEL_0		0x10028
#define GPIO1B_IOMUX_SEL_1		0x1002c
#define GPIO1_IOC_GPIO1A_PULL_0		0x210
#define GPIO1_IOC_GPIO1A_PULL_1		0x10210
#define GPIO1_IOC_GPIO1B_PULL		0x10214
#define GPIO1_IOC_JTAG_M2_CON		0x10810

#define GPIO2_IOC_BASE			0x20840000
#define GPIO2A_IOMUX_SEL_1_1		0x44

#define SGRF_SYS_BASE			0x20250000
#define SGRF_SYS_SOC_CON2		0x0008
#define SGRF_SYS_SOC_CON3		0x000c
#define SGRF_SYS_OTP_CON		0x0018
#define FIREWALL_CON0			0x0020
#define FIREWALL_CON1			0x0024
#define FIREWALL_CON2			0x0028
#define FIREWALL_CON3			0x002c
#define FIREWALL_CON4			0x0030
#define FIREWALL_CON5			0x0034
#define FIREWALL_CON7			0x003c
#define SGRF_SYS_HPMCU_BOOT_DDR		0x0080

#define SGRF_PMU_BASE			0x20260000
#define SGRF_PMU_SOC_CON0		0x0000
#define SGRF_PMU_PMUMCU_BOOT_ADDR	0x0020

#define SYS_GRF_BASE			0x20150000
#define GRF_SYS_PERI_CON2		0x08
#define GRF_SYS_USBPHY_CON0		0x50

#define TOP_CRU_BASE			0x20060000
#define TOPCRU_CRU_GLB_RST_CON 		0xc10

#define USBPHY_APB_BASE			0x20e10000
#define USBPHY_FSLS_DIFF_RECEIVER	0x0100

void board_debug_uart_init(void)
{
	/* No need to change uart */
}

void board_set_iomux(enum uclass_id uclass, int devnum, int routing)
{
	switch (uclass) {
	case UCLASS_MMC:
		if (devnum == 0) {
			writel(0xffff1111, GPIO1_IOC_BASE + GPIO1A_IOMUX_SEL_0);
			writel(0x00ff0011, GPIO1_IOC_BASE + GPIO1A_IOMUX_SEL_1_0);
		} else if (devnum == 1) {
#if CONFIG_SPL_BUILD
			/* set SDMMC D0-3/CMD/CLK to gpio and pull down */
			writel(0xf0000000, GPIO1_IOC_BASE + GPIO1A_IOMUX_SEL_1_1);
			writel(0xffff0000, GPIO1_IOC_BASE + GPIO1B_IOMUX_SEL_0);
			writel(0x000f0000, GPIO1_IOC_BASE + GPIO1B_IOMUX_SEL_1);
			writel(0xc0008000, GPIO1_IOC_BASE + GPIO1_IOC_GPIO1A_PULL_1);
			writel(0x03ff02AA, GPIO1_IOC_BASE + GPIO1_IOC_GPIO1B_PULL);

			/* SDMMC PWREN GPIO0A4 power down and power up */
			writel(0x00100010, GPIO0_BASE + GPIO_SWPORT_DR_L);
			writel(0x00100010, GPIO0_BASE + GPIO_SWPORT_DDR_L);
			mdelay(50);
			writel(0x00100000, GPIO0_BASE + GPIO_SWPORT_DR_L);
#endif
			/* set SDMMC D0-3/CMD/CLK and pull up */
			writel(0xff001100, GPIO1_IOC_BASE + GPIO1A_IOMUX_SEL_1_1);
			writel(0xffff1111, GPIO1_IOC_BASE + GPIO1B_IOMUX_SEL_0);
			writel(0x000f0001, GPIO1_IOC_BASE + GPIO1B_IOMUX_SEL_1);
			writel(0xc0004000, GPIO1_IOC_BASE + GPIO1_IOC_GPIO1A_PULL_1);
			writel(0x03ff0155, GPIO1_IOC_BASE + GPIO1_IOC_GPIO1B_PULL);
		}
		break;
	case UCLASS_MTD:
		if (routing == 0) {
			/* FSPI0 M0 */
			writel(0xffff2222, GPIO1_IOC_BASE + GPIO1A_IOMUX_SEL_0);
			writel(0x00ff0022, GPIO1_IOC_BASE + GPIO1A_IOMUX_SEL_1_0);
		}
		break;
	default:
		printf("Bootdev 0x%x is not support\n", uclass);
	}
}

#ifdef CONFIG_SPL_BUILD
void rockchip_stimer_init(void)
{
	/* If Timer already enabled, don't re-init it */
	u32 reg = readl(CONFIG_ROCKCHIP_STIMER_BASE + 0x4);
	if (reg & 0x1)
		return;
	writel(0x00010000, CONFIG_ROCKCHIP_STIMER_BASE + 0x4);

	asm volatile("mcr p15, 0, %0, c14, c0, 0"
		     : : "r"(CONFIG_COUNTER_FREQUENCY));
	writel(0xffffffff, CONFIG_ROCKCHIP_STIMER_BASE + 0x14);
	writel(0xffffffff, CONFIG_ROCKCHIP_STIMER_BASE + 0x18);
	writel(0x00010001, CONFIG_ROCKCHIP_STIMER_BASE + 0x4);
}

int spl_fit_standalone_release(char *id, uintptr_t entry_point)
{
	if (!strcmp(id, "mcu0")) {
		/* set the hpmcu boot address */
		writel(entry_point, SGRF_SYS_BASE + SGRF_SYS_HPMCU_BOOT_DDR);
	} else if (!strcmp(id, "mcu1")) {
		/* reset lpmcu */
		writel(0x000f000f, PMU0_CRU_BASE + PMUCRU_PMUSOFTRST_CON02);
		/* set the lpmcu boot address */
		writel(entry_point, SGRF_PMU_BASE + SGRF_PMU_PMUMCU_BOOT_ADDR);
		writel(0x00800000, SGRF_PMU_BASE + SGRF_PMU_SOC_CON0);
		/* release lpmcu */
		writel(0x000f0000, PMU0_CRU_BASE + PMUCRU_PMUSOFTRST_CON02);
	}

	return 0;
}

void rk_meta_process(void)
{
	/* trigger software irq to hpmcu that means meta was ready */
	writel(0x00080008, GRF_SYS_BASE + GRF_SYS_HPMCU_CACHE_MISC);
}

void spl_rk_board_prepare_for_jump(struct spl_image_info *spl_image)
{
	/* reset hpmcu released by maskrom if next stage is U-Boot. */
	if (spl_image->next_stage == SPL_NEXT_STAGE_UBOOT)
		writel(0xf000f0, PERI_CRU_BASE + PERICRU_PERISOFTRST_CON10);
}
#endif

#ifndef CONFIG_TPL_BUILD
int arch_cpu_init(void)
{
#if defined(CONFIG_SPL_BUILD) || defined(CONFIG_SUPPORT_USBPLUG)
	u32 cru_glbrst_st = readl(TOP_CRU_BASE + TOPCRU_CRU_GLBRST_ST);
	/* write BOOT_WATCHDOG to boot mode register, if reset by WDT */
	if (cru_glbrst_st & GLB_WDTn_RST_ST) {
		/*
		 * Don't change boot mode to BOOT_WATCHDOG in order to keep different from
		 * "boot mode: watchdog" if WDT reset was caused by Linux kernel panic.
		 */
		if (readl(CONFIG_ROCKCHIP_BOOT_MODE_REG) != BOOT_PANIC)
			writel(BOOT_WATCHDOG, CONFIG_ROCKCHIP_BOOT_MODE_REG);
		/* clear flag if reset by WDT trigger */
		writel((cru_glbrst_st & ~WDT_SYS_RST_ST), TOP_CRU_BASE + TOPCRU_CRU_GLBRST_ST);
	}

	/* Set all devices to Non-secure */
	writel(0xffff0000, SGRF_SYS_BASE + FIREWALL_CON0);
	writel(0xffff0000, SGRF_SYS_BASE + FIREWALL_CON1);
	writel(0xffff0000, SGRF_SYS_BASE + FIREWALL_CON2);
	writel(0xffff0000, SGRF_SYS_BASE + FIREWALL_CON3);
	writel(0xffff0000, SGRF_SYS_BASE + FIREWALL_CON4);
	writel(0xffff0000, SGRF_SYS_BASE + FIREWALL_CON5);
	writel(0x01f00000, SGRF_SYS_BASE + FIREWALL_CON7);
	/* Set OTP to none secure mode */
	writel(0x00020000, SGRF_SYS_BASE + SGRF_SYS_OTP_CON);

#if defined(CONFIG_ROCKCHIP_EMMC_IOMUX)
	/* Set the emmc iomux */
	board_set_iomux(UCLASS_MMC, 0, 0);
#elif defined(CONFIG_ROCKCHIP_SFC_IOMUX)
	/* Set the fspi iomux */
	board_set_iomux(UCLASS_MTD, 0, 0);
#endif

#if defined(CONFIG_ROCKCHIP_SDMMC_IOMUX)
	/* Set the sdmmc iomux and power cycle */
	board_set_iomux(UCLASS_MMC, 1, 0);
	/* Enable force_jtag */
	writel(0x00010001, GPIO1_IOC_BASE + GPIO1_IOC_JTAG_M2_CON);
#endif

	/* no-secure WDT reset output will reset SoC system. */
	writel(0x00010001, SYS_GRF_BASE + GRF_SYS_PERI_CON2);
	/* secure WDT reset output will reset SoC system. */
	writel(0x00010001, SGRF_SYS_BASE + SGRF_SYS_SOC_CON2);
	/*
	 * enable tsadc trigger global reset and select first reset.
	 * enable global reset and wdt trigger pmu reset.
	 * select first reset trigger pmu reset.
	 */
	writel(0x0000ffdf, TOP_CRU_BASE + TOPCRU_CRU_GLB_RST_CON);

	/*
	 * Set the USB2 PHY in suspend mode and turn off the
	 * USB2 PHY FS/LS differential receiver to save power:
	 * VCC1V8_USB : reduce 3.8 mA
	 * VDD_0V9 : reduce 4.4 mA
	 */
	writel(0x01ff01d1, SYS_GRF_BASE + GRF_SYS_USBPHY_CON0);
	writel(0x00000000, USBPHY_APB_BASE + USBPHY_FSLS_DIFF_RECEIVER);

#ifdef CONFIG_SPI_FLASH_AUTO_MERGE
	/* gpio1a5/gpio2a6 cs-gpio */
	writel(0x00F00000, GPIO1_IOC_BASE + GPIO1A_IOMUX_SEL_1_0);
	writel(0x0F000000, GPIO2_IOC_BASE + GPIO2A_IOMUX_SEL_1_1);
#endif
#endif

	return 0;
}
#endif

#ifdef CONFIG_ROCKCHIP_IMAGE_TINY
int rk_board_scan_bootdev(char **devtype, char **devnum)
{
	if (!run_command("blk dev mmc 1", 0) &&
	    !run_command("rkimgtest mmc 1", 0)) {
		*devtype = "mmc";
		*devnum = "1";
	} else {
		run_command("blk dev mtd 2", 0);
		*devtype = "mtd";
		*devnum = "2";
	}

	return 0;
}
#endif

#define GMAC_NODE_FDT_PATH			"/ethernet@20800000"
#define GMAC_PHY_FDT_PATH			"/ethernet@20800000/mdio/ethernet-phy"
#define OTP_MACPHY_TXLEVEL_FDT_PATH		"/otp/macphy-txlevel"
#define GMAC_COMPAT_NEW				"rockchip,rv1106bs-gmac\0snps,dwmac-4.20a"
#define GMAC_PHY_COMPAT_NEW			"ethernet-phy-id0680.8101\0ethernet-phy-ieee802.3-c22"
#define	PHY_ADDR				2
#define	PAGE_SWITCH				0x1f
#define	DISABLE_APS_REG				0x12
#define	DISABLE_APS_VAL				0x4824
#define	PHYAFE_PDCW_REG				0x1c
#define	PHYAFE_PDCW_VAL				0x8880
#define	PD_ANALOG_REG				0x0
#define PD_ANALOG_VAL				0x3900

#define GMAC_BASE				0x20800000
#define MDIO_ADDRESS				0x200
#define MDIO_DATA				0x204

#define EQOS_MAC_MDIO_ADDRESS_PA_SHIFT		21
#define EQOS_MAC_MDIO_ADDRESS_RDA_SHIFT		16
#define EQOS_MAC_MDIO_ADDRESS_CR_SHIFT		8
#define EQOS_MAC_MDIO_ADDRESS_SKAP		BIT(4)
#define EQOS_MAC_MDIO_ADDRESS_GOC_SHIFT		2
#define EQOS_MAC_MDIO_ADDRESS_GOC_READ		3
#define EQOS_MAC_MDIO_ADDRESS_GOC_WRITE		1
#define EQOS_MAC_MDIO_ADDRESS_C45E		BIT(1)
#define EQOS_MAC_MDIO_ADDRESS_GB		BIT(0)

#define EQOS_MAC_MDIO_DATA_GD_MASK		0xffff

#define EQOS_MAC_MDIO_ADDRESS_CR_100_150	1
#define EQOS_MAC_MDIO_ADDRESS_CR_20_35		2
#define EQOS_MAC_MDIO_ADDRESS_CR_250_300	5

static int gmac_mdio_wait_idle(void)
{
	u32 mdio_address = GMAC_BASE + MDIO_ADDRESS;

	return wait_for_bit_le32((u32 *)mdio_address,
				 EQOS_MAC_MDIO_ADDRESS_GB, false,
				 1000000, true);
}

static int gmac_mdio_write(int mdio_addr, int mdio_reg, u16 mdio_val)
{
	u32 val;
	int ret;

	ret = gmac_mdio_wait_idle();
	if (ret) {
		pr_err("MDIO not idle at entry, ret: %d", ret);
		return ret;
	}

	writel(mdio_val, GMAC_BASE + MDIO_DATA);

	val = readl(GMAC_BASE + MDIO_ADDRESS);
	val &= EQOS_MAC_MDIO_ADDRESS_SKAP |
		EQOS_MAC_MDIO_ADDRESS_C45E;
	val |= (mdio_addr << EQOS_MAC_MDIO_ADDRESS_PA_SHIFT) |
		(mdio_reg << EQOS_MAC_MDIO_ADDRESS_RDA_SHIFT) |
		(EQOS_MAC_MDIO_ADDRESS_CR_250_300 <<
		 EQOS_MAC_MDIO_ADDRESS_CR_SHIFT) |
		(EQOS_MAC_MDIO_ADDRESS_GOC_WRITE <<
		 EQOS_MAC_MDIO_ADDRESS_GOC_SHIFT) |
		EQOS_MAC_MDIO_ADDRESS_GB;
	writel(val, GMAC_BASE + MDIO_ADDRESS);

	ret = gmac_mdio_wait_idle();
	if (ret) {
		pr_err("MDIO read didn't complete, ret: %d", ret);
		return ret;
	}

	return 0;
}

static int rk_board_fdt_pwrdn_gmac(const void *blob)
{
	void *fdt = (void *)gd->fdt_blob;
	int gmac_node;

	/* Turn off GMAC FEPHY to reduce chip power consumption at uboot level,
	 * if the gmac node is disabled at kernel dtb. RV1106/1103 has the
	 * internal gmac phy, u-boot.dtb defines and enables the gmac node
	 * by default, so even if the gmac node of the kernel dts is disabled,
	 * U-Boot will enable and initialize the gmac phy. So it is not okay
	 * to turn off gmac phy by default in arch_cpu_init(), need to turn off
	 * gmac phy in the current function.
	 */
	gmac_node = fdt_path_offset(gd->fdt_blob, GMAC_NODE_FDT_PATH);
	if (fdt_stringlist_search(fdt, gmac_node, "status", "disabled") >= 0) {
		writel(0x08000800, PERI_CRU_BASE + PERICRU_PERISOFTRST_CON09);
		writel(0xffff0b40, GRF_SYS_BASE + GRF_SYS_MACPHY_CON0);
		udelay(20);
		writel(0x08000000, PERI_CRU_BASE + PERICRU_PERISOFTRST_CON09);
		udelay(100);

		/* switch to page 1 */
		gmac_mdio_write(PHY_ADDR, PAGE_SWITCH, 0x0100);
		gmac_mdio_write(PHY_ADDR, DISABLE_APS_REG,
				DISABLE_APS_VAL);
		/* switch to pae 6 */
		gmac_mdio_write(PHY_ADDR, PAGE_SWITCH, 0x0600);
		gmac_mdio_write(PHY_ADDR, PHYAFE_PDCW_REG,
				PHYAFE_PDCW_VAL);
		/* switch to page 0 */
		gmac_mdio_write(PHY_ADDR, PAGE_SWITCH, 0x0000);
		gmac_mdio_write(PHY_ADDR, PD_ANALOG_REG,
				PD_ANALOG_VAL);
		udelay(20);
		writel(0x00020002, GRF_SYS_BASE + GRF_SYS_MACPHY_CON0);
	}

	return 0;
}

/* Max nvmem cells of a node to remap */
#define NVMEM_CELLS_MAX		3

/*
 * Rewrite the nvmem-cells of the node at path with the phandles of the
 * cells at the given paths, up to NVMEM_CELLS_MAX cells are supported.
 * The property is rewritten in place as its length keeps unchanged.
 */
static int fdt_set_nvmem_cells(const void *blob, const char *path,
			       const char *cells[], int count)
{
	fdt32_t phandles[NVMEM_CELLS_MAX];
	int i, phandle;

	if (count > NVMEM_CELLS_MAX)
		return -EINVAL;

	for (i = 0; i < count; i++) {
		phandle = fdt_get_phandle_by_path(blob, cells[i]);
		if (phandle < 0) {
			printf("Failed to get phandle by path: %s, fixup %s\n", cells[i], path);
			return phandle;
		}

		phandles[i] = cpu_to_fdt32(phandle);
	}

	return fdt_find_and_setprop((void *)blob, path, "nvmem-cells",
				    phandles, count * sizeof(phandles[0]), 0);
}

/*
 * Drop the first entry of a fixed size entries property, nothing to do
 * if the property does not exist or it is the only entry. For string
 * list properties pass strlen(first string) + 1 as the entry size.
 * Note that fdt_setprop() may move the blob, so copy via a local buffer.
 */
static int fdt_del_prop_first_entry(const void *blob, int node,
				    const char *prop, int entry_size)
{
	char buf[64];
	const void *val;
	int len, ret;

	val = fdt_getprop(blob, node, prop, &len);
	if (!val || len <= entry_size || len > (int)sizeof(buf))
		return 0;

	memcpy(buf, val + entry_size, len - entry_size);
	ret = fdt_setprop((void *)blob, node, prop, buf, len - entry_size);
	if (ret)
		printf("Failed to fixup %s, ret: %d\n", prop, ret);

	return ret;
}

static int fdt_fixup_cpuinfo(const void *blob)
{
	const char *cells[] = {
		"/efuse@20820000/id@a",
		"/efuse@20820000/cpu-version@8",
		"/efuse@20820000/cpu-code@2",
	};
	int ret;

	/* Replace cpu id, cpu version and cpu code */
	ret = fdt_set_nvmem_cells(blob, "/cpuinfo", cells, ARRAY_SIZE(cells));
	if (ret)
		printf("Failed to fixup /cpuinfo nvmem-cells, ret: %d\n", ret);

	return ret;
}

static int fdt_fixup_cpu_opp_table(const void *blob)
{
	int cpu_node, len, phandle;
	u32 *pp;

	/* Replace opp table */
	phandle = fdt_get_phandle_by_path(blob, "/cpu0-opp-table-s");
	if (phandle < 0) {
		printf("Failed to get phandle by path: /cpu0-opp-table-s\n");
		return phandle;
	}

	/* rv1103b is single core, patch the cpu@0 node only */
	cpu_node = fdt_path_offset(blob, "/cpus/cpu@0");
	if (cpu_node < 0) {
		printf("Failed to get /cpus/cpu@0 node\n");
		return -EINVAL;
	}

	pp = (u32 *)fdt_getprop(blob, cpu_node, "operating-points-v2", &len);
	if (!pp) {
		printf("Failed to get cpu operating-points-v2 prop\n");
		return -EINVAL;
	}
	pp[0] = cpu_to_fdt32(phandle);

	return 0;
}

static int fdt_fixup_npu_opp_table(const void *blob)
{
	int npu_node;
	int phandle;
	int len;
	u32 *pp;

	phandle = fdt_get_phandle_by_path(blob, "/npu-opp-table-s");
	if (phandle < 0) {
		printf("Failed to get phandle by path: /npu-opp-table-s\n");
		return phandle;
	}

	npu_node = fdt_path_offset(blob, "/npu");
	if (npu_node < 0) {
		printf("Failed to get npu node\n");
		return -EINVAL;
	}

	pp = (u32 *)fdt_getprop(blob, npu_node, "operating-points-v2", &len);
	if (!pp)
		return 0;
	pp[0] = cpu_to_fdt32(phandle);

	return 0;
}

static int fdt_fixup_tsadc_trim(const void *blob)
{
	const char *cells[] = {
		"/efuse@20820000/cpu-tsadc-trim-l@1c",
		"/efuse@20820000/cpu-tsadc-trim-h@1d",
	};
	int ret;

	/* Replace tsadc trim l and tsadc trim h */
	ret = fdt_set_nvmem_cells(blob, "/tsadc", cells, ARRAY_SIZE(cells));
	if (ret)
		printf("Failed to fixup /tsadc nvmem-cells, ret: %d\n", ret);

	return ret;
}

static int fdt_fixup_pvtpll(const void *blob)
{
	const char *cpu_cells[] = {
		"/efuse@20820000/cpu-opp-info@3c"
	};
	const char *npu_cells[] = {
		"/efuse@20820000/npu-opp-info@48"
	};
	int ret;

	/* Replace cpu opp info */
	ret = fdt_set_nvmem_cells(blob, "/clocks/pvtpll-core@20460000",
				  cpu_cells, ARRAY_SIZE(cpu_cells));
	if (ret) {
		printf("Failed to fixup pvtpll-core nvmem-cells, ret: %d\n", ret);
		return ret;
	}

	/* Replace npu opp info */
	ret = fdt_set_nvmem_cells(blob, "/clocks/pvtpll-npu@20f80000",
				  npu_cells, ARRAY_SIZE(npu_cells));
	if (ret)
		printf("Failed to fixup pvtpll-npu nvmem-cells, ret: %d\n", ret);

	return ret;
}

/*
 * The rv1106bs gmac and its internal phy differ from rv1103b, patch the
 * kernel dtb compatible strings, and drop the first nvmem cell and the
 * first reset of the rmii phy, they are not present on rv1106bs (the
 * nvmem cell references the /otp node which is deleted on rv1106bs).
 */
static int fdt_fixup_gmac(const void *blob)
{
	int phy, ret;

	/* rewrite the compatibles, keep the fallback strings */
	do_fixup_by_path((void *)blob, GMAC_NODE_FDT_PATH, "compatible",
			 GMAC_COMPAT_NEW, sizeof(GMAC_COMPAT_NEW), 0);
	do_fixup_by_path((void *)blob, GMAC_PHY_FDT_PATH, "compatible",
			 GMAC_PHY_COMPAT_NEW, sizeof(GMAC_PHY_COMPAT_NEW), 0);

	/* relocate the phy node, the compatible resize above moved it */
	phy = fdt_path_offset(blob, GMAC_PHY_FDT_PATH);
	if (phy < 0)
		return 0;

	/* drop the first nvmem cell, it points to the deleted /otp node */
	ret = fdt_del_prop_first_entry(blob, phy, "nvmem-cells", sizeof(u32));
	if (!ret)
		ret = fdt_del_prop_first_entry(blob, phy, "nvmem-cell-names",
					       strlen("txlevel") + 1);

	/* drop the first reset, rv1106bs only has the macphy_s reset */
	if (!ret)
		ret = fdt_del_prop_first_entry(blob, phy, "resets", 2 * sizeof(u32));

	return ret;
}

/* Fix kernel dtb */
int rk_board_fdt_fixup(const void *blob)
{
	rk_board_fdt_pwrdn_gmac(blob);

	if (soc_is_rv1106bs()) {
		fdt_fixup_cpuinfo(blob);
		fdt_fixup_cpu_opp_table(blob);
		fdt_fixup_npu_opp_table(blob);
		fdt_fixup_tsadc_trim(blob);
		fdt_fixup_pvtpll(blob);
		fdt_fixup_gmac(blob);
		fdt_del_node_by_path(blob, "/otp");
	} else {
		fdt_del_node_by_path(blob, "/efuse");
	}

	return 0;
}

/* Fix kernel dtb that used in U-Boot rear stage */
int rk_board_early_fdt_fixup(const void *blob)
{
	if (soc_is_rv1106bs()) {
		fdt_fixup_gmac(blob);
		fdt_del_node_by_path(blob, "/otp");
		fdt_del_node_by_path(blob, "/secure-otp@20790000");
	} else {
		fdt_del_node_by_path(blob, "/efuse");
		fdt_del_node_by_path(blob, "/secure-efuse@20790000");
	}

	return 0;
}

/* Fix dtb that used in SPL and U-Boot front stage */
int fdtdec_board_setup(const void *fdt_blob)
{
	if (soc_is_rv1106bs()) {
		fdt_del_node_by_path(fdt_blob, "/otp");
		fdt_del_node_by_path(fdt_blob, "/secure-otp@20790000");
	} else {
		fdt_del_node_by_path(fdt_blob, "/efuse");
		fdt_del_node_by_path(fdt_blob, "/secure-efuse@20790000");
	}

	return 0;
}

int fit_standalone_release(char *id, uintptr_t entry_point)
{
	if (!strcmp(id, "hpmcu")) {
		/* reset hpmcu */
		writel(0x00f000f0, PERI_CRU_BASE + PERICRU_PERISOFTRST_CON10);
		writel(0x00080008, SGRF_SYS_BASE + SGRF_SYS_SOC_CON2);
		/* set the hpmcu boot address */
		writel(entry_point, SGRF_SYS_BASE + SGRF_SYS_HPMCU_BOOT_DDR);
		writel(0x80000000, SGRF_SYS_BASE + SGRF_SYS_SOC_CON3);
		/* release hpmcu */
		writel(0x00f00000, PERI_CRU_BASE + PERICRU_PERISOFTRST_CON10);
	}

	return 0;
}

#if defined(CONFIG_ROCKCHIP_EMMC_IOMUX) && defined(CONFIG_ROCKCHIP_SFC_IOMUX)
#error FSPI and eMMC iomux is incompatible for rv1103b Soc. You should close one of them.
#endif
