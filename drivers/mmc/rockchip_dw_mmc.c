// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2013 Google, Inc
 */

#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <dt-structs.h>
#include <dwmmc.h>
#include <errno.h>
#include <log.h>
#include <mapmem.h>
#include <pwrseq.h>
#include <syscon.h>
#include <time.h>
#include <asm/gpio.h>
#include <asm/arch-rockchip/clock.h>
#include <asm/arch-rockchip/periph.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/kernel.h>

#define USRID_INTER_PHASE	0x20230001
#define SDMMC_TIMING_CON0	0x130
#define SDMMC_TIMING_CON1	0x134
#define ROCKCHIP_MMC_DELAY_SEL BIT(10)
#define ROCKCHIP_MMC_DEGREE_MASK 0x3
#define ROCKCHIP_MMC_DELAYNUM_OFFSET 2
#define ROCKCHIP_MMC_DELAYNUM_MASK (0xff << ROCKCHIP_MMC_DELAYNUM_OFFSET)
#define PSECS_PER_SEC 1000000000000LL
#define ROCKCHIP_MMC_DELAY_ELEMENT_PSEC 60
#define HIWORD_UPDATE(val, mask, shift) \
		((val) << (shift) | (mask) << ((shift) + 16))

struct rockchip_mmc_plat {
#if CONFIG_IS_ENABLED(OF_PLATDATA)
	struct dtd_rockchip_rk3288_dw_mshc dtplat;
#endif
	struct mmc_config cfg;
	struct mmc mmc;
};

struct rockchip_dwmmc_priv {
	struct clk clk;
	struct clk sample_clk;
	struct dwmci_host host;
	int fifo_depth;
	bool fifo_mode;
	int usrid;
	u32 default_phase;
	u32 minmax[2];
};

static uint rockchip_dwmmc_get_mmc_clk(struct dwmci_host *host, uint freq)
{
	struct udevice *dev = host->priv;
	struct rockchip_dwmmc_priv *priv = dev_get_priv(dev);
	int ret;

	/*
	 * The clock frequency chosen here affects CLKDIV in the dw_mmc core.
	 * That can be either 0 or 1, but it must be set to 1 for eMMC DDR52
	 * 8-bit mode.  It will be set to 0 for all other modes.
	 */
	if (host->mmc->selected_mode == MMC_DDR_52 && host->mmc->bus_width == 8)
		freq *= 2;

	ret = clk_set_rate(&priv->clk, freq);
	if (ret < 0) {
		debug("%s: err=%d\n", __func__, ret);
		return 0;
	}

	return freq;
}

#if CONFIG_IS_ENABLED(MMC_SUPPORTS_TUNING)
#define NUM_PHASES	32
#define TUNING_ITERATION_TO_PHASE(i, num_phases) \
	(DIV_ROUND_UP((i) * 360, num_phases))

/*
 * Each fine delay is between 44ps-77ps. Assume each fine delay is 60ps to
 * simplify calculations. So 45degs could be anywhere between 33deg and 57.8deg.
 */
static int rockchip_mmc_get_phase(struct dwmci_host *host, bool sample)
{
	struct udevice *dev = host->priv;
	struct rockchip_dwmmc_priv *priv = dev_get_priv(dev);
	unsigned long rate = clk_get_rate(&priv->clk) / 2;
	u32 raw_value;
	u16 degrees;
	u32 delay_num = 0;

	/* Constant signal, no measurable phase shift */
	if (!rate)
		return 0;

	if (sample)
		raw_value = dwmci_readl(host, SDMMC_TIMING_CON1) >> 1;
	else
		raw_value = dwmci_readl(host, SDMMC_TIMING_CON0) >> 1;

	degrees = (raw_value & ROCKCHIP_MMC_DEGREE_MASK) * 90;
	if (raw_value & ROCKCHIP_MMC_DELAY_SEL) {
		/* degrees/delaynum * 1000000 */
		unsigned long factor = (ROCKCHIP_MMC_DELAY_ELEMENT_PSEC / 10) *
			36 * (rate / 10000);

		delay_num = (raw_value & ROCKCHIP_MMC_DELAYNUM_MASK);
		delay_num >>= ROCKCHIP_MMC_DELAYNUM_OFFSET;
		degrees += DIV_ROUND_CLOSEST(delay_num * factor, 1000000);
	}
	return degrees % 360;
}

