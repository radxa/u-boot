// SPDX-License-Identifier: GPL-2.0+
/*
 * Generic dsi panel driver
 *
 * Copyright (C) 2023 Allwinner.
 *
 */


#include <common.h>
#include <dm.h>
#include <dm/uclass-internal.h>
//#include <dm/devres.h>
#include <backlight.h>
#include <power/regulator.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <drm/drm_modes.h>
#include <drm/drm_dsc.h>
#include <drm/drm_dsc_helper.h>
#include "../sunxi_drm_panel.h"
#include "../sunxi_drm_helper_funcs.h"
#include <drm/drm_mipi_dsi.h>
#include "../drm_mipi_dsi.h"

struct panel_cmd_header {
	u8 data_type;
	u8 delay;
	u8 payload_length;
} __packed;

struct panel_cmd_desc {
	struct panel_cmd_header header;
	u8 *payload;
};

struct panel_cmd_seq {
	struct panel_cmd_desc *cmds;
	unsigned int cmd_cnt;
};

struct gpio_timing {
	u32 level;
	u32 delay;
};

struct reset_sequence {
	u32 items;
	struct gpio_timing *timing;
};

struct panel_desc {

	struct videomode video_mode;
	struct {
		unsigned int width;
		unsigned int height;
	} size;

	struct panel_cmd_seq *init_seq;
	struct panel_cmd_seq *exit_seq;
	struct drm_dsc_config *dsc;
};

struct panel_dsi {
	struct sunxi_drm_panel panel;
	struct udevice *dev;
	struct mipi_dsi_device *dsi;
	struct device_node *node;
	struct panel_desc *desc;

	struct udevice *backlight;

	unsigned int power_num;
	unsigned int gpio_num;
	uint32_t supply[10];
	uint32_t avdd_supply;
	uint32_t avee_supply;
	unsigned int avdd_output_voltage;
	unsigned int avee_output_voltage;
	ulong enable_gpio[10];
	ulong reset_gpio;
	unsigned int reset_num;
	struct {
		unsigned int power;
		unsigned int enable;
		unsigned int reset;
	} delay;
	struct reset_sequence rst_on_seq;
	struct reset_sequence rst_off_seq;
	unsigned int pll_ss_permille;
};

struct panel_desc_dsi {
	struct panel_desc desc;
	unsigned long flags;
	enum mipi_dsi_pixel_format format;
	unsigned int lanes;
};
/*
static const struct drm_display_mode auo_b080uan01_mode = {
	.clock = 154500,
	.hdisplay = 1200,
	.hsync_start = 1200 + 62,
	.hsync_end = 1200 + 62 + 4,
	.htotal = 1200 + 62 + 4 + 62,
	.vdisplay = 1920,
	.vsync_start = 1920 + 9,
	.vsync_end = 1920 + 9 + 2,
	.vtotal = 1920 + 9 + 2 + 8,
	.vrefresh = 60,
};

static const struct panel_desc_dsi auo_b080uan01 = {
	.desc = {
		.modes = &auo_b080uan01_mode,
		.bpc = 8,
		.size = {
			.width = 108,
			.height = 272,
		},
	},
	.flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_CLOCK_NON_CONTINUOUS,
	.format = MIPI_DSI_FMT_RGB888,
	.lanes = 4,
};
*/
static inline struct panel_dsi *to_panel_dsi(struct sunxi_drm_panel *panel)
{
	return container_of(panel, struct panel_dsi, panel);
}

static void panel_dsi_sleep(unsigned int msec)
{
	mdelay(msec);
}
struct device_node *sunxi_of_get_panel_node(struct udevice *dev, int reg)
{
	struct device_node *ep_node, *panel_node;
	ofnode port;

	ep_node = sunxi_of_graph_get_remote_node(dev_ofnode(dev), 1, reg);
	if (!ep_node)
		return NULL;

	port = ofnode_get_parent(np_to_ofnode(ep_node));
	if (!ofnode_valid(port))
		return NULL;

	panel_node = sunxi_of_graph_get_port_parent(port);
	if (!panel_node)
		return NULL;

