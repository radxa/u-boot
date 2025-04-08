/*
 * Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
*/

#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/clock.h>
#include <asm/arch/timer.h>
#include <asm/arch/usb.h>
#include <physical_key.h>
#include "private_uboot.h"

#define HOSC_19_2M			(BIT(14))
#define HOSC_26M			(BIT(15))

void clock_init_uart(void)
{
	return;
}


#ifndef FPGA_PLATFORM
#if 0  /* TODO */
static int clk_get_pll_para(struct core_pll_freq_tbl *factor, int pll_clk)
{
	int index;

	index = pll_clk / 24;
	factor->FactorP = 0;
	factor->FactorN = index;
	factor->FactorM = 0;

	return 0;
}

static void setbit(u32 cpux, u8 bit)
{
	u32 reg_val;
	reg_val = readl(cpux);
	reg_val |= (1 << bit);
	writel(reg_val, cpux);
}

static void clearbit(u32 cpux, u8 bit)
{
	u32 reg_val;
	reg_val = readl(cpux);
	reg_val &= ~(1 << bit);
	writel(reg_val, cpux);
}

static void enable_pll(u32 cpux, struct core_pll_freq_tbl *pll)
{
	return;
}
#endif
#endif

int clock_set_corepll(int frequency)
{
	return  0;
}

/* pll npu */
uint clock_get_pll_npu(void)
{
	uint pll_npu;
	uint reg_val;
	uint factor_n, factor_p0, factor_m1;
	reg_val = readl(SUNXI_CCM_BASE + PLL_NPU_LOCK_CTRL_REG);

	factor_n = ((reg_val >> 8) & 0xff) + 1;
	factor_p0 = ((reg_val >> 20) & 0x03) + 1;
	factor_m1 = ((reg_val >> 1) & 0x01) + 1;
	pll_npu = 24 * factor_n /factor_p0/factor_m1;

	return pll_npu;
}

/* pll ddr */
uint clock_get_pll_ddr(void)
{
	uint pll_ddr;
	uint reg_val;
	uint factor_n, factor_p0, factor_m1;
	reg_val = readl(SUNXI_CCM_BASE + PLL_DDR_CTRL_REG);

	factor_n = ((reg_val >> 8) & 0xff) + 1;
	factor_p0 = ((reg_val >> 20) & 0x03) + 1;
	factor_m1 = ((reg_val >> 1) & 0x01) + 1;
	pll_ddr = 24 * factor_n /factor_p0/factor_m1;

	return pll_ddr;
}

/* pll peri0 480m */
uint clock_get_pll_480m(void)
{
	uint pll_480m;
	uint reg_val;
	uint factor_n, factor_p0, factor_m1;

	reg_val = readl(SUNXI_CCM_BASE +  PLL_PERI0_CTRL_REG);

	factor_n = ((reg_val >> 8) & 0xff) + 1;
	factor_p0 = ((reg_val >> 2) & 0x03) + 1;
	factor_m1 = ((reg_val >> 1) & 0x01) + 1;
	pll_480m = 24 * factor_n /factor_p0/factor_m1;

	return pll_480m;
}

/* pll peri0 2x */
uint clock_get_pll6(void)
{
	uint pll6 ;
	uint reg_val;
	uint factor_n, factor_p0, factor_m1;

	reg_val = readl(SUNXI_CCM_BASE +  PLL_PERI0_CTRL_REG);

	factor_n = ((reg_val >> 8) & 0xff) + 1;
	factor_p0 = ((reg_val >> 20) & 0x03) + 1;
	factor_m1 = ((reg_val >> 1) & 0x01) + 1;
	pll6 = 24 * factor_n /factor_p0/factor_m1;

	return pll6;
}

uint clock_get_ddrpll(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	uint reg_val, ddrpll;
	uint factor_n, factor_m1, factor_m0;
	/*fre: ddrpll = 24*N/M1/M0 */
	reg_val = readl(&ccm->pll_ddr_ctrl_reg);

	factor_n  = ((reg_val >> 8) & 0xff) + 1;
	factor_m0 = ((reg_val >> 1) & 0x01) + 1;
	factor_m1 = ((reg_val >> 0) & 0x01) + 1;
	ddrpll    = 24 * factor_n / factor_m0 / factor_m1;

	return ddrpll;
}

uint clock_get_peri1_pll6_pll4(int div)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	uint reg_val;
	uint factor_n, factor_p0, factor_m1, pll6;

	reg_val = readl(&ccm->pll_peri1_ctrl_reg);

	/*pll = 24*N/M1/P0*/
	factor_n  = ((reg_val >> 8) & 0xff) + 1;
	factor_p0 = ((reg_val >> 16) & 0x07) + 1;
	factor_m1 = ((reg_val >> 1) & 0x01) + 1;
	pll6      = (24 * factor_n / factor_p0 / factor_m1) / div;

	return pll6;
}

