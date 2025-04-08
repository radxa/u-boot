/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner PCIe controller driver
 *
 * Copyright (C) 2022 allwinner Co., Ltd.
 *
 * Author: songjundong <songjundong@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _PCIE_SUNXI_H
#define _PCIE_SUNXI_H

#include <asm/io.h>
#include <asm/gpio.h>
#include <clk.h>
#include <reset.h>
#include <generic-phy.h>

#define SUNXI_PCIE_MODULE_VERSION	"1.1.1"

#define SUNXI_PCIE_DBI_ADDR	SUNXI_PCIE_BASE
#define SUNXI_PCIE_CFG_ADDR	0x20000000
#define SUNXI_PCIE_IO_ADDR	0x21000000
#define SUNXI_PCIE_MEM_ADDR	0x22000000
#define SUNXI_PCIE_ROOT_ADDR	0x20000000

#define SUNXI_PCIE_DBI_SIZE	0x480000
#define SUNXI_PCIE_CFG_SIZE	0x01000000
#define SUNXI_PCIE_IO_SIZE	0x01000000
#define SUNXI_PCIE_MEM_SIZE	0x07000000
#define SUNXI_PCIE_ROOT_SIZE	0x01000000

#define SUNXI_CFG_RW_SIZE 0x04

#define PCIE_PORT_LINK_CONTROL			0x710
#define PORT_LINK_MODE_MASK			(0x3f << 16)
#define PORT_LINK_MODE_1_LANES			(0x1 << 16)
#define PORT_LINK_MODE_2_LANES			(0x3 << 16)
#define PORT_LINK_MODE_4_LANES			(0x7 << 16)
#define PORT_LINK_LPBK_ENABLE		        (0x1 << 2)

#define PCIE_LINK_WIDTH_SPEED_CONTROL		0x80C
#define PORT_LOGIC_SPEED_CHANGE			(0x1 << 17)
#define PORT_LOGIC_LINK_WIDTH_MASK		(0x1ff << 8)
#define PORT_LOGIC_LINK_WIDTH_1_LANES		(0x1 << 8)
#define PORT_LOGIC_LINK_WIDTH_2_LANES		(0x2 << 8)
#define PORT_LOGIC_LINK_WIDTH_4_LANES		(0x4 << 8)

#define PCIE_ATU_VIEWPORT			0x900
#define PCIE_ATU_REGION_INBOUND			(0x1 << 31)
#define PCIE_ATU_REGION_OUTBOUND		(0x0 << 31)
#define PCIE_ATU_REGION_INDEX2			(0x2 << 0)
#define PCIE_ATU_REGION_INDEX1			(0x1 << 0)
#define PCIE_ATU_REGION_INDEX0			(0x0 << 0)

#define PCIE_ATU_INDEX0				0x0
#define PCIE_ATU_INDEX1				0x1
#define PCIE_ATU_INDEX2				0x2
#define PCIE_ATU_INDEX3				0x3
#define PCIE_ATU_INDEX4				0x4
#define PCIE_ATU_INDEX5				0x5
#define PCIE_ATU_INDEX6				0x6
#define PCIE_ATU_INDEX7				0x7

#define PCIE_EP_REBAR_SIZE_32M			0x200

#define PCIE_ATU_CR1_OUTBOUND(reg)		(0x300000 + ((reg) * 0x200))
#define PCIE_ATU_TYPE_MEM			(0x0 << 0)
#define PCIE_ATU_TYPE_IO			(0x2 << 0)
#define PCIE_ATU_TYPE_CFG0			(0x4 << 0)
#define PCIE_ATU_TYPE_CFG1			(0x5 << 0)
#define PCIE_ATU_CR2_OUTBOUND(reg)		(0x300004 + ((reg) * 0x200))
#define PCIE_ATU_DMA_BYPASS			BIT(27)
#define PCIE_ATU_BAR_MODE_ENABLE		BIT(30)
#define PCIE_ATU_ENABLE				BIT(31)

