/*
 *  * Copyright 2000-2009
 *   * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *    *
 *     * SPDX-License-Identifier:	GPL-2.0+
 *     */
#include <common.h>
#include <asm/io.h>
#include <physical_key.h>
#include <sys_config.h>
#include <fdt_support.h>
#include <console.h>
#include <asm/arch/clock.h>

__weak int sunxi_gpadc_init(void)
{
	uint reg_val = 0;

#if !defined(CONFIG_MACH_SUN60IW2)
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	/* reset */
	reg_val = readl(&ccm->gpadc_gate_reset);
	reg_val &= ~(1 << 16);
	writel(reg_val, ccm->gpadc_gate_reset);

	udelay(2);

	reg_val |= (1 << 16);
	writel(reg_val, ccm->gpadc_gate_reset);

	/* enable ADC gating */
	reg_val = readl(&ccm->gpadc_gate_reset);
	reg_val |= (1 << 0);
	writel(reg_val, ccm->gpadc_gate_reset);
#endif

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

int sunxi_gpadc_read(int channel)
{
	u32 ints;
	int key = -1;
	u32 snum = 0;

	writel(1 << channel, GP_CS_EN);
	udelay(1500);

	ints = readl(GP_DATA_INTS);
	/* clear the pending data */
	writel((ints & (0x1 << channel)), GP_DATA_INTS);

	/* if there is already data pending, read it */
	if (ints & (GPADC0_DATA_PENDING << channel)) {
		snum = readl(GP_CH0_DATA + (channel * 4));
	}

	key = snum;

	return key;
}

int sunxi_get_gpadc_vol(int channel)
{
	u32 vol_m;

	if (channel < 0) {
		printf("error: channel < 0\n");
		return -1;
	}

	vol_m = sunxi_gpadc_read(channel);

	/* convert to voltage, unit:mV */
	vol_m = (vol_m * 1800) / 4095;

	return vol_m;
}


