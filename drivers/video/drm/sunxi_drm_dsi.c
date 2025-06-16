/*
 * sunxi_drm_dsi/sunxi_drm_dsi.c
 *
 * Copyright (c) 2007-2024 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <asm/arch/gic.h>
#include <drm/drm_modes.h>
#include <generic-phy.h>
#include <tcon_lcd.h>
#include <sunxi_device/sunxi_tcon.h>
#include <drm/drm_print.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_dsc_helper.h>
#include "drm_mipi_dsi.h"
#include <asm/arch-sunxi/efuse.h>

#include "sunxi_drm_phy.h"
#include "sunxi_drm_crtc.h"
#include "sunxi_drm_connector.h"
#include "sunxi_drm_drv.h"
#include "sunxi_drm_helper_funcs.h"

#if IS_ENABLED(CONFIG_MACH_SUN55IW3) || IS_ENABLED(CONFIG_MACH_SUN55IW6) \
	|| IS_ENABLED(CONFIG_MACH_SUN60IW2) || IS_ENABLED(CONFIG_MACH_SUN65IW1)
#define DSI_DISPLL_CLK
#endif

struct dsi_data {
	int id;
};
struct sunxi_drm_dsi {
	struct sunxi_drm_connector connector;
	struct udevice *tcon_dev;
	struct udevice *dev;
	/*struct mipi_dsi_host host;*/
	struct drm_display_mode mode;
	struct disp_dsi_para dsi_para;
	unsigned int tcon_id;
	unsigned int tcon_top_id;
	/*struct drm_panel *panel;*/
	bool bound;
	struct sunxi_drm_dsi *master;
	struct sunxi_drm_dsi *slave;
	struct phy *phy;
	struct sunxi_drm_phy_cfg phy_opts;

	uintptr_t reg_base;
	uintptr_t dsc_base;
	struct drm_dsc_config *dsc;
	const struct dsi_data *dsi_data;
	struct sunxi_dsi_lcd dsi_lcd;

	u32 dsi_id;
	u32 panel_out_reg;
	u32 enable;
	interrupt_handler_t *irq_handler;
	void *irq_data;
	u32 irq_no;
	dev_t devid;

	struct clk *clk_combphy;
	struct clk *dsc_clk;
	struct clk *clk;
	unsigned long mode_flags;

	unsigned long hs_clk_rate;
	unsigned long ls_clk_rate;
	bool displl_clk;
	unsigned int pll_ss_permille;

	struct mutex mlock;
};

static const struct dsi_data dsi0_data = {
	.id = 0,
};

static const struct dsi_data dsi1_data = {
	.id = 1,
};

static const struct udevice_id sunxi_drm_dsi_match[] = {
	{ .compatible = "allwinner,dsi0", .data = (ulong)&dsi0_data },
	{ .compatible = "allwinner,dsi1", .data = (ulong)&dsi1_data },
	{},
};


static int sunxi_drm_dsi_host_attach(struct mipi_dsi_host *host,
				struct mipi_dsi_device *device)
{
	struct sunxi_drm_dsi *dsi = dev_get_priv(host->dev);
	/*struct drm_panel *panel = of_drm_find_panel(device->dev.of_node);*/

	DRM_INFO("[DSI]%s start\n", __FUNCTION__);

	/*dsi->panel = panel;*/
	dsi->panel_out_reg = device->out_reg;
	dsi->dsi_para.dsi_div = 6;
	dsi->dsi_para.lanes = device->lanes;
	dsi->dsi_para.channel = device->channel;
	dsi->dsi_para.format = device->format;
	dsi->dsi_para.mode_flags = device->mode_flags;
	dsi->dsi_para.hs_rate = device->hs_rate;
	dsi->dsi_para.lp_rate = device->lp_rate;
	dsi->pll_ss_permille = device->pll_ss_permille;
	if (device->dsc)
		dsi->dsc = device->dsc;

	DRM_INFO("[DSI]%s finish\n", __FUNCTION__);
	return 0;
}