	return panel_node;
}

static int ofnode_find_device_by_phandle(enum uclass_id id,
					 ofnode node,
					 const char *name,
					 struct udevice **devp)
{
	struct udevice *dev;
	struct uclass *uc;
	int find_phandle;
	int ret;

	*devp = NULL;
	find_phandle = ofnode_read_u32_default(node, name, -1);
	if (find_phandle <= 0)
		return -ENOENT;
	ret = uclass_get(id, &uc);
	if (ret)
		return ret;

	list_for_each_entry(dev, &uc->dev_head, uclass_node) {
		uint phandle;

		phandle = dev_read_phandle(dev);

		if (phandle == find_phandle) {
			*devp = dev;
			return 0;
		}
	}

	return -ENODEV;
}

static int ofnode_get_device_by_phandle(enum uclass_id id, ofnode node,
				 const char *name, struct udevice **devp)
{
	struct udevice *dev;
	int ret;

	*devp = NULL;
	ret = ofnode_find_device_by_phandle(id, node, name, &dev);
	return uclass_get_device_tail(dev, ret, devp);
}
static int of_parse_reset_seq(struct panel_dsi *dsi_panel, ofnode node)
{
	int bytes, rc;
	const void *prop;
	u32 *p;

	prop = ofnode_get_property(node, "reset-on-sequence", &bytes);
	if (!prop)
		return -EINVAL;

	p = kzalloc(bytes, GFP_KERNEL);
	if (!p)
		return -ENOMEM;
	rc = ofnode_read_u32_array(node, "reset-on-sequence",
						p, bytes / 4);
	if (rc) {
		DRM_ERROR("parse reset-on-sequence failed\n");
		kfree(p);
		return rc;
	}

	dsi_panel->rst_on_seq.items = bytes / 8;
	dsi_panel->rst_on_seq.timing = (struct gpio_timing *)p;

	prop = ofnode_get_property(node, "reset-off-sequence", &bytes);
	if (!prop) {
		DRM_ERROR("reset-off-sequence property not found\n");
		return -EINVAL;
	}

	p = kzalloc(bytes, GFP_KERNEL);
	if (!p)
		return -ENOMEM;
	rc = ofnode_read_u32_array(node, "reset-off-sequence",
						p, bytes / 4);
	if (rc) {
		DRM_ERROR("parse reset-off-sequence failed\n");
		kfree(p);
		return rc;
	}

	dsi_panel->rst_off_seq.items = bytes / 8;
	dsi_panel->rst_off_seq.timing = (struct gpio_timing *)p;

	return 0;
}

