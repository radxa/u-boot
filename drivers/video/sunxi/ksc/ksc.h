// SPDX-License-Identifier: GPL-2.0
/*
 * ksc.h
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
#ifndef _KSC_H
#define _KSC_H
#include <common.h>
#include <clk/clk.h>
#include <memalign.h>
#include <linux/list.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include "sys_config.h"
#include <fdt_support.h>
#include <linux/compat.h>
#include <linux/errno.h>
#include <fs.h>
#include <search.h>
#include <asm/atomic.h>
#include "ksc_macro_def.h"
#include "../disp2/disp/disp_sys_intf.h"
#include <sunxi_ksc.h>
#include "../bootGUI/video_misc_hal.h"

#define KSC_FDT_NODE "/soc/ksc"

enum ksc_irq_state {
	KSC_IRQ_INVALID = 0,
	KSC_IRQ_BE_FINISH = 0x1,
	KSC_IRQ_BE_ERR = 0x2,
	KSC_IRQ_FE_ERR = 0x4,
	KSC_IRQ_LUT_OVER_SPAN = 0x8,
	SVP2KSC_DMT_ERR = 0x10,
	KSC_IRQ_DE2KSC_DATA_VOLUME_ERR = 0x20,
};

struct ksc_drv_data {
	int version;
	bool support_offline_ksc;
	bool support_offline_up_scaler;
	bool support_offline_arbitrary_angel_rotation;
	bool support_offline_img_crop;
	bool support_offline_img_flip;
};

struct sunxi_ksc_data_match {
	const char *compatible;
	const struct ksc_drv_data *data;
};

struct dmabuf_item {
	struct list_head list;
	int fd;
	struct dma_buf *buf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	uintptr_t dma_addr;
	struct device *p_device;
};

/**
 * description here
 */
struct ksc_freeing_buffers {
	void *mem0_virt;
	uintptr_t mem0_phy_addr;
	void *mem1_virt;
	uintptr_t mem1_phy_addr;
	unsigned int mem_size;
};

struct ksc_buffers {
	bool alloced;
	struct device *p_device;

	void *mem0_virt;
	uintptr_t mem0_phy_addr;
	void *mem1_virt;
	uintptr_t mem1_phy_addr;
	unsigned int mem_size;
	unsigned int chm_offset;
	unsigned int pix_2b_offset;

	void *lut_virt;
	uintptr_t lut_phy_addr;
	unsigned int lut_size;

	void *lut_mem0_virt;
	uintptr_t lut_mem0_phy_addr;
	void *lut_mem1_virt;
	uintptr_t lut_mem1_phy_addr;

	struct dmabuf_item *src_item;
	struct dmabuf_item *dst_item;

	struct ksc_freeing_buffers buf_to_be_free;
};

/**
 * description here
 */
struct ksc_band_width_info {
	unsigned int bw_ctrl_en;
	unsigned int bw_ctrl_num;
	int fps;
	int clk_freq;
};

/**
 * ksc_para
 */
struct ksc_para {
	struct sunxi_ksc_para para;
	struct ksc_buffers buf;
	bool is_first_frame;
	struct ksc_band_width_info bw;
};

/**
 * ksc device
 */
struct ksc_device {
	bool enabled;
	struct mutex dev_lock;
	struct device *p_device;
	struct device attr_device;
	// struct attribute_group *p_attr_grp;

	struct ksc_regs *p_reg;
	void *tv_disp_top_reg;
	unsigned int irq_no;
	struct clk *clks[MAX_CLK_NUM];
	struct reset_control *rsts[MAX_CLK_NUM];
	wait_queue_head_t event_wait;
	const struct ksc_drv_data *drv_data;
	unsigned int clk_freq[MAX_CLK_NUM];
	bool offline_finish_flag;
	int be_err_cnt;
	int fe_err_cnt;
	int de2ksc_data_volume_err_cnt;
	int svp2ksc_dmt_err_cnt;
	int irq_cnt;
	// struct delayed_work free_buffer_work;

	struct ksc_para ksc_para;

	int (*enable)(struct ksc_device *p_ksc, bool enable);
	int (*set_ksc_para)(struct ksc_device *p_ksc, struct sunxi_ksc_para *para);
	struct ksc_buffers *(*get_ksc_buffers)(struct ksc_device *p_ksc);
	struct sunxi_ksc_para *(*get_ksc_para)(struct ksc_device *p_ksc);
};

struct ksc_device *ksc_dev_init(struct device *dev);
int get_ksc_fdt_node(void);
int ksc_add_kernel_iova_premap(unsigned long address, unsigned int length);
int ksc_save_string_to_kernel(char *name, char *str);
bool is_reg_update_finish(struct ksc_regs *p_reg);
int ksc_reg_update_all(struct ksc_regs *p_reg, struct ksc_para *para);
int ksc_reg_get_version(struct ksc_regs *p_reg);
int ksc_reg_switch_lut_buf(struct ksc_regs *p_reg, struct ksc_para *para);
int ksc_reg_irq_query(struct ksc_regs *p_reg);
bool is_mem_sync_finish(struct ksc_regs *p_reg);
int tvdisp_ksc_enable(void *tvdisp_reg, bool enable);

#endif /*End of file*/