static int sunxi_drm_dsi_host_detach(struct mipi_dsi_host *host,
				struct mipi_dsi_device *device)
{
	struct sunxi_drm_dsi *dsi = dev_get_priv(host->dev);
	/*dsi->panel = NULL;*/
	dsi->dsi_para.lanes = 0;
	dsi->dsi_para.channel = 0;
	dsi->dsi_para.format = 0;
	dsi->dsi_para.mode_flags = 0;
	dsi->dsi_para.hs_rate = 0;
	dsi->dsi_para.lp_rate = 0;
	memset(&dsi->dsi_para.timings, 0, sizeof(struct disp_video_timings));

	return 0;
}

static s32 sunxi_dsi_write_para(struct sunxi_drm_dsi *dsi, const struct mipi_dsi_msg *msg)
{
	struct mipi_dsi_packet packet;
	u32 ecc, crc, para_num;
	u8 *para = NULL;
	int ret = 0;

	/* create a packet to the DSI protocol */
	ret = mipi_dsi_create_packet(&packet, msg);
	if (ret) {
		DRM_ERROR("failed to create packet\n");
		return ret;
	}

	para = kmalloc(packet.size + 2, GFP_ATOMIC);
	if (!para) {
	//	printk("%s %s %s :kmalloc fail\n", __FILE__, __func__, __LINE__);
		return -1;
	}
	ecc = packet.header[0] | (packet.header[1] << 8) | (packet.header[2] << 16);
	para[0] = packet.header[0];
	para[1] = packet.header[1];
	para[2] = packet.header[2];
	para[3] = dsi_ecc_pro(ecc);
	para_num = 4;

	if (packet.payload_length) {
		memcpy(para + 4, packet.payload, packet.payload_length);
		crc = dsi_crc_pro((u8 *)packet.payload, packet.payload_length + 1);
		para[packet.size] = (crc >> 0) & 0xff;
		para[packet.size + 1] = (crc >> 8) & 0xff;
		para_num = packet.size + 2;
	}
	dsi_dcs_wr(&dsi->dsi_lcd, para, para_num);

	kfree(para);
	para = NULL;

	return 0;
}

static s32 sunxi_dsi_read_para(struct sunxi_drm_dsi *dsi, const struct mipi_dsi_msg *msg)
{
	s32 ret;
	struct mipi_dsi_msg max_pkt_size_msg;
	int i;
	u32 rx_cntr = msg->rx_len, rx_curr;
	u8 tx[2] = { 22 & 0xff, 22 >> 8 };
	u8 rx_bf[64] = {0}, *rx_buf = msg->rx_buf;

	max_pkt_size_msg.channel = msg->channel;
	max_pkt_size_msg.type = MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE;
	max_pkt_size_msg.tx_len = sizeof(tx);
	max_pkt_size_msg.tx_buf = tx;
	sunxi_dsi_write_para(dsi, &max_pkt_size_msg);

	while (rx_cntr) {
		if (rx_cntr >= 22)
			rx_curr = 22;
		else {
			rx_curr = rx_cntr;
			tx[0] = rx_curr & 0xff;
			tx[1] = rx_curr >> 8;
			max_pkt_size_msg.tx_buf = tx;
			sunxi_dsi_write_para(dsi, &max_pkt_size_msg);
		}
		rx_cntr -= rx_curr;

		sunxi_dsi_write_para(dsi, msg);
		ret = dsi_dcs_rd(&dsi->dsi_lcd, rx_bf, rx_curr);
		for (i = 0; i < rx_curr; i++) {
			*rx_buf = 0x00;
			*rx_buf |= *(rx_bf + i);
			rx_buf++;
		}
	}

	return ret;
}

static ssize_t sunxi_drm_dsi_transfer(struct sunxi_drm_dsi *dsi,
				const struct mipi_dsi_msg *msg)
{
	int ret = 0;
	bool is_read = (msg->rx_buf && msg->rx_len);

	if (is_read) {
		ret = sunxi_dsi_read_para(dsi, msg);
		return ret;
	} else
		sunxi_dsi_write_para(dsi, msg);

	if (dsi->slave)
		sunxi_drm_dsi_transfer(dsi->slave, msg);

	return msg->tx_len;
}


