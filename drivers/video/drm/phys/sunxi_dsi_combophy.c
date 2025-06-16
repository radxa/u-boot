/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2023 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <common.h>
#include <dm.h>
#include <generic-phy.h>
#include <drm/drm_print.h>
#include <phy-mipi-dphy.h>
#include <asm/arch/clock.h>
#include <clk/clk.h>
#include <reset.h>

#include "sunxi_dsi_combophy_reg.h"
#include "../sunxi_drm_phy.h"
#include "../sunxi_drm_helper_funcs.h"

#define PHY_LEVEL 3

struct dsi_combophy_data {
	unsigned int id;
	struct combophy_config phy_config[PHY_LEVEL];
};

/* TODO:
     1. implement sunxi_dsi_dphy as a phy without disp2 lowlevel, for now, only clk enable work.
     2. remove dsi_combophy_data.id
 */

struct sunxi_dsi_combophy {
	uintptr_t reg_base;
	int usage_count;
	unsigned int id;
	struct sunxi_dphy_lcd dphy_lcd;
	struct phy *phy;
	struct clk *phy_gating;
	struct clk *phy_clk;
	struct mutex lock;
};

static int sunxi_dsi_combophy_clk_enable(struct sunxi_dsi_combophy *cphy)
{
	struct clk *clks[] = {cphy->phy_gating, cphy->phy_clk};

	return sunxi_clk_enable(clks, ARRAY_SIZE(clks));
}

static int sunxi_dsi_combophy_clk_disable(struct sunxi_dsi_combophy *cphy)
{
	struct clk *clks[] = {cphy->phy_gating, cphy->phy_clk};

	return sunxi_clk_disable(clks, ARRAY_SIZE(clks));
}

static int sunxi_dsi_combophy_power_on(struct phy *phy)
{
	struct sunxi_dsi_combophy *cphy = dev_get_priv(phy->dev);

	DRM_INFO("%s start\n", __func__);
	mutex_lock(&cphy->lock);

	//FIXME: remove it?
	if (cphy->usage_count == 0)
		sunxi_dsi_combophy_clk_enable(cphy);

	cphy->usage_count++;

	mutex_unlock(&cphy->lock);
	DRM_INFO("%s end\n", __func__);
	return 0;
}

static int sunxi_dsi_combophy_power_off(struct phy *phy)
{
	struct sunxi_dsi_combophy *cphy = dev_get_priv(phy->dev);

	mutex_lock(&cphy->lock);

	//FIXME: remove it?
	if (cphy->usage_count == 1)
		sunxi_dsi_combophy_clk_disable(cphy);

	cphy->usage_count--;

	mutex_unlock(&cphy->lock);

	return 0;
}

static int sunxi_dsi_combophy_param_sel(struct sunxi_dsi_combophy *cphy, struct sunxi_drm_phy_cfg *opts)
{
	int ret, i;
	struct combophy_config *combophy_cfg;

	combophy_cfg = cphy->dphy_lcd.phy_config;

	if (!opts->mipi_dphy.hs_clk_rate) {
		DRM_ERROR("[PHY] Unset hs_clk_rate.\n");
		return -1;
	}

	for (i = 0; i < PHY_LEVEL; i++) {
		if ((!combophy_cfg->freq_lvl.lvl_min) || (!combophy_cfg->freq_lvl.lvl_max)) {
			DRM_WARN("[PHY] phy_config:%d Unset the lvl,Use the default cfg.\n", i);
			return -1;
		}
		ret = clamp(opts->mipi_dphy.hs_clk_rate, combophy_cfg->freq_lvl.lvl_min,
					combophy_cfg->freq_lvl.lvl_max);
		if (ret != opts->mipi_dphy.hs_clk_rate) {
			DRM_WARN("[PHY] hs_clk_rate not matched.\n");
			combophy_cfg++;
			continue;
		} else {
			cphy->dphy_lcd.phy_config = combophy_cfg;
			DRM_INFO("[PHY] Matched a suitable combophy configuration.\n");
			return 0;
		}
	}

	DRM_WARN("[PHY] No suitable combophy configuration matched, use the default cfg.\n");

	return 0;
}

