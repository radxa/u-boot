// SPDX-License-Identifier: GPL-2.0+

#ifndef __SUNXI_MMC_CLK_ORIGIN_H__
#define __SUNXI_MMC_CLK_ORIGIN_H__

int sunxi_mmc_set_clk_origin_tm1(struct sunxi_mmc_priv *priv, unsigned int hz, unsigned int mod_hz, unsigned int *fid);
int sunxi_mmc_set_clk_origin_tm4(struct sunxi_mmc_priv *priv, unsigned int hz, unsigned int mod_hz, unsigned int *fid);
#endif /* __SUNXI_MMC_CLK_ORIGIN_H__ */
