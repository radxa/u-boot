// SPDX-License-Identifier: GPL-2.0+
/*
 * eFuse driver for Rockchip devices
 *
 * Copyright 2017, Theobroma Systems Design und Consulting GmbH
 * Written by Philipp Tomsich <philipp.tomsich@theobroma-systems.com>
 */

#include <asm/io.h>
#include <command.h>
#include <display_options.h>
#include <dm.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <malloc.h>
#include <misc.h>

#define EFUSE_CTRL		0x0000
#define RK3036_A_SHIFT		8
#define RK3036_A_MASK		GENMASK(15, 8)
#define RK3036_ADDR(n)		((n) << RK3036_A_SHIFT)
#define RK3128_A_SHIFT		7
#define RK3128_A_MASK		GENMASK(15, 7)
#define RK3128_ADDR(n)		((n) << RK3128_A_SHIFT)
#define RK3288_A_SHIFT		6
#define RK3288_A_MASK		GENMASK(15, 6)
#define RK3288_ADDR(n)		((n) << RK3288_A_SHIFT)
#define RK3399_A_SHIFT		16
#define RK3399_A_MASK		GENMASK(25, 16)
#define RK3399_ADDR(n)		((n) << RK3399_A_SHIFT)
#define RK3399_STROBSFTSEL	BIT(9)
#define RK3399_RSB		BIT(7)
#define RK3399_PD		BIT(5)
#define EFUSE_PGENB		BIT(3)
#define EFUSE_LOAD		BIT(2)
#define EFUSE_STROBE		BIT(1)
#define EFUSE_CSB		BIT(0)
#define EFUSE_DOUT		0x0004
#define RK3328_INT_STATUS	0x0018
#define RK3328_INT_FINISH	BIT(0)
#define RK3328_DOUT		0x0020
#define RK3328_AUTO_CTRL	0x0024
#define RK3328_AUTO_RD		BIT(1)
#define RK3328_AUTO_ENB		BIT(0)

/* RV1103B efuse (new process EFUSE IP), auto read mode */
#define RV1103B_EFUSE_CFG_CTRL		0x0004
#define RV1103B_EFUSE_AUTO_REQ		0x000c
#define RV1103B_EFUSE_INT_CON		0x0014
#define RV1103B_EFUSE_INT_STATUS	0x0018
#define RV1103B_EFUSE_INT_CLR		0x0018
#define RV1103B_EFUSE_T_PGMEN_P		0x001c
#define RV1103B_EFUSE_T_RDEN_P		0x0020
#define RV1103B_EFUSE_T_E18V_P		0x0024
#define RV1103B_EFUSE_T_ADDR_P		0x0028
#define RV1103B_EFUSE_T_AEN_P		0x002c
#define RV1103B_EFUSE_T_PGMEN_R		0x0030
#define RV1103B_EFUSE_T_RDEN_R		0x0034
#define RV1103B_EFUSE_T_E18V_R		0x0038
#define RV1103B_EFUSE_T_ADDR_R		0x003c
#define RV1103B_EFUSE_T_AEN_R		0x0040
#define RV1103B_EFUSE_DOUT		0x0048

#define RV1103B_EFUSE_MODE_READ		0x1
#define RV1103B_EFUSE_ECC_CHECK_CD_S	BIT(2)
#define RV1103B_EFUSE_ADDR_SHIFT	8
#define RV1103B_EFUSE_ADDR_MASK		0xffff
#define RV1103B_EFUSE_REQ_START		((0x1 << 16) | 0x1)
#define RV1103B_EFUSE_INT_CON_DONE	((0x1 << 16) | 0x1)
#define RV1103B_EFUSE_INT_DONE		0x1
#define RV1103B_EFUSE_TIMEOUT		100000
/*
 * non-secure region in bytes, the secure one is served by the
 * rv1103b-secure-efuse driver
 */
#define RV1103B_EFUSE_NS_SIZE		0x80

struct rockchip_efuse_plat {
	void __iomem *base;
	unsigned long clk_gate_addr;
};

struct rockchip_efuse_data {
	int (*read)(struct udevice *dev, int offset, void *buf, int size);
	int offset;
	int size;
	int block_size;
};

#if defined(DEBUG)
static int dump_efuse(struct cmd_tbl *cmdtp, int flag,
		      int argc, char *const argv[])
{
	struct udevice *dev;
	u8 data[4];
	int ret, i;

	ret = uclass_get_device_by_driver(UCLASS_MISC,
					  DM_DRIVER_GET(rockchip_efuse), &dev);
	if (ret) {
		printf("%s: no misc-device found\n", __func__);
		return 0;
	}

	for (i = 0; true; i += sizeof(data)) {
		ret = misc_read(dev, i, &data, sizeof(data));
		if (ret <= 0)
			return 0;

		print_buffer(i, data, 1, sizeof(data), sizeof(data));
	}

	return 0;
}