uint get_hosc(void)
{
	u32 reg_val;
	reg_val = readl(SUNXI_RTC_BASE + RTC_XO_CONTROL0_REG);

	if (reg_val & HOSC_19_2M)
		return 19200000;
	else if (reg_val & HOSC_26M)
		return 26000000;
	else
		return 24000000;
}

uint clock_get_corepll(void)
{
	unsigned int reg_val;
	int 	div_m, div_p;
	int 	factor_n;
	int 	clock = 0;
	int	clock_src;

	reg_val   = readl(SUNXI_CPU_PLL_CFG_BASE + CPU_L_PLL_REG);
	clock_src = (reg_val >> 24) & 0x07;

	switch (clock_src) {
	case 0:
	case 1:
	case 2://hosc
		clock = get_hosc();
		break;
	case 3://CPU_L_PP/P
		reg_val  = readl(SUNXI_CPU_PLL_CFG_BASE + CPU_L_PLL_CONTROL_REG);
		factor_n = ((reg_val >> 8) & 0xff);
		div_m    = ((reg_val >> 0) & 0x3) + 1;
		reg_val  = readl(SUNXI_CPU_PLL_CFG_BASE + CPU_L_PLL_REG);
		div_p    = 1<<((reg_val >>16) & 0x3);
		clock = get_hosc();
		clock = clock*factor_n/div_m/div_p;
		break;
	case 4://PERI0PLL_DIV2
		reg_val  = readl(SUNXI_CCMU_BASE + PLL_PERI0_CTRL_REG);
		factor_n = ((reg_val >> 8) & 0xff) + 1;
		div_p    = ((reg_val >> 20) & 0x3) + 1;
		clock = 24*factor_n/div_p;
		break;
	case 5://CPU_BACK_PLL
		reg_val  = readl(SUNXI_CPU_PLL_CFG_BASE + CPU_BACK_PLL_CONTROL_REG);
		factor_n = ((reg_val >> 8) & 0xff) + 1;
		div_p    = ((reg_val >> 20) & 0x3) + 1;
		clock = get_hosc();
		clock = clock*factor_n/div_p;
		break;
	default:
		return 0;
	}

	return clock / 1000000;
}

uint clock_get_axi(void)
{

	return 0;
}


uint clock_get_ahb(void)
{
	unsigned int reg_val = 0;
	int factor_m = 0;
	int clock = 0;
	int src = 0, src_clock = 0;

	reg_val = readl(SUNXI_CCM_BASE + AHB_CLK_REG);
	src = (reg_val >> 24)&0x3;
	factor_m  = ((reg_val >> 0) & 0x1f) + 1;

	switch (src) {
	case 0://OSC24M
		src_clock = 24;
		break;
	case 1://CCMU_32K
		src_clock = 32/1000;
		break;
	case 2:	//RC16M
		src_clock = 16;
		break;
	case 3://PLL_PERI0(600M)
		src_clock   = clock_get_pll6()/2;
		break;
	default:
			return 0;
	}

	clock = src_clock/factor_m;

	return clock;
}


uint clock_get_apb1(void)
{
	unsigned int reg_val = 0;
	int src = 0, src_clock = 0;
	int clock = 0, factor_m = 0;

	reg_val = readl(SUNXI_CCM_BASE + APB1_CLK_REG);
	factor_m  = ((reg_val >> 0) & 0x1f) + 1;
	src = (reg_val >> 24)&0x3;

	switch (src) {
	case 0://OSC24M
		src_clock = 24;
		break;
	case 1://CCMU_32K
		src_clock = 32/1000;
		break;
	case 2:	//PSI
		src_clock = 16;
		break;
	case 3://PLL_PERI0(600m)
		src_clock = clock_get_pll6()/2;
		break;
	default:
		return 0;
	}

	clock = src_clock/factor_m;

	return clock;
}

uint clock_get_apb2(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	unsigned int reg_val = 0;
	int clock = 0, factor_m = 0, factor_n = 0;
	int src = 0, src_clock = 0;

	reg_val = readl(&ccm->apb2_cfg);
	src = (reg_val >> 24)&0x3;
	factor_m  = ((reg_val >> 0) & 0x1f) + 1;
	factor_n  = 1;

	switch (src) {
	case 0://OSC24M
		src_clock = 24;
		break;
	case 1://CCMU_32K
		src_clock = 32/1000;
		break;
	case 2:	//PSI
		src_clock = 16;
		break;
	case 3:	//PSI
		src_clock = clock_get_pll6();
		break;
	default:
			return 0;
	}

	clock = src_clock/factor_m/factor_n;

	return clock;

}


