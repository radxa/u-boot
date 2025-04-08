/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2007-2012
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
 */

#ifndef _SUNXI_GPIO_H
#define _SUNXI_GPIO_H

#include <linux/types.h>
#include <asm/arch/cpu.h>

/*
 * sunxi has 9 banks of gpio, they are:
 * PA0 - PA17 | PB0 - PB23 | PC0 - PC24
 * PD0 - PD27 | PE0 - PE31 | PF0 - PF5
 * PG0 - PG9  | PH0 - PH27 | PI0 - PI12
 */

#define SUNXI_GPIO_A	0
#define SUNXI_GPIO_B	1
#define SUNXI_GPIO_C	2
#define SUNXI_GPIO_D	3
#define SUNXI_GPIO_E	4
#define SUNXI_GPIO_F	5
#define SUNXI_GPIO_G	6
#define SUNXI_GPIO_H	7
#define SUNXI_GPIO_I	8
#define SUNXI_GPIO_J	9
#define SUNXI_GPIO_K	10

enum io_pow_mode_e {
	IO_MODE_1_8_V = 0,
	IO_MODE_3_3_V,
	IO_MODE_DEFAULT,
	IO_MODE_AUTO
};

enum pin_e {
	GPIO_GROUP_A = 0,
	GPIO_GROUP_B,
	GPIO_GROUP_C,
	GPIO_GROUP_D,
	GPIO_GROUP_E,
	GPIO_GROUP_F,
	GPIO_GROUP_G,
	GPIO_GROUP_H,
	GPIO_GROUP_I,
	GPIO_GROUP_J,
	GPIO_GROUP_K,
	GPIO_GROUP_L
};

/*
 * This defines the number of GPIO banks for the _main_ GPIO controller.
 * You should fix up the padding in struct sunxi_gpio_reg below if you
 * change this.
 */
#if CONFIG_IS_ENABLED(AW_GPIO_V4) || CONFIG_IS_ENABLED(AW_GPIO_V3) || CONFIG_IS_ENABLED(SUNXI_GPIO_V2)
#define SUNXI_GPIO_BANKS 11
#else
#define SUNXI_GPIO_BANKS 9
#endif

/*
 * sun6i/sun8i and later SoCs have an additional GPIO controller (R_PIO)
 * at a different register offset.
 *
 * sun6i has 2 banks:
 * PL0 - PL8  | PM0 - PM7
 *
 * sun8i has 1 bank:
 * PL0 - PL11
 *
 * sun9i has 3 banks:
 * PL0 - PL9  | PM0 - PM15  | PN0 - PN1
 */
#define SUNXI_GPIO_L	11
#define SUNXI_GPIO_M	12
#define SUNXI_GPIO_N	13

#if CONFIG_IS_ENABLED(AW_GPIO_V4)
struct sunxi_gpio {
	u32 cfg[4];
	u32 dat;
	u32 pad_until_0x14[3];
	u32 drv[4];
	u32 pull[2];
	u32 pad_until_0x38[2];
	u32 int_cfg[4];
	u32 int_ctl;
	u32 int_sta;
	u32 int_deb;
	u32 pad_until_0x5c[5];
	u32 sec;
	u32 pad_until_0x74[3];
};

struct sunxi_gpio_reg {
	u32 pad_until_0x00[12];
	u32 dbc_clk_set;
	u32 dbc_bypass;
	u32 pad_until_0x38[2];
	u32 pow_mod_state;
	u32 pad_until_0x44;
	u32 pow_val;
	u32 pad_until_0x4c[9];
	u32 pow_val_set_ctl;
	u32 pad_until_0x74;
	u32 version_id;
	u32 pad_until_0x7c;
	struct sunxi_gpio gpio_bank[SUNXI_GPIO_BANKS];
};

struct sunxi_r_gpio {
	u32 cfg[4];
	u32 dat;
	u32 pad_until_0x14[3];
	u32 drv[4];
	u32 pull[2];
	u32 pad_until_0x38[2];
	u32 int_cfg[4];
	u32 int_ctl;
	u32 int_sta;
	u32 int_deb;		/* interrupt debounce */
	u32 pad_until_0x5c[5];
	u32 int_sec;
	u32 pad_until_0x74[3];
};

struct sunxi_r_gpio_int {
	u32 int_cfg[4];
	u32 int_ctl;
	u32 int_sta;
	u32 int_deb;		/* interrupt debounce */
	u32 pad_until_0x5c[5];
	u32 int_sec;
	u32 pad_until_0x74[3];
};