static ssize_t sunxi_drm_dsi_host_transfer(struct mipi_dsi_host *host,
				const struct mipi_dsi_msg *msg)
{
	struct sunxi_drm_dsi *dsi = dev_get_priv(host->dev);

	return sunxi_drm_dsi_transfer(dsi, msg);
}


static const struct mipi_dsi_host_ops sunxi_drm_dsi_host_ops = {
	.attach = sunxi_drm_dsi_host_attach,
	.detach = sunxi_drm_dsi_host_detach,
	.transfer = sunxi_drm_dsi_host_transfer,
};


static void sunxi_dsi_irq_event_proc(void *parg)
{
	struct sunxi_drm_dsi *dsi = parg;
	/* NOTE: only for dsi40 */
	dsi_irq_query(&dsi->dsi_lcd, DSI_IRQ_VIDEO_VBLK);

	return dsi->irq_handler(dsi->irq_data);
}


static void sunxi_dsi_enable_vblank(bool enable, void *data)
{
	struct sunxi_drm_dsi *dsi = (struct sunxi_drm_dsi *)data;

	if (dsi->slave || (dsi->dsi_para.mode_flags & MIPI_DSI_SLAVE_MODE))
		sunxi_tcon_enable_vblank(dsi->tcon_dev, enable);
	else
		dsi_enable_vblank(&dsi->dsi_lcd, enable);
}

static bool sunxi_dsi_fifo_check(void *data)
{
	struct sunxi_drm_dsi *dsi = (struct sunxi_drm_dsi *)data;
	int status;
	status = dsi_get_status(&dsi->dsi_lcd);

	return status ? true : false;
}

static int sunxi_dsi_clk_enable(struct sunxi_drm_dsi *dsi)
{
	unsigned long dsi_rate = 0;
	unsigned long dsi_rate_set = 150000000;
	int ret = 0;
	struct clk *clks[] = {dsi->clk_combphy, dsi->clk};

	if (!IS_ERR_OR_NULL(dsi->clk)) {
		clk_set_rate(dsi->clk, dsi_rate_set);
		dsi_rate = clk_get_rate(dsi->clk);
		if (dsi_rate_set != dsi_rate)
			DRM_WARN("Dsi rate to be set:%lu, real clk rate:%lu\n",
				 dsi_rate_set, dsi_rate);
	}

	ret = sunxi_clk_enable(clks, ARRAY_SIZE(clks));

	return ret;
}

static int sunxi_dsi_connector_init(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	struct sunxi_drm_dsi *dsi = dev_get_priv(conn->dev);
	struct crtc_state *scrtc_state = &state->crtc_state;
	struct sunxi_drm_phy_cfg *phy_cfg = &dsi->phy_opts;
	int ret = 0;

	scrtc_state->tcon_id = dsi->tcon_id;
	scrtc_state->tcon_top_id = dsi->tcon_top_id;
	scrtc_state->check_status = sunxi_dsi_fifo_check;
	scrtc_state->enable_vblank = sunxi_dsi_enable_vblank;
	scrtc_state->vblank_enable_data = dsi;
	memset(phy_cfg, 0, sizeof(struct sunxi_drm_phy_cfg));

	sunxi_dsi_clk_enable(dsi);
	dsi_basic_cfg(&dsi->dsi_lcd, &dsi->dsi_para);
	if (dsi->slave) {
		sunxi_dsi_clk_enable(dsi);
		dsi_basic_cfg(&dsi->slave->dsi_lcd, &dsi->dsi_para);
	}

	phy_cfg->mode = PHY_MODE_MIPI_DPHY;
	phy_cfg->submode = PHY_SINGLE_ENABLE;
	if (dsi->phy) {
		ret = generic_phy_power_on(dsi->phy);
	}
	if ((dsi->slave) && (dsi->slave->phy)) {
		phy_cfg->submode = PHY_DUAL_ENABLE;
		ret = generic_phy_power_on(dsi->slave->phy);
		ret += generic_phy_configure(dsi->phy, phy_cfg);
		ret += generic_phy_configure(dsi->slave->phy, phy_cfg);
	} else {
		ret += generic_phy_configure(dsi->phy, phy_cfg);
	}

	pinctrl_select_state(dsi->dev, "active");
	if (dsi->slave) {
		pinctrl_select_state(dsi->slave->dev, "active");
	}

	return 0;
}

