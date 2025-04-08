/* SPDX-License-Identifier: GPL-2.0+ */
#include <asm/arch/plat-sun55iw3p1/clock_autogen.h>
#define  sunxi_ccm_reg CCMU_st
#define  pll1_cfg                pll_cpu0_ctrl_reg
#define  apb2_cfg                apb1_clk_reg
#define  uart_gate_reset         uart_bgr_reg
#define  cpu_axi_cfg             cpu_clk_reg
#define  pll6_cfg                pll_peri0_ctrl_reg
#define  psi_ahb1_ahb2_cfg       ahb_clk_reg
#define  apb1_cfg                apb0_clk_reg
#define  mbus_cfg                mbus_clk_reg
#define  ve_clk_cfg              ve_clk_reg
#define  de_clk_cfg              de0_clk_reg
#define  mbus_gate               mbus_mat_clk_gating_reg
#define  dma_gate_reset          dma_bgr_reg
#define  sd0_clk_cfg             smhc0_clk_reg
#define  sd1_clk_cfg             smhc1_clk_reg
#define  sd2_clk_cfg             smhc2_clk_reg
#define  sd_gate_reset           smhc_bgr_reg
#define  twi_gate_reset          twi_bgr_reg
#define  ce_gate_reset           ce_bgr_reg
#define  ce_clk_cfg              ce_clk_reg
#define  gpadc_gate_reset        gpadc_bgr_reg

#define  APB2_CLK_SRC_OSC24M     (APB1_CLK_REG_CLK_SRC_SEL_HOSC << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
#define  APB2_CLK_SRC_OSC32K     (APB1_CLK_REG_CLK_SRC_SEL_CLK32K << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
#define  APB2_CLK_SRC_PSI        (APB1_CLK_REG_CLK_SRC_SEL_CLK16M_RC << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
#define  APB2_CLK_SRC_PLL6       (APB1_CLK_REG_CLK_SRC_SEL_PERI0_600M_BUS << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)

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
#define CCM_MMC_CTRL_OSCM24             (SMHC0_CLK_REG_CLK_SRC_SEL_HOSC << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PLL6X2             (SMHC0_CLK_REG_CLK_SRC_SEL_PERI0_400M << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PLL_PERIPH2X2      (SMHC0_CLK_REG_CLK_SRC_SEL_PERI0_300M << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
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
#define CE_CLK_DIV_RATION_M_MASK          (CE_CLK_REG_FACTOR_M_CLEAR_MASK)
#define CE_CLK_DIV_RATION_M               (0)

#define CE_SCLK_ONOFF_BIT                 (CE_CLK_REG_CE_CLK_GATING_OFFSET)
#define CE_SCLK_ON                        (CE_CLK_REG_CE_CLK_GATING_CLOCK_IS_ON)

#define CE_GATING_PASS                    (CE_BGR_REG_CE_GATING_PASS)
#define CE_GATING_BIT                     (CE_BGR_REG_CE_GATING_OFFSET)

#define CE_MBUS_GATING_MASK               (1)
#define CE_MBUS_GATING_BIT                (MBUS_MAT_CLK_GATING_REG_CE_MCLK_EN_OFFSET)
#define CE_MBUS_GATING                    (MBUS_MAT_CLK_GATING_REG_CE_MCLK_EN_PASS)

#define CE_RST_BIT                        (CE_BGR_REG_CE_RST_OFFSET)
#define CE_DEASSERT                       (CE_BGR_REG_CE_SYS_RST_DE_ASSERT)
#define CE_SYS_RST_BIT                    (CE_BGR_REG_CE_SYS_RST_OFFSET)
#define CE_SYS_GATING_BIT                 (CE_BGR_REG_CE_SYS_GATING_OFFSET)