static int panel_dsi_debug_timing(struct panel_dsi *dsi_panel)
{
	int panel_debug;
	static struct gpio_timing on_timing[] = {{0, 1000}, {1, 50}, {0, 50}, {1, 100}};
	static struct gpio_timing off_timing[] = {{0, 500}};

	panel_debug = env_get_hex("drm_debug", 0);

	if (panel_debug == DEBUG_TIMING) {
		dsi_panel->rst_on_seq.items = 4;
		dsi_panel->rst_on_seq.timing = on_timing;
		dsi_panel->rst_off_seq.items = 1;
		dsi_panel->rst_off_seq.timing = off_timing;
		dsi_panel->reset_num = 0;
	}

	return 0;

}
static int panel_dsi_parse_dt(struct panel_dsi *dsi_panel, ofnode node)
{
	char power_name[40] = {0};
	char power_vol_name[40] = {0};
	char gpio_name[40] = {0};
	int i;
	ulong ret;

	dsi_panel->power_num = 2;
	ofnode_read_u32(node, "power-num", &dsi_panel->power_num);
	for (i = 0; i < dsi_panel->power_num; i++) {
		dsi_panel->supply[i] = 0;
		snprintf(power_name, 40, "power%d-supply", i);
		if (ofnode_read_bool(node, power_name)) {
			ret = ofnode_read_u32(node, power_name, &dsi_panel->supply[i]);
			if (ret) {
				pr_err("failed to request regulator(%s): %d\n", power_name, ret);
			}
		}
	}

	strcpy(power_name, "avdd-supply");
	dsi_panel->avdd_supply = 0;
	if (ofnode_read_bool(node, power_name)) {
		ret = ofnode_read_u32(node, power_name, &dsi_panel->avdd_supply);
		if (ret) {
			pr_err("failed to request regulator(%s): %d\n", power_name, ret);
		}
	}

	strcpy(power_vol_name, "avdd-output-voltage");
	dsi_panel->avdd_output_voltage = 0;
	if (ofnode_read_bool(node, power_vol_name)) {
		ret = ofnode_read_u32(node, power_vol_name, &dsi_panel->avdd_output_voltage);
		if (ret) {
			pr_err("failed to request regulator_vol(%s): %d\n", power_vol_name, ret);
		}
	}

	strcpy(power_name, "avee-supply");
	dsi_panel->avee_supply = 0;
	if (ofnode_read_bool(node, power_name)) {
		ret = ofnode_read_u32(node, power_name, &dsi_panel->avee_supply);
		if (ret) {
			pr_err("failed to request regulator(%s): %d\n", power_name, ret);
		}
	}

	strcpy(power_vol_name, "avee-output-voltage");
	dsi_panel->avee_output_voltage = 0;
	if (ofnode_read_bool(node, power_vol_name)) {
		ret = ofnode_read_u32(node, power_vol_name, &dsi_panel->avee_output_voltage);
		if (ret) {
			pr_err("failed to request regulator_vol(%s): %d\n", power_vol_name, ret);
		}
	}

	dsi_panel->gpio_num = 3;
	ofnode_read_u32(node, "gpio-num", &dsi_panel->gpio_num);
	/* Get GPIOs and backlight controller. */
	for (i = 0; i < dsi_panel->gpio_num; i++) {
		snprintf(gpio_name, 40, "enable%d-gpios", i);
		if (ofnode_read_bool(node, gpio_name)) {
			ret = sunxi_drm_gpio_node_request(node, gpio_name);
			if (ret < 0) {
				pr_err("failed to request %s GPIO: %d\n", gpio_name, ret);
			} else
				dsi_panel->enable_gpio[i] = ret;
		}
	}

	snprintf(gpio_name, 40, "reset-gpios");
	if (ofnode_read_bool(node, gpio_name)) {
		ret = sunxi_drm_gpio_node_request(node, gpio_name);
		if (ret < 0) {
			pr_err("failed to request %s GPIO: %d\n", "reset", ret);
		} else
			dsi_panel->reset_gpio = ret;
	}

	snprintf(gpio_name, 40, "reset-gpios");
	ret = sunxi_drm_gpio_node_request(node, gpio_name);
	if (ret < 0) {
		pr_err("failed to request %s GPIO: %d\n", "reset", ret);
	} else
		dsi_panel->reset_gpio = ret;

	ofnode_read_u32(node, "pll-ss-permille", &dsi_panel->pll_ss_permille);
	ofnode_read_u32(node, "power-delay-ms", &dsi_panel->delay.power);
	ofnode_read_u32(node, "enable-delay-ms", &dsi_panel->delay.enable);
	ofnode_read_u32(node, "reset-delay-ms", &dsi_panel->delay.reset);
	if (of_parse_reset_seq(dsi_panel, node))
		ofnode_read_u32(node, "reset-num", &dsi_panel->reset_num);

	panel_dsi_debug_timing(dsi_panel);

	return 0;
}
static int panel_simple_parse_cmd_seq(struct udevice *dev,
				const u8 *data, int length,
				struct panel_cmd_seq *seq)
{
	struct panel_cmd_header *header;
	struct panel_cmd_desc *desc;
	char *buf, *d;
	unsigned int i, cnt, len;

	if (!seq)
		return -EINVAL;

	buf = devm_kzalloc(dev, length, __GFP_ZERO);
	if (buf)
		memcpy(buf, data, length);
	else
		return -ENOMEM;

