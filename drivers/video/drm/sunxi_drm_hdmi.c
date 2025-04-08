/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/
#include <linux/delay.h>
#include <linux/compat.h>
#include <drm/drm_modes.h>

#include <dm.h>
#include <power/regulator.h>

#include "sunxi_device/sunxi_hdmi.h"
#include "sunxi_device/sunxi_tcon.h"

#include "sunxi_drm_drv.h"
#include "sunxi_drm_helper_funcs.h"
#include "sunxi_drm_crtc.h"


#define SUNXI_HDMI_EDID_LENGTH		(1024)
#define SUNXI_HDMI_POWER_CNT	     4
#define SUNXI_HDMI_POWER_NAME	     40

#define SUNXI_HDMI_HPD_FORCE_IN		(0x11)
#define SUNXI_HDMI_HPD_FORCE_OUT	(0x10)
#define SUNXI_HDMI_HPD_MASK_NOTIFY	(0x100)
#define SUNXI_HDMI_HPD_MASK_DETECT	(0x1000)

#if IS_ENABLED(CONFIG_EXTCON)
static const unsigned int sunxi_hdmi_cable[] = {
	EXTCON_DISP_HDMI,
	EXTCON_NONE,
};
#endif

enum sunxi_hdmi_reg_bank_e {
	SHDMI_REG_BANK_CTRL    = 0,
	SHDMI_REG_BANK_PHY     = 1,
	SHDMI_REG_BANK_SCDC    = 2,
	SHDMI_REG_BANK_HPI     = 3
};

struct sunxi_hdmi_res_s {
	struct clk  *clk_hdmi;
	struct clk  *clk_hdmi_24M;
	struct clk  *clk_hdmi_bus;
	struct clk  *clk_hdmi_ddc;
	struct clk  *clk_tcon_tv;
	struct clk  *clk_hdcp;
	struct clk  *clk_hdcp_bus;
	struct clk  *clk_cec;
	struct clk  *clk_cec_parent;
	struct clk  *rst_bus_sub;
	struct clk  *rst_bus_main;

	char  power_name[SUNXI_HDMI_POWER_CNT][SUNXI_HDMI_POWER_NAME];
	struct udevice *hdmi_regu[SUNXI_HDMI_POWER_CNT];
};

struct sunxi_hdmi_ctrl_s {
	int drm_enable;
	int drm_mode_set;
	int drm_hpd_force;

	/* dts power config */
	unsigned int drv_dts_power_cnt;
	unsigned int dts_fast_output;
	unsigned int dts_auto_select;
	unsigned int drv_dts_res_src;

	int drv_clock;
	int drv_enable;
	int drv_hpd_state;
	int drv_hpd_mask;

	u32 drv_pixel_format_cap;

	int drv_reg_bank;
	enum disp_data_bits   drv_max_bits;

	/* edid control */
	u8	drv_edid_dbg_mode;
	u8	drv_edid_dbg_data[SUNXI_HDMI_EDID_LENGTH];
	u8	drv_edid_dbg_size;
	struct mutex	drv_edid_lock;
	struct edid    *drv_edid_data;
};

struct sunxi_drm_hdmi {
	/* drm related members */
	struct udevice				*dev;
	struct udevice				*tcon_dev;
	struct sunxi_drm_connector	connector;
	struct drm_display_mode		drm_mode;
	unsigned int				tcon_id;
	unsigned int				tcon_top_id;

	int hdmi_irq;

	struct sunxi_hdmi_ctrl_s       hdmi_ctrl;

	struct sunxi_hdmi_res_s        hdmi_res;

	struct disp_device_config      disp_config;
	struct disp_video_timings      disp_timing;

	/* suxni hdmi core */
	struct sunxi_hdmi_s            hdmi_core;
};

const struct drm_display_mode _sunxi_hdmi_default_modes[1] = {
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500,
		1920, 2008, 2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC)},

	/* { DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000,
		3840, 4016, 4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC)}, */
};

/*******************************************************************************
 * drm sunxi hdmi encoder and connector container_of
 ******************************************************************************/
static inline struct sunxi_drm_hdmi *
drm_connector_to_hdmi(struct sunxi_drm_connector *connector)
{
	return container_of(connector, struct sunxi_drm_hdmi, connector);
}

#if IS_ENABLED(CONFIG_AW_DRM_HDMI20)
/*******************************************************************************
 * sunxi hdmi snd function
 ******************************************************************************/
/**
 * @desc: sound hdmi audio enable
 * @enable: 1 - enable hdmi audio
 *          0 - disable hdmi audio
 * @channel:
 * @return: 0 - success
 *         -1 - failed
 */
static s32 _sunxi_drv_audio_enable(u8 enable, u8 channel)
{
	int ret = 0;

	if (enable)
		ret = sunxi_hdmi_audio_enable();

	hdmi_inf("hdmi drv audio set %s %s\n",
		enable ? "enable" : "disable", ret ? "failed" : "done");
	return 0;
}

/**
 * @desc: sound hdmi audio param config
 * @audio_para: audio params
 * @return: 0 - success
 *         -1 - failed
 */
static s32 _sunxi_drv_audio_set_info(hdmi_audio_t *audio_para)
{
	int ret = 0;

	ret = sunxi_hdmi_audio_set_info(audio_para);
	hdmi_inf("hdmi drv audio set info %s\n", ret ? "failed" : "done");

	return 0;
}

int snd_hdmi_get_func(__audio_hdmi_func *func)
{
	if (IS_ERR_OR_NULL(func)) {
		shdmi_err(func);
		return -1;
	}

	func->hdmi_audio_enable   = _sunxi_drv_audio_enable;
	func->hdmi_set_audio_para = _sunxi_drv_audio_set_info;
	return 0;
}
EXPORT_SYMBOL_GPL(snd_hdmi_get_func);
#endif