/* SPIF clock bit field */
#define CCM_SPIF_CTRL_M(x)		((x) - 1)
#define CCM_SPIF_CTRL_N(x)		((x) << 8)
#define CCM_SPIF_CTRL_HOSC		(0x0 << 24)
#define CCM_SPIF_CTRL_PERI400M		(0x1 << 24)
#define CCM_SPIF_CTRL_PERI300M		(0x2 << 24)
#define CCM_SPIF_CTRL_ENABLE		(0x1 << 31)
#define GET_SPIF_CLK_SOURECS(x)		(x == CCM_SPIF_CTRL_PERI400M ? 400000000 : 300000000)
#define CCM_SPIF_CTRL_PERI		CCM_SPIF_CTRL_PERI400M
#define SPIF_RESET_SHIFT		(19)
#define SPIF_GATING_SHIFT		(3)

#define CCMU_PLL_CPU0_CTRL_REG (SUNXI_CPU_SYS_CFG_BASE + 0x817000)
#define CCMU_PLL_CPU1_CTRL_REG (SUNXI_CPU_SYS_CFG_BASE + 0x817000 + 0x4)
#define CCMU_PLL_CPU2_CTRL_REG (SUNXI_CPU_SYS_CFG_BASE + 0x817000 + 0x8)
#define CCMU_PLL_CPU3_CTRL_REG (SUNXI_CPU_SYS_CFG_BASE + 0x817000 + 0xc)

#define RTC_XO_CONTROL0_REG	0x0160

#define USBEHCI0_RST_BIT 20
#define USBEHCI0_GATIING_BIT 4
#define USBPHY0_RST_BIT 30
#define USBPHY0_SCLK_GATING_BIT 31

#define USBEHCI1_RST_BIT 21
#define USBEHCI1_GATIING_BIT 5
#define USBPHY1_RST_BIT 30
#define USBPHY1_SCLK_GATING_BIT 31

/* PCIE */
#define PCIE_AUX_CLK_GATING_BIT	31
#define PCIE_BRG_REG_RST		16
#define PCIE_BRG_REG_PWRUP_RST	PCIE_BRG_REG_RST
#define PCIE_REF_CLK_GATING		31

struct sunxi_cpu_pll_reg{
	uint32_t pll_cpu0_ctrl_reg; /*0x00*/
	uint32_t pll_cpu1_ctrl_reg;
	uint32_t pll_cpu2_ctrl_reg;
	uint32_t pll_cpu3_ctrl_reg;
	uint32_t pll_cpu1_pat0_reg; /*0x10*/
	uint32_t pll_cpu1_pat1_reg;
	uint32_t pll_cpu2_pat0_reg;
	uint32_t pll_cpu2_pat1_reg;
	uint32_t pll_cpu3_pat0_reg; /*0x20*/
	uint32_t pll_cpu3_pat1_reg;
	uint32_t pll_cpu0_bias_reg;
	uint32_t pll_cpu1_bias_reg;
	uint32_t pll_cpu2_bias_reg; /*0x30*/
	uint32_t pll_cpu3_bias_reg;
	uint32_t pll_cpu0_tun_reg;
	uint32_t pll_cpu1_tun0_reg;
	uint32_t pll_cpu1_tun1_reg; /*0x40*/
	uint32_t pll_cpu2_tun0_reg;
	uint32_t pll_cpu2_tun1_reg;
	uint32_t pll_cpu3_tun0_reg;
	uint32_t pll_cpu3_tun1_reg; /*0x50*/
	uint32_t pll_cpu1_ssc_reg;
	uint32_t pll_cpu2_ssc_reg;
	uint32_t pll_cpu3_ssc_reg;
	uint32_t cpua_clk_reg;      /*0x60*/
	uint32_t cpub_clk_reg;
	uint32_t cpu_gating_reg;
	uint32_t dsu_clk_reg;
	uint32_t pll_test_clk_sel;
};