#define PCIE_ATU_LOWER_BASE_OUTBOUND(reg)	(0x300008 + ((reg) * 0x200))
#define PCIE_ATU_UPPER_BASE_OUTBOUND(reg)	(0x30000c + ((reg) * 0x200))
#define PCIE_ATU_LIMIT_OUTBOUND(reg)		(0x300010 + ((reg) * 0x200))
#define PCIE_ATU_LOWER_TARGET_OUTBOUND(reg)	(0x300014 + ((reg) * 0x200))
#define PCIE_ATU_UPPER_TARGET_OUTBOUND(reg)	(0x300018 + ((reg) * 0x200))

#define PCIE_ATU_CR1_INBOUND(reg)		(0x300100 + ((reg) * 0x200))
#define PCIE_ATU_TYPE_MEM			(0x0 << 0)
#define PCIE_ATU_TYPE_IO			(0x2 << 0)
#define PCIE_ATU_TYPE_CFG0			(0x4 << 0)
#define PCIE_ATU_TYPE_CFG1			(0x5 << 0)
#define PCIE_ATU_FUNC_NUM(pf)			((pf) << 20)
#define PCIE_ATU_CR2_INBOUND(reg)		(0x300104 + ((reg) * 0x200))
#define PCIE_ATU_MATCH_MODE			BIT(30)
#define PCIE_ATU_FUNC_NUM_MATCH_EN		BIT(19)
#define PCIE_ATU_FUNC_NUM(pf)			((pf) << 20)

#define PCIE_ATU_LOWER_BASE_INBOUND(reg)	(0x300108 + ((reg) * 0x200))
#define PCIE_ATU_UPPER_BASE_INBOUND(reg)	(0x30010c + ((reg) * 0x200))
#define PCIE_ATU_LIMIT_INBOUND(reg)		(0x300110 + ((reg) * 0x200))
#define PCIE_ATU_LOWER_TARGET_INBOUND(reg)	(0x300114 + ((reg) * 0x200))
#define PCIE_ATU_UPPER_TARGET_INBOUND(reg)	(0x300118 + ((reg) * 0x200))

#define PCIE_ATU_BUS(x)				(((x) & 0xff) << 24)
#define PCIE_ATU_DEV(x)				(((x) & 0x1f) << 19)
#define PCIE_ATU_FUNC(x)			(((x) & 0x7) << 16)

#define PCIE_MISC_CONTROL_1_CFG			0x8bc
#define PCIE_TYPE1_CLASS_CODE_REV_ID_REG	0x08

#define PCIE_ADDRESS_ALIGNING			(~0x3)
#define PCIE_HIGH_16				16
#define PCIE_BAR_NUM				6
#define PCIE_MEM_FLAGS				0x4
#define PCIE_IO_FLAGS				0x1
#define PCIE_BAR_REG				0x4
#define PCIE_HIGH16_MASK			0xffff0000
#define PCIE_LOW16_MASK				0x0000ffff
#define PCIE_INTERRUPT_LINE_MASK		0xffff00ff
#define PCIE_INTERRUPT_LINE_ENABLE		0x00000100
#define	PCIE_PRIMARY_BUS_MASK			0xff000000
#define PCIE_PRIMARY_BUS_ENABLE			0x00010100
#define PCIE_MEMORY_MASK			0xfff00000

#define PCIE_CPU_BASE				0x20000000

#define PCIE_TYPE0_STATUS_COMMAND_REG		0x4

#define PCIE_DBI2_BASE				0x100000
#define DBI2_FUNC_OFFSET			0x10000
#define BAR_ENABLE				0x1

#define RESBAR_CAP_REG				0x4 /* from PCIe spec4.0 7.8.6  */
#define  RESBAR_SIZE_MASK			0xfffff0
#define RESBAR_CTL_REG				0x8
#define RESBAR_NEXT_BAR				0x8
#define SIZE_OF_1MB				20 /* 2^20 = 0x100000 */