	d = buf;
	len = length;
	cnt = 0;
	while (len > sizeof(*header)) {
		header = (struct panel_cmd_header *)d;

		d += sizeof(*header);
		len -= sizeof(*header);

		if (header->payload_length > len)
			return -EINVAL;

		d += header->payload_length;
		len -= header->payload_length;
		cnt++;
	}

	if (len)
		return -EINVAL;

	seq->cmd_cnt = cnt;
	seq->cmds = devm_kcalloc(dev, cnt, sizeof(*desc), GFP_KERNEL);
	if (!seq->cmds)
		return -ENOMEM;

	d = buf;
	len = length;
	for (i = 0; i < cnt; i++) {
		header = (struct panel_cmd_header *)d;
		len -= sizeof(*header);
		d += sizeof(*header);

		desc = &seq->cmds[i];
		desc->header = *header;
		desc->payload = (u8 *)d;

		d += header->payload_length;
		len -= header->payload_length;
	}

	return 0;
}

static int panel_of_get_desc_data(struct udevice *dev,
					struct panel_desc *desc, ofnode node)
{
	const void *data;
	const char *str = NULL;
	u32 value = 0;
	int len;
	int err;

	ofnode_read_u32(node, "width-mm", &desc->size.width);
	ofnode_read_u32(node, "height-mm", &desc->size.height);

	str = ofnode_read_string(node, "dsc,status");
	if (str && !strncmp(str, "okay", 4)) {
		desc->dsc = devm_kzalloc(dev, sizeof(*desc->dsc), GFP_KERNEL);
		if (!desc->dsc)
			return -ENOMEM;
		if (!ofnode_read_u32(node, "dsc,slice-height", &value))
			desc->dsc->slice_height = value;
		if (!ofnode_read_u32(node, "dsc,slice-width", &value))
			desc->dsc->slice_width = value;
		if (!ofnode_read_u32(node, "dsc,bits-per-component", &value))
			desc->dsc->bits_per_component = value;
		if (!ofnode_read_u32(node, "dsc,block-pred-enable", &value)) {
			if (value)
				desc->dsc->block_pred_enable = true;
			else
				desc->dsc->block_pred_enable = false;
		}

		desc->dsc->dsc_version_major = 0x1;
		desc->dsc->dsc_version_minor = 0x1;
		desc->dsc->slice_count = 2;
		desc->dsc->bits_per_pixel = 8 << 4;
		desc->dsc->line_buf_depth = 9;
	} else
		desc->dsc = NULL;

	data = ofnode_get_property(node, "panel-init-sequence", &len);
	if (data) {
		desc->init_seq = devm_kzalloc(dev, sizeof(*desc->init_seq),
					GFP_KERNEL);
		if (!desc->init_seq)
			return -ENOMEM;

		err = panel_simple_parse_cmd_seq(dev, data, len,
						 desc->init_seq);
		if (err) {
			dev_err(dev, "failed to parse init sequence\n");
			return err;
		}
	}

	data = ofnode_get_property(node, "panel-exit-sequence", &len);
	if (data) {
		desc->exit_seq = devm_kzalloc(dev, sizeof(*desc->exit_seq),
					GFP_KERNEL);
		if (!desc->exit_seq)
			return -ENOMEM;

		err = panel_simple_parse_cmd_seq(dev, data, len,
						desc->exit_seq);
		if (err) {
			dev_err(dev, "failed to parse exit sequence\n");
			return err;
		}
	}

	return 0;
}

static int panel_dsi_of_get_desc_data(struct udevice *dev,
					struct panel_desc_dsi *desc, ofnode node)
{
	u32 val;
	int ret;

	ret = panel_of_get_desc_data(dev, &desc->desc, node);
	if (ret)
		return ret;

	if (!ofnode_read_u32(node, "dsi,flags", &val))
		desc->flags = val;
	if (!ofnode_read_u32(node, "dsi,format", &val))
		desc->format = val;
	if (!ofnode_read_u32(node, "dsi,lanes", &val))
		desc->lanes = val;