static int sunxi_dsi_combophy_configure(struct phy *phy, void *params)
{
	struct sunxi_drm_phy_cfg *config = (struct sunxi_drm_phy_cfg *)params;
	struct sunxi_dsi_combophy *cphy = dev_get_priv(phy->dev);

	DRM_INFO("[PHY] %s start\n", __FUNCTION__);
	if (config->mode == PHY_MODE_MIPI_DPHY) {
		if (config->phy_print)
			return sunxi_dsi_combophy_print(&cphy->dphy_lcd);
		sunxi_dsi_combophy_param_sel(cphy, config);
		sunxi_dsi_combophy_set_dsi_mode(&cphy->dphy_lcd, config->submode);
	} else if (config->mode == PHY_MODE_LVDS) {
		sunxi_dsi_combophy_set_lvds_mode(&cphy->dphy_lcd, config->submode ? true : false);
	} else {
		DRM_ERROR("Invalid phy mode :%d\n", config->mode);
	}
	sunxi_dsi_combophy_configure_dsi(&cphy->dphy_lcd, config->mode, &config->mipi_dphy);

	return 0;
}

static int sunxi_dsi_combophy_init(struct phy *phy)
{
	struct sunxi_dsi_combophy *cphy = dev_get_priv(phy->dev);
	phy->id = cphy->id;
	cphy->phy = phy;
	DRM_INFO("%s finish\n", __func__);
	return 0;
}

static int displl_set_spread_spectrum(struct phy *phy, int precent)
{
	struct sunxi_dsi_combophy *cphy = dev_get_priv(phy->dev);
	u32 dcxo_rate = 24000000;
#if IS_ENABLED(CONFIG_MACH_SUN60IW2)
	dcxo_rate = get_hosc();
#endif
	DRM_INFO("[PHY] %s start\n", __FUNCTION__);
	phy_displl_ssc(&cphy->dphy_lcd, precent, dcxo_rate);

	return 0;
}

static const struct phy_ops sunxi_dsi_combophy_ops = {
	.init = sunxi_dsi_combophy_init,
	.power_on = sunxi_dsi_combophy_power_on,
	.power_off = sunxi_dsi_combophy_power_off,
	.configure = sunxi_dsi_combophy_configure,
	.set_speed = displl_set_spread_spectrum,
};

static int sunxi_dsi_combophy_probe(struct udevice *dev)
{
	struct sunxi_dsi_combophy *cphy = dev_get_priv(dev);
	const struct dsi_combophy_data *cphy_data = (struct dsi_combophy_data *)dev_get_driver_data(dev);

	DRM_INFO("[PHY] %s start\n", __FUNCTION__);


	cphy->reg_base = (uintptr_t)dev_read_addr_ptr(dev);
	if (!cphy->reg_base) {
		DRM_ERROR("unable to map dsi combo phy registers\n");
		return -EINVAL;
	}

	cphy->phy_gating = clk_get_by_name(dev, "phy_gating_clk");
	if (IS_ERR_OR_NULL(cphy->phy_gating)) {
		DRM_INFO("Maybe dsi clk gating is not need for dsi_combophy.\n");
	}

	cphy->phy_clk = clk_get_by_name(dev, "phy_clk");
	if (IS_ERR_OR_NULL(cphy->phy_clk)) {
		DRM_INFO("Maybe dsi phy clk is not need for dsi_combophy.\n");
	}

	cphy->id = cphy_data->id;
	cphy->dphy_lcd.dphy_index = cphy->id;
	cphy->dphy_lcd.phy_config = (struct combophy_config *)&cphy_data->phy_config;
	sunxi_dsi_combo_phy_set_reg_base(&cphy->dphy_lcd, cphy->reg_base);

	cphy->usage_count = 0;
	mutex_init(&cphy->lock);

	DRM_INFO("[PHY]%s finish\n", __FUNCTION__);

	return 0;
}