struct sunxi_r_gpio_reg {
	u32 pad_until_0x00[12];
	u32 s_dbc_clk_set;
	u32 s_dbc_btpass;
	u32 pad_until_0x38[2];
	u32 s_pow_mod_state;
	u32 pad_until_0x44;
	u32 s_pow_val;
	u32 pad_until_0x4c[11];
	u32 s_version_id;
	u32 pad_until_0x7c;
	struct sunxi_r_gpio gpio_bank[2];
};

#define BANK_TO_GPIO(bank)	(((bank) < SUNXI_GPIO_L) ? \
	(void *)&((struct sunxi_gpio_reg *)SUNXI_PIO_BASE)->gpio_bank[bank] : \
	(void *)&((struct sunxi_r_gpio_reg *)SUNXI_R_PIO_BASE)->gpio_bank[(bank) - SUNXI_GPIO_L])

#define GPIO_BANK(pin)		((pin) >> 5)
#define GPIO_NUM(pin)		((pin) & 0x1f)

#define GPIO_CFG_INDEX(pin)	(((pin) & 0x1f) >> 3)
#define GPIO_CFG_OFFSET(pin)	((((pin) & 0x1f) & 0x7) << 2)

#define GPIO_DRV_INDEX(pin)	(((pin) & 0x1f) >> 3)
#define GPIO_DRV_OFFSET(pin)	((((pin) & 0x1f) & 0x7) << 2)

#define GPIO_PULL_INDEX(pin)	(((pin) & 0x1f) >> 4)
#define GPIO_PULL_OFFSET(pin)	((((pin) & 0x1f) & 0xf) << 1)

#elif CONFIG_IS_ENABLED(AW_GPIO_V3)
struct sunxi_gpio {
	u32 cfg[4];
	u32 dat;
	u32 dat_set;
	u32 dat_clr;
	u32 pad_until_0xa0[1];
	u32 drv[4];
	u32 pull[2];
	u32 pad_until_0xc0[2];
	u32 int_cfg[4];
	u32 int_ctl;
	u32 int_sta;
	u32 int_deb;
	u32 pad_until_0xf0[5];
	u32 sec;
	u32 pad_until_0x100[3];
};

struct sunxi_gpio_reg {
	u32 base_addr_cfg[SUNXI_GPIO_BANKS];
	u32 pad_until_0x40[5];
	u32 pow_mod_sel;
	u32 pad_until_0x48[1];
	u32 pow_val;
	u32 pad_until_0x70[9];
	u32 pow_val_set;
	u32 pad_until_0x78[1];
	u32 ver;
	u32 pad_until_0x80[1];
	struct sunxi_gpio gpio_bank[SUNXI_GPIO_BANKS];
};

struct sunxi_r_gpio {
	u32 cfg[4];
	u32 dat;
	u32 drv[4];
	u32 pull[2];
	u32 pad_until_0x30[1];
};

struct sunxi_r_gpio_int {
	u32 cfg[4];
	u32 ctl;
	u32 sta;
	u32 deb;		/* interrupt debounce */
	u32 pad_until_0x220[1];
};

struct sunxi_r_gpio_reg {
	struct sunxi_r_gpio gpio_bank[2];
	u32 pad_until_0x200[104];
	struct sunxi_r_gpio_int gpio_int[2];
	u32 pad_until_0x340[64];
	u32 pow_mod_sel;
	u32 pad_until_0x348[1];
	u32 pow_val;
	u32 pad_until_0x370[9];
	u32 ver;
	u32 pad_until_0x400[35];
	u32 secure_bank[2];
};

#define BANK_TO_GPIO(bank)	(((bank) < SUNXI_GPIO_L) ? \
	(void *)&((struct sunxi_gpio_reg *)SUNXI_PIO_BASE)->gpio_bank[bank] : \
	(void *)&((struct sunxi_r_gpio_reg *)SUNXI_R_PIO_BASE)->gpio_bank[(bank) - SUNXI_GPIO_L])

#define GPIO_BANK(pin)		((pin) >> 5)
#define GPIO_NUM(pin)		((pin) & 0x1f)

#define GPIO_CFG_INDEX(pin)	(((pin) & 0x1f) >> 3)
#define GPIO_CFG_OFFSET(pin)	((((pin) & 0x1f) & 0x7) << 2)

