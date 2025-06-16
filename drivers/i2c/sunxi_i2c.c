/*
 * Copyright (C) 2016 Allwinner.
 * zhouhuacai <zhouhuacai@allwinnertech.com>
 * wangwei <wangwei@allwinnertech.com>
 *
 * SUNXI TWI Controller Definition
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <i2c.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/gpio.h>
#include <asm/arch/timer.h>
#include <sunxi_board.h>
#include "sunxi_i2c.h"
#include <fdtdec.h>
#include <fdt_support.h>


/* #define SUNXI_I2C_DEBUG */
#if defined(SUNXI_I2C_DEBUG)
#define I2C_DBG(msg...)                                                        \
	do {                                                                   \
		{                                                              \
			printf("[I2C-DBG] %s,line:%d:    ", __func__, __LINE__); \
			printf(msg);                                           \
		}                                                              \
	} while (0)
#else
#define I2C_DBG(fmt, arg...)
#endif /* endif defined(_SUNXI_I2C_DEBUG) */

#define I2C_ERR(msg...)								\
	do {									\
		{								\
			pr_msg("[I2C-ERROR]:%s() %d ", __func__, __LINE__);	\
			pr_msg(msg);						\
		}								\
	} while (0)

#define I2C_WRN(fmt, arg...)	printf("[I2C-WRN]:%s() %d "fmt, __func__, __LINE__)

#define MAX_SUNXI_I2C_NUM (SUNXI_PHY_I2C_BUS_MAX)
#define MAX_SUNXI_I2C_TIMEOUT (0xff)

__attribute__((section(".data")))
static  struct sunxi_twi_reg *sunxi_i2c[MAX_SUNXI_I2C_NUM] = {NULL, NULL, NULL, NULL};

static int sunxi_get_i2c_bus_num(int i2c_number, bool r_i2c_flag)
{
	int i = 0, bus_num = -1;
	char i2c_name[20];

	if (r_i2c_flag)
		sprintf(i2c_name, "sunxi_r_i2c%d", i2c_number);
	else
		sprintf(i2c_name, "sunxi_i2c%d", i2c_number);

	for (i = 0; i < CONFIG_SYS_NUM_I2C_BUSES; i++) {
		if (!strcmp(I2C_ADAP_NR(i)->name, i2c_name)) {
			bus_num = i;
			break;
		}
	}

	return bus_num;
}

static int find_node_offset_by_alias(void *fdt, const char *alias)
{
	int aliases_offset, node_offset;
	const char *path;

	aliases_offset = fdt_path_offset(fdt, "/aliases");
	if (aliases_offset < 0) {
		printf("Error: /aliases node not found\n");
		return -1;
	}

	path = fdt_getprop(fdt, aliases_offset, alias, NULL);
	if (!path) {
		printf("Error: alias %s not founed\n", alias);
		return -1;
	}

	node_offset = fdt_path_offset(fdt, path);
	if (node_offset < 0) {
		printf("Error: node for path %s not found\n", path);
		return -1;
	}

	return node_offset;
}

static bool sunxi_check_bind_i2c_from_device_addr(const char *i2c_aliases, u16 device_addr)
{
	int node, device_node;
	const fdt32_t *reg_prop;
	int addr;
	const char *i2c_name;

	node = find_node_offset_by_alias(working_fdt, i2c_aliases);
	if (node >= 0) {
		device_node = fdt_first_subnode(working_fdt, node);
		while (device_node >= 0) {
			i2c_name = fdt_get_name(working_fdt, device_node, NULL);

			reg_prop = fdt_getprop(working_fdt, device_node, "reg", NULL);
			if (!reg_prop) {
				device_node = fdt_next_subnode(working_fdt, device_node);
				continue;
			}

			/* Convert the reg property to an integer value */
			addr = fdt32_to_cpu(*reg_prop);

			if (addr == device_addr) {
				i2c_name = fdt_get_name(working_fdt, node, NULL);
				debug("device 0x%x at i2c name is: %s\n", device_addr, i2c_name);
				return true;
			}

			device_node = fdt_next_subnode(working_fdt, device_node);
		}
	}

	return false;
}