U_BOOT_CMD(
	dump_efuse, 1, 1, dump_efuse,
	"Dump the content of the efuse",
	""
);
#endif

static int rockchip_rk3036_efuse_read(struct udevice *dev, int offset,
				      void *buf, int size)
{
	struct rockchip_efuse_plat *efuse = dev_get_plat(dev);
	u8 *buffer = buf;

	/* Switch to read mode */
	writel(EFUSE_LOAD, efuse->base + EFUSE_CTRL);
	udelay(2);

	while (size--) {
		clrsetbits_le32(efuse->base + EFUSE_CTRL, RK3036_A_MASK,
				RK3036_ADDR(offset++));
		udelay(2);
		setbits_le32(efuse->base + EFUSE_CTRL, EFUSE_STROBE);
		udelay(2);
		*buffer++ = (u8)(readl(efuse->base + EFUSE_DOUT) & 0xFF);
		clrbits_le32(efuse->base + EFUSE_CTRL, EFUSE_STROBE);
		udelay(2);
	}

	/* Switch to inactive mode */
	writel(0x0, efuse->base + EFUSE_CTRL);

	return 0;
}

static int rockchip_rk3128_efuse_read(struct udevice *dev, int offset,
				      void *buf, int size)
{
	struct rockchip_efuse_plat *efuse = dev_get_plat(dev);
	u8 *buffer = buf;

	/* Switch to read mode */
	writel(EFUSE_LOAD, efuse->base + EFUSE_CTRL);
	udelay(2);

	while (size--) {
		clrsetbits_le32(efuse->base + EFUSE_CTRL, RK3128_A_MASK,
				RK3128_ADDR(offset++));
		udelay(2);
		setbits_le32(efuse->base + EFUSE_CTRL, EFUSE_STROBE);
		udelay(2);
		*buffer++ = (u8)(readl(efuse->base + EFUSE_DOUT) & 0xFF);
		clrbits_le32(efuse->base + EFUSE_CTRL, EFUSE_STROBE);
		udelay(2);
	}

	/* Switch to inactive mode */
	writel(0x0, efuse->base + EFUSE_CTRL);

	return 0;
}

static int rockchip_rk3288_efuse_read(struct udevice *dev, int offset,
				      void *buf, int size)
{
	struct rockchip_efuse_plat *efuse = dev_get_plat(dev);
	u8 *buffer = buf;

	/* Switch to read mode */
	writel(EFUSE_CSB, efuse->base + EFUSE_CTRL);
	writel(EFUSE_LOAD | EFUSE_PGENB, efuse->base + EFUSE_CTRL);
	udelay(2);

	while (size--) {
		clrsetbits_le32(efuse->base + EFUSE_CTRL, RK3288_A_MASK,
				RK3288_ADDR(offset++));
		udelay(2);
		setbits_le32(efuse->base + EFUSE_CTRL, EFUSE_STROBE);
		udelay(2);
		*buffer++ = (u8)(readl(efuse->base + EFUSE_DOUT) & 0xFF);
		clrbits_le32(efuse->base + EFUSE_CTRL, EFUSE_STROBE);
		udelay(2);
	}

	/* Switch to standby mode */
	writel(EFUSE_CSB | EFUSE_PGENB, efuse->base + EFUSE_CTRL);

	return 0;
}

static int rockchip_rk3328_efuse_read(struct udevice *dev, int offset,
				      void *buf, int size)
{
	struct rockchip_efuse_plat *efuse = dev_get_plat(dev);
	u32 status, *buffer = buf;
	int ret;

	while (size--) {
		writel(RK3328_AUTO_RD | RK3328_AUTO_ENB | RK3399_ADDR(offset++),
		       efuse->base + RK3328_AUTO_CTRL);
		udelay(1);

		ret = readl_poll_sleep_timeout(efuse->base + RK3328_INT_STATUS,
			status, (status & RK3328_INT_FINISH), 1, 50);
		if (ret)
			return ret;

		*buffer++ = readl(efuse->base + RK3328_DOUT);
		writel(RK3328_INT_FINISH, efuse->base + RK3328_INT_STATUS);
	}

	return 0;
}

