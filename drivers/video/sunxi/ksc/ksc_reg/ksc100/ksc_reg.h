// SPDX-License-Identifier: GPL-2.0
/*
 * ksc_reg.h
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
#ifndef _KSC_REG_H
#define _KSC_REG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/types.h>

struct ksc_para;

union ksc_top_ctrl_reg {
	unsigned int val;
	struct {
		unsigned int ksc_en : 1; // bit0
		unsigned int res0 : 3;
		unsigned int ksc_clk_fixed_en : 1; // bit4
		unsigned int res1 : 3;
		unsigned int ksc_run_mode : 2; // bit9:8
		unsigned int res2 : 2;
		unsigned int ksc_scal_en : 1; // bit12
		unsigned int res3 : 3;
		unsigned int ksc_dn_smpl_fmt : 2;
		unsigned int ksc_csc_dns_type : 1;
		unsigned int res4 : 1;
		unsigned int ksc_csc_mode : 3; // bit22:20
		unsigned int res5 : 1;
		unsigned int ksc_out_wb_fmt : 4;	 // bit27:24
		unsigned int ksc_pix_bit_depth : 1; // bit28
		unsigned int mem_byte_order : 1; // bit28
		unsigned int clk_dmt_sel : 1; // bit28
		unsigned int res6 : 1;
	} bits;
};

union ksc_func_cfg_reg {
	unsigned int val;
	struct {
		unsigned int ksc_post_proc_en : 1; // bit0
		unsigned int res0 : 3;
		unsigned int ksc_perf_mode_en : 1; // bit4
		unsigned int res1 : 3;
		unsigned int filter_lut_update_flag : 1; // bit8
		unsigned int res2 : 3;
		unsigned int hw_write_all_region_flag : 1; // bit12
		unsigned int stitch_mode_flag : 1; // bit13
		unsigned int res3 : 6;
		unsigned int crop_en : 1;	  // bit20
		unsigned int res4 : 3;
		unsigned int rot_angle : 2; // bit25:24
		unsigned int h_flip : 1;    // bit26
		unsigned int v_flip : 1;    // bit27
		unsigned int rec_en : 1;    // bit28
		unsigned int res5 : 3;
	} bits;
};

union ksc_interrupt_reg {
	unsigned int val;
	struct {
		unsigned int ksc_fe_finish_int : 1; // bit0
		unsigned int res0 : 3;
		unsigned int ksc_be_finish_int : 1; // bit4
		unsigned int res1 : 3;
		unsigned int svp2ksc_dmt_error_state : 1;// bit8
		unsigned int res2 : 3;
		unsigned int ksc_fe_under_perf_error_state : 1;      // bit12
		unsigned int res3 : 3;
		unsigned int ksc_be_error_int : 1; // bit16
		unsigned int res4 : 3;
		unsigned int ksc_regup_done_int : 1;//bit20
		unsigned int res5 : 3;
		unsigned int ksc_lut_over_span_int : 1;//bit24
		unsigned int res6 : 3;
		unsigned int ksc_m1_finish_state : 1; //bit28
		unsigned int res7 : 1;
		unsigned int fe_sync_idle : 1; //bit30
		unsigned int be_sync_idle : 1; //bit31
	} bits;
};

union ksc_version_reg {
	unsigned int val;
	struct {
		unsigned int ip_version : 16;  // bit31:16
		unsigned int rtl_version : 16; // bit15:0
	} bits;
};

union ksc_reset_reg {
	unsigned int val;
	struct {
		unsigned int ksc_sw_start : 1; // bit0
		unsigned int res0 : 3;
		unsigned int ksc_sw_reset : 1; // bit4
		unsigned int res1 : 3;
		unsigned int ksc_reg_parse_end : 1; // bit8
		unsigned int res2 : 23;
	} bits;
};

union ksc_intr_en_reg {
	unsigned int val;
	struct {
		unsigned int ksc_fe_finish_int_en : 1; // bit0
		unsigned int res0 : 3;
		unsigned int ksc_be_finish_int_en : 1; // bit4
		unsigned int res1 : 3;
		unsigned int ksc_fe_err_int_en : 1; // bit8
		unsigned int res2 : 3;
		unsigned int ksc_be_err_int_en : 1; // bit12
		unsigned int res3 : 3;
		unsigned int ksc_regup_done_int_en : 1; // bit16
		unsigned int res4 : 3;
		unsigned int ksc_lut_over_span_int_en : 1; // bit20
		unsigned int res5 : 11;
	} bits;
};

union ksc_status_reg {
	unsigned int val;
	struct {
		unsigned int dmt_data_hcnt_err : 1; // bit0
		unsigned int dmt_data_vcnt_err : 1; // bit1
		unsigned int fe_wb_oover_flow_err : 1; // bit2
		unsigned int fe_wb_frm_end_err : 1; // bit3
		unsigned int fe_reset_err : 1; // bit4
		unsigned int fe_start_err : 1; // bit5
		unsigned int be_under_flow_err : 1; // bit6
		unsigned int res0 : 9;
		unsigned int ksc_fe_busy : 1; // bit16
		unsigned int res1 : 3;
		unsigned int ksc_be_busy : 1; // bit20
		unsigned int res2 : 3;
		unsigned int ksc_core_busy : 1; // bit24
		unsigned int res3 : 7;
	} bits;
};

union src_2b_mem0_stride_reg {
	unsigned int val;
	struct {
		unsigned int ksc_src_pix_2b_mem_stride : 13; // bit12:0
		unsigned int res0 : 19;
	} bits;
};

union src_8b_mem0_stride_reg {
	unsigned int val;
	struct {
		unsigned int ksc_src_lum_8b_mem_stride : 13; // bit11:0
		unsigned int res0 : 3;
		unsigned int ksc_src_chm_8b_mem_stride : 13; // bit27:16
		unsigned int res1 : 3;
	} bits;
};

union src_2b_mem1_stride_reg {
	unsigned int val;
	struct {
		unsigned int ksc_src_pix_2b_mem_stride : 13; // bit12:0
		unsigned int res0 : 19;
	} bits;
};

union src_8b_mem1_stride_reg {
	unsigned int val;
	struct {
		unsigned int ksc_src_lum_8b_mem_stride : 13; // bit11:0
		unsigned int res0 : 3;
		unsigned int ksc_src_chm_8b_mem_stride : 13; // bit27:16
		unsigned int res1 : 3;
	} bits;
};

union dst_2b_mem_stride_reg {
	unsigned int val;
	struct {
		unsigned int ksc_dst_pix_2b_mem_stride : 13; // bit11:0
		unsigned int res0 : 19;
	} bits;
};

union dst_8b_mem_stride_reg {
	unsigned int val;
	struct {
		unsigned int ksc_dst_lum_8b_mem_stride : 13; // bit11:0
		unsigned int res0 : 3;
		unsigned int ksc_dst_chm_8b_mem_stride : 13; // bit11:0
		unsigned int res1 : 3;
	} bits;
};

union lut_mem0_stride_reg {
	unsigned int val;
	struct {
		unsigned int ksc_lut_mem0_stride : 12; // bit11:0
		unsigned int res0 : 20;
	} bits;
};

union lut_mem1_stride_reg {
	unsigned int val;
	struct {
		unsigned int ksc_lut_mem1_stride : 12; // bit11:0
		unsigned int res0 : 20;
	} bits;
};

union input_img_size_reg {
	unsigned int val;
	struct {
		unsigned int ksc_ori_img_hgt : 13; // bit11:0
		unsigned int res0 : 3;
		unsigned int ksc_ori_img_wth : 13; // bit27:16
		unsigned int res1 : 3;
	} bits;
};

union dst_img_size_reg {
	unsigned int val;
	struct {
		unsigned int ksc_dst_img_hgt : 13; // bit12:0
		unsigned int res0 : 3;
		unsigned int ksc_dst_img_wth : 13; // bit28:16
		unsigned int res1 : 3;
	} bits;
};

union ksc_scale_ratio_reg {
	unsigned int val;
	struct {
		unsigned int ksc_scale_ratio_x : 15; // bit14:0
		unsigned int res0 : 1;
		unsigned int ksc_scale_ratio_y : 15; // bit30:16
		unsigned int res1 : 1;
	} bits;
};

union ksc_roi_hor_pos_reg {
	unsigned int val;
	struct {
		unsigned int ksc_roi_hor_min_pos : 12; // bit11:0
		unsigned int res0 : 4;
		unsigned int ksc_roi_hor_max_pos : 12; // bit27:16
		unsigned int res1 : 4;
	} bits;
};

union ksc_roi_ver_pos_reg {
	unsigned int val;
	struct {
		unsigned int ksc_roi_ver_min_pos : 12; // bit10:0
		unsigned int res0 : 4;
		unsigned int ksc_roi_ver_max_pos : 12; // bit26:16
		unsigned int res1 : 4;
	} bits;
};

union src_mem0_base_addr_high8_reg {
	unsigned int val;
	struct {
		unsigned int ksc_src_lum_8b_mem_base_addr_h8 : 8; // bit7:0
		unsigned int ksc_src_chm_8b_mem_base_addr_h8 : 8; // bit15:8
		unsigned int ksc_src_pix_2b_mem_base_addr_h8 : 8; // bit15:8
		unsigned int res0 : 8;
	} bits;
};

union src_lum_8b_mem0_addr_reg {
	unsigned int val;
	struct {
		unsigned int ksc_src_lum_8b_mem_addr : 32; // bit31:0
	} bits;
};

union src_chm_8b_mem0_addr_reg {
	unsigned int val;
	struct {
		unsigned int ksc_src_lum_8b_mem_addr : 32; // bit31:0
	} bits;
};

union src_pix_2b_mem0_addr_reg {
	unsigned int val;
	struct {
		unsigned int ksc_src_pix_2b_mem_base_addr : 32; // bit31:0
	} bits;
};

union src_mem1_base_addr_high8_reg {
	unsigned int val;
	struct {
		unsigned int ksc_src_lum_8b_mem1_addr_high8 : 8; // bit7:0
		unsigned int ksc_src_chm_8b_mem1_addr_high8 : 8; // bit15:8
		unsigned int ksc_src_pix_2b_mem_addr_high8 : 8; // bit15:8
		unsigned int res0 : 8;
	} bits;
};

union src_lum_8b_mem1_addr_reg {
	unsigned int val;
	struct {
		unsigned int ksc_src_lum_8b_mem_addr : 32; // bit31:0
	} bits;
};

union src_chm_8b_mem1_addr_reg {
	unsigned int val;
	struct {
		unsigned int ksc_src_chm_8b_mem_addr : 32; // bit31:0
	} bits;
};


union src_pix_2b_mem1_addr_reg {
	unsigned int val;
	struct {
		unsigned int ksc_src_pix_2b_mem_addr : 32; // bit31:0
	} bits;
};


union dst_mem_base_addr_high8_reg {
	unsigned int val;
	struct {
		unsigned int ksc_dst_lum_8b_mem_addr_high8 : 8; // bit7:0
		unsigned int ksc_dst_chm_8b_mem_addr_high8 : 8; // bit15:8
		unsigned int ksc_dst_pix_2b_mem_addr_high8 : 8; // bit15:8
		unsigned int res0 : 8;
	} bits;
};

union dst_lum_8b_mem_addr_reg {
	unsigned int val;
	struct {
		unsigned int ksc_dst_lum_8b_mem_addr : 32; // bit31:0
	} bits;
};

union dst_chm_8b_mem_addr_reg {
	unsigned int val;
	struct {
		unsigned int ksc_dst_chm_8b_mem_addr : 32; // bit31:0
	} bits;
};

union dst_pix_2b_mem_addr_reg {
	unsigned int val;
	struct {
		unsigned int ksc_dst_pix_2b_mem_addr : 32; // bit31:0
	} bits;
};

union lut_mem0_base_addr_high8_reg {
	unsigned int val;
	struct {
		unsigned int ksc_lut_mem0_base_addr_high8 : 8; // bit7:0
		unsigned int res0 : 24;
	} bits;
};

union lut_mem0_base_addr_reg {
	unsigned int val;
	struct {
		unsigned int ksc_lut_mem0_base_addr : 32; // bit31:0
	} bits;
};

union ksc_border_blur1_reg {
	unsigned int val;
	struct {
		unsigned int border_pix_dif_v1 : 5; // bit4:0
		unsigned int res0 : 3;
		unsigned int border_pix_num_v1 : 4; // bit11:8
		unsigned int border_pix_dif_h1 : 5; // bit16:12
		unsigned int res1 : 3;
		unsigned int border_pix_num_h1 : 4; // bit23:20
		unsigned int res2 : 8;
	} bits;
};

union ksc_border_blur0_reg {
	unsigned int val;
	struct {
		unsigned int border_pix_dif_v0 : 5; // bit4:0
		unsigned int res0 : 3;
		unsigned int border_pix_num_v0 : 4; // bit11:8
		unsigned int border_pix_dif_h0 : 5; // bit16:12
		unsigned int res1 : 3;
		unsigned int border_pix_num_h0 : 4; // bit23:20
		unsigned int res2 : 8;
	} bits;
};


union lut_mem1_base_addr_reg {
	unsigned int val;
	struct {
		unsigned int ksc_lut_mem1_base_addr : 32; // bit31:0
	} bits;
};

union interp_coef_base_addr_high8_reg {
	unsigned int val;
	struct {
		unsigned int ksc_interp_coef_base_addr_high8 : 8; // bit7:0
		unsigned int res0 : 24;
	} bits;
};

union interp_coef_base_addr_reg {
	unsigned int val;
	struct {
		unsigned int ksc_interp_coef_base_addr : 32; // bit31:0
	} bits;
};

union dnscal_coef_base_addr_high8_reg {
	unsigned int val;
	struct {
		unsigned int ksc_dnscal_coef_base_addr_high8 : 8; // bit7:0
		unsigned int res0 : 24;
	} bits;
};

union dnscal_coef_base_addr_reg {
	unsigned int val;
	struct {
		unsigned int ksc_dnscal_coef_base_addr : 32; // bit31:0
	} bits;
};

union ksc_fe_run_pos_reg {
	unsigned int val;
	struct {
		unsigned int fe_pos_y : 16; // bit31:16
		unsigned int fe_pos_x : 16; // bit15:0
	} bits;
};

union ksc_be_run_pos_reg {
	unsigned int val;
	struct {
		unsigned int be_pos_y : 16; // bit31:16
		unsigned int be_pos_x : 16; // bit15:0
	} bits;
};

union ksc_cache_blk_pos_reg {
	unsigned int val;
	struct {
		unsigned int ksc_cache_blk_pos : 32; // bit31:0
	} bits;
};

union ksc_pix_out_pos_reg {
	unsigned int val;
	struct {
		unsigned int ksc_pix_out_pos : 32; // bit31:0
	} bits;
};

union ksc_mode1_config_reg {
	unsigned int val;
	struct {
		unsigned int run_mode1_flag : 1; // bit0
		unsigned int res0 : 7;
		unsigned int run_mode1_maxnum_plus1: 8; //15:8
		unsigned int run_mode1_write_num: 8; //23:16
		unsigned int res1 : 8;
	} bits;
};

union ksc_crop_offset_reg {
	unsigned int val;
	struct {
		unsigned int crop_x : 12; // bit11:0
		unsigned int res0 : 4;
		unsigned int crop_y : 12; // bit27:16
		unsigned int res1 : 4;
	} bits;
};

union ksc_scale_size_for_crop_reg {
	unsigned int val;
	struct {
		unsigned int scale_height_minus1_for_crop : 13; // bit12:0
		unsigned int res0 : 3;
		unsigned int scale_width_minus1_for_crop : 13; // bit28:16
		unsigned int res1 : 3;
	} bits;
};

union ksc_out_pos_offset_reg {
	unsigned int val;
	struct {
		unsigned int ksc_pos_ost_h : 13; // bit12:0
		unsigned int res0 : 3;
		unsigned int ksc_pos_ost_v : 13; // bit28:16
		unsigned int res1 : 3;
	} bits;
};

union ksc_phase_offset_reg {
	unsigned int val;
	struct {
		unsigned int ksc_phase_h : 5;
		unsigned int res0 : 3;
		unsigned int ksc_phase_v : 5;
		unsigned int res1 : 3;
		unsigned int alpha_default_val : 8;
		unsigned int res2 : 8;
	} bits;
};

//------KSC Core register------------//
union ksc_default_pix_reg {
	unsigned int val;
	struct {
		unsigned int ksc_default_pix_ch0 : 10; // bit9:0
		unsigned int ksc_default_pix_ch1 : 10; // bit19:10
		unsigned int ksc_default_pix_ch2 : 10; // bit29:20
		unsigned int res0 : 2;
	} bits;
};

union ksc_detail_control_reg {
	unsigned int val;
	struct {
		unsigned int ksc_detail_white_gain : 8;     // bit7:0
		unsigned int ksc_detail_black_gain : 8;     // bit15:8
		unsigned int ksc_detail_white_max_clip : 8; // bit23:16
		unsigned int ksc_detail_black_max_clip : 8; // bit31:24
	} bits;
};

union ksc_filt_sel_reg {
	unsigned int val;
	struct {
		unsigned int ksc_flt_sel_lo_thr : 6; // bit5:0
		unsigned int res0 : 2;
		unsigned int ksc_flt_sel_hi_thr : 7; // bit14:8
		unsigned int res1 : 1;
		unsigned int ksc_bord_flt_type : 2; // bit17:16
		unsigned int res2 : 14;
	} bits;
};

union ksc_anti_alias_thr_reg {
	unsigned int val;
	struct {
		unsigned int ksc_anti_alias_lo_thr : 6; // bit5:0
		unsigned int res0 : 2;
		unsigned int ksc_anti_alias_hi_thr : 7; // bit14:8
		unsigned int res1 : 1;
		unsigned int ksc_anti_alias_ref : 9; // bit24:16
		unsigned int res2 : 7;
	} bits;
};

union ksc_anti_alias_str_reg {
	unsigned int val;
	struct {
		unsigned int ksc_dir_rsid_avg_str : 8; // bit7:0
		unsigned int res0 : 8;
		unsigned int ksc_blend_weight_str : 8; // bit23:16
		unsigned int res1 : 8;
	} bits;
};

union ksc_dir_rsid_reg {
	unsigned int val;
	struct {
		unsigned int ksc_dir_rsid_avg_gain : 7; // bit6:0
		unsigned int res0 : 1;
		unsigned int ksc_dir_rsid_max_ratio : 7; // bit14:8
		unsigned int res1 : 1;
		unsigned int ksc_dir_rsid_min_clip : 7; // bit22:16
		unsigned int res2 : 1;
		unsigned int ksc_dir_rsid_max_clip : 7; // bit30:24
		unsigned int res3 : 1;
	} bits;
};

union ksc_dir_max_rsid_ratio_reg {
	unsigned int val;
	struct {
		unsigned int ksc_dir_pos_max_rsid_ratio : 8; // bit7:0
		unsigned int res0 : 8;
		unsigned int ksc_dir_neg_max_rsid_ratio : 8; // bit23:16
		unsigned int res1 : 8;
	} bits;
};

union ksc_default_rgb_reg {
	unsigned int val;
	struct {
		unsigned int ksc_default_rgb_ch0 : 10; // bit7:0
		unsigned int ksc_default_rgb_ch1 : 10; // bit15:8
		unsigned int ksc_default_rgb_ch2 : 10; // bit23:16
		unsigned int res0 : 2;   // bit31:24
	} bits;
};



union mem_req_bl_num_reg {
	unsigned int val;
	struct {
		unsigned int mem_req_bl_max : 5; // bit4:0
		unsigned int res0 : 11;
		unsigned int ksc_fe_wb_wait_num : 16; // bit31:16
	} bits;
};

union mode0_hw_control_reg {
	unsigned int val;
	struct {
		unsigned int hw_rst_cycle_num : 16; // bit15:0
		unsigned int ksc_1st_frm_en : 1;    // bit16
		unsigned int res0 : 15;
	} bits;
};

union ksc_mode1_lum_buf_size_reg {
	unsigned int val;
	struct {
		unsigned int ksc_m1_lum_buf_size : 20;
		unsigned int res0 : 12;
	} bits;
};


union ksc_mode1_chm_buf_size_reg {
	unsigned int val;
	struct {
		unsigned int ksc_m1_chm_buf_size : 21;
		unsigned int res0 : 11;
	} bits;
};

union ksc_mode1_pix2b_buf_size_reg {
	unsigned int val;
	struct {
		unsigned int ksc_m1_pix2b_buf_size : 21;
		unsigned int res0 : 11;
	} bits;
};

union ksc_debug_info_0_reg {
	unsigned int val;
	struct {
		unsigned int ksc_debug_info_0 : 32; // bit31:0
	} bits;
};

union ksc_debug_info_1_reg {
	unsigned int val;
	struct {
		unsigned int ksc_debug_info_1 : 32; // bit31:0
	} bits;
};

union ksc_debug_info_2_reg {
	unsigned int val;
	struct {
		unsigned int ksc_debug_info_2 : 32; // bit31:0
	} bits;
};

union ksc_debug_info_3_reg {
	unsigned int val;
	struct {
		unsigned int ksc_debug_info_3 : 32; // bit31:0
	} bits;
};

union ksc_fpga_in_rgb_addr_reg {
	unsigned int val;
	struct {
		unsigned int fpga_in_rgb_addr : 32; // bit31:0
	} bits;
};

union ksc_fpga_in_rgb_stride_reg {
	unsigned int val;
	struct {
		unsigned int fpga_in_rgb_stride : 12; // bit11:0
		unsigned int res0 : 20;
	} bits;
};

struct ksc_regs {
	//0~0xc
	union ksc_top_ctrl_reg ksc_top_ctrl;
	union ksc_func_cfg_reg ksc_func_cfg;
	union ksc_interrupt_reg ksc_intr;
	union ksc_version_reg version;

	//0x10~0x1c
	union ksc_reset_reg reset;
	union ksc_intr_en_reg intr_en;
	union ksc_status_reg status;
	union src_2b_mem0_stride_reg src_2b_mem0_stride;

	//0x20~0x2c
	union src_8b_mem0_stride_reg src_8b_mem0_stride;
	union src_2b_mem1_stride_reg src_2b_mem1_stride;
	union src_8b_mem1_stride_reg src_8b_mem1_stride;
	union dst_2b_mem_stride_reg dst_2b_mem_stride;

	//0x30~0x3c
	union dst_8b_mem_stride_reg  dst_8b_mem_stride;
	union lut_mem0_stride_reg lut_mem0_stride;
	union mode0_hw_control_reg mode0_hw_control;
	union input_img_size_reg input_img_size;


	//0x40~0x4c
	union dst_img_size_reg dst_img_size;
	union ksc_scale_ratio_reg ksc_scale_ratio;
	union ksc_roi_hor_pos_reg ksc_roi_hor_pos;
	union ksc_roi_ver_pos_reg ksc_roi_ver_pos;

	//0x50~0x5c
	union src_mem0_base_addr_high8_reg src_mem0_base_addr_high8;
	union src_lum_8b_mem0_addr_reg src_lum_8b_mem0_addr;
	union src_chm_8b_mem0_addr_reg src_chm_8b_mem0_addr;
	union src_pix_2b_mem0_addr_reg  src_pix_2b_mem0_addr;

	//0x60~0x6c
	union src_mem1_base_addr_high8_reg src_mem1_base_addr_high8;
	union src_lum_8b_mem1_addr_reg src_lum_8b_mem1_addr;
	union src_chm_8b_mem1_addr_reg src_chm_8b_mem1_addr;
	union src_pix_2b_mem1_addr_reg src_pix_2b_mem1_addr;

	//0x70~0x7c
	union dst_mem_base_addr_high8_reg dst_mem_base_addr_high8;
	union dst_lum_8b_mem_addr_reg dst_lum_8b_mem_addr;
	union dst_chm_8b_mem_addr_reg dst_chm_8b_mem_addr;
	union dst_pix_2b_mem_addr_reg dst_pix_2b_mem_add;

	//0x80~0x8c
	union lut_mem0_base_addr_high8_reg lut_mem0_base_addr_high8;
	union lut_mem0_base_addr_reg lut_mem0_base_addr;
	union ksc_border_blur1_reg ksc_border_blur1;
	union ksc_border_blur0_reg ksc_border_blur0;

	//0x90~0x9c
	union interp_coef_base_addr_high8_reg interp_coef_base_addr_high8;
	union interp_coef_base_addr_reg interp_coef_base_addr;
	union dnscal_coef_base_addr_high8_reg dnscal_coef_base_addr_high8;
	union dnscal_coef_base_addr_reg dnscal_coef_base_addr;

	//0xa0~0xac
	union ksc_fe_run_pos_reg ksc_fe_run_pos;
	union ksc_be_run_pos_reg ksc_be_run_pos;
	union ksc_cache_blk_pos_reg ksc_cache_blk_pos;
	union ksc_pix_out_pos_reg ksc_pix_out_pos;

	//0xb0~0xbc
	union ksc_mode1_config_reg  ksc_mode1_config;
	union ksc_crop_offset_reg ksc_crop_offset;
	union ksc_scale_size_for_crop_reg ksc_scale_size_for_crop;
	union ksc_out_pos_offset_reg ksc_out_pos_offset;

	//0xc0~0xcc
	union ksc_default_pix_reg ksc_default_pix;
	union ksc_detail_control_reg ksc_detail_control;
	union ksc_filt_sel_reg ksc_filt_sel;
	union ksc_anti_alias_thr_reg ksc_anti_alias_thr;

	//0xd0~0xdc
	union ksc_anti_alias_str_reg ksc_anti_alias_str;
	union ksc_dir_rsid_reg ksc_dir_rsid;
	union ksc_dir_max_rsid_ratio_reg ksc_dir_max_rsid_ratio;
	union ksc_phase_offset_reg ksc_phase_offset;

	//0xe0~0xec
	union ksc_default_rgb_reg ksc_default_rgb;
	union mem_req_bl_num_reg mem_req_bl_num;
	union ksc_mode1_lum_buf_size_reg ksc_m1_lum_buf_size;
	union ksc_mode1_chm_buf_size_reg ksc_m1_chm_buf_size;

	//0xf0~0xfc
	union ksc_mode1_pix2b_buf_size_reg ksc_m1_pix2b_buf_size;
	union ksc_debug_info_0_reg ksc_debug_info_0;
	union ksc_debug_info_1_reg ksc_debug_info_1;
	union ksc_debug_info_2_reg ksc_debug_info_2;

	//0x100
	union ksc_debug_info_3_reg ksc_debug_info_3;
};




#ifdef __cplusplus
}
#endif

#endif /*End of file*/