static void sunxi_dsi_clk_config(struct sunxi_drm_dsi *dsi)
{
	unsigned long combphy_rate;

	if (!IS_ERR_OR_NULL(dsi->clk_combphy)) {
		clk_set_rate(dsi->clk_combphy, dsi->hs_clk_rate);
		combphy_rate = clk_get_rate(dsi->clk_combphy);
		if (dsi->hs_clk_rate != combphy_rate)
			DRM_INFO("combphy rate to be set:%lu, real clk rate:%lu\n",
				 dsi->hs_clk_rate, combphy_rate);
	}
}

static int sunxi_dsi_clk_config_disable(struct sunxi_drm_dsi *dsi)
{
	struct clk *clks[] = {dsi->clk_combphy, dsi->clk};

	return sunxi_clk_disable(clks, ARRAY_SIZE(clks));
}


static int sunxi_dsi_enable_output(struct sunxi_drm_dsi *dsi)
{
	struct disp_dsi_para *dsi_para = &dsi->dsi_para;
	sunxi_tcon_enable_output(dsi->tcon_dev);
	dsi_open_hs_mode(&dsi->dsi_lcd, dsi_para);
	if (dsi->slave)
		dsi_open_hs_mode(&dsi->slave->dsi_lcd, dsi_para);

	return 0;
}

static int sunxi_dsi_disable_output(struct sunxi_drm_dsi *dsi)
{

	dsi_close(&dsi->dsi_lcd);
	if (dsi->slave)
		dsi_close(&dsi->slave->dsi_lcd);

	return 0;
}

static int sunxi_dsi_connector_disable(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	struct sunxi_drm_dsi *dsi = dev_get_priv(conn->dev);

	/*drm_panel_disable(dsi->panel);*/
	/*drm_panel_unprepare(dsi->panel);*/

	if (dsi->phy) {
		generic_phy_power_off(dsi->phy);
	}
	if (dsi->slave) {
		if (dsi->slave->phy)
			generic_phy_power_off(dsi->phy);
	}

	sunxi_dsi_clk_config_disable(dsi);
	if (dsi->slave)
		sunxi_dsi_clk_config_disable(dsi->slave);

	pinctrl_select_state(dsi->dev, "sleep");
	if (dsi->slave)
		pinctrl_select_state(dsi->slave->dev, "sleep");

	sunxi_dsi_disable_output(dsi);
	sunxi_tcon_mode_exit(dsi->dev);

	if (!(dsi->slave || (dsi->dsi_para.mode_flags & MIPI_DSI_SLAVE_MODE)))
		irq_free_handler(dsi->irq_no);

	DRM_DEBUG_DRIVER("%s finish\n", __FUNCTION__);
	return 0;

}
static int dsi_populate_dsc_params(struct sunxi_drm_dsi *dsi, struct drm_dsc_config *dsc)
{
	int ret;

	if (dsc->bits_per_pixel & 0xf) {
		DRM_ERROR("DSI does not support fractional bits_per_pixel\n");
		return -EINVAL;
	}

	if (dsc->bits_per_component != 8) {
		DRM_ERROR("DSI does not support bits_per_component != 8 yet\n");
		return -EOPNOTSUPP;
	}

	dsc->simple_422 = 0;
	dsc->convert_rgb = 1;
	dsc->vbr_enable = 0;

	drm_dsc_set_const_params(dsc);
	drm_dsc_set_rc_buf_thresh(dsc);

	/* handle only bpp = bpc = 8, pre-SCR panels */
	ret = drm_dsc_setup_rc_params(dsc, DRM_DSC_1_1_PRE_SCR);
	if (ret) {
		DRM_ERROR("could not find DSC RC parameters\n");
		return ret;
	}

	dsc->initial_scale_value = drm_dsc_initial_scale_value(dsc);
	dsc->line_buf_depth = dsc->bits_per_component + 1;

	return drm_dsc_compute_rc_parameters(dsc);
}

