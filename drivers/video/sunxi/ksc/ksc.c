// SPDX-License-Identifier: GPL-2.0
/*
 * ksc/ksc.c
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

#include "ksc.h"
#include "ksc_mem.h"
#include <pram_mem_alloc.h>
#include "ksc_reg/ksc110/ksc_reg.h"
struct ksc_buffers *ksc_buf;

uintptr_t ksc_getprop_regbase(char *main_name, char *sub_name)
{
	char compat[32];
	u32 len = 0;
	int node;
	int ret = -1;
	int value[2] = {0};
	uintptr_t reg_base = 0;

	len = sprintf(compat, "%s", main_name);
	if (len > 32)
		KSC_WRN("size of mian_name is out of range\n");

	node = fdt_path_offset(working_fdt, compat);
	if (node < 0) {
		KSC_WRN("fdt_path_offset %s fail\n", compat);
		goto exit;
	}

	ret = fdt_getprop_u32(working_fdt, node, sub_name, (uint32_t *)value);
	if (ret < 0)
		KSC_WRN("fdt_getprop_u32 %s.%s fail\n", main_name, sub_name);
	else {
		reg_base = value[0] + value[1];
	}
	KSC_INFO("reg_base = 0x%x\n", reg_base);
exit:
	return reg_base;
}

u32 ksc_getprop_irq(char *main_name, char *sub_name)
{
	char compat[32];
	u32 len = 0;
	int node;
	int ret = -1;
	int value[32] = {0};
	u32 irq = 0;

	len = sprintf(compat, "%s", main_name);
	if (len > 32)
		KSC_WRN("size of mian_name is out of range\n");

	node = fdt_path_offset(working_fdt, compat);
	if (node < 0) {
		KSC_WRN("fdt_path_offset %s fail\n", compat);
		goto exit;
	}

	ret = fdt_getprop_u32(working_fdt, node, sub_name, (uint32_t *)value);
	if (ret < 0)
		KSC_WRN("fdt_getprop_u32 %s.%s fail\n", main_name, sub_name);
	else
		irq = value[1];

exit:
	return irq;
}

int get_ksc_fdt_node(void)
{
	int node = -1;

	/* notice: make sure we use the only one nodt "disp". */
	node = fdt_path_offset(working_fdt, KSC_FDT_NODE);
	if (node < 0) {
		KSC_ERR("%s : %s\n", __FUNCTION__, node);
		return node;
	}
	assert(node >= 0);
	return node;
}

int ksc_save_string_to_kernel(char *name, char *str)
{
	int ret = -1;

#ifndef CONFIG_SUNXI_MULITCORE_BOOT
	int node = get_ksc_fdt_node();
	if (node < 0) {
		return ret;
	}
	ret = fdt_setprop_string(working_fdt, node, name, str);
#else
	ret = sunxi_fdt_getprop_store_string(working_fdt,
		KSC_FDT_NODE, name, str);
#endif
	return ret;
}

int ksc_add_kernel_iova_premap(unsigned long address, unsigned int length)
{
	int ret;

	int node = fdt_path_offset(working_fdt, "/soc/ksc");
	if (node < 0) {
		printf("Can't find /soc/ksc node in DTB\n");
		return node;
	}
	ret = fdt_appendprop_u64(working_fdt, node, "sunxi-iova-premap", address);
	if (ret) {
		printf("%s fail ret%d\n", __FUNCTION__, ret);
		return ret;
	}
	return ret;
}

int hal_reserve_ksc_mem_to_kernel(void)
{
	int ret;
	struct ksc_buffers *buf = ksc_buf;
	KSC_INFO("mem_size = %d\n", buf->mem_size);
	/* set reserve memory */
	ret = fdt_add_mem_rsv(working_fdt, (uint64_t)(buf->mem0_phy_addr), buf->mem_size);
	if (ret) {
		KSC_ERR("##add mem0_phy_addr rsv error: %s : %s\n",
				__func__, fdt_strerror(ret));
		return ret;
	}
	ret = fdt_add_mem_rsv(working_fdt, (uint64_t)(buf->mem1_phy_addr), buf->mem_size);
	if (ret) {
		KSC_ERR("##add mem1_phy_addr rsv error: %s : %s\n",
				__func__, fdt_strerror(ret));
		return ret;
	}
	return ret;
}