#define PCIE_COMBO_PHY_BGR		0x04
#define PHY_ACLK_EN			BIT(17)
#define PHY_HCLK_EN			BIT(16)
#define PHY_TERSTN			BIT(1)
#define PHY_PW_UP_RSTN			BIT(0)
#define PCIE_COMBO_PHY_CTL		0x10
#define PHY_USE_SEL			BIT(31)	/* 0:PCIE; 1:USB3 */
#define	PHY_CLK_SEL			BIT(30) /* 0:internal clk; 1:exteral clk */
#define PHY_BIST_EN			BIT(16)
#define PHY_PIPE_SW			BIT(9)
#define PHY_PIPE_SEL			BIT(8)  /* 0:PIPE resetn ctrl by PCIE ctrl; 1:PIPE resetn ctrl by  */
#define PHY_PIPE_CLK_INVERT		BIT(4)
#define PHY_FPGA_SYS_RSTN		BIT(1)  /* for PFGA  */
#define PHY_RSTN			BIT(0)

#define NEXT_CAP_PTR_MASK		0xff00
#define CAP_ID_MASK			0x00ff

/* Error values that may be returned by PCI functions */
#define PCIBIOS_SUCCESSFUL		0x00
#define PCIBIOS_FUNC_NOT_SUPPORTED	0x81
#define PCIBIOS_BAD_VENDOR_ID		0x83
#define PCIBIOS_DEVICE_NOT_FOUND	0x86
#define PCIBIOS_BAD_REGISTER_NUMBER	0x87
#define PCIBIOS_SET_FAILED		0x88
#define PCIBIOS_BUFFER_TOO_SMALL	0x89

/*
 * Maximum number of MSI IRQs can be 256 per controller. But keep
 * it 32 as of now. Probably we will never need more than 32. If needed,
 * then increment it in multiple of 32.
 */
#define INT_PCI_MSI_NR			32
#define MAX_MSI_IRQS			256
#define MAX_MSI_IRQS_PER_CTRL		32
#define MAX_MSI_CTRLS			(MAX_MSI_IRQS / MAX_MSI_IRQS_PER_CTRL)
#define MSI_REG_CTRL_BLOCK_SIZE		12

/* #define MAX_MSI_IRQS			32 */
/* #define MAX_MSI_CTRLS		(MAX_MSI_IRQS / 32) */
#define PCIE_LINK_WIDTH_SPEED_CONTROL	0x80C
#define PORT_LOGIC_SPEED_CHANGE		(0x1 << 17)
#define LINK_CONTROL2_LINK_STATUS2	0xa0
/* Parameters for the waiting for link up routine */
#define LINK_WAIT_MAX_RETRIE		20
#define LINK_WAIT_USLEEP_MIN		90000
#define LINK_WAIT_USLEEP_MAX		100000
#define SPEED_CHANGE_USLEEP_MIN		100
#define SPEED_CHANGE_USLEEP_MAX		1000

#define PCIE_MSI_ADDR_LO		0x820
#define PCIE_MSI_ADDR_HI		0x824
#define PCIE_MSI_INTR_ENABLE(reg)	(0x828 + ((reg) * 0x0c))
/* #define PCIE_MSI_INTR_MASK(reg)	(0x82C + ((reg) * 0x0c)) */
/* #define PCIE_MSI_INTR_STATUS(reg)	(0x830 + ((reg) * 0x0c)) */
/* #define PCIE_MSI_INTR_ENABLE		0x828 */
#define PCIE_MSI_INTR_MASK		0x82C
#define PCIE_MSI_INTR_STATUS		0x830

#define PCIE_CTRL_MGMT_BASE		0x900000