#define GPIO_DRV_INDEX(pin)	(((pin) & 0x1f) >> 3)
#define GPIO_DRV_OFFSET(pin)	((((pin) & 0x1f) & 0x7) << 2)

#define GPIO_PULL_INDEX(pin)	(((pin) & 0x1f) >> 4)
#define GPIO_PULL_OFFSET(pin)	((((pin) & 0x1f) & 0xf) << 1)

#elif CONFIG_IS_ENABLED(SUNXI_GPIO_V2)
struct sunxi_gpio {
	u32 cfg[4];
	u32 dat;
	u32 drv[4];
	u32 pull[2];
	u32 res;
};

/* gpio interrupt control */
struct sunxi_gpio_int {
	u32 cfg[3];
	u32 ctl;
	u32 sta;
	u32 deb;		/* interrupt debounce */
};

struct sunxi_gpio_reg {
	struct sunxi_gpio gpio_bank[SUNXI_GPIO_BANKS];
	u8 res[0x50];/*pad to 0x200*/
	struct sunxi_gpio_int gpio_int;
};

#ifdef CONFIG_MACH_SUN55IW3
#define SUNXI_PK_BASE	0x2000500
#define BANK_TO_GPIO(bank) \
	({\
		struct sunxi_gpio *pio; \
		if (bank == 10) { \
			pio = &((struct sunxi_gpio_reg *)SUNXI_PK_BASE)->gpio_bank[0];  \
		} else if ((bank) < SUNXI_GPIO_L) { \
			pio = &((struct sunxi_gpio_reg *)SUNXI_PIO_BASE)->gpio_bank[bank];  \
		} else { \
			pio = &((struct sunxi_gpio_reg *)SUNXI_R_PIO_BASE)->gpio_bank[(bank) - SUNXI_GPIO_L];  \
		} \
		pio; \
	})

#else
#define BANK_TO_GPIO(bank)	(((bank) < SUNXI_GPIO_L) ? \
	&((struct sunxi_gpio_reg *)SUNXI_PIO_BASE)->gpio_bank[bank] : \
	&((struct sunxi_gpio_reg *)SUNXI_R_PIO_BASE)->gpio_bank[(bank) - SUNXI_GPIO_L])
#endif

#define GPIO_BANK(pin)		((pin) >> 5)
#define GPIO_NUM(pin)		((pin) & 0x1f)

#define GPIO_CFG_INDEX(pin)	(((pin) & 0x1f) >> 3)
#define GPIO_CFG_OFFSET(pin)	((((pin) & 0x1f) & 0x7) << 2)

#define GPIO_DRV_INDEX(pin)	(((pin) & 0x1f) >> 3)
#define GPIO_DRV_OFFSET(pin)	((((pin) & 0x1f) & 0x7) << 2)

#define GPIO_PULL_INDEX(pin)	(((pin) & 0x1f) >> 4)
#define GPIO_PULL_OFFSET(pin)	((((pin) & 0x1f) & 0xf) << 1)
#else
struct sunxi_gpio {
	u32 cfg[4];
	u32 dat;
	u32 drv[2];
	u32 pull[2];
};

/* gpio interrupt control */
struct sunxi_gpio_int {
	u32 cfg[3];
	u32 ctl;
	u32 sta;
	u32 deb;		/* interrupt debounce */
};

struct sunxi_gpio_reg {
	struct sunxi_gpio gpio_bank[SUNXI_GPIO_BANKS];
	u8 res[0xbc];
	struct sunxi_gpio_int gpio_int;
};

#define BANK_TO_GPIO(bank)	(((bank) < SUNXI_GPIO_L) ? \
	&((struct sunxi_gpio_reg *)SUNXI_PIO_BASE)->gpio_bank[bank] : \
	&((struct sunxi_gpio_reg *)SUNXI_R_PIO_BASE)->gpio_bank[(bank) - SUNXI_GPIO_L])

#define GPIO_BANK(pin)		((pin) >> 5)
#define GPIO_NUM(pin)		((pin) & 0x1f)

#define GPIO_CFG_INDEX(pin)	(((pin) & 0x1f) >> 3)
#define GPIO_CFG_OFFSET(pin)	((((pin) & 0x1f) & 0x7) << 2)

#define GPIO_DRV_INDEX(pin)	(((pin) & 0x1f) >> 4)
#define GPIO_DRV_OFFSET(pin)	((((pin) & 0x1f) & 0xf) << 1)