static int rockchip_rk3399_efuse_read(struct udevice *dev, int offset,
				      void *buf, int size)
{
	struct rockchip_efuse_plat *efuse = dev_get_plat(dev);
	u32 *buffer = buf;

	/* Switch to array read mode */
	writel(EFUSE_LOAD | EFUSE_PGENB | RK3399_STROBSFTSEL | RK3399_RSB,
	       efuse->base + EFUSE_CTRL);
	udelay(1);

	while (size--) {
		setbits_le32(efuse->base + EFUSE_CTRL,
			     EFUSE_STROBE | RK3399_ADDR(offset++));
		udelay(1);
		*buffer++ = readl(efuse->base + EFUSE_DOUT);
		clrbits_le32(efuse->base + EFUSE_CTRL, EFUSE_STROBE);
		udelay(1);
	}

	/* Switch to power-down mode */
	writel(RK3399_PD | EFUSE_CSB, efuse->base + EFUSE_CTRL);

	return 0;
}

static void rv1103b_efuse_timing_init(void __iomem *base)
{
	static bool init;

	if (init)
		return;

	/* auto mode timings for a 24MHz efuse clock */
	writel((0x036 << 16) | 0x1aa, base + RV1103B_EFUSE_T_PGMEN_P);
	writel((0x022 << 16) | 0x03b, base + RV1103B_EFUSE_T_RDEN_P);
	writel((0x01d << 16) | 0x1c4, base + RV1103B_EFUSE_T_E18V_P);
	writel((0x020 << 16) | 0x14b, base + RV1103B_EFUSE_T_ADDR_P);
	writel((0x039 << 16) | 0x147, base + RV1103B_EFUSE_T_AEN_P);

	writel((0x00a << 16) | 0x03b, base + RV1103B_EFUSE_T_PGMEN_R);
	writel((0x006 << 16) | 0x023, base + RV1103B_EFUSE_T_RDEN_R);
	writel((0x00a << 16) | 0x023, base + RV1103B_EFUSE_T_E18V_R);
	writel((0x008 << 16) | 0x01b, base + RV1103B_EFUSE_T_ADDR_R);
	writel((0x00a << 16) | 0x017, base + RV1103B_EFUSE_T_AEN_R);

	init = true;
}

static int rv1103b_efuse_wait_done(void __iomem *base)
{
	int delay = RV1103B_EFUSE_TIMEOUT;

	while (1) {
		if ((readl(base + RV1103B_EFUSE_INT_STATUS) & 0x3) == RV1103B_EFUSE_INT_DONE) {
			writel(RV1103B_EFUSE_INT_DONE, base + RV1103B_EFUSE_INT_CLR);
			return 0;
		}

		if (delay-- <= 0) {
			printf("%s: wait auto operation done timeout\n", __func__);
			return -ETIMEDOUT;
		}

		udelay(1);
	}
}

/*
 * The new process RV1103B replaces the OTPC IP with an EFUSE IP, its
 * non-secure instance reaches the efuse array bytes 0x00-0x7f with an
 * auto read engine working on halfword granularity. The secure region
 * starts at byte 0x80 and is served by the rv1103b-secure-efuse
 * driver.
 */
static int rockchip_rv1103b_efuse_read(struct udevice *dev, int offset,
				       void *buf, int size)
{
	struct rockchip_efuse_plat *plat = dev_get_plat(dev);
	u8 *buffer = buf;
	u16 half_val;
	u32 half, half_end, b, lo, hi, end;
	int ret;

	if (!size || offset < 0 || offset + size > RV1103B_EFUSE_NS_SIZE) {
		printf("%s: offset 0x%x size 0x%x out of non-secure region\n",
		       __func__, offset, size);
		return -EINVAL;
	}

	if (plat->clk_gate_addr)
		writel(0x1 << 16, (void __iomem *)plat->clk_gate_addr);

	rv1103b_efuse_timing_init(plat->base);

	half = offset / 2;
	half_end = (offset + size - 1) / 2;
	end = offset + size;
	for (; half <= half_end; half++) {
		writel(RV1103B_EFUSE_ECC_CHECK_CD_S | RV1103B_EFUSE_MODE_READ |
		       ((half & RV1103B_EFUSE_ADDR_MASK) << RV1103B_EFUSE_ADDR_SHIFT),
		       plat->base + RV1103B_EFUSE_CFG_CTRL);
		writel(RV1103B_EFUSE_INT_CON_DONE,
		       plat->base + RV1103B_EFUSE_INT_CON);
		writel(RV1103B_EFUSE_REQ_START,
		       plat->base + RV1103B_EFUSE_AUTO_REQ);

		ret = rv1103b_efuse_wait_done(plat->base);
		if (ret)
			break;

		half_val = readl(plat->base + RV1103B_EFUSE_DOUT) & 0xffff;

		lo = offset > half * 2 ? offset : half * 2;
		hi = end < half * 2 + 2 ? end : half * 2 + 2;
		for (b = lo; b < hi; b++)
			*buffer++ = (half_val >> ((b - half * 2) * 8)) & 0xff;
	}

	return ret;
}

