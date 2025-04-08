// SPDX-License-Identifier: GPL-2.0+

#ifndef __SUNXI_MMC_HOST_TM1_H__
#define __SUNXI_MMC_HOST_TM1_H__

void sunxi_mmc_host_tm1_init(int sdc_no);
int mmc_config_delay_tm1(struct sunxi_mmc_priv *mmcpriv);

#endif /* __SUNXI_MMC_HOST_TM1_H__ */