	return 0;
}
static int panel_dsi_cmd_seq(struct panel_dsi *dsi_panel,
		struct panel_cmd_seq *seq)
{
	struct mipi_dsi_device *dsi = dsi_panel->dsi;
	unsigned int i;
	int err;
#ifdef DSI_RX_DEBUG
	u32 j;
	u8 value[100];
#endif
	if (!seq)
		return -EINVAL;

	for (i = 0; i < seq->cmd_cnt; i++) {
		struct panel_cmd_desc *cmd = &seq->cmds[i];

		switch (cmd->header.data_type) {
		case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
		case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
		case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
		case MIPI_DSI_GENERIC_LONG_WRITE:
			err = mipi_dsi_generic_write(dsi, cmd->payload,
						cmd->header.payload_length);
			break;
		case MIPI_DSI_DCS_SHORT_WRITE:
		case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
		case MIPI_DSI_DCS_LONG_WRITE:
			err = mipi_dsi_dcs_write_buffer(dsi, cmd->payload,
					cmd->header.payload_length);
			break;
		default:
			return -EINVAL;
		}

		if (err < 0)
			dev_err(dsi_panel->dev, "failed to write dcs cmd: %d\n", err);

		if (cmd->header.delay)
			panel_dsi_sleep(cmd->header.delay);
#ifdef DSI_RX_DEBUG
		panel_dsi_sleep(500);
		mipi_dsi_dcs_read(dsi, cmd->payload[0], value, cmd->header.payload_length - 1);
		printf("%x: ", cmd->payload[0]);
		for (j = 0; j < cmd->header.payload_length - 1; j++)
			printf("%x  ", value[j]);
		printf("\n=============\n");
#endif
	}

	return 0;
}

static int panel_dsi_disable(struct sunxi_drm_panel *panel)
{
	struct panel_dsi *dsi = to_panel_dsi(panel);

	if (dsi->backlight)
		backlight_disable(dsi->backlight);

	return 0;
}
static void panel_dsi_power_disable(struct panel_dsi *dsi_panel)
{
	struct gpio_timing *timing;
	int i, items;

	for (i = dsi_panel->gpio_num; i > 0; i--) {
		if (dsi_panel->enable_gpio[i - 1]) {
			sunxi_drm_gpio_set_value(dsi_panel->enable_gpio[i - 1], 0);
			if (dsi_panel->delay.enable)
				panel_dsi_sleep(dsi_panel->delay.enable);
		}
	}
	if (dsi_panel->reset_gpio) {
		items = dsi_panel->rst_off_seq.items;
		timing = dsi_panel->rst_off_seq.timing;
		for (i = 0; i < items; i++) {
			sunxi_drm_gpio_set_value(dsi_panel->reset_gpio, timing[i].level);
			panel_dsi_sleep(timing[i].delay);
		}
	}

	if (dsi_panel->reset_num) {
		if (dsi_panel->reset_gpio)
			sunxi_drm_gpio_set_value(dsi_panel->reset_gpio, 0);
		if (dsi_panel->delay.reset)
			panel_dsi_sleep(dsi_panel->delay.reset);
	}

	if (dsi_panel->avee_supply) {
		sunxi_drm_power_disable(dsi_panel->avee_supply);
		if (dsi_panel->delay.power)
			panel_dsi_sleep(dsi_panel->delay.power);
	}
	if (dsi_panel->avdd_supply) {
		sunxi_drm_power_disable(dsi_panel->avdd_supply);
		if (dsi_panel->delay.power)
			panel_dsi_sleep(dsi_panel->delay.power);
	}

	for (i = dsi_panel->power_num; i > 0; i--) {
		if (dsi_panel->supply[i - 1]) {
			sunxi_drm_power_disable(dsi_panel->supply[i - 1]);
			if (dsi_panel->delay.power)
				panel_dsi_sleep(dsi_panel->delay.power);
		}
	}
}