typedef struct {
	unsigned int pll_ctrl_reg0; /* 0x0 pll ctrl register 0 */
	unsigned int pll_ctrl_reg1; /* 0x4 pll ctrl register 1 */
	unsigned int prcm_sec_swtich_reg; /* 0x8 pll security switch register */
	unsigned int audio_ctrl_reg; /* 0xc pll audio ctrl register */
	unsigned int audio_pat0_ctrl_reg; /* 0x10 pll audio pattern0 ctrl reg */
	unsigned int audio_pat1_ctrl_reg; /* 0x14 pll audio pattern1 ctrl reg */
	unsigned int audio_bias_reg; /* 0x18 audio bias reg */
	unsigned int audio_clk_reg; /* 0x1C audio out clock reg */
	unsigned int dsp_clk_reg; /* 0x20 dsp clock reg */
	unsigned int reserved1[2];
	unsigned int i2s0_clk_reg; /* 0x2C i2s0 clock register */
	unsigned int i2s1_clk_reg; /* 0x30 i2s1 clock register */
	unsigned int i2s2_clk_reg; /* 0x34 i2s2 clock register */
	unsigned int i2s3_clk_reg; /* 0x38 i2s3 clock register */
	unsigned int i2s3_asrc_clk_reg; /* 0x3C i2s3 asrc clock register */
	unsigned int i2s_bgr_reg; /* 0x40 i2s bus gating reset register */
	unsigned int spdif_tx_clk_reg; /* 0x44 spdif tx clock register */
	unsigned int spdif_rx_clk_reg; /* 0x48 spdif rx clock register */
	unsigned int spdif_brg_reg; /* 0x4C spdif bus gating reset register */
	unsigned int dmic_clk_reg; /* 0x50 dmic rx clock register */
	unsigned int dmic_brg_reg; /* 0x54 dmic bus gating reset register */
	unsigned int audio_codec_dac_clk_reg; /* 0x58 audio codec dac clock register */
	unsigned int audio_codec_adc_clk_reg; /* 0x5C audio codec adc clock register */
	unsigned int audio_codec_brg_reg; /* 0x60 audio codec bus gating reset register */
	unsigned int ahbs_rdy_tout_ctrl_reg; /* 0x64 ahbs ready timeout control register */
	unsigned int dsp_msgbox_brg_reg; /* 0x68 dsp msgbox bus gating reset register */
	unsigned int dsp_cfg_brg_reg; /* 0x6C dsp config bus gating reset register */
	unsigned int npu_brg_reg; /* 0x70 npu bus gating reset register */
	unsigned int timer0_clk_reg; /* 0x74 timer0 clock register */
	unsigned int timer1_clk_reg; /* 0x78 timer1 clock register */
	unsigned int timer2_clk_reg; /* 0x7C timer2 clock register */
	unsigned int timer3_clk_reg; /* 0x80 timer3 clock register */
	unsigned int timer4_clk_reg; /* 0x84 timer4 clock register */
	unsigned int timer5_clk_reg; /* 0x88 timer5 clock register */
	unsigned int timer_bus_brg_reg; /* 0x8C timer bus gating reset register */
	unsigned int reserved2[28];
	unsigned int dsp_dbg_brg_reg; /* 0x100 dsp debug bus gating register */
	unsigned int dsp_dma_brg_reg; /* 0x104 dsp dma bus gating register */
	unsigned int tzma0_brg_reg; /* 0x108 tzma0 bus gating reset register */
	unsigned int tzma1_brg_reg; /* 0x10C tzma1 bus gating reset register */
	unsigned int tzma2_brg_reg; /* 0x110 tzma2 bus gating reset register */
	unsigned int pubsram_brg_reg; /* 0x114 pubsram bus gating register */
	unsigned int ahbs1_rdy_tout_ctrl_reg; /* 0x118 ahbs1 ready timeout control register */
	unsigned int mclk_gating_cfg_reg; /* 0x11C mclk gating config register */
	unsigned int rv_clk_reg; /* 0x120 riscv clock register */
	unsigned int rv_cfg_brg_reg; /* 0x124 riscv config bus gating reset register */
	unsigned int rv_msgbox_brg_reg; /* 0x128 riscv msgbox bus gating reset register */
	unsigned int reserved3[1];
	unsigned int pwm_clk_reg; /* 0x130 pwm clock register */
	unsigned int pwm_brg_reg; /* 0x134 pwm bus gating reset register */
	unsigned int reserved4[10];
	unsigned int pll_backdoor_output_en_reg; /* 0x160 pll backdoor output enable register */
	unsigned int test_dbg_reg; /* 0x164 test debug register */
} sunxi_dsp_ccu_reg;