static const struct dsi_combophy_data phy0_data = {
	.id = 0,
	.phy_config[0] = {
		.dphy_tx_time0 = {
			.bits = {
				.hs_trail_set = 4,
				.hs_pre_set = 6,
				.lpx_tm_set = 0x0e,
			},
		},
		.dphy_ana0 = {
			.bits = {
				.reg_lptx_setr = 7,
				.reg_lptx_setc = 7,
				.reg_preemph3 = 0,
				.reg_preemph2 = 0,
				.reg_preemph1 = 0,
				.reg_preemph0 = 0,
			},
		},
		.dphy_ana4 = {
			.bits = {
				.reg_soft_rcal = 0x18,
				.en_soft_rcal = 1,
				.on_rescal = 0,
				.en_rescal = 0,
				.reg_vlv_set = 5,
				.reg_vlptx_set = 3,
				.reg_vtt_set = 6,
				.reg_vres_set = 3,
				.reg_vref_source = 0,
				.reg_ib = 4,
				.reg_comtest = 0,
				.en_comtest = 0,
				.en_mipi = 1,
			},
		},
		.combo_phy_reg0 = {
			.bits = {
				.en_cp = 1,
				.en_comboldo = 1,
				.en_lvds = 0,
				.en_mipi = 1,
				.en_test_0p8 = 0,
				.en_test_comboldo = 0,
			},
		},
		.combo_phy_reg1 = {
			.dwval = 0x43,
		},
	},
	.phy_config[1] = {},
};

static const struct dsi_combophy_data phy1_data = {
	.id = 1,
	.phy_config[0] = {
		.dphy_tx_time0 = {
			.bits = {
				.hs_trail_set = 4,
				.hs_pre_set = 6,
				.lpx_tm_set = 0x0e,
			},
		},
		.dphy_ana0 = {
			.bits = {
				.reg_lptx_setr = 7,
				.reg_lptx_setc = 7,
				.reg_preemph3 = 0,
				.reg_preemph2 = 0,
				.reg_preemph1 = 0,
				.reg_preemph0 = 0,
			},
		},
		.dphy_ana4 = {
			.bits = {
				.reg_soft_rcal = 0x18,
				.en_soft_rcal = 1,
				.on_rescal = 0,
				.en_rescal = 0,
				.reg_vlv_set = 5,
				.reg_vlptx_set = 3,
				.reg_vtt_set = 6,
				.reg_vres_set = 3,
				.reg_vref_source = 0,
				.reg_ib = 4,
				.reg_comtest = 0,
				.en_comtest = 0,
				.en_mipi = 1,
			},
		},
		.combo_phy_reg0 = {
			.bits = {
				.en_cp = 1,
				.en_comboldo = 1,
				.en_lvds = 0,
				.en_mipi = 1,
				.en_test_0p8 = 0,
				.en_test_comboldo = 0,
			},
		},
		.combo_phy_reg1 = {
			.dwval = 0x43,
		},
	},
	.phy_config[1] = {},
};

static const struct dsi_combophy_data sun55iw6_data0 = {
	.id = 0,
	.phy_config[0] = {
		.dphy_tx_time0 = {
			.bits = {
				.hs_trail_set = 4,
				.hs_pre_set = 6,
				.lpx_tm_set = 0x0e,
			},
		},
		.dphy_ana0 = {
			.bits = {
				.reg_lptx_setr = 7,
				.reg_lptx_setc = 7,
				.reg_preemph3 = 0,
				.reg_preemph2 = 0,
				.reg_preemph1 = 0,
				.reg_preemph0 = 0,
			},
		},
		.dphy_ana4 = {
			.bits = {
				.reg_soft_rcal = 0x18,
				.en_soft_rcal = 1,
				.on_rescal = 0,
				.en_rescal = 0,
				.reg_vlv_set = 5,
				.reg_vlptx_set = 3,
				.reg_vtt_set = 6,
				.reg_vres_set = 3,
				.reg_vref_source = 0,
				.reg_ib = 4,
				.reg_comtest = 0,
				.en_comtest = 0,
				.en_mipi = 1,
			},
		},
		.combo_phy_reg0 = {
			.bits = {
				.en_cp = 1,
				.en_comboldo = 1,
				.en_lvds = 0,
				.en_mipi = 1,
				.en_test_0p8 = 0,
				.en_test_comboldo = 0,
			},
		},
		.combo_phy_reg1 = {
			.dwval = 0x53,
		},
	},
	.phy_config[1] = {},
};