static int panel_dsi_unprepare(struct sunxi_drm_panel *panel)
{
	struct panel_dsi *dsi_panel = to_panel_dsi(panel);
	int ret = 0;

	if (dsi_panel->desc->exit_seq)
		if (dsi_panel->dsi)
			panel_dsi_cmd_seq(dsi_panel, dsi_panel->desc->exit_seq);

	if (dsi_panel->desc->dsc) {
		ret = mipi_dsi_dcs_set_display_off(dsi_panel->dsi);
		if (ret < 0) {
			dev_err(dsi_panel->dev, "Failed to set display off: %d\n", ret);
			return ret;
		}
		panel_dsi_sleep(20);

		ret = mipi_dsi_dcs_enter_sleep_mode(dsi_panel->dsi);
		if (ret < 0) {
			dev_err(dsi_panel->dev, "Failed to enter sleep mode: %d\n", ret);
			return ret;
		}
		panel_dsi_sleep(120);
	}

	panel_dsi_power_disable(dsi_panel);

	return 0;
}

static int panel_dsi_uboot_reset(struct sunxi_drm_panel *panel)
{
	struct panel_dsi *dsi_panel = to_panel_dsi(panel);
	struct gpio_timing *timing;
	int i, items;

	if (dsi_panel->desc->exit_seq)
		if (dsi_panel->dsi)
			panel_dsi_cmd_seq(dsi_panel, dsi_panel->desc->exit_seq);

	for (i = dsi_panel->gpio_num - 1; i >= 0; i--) {
		if (dsi_panel->enable_gpio[i]) {
			sunxi_drm_gpio_set_value(dsi_panel->enable_gpio[i], 0);
			if (dsi_panel->delay.enable)
				panel_dsi_sleep(dsi_panel->delay.enable);
		}
	}
	if (dsi_panel->reset_gpio) {
		items = dsi_panel->rst_off_seq.items;
		timing = dsi_panel->rst_off_seq.timing;
		for (i = 0; i < items; i++) {
			sunxi_drm_gpio_set_value(dsi_panel->reset_gpio, timing[i].level);
			panel_dsi_sleep(timing[i].delay);
		}
	}

	if (dsi_panel->reset_num) {
		if (dsi_panel->reset_gpio)
			sunxi_drm_gpio_set_value(dsi_panel->reset_gpio, 0);
		if (dsi_panel->delay.reset)
			panel_dsi_sleep(dsi_panel->delay.reset);
	}

	return 0;
}

static int panel_dsi_power_enable(struct panel_dsi *dsi_panel)
{
	int err = 0, i, items;
	struct gpio_timing *timing;

	for (i = 0; i < dsi_panel->power_num; i++) {
		if (dsi_panel->supply[i]) {
			err = sunxi_drm_power_enable(dsi_panel->supply[i], 0);
			if (err < 0) {
				dev_err(dsi_panel->dev, "failed to enable supply%d: %d\n",
					i, err);
				return err;
			}
			if (dsi_panel->delay.power)
				panel_dsi_sleep(dsi_panel->delay.power);
		}
	}

	if (dsi_panel->avdd_supply) {
		err = sunxi_drm_power_enable(dsi_panel->avdd_supply, dsi_panel->avdd_output_voltage);
		if (err < 0) {
			dev_err(dsi_panel->dev, "failed to enable avdd-supply: %d\n", err);
			return err;
		}
	}

	if (dsi_panel->avee_supply) {
		err = sunxi_drm_power_enable(dsi_panel->avee_supply, dsi_panel->avee_output_voltage);
		if (err < 0) {
			dev_err(dsi_panel->dev, "failed to enable avee-supply: %d\n",
					err);
			return err;
		}
	}

	for (i = 0; i < dsi_panel->gpio_num; i++) {
		if (dsi_panel->enable_gpio[i]) {
			sunxi_drm_gpio_set_value(dsi_panel->enable_gpio[i], 1);

			if (dsi_panel->delay.enable)
				panel_dsi_sleep(dsi_panel->delay.enable);
		}
	}

	for (i = 0; i < dsi_panel->reset_num; i++) {
		if (dsi_panel->reset_gpio)
			sunxi_drm_gpio_set_value(dsi_panel->reset_gpio, 0);
		if (dsi_panel->delay.reset)
			panel_dsi_sleep(dsi_panel->delay.reset);
		if (dsi_panel->reset_gpio)
			sunxi_drm_gpio_set_value(dsi_panel->reset_gpio, 1);
		if (dsi_panel->delay.reset)
			panel_dsi_sleep(dsi_panel->delay.reset);
	}

	if (dsi_panel->reset_gpio) {
		items = dsi_panel->rst_on_seq.items;
		timing = dsi_panel->rst_on_seq.timing;
		for (i = 0; i < items; i++) {
			sunxi_drm_gpio_set_value(dsi_panel->reset_gpio, timing[i].level);
			panel_dsi_sleep(timing[i].delay);
		}
	}

	return 0;
}

