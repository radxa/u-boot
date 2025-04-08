/*
 * sunxi_drm_ofnode_helper/sunxi_drm_ofnode_helper.c
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
#include "sunxi_drm_helper_funcs.h"
#include <dm.h>
#include <drm/drm_connector.h>
#include <drm/drm_print.h>
#include <sunxi_power/power_manage.h>
#include <sys_config.h>

struct device_node *sunxi_of_graph_get_port_by_id(ofnode node, int id)
{
	ofnode ports, port;
	u32 reg;

	ports = ofnode_find_subnode(node, "ports");
	if (!ofnode_valid(ports))
		return NULL;

	ofnode_for_each_subnode(port, ports) {
		if (ofnode_read_u32(port, "reg", &reg))
			continue;

		if (reg == id)
			break;
	}

	if (reg == id)
		return (struct device_node *)ofnode_to_np(port);

	return NULL;
}


struct device_node *
sunxi_of_graph_get_endpoint_by_regs(ofnode node, int port, int endpoint)
{
	struct device_node *port_node;
	ofnode ep;
	u32 reg;

	port_node = sunxi_of_graph_get_port_by_id(node, port);
	if (!port_node)
		return NULL;

	ofnode_for_each_subnode(ep, np_to_ofnode(port_node)) {
		if (ofnode_read_u32(ep, "reg", &reg))
			break;
		if (reg == endpoint)
			break;
	}

	if (!ofnode_valid(ep))
		return NULL;

	return (struct device_node *)ofnode_to_np(ep);
}

struct device_node *
sunxi_of_graph_get_remote_node(ofnode node, int port, int endpoint)
{
	struct device_node *ep_node;
	ofnode ep;
	uint phandle;

	ep_node = sunxi_of_graph_get_endpoint_by_regs(node, port, endpoint);
	if (!ep_node)
		return NULL;

	if (ofnode_read_u32(np_to_ofnode(ep_node), "remote-endpoint", &phandle))
		return NULL;

	ep = ofnode_get_by_phandle(phandle);
	if (!ofnode_valid(ep))
		return NULL;

	return (struct device_node *)ofnode_to_np(ep);
}


struct device_node *sunxi_of_graph_get_port_parent(ofnode port)
{
	ofnode parent;
	int is_ports_node;

	parent = ofnode_get_parent(port);

	is_ports_node = strstr(ofnode_get_name(parent), "ports") ? 1 : 0;
	if (is_ports_node)
		parent = ofnode_get_parent(parent);

	return (struct device_node *)ofnode_to_np(parent);
}


struct device_node *sunxi_of_graph_get_remote_endpoint(struct device_node *endpoint)
{
	unsigned int phandle;
	ofnode ep, rep;
	struct device_node *remote_endpoint = NULL;

	ofnode_for_each_subnode(ep, np_to_ofnode(endpoint)) {
		//get remote ep
		if (ofnode_read_u32(ep, "remote-endpoint", &phandle))
			continue;

		rep = ofnode_get_by_phandle(phandle);
		if (!ofnode_valid(rep))
			continue;
		remote_endpoint = (struct device_node *)ofnode_to_np(rep);

		break;
	}

	return remote_endpoint;
}

int sunxi_of_get_irq_number(struct udevice *dev, u32 index)
{
	ofnode main_node;
	u32 *value = NULL;
	size_t size = (index + 1) * 3;
	int ret = -1, start_i = 0, irq_num = 0;

	value = malloc(size * sizeof(u32));
	if (!value) {
		return -1;
	}

	main_node = dev_ofnode(dev);
	if (!ofnode_valid(main_node)) {
		goto OUT;
	}
	start_i = index * 3;
	ret = ofnode_read_u32_array(main_node, "interrupts", value, size);
	if (!ret) {

		if (0 == value[start_i])
			irq_num = (value[start_i + 1] + 32);
		else
			irq_num = value[start_i + 1];
	} else
		irq_num = ret;

OUT:
	if (value) {
		free(value);
	}
	return irq_num;
}


int sunxi_of_get_panel_orientation(struct udevice *dev,
				 enum drm_panel_orientation *orientation)
{
	int ret;
	u32 rotation;

	ret = dev_read_u32(dev, "rotation", &rotation);
	if (ret == -EINVAL) {
		/* Don't return an error if there's no rotation property. */
		*orientation = DRM_MODE_PANEL_ORIENTATION_UNKNOWN;
		return 0;
	}

	if (ret < 0)
		return ret;

	if (rotation == 0)
		*orientation = DRM_MODE_PANEL_ORIENTATION_NORMAL;
	else if (rotation == 90)
		*orientation = DRM_MODE_PANEL_ORIENTATION_RIGHT_UP;
	else if (rotation == 180)
		*orientation = DRM_MODE_PANEL_ORIENTATION_BOTTOM_UP;
	else if (rotation == 270)
		*orientation = DRM_MODE_PANEL_ORIENTATION_LEFT_UP;
	else
		return -EINVAL;

	return 0;
}