int sunxi_get_i2c_bus_num_from_device_addr(u16 device_addr)
{
	int aliases_node;
	int prop_offset;
	const char *alias_name;
	const char *alias_path;
	const struct fdt_property *prop;
	int len;
	int i2c_number = 0;
	int bus_num = -1;
	int ret = -1;

	aliases_node = fdt_path_offset(working_fdt, "/aliases");
	if (aliases_node < 0) {
		printf("Error: No aliases node found in device tree\n");
		return -1;
	}

	fdt_for_each_property_offset(prop_offset, working_fdt, aliases_node) {
		prop = fdt_get_property_by_offset(working_fdt, prop_offset, &len);

		if (!prop) {
			printf("Error reading property\n");
			continue;
		}

		alias_name = fdt_string(working_fdt, fdt32_to_cpu(prop->nameoff));
		alias_path = prop->data;

		if (!alias_name)
			continue;

		if (strncmp(alias_name, "twi", 3) == 0) {
			alias_path = fdt_getprop_by_offset(working_fdt, prop_offset, NULL, &len);
			if (alias_path) {
				debug("Alias:%s -> %s\n", alias_name, alias_path);

				ret = sunxi_check_bind_i2c_from_device_addr(alias_name, device_addr);
				if (ret) {
					if (strstr(alias_path, "s_twi")) {
						/* TODO: select the bus num of sunxi_r_i2c0 temporarily */
						bus_num = sunxi_get_i2c_bus_num(0, true);
					} else {
						i2c_number = simple_strtoul(alias_name + 3, NULL, 10);
						bus_num = sunxi_get_i2c_bus_num(i2c_number, false);
					}
				}
			}
		}
	}

	return bus_num;
}

static inline void twi_soft_reset(int bus_num)
{
	struct sunxi_twi_reg *i2c = sunxi_i2c[bus_num];

	i2c->eft  = 0;
	i2c->srst = 1;
}

static inline void twi_set_start(int bus_num)
{
	struct sunxi_twi_reg *i2c = sunxi_i2c[bus_num];

	i2c->ctl  |= TWI_CTL_STA;
}

static inline void twi_clear_irq_flag(int bus_num)
{
	struct sunxi_twi_reg *i2c = sunxi_i2c[bus_num];

	i2c->ctl |= TWI_CTL_INTFLG;
}

static inline int twi_wait_irq_flag(int bus_num, u32 time)
{
	struct sunxi_twi_reg *i2c = sunxi_i2c[bus_num];

	while ((--time) && (!(i2c->ctl & TWI_CTL_INTFLG)))
		;

	if (time <= 0) {
		I2C_DBG("twi wait clear irq flag timeout\n");
		return SUNXI_I2C_TOUT;
	}

	return SUNXI_I2C_OK;
}

static inline void twi_enable_ack(int bus_num)
{
	struct sunxi_twi_reg *i2c = sunxi_i2c[bus_num];

	i2c->ctl |= TWI_CTL_ACK;
}

static inline void twi_disable_ack(int bus_num)
{
	struct sunxi_twi_reg *i2c = sunxi_i2c[bus_num];

	i2c->ctl &= ~TWI_CTL_ACK;
}

static inline void twi_set_stop(int bus_num)
{
	struct sunxi_twi_reg *i2c = sunxi_i2c[bus_num];

	i2c->ctl  |= TWI_CTL_STP;
}

static inline int twi_wait_status(int bus_num, u8 status, u32 time)
{
	struct sunxi_twi_reg *i2c = sunxi_i2c[bus_num];

	while ((--time) && (i2c->status != status))
		;

	if (time <= 0) {
		I2C_ERR("need 0x%x but status is 0x%x\n", status, i2c->status);
		return SUNXI_I2C_TOUT;
	}

	return SUNXI_I2C_OK;
}

static void twi_enable_lcr(int bus_num)
{
	struct sunxi_twi_reg *i2c = sunxi_i2c[bus_num];

	i2c->lcr |= (TWI_LCR_SCL_EN | TWI_LCR_SDA_EN);
}