int ksc_update_param_to_kernel(void)
{
	int ret;
	struct ksc_buffers *buf = ksc_buf;
	char ksc_mem_phy_addr_str[512];

	/* set ksc_addr para to kernel */
	sprintf(ksc_mem_phy_addr_str, "%lu,%lu\n", buf->mem0_phy_addr, buf->mem1_phy_addr);
	// KSC_WRN("ksc_mem_phy_addr_str = %s\n",ksc_mem_phy_addr_str);
	KSC_WRN("buf->mem0_phy_addr = 0x%x\n", (uint64_t)buf->mem0_phy_addr);
	KSC_WRN("buf->mem1_phy_addr = 0x%x\n", (uint64_t)buf->mem1_phy_addr);
	ret = ksc_save_string_to_kernel("ksc_mem_phy_addr_str", ksc_mem_phy_addr_str);
	if (ret < 0) {
		KSC_ERR("ksc_save_string_to_kernel error: %s : %d\n",
				__func__, fdt_strerror(ret));
	}

	/* premap ksc iommu */
	ksc_add_kernel_iova_premap((unsigned long)(buf->mem0_phy_addr), buf->mem_size);
	ksc_add_kernel_iova_premap((unsigned long)(buf->mem1_phy_addr), buf->mem_size);

	return 0;
}


void ksc_irq_handler(void *parg)
{
	struct ksc_device *p_ksc = parg;
	int state = 0;

	state = ksc_reg_irq_query(p_ksc->p_reg);
	if (state & KSC_IRQ_BE_FINISH) {
		p_ksc->offline_finish_flag = true;
		wake_up_process(&p_ksc->event_wait);
	}

	if (state & KSC_IRQ_FE_ERR) {
		p_ksc->fe_err_cnt++;
	}

	if (state & KSC_IRQ_DE2KSC_DATA_VOLUME_ERR) {
		p_ksc->de2ksc_data_volume_err_cnt++;
	}

	if (state & SVP2KSC_DMT_ERR) {
		p_ksc->svp2ksc_dmt_err_cnt++;
	}


	if (state & KSC_IRQ_BE_ERR) {
		p_ksc->be_err_cnt++;
	}

	p_ksc->irq_cnt++;

	return ;
}

static void __ksc_free_memory(struct ksc_device *p_ksc)
{
	struct ksc_buffers *buf = &p_ksc->ksc_para.buf;

	if (buf->mem0_phy_addr)
		free((void *)buf->mem0_phy_addr);
	if (buf->mem1_phy_addr)
		free((void *)buf->mem1_phy_addr);

	buf->mem0_virt = NULL;
	buf->mem1_virt = NULL;
	buf->mem0_phy_addr = 0;
	buf->mem1_phy_addr = 0;

#if defined(__LINUX_PLAT__)
	if (buf->lut_phy_addr)
		free((void *)buf->lut_phy_addr);
	buf->lut_virt = NULL;
	buf->lut_phy_addr = 0;

	if (buf->src_item) {
		if (!IS_ERR_OR_NULL(buf->src_item->buf)) {
			ksc_dma_unmap(buf->src_item);
		}
		free(buf->src_item);
		buf->src_item = NULL;
	}

	if (buf->dst_item) {
		if (!IS_ERR_OR_NULL(buf->dst_item->buf)) {
			ksc_dma_unmap(buf->dst_item);
		}
		free(buf->dst_item);
		buf->dst_item = NULL;
	}
#endif

	buf->alloced = false;
}