uint clock_get_mbus(void)
{
	unsigned int reg_val;
	unsigned int src = 0, clock = 0, div = 0;
	reg_val = readl(SUNXI_CCM_BASE + MBUS_CLK_REG);

	//get src
	src = (reg_val >> 24) & 0x7;
	//get div M, the divided clock is divided by M+1
	div = (reg_val & 0x1f) + 1;

	switch (src) {
	case 0: //src is sys_clk24m
		clock = 24;
		break;
	case 1: //src is   pll_periph0(2x)/2
		clock = clock_get_pll6() / 2;
		break;
	case 2: //src is pll_ddr
		clock_get_pll_ddr();
		break;
	case 3: //src is periph0_480
		clock_get_pll_480m();
		break;
	case 4: //src is periph0_400
		clock = clock_get_pll6() / 3;
		break;
	case 5: //src is pll_npu
		clock_get_pll_npu();
		break;
	}

	clock = clock / div;

	return clock;
}

int usb_open_clock(void)
{
	unsigned int reg_val;
	//usb 0
	setbits_le32(SUNXI_CCM_BASE + USB0_CLK_REG,
		     BIT(USB0_CLK_REG_USB0_PHY_RSTN_OFFSET));
	setbits_le32(SUNXI_CCM_BASE + USB0_BGR_REG,
		     BIT(USB0_BGR_REG_USB0_DEVICE_GATING_OFFSET));
	clrbits_le32(SUNXI_CCM_BASE + USB0_BGR_REG,
		     BIT(USB0_BGR_REG_USB0_DEVICE_RST_OFFSET));
	setbits_le32(SUNXI_CCM_BASE + USB0_BGR_REG,
		     BIT(USB0_BGR_REG_USB0_DEVICE_RST_OFFSET));

	/* FIXME: Reference from sun60iw2 USB2.0 DRD operation guide */
	reg_val = readl(SUNXI_USBOTG_BASE + USBC_REG_o_SEL);
	reg_val |= (0x01 << 0);
	writel(reg_val, (SUNXI_USBOTG_BASE + USBC_REG_o_SEL));

	reg_val = readl((const volatile void __iomem *)(SUNXI_USBOTG_BASE + USBC_REG_o_PHYCTL));
	reg_val &= ~(0x01 << USBC_PHY_CTL_SIDDQ);
	reg_val |= 0x01 << USBC_PHY_CTL_VBUSVLDEXT;
	writel(reg_val, (volatile void __iomem *)(SUNXI_USBOTG_BASE + USBC_REG_o_PHYCTL));

	return 0;
}

int usb_close_clock(void)
{
	unsigned int reg_val;

	reg_val = readl((const volatile void __iomem *)(SUNXI_USBOTG_BASE + USBC_REG_o_PHYCTL));
	reg_val |= (0x01 << USBC_PHY_CTL_SIDDQ);
	writel(reg_val, (volatile void __iomem *)(SUNXI_USBOTG_BASE + USBC_REG_o_PHYCTL));

	clrbits_le32(SUNXI_CCM_BASE + USB0_BGR_REG,
		     BIT(USB0_BGR_REG_USB0_DEVICE_RST_OFFSET) |
			     BIT(USB0_BGR_REG_USB0_EHCI_RST_OFFSET));
	__msdelay(1);

	clrbits_le32(SUNXI_CCM_BASE + USB0_BGR_REG,
		     BIT(USB0_BGR_REG_USB0_DEVICE_GATING_OFFSET) |
			     BIT(USB0_BGR_REG_USB0_EHCI_GATING_OFFSET));
	__msdelay(1);

	clrbits_le32(SUNXI_CCM_BASE + USB0_CLK_REG,
		     BIT(USB0_CLK_REG_USB0_PHY_RSTN_OFFSET) |
			     BIT(USB0_CLK_REG_USB0_CLKEN_OFFSET));
	__msdelay(1);

	return 0;
}