/* send 9 clock to release sda */
static int twi_send_clk_9pulse(int bus_num)
{
	int cycle = 10;

	struct sunxi_twi_reg *i2c = sunxi_i2c[bus_num];
	twi_enable_lcr(bus_num);
	__usdelay(500);

	/* toggle I2C SCL and SDA until bus idle */
	while ((cycle > 0) && ((i2c->lcr & TWI_LCR_SDA_CTL) != TWI_LCR_SDA_CTL)) {
		/*control scl and sda output high level*/
		i2c->lcr |= TWI_LCR_SCL_CTL;
		i2c->lcr |= TWI_LCR_SDA_CTL;
		__usdelay(1000);
		/*control scl and sda output low level*/
		i2c->lcr &= ~TWI_LCR_SCL_CTL;
		i2c->lcr &= ~TWI_LCR_SDA_CTL;
		__usdelay(1000);
		cycle--;
	}

	if ((i2c->lcr & TWI_LCR_SDA_CTL) != TWI_LCR_SDA_CTL) {
		I2C_ERR("SDA is still Stuck Low, failed. \n");
		return SUNXI_I2C_FAIL;
	}

	i2c->lcr = 0x0;
	__usdelay(500);

	return SUNXI_I2C_OK;
}

static int twi_start(int bus_num)
{
	u32 timeout = MAX_SUNXI_I2C_TIMEOUT;

	twi_soft_reset(bus_num);
	twi_set_start(bus_num);

	if (twi_wait_irq_flag(bus_num, timeout)) {
		I2C_ERR("START can't sendout!\n");
		return SUNXI_I2C_FAIL;
	}

	if (twi_wait_status(bus_num, I2C_START_TRANSMIT, timeout))
		return SUNXI_I2C_FAIL;

	return SUNXI_I2C_OK;
}

static int twi_restart(int bus_num)
{
	u32 timeout = MAX_SUNXI_I2C_TIMEOUT;

	twi_set_start(bus_num);
	twi_clear_irq_flag(bus_num);
	if (twi_wait_irq_flag(bus_num, timeout)) {
		I2C_ERR("Restart can't sendout!\n");
		return SUNXI_I2C_FAIL;
	}

	if (twi_wait_status(bus_num, I2C_RESTART_TRANSMIT, timeout))
		return SUNXI_I2C_FAIL;

	return SUNXI_I2C_OK;
}

static int twi_send_slave_addr(int bus_num, u32 saddr,  u32 rw)
{
	u32 timeout = MAX_SUNXI_I2C_TIMEOUT;
	struct sunxi_twi_reg *i2c = sunxi_i2c[bus_num];

	rw &= 1;
	i2c->data = ((saddr & 0xff) << 1) | rw;
	twi_clear_irq_flag(bus_num);

	if (twi_wait_irq_flag(bus_num, timeout))
		return SUNXI_I2C_TOUT;

	if ((rw == I2C_WRITE) && (twi_wait_status(bus_num, I2C_ADDRWRITE_ACK, timeout)))
		return SUNXI_I2C_FAIL;
	else if ((rw == I2C_READ) && (twi_wait_status(bus_num, I2C_ADDRREAD_ACK, timeout)))
		return SUNXI_I2C_FAIL;

	return SUNXI_I2C_OK;
}

static int  twi_send_byte_addr(int bus_num, u32 byteaddr)
{
	u32 timeout = MAX_SUNXI_I2C_TIMEOUT;
	struct sunxi_twi_reg *i2c = sunxi_i2c[bus_num];

	i2c->data = byteaddr & 0xff;
	twi_clear_irq_flag(bus_num);

	if (twi_wait_irq_flag(bus_num, timeout))
		return SUNXI_I2C_TOUT;

	if (twi_wait_status(bus_num, I2C_DATAWRITE_ACK, timeout))
		return SUNXI_I2C_FAIL;

	return SUNXI_I2C_OK;
}

static int twi_send_addr(int bus_num, uint addr, int alen)
{
	int i, ret, addr_len;
	char *slave_reg;

	if (alen >= 3)
		addr_len = 2;
	else if (alen <= 1)
		addr_len = 0;
	else
		addr_len = 1;

	slave_reg = (char *)&addr;

	for (i = addr_len; i >= 0; i--) {
		ret = twi_send_byte_addr(bus_num, slave_reg[i] & 0xff);

		if (ret != SUNXI_I2C_OK)
			goto twi_send_addr_err;
	}

twi_send_addr_err:
	return ret;

}