static int _sunxi_drv_hdmi_regulator_on(struct sunxi_drm_hdmi *hdmi)
{
	int ret = 0, loop = 0;

	for (loop = 0; loop < hdmi->hdmi_ctrl.drv_dts_power_cnt; loop++) {
		if (!hdmi->hdmi_res.hdmi_regu[loop])
			continue;

		ret = regulator_set_enable(hdmi->hdmi_res.hdmi_regu[loop], 0x1);
		hdmi_trace("hdmi drv enable regulator %s %s\n",
			hdmi->hdmi_res.power_name[loop], ret != 0 ? "failed" : "success");
	}
	return 0;
}

static int _sunxi_drv_hdmi_clock_on(struct sunxi_drm_hdmi *hdmi)
{
	struct sunxi_hdmi_res_s  *pclk = &hdmi->hdmi_res;
	struct sunxi_hdmi_ctrl_s *pctl = &hdmi->hdmi_ctrl;

	if (pctl->drv_clock) {
		hdmi_trace("hdmi drv clock has been enable\n");
		return 0;
	}

	if (!IS_ERR_OR_NULL(pclk->rst_bus_main)) {
		hdmi_trace("hdmi drv main bus gating enable\n");
		clk_prepare_enable(pclk->rst_bus_main);
	}

	if (!IS_ERR_OR_NULL(pclk->rst_bus_sub)) {
		hdmi_trace("hdmi drv sub bus gating enable\n");
		clk_prepare_enable(pclk->rst_bus_sub);
	}

	if (!IS_ERR_OR_NULL(pclk->clk_hdmi_24M)) {
		hdmi_trace("hdmi drv clock 24M enable\n");
		clk_prepare_enable(pclk->clk_hdmi_24M);
	}

	if (!IS_ERR_OR_NULL(pclk->clk_hdmi)) {
		hdmi_trace("hdmi drv clock enable\n");
		clk_prepare_enable(pclk->clk_hdmi);
	}

	hdmi_trace("hdmi drv all clock enable done\n");
	pctl->drv_clock = 0x1;
	return 0;
}

static int _sunxi_drv_hdmi_clock_off(struct sunxi_drm_hdmi *hdmi)
{
	struct sunxi_hdmi_res_s  *pclk = &hdmi->hdmi_res;
	struct sunxi_hdmi_ctrl_s *pctl = &hdmi->hdmi_ctrl;

	if (!pctl->drv_clock) {
		hdmi_trace("hdmi drv clock has been disable\n");
		return 0;
	}

	/* not disable main/sub reset */

	if (!IS_ERR_OR_NULL(pclk->clk_hdmi)) {
		hdmi_trace("hdmi drv clock disable\n");
		clk_disable(pclk->clk_hdmi);
	}

	if (!IS_ERR_OR_NULL(pclk->clk_hdmi_24M)) {
		hdmi_trace("hdmi drv clock 24M disable\n");
		clk_disable(pclk->clk_hdmi_24M);
	}

	hdmi_trace("hdmi drv all clock disable done\n");
	pctl->drv_clock = 0x0;
	return 0;
}
static int _sunxi_drv_hdmi_read_edid(struct sunxi_drm_hdmi *hdmi)
{
	struct i2c_msg msgs[2];
	u8 *edid_data = NULL, read_addr = 0x0, edid_block = 0x1;
	int i = 0;

	edid_data = kzalloc(512, GFP_KERNEL);
	if (IS_ERR_OR_NULL(edid_data)) {
		shdmi_err(edid_data);
		return -1;
	}

	for (i = 0; i < edid_block; i++) {
		read_addr = (i * 128);
		msgs[0].addr  = 0x50;
		msgs[0].flags = 0;
		msgs[0].len   = 0x1;
		msgs[0].buf   = &read_addr;

		/* Prepare read message */
		msgs[1].addr = 0x50;
		msgs[1].flags = I2C_M_RD;
		msgs[1].len = 128;
		msgs[1].buf = edid_data + (i * 128);
		sunxi_hdmi_i2cm_xfer(msgs, ARRAY_SIZE(msgs));

		if (i != 0)
			continue;

		if (edid_data[0x7E] == 0)
			break;

		edid_block = edid_data[0x7E] + 0x1;
	}

	hdmi->hdmi_ctrl.drv_edid_data = (struct edid *)edid_data;
	return 0;
}

static int _sunxi_drv_hdmi_hpd_get(struct sunxi_drm_hdmi *hdmi)
{
	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return 0;
	}

	hdmi->hdmi_ctrl.drv_hpd_state = sunxi_hdmi_get_hpd();
	return hdmi->hdmi_ctrl.drv_hpd_state;
}

/*******************************************************************************
 * drm sunxi hdmi driver functions
 ******************************************************************************/
int _sunxi_drv_hdmi_check_disp_info(struct sunxi_drm_hdmi *hdmi)
{
	struct disp_device_config *old_info = sunxi_hdmi_get_disp_info();
	struct disp_device_config *new_info = &hdmi->disp_config;
	int count = 0;

	if (old_info->format != new_info->format) {
		hdmi_inf("hdmi drv check info format %d != %d\n",
			old_info->format, new_info->format);
		count |= BIT(0);
	}

	if (old_info->bits != new_info->bits) {
		hdmi_inf("hdmi drv check info bits   %d != %d\n",
			old_info->bits, new_info->bits);
		count |= BIT(1);
	}

	if (old_info->dvi_hdmi != new_info->dvi_hdmi) {
		hdmi_inf("hdmi drv check info dvi_hdmi %d != %d\n",
			old_info->dvi_hdmi, new_info->dvi_hdmi);
		count |= BIT(2);
	}

	if (old_info->eotf != new_info->eotf) {
		hdmi_inf("hdmi drv check info eotf %d != %d\n",
			old_info->eotf, new_info->eotf);
		count |= BIT(3);
	}

	if (old_info->cs != new_info->cs) {
		hdmi_inf("hdmi drv check info cs %d != %d\n",
			old_info->cs, new_info->cs);
		count |= BIT(4);
	}

	if (old_info->range != new_info->range) {
		hdmi_inf("hdmi drv check info range %d != %d\n",
			old_info->range, new_info->range);
		count |= BIT(5);
	}

	if (old_info->scan != new_info->scan) {
		hdmi_inf("hdmi drv check info scan %d != %d\n",
			old_info->scan, new_info->scan);
		count |= BIT(6);
	}

	if (old_info->aspect_ratio != new_info->aspect_ratio) {
		hdmi_inf("hdmi drv check info aspect_ratio %d != %d\n",
			old_info->aspect_ratio, new_info->aspect_ratio);
		count |= BIT(7);
	}

	return count;
}