static int rockchip_mmc_set_phase(struct dwmci_host *host, bool sample, int degrees)
{
	struct udevice *dev = host->priv;
	struct rockchip_dwmmc_priv *priv = dev_get_priv(dev);
	unsigned long rate = clk_get_rate(&priv->clk) / 2;
	u8 nineties, remainder;
	u8 delay_num;
	u32 raw_value;
	u32 delay;

	/*
	 * The below calculation is based on the output clock from
	 * MMC host to the card, which expects the phase clock inherits
	 * the clock rate from its parent, namely the output clock
	 * provider of MMC host. However, things may go wrong if
	 * (1) It is orphan.
	 * (2) It is assigned to the wrong parent.
	 *
	 * This check help debug the case (1), which seems to be the
	 * most likely problem we often face and which makes it difficult
	 * for people to debug unstable mmc tuning results.
	 */
	if (!rate) {
		printf("%s: invalid clk rate\n", __func__);
		return -EINVAL;
	}

	nineties = degrees / 90;
	remainder = (degrees % 90);

	/*
	 * Due to the inexact nature of the "fine" delay, we might
	 * actually go non-monotonic.  We don't go _too_ monotonic
	 * though, so we should be OK.
	 *
	 * Convert to delay; do a little extra work to make sure we
	 * don't overflow 32-bit numbers.
	 */
	delay = 10000000; /* PSECS_PER_SEC / 10000 / 10 */
	delay *= remainder;
	delay = DIV_ROUND_CLOSEST(delay,
			(rate / 1000) * 36 *
				(ROCKCHIP_MMC_DELAY_ELEMENT_PSEC / 10));

	if (delay > 255)
		delay = 255;

	delay_num = (u8)delay;

	raw_value = delay_num ? ROCKCHIP_MMC_DELAY_SEL : 0;
	raw_value |= delay_num << ROCKCHIP_MMC_DELAYNUM_OFFSET;
	raw_value |= nineties;

	if (sample)
		dwmci_writel(host, SDMMC_TIMING_CON1, HIWORD_UPDATE(raw_value, 0x07ff, 1));
	else
		dwmci_writel(host, SDMMC_TIMING_CON0, HIWORD_UPDATE(raw_value, 0x07ff, 1));

	debug("set %s_phase(%d) delay_nums=%u actual_degrees=%d\n",
		sample ? "sample" : "drv", degrees, delay_num,
		rockchip_mmc_get_phase(host, sample)
	);

	return 0;
}

static int rockchip_dwmmc_execute_tuning(struct dwmci_host *host, u32 opcode)
{
	struct mmc *mmc = host->mmc;
	struct udevice *dev = host->priv;
	struct rockchip_dwmmc_priv *priv = dev_get_priv(dev);
	int ret = 0;
	int i, num_phases = NUM_PHASES;
	bool v, prev_v = 0, first_v;
	struct range_t {
		short start;
		short end; /* inclusive */
	};
	struct range_t ranges[NUM_PHASES / 2 + 1];
	unsigned int range_count = 0;
	int longest_range_len = -1;
	int longest_range = -1;
	int middle_phase, real_middle_phase;
	ulong ts;

	if (!(priv->sample_clk.dev) && priv->usrid != USRID_INTER_PHASE)
		return -EIO;
	ts = get_timer(0);

	/* Try each phase and extract good ranges */
	for (i = 0; i < num_phases; ) {
		/* Cannot guarantee any phases larger than 270 would work well */
		if (TUNING_ITERATION_TO_PHASE(i, num_phases) > 270)
			break;
		if (priv->usrid == USRID_INTER_PHASE)
			rockchip_mmc_set_phase(host, true, TUNING_ITERATION_TO_PHASE(i, num_phases));
		else
			clk_set_phase(&priv->sample_clk, TUNING_ITERATION_TO_PHASE(i, num_phases));

		v = !mmc_send_tuning(mmc, opcode);
		debug("3 Tuning phase is %d v = %x\n", TUNING_ITERATION_TO_PHASE(i, num_phases), v);
		if (i == 0)
			first_v = v;

		if ((!prev_v) && v) {
			range_count++;
			ranges[range_count - 1].start = i;
		}

		if (v)
			ranges[range_count - 1].end = i;
		i++;
		prev_v = v;
	}

	if (range_count == 0) {
		dev_warn(mmc->dev, "All phases bad!");
		return -EIO;
	}

	/* wrap around case, merge the end points */
	if ((range_count > 1) && first_v && v) {
		ranges[0].start = ranges[range_count - 1].start;
		range_count--;
	}

	/* Find the longest range */
	for (i = 0; i < range_count; i++) {
		int len = (ranges[i].end - ranges[i].start + 1);

		if (len < 0)
			len += num_phases;

		if (longest_range_len < len) {
			longest_range_len = len;
			longest_range = i;
		}

		debug("Good phase range %d-%d (%d len)\n",
			  TUNING_ITERATION_TO_PHASE(ranges[i].start, num_phases),
			  TUNING_ITERATION_TO_PHASE(ranges[i].end, num_phases),
			  len);
	}

	printf("Best phase range %d-%d (%d len)\n",
		   TUNING_ITERATION_TO_PHASE(ranges[longest_range].start, num_phases),
		   TUNING_ITERATION_TO_PHASE(ranges[longest_range].end, num_phases),
		   longest_range_len);

	middle_phase = ranges[longest_range].start + longest_range_len / 2;
	middle_phase %= num_phases;
	real_middle_phase = TUNING_ITERATION_TO_PHASE(middle_phase, num_phases);

	/*
	 * Since we cut out 270 ~ 360, the original algorithm
	 * still rolling ranges before and after 270 together
	 * in some corner cases, we should adjust it to avoid
	 * using any middle phase located between 270 and 360.
	 * By calculatiion, it happends due to the bad phases
	 * lay between 90 ~ 180. So others are all fine to chose.
	 * Pick 270 is a better choice in those cases. In case of
	 * bad phases exceed 180, the middle phase of rollback
	 * would be bigger than 315, so we chose 360.
	 */
	if (real_middle_phase > 270) {
		if (real_middle_phase < 315)
			real_middle_phase = 270;
		else
			real_middle_phase = 0;
	}

	printf("Successfully tuned phase to %d, used %ldms\n", real_middle_phase, get_timer(0) - ts);

	if (priv->usrid == USRID_INTER_PHASE)
		rockchip_mmc_set_phase(host, true, real_middle_phase);
	else
		clk_set_phase(&priv->sample_clk, real_middle_phase);

	return ret;
}
#else
static int rockchip_dwmmc_execute_tuning(struct dwmci_host *host, u32 opcode) { return 0; }
static int rockchip_mmc_set_phase(struct dwmci_host *host, bool sample, int degrees) { return 0; }
#endif