static int twi_get_data(int bus_num, u8 *data_addr, u32 data_count)
{
	u32 timeout = MAX_SUNXI_I2C_TIMEOUT;
	u32  i;
	struct sunxi_twi_reg *i2c = sunxi_i2c[bus_num];

	if (data_count == 1) {
		/* no need ack  */
		twi_clear_irq_flag(bus_num);

		if (twi_wait_irq_flag(bus_num, timeout))
			return SUNXI_I2C_TOUT;

		if (twi_wait_status(bus_num, I2C_DATAREAD_NACK, timeout))
			return SUNXI_I2C_FAIL;

		*data_addr = i2c->data;
	} else {
		for (i = 0; i < data_count - 1; i++) {
			/* need ack  */
			twi_enable_ack(bus_num);
			twi_clear_irq_flag(bus_num);

			if (twi_wait_irq_flag(bus_num, timeout))
				return SUNXI_I2C_TOUT;

			if (twi_wait_status(bus_num, I2C_DATAREAD_ACK, timeout))
				return SUNXI_I2C_FAIL;

			data_addr[i] = i2c->data;
		}

		/* received the last byte  */
		twi_disable_ack(bus_num);
		twi_clear_irq_flag(bus_num);

		if (twi_wait_irq_flag(bus_num, timeout))
			return SUNXI_I2C_TOUT;

		if (twi_wait_status(bus_num, I2C_DATAREAD_NACK, timeout))
			return SUNXI_I2C_FAIL;

		data_addr[data_count - 1] = i2c->data;
	}

	return SUNXI_I2C_OK;
}

static int twi_send_data(int bus_num, u8  *data_addr, u32 data_count)
{
	u32 timeout = MAX_SUNXI_I2C_TIMEOUT;
	u32  i;
	struct sunxi_twi_reg *i2c = sunxi_i2c[bus_num];

	for (i = 0; i < data_count; i++) {
		i2c->data = data_addr[i];
		twi_clear_irq_flag(bus_num);

		if (twi_wait_irq_flag(bus_num, timeout))
			return SUNXI_I2C_TOUT;

		if (twi_wait_status(bus_num, I2C_DATAWRITE_ACK, timeout))
			return SUNXI_I2C_FAIL;
	}

	return SUNXI_I2C_OK;
}

static int twi_stop(int bus_num)
{
	u32 timeout = MAX_SUNXI_I2C_TIMEOUT;
	struct sunxi_twi_reg *i2c = sunxi_i2c[bus_num];

	i2c->ctl |= (0x01 << 4);
	i2c->ctl |= (0x01 << 3);

	twi_set_stop(bus_num);
	twi_clear_irq_flag(bus_num);

	if (twi_wait_status(bus_num, TWI_STAT_IDLE, timeout))
		return SUNXI_I2C_TFAIL;

	return SUNXI_I2C_OK;
}

static void i2c_set_clock(int bus_num, int speed)
{
	int timeout, clk_n, clk_m, i, pow_2_clk_n;
	struct sunxi_twi_reg *i2c = sunxi_i2c[bus_num];

	timeout = MAX_SUNXI_I2C_TIMEOUT;
	twi_soft_reset(bus_num);

	while ((i2c->srst) && (timeout--))
		;

	if ((i2c->lcr & TWI_LCR_NORM_STATUS) != TWI_LCR_NORM_STATUS) {
		I2C_DBG("[i2c%d] bus is busy, lcr = %x\n", bus_num, i2c->lcr);
		twi_send_clk_9pulse(bus_num);
	}
	speed /= 1000; /* khz */

	if (speed < 100)
		speed = 100;
	else if (speed > 400)
		speed = 400;
	/* Foscl=24000/(2^CLK_N*(CLK_M+1)*10) */
	clk_n = (speed == 100) ? 1 : 0;
	pow_2_clk_n = 1;
	for (i = 0; i < clk_n; ++i)
		pow_2_clk_n *= 2;
	clk_m = 2400 / (pow_2_clk_n * speed) - 1;

	i2c->clk = (clk_m << 3) | clk_n;
	i2c->ctl |= TWI_CTL_BUSEN;
	i2c->eft = 0;
}

