/* SPDX-License-Identifier: GPL-2.0+ */
#include <asm/arch/plat-sun60iw2p1/clock_autogen.h>
#define  sunxi_ccm_reg CCMU_st
#define  pll1_cfg                pll_cpu0_ctrl_reg
#define  apb2_cfg                apb1_clk_reg
#define  uart_gate_reset         uart0_bgr_reg
#define  cpu_axi_cfg             cpu_clk_reg
#define  pll6_cfg                pll_peri0_ctrl_reg
#define  psi_ahb1_ahb2_cfg       ahb_clk_reg
#define  apb1_cfg                apb0_clk_reg
#define  mbus_cfg                mbus_clk_reg
#define  ve_clk_cfg		 pll_ve0_ctrl_reg
#define  de_clk_cfg              de0_clk_reg
#define  mbus_gate               mbus_mat_clk_gating_reg
#define  dma_gate_reset          dma0_bgr_reg
#define spi0_clk_cfg             spi0_clk_reg
#define spi_gate_reset           spi0_bgr_reg
#define  sd0_clk_cfg             smhc0_clk_reg
#define  sd1_clk_cfg             smhc1_clk_reg
#define  sd2_clk_cfg             smhc2_clk_reg
#define  sd3_clk_cfg             smhc3_clk_reg
#define  sd_gate_reset           smhc0_bgr_reg
#define  twi_gate_reset          twi0_bgr_reg
#define  ce_gate_reset           ce_bgr_reg
#define  ce_clk_cfg              ce_clk_reg
#define  lradc_gate_reset        lradc_bgr_reg
#define  gpadc_gate_reset        gpadc0_bgr_reg
#define  pcie_aux_clk_reg        pcie0_aux_clk_reg
#define  pcie_axi_slv_clk_reg    pcie0_axi_slv_clk_reg
#define  pcie_bgr_reg            pcie0_bgr_reg
#define  its_bgr_reg             its0_bgr_reg

// #define  APB2_CLK_SRC_OSC24M     (APB1_CLK_REG_CLK_SRC_SEL_HOSC << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
// #define  APB2_CLK_SRC_OSC32K     (APB1_CLK_REG_CLK_SRC_SEL_CLK32K << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
// #define  APB2_CLK_SRC_PSI        (APB1_CLK_REG_CLK_SRC_SEL_CLK16M_RC << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
// #define  APB2_CLK_SRC_PLL6       (APB1_CLK_REG_CLK_SRC_SEL_PERI0_600M_BUS << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)

#define APB2_CLK_RATE_N_1               (0x0 << 8)
#define APB2_CLK_RATE_N_2               (0x1 << 8)
#define APB2_CLK_RATE_N_4               (0x2 << 8)
#define APB2_CLK_RATE_N_8               (0x3 << 8)
#define APB2_CLK_RATE_N_MASK            (3 << 8)
#define APB2_CLK_RATE_M(m)              (((m)-1) << APB1_CLK_REG_FACTOR_M_OFFSET)
#define APB2_CLK_RATE_M_MASK            (3 << APB1_CLK_REG_FACTOR_M_OFFSET)