int dwmci_post_set_ios(struct udevice *dev)
{
	struct rockchip_dwmmc_priv *priv = dev_get_priv(dev);
	struct dwmci_host *host = &priv->host;
	struct mmc *mmc = mmc_get_mmc_dev(dev);

	/*
	 * HS200 tuning leaves the sample phase tuned for the high clock.
	 * At HS/legacy speeds (e.g. the pre-OS downgrade in mmc_deinit)
	 * that phase mismatches and cmd6/cmd8 fail with -EIO, so restore
	 * the default phase whenever we run at or below 52 MHz.
	 */
	if (mmc->clock <= 52000000) {
		if (priv->usrid == USRID_INTER_PHASE)
			rockchip_mmc_set_phase(host, true, priv->default_phase);
		else if (priv->sample_clk.dev)
			clk_set_phase(&priv->sample_clk, priv->default_phase);
	}

	return 0;
}

static int rockchip_dwmmc_of_to_plat(struct udevice *dev)
{
	struct rockchip_dwmmc_priv *priv = dev_get_priv(dev);
	struct dwmci_host *host = &priv->host;

	if (!CONFIG_IS_ENABLED(OF_REAL))
		return 0;

	host->name = dev->name;
	host->ioaddr = dev_read_addr_ptr(dev);
	host->buswidth = dev_read_u32_default(dev, "bus-width", 4);
	host->get_mmc_clk = rockchip_dwmmc_get_mmc_clk;
	host->priv = dev;

	/* use non-removeable as sdcard and emmc as judgement */
	if (dev_read_bool(dev, "non-removable"))
		host->dev_index = 0;
	else
		host->dev_index = 1;

	priv->fifo_depth = dev_read_u32_default(dev, "fifo-depth", 0);

	if (priv->fifo_depth < 0)
		return log_msg_ret("rkp", -EINVAL);
	priv->fifo_mode = dev_read_bool(dev, "fifo-mode");

#ifdef CONFIG_XPL_BUILD
	if (!priv->fifo_mode)
		priv->fifo_mode = dev_read_bool(dev, "u-boot,spl-fifo-mode");
#endif

	/*
	 * 'clock-freq-min-max' is deprecated
	 * (see https://github.com/torvalds/linux/commit/b023030f10573de738bbe8df63d43acab64c9f7b)
	 */
	if (dev_read_u32_array(dev, "clock-freq-min-max", priv->minmax, 2)) {
		int val = dev_read_u32_default(dev, "max-frequency", -EINVAL);

		if (val < 0)
			return log_msg_ret("rkc", val);

		priv->minmax[0] = 400000;  /* 400 kHz */
		priv->minmax[1] = val;
	} else {
		debug("%s: 'clock-freq-min-max' property was deprecated.\n",
		      __func__);
	}

	return 0;
}