#if defined(CONFIG_MACH_SUN60IW2)
static uint32_t *sunxi_get_twi_bgr_reg(struct CCMU_st *const ccm, int bus_num)
{
	uint32_t *twi_bgr_reg = NULL;

	switch (bus_num) {
	case 0:
		twi_bgr_reg = &ccm->twi0_bgr_reg;
		break;
	case 1:
		twi_bgr_reg = &ccm->twi1_bgr_reg;
		break;
	case 2:
		twi_bgr_reg = &ccm->twi2_bgr_reg;
		break;
	case 3:
		twi_bgr_reg = &ccm->twi3_bgr_reg;
		break;
	case 4:
		twi_bgr_reg = &ccm->twi4_bgr_reg;
		break;
	case 5:
		twi_bgr_reg = &ccm->twi5_bgr_reg;
		break;
	case 6:
		twi_bgr_reg = &ccm->twi6_bgr_reg;
		break;
	case 7:
		twi_bgr_reg = &ccm->twi7_bgr_reg;
		break;
	case 8:
		twi_bgr_reg = &ccm->twi8_bgr_reg;
		break;
	case 9:
		twi_bgr_reg = &ccm->twi9_bgr_reg;
		break;
	case 10:
		twi_bgr_reg = &ccm->twi10_bgr_reg;
		break;
	case 11:
		twi_bgr_reg = &ccm->twi11_bgr_reg;
		break;
	case 12:
		twi_bgr_reg = &ccm->twi12_bgr_reg;
		break;
	default:
		I2C_ERR("get twi bgr reg error, bus_num:%d\n", bus_num);
		break;
	}

	return twi_bgr_reg;
}
#endif

static void sunxi_i2c_bus_setting(int bus_num, int onoff)
{
	int reg_value = 0;

#if defined(CONFIG_MACH_SUN300IW1)
	volatile void __iomem *clk_reg;
	volatile void __iomem *rst_reg;
#elif defined(CONFIG_MACH_SUN60IW2)
	uint32_t *twi_bgr_reg = NULL;
	struct CCMU_st *const ccm =
		(struct CCMU_st *)SUNXI_CCM_BASE;
	twi_bgr_reg = sunxi_get_twi_bgr_reg(ccm, bus_num);
	if (!twi_bgr_reg) {
		I2C_ERR("i2c bus setting error\n");
		return;
	}
#else
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
#endif

#if defined(CONFIG_SUNXI_VERSION1)
#if defined(CONFIG_MACH_SUN8IW11)
	if (bus_num > 3)
		bus_num += 11; /* clk setting for twi4 start at bit15 (4+11=15) */
#endif
	if (onoff) {
		/* de-assert */
		reg_value = readl(&ccm->apb2_reset_cfg);
		reg_value |= (1 << bus_num);
		writel(reg_value, &ccm->apb2_reset_cfg);

		/* gating clock pass */
		reg_value = readl(&ccm->apb2_gate);
		reg_value &= ~(1 << bus_num);
		writel(reg_value, &ccm->apb2_gate);
		__msdelay(1);
		reg_value |= (1 << bus_num);
		writel(reg_value, &ccm->apb2_gate);
	} else {
		/* gating clock mask */
		reg_value = readl(&ccm->apb2_gate);
		reg_value &= ~(1 << bus_num);
		writel(reg_value, &ccm->apb2_gate);

		/* assert */
		reg_value = readl(&ccm->apb2_reset_cfg);
		reg_value &= ~(1 << bus_num);
		writel(reg_value, &ccm->apb2_reset_cfg);
	}

#elif defined(CONFIG_MACH_SUN300IW1)
	if (bus_num == 0) {
		clk_reg = IOMEM_ADDR(CCMU_BUS_CLK_GATING0_REG);
		rst_reg = IOMEM_ADDR(CCMU_BUS_Reset0_REG);
	} else {
		clk_reg = IOMEM_ADDR(CCMU_BUS_CLK_GATING1_REG);
		rst_reg = IOMEM_ADDR(CCMU_BUS_Reset1_REG);
	}

	if (onoff) {
		/* de-assert */
		reg_value = readl(rst_reg);
		reg_value |= ((bus_num == 0) ? \
		(1 << BUS_Reset0_REG_PRESETN_TWI0_SW_OFFSET) : \
		(1 << (23 + bus_num)));
		writel(reg_value, rst_reg);
		/* gating clock pass */
		reg_value = readl(clk_reg);
		reg_value &= ~((bus_num == 0) ? \
		(1 << BUS_CLK_GATING0_REG_TWI0_PCLK_EN_OFFSET) : \
		(1 << (23 + bus_num)));
		writel(reg_value, clk_reg);
		__msdelay(1);
		reg_value |= ((bus_num == 0) ? \
		(1 << BUS_CLK_GATING0_REG_TWI0_PCLK_EN_OFFSET) : \
		(1 << (23 + bus_num)));
		writel(reg_value, clk_reg);
	} else {
		/* gating clock mask */
		reg_value = readl(clk_reg);
		reg_value &= ~((bus_num == 0) ? \
		(1 << BUS_CLK_GATING0_REG_TWI0_PCLK_EN_OFFSET) : \
		(1 << (23 + bus_num)));
		writel(reg_value, clk_reg);

		/* assert */
		reg_value = readl(rst_reg);
		reg_value &= ~((bus_num == 0) ? \
		(1 << BUS_Reset0_REG_PRESETN_TWI0_SW_OFFSET) : \
		(1 << (23 + bus_num)));
		writel(reg_value, rst_reg);
	}

#elif defined(CONFIG_MACH_SUN60IW2)
	if (onoff) {
		/* de-assert */
		reg_value = readl(twi_bgr_reg);
		reg_value |= (1 << 16);
		writel(reg_value, twi_bgr_reg);

		/* gating clock pass */
		reg_value = readl(twi_bgr_reg);
		reg_value &= ~(1 << 0);
		writel(reg_value, twi_bgr_reg);
		__msdelay(1);
		reg_value |= (1 << 0);
		writel(reg_value, twi_bgr_reg);
	} else {
		/* gating clock mask */
		reg_value = readl(twi_bgr_reg);
		reg_value &= ~(1 << 0);
		writel(reg_value, twi_bgr_reg);

		/* assert */
		reg_value = readl(twi_bgr_reg);
		reg_value &= ~(1 << 16);
		writel(reg_value, twi_bgr_reg);
	}
#else
	if (onoff) {
		/* de-assert */
		reg_value = readl(&ccm->twi_gate_reset);
		reg_value |= (1 << (16 + bus_num));
		writel(reg_value, &ccm->twi_gate_reset);

		/* gating clock pass */
		reg_value = readl(&ccm->twi_gate_reset);
		reg_value &= ~(1 << bus_num);
		writel(reg_value, &ccm->twi_gate_reset);
		__msdelay(1);
		reg_value |= (1 << bus_num);
		writel(reg_value, &ccm->twi_gate_reset);
	} else {
		/* gating clock mask */
		reg_value = readl(&ccm->twi_gate_reset);
		reg_value &= ~(1 << bus_num);
		writel(reg_value, &ccm->twi_gate_reset);

		/* assert */
		reg_value = readl(&ccm->twi_gate_reset);
		reg_value &= ~(1 << (16 + bus_num));
		writel(reg_value, &ccm->twi_gate_reset);
	}
#endif

}