void clock_open_timer(int timernum)
{
	u32 reg_value = 0;

	reg_value = readl(SUNXI_CCM_BASE + TIMER0_CLK0_CLK_REG + timernum * 4);
	reg_value |= (0 <<24);
	reg_value |= (2 << 0);
	writel(reg_value, (SUNXI_CCM_BASE + TIMER0_CLK0_CLK_REG + timernum * 4));

	/*enable timer*/
	reg_value = readl(SUNXI_CCM_BASE + TIMER0_CLK0_CLK_REG + timernum * 4);
	reg_value |= (1 << 31);
	writel(reg_value, (SUNXI_CCM_BASE + TIMER0_CLK0_CLK_REG + timernum * 4));

	reg_value = readl(SUNXI_CCM_BASE + TIMER0_BGR_REG);
	reg_value |= (1 << 16);
	writel(reg_value, (SUNXI_CCM_BASE + TIMER0_BGR_REG));

	reg_value = readl(SUNXI_CCM_BASE + TIMER0_BGR_REG);
	reg_value |= (1 << 0);
	writel(reg_value, (SUNXI_CCM_BASE + TIMER0_BGR_REG));
	__msdelay(1);

}

void clock_close_timer(int timernum)
{
	/*
	 * We can't reset the timer,
	 * because it will cause the timer to be secure mode
	 */
	u32 reg_value = 0;

	/*disable timer*/
	reg_value = readl(SUNXI_CCM_BASE + TIMER0_CLK0_CLK_REG + timernum * 4);
	reg_value &= ~(1 << 31);
	writel(reg_value, (SUNXI_CCM_BASE + TIMER0_CLK0_CLK_REG + timernum * 4));

	reg_value = readl(SUNXI_CCM_BASE + TIMER0_BGR_REG);
	reg_value &= ~(1 << 0);
	writel(reg_value, (SUNXI_CCM_BASE + TIMER0_BGR_REG));
	__msdelay(1);
}

int clock_gate_rate(int timernum)
{
	u32 reg_value, factor_p_val, clk_src_val, clk_rate, i;
	u32 factor_p = 1;

	reg_value = readl(SUNXI_CCM_BASE + TIMER0_CLK0_CLK_REG + timernum * 4);
	factor_p_val = (reg_value >> TIMER0_CLK0_CLK_REG_FACTOR_P_OFFSET) & 0x3;
	for (i = 0; i < factor_p_val; i++) {
		factor_p *= 2;
	}
	clk_src_val = (reg_value >> TIMER0_CLK0_CLK_REG_CLK_SRC_SEL_OFFSET) & 0x7;

	switch (clk_src_val) {
	case 0:
		clk_rate = 24000000 / factor_p;
		break;
	case 1:
		clk_rate = 16000000 / factor_p;
		break;
	case 2:
		clk_rate = 32000 / factor_p;
		break;
	case 3:
		clk_rate = 200000000 / factor_p;
		break;
	case 4:
		clk_rate = get_hosc() / factor_p;
		break;
	default:
		clk_rate = 0;
		break;
	}

	return clk_rate;
}

void clock_set_gic(void)
{
	u32 reg_value = 0;

	reg_value = readl(SUNXI_CCM_BASE + GIC_CLK_REG);
	reg_value &= ~((1 << 31) | (7 << 24) | (0x1f << 0));
	reg_value |= ((1 << 31) | (3 << 24) | (0 << 0));
	writel(reg_value, SUNXI_CCM_BASE + GIC_CLK_REG);
	udelay(20);
}

int sunxi_gpadc_init(void)
{
	u32 reg_val = 0;

	/* deassert */
	reg_val = readl(SUNXI_CCM_BASE + GPADC0_BGR_REG);
	reg_val |= (1 << 16);
	writel(reg_val, SUNXI_CCM_BASE + GPADC0_BGR_REG);

	/* enable ADC gating */
	reg_val = readl(SUNXI_CCM_BASE + GPADC0_BGR_REG);
	reg_val |= (1 << 0);
	writel(reg_val, SUNXI_CCM_BASE + GPADC0_BGR_REG);

	/* enable ADC 24m clk */
	reg_val = readl(SUNXI_CCM_BASE + GPADC0_24M_CLK_REG);
	reg_val |= (1 << 31);
	writel(reg_val, SUNXI_CCM_BASE + GPADC0_24M_CLK_REG);

	/*ADC sample ferquency divider*ferquency=CLK_IN/(n+1), 24000000/(0x5dbf+1)=1000Hz
	* ADC acquire time
	* time=CLK_IN/(n+1) ,24000000/(0x2f+1)=500000Hz
	*/
	reg_val = readl(GP_SR_CON);
	reg_val &= ~(0xffff << 16);
	reg_val |= (0x5dbf << 16);
	writel(reg_val, GP_SR_CON);

	/*choose continue work mode and enable ADC*/
	reg_val = readl(GP_CTRL);
	reg_val &= ~(1<<18);
	reg_val |= ((1<<19) | (1<<16));
	writel(reg_val, GP_CTRL);

	/* disable all key irq */
	writel(0, GP_DATA_INTC);
	writel(1, GP_DATA_INTS);

	return 0;
}