static int _sunxi_drv_hdmi_set_rate(struct sunxi_hdmi_res_s *p_clk)
{
	unsigned long clk_rate = 0;

	if (IS_ERR_OR_NULL(p_clk)) {
		shdmi_err(p_clk);
		return -1;
	}

	clk_rate = clk_get_rate(p_clk->clk_tcon_tv);
	if (clk_rate == 0) {
		hdmi_err("tcon clock rate is 0");
		return -1;
	}

	clk_set_rate(p_clk->clk_hdmi, clk_rate);
	return 0;
}

static int _sunxi_drv_hdmi_enable(struct sunxi_drm_hdmi *hdmi)
{
	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	if (hdmi->hdmi_ctrl.drv_enable) {
		hdmi_inf("hdmi drv has been enable!\n");
		return 0;
	}

	_sunxi_drv_hdmi_set_rate(&hdmi->hdmi_res);

	/* hdmi driver video ops enable */
	sunxi_hdmi_config();

	hdmi->hdmi_ctrl.drv_enable = 0x1;
	hdmi_inf("hdmi drv enable output done\n");
	return 0;
}

static int _sunxi_drv_hdmi_disable(struct sunxi_drm_hdmi *hdmi)
{
	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	if (!hdmi->hdmi_ctrl.drv_enable) {
		hdmi_inf("hdmi drv has been disable!\n");
		return 0;
	}

	sunxi_hdmi_disconfig();

	hdmi->hdmi_ctrl.drv_enable = 0x0;
	hdmi_trace("hdmi drv disable done\n");
	return 0;
}

static bool _sunxi_drv_hdmi_fifo_check(void *data)
{
	struct sunxi_drm_hdmi *hdmi = (struct sunxi_drm_hdmi *)data;
	return sunxi_tcon_check_fifo_status(hdmi->tcon_dev);
}

static void _sunxi_drv_hdmi_vblank_enable(bool enable, void *data)
{
	struct sunxi_drm_hdmi *hdmi = (struct sunxi_drm_hdmi *)data;

	sunxi_tcon_enable_vblank(hdmi->tcon_dev, enable);
}

int _sunxi_drv_hdmi_color_cap_set(struct sunxi_drm_hdmi *hdmi, u32 cap)
{
	struct disp_device_config *config = &hdmi->disp_config;
	u32 cap_bits = 0;

	if (cap > 0xFFFF) {
		config->format = DISP_CSC_TYPE_RGB;
		config->bits   = DISP_DATA_8BITS;
		goto exit_mode;
	}

	cap_bits = (u32)(ffs(cap) - 1);

	config->format = (cap_bits % 4);
	config->bits   = (cap_bits / 4);

exit_mode:
	hdmi_inf("hdmi drv color cap update format: %d, bits: %d\n",
		config->format, config->bits);
	return 0;
}

static int _sunxi_drv_hdmi_select_output(struct sunxi_drm_hdmi *hdmi)
{
#if 0
	int ret = 0;
	struct disp_device_config *config = NULL;
	u32 vic = (u32)drm_match_cea_mode(&hdmi->drm_mode_adjust);

	config = &hdmi->disp_config;

	/* if dvi mode, use base config info */
	if (config->dvi_hdmi == DISP_DVI) {
		config->format = DISP_CSC_TYPE_RGB;
		config->bits   = DISP_DATA_8BITS;
		config->eotf   = DISP_EOTF_GAMMA22; /* SDR */
		config->cs     = DISP_BT709;
		hdmi_inf("hdmi drv select dvi output\n");
		goto check_clock;
	}

	sunxi_hdmi_disp_select_eotf(config);

	sunxi_hdmi_disp_select_space(config, vic);

format_select:
	sunxi_hdmi_disp_select_format(config, vic);

check_clock:
	ret = sunxi_hdmi_video_check_tmds_clock(config->format,
			config->bits, hdmi->drm_mode_adjust.clock);
	if (ret == 0x0) {
		/* 1. Reduce color depth */
		if ((config->bits < DISP_DATA_16BITS) &&
				(config->bits != DISP_DATA_8BITS)) {
			config->bits--;
			hdmi_inf("hdmi drv auto download bits: %s\n",
				sunxi_hdmi_color_depth_string(config->bits));
			goto format_select;
		}
		if ((config->format < DISP_CSC_TYPE_YUV420) &&
				(config->format != DISP_CSC_TYPE_YUV420)) {
			config->format++;
			hdmi_inf("hdmi drv auto download format: %s\n",
				sunxi_hdmi_color_format_string(config->format));
			goto format_select;
		}
		hdmi_inf("hdmi drv select output failed when clock overflow\n");
		return -1;
	}

	config->range = (config->format == DISP_CSC_TYPE_RGB) ?
			DISP_COLOR_RANGE_0_255 : DISP_COLOR_RANGE_16_235;
	config->scan  = DISP_SCANINFO_NO_DATA;
    config->aspect_ratio = HDMI_ACTIVE_ASPECT_PICTURE;
#endif
	return 0;
}

static ssize_t _sunxi_hdmi_sysfs_reg_bank_show(struct udevice *dev)
{
	struct sunxi_drm_hdmi *hdmi = dev_get_priv(dev);

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	printf(" - current reg bank index: %d\n", hdmi->hdmi_ctrl.drv_reg_bank);
	return 1;
}