/* MMC clock bit field */
#define CCM_MMC_CTRL_M(x)               (x)
#define CCM_MMC_CTRL_N(x)               ((x) << SMHC0_CLK_REG_FACTOR_N_OFFSET)
#define CCM_MMC_CTRL_OSCM24             (SMHC0_CLK_REG_CLK_SRC_SEL_SYS_CLK24M << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PLL6X2             (SMHC0_CLK_REG_CLK_SRC_SEL_PERI0_400M << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PLL_PERIPH2X2      (SMHC0_CLK_REG_CLK_SRC_SEL_PERI0_300M << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PERI0_400M_freq         (400000000)
#define CCM_MMC_CTRL_PERI0_300M_freq         (300000000)
#define CCM_MMC_CTRL_PERI0_800M_freq         (800000000)
#define CCM_MMC_CTRL_PERI0_600M_freq         (600000000)
#define CCM_MMC_CTRL_PERI0_400M         (0x1 << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PERI0_300M         (0x2 << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PERI0_800M         (0x1 << SMHC2_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PERI0_600M         (0x2 << SMHC2_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PERI1_400M		(0x3 << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PERI1_300M		(0x4 << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PERI1_800M		(0x3 << SMHC2_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PERI1_600M		(0x4 << SMHC2_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_ENABLE             (SMHC0_CLK_REG_SMHC0_CLK_GATING_CLOCK_IS_ON << SMHC0_CLK_REG_SMHC0_CLK_GATING_OFFSET)
/* if doesn't have these delays */
#define CCM_MMC_CTRL_OCLK_DLY(a)        ((void) (a), 0)
#define CCM_MMC_CTRL_SCLK_DLY(a)        ((void) (a), 0)

/* Module gate/reset shift*/
#define RESET_SHIFT                     (16)
#define GATING_SHIFT                    (0)

/*CE*/
#define CE_CLK_SRC_MASK                   (0x7)
#define CE_CLK_SRC_SEL_BIT                (CE_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CE_CLK_SRC                        (CE_CLK_REG_CLK_SRC_SEL_PERI0_400M)

#define CE_CLK_DIV_RATION_N_BIT           (8)
#define CE_CLK_DIV_RATION_N_MASK          (0x3)
#define CE_CLK_DIV_RATION_N               (0)

#define CE_CLK_DIV_RATION_M_BIT           (CE_CLK_REG_FACTOR_M_OFFSET)
//@TODO
#define CE_CLK_DIV_RATION_M_MASK          (CE_CLK_REG_FACTOR_M_CLEAR_MASK)
#define CE_CLK_DIV_RATION_M               (0)

#define CE_SCLK_ONOFF_BIT                 (CE_CLK_REG_CE_CLK_GATING_OFFSET)
//@TODO
#define CE_SCLK_ON                        (CE_CLK_REG_CE_CLK_GATING_CLOCK_IS_OFF)

//@TODO
#define CE_GATING_PASS                    (CE_BGR_REG_CE_GATING_MASK)
#define CE_GATING_BIT                     (CE_BGR_REG_CE_GATING_OFFSET)

#define CE_RST_BIT                        (CE_BGR_REG_CE_RST_OFFSET)
//@TODO
#define CE_DEASSERT                       (CE_BGR_REG_CE_SYS_RST_ASSERT)
#define CE_SYS_RST_BIT                    (CE_BGR_REG_CE_SYS_RST_OFFSET)
#define CE_SYS_GATING_BIT                 (CE_BGR_REG_CE_SYS_GATING_OFFSET)

#define CE_MBUS_GATING_MASK (1)
#define CE_MBUS_GATING_BIT (MBUS_GATE_EN_REG_CE_MCLK_EN_OFFSET)
#define CE_MBUS_GATING (1)

/* PLL_CPU */
#define CPU_L_PLL_REG		0x101c
#define CPU_L_PLL_CONTROL_REG	0x1000
#define RTC_XO_CONTROL0_REG	0x0160
#define CPU_BACK_PLL_CONTROL_REG	0x0000

#define USBEHCI0_RST_BIT 20
#define USBEHCI0_GATIING_BIT 4
#define USBPHY0_RST_BIT 30
#define USBPHY0_SCLK_GATING_BIT 31

#define USBEHCI1_RST_BIT 20
#define USBEHCI1_GATIING_BIT 4
#define USBPHY1_RST_BIT 30
#define USBPHY1_SCLK_GATING_BIT 31

/* PCIE */
#define ITS_PCIE_RST			16
#define ITS_PCIE_CLK_GATING_BIT	1
#define PCIE_AUX_CLK_GATING_BIT	31
#define PCIE_AXI_SLV_GATING_BIT	31
#define PCIE_BRG_REG_RST		17
#define PCIE_BRG_REG_PWRUP_RST	16