static int rockchip_dwmmc_probe(struct udevice *dev)
{
	struct rockchip_mmc_plat *plat = dev_get_plat(dev);
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct rockchip_dwmmc_priv *priv = dev_get_priv(dev);
	struct dwmci_host *host = &priv->host;
	int ret;

#if CONFIG_IS_ENABLED(OF_PLATDATA)
	struct dtd_rockchip_rk3288_dw_mshc *dtplat = &plat->dtplat;

	host->name = dev->name;
	host->ioaddr = map_sysmem(dtplat->reg[0], dtplat->reg[1]);
	host->buswidth = dtplat->bus_width;
	host->get_mmc_clk = rockchip_dwmmc_get_mmc_clk;
	host->priv = dev;
	host->dev_index = 0;
	priv->fifo_depth = dtplat->fifo_depth;
	priv->fifo_mode = dtplat->u_boot_spl_fifo_mode;
	priv->minmax[0] = 400000;  /*  400 kHz */
	priv->minmax[1] = dtplat->max_frequency;

	ret = clk_get_by_phandle(dev, &dtplat->clocks[1], &priv->clk);
#else
	ret = clk_get_by_index(dev, 1, &priv->clk);
#endif
	if (ret < 0 && ret != -ENOSYS)
		return log_msg_ret("clk", ret);

	priv->usrid = dwmci_readl(host, DWMCI_USRID);
	host->execute_tuning = rockchip_dwmmc_execute_tuning;
	if (priv->usrid == USRID_INTER_PHASE)
		goto internal_phase;

	ret = clk_get_by_name(dev, "ciu-sample", &priv->sample_clk);
	if (ret < 0)
		debug("MMC: sample clock not found, not support hs200!\n");
internal_phase:
	host->fifo_depth = priv->fifo_depth;
	host->fifo_mode = priv->fifo_mode;

#if CONFIG_IS_ENABLED(MMC_PWRSEQ)
	/* Enable power if needed */
	ret = mmc_pwrseq_get_power(dev, &plat->cfg);
	if (!ret) {
		ret = pwrseq_set_power(plat->cfg.pwr_dev, true);
		if (ret)
			return ret;
	}
#endif
	dwmci_setup_cfg(&plat->cfg, host, priv->minmax[1], priv->minmax[0]);
	if (dev_read_bool(dev, "mmc-hs200-1_8v"))
		plat->cfg.host_caps |= MMC_MODE_HS200;
	priv->default_phase = dev_read_u32_default(dev, "default-sample-phase", 0);

	/* Set default sample phase for initialization */
	if (priv->usrid == USRID_INTER_PHASE) {
		if (rockchip_mmc_set_phase(host, true, priv->default_phase))
			debug("MMC: can not set default phase!\n");
	} else if (priv->sample_clk.dev) {
		if (clk_set_phase(&priv->sample_clk, priv->default_phase))
			debug("MMC: can not set default phase!\n");
	}

	host->mmc = &plat->mmc;
	host->mmc->priv = &priv->host;
	host->mmc->dev = dev;
	upriv->mmc = host->mmc;

	/* Hosts capable of 8-bit can also do 4 bits */
	if (host->buswidth == 8)
		plat->cfg.host_caps |= MMC_MODE_4BIT;

	return dwmci_probe(dev);
}

static int rockchip_dwmmc_bind(struct udevice *dev)
{
	struct rockchip_mmc_plat *plat = dev_get_plat(dev);

	return dwmci_bind(dev, &plat->mmc, &plat->cfg);
}

static const struct udevice_id rockchip_dwmmc_ids[] = {
	{ .compatible = "rockchip,rk2928-dw-mshc" },
	{ .compatible = "rockchip,rk3288-dw-mshc" },
	{ .compatible = "rockchip,rk3576-dw-mshc" },
	{ .compatible = "rockchip,rk182x-dw-mshc" },
	{ }
};

U_BOOT_DRIVER(rockchip_rk3288_dw_mshc) = {
	.name		= "rockchip_rk3288_dw_mshc",
	.id		= UCLASS_MMC,
	.of_match	= rockchip_dwmmc_ids,
	.of_to_plat = rockchip_dwmmc_of_to_plat,
	.ops		= &dm_dwmci_ops,
	.bind		= rockchip_dwmmc_bind,
	.probe		= rockchip_dwmmc_probe,
	.priv_auto	= sizeof(struct rockchip_dwmmc_priv),
	.plat_auto	= sizeof(struct rockchip_mmc_plat),
};

DM_DRIVER_ALIAS(rockchip_rk3288_dw_mshc, rockchip_rk2928_dw_mshc)
DM_DRIVER_ALIAS(rockchip_rk3288_dw_mshc, rockchip_rk3328_dw_mshc)
DM_DRIVER_ALIAS(rockchip_rk3288_dw_mshc, rockchip_rk3368_dw_mshc)