ssize_t _sunxi_hdmi_sysfs_reg_bank_store(struct udevice *dev, int bank)
{
	struct sunxi_drm_hdmi *hdmi = dev_get_priv(dev);

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	hdmi->hdmi_ctrl.drv_reg_bank = (u8)bank;
	printf(" - set reg bank index: %d\n", hdmi->hdmi_ctrl.drv_reg_bank);
	return 1;
}

ssize_t _sunxi_hdmi_sysfs_reg_read_store(struct udevice *dev, u32 reg, u32 count)
{
	struct sunxi_drm_hdmi *hdmi = dev_get_priv(dev);
	u32 r_value = 0, i = 0;

	for (i = 0; i < count; i++) {
		switch (hdmi->hdmi_ctrl.drv_reg_bank) {
		case SHDMI_REG_BANK_PHY:
			sunxi_hdmi_phy_read((u8)reg, &r_value);
			printf(" - phy read: 0x%x = 0x%x\n", (u8)reg, r_value);
			break;
		case SHDMI_REG_BANK_SCDC:
			r_value = sunxi_hdmi_scdc_read((u8)reg);
			printf(" - scdc read: 0x%x = 0x%x\n", (u8)reg, (u8)r_value);
			break;
		default:
			r_value = sunxi_hdmi_ctrl_read(reg);
			printf(" - ctrl read: 0x%x = 0x%x\n", reg, r_value);
			break;
		}
		reg++;
	}
	return count;
}

ssize_t _sunxi_hdmi_sysfs_reg_write_store(struct udevice *dev, u32 reg, u32 value)
{
	struct sunxi_drm_hdmi *hdmi = dev_get_priv(dev);

	switch (hdmi->hdmi_ctrl.drv_reg_bank) {
	case SHDMI_REG_BANK_PHY:
		sunxi_hdmi_phy_write((u8)reg, (u32)value);
		printf(" - phy write: 0x%x = 0x%x\n", (u8)reg, (u32)value);
		break;
	case SHDMI_REG_BANK_SCDC:
		sunxi_hdmi_scdc_write(reg, value);
		printf(" - scdc write: 0x%x = 0x%x\n", (u8)reg, (u8)value);
		break;
	case SHDMI_REG_BANK_HPI:
		printf(" - current unsupport hpi write\n");
		break;
	default:
		sunxi_hdmi_ctrl_write((uintptr_t)reg, (u32)value);
		printf(" - ctrl write: 0x%x = 0x%x\n", reg, (u32)value);
		break;
	}

	return 1;
}

ssize_t _sunxi_hdmi_sysfs_pattern_store(struct udevice *dev, int bit)
{
	struct sunxi_drm_hdmi *hdmi = dev_get_priv(dev);

	switch (bit) {
	case 1:
		sunxi_hdmi_video_set_pattern(0x1, 0xFF0000);
		break;
	case 2:
		sunxi_hdmi_video_set_pattern(0x1, 0x00FF00);
		break;
	case 3:
		sunxi_hdmi_video_set_pattern(0x1, 0x0000FF);
		break;
	case 4:
		sunxi_hdmi_video_set_pattern(0x0, 0x0);
		sunxi_tcon_show_pattern(hdmi->tcon_dev, 0x1);
		break;
	default:
		sunxi_hdmi_video_set_pattern(0x0, 0x0);
		sunxi_tcon_show_pattern(hdmi->tcon_dev, 0x0);
		break;
	}

	return 1;
}

static ssize_t _sunxi_hdmi_sysfs_hdmi_source_show(struct udevice *dev)
{
	int n = 0;
	struct sunxi_drm_hdmi   *hdmi = dev_get_priv(dev);
	char buf[4096];

	memset(buf, 0x0, ARRAY_SIZE(buf));
#if 1
	n += sprintf(buf + n, "\n[ver]\n");
	n += sprintf(buf + n, " - hw: 2.0\n");
	n += sprintf(buf + n, " - sw: 2.25.0403: Ie8e1729bbeaf78511ca893b7b099eb2233492d2d\n");

	n += sprintf(buf + n, "\n[drv cfg]\n");
	n += sprintf(buf + n, "|       |          dts         |              drm              |\n");
	n += sprintf(buf + n, "|  name |----------------------+-------------------------------|\n");
	n += sprintf(buf + n, "|       | boot mode | resistor | enable | mode set | mode info |\n");
	n += sprintf(buf + n, "|-------+-----------+----------+--------+----------+-----------|\n");
	n += sprintf(buf + n, "| state |  %-6s   |  %-7s |  %-3s   |   %-3s    | %-4dx%-4d |\n",
		hdmi->hdmi_ctrl.dts_fast_output ? "fast" : "normal",
		hdmi->hdmi_ctrl.drv_dts_res_src ? "onboard" : "onchip",
		hdmi->hdmi_ctrl.drm_enable ? "yes" : "no",
		hdmi->hdmi_ctrl.drm_mode_set ? "yes" : "no",
		hdmi->drm_mode.hdisplay, hdmi->drm_mode.vdisplay);

	n += sprintf(buf + n, "\n[drv state]\n");
	n += sprintf(buf + n, "|       |     driver     |     hpd      |                |\n");
	n += sprintf(buf + n, "|  name |----------------+--------------+ support format |\n");
	n += sprintf(buf + n, "|       | clock | output | state | mask |                |\n");
	n += sprintf(buf + n, "|-------+-------+--------+-------+------+----------------|\n");
	n += sprintf(buf + n, "| state |  %-4s |  %-4s  |  %-3s  | 0x%-2x |    0x%-4x      |\n",
		hdmi->hdmi_ctrl.drv_clock ? "on" : "off",
		hdmi->hdmi_ctrl.drv_enable ? "on" : "off",
		hdmi->hdmi_ctrl.drv_hpd_state ? "in" : "out",
		hdmi->hdmi_ctrl.drv_hpd_mask,
		hdmi->hdmi_ctrl.drv_pixel_format_cap);
#endif
	printf("%s\n", buf);
	memset(buf, 0x0, ARRAY_SIZE(buf));
	n = 0;

	n += sunxi_hdmi_tx_dump(buf + n);
	printf("%s\n", buf);
	memset(buf, 0x0, ARRAY_SIZE(buf));

	return 1;
}