int __ksc_enable(struct ksc_device *p_ksc, bool enable)
{
	// int ret = 0;
	int i = 0;
	// struct ksc_buffers *buf = &p_ksc->ksc_para.buf;

	mutex_lock(&p_ksc->dev_lock);

	for (i = 0; i < MAX_CLK_NUM; ++i) {
		if (enable) {
			if (!IS_ERR_OR_NULL(p_ksc->clks[i])) {
				if (p_ksc->clk_freq[i]) {
					clk_set_rate(p_ksc->clks[i], p_ksc->clk_freq[i]);
				}
				clk_prepare_enable(p_ksc->clks[i]);
			}
		} else {
			if (!IS_ERR_OR_NULL(p_ksc->clks[i]))
				clk_disable(p_ksc->clks[i]);
		}
	}
	tvdisp_ksc_enable(p_ksc->tv_disp_top_reg, enable);

	p_ksc->ksc_para.is_first_frame = true;

	irq_install_handler(p_ksc->irq_no, (interrupt_handler_t *)ksc_irq_handler, (void *)p_ksc);
	irq_enable(p_ksc->irq_no);

#if defined(__LINUX_PLAT__)
	if (enable) {
		buf->src_item = memalign(KSC_MEMALIGN, sizeof(struct dmabuf_item));
		buf->dst_item = memalign(KSC_MEMALIGN, sizeof(struct dmabuf_item));
		if (!buf->src_item || !buf->dst_item) {
			ret = -ENOMEM;
			goto OUT;
		}
		buf->src_item->p_device = p_ksc->p_device;
		buf->dst_item->p_device = p_ksc->p_device;

		buf->lut_size = LUT_MEM_SIZE;//double buffer
		buf->lut_phy_addr = (uintptr_t)memalign(KSC_MEMALIGN, buf->lut_size * 2);
		if (buf->lut_phy_addr) {
			KSC_ERR("Memory alloc %d Byte ksc lut buffer fail!\n", buf->lut_size);
			ret = -ENOMEM;
			goto OUT;
		}
		// buf->lut_mem0_virt = buf->lut_virt;
		buf->lut_mem0_phy_addr = buf->lut_phy_addr;
		// buf->lut_mem1_virt = buf->lut_virt + LUT_MEM_SIZE;
		buf->lut_mem1_phy_addr = buf->lut_phy_addr + LUT_MEM_SIZE;
	} else {
		free_irq(p_ksc->irq_no, (void *)p_ksc);
		__ksc_free_memory(p_ksc);
	}
#endif
	p_ksc->offline_finish_flag = false;
	p_ksc->enabled = enable;

	mutex_unlock(&p_ksc->dev_lock);
	return 0;

// OUT:
// 	if (ret) {
// 		for (i = 0; i < MAX_CLK_NUM; ++i) {
// 			if (!IS_ERR_OR_NULL(p_ksc->clks[i]))
// 				clk_disable(p_ksc->clks[i]);
// 		}
// 		free_irq(p_ksc->irq_no, (void *)p_ksc);
// 		__ksc_free_memory(p_ksc);
// 	}
// 	mutex_unlock(&p_ksc->dev_lock);
// 	return ret;
}


