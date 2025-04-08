/* SPDX-License-Identifier: GPL-2.0+ */
#include <asm/arch/plat-sun55iw3p1/cpu_autogen.h>

#define SUNXI_GIC600_BASE             (SUNXI_CPU_GIC600_BASE)
#define SUNXI_PIO_BASE                (SUNXI_GPIO_BASE)
#define SUNXI_R_PIO_BASE              (SUNXI_R_GPIO_BASE)
#define SUNXI_RTC_DATA_BASE           (SUNXI_RTC_BASE+0x100)
/*#define SUNXI_RSB_BASE*/
#define SUNXI_CCM_BASE                (SUNXI_CCMU_BASE)
#define SUNXI_DSP_CCM_BASE            (SUNXI_DSP_PRCM_BASE)
#define SUNXI_EHCI0_BASE              (SUNXI_USB0_BASE + 0x1000)
#define SUNXI_EHCI1_BASE              (SUNXI_USB1_BASE)
#define SUNXI_USBOTG_BASE             (SUNXI_USB0_BASE)
#define SUNXI_PCIE_BASE               (SUNXI_PCIE_SYS_BASE)
#define SUNXI_KEYADC_BASE             (SUNXI_LRADC_BASE)
#if defined(CONFIG_SOUND_SUNXI_DSP_DMAC)
#define SUNXI_DMA_BASE                (SUNXI_DSP_DMA_BASE)
#else
#define SUNXI_DMA_BASE                (SUNXI_DMAC_BASE)
#endif
#define SUNXI_MMC0_BASE               (SUNXI_SMHC0_BASE)
#define SUNXI_MMC1_BASE               (SUNXI_SMHC1_BASE)
#define SUNXI_MMC2_BASE               (SUNXI_SMHC2_BASE)
#define SUNXI_PRCM_BASE               (SUNXI_R_PRCM_BASE)
#define SUNXI_NFC_BASE                (SUNXI_NAND_BASE)
#define SUNXI_SS_BASE                 (SUNXI_CE_NS_BASE)
#define SUNXI_WDT_BASE                (0x02050000)
#define SUNXI_SPIF_BASE		      (0x047F0000)

#define SUNXI_SRAMC_BASE              0x00040000
#define R_PRCM_REG_BASE               (0x07010000)
#define SUNXI_RTWI_BRG_REG            (SUNXI_PRCM_BASE+0x019c)
#define SUNXI_R_UART_BASE             (0x07080000)
#define SUNXI_R_TWI_BASE              (0x07081400)

#define PIOC_REG_o_POW_MOD_SEL 0x380
#define PIOC_REG_o_POW_MS_CTL 0x384
#define PIOC_REG_o_POW_MS_VAL 0x388

#define PIOC_REG_POW_MOD_SEL (SUNXI_PIO_BASE + PIOC_REG_o_POW_MOD_SEL)
#define PIOC_REG_POW_MS_CTL (SUNXI_PIO_BASE + PIOC_REG_o_POW_MS_CTL)
#define PIOC_REG_POW_VAL (SUNXI_PIO_BASE + PIOC_REG_o_POW_MS_VAL)

#define PIOC_SEL_Px_3_3V_VOL 1
#define PIOC_SEL_Px_1_8V_VOL 0

#define PIOC_CTL_Px_ENABLE 0
#define PIOC_CTL_Px_DISABLE 1

#define PIOC_VAL_Px_3_3V_VOL 0
#define PIOC_VAL_Px_1_8V_VOL 1

#define PIOC_CTL_Px_DEFUALT PIOC_CTL_Px_ENABLE
#define PIOC_SEL_Px_DEFAULT PIOC_SEL_Px_1_8V_VOL

#ifndef __ASSEMBLY__
void sunxi_board_init(void);
void sunxi_reset(void);
int sunxi_get_sid(unsigned int *sid);
#endif