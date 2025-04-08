// SPDX-License-Identifier: GPL-2.0
/*
 * ksc_reg/ksc_reg.c
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
#include "ksc_reg.h"
#include "../../ksc.h"
#include "../../ksc_macro_def.h"
#include <linux/delay.h>

bool is_mem_sync_finish(struct ksc_regs *p_reg)
{
	if (p_reg->status.bits.sync_idle == 1) {
		p_reg->status.bits.sync_idle = 1;
		return true;
	}

	return false;
}

bool is_reg_update_finish(struct ksc_regs *p_reg)
{
	if (p_reg->ksc_intr.bits.ksc_regup_done_int) {
		//clear reg done
		p_reg->ksc_intr.bits.ksc_regup_done_int  = 1;
		return true;
	}

	return false;
}

int ksc_reg_get_version(struct ksc_regs *p_reg)
{
	return p_reg->version.bits.ip_version;
}

static void ksc_reg_start_update_reg(struct ksc_regs *p_reg)
{
	p_reg->reset.bits.ksc_reg_parse_end = 1;
}

static void ksc_reg_start_offline_wb(struct ksc_regs *p_reg)
{
	p_reg->reset.bits.ksc_sw_start = 1;
}

static void ksc_reg_reset(struct ksc_regs *p_reg)
{
	p_reg->reset.bits.ksc_sw_reset = 1;
	udelay(20);
	p_reg->reset.bits.ksc_sw_reset = 0;
}

static void ksc_reg_set_top_ctrl(struct ksc_regs *p_reg, struct sunxi_ksc_para *para)
{
	p_reg->ksc_top_ctrl.bits.ksc_mbus_clk_gating_en = 1;
	p_reg->ksc_top_ctrl.bits.ksc_run_mode = (int)para->mode;

	if (para->mode != ONLINE_MODE) {
		ksc_reg_reset(p_reg);
	}

	p_reg->ksc_top_ctrl.bits.ksc_csc_mode = para->csc_fmt;
	switch (para->wb_fmt) {
	case YUV444P:
	case YVU444P:
	case YUV444SP:
	case YVU444SP:
		p_reg->ksc_top_ctrl.bits.ksc_csc_mode = 0x7; // bypass
		p_reg->ksc_top_ctrl.bits.ksc_out_wb_fmt = 0x5;
		break;
	case RGB888:
	case BGR888:
		p_reg->ksc_top_ctrl.bits.ksc_out_wb_fmt = 0x4;
		break;
	case ARGB8888:
		p_reg->ksc_top_ctrl.bits.ksc_out_wb_fmt = 0x0;
		break;
	case ABGR8888:
		p_reg->ksc_top_ctrl.bits.ksc_out_wb_fmt = 0x1;
		break;
	case RGBA8888:
		p_reg->ksc_top_ctrl.bits.ksc_out_wb_fmt = 0x2;
		break;
	case BGRA8888:
		p_reg->ksc_top_ctrl.bits.ksc_out_wb_fmt = 0x3;
		break;
	default:
		p_reg->ksc_top_ctrl.bits.ksc_out_wb_fmt = 0x4;
		break;
	}
	p_reg->ksc_top_ctrl.bits.ksc_csc_dns_type = para->pq_para.csc_dns_type;
	p_reg->ksc_top_ctrl.bits.ksc_scal_en = (int)para->scaler_en;
	p_reg->ksc_top_ctrl.bits.ksc_clk_fixed_en = 0;
	p_reg->ksc_top_ctrl.bits.ksc_en = 1;//ksc IP enable

	p_reg->ksc_func_cfg.bits.rec_en = para->ksc_en;
	p_reg->ksc_func_cfg.bits.v_flip = para->flip_v;
	p_reg->ksc_func_cfg.bits.h_flip = para->flip_h;
	p_reg->ksc_func_cfg.bits.rot_angle = para->rot_angle;
	//online be writeback, debug purpose
	p_reg->ksc_func_cfg.bits.ksc_m0_out_wb_en = 0;
	p_reg->ksc_func_cfg.bits.crop_en = (int)para->crop_en;
	p_reg->ksc_func_cfg.bits.stitch_mode_flag = 0;
	p_reg->ksc_func_cfg.bits.hw_write_all_region_flag = 1;
	p_reg->ksc_func_cfg.bits.filter_lut_update_flag = 0;
	p_reg->ksc_func_cfg.bits.ksc_perf_mode_en = (int)para->perf_mode_en;
	p_reg->ksc_func_cfg.bits.ksc_post_proc_en = (int)para->pq_para.post_proc_en;
}

static void ksc_reg_irq_setting(struct ksc_regs *p_reg)
{
	//enable error interrupt
	p_reg->intr_en.bits.ksc_be_err_int_en = 1;
	p_reg->intr_en.bits.ksc_fe_err_int_en = 1;
	p_reg->intr_en.bits.ksc_lut_over_span_int_en = 1;
	//offline finish interrupt
	if (p_reg->ksc_top_ctrl.bits.ksc_run_mode != 0) {
		p_reg->intr_en.bits.ksc_be_finish_int_en = 1;
	} else {
		p_reg->intr_en.bits.ksc_be_finish_int_en = 0;
	}
}

int ksc_reg_irq_query(struct ksc_regs *p_reg)
{
	int state = 0;

	if (p_reg->ksc_intr.bits.ksc_be_finish_int) {
		p_reg->ksc_intr.bits.ksc_be_finish_int = 1;
		state |= KSC_IRQ_BE_FINISH;
	}

	if (p_reg->ksc_intr.bits.ksc_be_error_int) {
		p_reg->ksc_intr.bits.ksc_be_error_int = 1;
		state |= KSC_IRQ_BE_ERR;
	}

	if (p_reg->ksc_intr.bits.ksc_fe_error_int) {
		if (p_reg->ksc_intr.bits.de2ksc_data_volume_error) {
			p_reg->ksc_intr.bits.de2ksc_data_volume_error = 1;
			p_reg->ksc_intr.bits.ksc_fe_error_int = 1;
			state |= KSC_IRQ_DE2KSC_DATA_VOLUME_ERR;
		} else {
			p_reg->ksc_intr.bits.ksc_fe_error_int = 1;
			state |= KSC_IRQ_FE_ERR;
		}
	}

	if (p_reg->ksc_intr.bits.ksc_lut_over_span_int) {
		p_reg->ksc_intr.bits.ksc_lut_over_span_int = 1;
		state |= KSC_IRQ_LUT_OVER_SPAN;
	}

	return state;
}

int ksc_reg_switch_lut_buf(struct ksc_regs *p_reg, struct ksc_para *para)
{
	struct ksc_buffers *buf = &para->buf;

	if (p_reg->lut_mem0_base_addr.val == buf->lut_mem0_phy_addr) {
		// buf->lut_virt = buf->lut_mem1_virt;
		buf->lut_phy_addr = buf->lut_mem1_phy_addr;
	} else if (p_reg->lut_mem0_base_addr.val == buf->lut_mem1_phy_addr) {
		// buf->lut_virt = buf->lut_mem0_virt;
		buf->lut_phy_addr = buf->lut_mem0_phy_addr;
	} else {
		// buf->lut_virt = buf->lut_mem0_virt;
		buf->lut_phy_addr = buf->lut_mem0_phy_addr;
	}

	return 0;

}

static void ksc_reg_mem_setting(struct ksc_regs *p_reg, struct ksc_para *para)
{
	int src_lum_stride64 = 0, src_chm_stride64 = 0;
	int src_w = para->para.dns_w;
	int act_dst_w = 0, dst_align16_width = 0, dst_align32_width = 0, dst_align64_width = 0;
	unsigned int chm_offset = 0;

	if (para->para.mode == OFFLINE_MODE2) {
		src_lum_stride64 = ALIGN(src_w, 16) << 2;
		src_chm_stride64 = ALIGN(src_w, 16) << 2;
	} else {
		src_lum_stride64 = ALIGN(src_w, 32) << 1;
		src_chm_stride64 = ALIGN(src_w, 32) << 1;
	}
	p_reg->src_mem0_stride.bits.ksc_src_chm_mem0_stride = src_chm_stride64 >> 2;
	p_reg->src_mem0_stride.bits.ksc_src_lum_mem0_stride = src_lum_stride64 >> 2;
	p_reg->src_mem1_stride.bits.ksc_src_chm_mem1_stride = src_chm_stride64 >> 2;
	p_reg->src_mem1_stride.bits.ksc_src_lum_mem1_stride = src_lum_stride64 >> 2;


	if (para->para.mode == ONLINE_MODE)
		act_dst_w = para->para.src_w;

	if (para->para.mode != ONLINE_MODE)
		act_dst_w = para->para.dst_w;

	if (para->para.mode != ONLINE_MODE && para->para.crop_en)//crop,dst_size=crop_size
		act_dst_w = para->para.crop_w;

	dst_align16_width = ALIGN(act_dst_w, 16);
	dst_align32_width = ALIGN(act_dst_w, 32);
	dst_align64_width = ALIGN(act_dst_w, 64);
	if (para->para.mode == ONLINE_MODE) {
		p_reg->dst_stride.bits.ksc_dst_lum_mem_stride =
			dst_align64_width / 4;
		p_reg->dst_stride.bits.ksc_dst_chm_mem_stride =
			(dst_align32_width / 4) * 2;

		p_reg->ksc_func_cfg.bits.ksc_m0_out_wb_en = para->para.online_wb_en;

		if (para->para.online_wb_en == true) {
			chm_offset = ALIGN(para->para.dst_img.w, 64) * para->para.dst_img.h;
			if (para->buf.dst_item) {
				p_reg->dst_lum_mem_base_addr.val = (unsigned int)para->buf.dst_item->dma_addr;
				p_reg->dst_chm_mem_base_addr.val = (unsigned int)(para->buf.dst_item->dma_addr + chm_offset);
			}
			if (sizeof(dma_addr_t) == 8) {
				p_reg->dst_mem_base_addr_high8.bits.ksc_dst_lum_mem_base_addr_high8 = (unsigned int)(para->buf.dst_item->dma_addr >> 32);
				p_reg->dst_mem_base_addr_high8.bits
					.ksc_dst_chm_mem_base_addr_high8 =
					(unsigned int)((para->buf.dst_item
								->dma_addr +
							chm_offset) >>
						       32);
			}
		}

		//fe output size
		p_reg->dst_img_size.bits.ksc_dst_img_wth = CLIP(para->para.dns_w, 64, 2048);
		p_reg->dst_img_size.bits.ksc_dst_img_hgt = CLIP(para->para.dns_h, 64, 2048);

		p_reg->src_lum_mem0_base_addr.val = (unsigned int)para->buf.mem0_phy_addr;
		p_reg->src_chm_mem0_base_addr.val = (unsigned int)(para->buf.mem0_phy_addr + para->buf.chm_offset);

		p_reg->src_lum_mem1_base_addr.val = (unsigned int)para->buf.mem1_phy_addr;
		p_reg->src_chm_mem1_base_addr.val = (unsigned int)(para->buf.mem1_phy_addr + para->buf.chm_offset);
		if (sizeof(dma_addr_t) == 8) {
			p_reg->src_mem0_base_addr_high8.bits.ksc_src_lum_mem0_base_addr_high8 = (unsigned int)(para->buf.mem0_phy_addr >> 32);
			p_reg->src_mem0_base_addr_high8.bits.ksc_src_chm_mem0_base_addr_high8 = (unsigned int)((para->buf.mem0_phy_addr + para->buf.chm_offset) >> 32);
			p_reg->src_mem1_base_addr_high8.bits.ksc_src_lum_mem1_base_addr_high8 =  (unsigned int)(para->buf.mem1_phy_addr >> 32);
			p_reg->src_mem1_base_addr_high8.bits.ksc_src_chm_mem1_base_addr_high8 = (unsigned int)((para->buf.mem1_phy_addr + para->buf.chm_offset) >> 32);
		}

	} else {
		p_reg->dst_stride.bits.ksc_dst_lum_mem_stride =
			(dst_align16_width / 4) * 4;
		p_reg->dst_img_size.bits.ksc_dst_img_wth = CLIP(para->para.dst_img.w, 64, 2048);
		p_reg->dst_img_size.bits.ksc_dst_img_hgt = CLIP(para->para.dst_img.h, 64, 2048);
		//For offline,  src mem0 used to store source image memory address
		p_reg->src_lum_mem0_base_addr.val = (unsigned int)para->buf.src_item->dma_addr;
		p_reg->dst_lum_mem_base_addr.val = (unsigned int)para->buf.dst_item->dma_addr;
		if (sizeof(dma_addr_t) == 8) {
			p_reg->src_mem0_base_addr_high8.bits.ksc_src_lum_mem0_base_addr_high8 = (unsigned int)(para->buf.src_item->dma_addr >> 32);
			p_reg->dst_mem_base_addr_high8.bits.ksc_dst_lum_mem_base_addr_high8 = (unsigned int)(para->buf.dst_item->dma_addr >> 32);
		}


	}

	p_reg->lut_mem0_stride.bits.ksc_lut_mem0_stride = para->para.lut.lut_stride;
	p_reg->lut_mem1_stride.bits.ksc_lut_mem1_stride = para->para.lut.lut_stride;

	p_reg->lut_mem0_base_addr.val = (unsigned int)para->buf.lut_phy_addr;
	p_reg->interp_coef_base_addr.val = (unsigned int)(para->buf.lut_phy_addr + para->para.interp_coef_offset);
	p_reg->dnscal_coef_base_addr.val = (unsigned int)(para->buf.lut_phy_addr + para->para.dnscal_coef_offset);

	if (sizeof(dma_addr_t) == 8) {
		p_reg->lut_mem0_base_addr_high8.bits.ksc_lut_mem0_base_addr_high8 = (unsigned int)(para->buf.lut_phy_addr >> 32);
		p_reg->interp_coef_base_addr_high8.bits.ksc_interp_coef_base_addr_high8 = (unsigned int)((para->buf.lut_phy_addr + para->para.interp_coef_offset) >> 32);
		p_reg->dnscal_coef_base_addr_high8.bits.ksc_dnscal_coef_base_addr_high8 = (unsigned int)((para->buf.lut_phy_addr + para->para.dnscal_coef_offset) >> 32);
	}

	p_reg->input_img_size.bits.ksc_ori_img_wth = CLIP(para->para.src_w, 64, 2048);
	p_reg->input_img_size.bits.ksc_ori_img_hgt = CLIP(para->para.src_h, 64, 2048);

}

void ksc_reg_other_setting(struct ksc_regs *p_reg, struct ksc_para *para)
{
	if (para->para.scaler_en) {
		p_reg->ksc_scale_ratio.bits.ksc_scale_ratio_x = para->para.scale_ratio_x;
		p_reg->ksc_scale_ratio.bits.ksc_scale_ratio_y = para->para.scale_ratio_y;
	} else {
		p_reg->ksc_scale_ratio.bits.ksc_scale_ratio_x = 0xfff;
		p_reg->ksc_scale_ratio.bits.ksc_scale_ratio_y = 0xfff;
	}

	if (para->para.mode != ONLINE_MODE && para->para.crop_en == true) {
		p_reg->ksc_roi_hor_pos.bits.ksc_roi_hor_min_pos = 0;
		p_reg->ksc_roi_hor_pos.bits.ksc_roi_hor_max_pos = CLIP(para->para.dst_img.w - 1, 0, 2047);
		p_reg->ksc_roi_ver_pos.bits.ksc_roi_ver_min_pos = 0;
		p_reg->ksc_roi_ver_pos.bits.ksc_roi_ver_max_pos = CLIP(para->para.dst_img.h - 1, 0, 2048);
		p_reg->ksc_crop_offset.bits.crop_x = para->para.crop_x;
		p_reg->ksc_crop_offset.bits.crop_y = para->para.crop_y;
		p_reg->ksc_scale_size_for_crop.bits.scale_width_minus1_for_crop = para->para.dst_w - 1;
		p_reg->ksc_scale_size_for_crop.bits.scale_height_minus1_for_crop = para->para.dst_h - 1;
	} else {
		p_reg->ksc_roi_hor_pos.bits.ksc_roi_hor_min_pos = CLIP(para->para.lut.roi_x_min, 0, 2047);
		p_reg->ksc_roi_hor_pos.bits.ksc_roi_hor_max_pos = CLIP(para->para.lut.roi_x_max_non_ali, 0, 2047);
		p_reg->ksc_roi_ver_pos.bits.ksc_roi_ver_min_pos = CLIP(para->para.lut.roi_y_min, 0, 2048);
		p_reg->ksc_roi_ver_pos.bits.ksc_roi_ver_max_pos = CLIP(para->para.lut.roi_y_max_non_ali, 0, 2048);
	}

	p_reg->ksc_out_pos_offset.bits.ksc_pos_ost_v = para->para.ost_v;
	p_reg->ksc_out_pos_offset.bits.ksc_pos_ost_h = para->para.ost_h;
	p_reg->ksc_phase_offset.bits.ksc_phase_v = para->para.phase_v;
	p_reg->ksc_phase_offset.bits.ksc_phase_h = para->para.phase_h;
	p_reg->mem_req_bl_num.bits.mem_req_bl_max = 31;
	p_reg->ksc_debug_info_0.val = 0;
	p_reg->ksc_debug_info_1.val = 0;
	p_reg->ksc_debug_info_2.val = 0;
	p_reg->ksc_debug_info_3.val = 0;
	p_reg->mode0_hw_control.bits.ksc_1st_frm_en = (int)para->is_first_frame;
	p_reg->mode0_hw_control.bits.hw_rst_cycle_num = 1024;
	p_reg->mode0_hw_control.bits.band_width_ctrl_en = para->bw.bw_ctrl_en;
	if (para->bw.bw_ctrl_en) {
		p_reg->mode0_hw_control.bits.band_wdith_ctrl_num =
					(para->bw.bw_ctrl_num > 15) ? 15 : para->bw.bw_ctrl_num;
	} else
		p_reg->mode0_hw_control.bits.band_wdith_ctrl_num = 8;
}

void ksc_reg_pq_setting(struct ksc_regs *p_reg, struct ksc_para *para)
{
	struct ksc_pq_para *pq_para = &para->para.pq_para;

	p_reg->ksc_default_pix.bits.ksc_default_pix_ch0 = CLIP(pq_para->def_val_ch0, 0, 255);
	p_reg->ksc_default_pix.bits.ksc_default_pix_ch1 = CLIP(pq_para->def_val_ch1, 0, 255);
	p_reg->ksc_default_pix.bits.ksc_default_pix_ch2 = CLIP(pq_para->def_val_ch2, 0, 255);

	p_reg->ksc_detail_control.bits.ksc_detail_black_max_clip = CLIP(pq_para->blk_clip_val, 0, 128);
	p_reg->ksc_detail_control.bits.ksc_detail_white_max_clip = CLIP(pq_para->wht_clip_val, 0, 128);
	p_reg->ksc_detail_control.bits.ksc_detail_black_gain = CLIP(pq_para->blk_gain, 0, 128);
	p_reg->ksc_detail_control.bits.ksc_detail_white_gain = CLIP(pq_para->wht_gain, 0, 128);

	p_reg->ksc_filt_sel.bits.ksc_bord_flt_type = pq_para->filt_type_bod;
	p_reg->ksc_filt_sel.bits.ksc_flt_sel_hi_thr = CLIP(pq_para->filt_type_thH, 1, 64);
	p_reg->ksc_filt_sel.bits.ksc_flt_sel_lo_thr = CLIP(pq_para->filt_type_thL, 0, 63);

	p_reg->ksc_anti_alias_thr.bits.ksc_anti_alias_ref = CLIP(pq_para->antialias_ref, 0, 511);
	p_reg->ksc_anti_alias_thr.bits.ksc_anti_alias_hi_thr = CLIP(pq_para->antialias_thH, 1, 64);
	p_reg->ksc_anti_alias_thr.bits.ksc_anti_alias_lo_thr = CLIP(pq_para->antialias_thL, 0, 63);

	p_reg->ksc_anti_alias_str.bits.ksc_blend_weight_str = CLIP(pq_para->antialias_str, 0, 128);
	p_reg->ksc_anti_alias_str.bits.ksc_dir_rsid_avg_str = CLIP(pq_para->res_str, 0, 128);
	p_reg->ksc_dir_rsid.bits.ksc_dir_rsid_max_clip = CLIP(pq_para->res_max_clp, 0, 64);
	p_reg->ksc_dir_rsid.bits.ksc_dir_rsid_min_clip = CLIP(pq_para->res_min_clp, 0, 64);
	p_reg->ksc_dir_rsid.bits.ksc_dir_rsid_max_ratio = CLIP(pq_para->res_peak_rat, 0, 64);
	p_reg->ksc_dir_rsid.bits.ksc_dir_rsid_avg_gain = CLIP(pq_para->res_gain, 0, 64);
	p_reg->ksc_dir_max_rsid_ratio.bits.ksc_dir_neg_max_rsid_ratio = CLIP(pq_para->blk_clip_rat, 0, 128);
	p_reg->ksc_dir_max_rsid_ratio.bits.ksc_dir_pos_max_rsid_ratio = CLIP(pq_para->wht_clip_rat, 0, 128);
	p_reg->ksc_default_rgb.bits.alpha_default_val = 0;
	p_reg->ksc_default_rgb.bits.ksc_default_rgb_ch0 = CLIP(para->para.ksc_default_rgb_ch0, 0, 255);
	p_reg->ksc_default_rgb.bits.ksc_default_rgb_ch1 = CLIP(para->para.ksc_default_rgb_ch1, 0, 255);
	p_reg->ksc_default_rgb.bits.ksc_default_rgb_ch2 = CLIP(para->para.ksc_default_rgb_ch2, 0, 255);
	p_reg->ksc_border_blur1.bits.border_pix_num_h1 = pq_para->bod_pix_num_h1;
	p_reg->ksc_border_blur1.bits.border_pix_dif_h1 = pq_para->bod_pix_dif_h1;
	p_reg->ksc_border_blur1.bits.border_pix_num_v1 = pq_para->bod_pix_num_v1;
	p_reg->ksc_border_blur1.bits.border_pix_dif_v1 = pq_para->bod_pix_dif_v1;

	p_reg->ksc_border_blur0.bits.border_pix_num_h0 = pq_para->bod_pix_num_h0;
	p_reg->ksc_border_blur0.bits.border_pix_dif_h0 = pq_para->bod_pix_dif_h0;
	p_reg->ksc_border_blur0.bits.border_pix_num_v0 = pq_para->bod_pix_num_v0;
	p_reg->ksc_border_blur0.bits.border_pix_dif_v0 = pq_para->bod_pix_dif_v0;
}


int ksc_reg_update_all(struct ksc_regs *p_reg, struct ksc_para *para)
{
	ksc_reg_set_top_ctrl(p_reg, &para->para);
	ksc_reg_irq_setting(p_reg);
	ksc_reg_mem_setting(p_reg, para);
	ksc_reg_other_setting(p_reg, para);
	ksc_reg_pq_setting(p_reg, para);

	if (para->para.mode != ONLINE_MODE) {
		ksc_reg_start_offline_wb(p_reg);
	} else
		ksc_reg_start_update_reg(p_reg);

	return 0;
}

int tvdisp_ksc_enable(void *tvdisp_reg, bool enable)
{
	return 0;
}