static ssize_t _sunxi_hdmi_sysfs_hdmi_sink_show(struct udevice *dev)
{
	ssize_t n = 0;
	u8 data = 0, status = 0;
	struct sunxi_drm_hdmi   *hdmi = dev_get_priv(dev);
	struct drm_display_info *info = &hdmi->connector.display_info;
	char buf[2048];

	if (!_sunxi_drv_hdmi_hpd_get(hdmi)) {
		n += sprintf(buf + n, "not sink info when hpd plugout!\n");
		goto exit;
	}

	memset(buf, 0x0, ARRAY_SIZE(buf));
	n += sprintf(buf + n, "\n========= [hdmi sink] =========\n");
	n += sunxi_hdmi_rx_dump(buf + n);

	printf("%s\n", buf);
	memset(buf, 0x0, ARRAY_SIZE(buf));
	n = 0;

	/* dump scdc */
	if (!info->hdmi.scdc.supported)
		goto exit;

	n += sprintf(buf + n, "[scdc]\n");
	data = sunxi_hdmi_scdc_read(SCDC_TMDS_CONFIG);
	status = sunxi_hdmi_scdc_read(SCDC_SCRAMBLER_STATUS) & SCDC_SCRAMBLING_STATUS;
	n += sprintf(buf + n, " - clock ratio    : %s\n",
			(data & SCDC_TMDS_BIT_CLOCK_RATIO_BY_40) ? "1/40" : "1/10");
	n += sprintf(buf + n, " - scramble       : %s\n",
			(data & SCDC_TMDS_BIT_CLOCK_RATIO_BY_40) ? "set" : "unset");
	n += sprintf(buf + n, " - scramble state : %s\n",
			status ? "enable" : "disable");

	data = sunxi_hdmi_scdc_read(SCDC_STATUS_FLAGS_0);
	n += sprintf(buf + n, " - clock channel  : %s\n",
			data & SCDC_CLOCK_DETECT ? "lock" : "unlock");
	n += sprintf(buf + n, " - data0 channel  : %s\n",
			data & SCDC_CH0_LOCK ? "lock" : "unlock");
	n += sprintf(buf + n, " - data1 channel  : %s\n",
			data & SCDC_CH1_LOCK ? "lock" : "unlock");
	n += sprintf(buf + n, " - data2 channel  : %s\n",
			data & SCDC_CH2_LOCK ? "lock" : "unlock");

	printf("%s\n", buf);
	memset(buf, 0x0, ARRAY_SIZE(buf));

exit:
	return 1;
}

/*******************************************************************************
 * drm sunxi hdmi encoder helper functions
 ******************************************************************************/
int _sunxi_drm_hdmi_disable(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	struct sunxi_drm_hdmi *hdmi = drm_connector_to_hdmi(conn);
	int ret = 0;

	ret = _sunxi_drv_hdmi_disable(hdmi);
	if (ret != 0) {
		hdmi_err("sunxi hdmi driver disable failed\n");
		return 0;
	}

	ret = sunxi_tcon_mode_exit(hdmi->tcon_dev);
	if (ret != 0) {
		hdmi_err("sunxi tcon hdmi mode exit failed\n");
		return 0;
	}

	_sunxi_drv_hdmi_clock_off(hdmi);
	mdelay(10);
	_sunxi_drv_hdmi_clock_on(hdmi);

	hdmi->hdmi_ctrl.drm_enable   = 0x0;
	hdmi->hdmi_ctrl.drm_mode_set = 0x0;
	hdmi->hdmi_ctrl.drv_enable   = 0x0;
	hdmi_inf("drm hdmi atomic disable done.\n");
	return 0;
}

int _sunxi_drm_hdmi_enable(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	struct sunxi_drm_hdmi *hdmi = drm_connector_to_hdmi(conn);
	struct crtc_state *scrtc_state = &state->crtc_state;
	struct disp_output_config disp_cfg;
	int hw_id = sunxi_drm_crtc_get_hw_id(scrtc_state->crtc);
	int ret = 0;

	_sunxi_drv_hdmi_clock_on(hdmi);

	ret = sunxi_hdmi_set_disp_info(&hdmi->disp_config);
	if (ret != 0)
		hdmi_inf("drm atomic enable fill config failed\n");

	/* set tcon config data */
	memset(&disp_cfg, 0x0, sizeof(disp_cfg));
	memcpy(&disp_cfg.timing, &hdmi->disp_timing, sizeof(struct disp_video_timings));
	disp_cfg.type  = INTERFACE_HDMI;
	disp_cfg.de_id = hw_id;
	disp_cfg.irq_handler = scrtc_state->crtc_irq_handler;
	disp_cfg.irq_data = state;

	/* tcon hdmi enable */
	ret = sunxi_tcon_mode_init(hdmi->tcon_dev, &disp_cfg);
	if (ret != 0) {
		hdmi_err("sunxi tcon hdmi mode init failed\n");
		return 0;
	}

	ret = _sunxi_drv_hdmi_enable(hdmi);
	if (ret != 0) {
		hdmi_err("sunxi hdmi driver enable failed\n");
		return 0;
	}

	hdmi->hdmi_ctrl.drm_enable = 0x1;
	hdmi_inf("drm hdmi atomic enable done.\n");
	return 0;
}