static int rockchip_efuse_read(struct udevice *dev, int offset,
			       void *buf, int size)
{
	const struct rockchip_efuse_data *data =
		(void *)dev_get_driver_data(dev);
	u32 block_start, block_end, block_offset, blocks;
	u8 *buffer;
	int ret;

	if (offset < 0 || !buf || size <= 0 || offset + size > data->size)
		return -EINVAL;

	if (!data->read)
		return -ENOSYS;

	offset += data->offset;

	if (data->block_size <= 1) {
		ret = data->read(dev, offset, buf, size);
		goto done;
	}

	block_start = offset / data->block_size;
	block_offset = offset % data->block_size;
	block_end = DIV_ROUND_UP(offset + size, data->block_size);
	blocks = block_end - block_start;

	buffer = calloc(blocks, data->block_size);
	if (!buffer)
		return -ENOMEM;

	ret = data->read(dev, block_start, buffer, blocks);
	if (!ret)
		memcpy(buf, buffer + block_offset, size);

	free(buffer);

done:
	return ret < 0 ? ret : size;
}

static const struct misc_ops rockchip_efuse_ops = {
	.read = rockchip_efuse_read,
};

static int rockchip_efuse_of_to_plat(struct udevice *dev)
{
	struct rockchip_efuse_plat *plat = dev_get_plat(dev);

	plat->base = dev_read_addr_ptr(dev);
	plat->clk_gate_addr =
		dev_read_u32_default(dev, "clk_gate_addr", 0) & 0xffffffff;

	return 0;
}

static const struct rockchip_efuse_data rk3036_data = {
	.read = rockchip_rk3036_efuse_read,
	.size = 0x20,
};

static const struct rockchip_efuse_data rk3128_data = {
	.read = rockchip_rk3128_efuse_read,
	.size = 0x40,
};

static const struct rockchip_efuse_data rk3288_data = {
	.read = rockchip_rk3288_efuse_read,
	.size = 0x20,
};

static const struct rockchip_efuse_data rk3328_data = {
	.read = rockchip_rk3328_efuse_read,
	.offset = 0x60,
	.size = 0x20,
	.block_size = 4,
};

static const struct rockchip_efuse_data rk3399_data = {
	.read = rockchip_rk3399_efuse_read,
	.size = 0x80,
	.block_size = 4,
};

static const struct rockchip_efuse_data rv1103b_data = {
	.read = rockchip_rv1103b_efuse_read,
	.size = RV1103B_EFUSE_NS_SIZE,
};

static const struct udevice_id rockchip_efuse_ids[] = {
	{
		.compatible = "rockchip,rk3036-efuse",
		.data = (ulong)&rk3036_data,
	},
	{
		.compatible = "rockchip,rk3066a-efuse",
		.data = (ulong)&rk3288_data,
	},
	{
		.compatible = "rockchip,rk3128-efuse",
		.data = (ulong)&rk3128_data,
	},
	{
		.compatible = "rockchip,rk3188-efuse",
		.data = (ulong)&rk3288_data,
	},
	{
		.compatible = "rockchip,rk3228-efuse",
		.data = (ulong)&rk3288_data,
	},
	{
		.compatible = "rockchip,rk3288-efuse",
		.data = (ulong)&rk3288_data,
	},
	{
		.compatible = "rockchip,rk3328-efuse",
		.data = (ulong)&rk3328_data,
	},
	{
		.compatible = "rockchip,rk3399-efuse",
		.data = (ulong)&rk3399_data,
	},
	{
		.compatible = "rockchip,rv1103b-efuse",
		.data = (ulong)&rv1103b_data,
	},
	{
		.compatible = "rockchip,rv1106bs-efuse",
		.data = (ulong)&rv1103b_data,
	},
	{}
};

U_BOOT_DRIVER(rockchip_efuse) = {
	.name = "rockchip_efuse",
	.id = UCLASS_MISC,
	.of_match = rockchip_efuse_ids,
	.of_to_plat = rockchip_efuse_of_to_plat,
	.plat_auto = sizeof(struct rockchip_efuse_plat),
	.ops = &rockchip_efuse_ops,
};