#define GPIO_PULL_INDEX(pin)	(((pin) & 0x1f) >> 4)
#define GPIO_PULL_OFFSET(pin)	((((pin) & 0x1f) & 0xf) << 1)
#endif

/* GPIO bank sizes */
#define SUNXI_GPIO_A_NR		32
#define SUNXI_GPIO_B_NR		32
#define SUNXI_GPIO_C_NR		32
#define SUNXI_GPIO_D_NR		32
#define SUNXI_GPIO_E_NR		32
#define SUNXI_GPIO_F_NR		32
#define SUNXI_GPIO_G_NR		32
#define SUNXI_GPIO_H_NR		32
#define SUNXI_GPIO_I_NR		32
#define SUNXI_GPIO_J_NR		32
#define SUNXI_GPIO_K_NR		32
#define SUNXI_GPIO_L_NR		32
#define SUNXI_GPIO_M_NR		32

#define SUNXI_GPIO_NEXT(__gpio) \
	((__gpio##_START) + (__gpio##_NR) + 0)

enum sunxi_gpio_number {
	SUNXI_GPIO_A_START = 0,
	SUNXI_GPIO_B_START = SUNXI_GPIO_NEXT(SUNXI_GPIO_A),
	SUNXI_GPIO_C_START = SUNXI_GPIO_NEXT(SUNXI_GPIO_B),
	SUNXI_GPIO_D_START = SUNXI_GPIO_NEXT(SUNXI_GPIO_C),
	SUNXI_GPIO_E_START = SUNXI_GPIO_NEXT(SUNXI_GPIO_D),
	SUNXI_GPIO_F_START = SUNXI_GPIO_NEXT(SUNXI_GPIO_E),
	SUNXI_GPIO_G_START = SUNXI_GPIO_NEXT(SUNXI_GPIO_F),
	SUNXI_GPIO_H_START = SUNXI_GPIO_NEXT(SUNXI_GPIO_G),
	SUNXI_GPIO_I_START = SUNXI_GPIO_NEXT(SUNXI_GPIO_H),
	SUNXI_GPIO_L_START = 352,
	SUNXI_GPIO_M_START = SUNXI_GPIO_NEXT(SUNXI_GPIO_L),
	SUNXI_GPIO_N_START = SUNXI_GPIO_NEXT(SUNXI_GPIO_M),
	SUNXI_GPIO_AXP0_START = 1024,
};

/* SUNXI GPIO number definitions */
#define SUNXI_GPA(_nr)	(SUNXI_GPIO_A_START + (_nr))
#define SUNXI_GPB(_nr)	(SUNXI_GPIO_B_START + (_nr))
#define SUNXI_GPC(_nr)	(SUNXI_GPIO_C_START + (_nr))
#define SUNXI_GPD(_nr)	(SUNXI_GPIO_D_START + (_nr))
#define SUNXI_GPE(_nr)	(SUNXI_GPIO_E_START + (_nr))
#define SUNXI_GPF(_nr)	(SUNXI_GPIO_F_START + (_nr))
#define SUNXI_GPG(_nr)	(SUNXI_GPIO_G_START + (_nr))
#define SUNXI_GPH(_nr)	(SUNXI_GPIO_H_START + (_nr))
#define SUNXI_GPI(_nr)	(SUNXI_GPIO_I_START + (_nr))
#define SUNXI_GPL(_nr)	(SUNXI_GPIO_L_START + (_nr))
#define SUNXI_GPM(_nr)	(SUNXI_GPIO_M_START + (_nr))
#define SUNXI_GPN(_nr)	(SUNXI_GPIO_N_START + (_nr))

#define SUNXI_GPAXP0(_nr)	(SUNXI_GPIO_AXP0_START + (_nr))

/* GPIO pin function config */
#define SUNXI_GPIO_INPUT	0
#define SUNXI_GPIO_OUTPUT	1
#define SUNXI_GPIO_DISABLE	7

#define SUNXI_GPA_EMAC		2
#define SUN6I_GPA_GMAC		2
#define SUN7I_GPA_GMAC		5
#define SUN6I_GPA_SDC2		5
#define SUN6I_GPA_SDC3		4
#define SUN8I_H3_GPA_UART0	2

#define SUN4I_GPB_PWM		2
#define SUN4I_GPB_TWI0		2
#define SUN4I_GPB_TWI1		2
#define SUN5I_GPB_TWI1		2
#define SUN4I_GPB_TWI2		2
#define SUN5I_GPB_TWI2		2
#define SUN4I_GPB_UART0		2
#define SUN5I_GPB_UART0		2
#define SUN8I_GPB_UART2		2
#define SUN8I_A33_GPB_UART0	3
#define SUN8I_A83T_GPB_UART0	2
#define SUN8I_V3S_GPB_UART0	3
#define SUN50I_GPB_UART0	4

#define SUNXI_GPC_NAND		2
#define SUNXI_GPC_SPI0		3
#define SUNXI_GPC_SDC2		3
#define SUN6I_GPC_SDC3		4
#define SUN50I_GPC_SPI0		4

#define SUN8I_GPD_SDC1		3
#define SUNXI_GPD_LCD0		2
#define SUNXI_GPD_LVDS0		3

#define SUN5I_GPE_SDC2		3
#define SUN8I_GPE_TWI2		3

#define SUNXI_GPF_SDC0		2
#define SUNXI_GPF_UART0		4
#define SUN8I_GPF_UART0		3

#define SUN4I_GPG_SDC1		4
#define SUN5I_GPG_SDC1		2
#define SUN6I_GPG_SDC1		2
#define SUN8I_GPG_SDC1		2
#define SUN6I_GPG_TWI3		2
#define SUN5I_GPG_UART1		4

#define SUN6I_GPH_PWM		2
#define SUN8I_GPH_PWM		2
#define SUN4I_GPH_SDC1		5
#define SUN6I_GPH_TWI0		2
#define SUN8I_GPH_TWI0		2
#define SUN6I_GPH_TWI1		2
#define SUN8I_GPH_TWI1		2
#define SUN6I_GPH_TWI2		2
#define SUN6I_GPH_UART0		2
#define SUN9I_GPH_UART0		2
#define SUN50IW3_GPH_UART0	4

#define SUNXI_GPI_SDC3		2
#define SUN7I_GPI_TWI3		3
#define SUN7I_GPI_TWI4		3

#define SUN6I_GPL0_R_P2WI_SCK	3
#define SUN6I_GPL1_R_P2WI_SDA	3

#define SUN8I_GPL_R_RSB		2
#define SUN8I_H3_GPL_R_TWI	2
#define SUN8I_A23_GPL_R_TWI	3
#define SUN8I_GPL_R_UART	2

#define SUN9I_GPN_R_RSB		3

/* GPIO pin pull-up/down config */
#define SUNXI_GPIO_PULL_DISABLE	0
#define SUNXI_GPIO_PULL_UP	1
#define SUNXI_GPIO_PULL_DOWN	2

/* Virtual AXP0 GPIOs */
#define SUNXI_GPIO_AXP0_PREFIX "AXP0-"
#define SUNXI_GPIO_AXP0_VBUS_DETECT	4
#define SUNXI_GPIO_AXP0_VBUS_ENABLE	5
#define SUNXI_GPIO_AXP0_GPIO_COUNT	6

#if CONFIG_IS_ENABLED(AW_GPIO_V3) || CONFIG_IS_ENABLED(AW_GPIO_V4)
void sunxi_gpio_set_cfgbank(void *gpio, int bank_offset, u32 val);
int sunxi_gpio_get_cfgbank(void *gpio, int bank_offset);
#else
void sunxi_gpio_set_cfgbank(struct sunxi_gpio *pio, int bank_offset, u32 val);
int sunxi_gpio_get_cfgbank(struct sunxi_gpio *pio, int bank_offset);
#endif

void sunxi_gpio_set_cfgpin(u32 pin, u32 val);
int sunxi_gpio_get_cfgpin(u32 pin);
int sunxi_gpio_set_drv(u32 pin, u32 val);
int sunxi_gpio_set_pull(u32 pin, u32 val);
int sunxi_name_to_gpio_bank(const char *name);
int sunxi_name_to_gpio(const char *name);

enum io_pow_mode_e io_get_volt_val(enum pin_e port_group);
#if CONFIG_SUNXI_GPIO_POWER_VOL_MODE
void sunxi_io_set_pow_mode_on_actual_val(enum pin_e port_group);
void sunxi_io_set_pow_mode_to_default(enum pin_e port_group);
#endif

#define name_to_gpio(name) sunxi_name_to_gpio(name)

#if !defined CONFIG_SPL_BUILD && defined CONFIG_AXP_GPIO
int axp_gpio_init(void);
#else
static inline int axp_gpio_init(void) { return 0; }
#endif

#endif /* _SUNXI_GPIO_H */