static int sunxi_dsc_panel_enable(struct panel_dsi *dsi_panel)
{
	struct mipi_dsi_device *dsi = dsi_panel->dsi;
	struct drm_dsc_picture_parameter_set pps;
	int ret;
//	void *value = NULL;

	mipi_dsi_compression_mode(dsi, true);
	drm_dsc_pps_payload_pack(&pps, dsi->dsc);
	ret = mipi_dsi_picture_parameter_set(dsi, &pps);
	if (ret) {
		dev_err(&dsi->dev, "Failed to set PPS\n");
		return ret;
	}
/*	value = &pps;
	for (i = 0; i < 100; i++)
		printk("****%d: 0x%x ", i , *((u8 *)value + i));
*/
	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(&dsi->dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}
	panel_dsi_sleep(120);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		dev_err(&dsi->dev, "Failed on set display on: %d\n", ret);
		return ret;
	}
	panel_dsi_sleep(20);

	return 0;
}

static int panel_dsi_prepare(struct sunxi_drm_panel *panel)
{
	struct panel_dsi *dsi_panel = to_panel_dsi(panel);

	panel_dsi_power_enable(dsi_panel);
	if (dsi_panel->desc->init_seq)
		if (dsi_panel->dsi)
			panel_dsi_cmd_seq(dsi_panel, dsi_panel->desc->init_seq);

	if (dsi_panel->desc->dsc)
		sunxi_dsc_panel_enable(dsi_panel);

	return 0;
}

static int panel_dsi_enable(struct sunxi_drm_panel *panel)
{
	struct panel_dsi *dsi = to_panel_dsi(panel);

	if (dsi->backlight)
		backlight_enable(dsi->backlight);

	return 0;
}

static int panel_get_timing_from_dts(ofnode node,
				       struct drm_display_mode *mode)
{
	struct ofnode_phandle_args args;
	ofnode dt, timing;
	u32 bus_flags = 0, value = 0;
	int ret;

	dt = ofnode_find_subnode(node, "display-timings");
	if (ofnode_valid(dt)) {
		ret = ofnode_parse_phandle_with_args(dt, "native-mode", NULL,
						     0, 0, &args);
		if (ret)
			return ret;

		timing = args.node;
	} else {
		timing = ofnode_find_subnode(node, "panel-timing");
	}

	if (!ofnode_valid(timing)) {
		DRM_ERROR("failed to get display timings from DT\n");
		return -ENXIO;
	}
	sunxi_ofnode_get_display_mode(timing, mode, &bus_flags);
	if (!ofnode_read_u32(node, "width-mm", &value))
		mode->width_mm = value;
	if (!ofnode_read_u32(node, "height-mm", &value))
		mode->height_mm = value;
	mode->type |= DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;

	return 0;
}