static void sunxi_r_i2c_bus_setting(int bus_num, int onoff)
{
#ifdef SUNXI_RTWI_BRG_REG
	int reg_value = 0;
	int r_bus_num = bus_num - SUNXI_PHY_R_I2C0;
	if (onoff) {
		/* de-assert */
		reg_value = readl(SUNXI_RTWI_BRG_REG);
		reg_value &= ~(1 << (16 + r_bus_num));
		writel(reg_value, SUNXI_RTWI_BRG_REG);

		reg_value = readl(SUNXI_RTWI_BRG_REG);
		reg_value |= (1 << (16 + r_bus_num));
		writel(reg_value, SUNXI_RTWI_BRG_REG);

		/* gating clock pass */
		reg_value = readl(SUNXI_RTWI_BRG_REG);
		reg_value &= ~(1 << r_bus_num);
		writel(reg_value, SUNXI_RTWI_BRG_REG);
		__msdelay(1);
		reg_value |= (1 << r_bus_num);
		writel(reg_value, SUNXI_RTWI_BRG_REG);
	} else {
		/* gating clock mask */
		reg_value = readl(SUNXI_RTWI_BRG_REG);
		reg_value &= ~(1 << r_bus_num);
		writel(reg_value, SUNXI_RTWI_BRG_REG);

		/* assert */
		reg_value = readl(SUNXI_RTWI_BRG_REG);
		reg_value &= ~(1 << (16 + r_bus_num));
		writel(reg_value, SUNXI_RTWI_BRG_REG);
	}
#endif
}


