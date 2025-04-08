// SPDX-License-Identifier: GPL-2.0
/*
 * ksc/ksc_drv/ksc_drv.c
 *
 * Copyright (c) 2007-2024 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
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


#include "ksc.h"


struct ksc_drv_info {
	bool inited;
	struct cdev *p_cdev;
	dev_t dev_id;
	struct class *p_class;
	struct device *p_device;
	struct platform_driver *p_ksc_driver;
	const struct file_operations *p_ksc_fops;
	atomic_t driver_ref_count;
	struct platform_device *p_plat_dev;
	struct ksc_device *p_ksc_device;
};

static struct ksc_drv_info g_drv_info;

static const struct ksc_drv_data ksc110_data = {
	.version = 0x110,
	.support_offline_ksc = false,
	.support_offline_up_scaler = false,
	.support_offline_arbitrary_angel_rotation = true,
	.support_offline_img_crop = false,
	.support_offline_img_flip = false,
};

static const struct ksc_drv_data ksc100_data = {
	.version = 0x100,
	.support_offline_ksc = true,
	.support_offline_up_scaler = true,
	.support_offline_arbitrary_angel_rotation = false,
	.support_offline_img_crop = true,
	.support_offline_img_flip = true,
};

static const struct sunxi_ksc_data_match sunxi_ksc_match[] = {
	{
		.compatible = "allwinner,ksc110", .data = &ksc110_data,
	},
	{
		.compatible = "allwinner,ksc100", .data = &ksc100_data,
	},
	{},
};

int do_ksc_show_info(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
#define SYS_BUF_SIZE 5120
	ssize_t count = 0;
	unsigned int i = 0;
	void *reg_base;
	char *buf = NULL;

	struct ksc_device *p_ksc = g_drv_info.p_ksc_device;
	if (!p_ksc) {
		dev_err(dev, "Null ksc_device\n");
		goto OUT;
	}

	buf = kmalloc(SYS_BUF_SIZE, GFP_KERNEL | __GFP_ZERO);
	if (!buf) {
		printf("malloc memory fail!\n");
		return -1;
	}

	count += sprintf(buf + count, "Enabled:%d irq:%d irq_no:%u be err:%d fe err:%d svp2ksc_dmt:%d de2ksc_data_volume_err:%d\n",
			 p_ksc->enabled, p_ksc->irq_cnt, p_ksc->irq_no,
			 p_ksc->be_err_cnt, p_ksc->fe_err_cnt, p_ksc->svp2ksc_dmt_err_cnt, p_ksc->de2ksc_data_volume_err_cnt);

	count += sprintf(buf + count, "\n");

	if (p_ksc->enabled == true) {
		count += sprintf(buf + count, "=========== ksc reg value ===========\n");
		reg_base = (void * __force)p_ksc->p_reg;
		for (i = 0; i < 60; i += 4) {
			count += sprintf(buf + count, "0x%.8lx: 0x%.8x 0x%.8x 0x%.8x 0x%.8x\n",
					 (unsigned long)reg_base, readl(reg_base), readl(reg_base + 4),
					 readl(reg_base + 8), readl(reg_base + 12));
			reg_base += 0x10;
		}

		printf("%s\n", buf);
		memset(buf, 0, SYS_BUF_SIZE);
		count = 0;

		count += sprintf(buf + count, "=========== ksc clk info ===========\n");
		for (i = 0; i < MAX_CLK_NUM; ++i) {
			if (!IS_ERR_OR_NULL(p_ksc->clks[i])) {
				count += sprintf(buf + count, "clk%d rate:%lu\n", i, clk_get_rate(p_ksc->clks[i]));
			}
		}

		count += sprintf(buf + count, "\n");

		count += sprintf(buf + count, "=========== ksc bandwidth control configuration info ===========\n");
		count += sprintf(buf + count, "bandwidth control Enabled: %d\n", p_ksc->ksc_para.bw.bw_ctrl_en);
		if (p_ksc->ksc_para.bw.bw_ctrl_en) {
			count += sprintf(buf + count, "bandwidth control num: %d\n", p_ksc->ksc_para.bw.bw_ctrl_num);
		}
	}
	printf("%s\n", buf);

	kfree(buf);
	return 0;
OUT:
	return count;
}

U_BOOT_CMD(
	ksc,	3,	0,	do_ksc_show_info,
	"show ksc status",
	"\nparameters  : NULL\n"
);

s32 get_ksc_match_data(void)
{
	char main_key[20], value[60];
	u32 i = 0;
	const char *str;

	sprintf(main_key, "/soc/ksc");
    if (fdt_getprop_string(working_fdt, fdt_path_offset(working_fdt, main_key),
							"compatible", (char **)&str) >= 0)
		memcpy((void *)value, str, strlen(str) + 1);
	KSC_WRN("%s's compatible: %s\n", main_key, value);

	for (i = 0; i < (sizeof(sunxi_ksc_match) / sizeof(struct sunxi_ksc_data_match)); i++) {
		if (strncmp(value, sunxi_ksc_match[i].compatible, 60) == 0) {
			KSC_WRN("match compatible: %s\n", sunxi_ksc_match[i].compatible);
			return i;
		}
	}


	return -1;
}

int ksc_init(void)
{
	// int ret;
	s32 match = -1;
	// char main_key[20], str[32];
	// /* node status */
	// sprintf(main_key, "/soc/ksc");
	// ret = disp_sys_script_get_item(main_key, "status", (int *)str, 2);
	// if (ret != 2 || strncmp(str, "okay", 10) != 0) {
	// 	KSC_ERR("fetch ksc err.\n");
	// 	return -1;
	// }

	/* get match data */
	match = get_ksc_match_data();
	if (match < 0) {
		KSC_ERR("Unable to match ksc data\n");
		return match;
	}

	g_drv_info.p_ksc_device->drv_data = sunxi_ksc_match[match].data;
	/* get ksc_device */
	g_drv_info.p_ksc_device = ksc_dev_init(g_drv_info.p_device);
	if (!g_drv_info.p_ksc_device) {
		KSC_ERR("ksc_dev_init fail!\n");
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(ksc_init);

int parse_dts_param(struct sunxi_ksc_para *para)
{
	int ret = 0;
	u64 bw_num = 0;
	struct ksc_device *p_ksc = g_drv_info.p_ksc_device;
	int node;
	node = fdt_path_offset(working_fdt, KSC_FDT_NODE);
	if (node < 0) {
		KSC_WRN("fdt_path_offset %s fail\n", KSC_FDT_NODE);
		ret = -1;
		return ret;
	}

	if (fdt_getprop_u32(working_fdt, node, "flip_h", (unsigned int *)&(para->flip_h)) <= 0) {
			KSC_ERR("Get flip_h property failed\n");
			ret = -1;
			para->flip_h = 0;
	}
	if (fdt_getprop_u32(working_fdt, node, "flip_v", (unsigned int *)&(para->flip_v)) <= 0) {
			KSC_ERR("Get flip_v property failed\n");
			ret = -1;
			para->flip_v = 0;
	}
	KSC_WRN("flip_h = %d, flip_v = %d\n", para->flip_h, para->flip_v);

	if (fdt_getprop_u32(working_fdt, node, "bw_ctrl_en",
			(unsigned int *)&(p_ksc->ksc_para.bw.bw_ctrl_en)) <= 0) {
		KSC_ERR("Get %s property failed\n", "bw_ctrl_en");
		ret = -1;
		p_ksc->ksc_para.bw.bw_ctrl_en = 0;
	}

	if (p_ksc->ksc_para.bw.bw_ctrl_en) {
		if (p_ksc->ksc_para.bw.fps && p_ksc->ksc_para.bw.clk_freq) {
			bw_num = DIV_ROUND_CLOSEST((u64)((u64)para->src_w *
						(u64)para->src_h *
						(u64)p_ksc->ksc_para.bw.fps * 32),
						(u64)p_ksc->ksc_para.bw.clk_freq)
						+ 2;
		} else
			KSC_ERR("invalid fps and clk_freq value!!!\n");

		if (fdt_getprop_u32(working_fdt, node, "bw_ctrl_num",
				(unsigned int *)&(p_ksc->ksc_para.bw.bw_ctrl_num)) <= 0) {
			KSC_ERR("Get %s property failed\n", "bw_ctrl_num");
			if (bw_num) {
				p_ksc->ksc_para.bw.bw_ctrl_num = bw_num;
			}
			ret = -1;
		}

		if (p_ksc->ksc_para.bw.bw_ctrl_num < bw_num) {
			KSC_ERR("invalid dts ksc node bw_ctrl_num value!!!\n");
		}

		// KSC_WRN("bw_ctrl_num = %d, bw_ctrl_num =  %d\n",
		// 		p_ksc->ksc_para.bw.bw_ctrl_en, p_ksc->ksc_para.bw.bw_ctrl_num);
	}
	return ret;
}

int ksc_online_enable(bool enable, enum ksc_pix_fmt in_fmt, int w, int h,
		      int bit_depth, int fps, int clk_freq)
{
	int ret = -1;
	struct sunxi_ksc_para para;

	if (g_drv_info.p_ksc_device) {
		g_drv_info.p_ksc_device->ksc_para.bw.fps = fps;
		g_drv_info.p_ksc_device->ksc_para.bw.clk_freq = clk_freq;
		ret = g_drv_info.p_ksc_device->enable(g_drv_info.p_ksc_device, enable);
		if (ret) {
			KSC_ERR("Enable %d ksc module fail:%d\n", enable, ret);
			goto OUT;
		}
		if (enable == true) {
			atomic_inc(&g_drv_info.driver_ref_count);
			memset(&para, 0, sizeof(struct sunxi_ksc_para));
			para.ksc_en = false;
			para.scaler_en = false;
			para.src_w = w;
			para.src_h = h;
			para.dns_w = w;
			para.dns_h = h;
			para.dst_w = w;
			para.dst_h = h;
			para.mode = ONLINE_MODE;
			para.pq_para.def_val_ch0 = 0;
			para.pq_para.def_val_ch1 = 0;
			para.pq_para.def_val_ch2 = 0;
			para.pq_para.def_val_ch3 = 0;
			para.bit_depth = bit_depth;
			para.lut.roi_x_max_non_ali = w - 1;
			para.lut.roi_y_max_non_ali = h - 1;
			if (g_drv_info.p_ksc_device->drv_data->version ==
			    0x110) {
				if (!IsRGB888(in_fmt) && !IsYUV444(in_fmt)) {
					KSC_ERR("ksc110 online mode only "
						     "support RGB888/YUV444:%u\n", in_fmt);
					in_fmt = RGB888;
				}
			}
			para.file_fmt = in_fmt;
			if (IsYUV420(in_fmt)) {
				para.pix_fmt = in_fmt;
			} else {
				para.pix_fmt = YUV422SP;
			}
			para.wb_fmt = in_fmt;
			parse_dts_param(&para);
			KSC_WRN("input size:[%u x %u] bpp:%u infmt:%u pixfmt:%u wbfmt:%u\n",
				 para.src_w, para.src_h, para.bit_depth,
				 para.file_fmt, para.pix_fmt, para.wb_fmt);
			ret = g_drv_info.p_ksc_device->set_ksc_para(g_drv_info.p_ksc_device, &para);
			if (ret) {
				KSC_ERR("ksc set_ksc_para fail!:%d\n", ret);
			}
		} else {
			atomic_dec(&g_drv_info.driver_ref_count);
		}
	} else {
		pr_err("KSC module not probe yet!\n");
	}

OUT:
	return ret;
}

EXPORT_SYMBOL_GPL(ksc_online_enable);

#if defined(__LINUX_PLAT__)
static int ksc_probe(struct platform_device *pdev)
{
	pdev->dev.dma_mask = &dmamask;

	if (g_drv_info.p_device) {
		g_drv_info.p_device->parent = &pdev->dev;
	}

	g_drv_info.p_ksc_device = ksc_dev_init(g_drv_info.p_device);

	if (!g_drv_info.p_ksc_device) {
		dev_err(&pdev->dev, "ksc_dev_init fail!\n");
		return -1;
	}

	/*pm_runtime_enable(&pdev->dev);*/

	return 0;
}

static int ksc_remove(struct platform_device *pdev)
{
	return 0;
}

static void ksc_shutdown(struct platform_device *pdev)
{
}


int ksc_open(struct inode *inode, struct file *file)
{
	atomic_inc(&g_drv_info.driver_ref_count);
	return  g_drv_info.p_ksc_device->enable(g_drv_info.p_ksc_device, true);
}

int ksc_release(struct inode *inode, struct file *file)
{
	if (!atomic_dec_and_test(&g_drv_info.driver_ref_count))
		return 0;

	return  g_drv_info.p_ksc_device->enable(g_drv_info.p_ksc_device, false);
}

static struct platform_driver ksc_driver = {
	.probe    = ksc_probe,
	.remove   = ksc_remove,
	.shutdown = ksc_shutdown,
	.driver   = {
		  .name           = "ksc",
		  .owner          = THIS_MODULE,
		  /*.pm             = &ksc_runtime_pm_ops,*/
		  .of_match_table = sunxi_ksc_match,
	},
};

static long ksc_ioctl(struct file *file, unsigned int cmd,
			      unsigned long arg)
{
	uint64_t karg[4];
	uint64_t ubuffer[4];
	int ret = 0;
	ret = copy_from_user((void *)karg, (void __user *)arg,
			4 * sizeof(uint64_t));
	if (ret) {
		dev_err(g_drv_info.p_device, "copy from user fail!\n");
		goto OUT;
	}
	ubuffer[0] = *(uint64_t *)karg;
	ubuffer[1] = *(uint64_t *)(karg + 1);
	ubuffer[2] = *(uint64_t *)(karg + 2);
	ubuffer[3] = *(uint64_t *)(karg + 3);

	switch (cmd) {
	case KSC_SET_PARA: {
		struct sunxi_ksc_para para;
		if (copy_from_user(&para, u64_to_user_ptr(ubuffer[0]),
				   sizeof(struct sunxi_ksc_para))) {
			dev_err(g_drv_info.p_device, "copy_from_user fail\n");
			ret = -EFAULT;
		} else
			ret = g_drv_info.p_ksc_device->set_ksc_para(g_drv_info.p_ksc_device, &para);
		break;
	}
	case KSC_GET_PARA: {
		struct sunxi_ksc_para *para = NULL;
		para = g_drv_info.p_ksc_device->get_ksc_para(g_drv_info.p_ksc_device);
		if (para) {
			if (copy_to_user(u64_to_user_ptr(ubuffer[0]), para,
					 sizeof(struct sunxi_ksc_para))) {
				dev_err(g_drv_info.p_device, "copy_to_user fail\n");
				ret = -EFAULT;
			}
		}
		break;
	}
	case KSC_ONLINE_ENABLE: {
		struct sunxi_ksc_online_para para;
		if (copy_from_user(&para, u64_to_user_ptr(ubuffer[0]),
				   sizeof(struct sunxi_ksc_online_para))) {
			dev_err(g_drv_info.p_device, "copy_from_user fail\n");
			ret = -EFAULT;
		} else
			ret = ksc_online_enable(para.enable, para.in_fmt, para.w, para.h, para.bit_depth);
		break;
	}

	default:
		dev_err(g_drv_info.p_device, "Unkown cmd :0x%x\n", cmd);
		break;
	}
OUT:
	return ret;
}


int ksc_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct ksc_buffers *buf = g_drv_info.p_ksc_device->get_ksc_buffers(g_drv_info.p_ksc_device);


	if (buf->lut_virt && buf->lut_phy_addr && buf->lut_size) {
		return dma_mmap_wc(g_drv_info.p_device, vma, buf->lut_virt,
					     (dma_addr_t)buf->lut_phy_addr,
					     (size_t)buf->lut_size);
	}

	return -ENOMEM;
}