static int panel_dsi_get_modes(struct sunxi_drm_panel *panel, struct drm_display_mode *mode)
{
	struct panel_dsi *dsi_panel = to_panel_dsi(panel);
	struct panel_desc_dsi *desc = NULL;
	struct panel_desc_dsi *d;
	struct device_node *node = NULL;
	const u8 *data;
	int len, i = 0, ret;
	u8 id_reg, id_value, value = 0;

	node = sunxi_of_get_panel_node(dsi_panel->dev, 1);
	if (!node) {
		node = sunxi_of_get_panel_node(dsi_panel->dev, 0);
		panel_dsi_parse_dt(dsi_panel, np_to_ofnode(node));
	} else
		while (true) {
			node = sunxi_of_get_panel_node(dsi_panel->dev, i);
			if (!node)
				break;
			if (!ofnode_is_available(np_to_ofnode(node))) {
				i++;
				continue;
			}

			panel_dsi_parse_dt(dsi_panel, np_to_ofnode(node));
			panel_dsi_power_enable(dsi_panel);

			data = ofnode_get_property(np_to_ofnode(node), "panel-id-value", &len);
			if (!data || len != 2) {
				panel_dsi_power_disable(dsi_panel);
				dev_err(dsi_panel->dev, "Failed to get panel-id-value property\n");
				break;
			}
			id_reg = data[0];
			id_value = data[1];
			panel_dsi_sleep(50);
			mipi_dsi_dcs_read(dsi_panel->dsi, id_reg, &value, 1);
			printf("dsi rx panel id 0x%x: 0x%x\n", id_reg, value);
			panel_dsi_power_disable(dsi_panel);
			if (value == id_value)
				break;
			i++;
		}
	if (!node)
		return -1;
	dsi_panel->node = node;

	ret = ofnode_get_device_by_phandle(UCLASS_PANEL_BACKLIGHT, np_to_ofnode(node),
			"backlight", &dsi_panel->backlight);
	if (ret)
		DRM_ERROR("%s: Cannot get backlight: %d\n", __func__, ret);

//	desc = (struct panel_desc_dsi *)dev_get_driver_data(dsi_panel->dev);
	if (!desc) {
		d = devm_kzalloc(dsi_panel->dev, sizeof(*d), GFP_KERNEL);
		if (!d)
			return -ENOMEM;
		ret = panel_dsi_of_get_desc_data(dsi_panel->dev, d, np_to_ofnode(node));
		if (ret) {
			dev_err(dsi_panel->dev, "failed to get desc data: %d\n", ret);
			return ret;
		}
		desc = d;
	}
	dsi_panel->desc = &desc->desc;
	dsi_panel->dsi->dsc = dsi_panel->desc->dsc;;
	dsi_panel->dsi->mode_flags = desc->flags;
	dsi_panel->dsi->format = desc->format;
	dsi_panel->dsi->lanes = desc->lanes;
	dsi_panel->dsi->out_reg = i;
	mipi_dsi_attach(dsi_panel->dsi);

	panel_get_timing_from_dts(np_to_ofnode(node), mode);

	return 0;
}
static const struct sunxi_drm_panel_funcs panel_dsi_funcs = {
	.unprepare = panel_dsi_unprepare,
	.disable = panel_dsi_disable,
	.prepare = panel_dsi_prepare,
	.enable = panel_dsi_enable,
	.get_mode = panel_dsi_get_modes,
	.uboot_reset = panel_dsi_uboot_reset,
//	.get_timings = panel_dsi_get_timings,
};

static const struct udevice_id dsi_of_match[] = {
	{
		.compatible = "allwinner,virtual-panel",
	},
/*	{
		.compatible = "auo,b080uan01",
		.data = &auo_b080uan01
	}, */
	{ /* Sentinel */ },
};
static int panel_dsi_probe(struct udevice *dev)
{
	struct panel_dsi *dsi_panel = dev_get_priv(dev);
	struct mipi_dsi_device *dsi = dev_get_parent_platdata(dev);
	struct sunxi_drm_panel *panel = &dsi_panel->panel;

	DRM_INFO("[DSI-PANEL] panel_dsi_probe start\n");
	dsi_panel->dev = dev;
	dsi_panel->dsi = dsi;
	dev->driver_data = (ulong)panel;
	panel->dev = dev;
	panel->funcs = &panel_dsi_funcs;
	sunxi_of_get_panel_orientation(dsi_panel->dev, &dsi_panel->panel.orientation);

	DRM_INFO("[DSI-PANEL] panel_dsi_probe finish\n");

	return 0;
}



U_BOOT_DRIVER(panel_dsi) = {
	.name	= "panel_dsi",
	.id	= UCLASS_PANEL,
	.of_match = dsi_of_match,
	.probe	= panel_dsi_probe,
	.priv_auto_alloc_size = sizeof(struct panel_dsi),
};