static int sunxi_i2c_read(struct i2c_adapter *adap, uint8_t chip,
				uint32_t addr, int alen, uint8_t *buffer, int len)
{
	int  ret;

	ret = twi_start(adap->hwadapnr);
	if (ret) {
		I2C_ERR("twi_read start error\n");
		goto i2c_read_err_occur;
	}

	ret = twi_send_slave_addr(adap->hwadapnr, chip, I2C_WRITE);
	if (ret){
		I2C_ERR("twi_read send slave addr error!\n");
		goto i2c_read_err_occur;
	}

	ret = twi_send_addr(adap->hwadapnr, addr, alen);
	if (ret) {
		I2C_ERR("twi_read send addr error\n");
		goto i2c_read_err_occur;
	}

	ret = twi_restart(adap->hwadapnr);
	if (ret) {
		I2C_ERR("twi_restart error\n");
		goto i2c_read_err_occur;
	}

	ret = twi_send_slave_addr(adap->hwadapnr, chip, I2C_READ);
	if (ret) {
		I2C_ERR("twi_read send slave addr error!\n");
		goto i2c_read_err_occur;
	}

	ret = twi_get_data(adap->hwadapnr, buffer, len);
	if (ret) {
		I2C_ERR("twi_get_data error\n");
		goto i2c_read_err_occur;
	}

i2c_read_err_occur:
	twi_stop(adap->hwadapnr);
	return ret;

}

static int sunxi_i2c_write(struct i2c_adapter *adap, uint8_t chip,
				uint32_t addr, int alen, uint8_t *buffer, int len)
{
	int ret;

	ret = twi_start(adap->hwadapnr);
	if (ret) {
		I2C_ERR("twi_write start error\n");
		goto i2c_write_err_occur;
	}

	ret = twi_send_slave_addr(adap->hwadapnr, chip, I2C_WRITE);
	if (ret) {
		I2C_ERR("twi_write send slave addr error!\n");
		goto i2c_write_err_occur;
	}

	ret = twi_send_addr(adap->hwadapnr, addr, alen);
	if (ret) {
		I2C_ERR("twi_write send addr error!\n");
		goto i2c_write_err_occur;
	}

	ret = twi_send_data(adap->hwadapnr, buffer, len);
	if (ret) {
		I2C_ERR("twi_write send data error!\n");
		goto i2c_write_err_occur;
	}

i2c_write_err_occur:
	twi_stop(adap->hwadapnr);
	return ret;

}

static struct sunxi_twi_reg *sunxi_get_base(struct i2c_adapter *adap)
{
#ifdef SUNXI_R_TWI_BASE
	if (strstr(adap->name, "r_i2c") != NULL) {
		return (struct sunxi_twi_reg *)(SUNXI_R_TWI_BASE +
					((adap->hwadapnr - SUNXI_PHY_R_I2C0) * TWI_CONTROL_OFFSET));
	} else
#endif
	{
#if defined(CONFIG_MACH_SUN8IW11)
		switch (adap->hwadapnr) {
		case 4:
			return (struct sunxi_twi_reg *)SUNXI_TWI4_BASE;
		default:
			break;
		}
#endif
		return (struct sunxi_twi_reg *)(SUNXI_TWI0_BASE +
					(adap->hwadapnr* TWI_CONTROL_OFFSET));
	}
}

static uint sunxi_i2c_setspeed(struct i2c_adapter *adap, uint speed)
{
	i2c_set_clock(adap->hwadapnr, speed);
	adap->speed = speed;
	return 0;
}

static int sunxi_i2c_probe(struct i2c_adapter *adap, uint8_t chip)
{
	return 0;
}

void sunxi_i2c_init(struct i2c_adapter *adap, int speed, int slaveaddr)
{

	if (sunxi_i2c[adap->hwadapnr] != NULL) {
		I2C_DBG("[I2C-WRN]:i2c%d has been initialized\n", adap->hwadapnr);
		goto OUT;
	}

	sunxi_i2c[adap->hwadapnr] = sunxi_get_base(adap);

	if (strstr(adap->name, "r_i2c") != NULL) {
		sunxi_r_i2c_bus_setting(adap->hwadapnr, 1);
	} else {
		sunxi_i2c_bus_setting(adap->hwadapnr, 1);
	}
	sunxi_i2c_setspeed(adap, speed);
OUT:
	I2C_DBG("i2c%d info:%x(slaveaddr),%d(speed)\n", adap->hwadapnr,
	       adap->slaveaddr, adap->speed);
}