static void dsi_timing_setup(struct sunxi_drm_dsi *dsi, struct disp_video_timings *timing)
{
	int ret;
	struct drm_dsc_config *dsc = dsi->dsc;

	dsc->pic_width = timing->x_res;
	dsc->pic_height = timing->y_res;

	ret = dsi_populate_dsc_params(dsi, dsc);
	if (ret)
		DRM_ERROR("One or more PPS parameters exceeded their allowed bit depth.");
}
static int sunxi_dsi_connector_prepare(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	int ret = -1, bpp = 0, lcd_div;
	struct sunxi_drm_dsi *dsi = dev_get_priv(conn->dev);
	struct crtc_state *scrtc_state = &state->crtc_state;
	struct disp_output_config disp_cfg;
	struct connector_state *conn_state = &state->conn_state;
	struct sunxi_drm_phy_cfg *phy_cfg = &dsi->phy_opts;

	memcpy(&dsi->mode, &conn_state->mode, sizeof(struct drm_display_mode));
	memset(&disp_cfg, 0, sizeof(struct disp_output_config));
	memset(phy_cfg, 0, sizeof(struct sunxi_drm_phy_cfg));

	drm_mode_to_sunxi_video_timings(&dsi->mode, &dsi->dsi_para.timings);
	bpp = mipi_dsi_pixel_format_to_bpp(dsi->dsi_para.format);

	if (dsi->slave)
		lcd_div = bpp / (dsi->dsi_para.lanes * 2);
	else if (dsi->dsc)
		lcd_div = dsi->dsc->bits_per_component / dsi->dsi_para.lanes;
	else
		lcd_div = bpp / dsi->dsi_para.lanes;

	memcpy(&disp_cfg.dsi_para, &dsi->dsi_para,
	       sizeof(struct disp_dsi_para));

	disp_cfg.type = INTERFACE_DSI;
	disp_cfg.de_id = sunxi_drm_crtc_get_hw_id(scrtc_state->crtc);
	disp_cfg.irq_handler = scrtc_state->crtc_irq_handler;
	disp_cfg.irq_data = state;
#ifdef DSI_DISPLL_CLK
	disp_cfg.displl_clk = true;
	disp_cfg.tcon_lcd_div = 1;
#else
	disp_cfg.displl_clk = false;
	disp_cfg.tcon_lcd_div = lcd_div;
#endif
	dsi->displl_clk = disp_cfg.displl_clk;
	if (dsi->slave || (dsi->dsi_para.mode_flags & MIPI_DSI_SLAVE_MODE))
		disp_cfg.slave_dsi = true;

	dsi_basic_cfg(&dsi->dsi_lcd, &dsi->dsi_para);
	if (dsi->slave) {
		sunxi_dsi_clk_enable(dsi);
		dsi_basic_cfg(&dsi->slave->dsi_lcd, &dsi->dsi_para);
	}

	sunxi_tcon_mode_init(dsi->tcon_dev, &disp_cfg);

	if (dsi->phy) {
		phy_mipi_dphy_get_default_config(dsi->dsi_para.timings.pixel_clk,
					bpp, dsi->dsi_para.lanes, &dsi->phy_opts.mipi_dphy);
	}

	/* dual dsi use tcon's irq, single dsi use its own irq */
	if (!disp_cfg.slave_dsi) {
		dsi->irq_handler = scrtc_state->crtc_irq_handler;
		dsi->irq_data = state;
		pr_debug("irq no:%d\n", dsi->irq_no);
		irq_install_handler(dsi->irq_no, sunxi_dsi_irq_event_proc, (void *)dsi);
		irq_enable(dsi->irq_no);
	}
	if (dsi->dsi_para.mode_flags & MIPI_DSI_MODE_VIDEO_BURST)
		dsi->dsi_para.timings.pixel_clk = dsi->dsi_para.timings.pixel_clk * 10 / 9;
	dsi->ls_clk_rate = dsi->dsi_para.timings.pixel_clk;
	dsi->hs_clk_rate = dsi->dsi_para.timings.pixel_clk * lcd_div;
	sunxi_dsi_clk_config(dsi);
	if (dsi->slave)
		sunxi_dsi_clk_config(dsi->slave);
	if (dsi->dsc) {
		tcon_lcd_dsc_src_sel();
		dsi_timing_setup(dsi, &dsi->dsi_para.timings);
		dsi->dsi_para.timings.x_res /= (bpp / dsi->dsc->bits_per_component);
		dsi->dsi_para.timings.hor_total_time /= (bpp / dsi->dsc->bits_per_component);
		dsi->dsi_para.timings.hor_back_porch /= (bpp / dsi->dsc->bits_per_component);
		dsi->dsi_para.timings.hor_sync_time /= (bpp / dsi->dsc->bits_per_component);
		dsc_config_pps(&dsi->dsi_lcd, dsi->dsc);
		dec_dsc_config(&dsi->dsi_lcd, &dsi->dsi_para.timings);
	}
	dsi_packet_cfg(&dsi->dsi_lcd, &dsi->dsi_para);
	if (dsi->slave)
		dsi_packet_cfg(&dsi->slave->dsi_lcd, &dsi->dsi_para);

	phy_cfg->mode = PHY_MODE_MIPI_DPHY;
	phy_cfg->submode = PHY_SINGLE_ENABLE;
	phy_cfg->mipi_dphy.hs_clk_rate = dsi->hs_clk_rate;
	phy_cfg->mipi_dphy.lp_clk_rate = dsi->ls_clk_rate;
	if ((dsi->slave) && (dsi->slave->phy)) {
		phy_cfg->submode = PHY_DUAL_ENABLE;
		ret += generic_phy_configure(dsi->phy, phy_cfg);
		ret += generic_phy_configure(dsi->slave->phy, phy_cfg);
	} else {
		ret += generic_phy_configure(dsi->phy, phy_cfg);
	}
	if (dsi->pll_ss_permille)
		generic_phy_set_speed(dsi->phy, dsi->pll_ss_permille);

	return ret;
}