#define PCIE_USER_DEFINED_REGISTER	0x400000
#define PCIE_VER			0x00
#define PCIE_ADDR_PAGE_CFG		0x04
#define PCIE_AWMISC_CTRL		0x200
#define PCIE_ARMISC_CTRL		0x220
#define PCIE_LTSSM_CTRL			0xc00
#define PCIE_LINK_TRAINING		BIT(0) /* 0:disable; 1:enable  */
#define DEVICE_TYPE_MASK		GENMASK(7, 4)
#define DEVICE_TYPE_RC			BIT(6)
#define PCIE_INT_ENABLE_CLR		0xE04  /* BIT(1):RDLH_LINK_MASK; BIT(0):SMLH_LINK_MASK  */
#define PCIE_LINK_STAT			0xE0C  /* BIT(1):RDLH_LINK;      BIT(0):SMLH_LINK  */
#define RDLH_LINK_UP			BIT(1)
#define SMLH_LINK_UP			BIT(0)
#define PCIE_LINK_INT_EN		(BIT(0) | BIT(1))

#define PCIE_PHY_CFG			0x800
#define SYS_CLK				0
#define PAD_CLK				1
#define PCIE_LINK_UP_MASK		(0x3<<16)

#define PCIE_RC_RP_ATS_BASE		0x400000

#define SUNXI_PCIE_BAR_CFG_CTRL_DISABLED		0x0
#define SUNXI_PCIE_BAR_CFG_CTRL_IO_32BITS		0x1
#define SUNXI_PCIE_BAR_CFG_CTRL_MEM_32BITS		0x4
#define SUNXI_PCIE_BAR_CFG_CTRL_PREFETCH_MEM_32BITS	0x5
#define SUNXI_PCIE_BAR_CFG_CTRL_MEM_64BITS		0x6
#define SUNXI_PCIE_BAR_CFG_CTRL_PREFETCH_MEM_64BITS	0x7

#define SUNXI_PCIE_EP_MSI_CTRL_REG			0x90
#define SUNXI_PCIE_EP_MSI_CTRL_MMC_OFFSET		17
#define SUNXI_PCIE_EP_MSI_CTRL_MMC_MASK			GENMASK(19, 17)
#define SUNXI_PCIE_EP_MSI_CTRL_MME_OFFSET		20
#define SUNXI_PCIE_EP_MSI_CTRL_MME_MASK			GENMASK(22, 20)
#define SUNXI_PCIE_EP_MSI_CTRL_ME			BIT(16)
#define SUNXI_PCIE_EP_MSI_CTRL_MASK_MSI_CAP		BIT(24)
#define SUNXI_PCIE_EP_DUMMY_IRQ_ADDR			0x1

#define PCIE_PHY_FUNC_CFG		(PCIE_CTRL_MGMT_BASE + 0x2c0)
#define PCIE_RC_BAR_CONF		(PCIE_CTRL_MGMT_BASE + 0x300)

//ECC
#define PCIE_RASDP_ERR_PROT_CTRL_OFF			0X1F0
#define PCIE_RASDP_ERR_INJ_CTRL_OFF			0X204
#define PCIE_RASDP_UNCORR_COUNTER_CTRL_OFF		0X1FC
#define PCIE_RASDP_UNCORR_COUNTER_REPORT_OFF		0X200
#define PCIE_RASDP_UNCORR_ERROR_LOCATION_OFF		0X20C
#define PCIE_RASDP_ERROR_MODR_CLEAR_OFF			0X214

#define PCIE_RASDP_CORR_COUNTER_CTRL_OFF		0X1F4
#define PCIE_RASDP_CORR_COUNTER_REPORT_OFF		0X1F8
#define PCIE_RASDP_CORR_ERROR_LOCATION_OFF		0X208

#define PCIE_SII_INT_MASK_RES2				0XE10
#define PCIE_SII_INT_RES2				0XE18
#define APP_PARITY_ERRS2_MASK				BIT(12)
#define APP_PARITY_ERRS1_MASK				BIT(11)
#define APP_PARITY_ERRS0_MASK				BIT(10)
#define SLV_RASDP_ERR_MODE_MASK				BIT(9)
#define MATR_RASDP_ERR_MODE_MASK			BIT(8)
#define RASDP_ERR_PENDING 				(BIT(8) | BIT(9) | BIT(10) | BIT(11) | BIT(12))
#define PCIE_SII_INT_RES2_ECC_MASK 			GENMASK(12, 8)