int sunxi_clk_enable(struct clk **clk_array, u32 no_of_clk)
{
	int ret = 0, i = 0;

	for (i = 0; i < no_of_clk; ++i) {
		if (!IS_ERR_OR_NULL(clk_array[i])) {
			ret = clk_prepare_enable(clk_array[i]);
			if (ret != 0) {
				DRM_INFO("fail enable NO.%d clock!\n", i);
			}
		}
	}
	return 0;
}

int sunxi_clk_disable(struct clk **clk_array, u32 no_of_clk)
{
	int ret = 0, i = 0;

	for (i = 0; i < no_of_clk; ++i) {
		if (!IS_ERR_OR_NULL(clk_array[i])) {
			ret = clk_disable(clk_array[i]);
			if (ret != 0) {
				DRM_INFO("fail enable NO.%d clock!\n", i);
			}
		}
	}

	return 0;
}

/**
 * sunxi_drm_gpio_request() - request an gpio
 *
 *
 * @dev: which dev to request
 * @sub_name: gpio name in dev's dts
 * @return: gpio handle, 0 if fail
 */

ulong sunxi_drm_gpio_request(struct udevice *dev, char *sub_name)
{
	int ret = 0;
	user_gpio_set_t  gpio_info;
	user_gpio_set_t  tmp;

	ret = fdt_get_one_gpio_by_offset(ofnode_to_offset(dev_ofnode(dev)), sub_name, &tmp);

	if (ret > 0) {
		memcpy(&gpio_info, &tmp, sizeof(tmp));
		memcpy(gpio_info.gpio_name, sub_name, strlen(sub_name)+1);
		if (ret == 4) {
			//<&pio PH 16 GPIO_ACTIVE_HIGH>;
			//rearrange and fix
			gpio_info.mul_sel = 1;//output
			gpio_info.drv_level = 1;
			gpio_info.pull = 0;
			gpio_info.data = tmp.mul_sel;
		}
		return sunxi_gpio_request(&gpio_info, 1);
	}
	return 0;
}

ulong sunxi_drm_gpio_node_request(ofnode node, char *sub_name)
{
	int ret = 0;
	user_gpio_set_t  gpio_info;
	user_gpio_set_t  tmp;

	ret = fdt_get_one_gpio_by_offset(ofnode_to_offset(node), sub_name, &tmp);

	if (ret > 0) {
		memcpy(&gpio_info, &tmp, sizeof(tmp));
		memcpy(gpio_info.gpio_name, sub_name, strlen(sub_name)+1);
		if (ret == 4) {
			//<&pio PH 16 GPIO_ACTIVE_HIGH>;
			//rearrange and fix
			gpio_info.mul_sel = 1;//output
			gpio_info.drv_level = 1;
			gpio_info.pull = 0;
			gpio_info.data = tmp.mul_sel;
		}
		return sunxi_gpio_request(&gpio_info, 1);
	}
	return 0;
}
int sunxi_drm_gpio_set_value(ulong p_handler, u32 value)
{
	return gpio_write_one_pin_value(p_handler, value, NULL);
}