int _sunxi_drm_hdmi_mode_set(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	int ret = 0;
	struct connector_state *conn_state = &state->conn_state;
	struct sunxi_drm_hdmi *hdmi = drm_connector_to_hdmi(conn);

	if (!hdmi) {
		hdmi_err("%s param hdmi is null!!!\n", __func__);
		return -1;
	}

	memcpy(&hdmi->drm_mode, &conn_state->mode, sizeof(struct drm_display_mode));

	ret = sunxi_hdmi_set_disp_mode(&hdmi->drm_mode);
	if (ret != 0) {
		hdmi_err("drm mode set convert failed\n");
		return 0;
	}

	memset(&hdmi->disp_timing, 0x0, sizeof(hdmi->disp_timing));
	ret = drm_mode_to_sunxi_video_timings(&hdmi->drm_mode, &hdmi->disp_timing);
	if (ret != 0) {
		hdmi_err("drm mode disp_timing %d*%d convert disp disp_timing failed\n",
			hdmi->drm_mode.hdisplay, hdmi->drm_mode.vdisplay);
		return 0;
	}

	ret = _sunxi_drv_hdmi_select_output(hdmi);
	if (ret != 0) {
		hdmi_err("drm mode select output info failed\n");
		return 0;
	}

	hdmi->hdmi_ctrl.drm_mode_set = 0x1;
	hdmi_inf("drm hdmi mode set: %d*%d done.\n",
			conn_state->mode.hdisplay, conn_state->mode.vdisplay);
	return 0;
}

/*******************************************************************************
 * drm sunxi hdmi connect helper functions
 ******************************************************************************/
int _sunxi_drm_hdmi_get_modes(struct sunxi_drm_connector *conn,
		struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct sunxi_drm_hdmi *hdmi = drm_connector_to_hdmi(conn);
	struct drm_display_mode *sel_mode = &conn_state->mode;
	struct drm_display_mode *mode;
	int rate = 0x0;
	int mode_num = 0;
	struct edid *edid = NULL;

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	edid = hdmi->hdmi_ctrl.drv_edid_data;
	if (!IS_ERR_OR_NULL(edid)) {
		mode_num += drm_add_edid_modes(conn, edid);
		hdmi_trace("drm hdmi get mode num: %d\n", mode_num);
	}

	if (mode_num == 0) {
		memcpy(sel_mode, &_sunxi_hdmi_default_modes[0],
				sizeof(struct drm_display_mode));
		hdmi_trace("drm hdmi use default mode\n");
		goto exit;
	}

	list_for_each_entry(mode, &conn->probed_modes, head) {
		if (mode->type & DRM_MODE_TYPE_PREFERRED) {
			memcpy(sel_mode, mode, sizeof(struct drm_display_mode));
			break;
		}
	}

exit:
	rate = drm_mode_vrefresh(sel_mode);
	pr_err("drm hdmi get mode: %dx%d@%dHz\n",
			sel_mode->hdisplay, sel_mode->vdisplay, rate);
	return 0;
}

static int _sunxi_drm_hdmi_mode_valid(struct sunxi_drm_connector *conn,
		struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	int rate = drm_mode_vrefresh(&conn_state->mode);

	hdmi_inf("drm hdmi check mode valid\n");

	/* check frame rate support */
	if (rate > 60) {
		return MODE_BAD;
	}

	return MODE_OK;
}

int _sunxi_drm_hdmi_detect(struct sunxi_drm_connector *conn,
		struct display_state *state)
{
	int ret = 0;
	struct sunxi_drm_hdmi    *hdmi = drm_connector_to_hdmi(conn);
	struct sunxi_hdmi_ctrl_s *pctl = NULL;
	u8 *raw_edid = NULL;

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return 0;
	}
	pctl = &hdmi->hdmi_ctrl;

	_sunxi_drv_hdmi_hpd_get(hdmi);
	if (pctl->dts_fast_output) {
		hdmi_trace("uhdmi use fast output mode\n");
		goto exit;
	}

	if (pctl->drv_hpd_state != 0x1)
		goto exit;

	ret = _sunxi_drv_hdmi_read_edid(hdmi);
	if (ret != 0)
		goto exit;

	raw_edid = (u8 *)hdmi->hdmi_ctrl.drv_edid_data;
	ret = sunxi_hdmi_edid_parse(raw_edid);
	if (ret != 0)
		goto exit;

exit:
	hdmi_inf("drm hdmi get hpd: %s\n", ret ? "plugin" : "plugout");
	return pctl->drv_hpd_state;
}

static int _sunxi_drm_hdmi_init(struct sunxi_drm_connector *conn,
					  struct display_state *state)
{
	struct sunxi_drm_hdmi *hdmi = drm_connector_to_hdmi(conn);
	struct crtc_state *scrtc_state = &state->crtc_state;

	scrtc_state->tcon_id       = hdmi->tcon_id;
	scrtc_state->tcon_top_id   = hdmi->tcon_top_id;
	scrtc_state->enable_vblank = _sunxi_drv_hdmi_vblank_enable;
	scrtc_state->check_status  = _sunxi_drv_hdmi_fifo_check;
	scrtc_state->vblank_enable_data = hdmi;
	hdmi_inf("drm hdmi init done\n");

	return 0;
}

static const struct sunxi_drm_connector_funcs sunxi_hdmi_connector_funcs = {
	.init       = _sunxi_drm_hdmi_init,
	.prepare    = _sunxi_drm_hdmi_mode_set,
	.enable     = _sunxi_drm_hdmi_enable,
	.disable    = _sunxi_drm_hdmi_disable,
	.detect     = _sunxi_drm_hdmi_detect,
	.mode_valid = _sunxi_drm_hdmi_mode_valid,
	.get_timing = _sunxi_drm_hdmi_get_modes,
};

/*******************************************************************************
 * @desc: sunxi hdmi driver init function
 ******************************************************************************/