#define  PCI_EXP_LNKCTL2_TLS		0x000f
#define PCI_EXP_LNKCTL2_TLS_2_5GT	0x0001 /* Supported Speed 2.5GT/s */
#define PCI_EXP_LNKCTL2_TLS_5_0GT	0x0002 /* Supported Speed 5GT/s */
#define PCI_EXP_LNKCTL2_TLS_8_0GT	0x0003 /* Supported Speed 8GT/s */
#define PCI_EXP_LNKCTL2_TLS_16_0GT	0x0004 /* Supported Speed 16GT/s */
#define PCI_EXP_LNKCTL2_TLS_32_0GT	0x0005 /* Supported Speed 32GT/s */
#define PCI_EXP_LNKCTL2_HASD		0x0020 /* HW Autonomous Speed Disable */

#define PCI_EXP_LNKCAP		12	/* Link Capabilities */
#define PCI_EXP_LNKCAP_SLS	0x0000000f /* Supported Link Speeds */
#define PCI_EXP_LNKCAP_SLS_2_5GB 0x00000001 /* LNKCAP2 SLS Vector bit 0 */
#define PCI_EXP_LNKCAP_SLS_5_0GB 0x00000002 /* LNKCAP2 SLS Vector bit 1 */
#define PCI_EXP_LNKCAP_MLW	0x000003f0 /* Maximum Link Width */
#define PCI_EXP_LNKCAP_ASPMS	0x00000c00 /* ASPM Support */
#define PCI_EXP_LNKCAP_L0SEL	0x00007000 /* L0s Exit Latency */
#define PCI_EXP_LNKCAP_L1EL	0x00038000 /* L1 Exit Latency */
#define PCI_EXP_LNKCAP_CLKPM	0x00040000 /* Clock Power Management */
#define PCI_EXP_LNKCAP_SDERC	0x00080000 /* Surprise Down Error Reporting Capable */
#define PCI_EXP_LNKCAP_DLLLARC	0x00100000 /* Data Link Layer Link Active Reporting Capable */
#define PCI_EXP_LNKCAP_LBNC	0x00200000 /* Link Bandwidth Notification Capability */
#define PCI_EXP_LNKCAP_PN	0xff000000 /* Port Number */

#define  PCI_CAP_ID_AF		0x13	/* PCI Advanced Features */
#define  PCI_CAP_ID_MAX		PCI_CAP_ID_AF

#define PCI_EXP_LNKCTL2		48	/* Link Control 2 */
#define PCI_EXP_LNKSTA2		50	/* Link Status 2 */
#define PCI_EXP_SLTCAP2		52	/* Slot Capabilities 2 */
#define PCI_EXP_SLTCTL2		56	/* Slot Control 2 */
#define PCI_EXP_SLTSTA2		58	/* Slot Status 2 */



/* Resizable BARs */
#define PCI_REBAR_CAP		4	/* capability register */
#define PCI_REBAR_CAP_SIZES		0x00FFFFF0  /* supported BAR sizes */
#define PCI_REBAR_CTRL		8	/* control register */
#define PCI_REBAR_CTRL_BAR_IDX		0x00000007  /* BAR index */
#define PCI_REBAR_CTRL_NBAR_MASK	0x000000E0  /* # of resizable BARs */
#define PCI_REBAR_CTRL_NBAR_SHIFT	5	    /* shift for # of BARs */
#define PCI_REBAR_CTRL_BAR_SIZE	0x00001F00  /* BAR size */
#define PCI_REBAR_CTRL_BAR_SHIFT	8	    /* shift for BAR size */

#define  PCI_HEADER_TYPE_MASK		0x7f