static int __ksc_init_memory(struct ksc_device *p_ksc, struct sunxi_ksc_para *para)
{
	int mem_size = 0;
	struct ksc_buffers *buf = &p_ksc->ksc_para.buf;


	if (para->pix_fmt == YUV422SP) {
		//Greater then actual size
		mem_size = ALIGN(para->dns_w, 64) * para->dns_h * 2;
		buf->chm_offset = ALIGN(para->dns_w, 64) * para->dns_h;
		if (para->bit_depth == 10) {
			mem_size += ALIGN(para->dns_w, 64) * para->dns_h * 2 / 4;
			buf->pix_2b_offset = buf->chm_offset + ALIGN(para->dns_w, 64) * para->dns_h;
		}
	} else if (para->pix_fmt == AYUV) {
		mem_size = ALIGN(para->dns_w, 16) * para->dns_h * 4;
		buf->chm_offset = ALIGN(para->dns_w, 16) * para->dns_h * 2;
		if (para->bit_depth == 10) {
			mem_size += ALIGN(para->dns_w, 64) * para->dns_h;
			buf->pix_2b_offset = buf->chm_offset + ALIGN(para->dns_w, 64) * para->dns_h * 2;
		}
	} else {
		if (para->mode == ONLINE_MODE) {
			KSC_ERR("Invalid FE output pixel format:%d\n", para->pix_fmt);
			return -1;
		}
	}

	if ((buf->alloced == false || mem_size != buf->mem_size) &&
	    para->mode == ONLINE_MODE) {
		KSC_INFO("Alloc %u Byte memory for ksc\n", mem_size);
		buf->mem_size = mem_size;

		/* alloc frame buffer */
#if IS_ENABLED(CONFIG_SUNXI_PRAM_MEM)
		pram_memalign_init();
#endif
		buf->mem0_phy_addr = ksc_mem_alloc(KSC_MEMALIGN, buf->mem_size);
		buf->mem1_phy_addr = ksc_mem_alloc(KSC_MEMALIGN, buf->mem_size);
		if (!buf->mem0_phy_addr || !buf->mem1_phy_addr) {
			KSC_ERR(
				"Memory alloc %d Byte ksc double buffer fail!\n",
				buf->mem_size * 2);
			goto OUT;
		}

#if defined(__LINUX_PLAT__)
		if (buf->alloced == true) {
			if (buf->buf_to_be_free.mem_size != 0) {
				cancel_delayed_work_sync(
				    &p_ksc->free_buffer_work);
				dev_info(p_ksc->p_device, "Last frame's memory "
							  "has not been free "
							  "yet!\n");
			}
			buf->buf_to_be_free.mem_size = buf->mem_size;
			buf->buf_to_be_free.mem0_phy_addr = buf->mem0_phy_addr;
			buf->buf_to_be_free.mem1_phy_addr = buf->mem1_phy_addr;
			buf->alloced = false;
		}
		buf->mem0_virt =
			dma_alloc_coherent(p_ksc->p_device, buf->mem_size,
					   &buf->mem0_phy_addr, GFP_KERNEL);
		buf->mem1_virt =
			dma_alloc_coherent(p_ksc->p_device, buf->mem_size,
					   &buf->mem1_phy_addr, GFP_KERNEL);

		if (!buf->mem0_virt || !buf->mem0_phy_addr || !buf->mem1_virt ||
		    !buf->mem1_phy_addr) {
			KSC_ERR(
			    p_ksc->p_device,
			    "Memory alloc %d Byte ksc double buffer fail!\n",
			    buf->mem_size * 2);
			goto OUT;
		}


		if (buf->buf_to_be_free.mem_size) {
			schedule_delayed_work(&p_ksc->free_buffer_work,
					      msecs_to_jiffies(20));
		}
#endif
		buf->alloced = true;
	}

	return 0;
OUT:
	__ksc_free_memory(p_ksc);
	return -1;
}

int __ksc_set_para(struct ksc_device *p_ksc, struct sunxi_ksc_para *para)
{
	int ret = -1;

	mutex_lock(&p_ksc->dev_lock);


	if (p_ksc->enabled == false) {
		KSC_ERR("Ksc has not been enabled yet!\n");
		goto OUT;
	}
	memcpy(&p_ksc->ksc_para.para, para, sizeof(struct sunxi_ksc_para));


	ret = __ksc_init_memory(p_ksc, para);
	if (ret) {
		goto OUT;
	}

	ret = ksc_reg_update_all(p_ksc->p_reg, &p_ksc->ksc_para);
	if (ret) {
		goto OUT;
	}

	if (p_ksc->ksc_para.is_first_frame == true) {
		p_ksc->ksc_para.is_first_frame = false;
	}
#if defined(__LINUX_PLAT__)
	bool is_finished = false;
	long timeout = 0;

	if (para->mode != ONLINE_MODE) {
		//wait offline process finish
		timeout = wait_event_interruptible(p_ksc->event_wait,
					    p_ksc->offline_finish_flag == true, 0);
		if (timeout <= 0) {
			KSC_ERR("Wait ksc offline process finish timout!\n");
			p_ksc->offline_finish_flag = true;
			wake_up_process(&p_ksc->event_wait);
		}
		p_ksc->offline_finish_flag = false;
	} else {
		if (para->online_wb_en == true) {
			timeout = 0;
			is_finished = false;
			timeout = read_poll_timeout(is_mem_sync_finish, is_finished,
						    is_finished, 100, 50000, p_ksc->p_reg);
			if (timeout) {
				KSC_ERR("Wait mem sync timout!\n");
			}
		}
	}
#endif

	ksc_buf = &p_ksc->ksc_para.buf;
OUT:
	mutex_unlock(&p_ksc->dev_lock);
	return ret;
}

#if defined(__LINUX_PLAT__)
struct ksc_buffers *__get_ksc_buffers(struct ksc_device *p_ksc)
{
	bool is_finished = false;
	long timeout = 0;

	if (p_ksc->ksc_para.is_first_frame == false &&
	    p_ksc->ksc_para.para.mode == ONLINE_MODE) {
		// wait online reg update finish
		timeout = read_poll_timeout(is_reg_update_finish, is_finished,
					    is_finished, 100, 150000, p_ksc->p_reg);
		if (timeout) {
			KSC_ERR("Wait reg update timout!\n");
		}
	}
	ksc_reg_switch_lut_buf(p_ksc->p_reg, &p_ksc->ksc_para);