static int sunxi_dsi_connector_enable(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	int ret = -1;
	struct sunxi_drm_dsi *dsi = dev_get_priv(conn->dev);

	ret = sunxi_dsi_enable_output(dsi);
	if (ret < 0)
		DRM_ERROR("failed to enable dsi ouput\n");
	DRM_INFO("[DSI] %s finish\n", __FUNCTION__);
	return ret;

}
static int sunxi_dsi_connector_save_para(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	struct sunxi_drm_dsi *dsi = dev_get_priv(conn->dev);
	char name[32];
	int node;

	snprintf(name, 32, "/soc/dsi%d/panel", dsi->dsi_id);
	node = fdt_path_offset(working_fdt, name);
	fdt_appendprop_u32(working_fdt, node, "panel-out-reg", (uint32_t)dsi->panel_out_reg);
//	fdt_setprop_u32(working_fdt, node, "panel-out-reg", dsi->panel_out_reg, NULL, 10);

	return 0;
}

static int sunxi_dsi_connector_print(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	struct sunxi_drm_dsi *dsi = dev_get_priv(conn->dev);
	struct disp_video_timings timings;
	struct sunxi_drm_phy_cfg *phy_cfg = &dsi->phy_opts;
	u32 pclk = 0;

	dsi_get_timing(&dsi->dsi_lcd, &timings);

	drm_printf(p, "connector@[dsi%d]:\n", dsi->dsi_id);
	drm_printf(p, "\t interface type: %s, \t output panel: panel_%d\n", dsi->slave ? "dual-dsi" : "single-dsi",
			dsi->panel_out_reg);
	drm_printf(p, "\t dsi_mode: %s, \t 3dfifo: %s\n", dsi->dsi_para.mode_flags & MIPI_DSI_SLAVE_MODE ? "slave-dsi" : "master-dsi",
			dsi->dsi_para.mode_flags & MIPI_DSI_EN_3DFIFO ? "enabled" : "disabled");

	phy_cfg->phy_print = true;
	pclk = generic_phy_configure(dsi->phy, phy_cfg);
	drm_printf(p, "\t clk source: %s\n", dsi->displl_clk ? "displl" : "ccmu");
	if (dsi->displl_clk)
		drm_printf(p, "\t\t pixel_clk rate to be set:%luKHz, real pixel_clk rate:%dKHz\n", dsi->ls_clk_rate / 1000, pclk);
	drm_printf(p, "\t  hsync-len | hback-porch |  hactive  | hfront-porch | vsync-len | vback-porch | vactive | vfront-porch \n");
	drm_printf(p, "\t -----------+-------------+-----------+--------------+-----------+-------------+---------+--------------\n");
	drm_printf(p, "\t     %3d    |    %4d     |   %4d    |     %4d     |    %3d    |    %4d     |   %4d  |     %4d\n",
			timings.hor_sync_time, timings.hor_back_porch, timings.x_res, timings.hor_front_porch,
			timings.ver_sync_time, timings.ver_back_porch, timings.y_res, timings.ver_front_porch);


	drm_printf(p, "\n************ tcon reg dump **************\n");
	sunxi_tcon_print(dsi->tcon_dev);

	drm_printf(p, "\n************ dsi reg dump **************\n");
	/* 0x00 ~ 0x14c */
	print_buffer((ulong)dsi->reg_base, (void *)dsi->reg_base, 4, 84, 4);

	/* 0x1f0 ~ 0x2fc */
	print_buffer((ulong)dsi->reg_base + 0x1f0, (void *)dsi->reg_base + 0x1f0, 4, 68, 4);

	/* 0x1000 ~ 0x113c */
	print_buffer((ulong)dsi->reg_base + 0x1000, (void *)dsi->reg_base + 0x1000, 4, 80, 4);

	return 0;

}
static const struct sunxi_drm_connector_funcs dsi_connector_funcs = {
	.init = sunxi_dsi_connector_init,
	.prepare = sunxi_dsi_connector_prepare,
	.enable = sunxi_dsi_connector_enable,
	.disable = sunxi_dsi_connector_disable,
	.save_kernel_para = sunxi_dsi_connector_save_para,
	.print_conn = sunxi_dsi_connector_print,
	/*.check = sunxi_lvds_connector_check,*/
};