#define PCI_STD_NUM_BARS	6	/* Number of standard BARs */
enum sunxi_pcie_device_mode {
	SUNXI_PCIE_EP_TYPE,
	SUNXI_PCIE_RC_TYPE,
};

/* See matching string table in pci_speed_string() */
enum pci_bus_speed {
	PCI_SPEED_33MHz			= 0x00,
	PCI_SPEED_66MHz			= 0x01,
	PCI_SPEED_66MHz_PCIX		= 0x02,
	PCI_SPEED_100MHz_PCIX		= 0x03,
	PCI_SPEED_133MHz_PCIX		= 0x04,
	PCI_SPEED_66MHz_PCIX_ECC	= 0x05,
	PCI_SPEED_100MHz_PCIX_ECC	= 0x06,
	PCI_SPEED_133MHz_PCIX_ECC	= 0x07,
	PCI_SPEED_66MHz_PCIX_266	= 0x09,
	PCI_SPEED_100MHz_PCIX_266	= 0x0a,
	PCI_SPEED_133MHz_PCIX_266	= 0x0b,
	AGP_UNKNOWN			= 0x0c,
	AGP_1X				= 0x0d,
	AGP_2X				= 0x0e,
	AGP_4X				= 0x0f,
	AGP_8X				= 0x10,
	PCI_SPEED_66MHz_PCIX_533	= 0x11,
	PCI_SPEED_100MHz_PCIX_533	= 0x12,
	PCI_SPEED_133MHz_PCIX_533	= 0x13,
	PCIE_SPEED_2_5GT		= 0x14,
	PCIE_SPEED_5_0GT		= 0x15,
	PCIE_SPEED_8_0GT		= 0x16,
	PCIE_SPEED_16_0GT		= 0x17,
	PCIE_SPEED_32_0GT		= 0x18,
	PCI_SPEED_UNKNOWN		= 0xff,
};

struct sunxi_pcie_of_data {
	const struct sunxi_pcie_ep_ops *ops;
	enum sunxi_pcie_device_mode mode;
	u32 func_offset;
	bool cpu_pcie_addr_quirk;
	bool has_pcie_slv_clk;
	bool need_pcie_rst;
	bool pcie_slv_clk_400m;
	bool has_pcie_its_clk;
	bool has_pcie_ecc;
};

struct sunxi_pcie_ep_func {
	struct list_head list;
	u8 func_no;
	u8 msi_cap;
	u8 msix_cap;
};

struct sunxi_pcie_ep {
	struct pci_epc		*epc;
	struct list_head	func_list;
	const struct sunxi_pcie_ep_ops *ops;
	phys_addr_t		phys_base;
	size_t			addr_size;
	size_t			page_size;
	u8			bar_to_atu[PCI_STD_NUM_BARS];
	phys_addr_t		*outbound_addr;
	u32			num_ib_windows;
	u32			num_ob_windows;
	unsigned long		*ib_window_map;
	unsigned long		*ob_window_map;
	u8			max_functions;
	void __iomem		*msi_mem;
	phys_addr_t		msi_mem_phys;
	struct pci_bar	*epf_bar[PCI_STD_NUM_BARS];
};

struct sunxi_pcie_ep_ops {
	void	(*ep_init)(struct sunxi_pcie_ep *ep);
	// int	(*raise_irq)(struct sunxi_pcie_ep *ep, u8 func_no,
	// 		     enum pci_epc_irq_type type, u16 interrupt_num);
	const struct pci_epc_features *(*get_features)(struct sunxi_pcie_ep *ep);
	unsigned int (*func_conf_select)(struct sunxi_pcie_ep *ep, u8 func_no);
};