static const struct file_operations ksc_fops = {
	.owner          = THIS_MODULE,
	.open           = ksc_open,
	.release        = ksc_release,
	.unlocked_ioctl = ksc_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl   = ksc_ioctl,
#endif
	.mmap           = ksc_mmap,
};

static int __init ksc_module_init(void)
{
	int err;

	alloc_chrdev_region(&g_drv_info.dev_id, 0, 1, "ksc");

	if (g_drv_info.p_cdev)
		cdev_del(g_drv_info.p_cdev);
	g_drv_info.p_cdev = cdev_alloc();
	if (!g_drv_info.p_cdev) {
		pr_err("cdev_alloc err\n");
		return -1;
	}
	g_drv_info.p_ksc_fops = &ksc_fops;
	cdev_init(g_drv_info.p_cdev, &ksc_fops);
	g_drv_info.p_cdev->owner = THIS_MODULE;
	err = cdev_add(g_drv_info.p_cdev, g_drv_info.dev_id, 1);
	if (err) {
		pr_err("cdev_add fail\n");
		goto OUT;
	}

	g_drv_info.p_class = class_create("ksc");
	if (IS_ERR_OR_NULL(g_drv_info.p_class)) {
		pr_err("class_create fail\n");
		goto OUT;
	}

	g_drv_info.p_device = device_create(g_drv_info.p_class, NULL, g_drv_info.dev_id, NULL, "ksc");
	if (IS_ERR_OR_NULL(g_drv_info.p_device)) {
		pr_err("device_create fail\n");
		goto OUT;
	}

	g_drv_info.p_ksc_driver = &ksc_driver;
	pr_err("%s finish\n", __func__);
	return platform_driver_register(g_drv_info.p_ksc_driver);

OUT:
	if (g_drv_info.p_cdev)
		cdev_del(g_drv_info.p_cdev);

	if (!IS_ERR_OR_NULL(g_drv_info.p_class))
		class_destroy(g_drv_info.p_class);
	return -1;

}

static void __exit ksc_module_exit(void)
{

}




module_init(ksc_module_init);
module_exit(ksc_module_exit);

MODULE_AUTHOR("AllWinner");
MODULE_IMPORT_NS(DMA_BUF);
MODULE_DESCRIPTION("KSC driver");
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ksc");
#endif

//End of File