int __sunxi_hdmi_init_dts(struct sunxi_drm_hdmi *hdmi)
{
	u32 value = 0;
	int ret = 0;
	struct udevice *dev = hdmi->dev;
	ofnode node = dev_ofnode(dev);
	struct sunxi_hdmi_res_s  *pclk = &hdmi->hdmi_res;
	struct sunxi_hdmi_ctrl_s *pcfg = &hdmi->hdmi_ctrl;

	hdmi->hdmi_core.reg_base = (uintptr_t)dev_read_addr(dev);
	if (hdmi->hdmi_core.reg_base <= 0) {
		hdmi_err("unable to map hdmi registers");
		return -1;
	}

	hdmi->hdmi_irq = sunxi_of_get_irq_number(dev, 0);
	if (hdmi->hdmi_irq < 0) {
		hdmi_err("hdmi drv detect dts not set irq.\n");
		hdmi->hdmi_irq = -1;
	}

	ret = ofnode_read_u32(node, "uhdmi_fast_output", &value);
	pcfg->dts_fast_output = (ret != 0x0) ? 0x0 : value;

	ret = ofnode_read_u32(node, "uhdmi_power_count", &value);
	pcfg->drv_dts_power_cnt = (ret != 0x0) ? 0x0 : value;

	ret = ofnode_read_u32(node, "uhdmi_resistor_select", &value);
	pcfg->drv_dts_res_src = (ret != 0x0) ? 0x1 : value;

	/* parse tcon clock */
	pclk->clk_tcon_tv = clk_get_by_name(dev, "clk_tcon_tv");
	if (IS_ERR_OR_NULL(pclk->clk_tcon_tv))
		pclk->clk_tcon_tv = NULL;

	/* parse hdmi clock */
	pclk->clk_hdmi = clk_get_by_name(dev, "clk_hdmi");
	if (IS_ERR_OR_NULL(pclk->clk_hdmi))
		pclk->clk_hdmi = NULL;

	/* parse hdmi 24M clock */
	pclk->clk_hdmi_24M = clk_get_by_name(dev, "clk_hdmi_24M");
	if (IS_ERR_OR_NULL(pclk->clk_hdmi_24M))
		pclk->clk_hdmi_24M = NULL;

	/* parse hdmi bus clock */
	pclk->clk_hdmi_bus = clk_get_by_name(dev, "clk_bus_hdmi");
	if (IS_ERR_OR_NULL(pclk->clk_hdmi_bus))
		pclk->clk_hdmi_bus = NULL;

	/* parse hdmi ddc clock */
	pclk->rst_bus_main = clk_get_by_name(dev, "rst_main");
	if (IS_ERR_OR_NULL(pclk->rst_bus_main))
		pclk->rst_bus_main = NULL;

	pclk->rst_bus_sub = clk_get_by_name(dev, "rst_sub");
	if (IS_ERR_OR_NULL(pclk->rst_bus_sub))
		pclk->rst_bus_sub = NULL;

	hdmi_inf("hdmi drv init dts done\n");
	return 0;
}

int __sunxi_hdmi_init_resource(struct sunxi_drm_hdmi *hdmi)
{
	int ret = 0, loop = 0;
	char power_name[40];

	struct sunxi_hdmi_res_s *pres = &hdmi->hdmi_res;

	for (loop = 0; loop < hdmi->hdmi_ctrl.drv_dts_power_cnt; loop++) {
		sprintf(power_name, "hdmi_power%d", loop);
		ret = uclass_get_device_by_phandle(UCLASS_REGULATOR, hdmi->dev,
				power_name, &pres->hdmi_regu[loop]);
		if (ret) {
			hdmi_wrn("failed to request regulator(%s): %d\n", power_name, ret);
			continue;
		}

		hdmi_inf("hdmi drv power name: %s\n", power_name);
	}

	if (hdmi->tcon_dev) {
		sunxi_tcon_hdmi_open(hdmi->tcon_dev);
		sunxi_tcon_hdmi_src(hdmi->tcon_dev, 0x0);
	}

	_sunxi_drv_hdmi_regulator_on(hdmi);
	_sunxi_drv_hdmi_clock_on(hdmi);

	mutex_init(&hdmi->hdmi_ctrl.drv_edid_lock);

	return 0;
}

int __sunxi_hdmi_init_value(struct sunxi_drm_hdmi *hdmi)
{
	struct disp_device_config *init_info = &hdmi->disp_config;

	init_info->format       = DISP_CSC_TYPE_RGB;
	init_info->bits         = DISP_DATA_8BITS;
	init_info->eotf         = DISP_EOTF_GAMMA22; /* SDR */
	init_info->cs           = DISP_BT709;
	init_info->dvi_hdmi     = DISP_HDMI;
	init_info->range        = DISP_COLOR_RANGE_DEFAULT;
	init_info->scan         = DISP_SCANINFO_NO_DATA;
//	init_info->aspect_ratio = HDMI_ACTIVE_ASPECT_PICTURE;

	hdmi->hdmi_ctrl.drv_enable   = 0x0;
	hdmi->hdmi_ctrl.drv_pixel_format_cap  = BIT(SHDMI_RGB888_8BITS);
	hdmi->hdmi_ctrl.drv_max_bits   = DISP_DATA_10BITS;
	return 0;
}

int _sunxi_hdmi_init_drv(struct sunxi_drm_hdmi *hdmi)
{
	int ret = -1;

	/* parse sunxi hdmi dts */
	ret = __sunxi_hdmi_init_dts(hdmi);
	if (ret != 0) {
		hdmi_err("sunxi hdmi init dts failed!!!\n");
		return -1;
	}

	/* sunxi hdmi resource alloc and enable */
	ret = __sunxi_hdmi_init_resource(hdmi);
	if (ret != 0) {
		hdmi_err("sunxi hdmi init resource failed!!!\n");
		return -1;
	}
	hdmi_inf("hdmi drv init resource done\n");

	/* sunxi hdmi config value init */
	ret = __sunxi_hdmi_init_value(hdmi);
	if (ret != 0) {
		hdmi_err("sunxi hdmi init config value failed\n");
	}

	/* sunxi hdmi core level init */
	hdmi->hdmi_core.dev = hdmi->dev;
	hdmi->hdmi_core.clock_src = 0x0;
	hdmi->hdmi_core.resistor_src = hdmi->hdmi_ctrl.drv_dts_res_src;
	ret = sunxi_hdmi_init(&hdmi->hdmi_core);
	if (ret != 0) {
		hdmi_err("sunxi hdmi init core failed!!!\n");
		return -1;
	}

	hdmi_inf("hdmi drv init done\n");
	return 0;
}