void sunxi_i2c_exit(int bus_num)
{
	sunxi_i2c_bus_setting(bus_num, 0);
	sunxi_i2c[bus_num] = NULL;
	I2C_DBG("exit");
	return ;
}

#ifdef CONFIG_I2C0_ENABLE
U_BOOT_I2C_ADAP_COMPLETE(sunxi_i2c0, sunxi_i2c_init, sunxi_i2c_probe,
			 sunxi_i2c_read, sunxi_i2c_write, sunxi_i2c_setspeed,
			 CONFIG_SYS_SUNXI_I2C0_SPEED,
			 CONFIG_SYS_SUNXI_I2C0_SLAVE,
			 SUNXI_PHY_I2C0)
#endif
#ifdef CONFIG_I2C1_ENABLE
U_BOOT_I2C_ADAP_COMPLETE(sunxi_i2c1, sunxi_i2c_init, sunxi_i2c_probe,
			 sunxi_i2c_read, sunxi_i2c_write, sunxi_i2c_setspeed,
			 CONFIG_SYS_SUNXI_I2C1_SPEED,
			 CONFIG_SYS_SUNXI_I2C1_SLAVE,
			 SUNXI_PHY_I2C1)
#endif
#ifdef CONFIG_I2C2_ENABLE
U_BOOT_I2C_ADAP_COMPLETE(sunxi_i2c2, sunxi_i2c_init, sunxi_i2c_probe,
			 sunxi_i2c_read, sunxi_i2c_write, sunxi_i2c_setspeed,
			 CONFIG_SYS_SUNXI_I2C2_SPEED,
			 CONFIG_SYS_SUNXI_I2C2_SLAVE,
			 SUNXI_PHY_I2C2)
#endif
#ifdef CONFIG_I2C3_ENABLE
U_BOOT_I2C_ADAP_COMPLETE(sunxi_i2c3, sunxi_i2c_init, sunxi_i2c_probe,
			 sunxi_i2c_read, sunxi_i2c_write, sunxi_i2c_setspeed,
			 CONFIG_SYS_SUNXI_I2C3_SPEED,
			 CONFIG_SYS_SUNXI_I2C3_SLAVE,
			 SUNXI_PHY_I2C3)
#endif
#ifdef CONFIG_I2C4_ENABLE
U_BOOT_I2C_ADAP_COMPLETE(sunxi_i2c4, sunxi_i2c_init, sunxi_i2c_probe,
			 sunxi_i2c_read, sunxi_i2c_write, sunxi_i2c_setspeed,
			 CONFIG_SYS_SUNXI_I2C4_SPEED,
			 CONFIG_SYS_SUNXI_I2C4_SLAVE,
			 SUNXI_PHY_I2C4)
#endif
#ifdef CONFIG_I2C5_ENABLE
U_BOOT_I2C_ADAP_COMPLETE(sunxi_i2c5, sunxi_i2c_init, sunxi_i2c_probe,
			 sunxi_i2c_read, sunxi_i2c_write, sunxi_i2c_setspeed,
			 CONFIG_SYS_SUNXI_I2C5_SPEED,
			 CONFIG_SYS_SUNXI_I2C5_SLAVE,
			 SUNXI_PHY_I2C5)
#endif

#ifdef CONFIG_R_I2C0_ENABLE
U_BOOT_I2C_ADAP_COMPLETE(sunxi_r_i2c0, sunxi_i2c_init, sunxi_i2c_probe,
			 sunxi_i2c_read, sunxi_i2c_write, sunxi_i2c_setspeed,
			 CONFIG_SYS_SUNXI_R_I2C0_SPEED,
			 CONFIG_SYS_SUNXI_R_I2C0_SLAVE,
			 SUNXI_PHY_R_I2C0)
#endif
#ifdef CONFIG_R_I2C1_ENABLE
U_BOOT_I2C_ADAP_COMPLETE(sunxi_r_i2c1, sunxi_i2c_init, sunxi_i2c_probe,
			 sunxi_i2c_read, sunxi_i2c_write, sunxi_i2c_setspeed,
			 CONFIG_SYS_SUNXI_R_I2C1_SPEED,
			 CONFIG_SYS_SUNXI_R_I2C1_SLAVE,
			 SUNXI_PHY_R_I2C1)
#endif