struct sunxi_pcie_port {
	struct device			*dev;
	void __iomem			*dbi_base;
	u32				cfg0_base;
	void __iomem			*va_cfg0_base;
	u32				cfg0_size;
	resource_size_t			io_base;
	phys_addr_t			io_bus_addr;
	u32				io_size;
	phys_addr_t			mem_base;
	u32				mem_size;
	u32				num_ob_windows;
	struct sunxi_pcie_host_ops	*ops;
	struct sunxi_pcie		*pcie;
	int				msi_irq;
	struct irq_domain		*irq_domain;
	struct irq_domain		*msi_domain;
	struct pci_host_bridge		*bridge;
	// raw_spinlock_t			lock;
	unsigned long			msi_map[BITS_TO_LONGS(INT_PCI_MSI_NR)];
	bool				has_its;
	bool				cpu_pcie_addr_quirk;
};

struct sunxi_pci_edma_chan;

struct sunxi_pcie {
	struct udevice		*dev;
	void		*dbi_base;
	void 		*app_base;
    // fdt_addr_t dbi_phys_base;
#if defined(CONFIG_AW_FPGA_S4) || defined(CONFIG_AW_FPGA_V7)
	void __iomem		*phy_base;
	void __iomem		*clk_base;
	struct regmap		*phy_i2c;
#endif
	int			link_gen;
	int		first_busno;
	struct sunxi_pcie_port	pcie_port;
	struct sunxi_pcie_ep	ep;
	struct clk		pcie_aux;
	struct clk		pcie_slv;
	struct clk		pcie_its;
	struct reset_ctl    pcie_rst;
	struct reset_ctl    pwrup_rst;
	struct reset_ctl    pcie_its_rst;
	struct phy *phy;
	struct dma_trx_obj	*dma_obj;
	const struct sunxi_pcie_of_data *drvdata;
	struct gpio_desc	rst_gpio;
	struct gpio_desc	wake_gpio;
	u32			lanes;
	u32			num_edma;
	unsigned long		*rd_edma_map;
	unsigned long		*wr_edma_map;
	struct sunxi_pci_edma_chan	*dma_wr_chn;
	struct sunxi_pci_edma_chan	*dma_rd_chn;
	const char *pcie1v8_supply;
	const char *pcie3v3_supply;
};

#define to_sunxi_pcie_from_pp(x)	\
		container_of((x), struct sunxi_pcie, pcie_port)

#define to_sunxi_pcie_from_ep(endpoint)   \
		container_of((endpoint), struct sunxi_pcie, ep)

void sunxi_pcie_plat_init(struct udevice *dev);
int sunxi_pcie_plat_hw_init(struct udevice *dev);

void sunxi_pcie_plat_set_rate(struct sunxi_pcie *pci);
void sunxi_pcie_plat_set_mode(struct sunxi_pcie *pci);
void sunxi_pcie_plat_ltssm_enable(struct sunxi_pcie *pci);
void sunxi_pcie_plat_ltssm_disable(struct sunxi_pcie *pci);

int sunxi_pcie_cfg_write(void __iomem *addr, int size, ulong val);
int sunxi_pcie_cfg_read(void __iomem *addr, int size, ulong *val);

u32 sunxi_pcie_readl(struct sunxi_pcie *pcie, u32 offset);
void sunxi_pcie_writel(u32 val, struct sunxi_pcie *pcie, u32 offset);

void sunxi_pcie_writel_dbi(struct sunxi_pcie *pci, u32 reg, u32 val);
u32 sunxi_pcie_readl_dbi(struct sunxi_pcie *pci, u32 reg);
void sunxi_pcie_writew_dbi(struct sunxi_pcie *pci, u32 reg, u16 val);
u16 sunxi_pcie_readw_dbi(struct sunxi_pcie *pci, u32 reg);
void sunxi_pcie_writeb_dbi(struct sunxi_pcie *pci, u32 reg, u8 val);
u8 sunxi_pcie_readb_dbi(struct sunxi_pcie *pci, u32 reg);

void sunxi_pcie_dbi_ro_wr_en(struct sunxi_pcie *pci);
void sunxi_pcie_dbi_ro_wr_dis(struct sunxi_pcie *pci);

#endif /* _PCIE_SUNXI_H */