int sunxi_drm_hdmi_probe(struct udevice *dev)
{
	struct sunxi_drm_hdmi *hdmi = dev_get_priv(dev);
	int ret = 0;

	hdmi_inf("hdmi devices probe start >>>>>>>>>>\n");

	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);

	hdmi->dev = dev;
	hdmi->tcon_dev = sunxi_tcon_of_get_tcon_dev(dev);
	if (hdmi->tcon_dev == NULL) {
		hdmi_err("sunxi hdmi init get tcon device failed\n");
		goto failed_exit;
	}
	hdmi->tcon_id = sunxi_tcon_of_get_id(hdmi->tcon_dev);
	hdmi->tcon_top_id = sunxi_tcon_of_get_top_id(hdmi->tcon_dev);

	ret = _sunxi_hdmi_init_drv(hdmi);
	if (ret != 0) {
		hdmi_err("sunxi hdmi init drv fail\n");
		goto failed_exit;
	}

	sunxi_drm_connector_bind(&hdmi->connector, hdmi->dev, 0,
			&sunxi_hdmi_connector_funcs, NULL, DRM_MODE_CONNECTOR_HDMIA);

	hdmi_inf("hdmi devices probe end <<<<<<<<<<\n\n");
	of_periph_clk_config_setup(ofnode_to_offset(dev_ofnode(dev)));
	return 0;

failed_exit:
	hdmi_err("sunxi hdmi probe failed!!!!!\n");
	return -1;
}

const struct udevice_id sunxi_hdmi_match[] = {
	{ .compatible =	"allwinner,sunxi-hdmi" },
	{ }
};

U_BOOT_DRIVER(sunxi_drm_hdmi) = {
	.name      = "sunxi_drm_hdmi",
	.id        = UCLASS_DISPLAY,
	.probe     = sunxi_drm_hdmi_probe,
	.of_match  = sunxi_hdmi_match,
	.priv_auto_alloc_size = sizeof(struct sunxi_drm_hdmi),
};

static int sunxi_drm_hdmi_debug_cmd(struct cmd_tbl_s *cmdtp, int flag,
		int argc, char *const argv[])
{
	int ret = CMD_RET_USAGE, bit = 0;
	struct display_state *state = NULL;
	struct sunxi_drm_device *drm = sunxi_drm_device_get();
	struct sunxi_drm_connector *connector = NULL;
	struct udevice *dev = NULL;
	u32 param1 = 0, param2 = 0;

	sunxi_drm_for_each_display(state, drm) {
		connector = state->conn_state.connector;
		if (connector->type == DRM_MODE_CONNECTOR_HDMIA) {
			dev = connector->dev;
			break;
		}
	}

	if (IS_ERR_OR_NULL(dev)) {
		shdmi_err(dev);
		return CMD_RET_FAILURE;
	}

	if (!strcmp(argv[1], "hdmi_source"))
		ret = _sunxi_hdmi_sysfs_hdmi_source_show(dev);
	else if (!strcmp(argv[1], "hdmi_sink"))
		ret = _sunxi_hdmi_sysfs_hdmi_sink_show(dev);
	else if (!strcmp(argv[1], "pattern")) {
		if (argc == 3) {
			bit = simple_strtol(argv[2], NULL, 10);
			ret = _sunxi_hdmi_sysfs_pattern_store(dev, bit);
		}
	} else if (!strcmp(argv[1], "reg_bank")) {
		if (argc != 3) {
			ret = _sunxi_hdmi_sysfs_reg_bank_show(dev);
			goto exit;
		}
		bit = simple_strtol(argv[2], NULL, 10);
		ret = _sunxi_hdmi_sysfs_reg_bank_store(dev, bit);
	} else if (!strcmp(argv[1], "reg_read")) {
		if (argc == 4) {
			param1 = simple_strtol(argv[2], NULL, 0);
			param2 = simple_strtol(argv[3], NULL, 0);
			ret = _sunxi_hdmi_sysfs_reg_read_store(dev, param1, param2);
		}
	} else if (!strcmp(argv[1], "reg_write")) {
		if (argc == 4) {
			param1 = simple_strtol(argv[2], NULL, 0);
			param2 = simple_strtol(argv[3], NULL, 0);
			ret = _sunxi_hdmi_sysfs_reg_write_store(dev, param1, param2);
		}
	}

exit:
	return ret;
}

static char sunxi_drm_hdmi_help[] =
	"\n - hdmi_source: dump hdmi tx info"
	"\n - hdmi_sink  : dump hdmi rx info"
	"\n - pattern $index : debug pattern"
	"\n     - 0: disable pattern output"
	"\n     - 1: enable frame composer pattern output - red"
	"\n     - 2: enable frame composer pattern output - green"
	"\n     - 3: enable frame composer pattern output - blue"
	"\n     - 4: enable tcon colorbar(rgb) pattern output"
	"\n - reg_bank arg1: set hdmi reg read or write register bank"
	"\n     - 0: hdmi control register bank"
	"\n     - 1: hdmi extern phy register bank"
	"\n     - 2: hdmi scdc register bank"
	"\n - reg_read arg1 arg2 : read hdmi register"
	"\n     - arg1: read start register address"
	"\n     - arg2: read register count"
	"\n - reg_write arg1 arg2: write hdmi register"
	"\n     - arg1: write start register address"
	"\n     - arg2: write register value"
	;

U_BOOT_CMD(
	sunxi_hdmi20, 4, 1, sunxi_drm_hdmi_debug_cmd,
	"sunxi drm hdmi debug cmd",
	sunxi_drm_hdmi_help
);