int sunxi_drm_power_enable(uint32_t phandle, unsigned int vol)
{
	int ret = 0, offset;
	char *name;

	offset = fdt_node_offset_by_phandle(working_fdt, phandle);
	if (offset <= 0) {
		DRM_ERROR("invalid power phandle, ret=%d\n", offset);
		return -1;
	}
	name = (char *)fdt_get_name(working_fdt, offset, NULL);
	if (!name) {
		DRM_ERROR("invalid power phandle, name not found\n");
		return -1;
	}
#if defined(CONFIG_SUNXI_PMU)
	/*TODO:bmu*/
	ret = pmu_set_voltage(name, 0, 1);
	if (ret < 0)
		DRM_ERROR("enable power %s, ret=%d\n", name, ret);
#ifdef CONFIG_SUNXI_PMU_EXT
	ret = pmu_ext_set_voltage(name, 0, 1);
	if (ret < 0)
		DRM_ERROR("enable power_ext %s, ret=%d\n", name, ret);
#endif
#else
	__wrn("SUNXI_POWER is not enabled!\n");
#endif

	return 0;
}

int sunxi_drm_power_disable(uint32_t phandle)
{
	int ret = 0, offset;
	char *name;

	offset = fdt_node_offset_by_phandle(working_fdt, phandle);
	if (offset <= 0) {
		DRM_ERROR("invalid power phandle, ret=%d\n", offset);
		return -1;
	}
	name = (char *)fdt_get_name(working_fdt, offset, NULL);
	if (!name) {
		DRM_ERROR("invalid power phandle, name not found\n");
		return -1;
	}
#if defined(CONFIG_SUNXI_PMU)
	/*TODO:bmu*/
	ret = pmu_set_voltage(name, 0, 0);
	if (ret < 0)
		DRM_ERROR("disable power %s, ret=%d\n", name, ret);
#ifdef CONFIG_SUNXI_PMU_EXT
	ret = pmu_ext_set_voltage(name, 0, 0);
	if (ret < 0)
		DRM_ERROR("disable power_ext %s, ret=%d\n", name, ret);
#endif
#else
	__wrn("SUNXI_POWER is not enabled!\n");
#endif

	return 0;
}

/* get phy from phandle, otherwise from phandle's parent node if fail */
int sunxi_phy_get_by_index(struct udevice *dev, int index,
			     struct phy *phy)
{
	struct ofnode_phandle_args args;
	struct phy_ops *ops;
	int ret;
	struct udevice *phydev;

	debug("%s(dev=%p, index=%d, phy=%p)\n", __func__, dev, index, phy);

	assert(phy);
	phy->dev = NULL;
	ret = dev_read_phandle_with_args(dev, "phys", "#phy-cells", 0, index,
					 &args);
	if (ret) {
		debug("%s: dev_read_phandle_with_args failed: err=%d\n",
		      __func__, ret);
		return ret;
	}

	ret = uclass_get_device_by_ofnode(UCLASS_PHY, args.node, &phydev);
	if (ret) {
		debug("%s: uclass_get_device_by_ofnode failed: err=%d node_offset:%ld\n",
		      __func__, ret, args.node.of_offset);
		ret = uclass_get_device_by_ofnode(UCLASS_PHY, ofnode_get_parent(args.node), &phydev);
		if (ret) {
			pr_err("%s: uclass_get_device_by_ofnode from parent failed: err=%d\n",
				__func__, ret);
			return ret;
		}
	}

	phy->dev = phydev;

	ops = (struct phy_ops *)phydev->driver->ops;

	if (ops->of_xlate) {
		ret = ops->of_xlate(phy, &args);
		if (ret) {
			debug("of_xlate() failed: %d\n", ret);
			goto err;
		}
	} else {
		if (args.args_count > 1) {
			debug("Invaild args_count: %d\n", args.args_count);
			return -EINVAL;
		}

		if (args.args_count)
			phy->id = args.args[0];
		else
			phy->id = 0;
	}

	return 0;

err:
	return ret;
}

int sunxi_phy_get_by_name(struct udevice *dev, const char *phy_name,
			    struct phy *phy)
{
	int index;

	debug("%s(dev=%p, name=%s, phy=%p)\n", __func__, dev, phy_name, phy);

	index = dev_read_stringlist_search(dev, "phy-names", phy_name);
	if (index < 0) {
		debug("dev_read_stringlist_search() failed: %d\n", index);
		return index;
	}

	return sunxi_phy_get_by_index(dev, index, phy);
}


//End of File
