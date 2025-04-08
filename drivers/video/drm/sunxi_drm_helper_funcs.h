/*
 * sunxi_drm_helper_funcs.h
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
#ifndef _SUNXI_DRM_HELPER_FUNCS
#define _SUNXI_DRM_HELPER_FUNCS

#include <dm/ofnode.h>
#include <linux/types.h>
#include <clk/clk.h>
#include <reset.h>
#include <drm/drm_modes.h>
#include <generic-phy.h>

struct udevice;
enum uclass_id;
enum drm_panel_orientation;
struct device_node;

enum {
	PORT_DIR_IN,
	PORT_DIR_OUT,
};
int sunxi_ofnode_get_display_mode(ofnode node, struct drm_display_mode *mode, u32 *bus_flags);

struct device_node *
sunxi_of_graph_get_endpoint_by_regs(ofnode node, int port, int endpoint);

struct device_node *
sunxi_of_graph_get_remote_node(ofnode node, int port, int endpoint);

struct device_node *sunxi_of_graph_get_port_parent(ofnode port);

struct device_node *sunxi_of_graph_get_remote_endpoint(struct device_node *endpoint);

int sunxi_of_get_irq_number(struct udevice *dev, u32 index);

struct udevice *sunxi_of_graph_get_remote_device(enum uclass_id id, struct udevice dev);

int sunxi_of_get_panel_orientation(struct udevice *dev,
				 enum drm_panel_orientation *orientation);

struct device_node *sunxi_of_graph_get_port_by_id(ofnode node, int id);

int sunxi_clk_enable(struct clk **clk_array, u32 no_of_clk);

int sunxi_clk_disable(struct clk **clk_array, u32 no_of_clk);

ulong sunxi_drm_gpio_request(struct udevice *dev, char *sub_name);
ulong sunxi_drm_gpio_node_request(ofnode node, char *sub_name);

int sunxi_drm_gpio_set_value(ulong p_handler, u32 value);

int sunxi_drm_power_enable(uint32_t phandle, unsigned int vol);
int sunxi_drm_power_disable(uint32_t phandle);

int sunxi_phy_get_by_name(struct udevice *dev, const char *phy_name,
			    struct phy *phy);
#endif /*End of file*/