	return &p_ksc->ksc_para.buf;
}

struct sunxi_ksc_para *__get_ksc_para(struct ksc_device *p_ksc)
{
	struct sunxi_ksc_para *para;
	mutex_lock(&p_ksc->dev_lock);
	para = &p_ksc->ksc_para.para;
	mutex_unlock(&p_ksc->dev_lock);
	return para;
}

static void __free_buffer_work(struct work_struct *w)
{
	struct ksc_device *p_ksc = container_of(w, struct ksc_device, free_buffer_work.work);
	struct ksc_buffers *buf = &p_ksc->ksc_para.buf;

	if (buf->buf_to_be_free.mem_size) {
		if (buf->buf_to_be_free.mem0_phy_addr)
			free((void *)(buf->buf_to_be_free.mem0_phy_addr));
		if (buf->buf_to_be_free.mem1_phy_addr)
			free((void *)buf->buf_to_be_free.mem1_phy_addr);

		buf->buf_to_be_free.mem0_phy_addr = 0;
		buf->buf_to_be_free.mem1_phy_addr = 0;
		buf->buf_to_be_free.mem0_virt = NULL;
		buf->buf_to_be_free.mem1_virt = NULL;
		KSC_WRN("Free memory %u\n", buf->buf_to_be_free.mem_size);
		buf->buf_to_be_free.mem_size = 0;
	}

}
#endif

struct ksc_device *ksc_dev_init(struct device *dev)
{
	struct ksc_device *p_ksc = NULL;
	int i;
	char id[32];
	int node_offset = 0;
	int value[64] = {0};

	p_ksc = malloc(sizeof(struct ksc_device));
	if (!p_ksc) {
		KSC_ERR("Malloc ksc_device fail!\n");
		goto OUT;
	}

	p_ksc->p_reg =
		(struct ksc_regs *)ksc_getprop_regbase(KSC_FDT_NODE, "reg");
	if (!p_ksc->p_reg) {
		KSC_ERR("unable to map ksc registers\n");
		goto OUT;
	}

	p_ksc->tv_disp_top_reg =
		(struct ksc_regs *)ksc_getprop_regbase(KSC_FDT_NODE, "tv_reg");
	if (!p_ksc->tv_disp_top_reg) {
		KSC_ERR("unable to map tv display top registers\n");
	}

	p_ksc->irq_no =
		ksc_getprop_irq(KSC_FDT_NODE, "interrupts");
	if (!p_ksc->irq_no) {
		KSC_ERR("irq_of_parse_and_map dec irq fail\n");
	}

	node_offset = fdt_path_offset(working_fdt, KSC_FDT_NODE);
	if (node_offset < 0) {
		KSC_ERR("fdt_path_offset node_offset fail\n");
		goto OUT;
	}

	of_periph_clk_config_setup(node_offset);

	for (i = 0; i < MAX_CLK_NUM; ++i) {
		p_ksc->clks[i] = of_clk_get(node_offset, i);
		if (IS_ERR_OR_NULL(p_ksc->clks[i])) {
			KSC_WRN("Fail to get clk%d\n", i);
		}

		snprintf(id, 32, "clk%d_freq", i);
		if (fdt_getprop_u32(working_fdt, node_offset, id, (uint32_t *)value) != 0) {
			KSC_WRN("Get %s property failed\n", id);
			p_ksc->clk_freq[i] = 0;
		}
	}

	p_ksc->enable = __ksc_enable;
	p_ksc->set_ksc_para = __ksc_set_para;
	// p_ksc->get_ksc_buffers = __get_ksc_buffers;
	// p_ksc->get_ksc_para = __get_ksc_para;


	mutex_init(&p_ksc->dev_lock);
	init_waitqueue_head(&p_ksc->event_wait);
	// INIT_DELAYED_WORK(&p_ksc->free_buffer_work, __free_buffer_work);

	printf("%s finsih\n", __func__);

	return p_ksc;

OUT:
	if (p_ksc) {
		printf("ksc dev init out\n");
		free(p_ksc);
	}
	return NULL;
}

//End of File