static const struct dsi_combophy_data sun60iw2_data0 = {
	.id = 0,
	.phy_config[0] = {
		.dphy_tx_time0 = {
			.bits = {
				.hs_trail_set = 9,
				.hs_pre_set = 6,
				.lpx_tm_set = 0x0e,
			},
		},
		.dphy_ana0 = {
			.bits = {
				.reg_lptx_setr = 7,
				.reg_lptx_setc = 7,
				.reg_preemph3 = 7,
				.reg_preemph2 = 7,
				.reg_preemph1 = 7,
				.reg_preemph0 = 7,
			},
		},
		.dphy_ana4 = {
			.bits = {
				.reg_soft_rcal = 0x1f,
				.en_soft_rcal = 1,
				.on_rescal = 1,
				.en_rescal = 0,
				.reg_vlv_set = 2,
				.reg_vlptx_set = 3,
				.reg_vtt_set = 4,
				.reg_vres_set = 3,
				.reg_vref_source = 1,
				.reg_ib = 4,
				.reg_comtest = 2,
				.en_comtest = 1,
				.en_mipi = 1,
			},
		},
		.combo_phy_reg0 = {
			.bits = {
				.en_cp = 1,
				.en_comboldo = 1,
				.en_lvds = 0,
				.en_mipi = 1,
				.en_test_0p8 = 0,
				.en_test_comboldo = 0,
			},
		},
		.combo_phy_reg1 = {
			.dwval = 0x63,
		},
	},
	.phy_config[1] = {},
};

static const struct dsi_combophy_data sun60iw2_data1 = {
	.id = 1,
	.phy_config[0] = {
		.dphy_tx_time0 = {
			.bits = {
				.hs_trail_set = 9,
				.hs_pre_set = 6,
				.lpx_tm_set = 0x0e,
			},
		},
		.dphy_ana0 = {
			.bits = {
				.reg_lptx_setr = 7,
				.reg_lptx_setc = 7,
				.reg_preemph3 = 7,
				.reg_preemph2 = 7,
				.reg_preemph1 = 7,
				.reg_preemph0 = 7,
			},
		},
		.dphy_ana4 = {
			.bits = {
				.reg_soft_rcal = 0x1f,
				.en_soft_rcal = 1,
				.on_rescal = 1,
				.en_rescal = 0,
				.reg_vlv_set = 2,
				.reg_vlptx_set = 3,
				.reg_vtt_set = 4,
				.reg_vres_set = 3,
				.reg_vref_source = 1,
				.reg_ib = 4,
				.reg_comtest = 2,
				.en_comtest = 1,
				.en_mipi = 1,
			},
		},
		.combo_phy_reg0 = {
			.bits = {
				.en_cp = 1,
				.en_comboldo = 1,
				.en_lvds = 0,
				.en_mipi = 1,
				.en_test_0p8 = 0,
				.en_test_comboldo = 0,
			},
		},
		.combo_phy_reg1 = {
			.dwval = 0x63,
		},
	},
	.phy_config[1] = {},
};