static int sunxi_drm_dsi_bind(struct udevice *dev)
{
	struct mipi_dsi_host *host = dev_get_platdata(dev);

	host->dev = dev;
	host->ops = &sunxi_drm_dsi_host_ops;

	return dm_scan_fdt_dev(dev);
}

static int sunxi_drm_dsi_probe(struct udevice *dev)
{
	struct sunxi_drm_dsi *dsi = dev_get_priv(dev);
	dsi->dsi_data = (struct dsi_data *)dev_get_driver_data(dev);
	struct udevice *secondary = NULL;
	fdt_addr_t addr;
	int ret = -1;
	u32 phandle;

	DRM_INFO("%s:%d\n", __func__, __LINE__);

	addr = dev_read_addr_index(dev, 0);
	if (addr == FDT_ADDR_T_NONE) {
		DRM_ERROR("unable to map dsi registers\n");
		return -EINVAL;
	} else {
		dsi->reg_base = (uintptr_t)addr;
		dsi_set_reg_base(&dsi->dsi_lcd, dsi->reg_base);
	}

	addr = dev_read_addr_index(dev, 1);
	if (addr != FDT_ADDR_T_NONE) {
		dsi->dsc_base = (uintptr_t)addr;
		dsc_set_reg_base(&dsi->dsi_lcd, dsi->dsc_base);
		dsi->dsc_clk = clk_get_by_name(dev, "dsc_clk");
		if (IS_ERR(dsi->dsc_clk)) {
			DRM_ERROR("fail to get clk clk_mipi_dsc\n");
		}
	}

	dsi->irq_no = sunxi_of_get_irq_number(dev, 0);
	if (!dsi->irq_no) {
		DRM_ERROR("get irq no of dsi failed\n");
		return -EINVAL;
	}

	dsi->clk = clk_get_by_name(dev, "dsi_clk");
	if (IS_ERR(dsi->clk)) {
		DRM_ERROR("fail to get clk clk_mipi_dsi\n");
	}

	dsi->clk_combphy = clk_get_by_name(dev, "combphy_clk");
	if (IS_ERR(dsi->clk_combphy)) {
		DRM_ERROR("fail to get clk combphy_clk\n");
	}

	dsi->dev = dev;
	dsi->dsi_id = dsi->dsi_data->id;
	dsi->dsi_lcd.dsi_index = dsi->dsi_id;
	dsi->phy = kmalloc(sizeof(struct phy), __GFP_ZERO);
	ret = generic_phy_get_by_name(dev, "combophy", dsi->phy);
	if (IS_ERR_OR_NULL(dsi->phy))
		DRM_INFO("dsi%d's combophy not setting, maybe not used!\n", dsi->dsi_id);

	ret = dev_read_u32(dev, "dual-channel", &phandle);
	if (!ret) {
		uclass_get_device_by_ofnode(UCLASS_DISPLAY, ofnode_get_by_phandle(phandle), &secondary);
	}

	if (secondary) {
		dsi->slave = dev_get_priv(secondary);
		dsi->slave->master = dsi;
		dsi->dsi_para.dual_dsi = 1;
	}
	dsi->tcon_dev = sunxi_tcon_of_get_tcon_dev(dev);
	if (!dsi->tcon_dev) {
		DRM_ERROR("%s:Get tcon dev fail!\n", __func__);
		return 0;
	}
	dsi->tcon_id = sunxi_tcon_of_get_id(dsi->tcon_dev);
	dsi->tcon_top_id = sunxi_tcon_of_get_id(dsi->tcon_dev);

	ret = sunxi_drm_connector_bind(&dsi->connector, dev, dsi->dsi_id, &dsi_connector_funcs,
				NULL, DRM_MODE_CONNECTOR_DSI);

	DRM_INFO("%s:%d\n", __func__, __LINE__);
	of_periph_clk_config_setup(ofnode_to_offset(dev_ofnode(dev)));
	dsi->bound = true;
	return ret;

}


static int sunxi_drm_dsi_post_bind(struct udevice *dev)
{
	struct mipi_dsi_host *host = dev_get_platdata(dev->parent);
	struct mipi_dsi_device *device = dev_get_parent_platdata(dev);
	char name[20];

	DRM_INFO("%s:%d\n", __func__, __LINE__);
	sprintf(name, "%s.%d", host->dev->name, device->channel);
	device_set_name(dev, name);

	device->dev = dev;
	device->host = host;

	return 0;
}

U_BOOT_DRIVER(sunxi_drm_dsi) = {
	.name = "sunxi_drm_dsi",
	.id = UCLASS_DISPLAY,
	.of_match = sunxi_drm_dsi_match,
	.bind = sunxi_drm_dsi_bind,
	.probe = sunxi_drm_dsi_probe,
	.priv_auto_alloc_size = sizeof(struct sunxi_drm_dsi),
	.platdata_auto_alloc_size = sizeof(struct mipi_dsi_host),
	//child node is dsi panel
	.per_child_platdata_auto_alloc_size = sizeof(struct mipi_dsi_device),
	.child_post_bind = sunxi_drm_dsi_post_bind,
};

//End of File