static const struct dsi_combophy_data sun65iw1_data0 = {
	.id = 0,
	.phy_config[0] = {
		.freq_lvl = {
			.lvl_min = 80000000,	/* 80Mhz */
			.lvl_max = 500000000,	/* 500Mhz */
		},
		.dphy_tx_time0 = {
			.bits = {
				.hs_trail_set = 5,
				.hs_pre_set = 6,
				.lpx_tm_set = 0x0e,
			},
		},
		.dphy_ana0 = {
			.bits = {
				.reg_lptx_setr = 7,
				.reg_lptx_setc = 7,
				.reg_preemph3 = 0,
				.reg_preemph2 = 0,
				.reg_preemph1 = 0,
				.reg_preemph0 = 0,
			},
		},
		.dphy_ana4 = {
			.bits = {
				.reg_soft_rcal = 0,
				.reg_vlv_set = 4,
				.reg_vlptx_set = 3,
				.reg_vtt_set = 3,
				.reg_vres_set = 3,
				.reg_vref_source = 0,
				.reg_ib = 4,
				.reg_comtest = 0,
				.en_comtest = 0,
				.en_mipi = 1,
			},
		},
		.combo_phy_reg0 = {
			.bits = {
				.en_cp = 1,
				.en_comboldo = 1,
				.en_lvds = 0,
				.en_mipi = 1,
				.en_test_0p8 = 0,
				.en_test_comboldo = 0,
			},
		},
		.combo_phy_reg1 = {
			.dwval = 0x53,
		},
	},
	.phy_config[1] = {
		.freq_lvl = {
			.lvl_min = 500000000,	/* 500Mhz */
			.lvl_max = 1000000000,	/* 1Ghz */
		},
		.dphy_tx_time0 = {
			.bits = {
				.hs_trail_set = 0x08,
				.hs_pre_set = 6,
				.lpx_tm_set = 0x0e,
			},
		},
		.dphy_ana0 = {
			.bits = {
				.reg_lptx_setr = 7,
				.reg_lptx_setc = 7,
				.reg_preemph3 = 0,
				.reg_preemph2 = 0,
				.reg_preemph1 = 0,
				.reg_preemph0 = 0,
			},
		},
		.dphy_ana4 = {
			.bits = {
				.reg_soft_rcal = 0,
				.reg_vlv_set = 4,
				.reg_vlptx_set = 3,
				.reg_vtt_set = 2,
				.reg_vres_set = 3,
				.reg_vref_source = 0,
				.reg_ib = 4,
				.reg_comtest = 0,
				.en_comtest = 0,
				.en_mipi = 1,
			},
		},
		.combo_phy_reg0 = {
			.bits = {
				.en_cp = 1,
				.en_comboldo = 1,
				.en_lvds = 0,
				.en_mipi = 1,
				.en_test_0p8 = 0,
				.en_test_comboldo = 0,
			},
		},
		.combo_phy_reg1 = {
			.dwval = 0x53,
		},
	},
	.phy_config[2] = {
		.freq_lvl = {
			.lvl_min = 1000000000,	/* 1Ghz */
			.lvl_max = 1500000000,	/* 1.5Ghz */
		},
		.dphy_tx_time0 = {
			.bits = {
				.hs_trail_set = 0x0a,
				.hs_pre_set = 6,
				.lpx_tm_set = 0x0e,
			},
		},
		.dphy_ana0 = {
			.bits = {
				.reg_lptx_setr = 7,
				.reg_lptx_setc = 7,
				.reg_preemph3 = 0,
				.reg_preemph2 = 0,
				.reg_preemph1 = 0,
				.reg_preemph0 = 0,
			},
		},
		.dphy_ana4 = {
			.bits = {
				.reg_soft_rcal = 0,
				.reg_vlv_set = 4,
				.reg_vlptx_set = 3,
				.reg_vtt_set = 2,
				.reg_vres_set = 3,
				.reg_vref_source = 0,
				.reg_ib = 4,
				.reg_comtest = 0,
				.en_comtest = 0,
				.en_mipi = 1,
			},
		},
		.combo_phy_reg0 = {
			.bits = {
				.en_cp = 1,
				.en_comboldo = 1,
				.en_lvds = 0,
				.en_mipi = 1,
				.en_test_0p8 = 0,
				.en_test_comboldo = 0,
			},
		},
		.combo_phy_reg1 = {
			.dwval = 0x53,
		},
	},
};

static const struct udevice_id sunxi_dsi_combophy_of_table[] = {
	{ .compatible = "allwinner,sunxi-dsi-combo-phy0", .data = (ulong)&phy0_data },
	{ .compatible = "allwinner,sunxi-dsi-combo-phy1", .data = (ulong)&phy1_data },
	{ .compatible = "allwinner,sunxi-dsi-combo-phy0,sun55iw6", .data = (ulong)&sun55iw6_data0 },
	{ .compatible = "allwinner,sunxi-dsi-combo-phy0,sun60iw2", .data = (ulong)&sun60iw2_data0 },
	{ .compatible = "allwinner,sunxi-dsi-combo-phy1,sun60iw2", .data = (ulong)&sun60iw2_data1 },
	{ .compatible = "allwinner,sunxi-dsi-combo-phy0,sun65iw1", .data = (ulong)&sun65iw1_data0 },
	{}
};

U_BOOT_DRIVER(sunxi_dsi_combo_phy) = {
	.name = "sunxi_dsi_combo_phy",
	.id = UCLASS_PHY,
	.of_match = sunxi_dsi_combophy_of_table,
	.probe = sunxi_dsi_combophy_probe,
	.ops = &sunxi_dsi_combophy_ops,
	.priv_auto_alloc_size = sizeof(struct sunxi_dsi_combophy),
};
