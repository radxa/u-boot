/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef _ROCKCHIP_SHARP_REGS_H_
#define _ROCKCHIP_SHARP_REGS_H_

#include <linux/types.h>

#define SHARP_FULL_REGS_LENGTH	173
#define SHARP_LITE_REGS_LENGTH	14

#define SHARP_REG_SIZE (0x02B4)
#define SHP_PEAKING_CORE_HOR 11
#define SHP_FILT_H_CENTER (SHP_PEAKING_CORE_HOR >> 1)
#define SHP_PADDING_RADIUS_H (SHP_PEAKING_CORE_HOR >> 1)
#define SHP_PEAKING_CORE_VER 5
#define SHP_FILT_V_CENTER (SHP_PEAKING_CORE_VER >> 1)
#define SHP_PADDING_RADIUS_V (SHP_PEAKING_CORE_VER >> 1)

#define SHP_PEAKING_BAND_NUM 8
#define SHP_GAINCTRL_LUTLENGTH 1024

/* Sharp configuration */
struct rkvop_cfg_lti_struct {
	u8 lti_enable;
	u16 lti_radius;
	u16 lti_slope;
	u16 lti_thresold;
	u16 lti_gain;
	u16 lti_noise_thr_pos;
	u16 lti_noise_thr_neg;
};

struct rkvop_cfg_peaking_struct {
	u8 peaking_enable;
	u16 peaking_gain;	/* U10: [0, 1023], default: 300 */

	u8 coring_enable;	/* U1: [0, 1] */
	u16 coring_zero[8];
	u16 coring_thr[8];	/* U10, [0, 1023] */
	u16 coring_ratio[8];

	u8 gain_enable;		/* U1: [0, 1] */
	u16 gain_pos[8];	/* U11+: [0, 2048] */
	u16 gain_neg[8];	/* U11+: [0, 2048] */

	u8 limit_ctrl_enable;		/* U1: [0, 1] */
	u16 limit_ctrl_pos0[8];		/* U11+, [0, 2048] */
	u16 limit_ctrl_pos1[8];		/* U11+, [0, 2048] */
	u16 limit_ctrl_neg0[8];		/* U11+, [0, 2048] */
	u16 limit_ctrl_neg1[8];		/* U11+, [0, 2048] */
	u16 limit_ctrl_ratio[8];	/* U11+, [0, 2048] */
	u16 limit_ctrl_bnd_pos[8];	/* U11+, [0, 2048] */
	u16 limit_ctrl_bnd_neg[8];	/* U11+, [0, 2048] */

	u8 shoot_adj_enable;
	u8 shoot_adj_delta_offset[8];
	u8 shoot_adj_alpha_over[8];
	u8 shoot_adj_alpha_under[8];
	u8 shoot_adj_alpha_over_unlimit[8];
	u8 shoot_adj_alpha_under_unlimit[8];

	u8 edge_ctrl_enable;
	u8 edge_ctrl_non_dir_thr;
	u8 edge_ctrl_dir_cmp_ratio;
	u8 edge_ctrl_non_dir_wgt_offset;
	u8 edge_ctrl_non_dir_wgt_ratio;
	u8 edge_ctrl_dir_cnt_thr;
	u8 edge_ctrl_dir_cnt_avg;
	u8 edge_ctrl_dir_cnt_offset;
	u8 edge_ctrl_diag_dir_thr;
	u8 edge_ctrl_diag_adj_gain_tab[8];

	u8 edge_shoot_ctrl_enable;
	u8 edge_shoot_ctrl_delta_offset_h;
	u8 edge_shoot_ctrl_alpha_over_h;
	u8 edge_shoot_ctrl_alpha_under_h;
	u8 edge_shoot_ctrl_alpha_over_unlimit_h;
	u8 edge_shoot_ctrl_alpha_under_unlimit_h;
	u8 edge_shoot_ctrl_delta_offset_v;
	u8 edge_shoot_ctrl_alpha_over_v;
	u8 edge_shoot_ctrl_alpha_under_v;
	u8 edge_shoot_ctrl_alpha_over_unlimit_v;
	u8 edge_shoot_ctrl_alpha_under_unlimit_v;
	u8 edge_shoot_ctrl_delta_offset_d0;
	u8 edge_shoot_ctrl_alpha_over_d0;
	u8 edge_shoot_ctrl_alpha_under_d0;
	u8 edge_shoot_ctrl_alpha_over_unlimit_d0;
	u8 edge_shoot_ctrl_alpha_under_unlimit_d0;
	u8 edge_shoot_ctrl_delta_offset_d1;
	u8 edge_shoot_ctrl_alpha_over_d1;
	u8 edge_shoot_ctrl_alpha_under_d1;
	u8 edge_shoot_ctrl_alpha_over_unlimit_d1;
	u8 edge_shoot_ctrl_alpha_under_unlimit_d1;
	u8 edge_shoot_ctrl_delta_offset_non;
	u8 edge_shoot_ctrl_alpha_over_non;
	u8 edge_shoot_ctrl_alpha_under_non;
	u8 edge_shoot_ctrl_alpha_over_unlimit_non;
	u8 edge_shoot_ctrl_alpha_under_unlimit_non;
	u8 filter_cfg_diag_enh_coef;
	s16 filter_cfg_filt_core_H0[6];
	s16 filter_cfg_filt_core_H1[6];
	s16 filter_cfg_filt_core_H2[6];
	s16 filter_cfg_filt_core_H3[6];
	s16 filter_cfg_filt_core_V0[3];
	s16 filter_cfg_filt_core_V1[3];
	s16 filter_cfg_filt_core_V2[3];
	s16 filter_cfg_filt_core_USM[3];
};

struct rkvop_cfg_shootctrl_struct {
	u8 shootctrl_enable;
	u8 filter_radius;
	u8 delta_offset;
	u16 alpha_over;			/* U7: [0, 127] */
	u16 alpha_under;		/* U7: [0, 127] */
	u16 alpha_over_unlimit;		/* U7: [alpha_over, 127] */
	u16 alpha_under_unlimit;	/* U7: [alpha_under, 127] */
};

struct rkvop_cfg_global_gain_struct {
	u8 global_gain_enable;
	u8 lum_mode;
	u16 lum_grd[6];
	u8 lum_val[6];
	u16 adp_grd[6];
	u8 adp_val[6];
	u16 var_grd[6];
	u8 var_val[6];
};

struct rkvop_cfg_color_ctrl_struct {
	u8 color_ctrl_enable;

	u8 p0_scaling_coef;
	u16 p0_ctrl_point_u;
	u16 p0_ctrl_point_v;
	u8 p0_ctrl_roll_tab[16];

	u8 p1_scaling_coef;
	u16 p1_ctrl_point_u;
	u16 p1_ctrl_point_v;
	u8 p1_ctrl_roll_tab[16];

	u8 p2_scaling_coef;
	u16 p2_ctrl_point_u;
	u16 p2_ctrl_point_v;
	u8 p2_ctrl_roll_tab[16];

	u8 p3_scaling_coef;
	u16 p3_ctrl_point_u;
	u16 p3_ctrl_point_v;
	u8 p3_ctrl_roll_tab[16];
};

struct rkvop_cfg_texture_adj_struct {
	u8 textureAdj_enable;
	u8 y_mode_select;
	u8 idx_mode_select;
	u16 texture_grd[6];
	u8 texture_val[6];	/* U7, [0, 127] */
};

typedef struct {
	u8 sharp_enable;

	/* params config */
	struct rkvop_cfg_lti_struct sharp_lti_h_cfg;
	struct rkvop_cfg_lti_struct sharp_cti_h_cfg;
	struct rkvop_cfg_lti_struct sharp_lti_v_cfg;
	struct rkvop_cfg_lti_struct sharp_cti_v_cfg;
	struct rkvop_cfg_peaking_struct sharp_peaking_cfg;
	struct rkvop_cfg_shootctrl_struct sharp_shootctrl_cfg;
	struct rkvop_cfg_global_gain_struct sharp_global_gain_cfg;
	struct rkvop_cfg_color_ctrl_struct sharp_color_adj_cfg;
	struct rkvop_cfg_texture_adj_struct sharp_texture_adj_cfg;
	/* TODO: add roi info */
} rk_hwpq_shp_cfg;

typedef struct sharp_edge_proc_cfg {
	u8 edge_proc_enable;	/* u1bit [0, 1] */
	u8 non_dir_thr;		/* u7bit [0, 127] */
	u8 dir_cmp_ratio;	/* u4bit [0, 15] */
	u8 non_dir_wgt_ratio;	/* u5bit [0, 31], 4bitFix */
	u8 non_dir_wgt_offset;	/* u8bit [0, 255] */

	u8 dir_cnt_thr;		/* u4bit [0, 12] */
	u8 dir_cnt_avg;		/* u3bit [0, 7] */
	u8 dir_cnt_offset;	/* u4bit [0, 15] */

	u8 diag_adj_gain_tab[8];	/* u4bit [0, 15] */
	u8 diag_dir_thr;		/* u7bit [0, 127] */
} sharp_edge_proc_cfg;

typedef struct sharp_edge_shoot_ctrl_cfg {
	u8 edge_shoot_ctrl_enable;	/* u1bit [0, 1] */
	u8 delta_offset_h;		/* u8bit, [0, 255] */
	u8 alpha_over_h;		/* u7bit, [0, 127], 7bitFix */
	u8 alpha_under_h;		/* u7bit, [0, 127], 7bitFix */
	u8 alpha_over_unlimit_h;	/* u7bit, [0, 127], 7bitFix */
	u8 alpha_under_unlimit_h;	/* u7bit, [0, 127], 7bitFix */
	u8 delta_offset_v;		/* u8bit, [0, 255] */
	u8 alpha_over_v;		/* u7bit, [0, 127], 7bitFix */
	u8 alpha_under_v;		/* u7bit, [0, 127], 7bitFix */
	u8 alpha_over_unlimit_v;	/* u7bit, [0, 127], 7bitFix */
	u8 alpha_under_unlimit_v;	/* u7bit, [0, 127], 7bitFix */
	u8 delta_offset_d0;		/* u8bit, [0, 255] */
	u8 alpha_over_d0;		/* u7bit, [0, 127], 7bitFix */
	u8 alpha_under_d0;		/* u7bit, [0, 127], 7bitFix */
	u8 alpha_over_unlimit_d0;	/* u7bit, [0, 127], 7bitFix */
	u8 alpha_under_unlimit_d0;	/* u7bit, [0, 127], 7bitFix */
	u8 delta_offset_d1;		/* u8bit, [0, 255] */
	u8 alpha_over_d1;		/* u7bit, [0, 127], 7bitFix */
	u8 alpha_under_d1;		/* u7bit, [0, 127], 7bitFix */
	u8 alpha_over_unlimit_d1;	/* u7bit, [0, 127], 7bitFix */
	u8 alpha_under_unlimit_d1;	/* u7bit, [0, 127], 7bitFix */
	u8 delta_offset_non;		/* u8bit, [0, 255] */
	u8 alpha_over_non;		/* u7bit, [0, 127], 7bitFix */
	u8 alpha_under_non;		/* u7bit, [0, 127], 7bitFix */
	u8 alpha_over_unlimit_non;	/* u7bit, [0, 127], 7bitFix */
	u8 alpha_under_unlimit_non;	/* u7bit, [0, 127], 7bitFix */
} sharp_edge_shoot_ctrl_cfg;

typedef struct sharp_gain_ctrl_cfg {
	u16	adp_idx0;	/* u10bit, [0, 1023] */
	u16	adp_idx1;	/* u10bit, [0, 1023] */
	u16	adp_idx2;	/* u10bit, [0, 1023] */
	u16	adp_idx3;	/* u10bit, [0, 1023] */
	u8 adp_gain0;		/* u7bit, [0, 127], 6bitFix */
	u8 adp_gain1;		/* u7bit, [0, 127], 6bitFix */
	u8 adp_gain2;		/* u7bit, [0, 127], 6bitFix */
	u8 adp_gain3;		/* u7bit, [0, 127], 6bitFix */
	u8 adp_gain4;		/* u7bit, [0, 127], 6bitFix */
	/* u8 adp_gain5; u7bit, [0, 127], 6bitFix NO USE */
	s16 adp_slp01;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
	s16 adp_slp12;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
	s16 adp_slp23;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
	s16 adp_slp34;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
	s16 adp_slp45;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */

	u16	var_idx0;	/* u10bit, [0, 1023] */
	u16	var_idx1;	/* u10bit, [0, 1023] */
	u16	var_idx2;	/* u10bit, [0, 1023] */
	u16	var_idx3;	/* u10bit, [0, 1023] */
	/* u16 var_idx4; u10bit, [0, 1023] */
	u8 var_gain0;		/* u7bit, [0, 127], 6bitFix */
	u8 var_gain1;		/* u7bit, [0, 127], 6bitFix */
	u8 var_gain2;		/* u7bit, [0, 127], 6bitFix */
	u8 var_gain3;		/* u7bit, [0, 127], 6bitFix */
	u8 var_gain4;		/* u7bit, [0, 127], 6bitFix */
	/* u8 var_gain5; u7bit, [0, 127], 6bitFix NO USE */
	s16 var_slp01;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
	s16 var_slp12;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
	s16 var_slp23;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
	s16 var_slp34;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
	s16 var_slp45;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */

	u8	lum_select;	/* u2bit, [0, 3] */
	u16	lum_idx0;	/* u10bit, [0, 1023] */
	u16	lum_idx1;	/* u10bit, [0, 1023] */
	u16	lum_idx2;	/* u10bit, [0, 1023] */
	u16	lum_idx3;	/* u10bit, [0, 1023] */
	/* u16 lum_idx4; u10bit, [0, 1023] */
	u8 lum_gain0;		/* u7bit, [0, 127], 6bitFix */
	u8 lum_gain1;		/* u7bit, [0, 127], 6bitFix */
	u8 lum_gain2;		/* u7bit, [0, 127], 6bitFix */
	u8 lum_gain3;		/* u7bit, [0, 127], 6bitFix */
	u8 lum_gain4;		/* u7bit, [0, 127], 6bitFix */
	/* u8 lum_gain5; u7bit, [0, 127], 6bitFix NO USE */
	s16 lum_slp01;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
	s16 lum_slp12;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
	s16 lum_slp23;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
	s16 lum_slp34;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
	s16 lum_slp45;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
} sharp_gain_ctrl_cfg;

typedef struct sharp_texture_adj_cfg {
	u8 idx_mode_select;	/* u1bit, [0, 1], 0: use direct_wgt, 1: use abs(highFreq) */
	u8 Y_mode_select;	/* u2bit, [0, 3] */
	u16	tex_idx0;	/* u10bit, [0, 1023] */
	u16	tex_idx1;	/* u10bit, [0, 1023] */
	u16	tex_idx2;	/* u10bit, [0, 1023] */
	u16	tex_idx3;	/* u10bit, [0, 1023] */
	u8 tex_gain0;		/* u7bit, [0, 127], 7bitFix, wgt of LowFreq Y */
	u8 tex_gain1;		/* u7bit, [0, 127], 7bitFix, wgt of LowFreq Y */
	u8 tex_gain2;		/* u7bit, [0, 127], 7bitFix, wgt of LowFreq Y */
	u8 tex_gain3;		/* u7bit, [0, 127], 7bitFix, wgt of LowFreq Y */
	u8 tex_gain4;		/* u7bit, [0, 127], 7bitFix, wgt of LowFreq Y */
	s16 tex_slp01;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
	s16 tex_slp12;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
	s16 tex_slp23;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
	s16 tex_slp34;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
	s16 tex_slp45;		/* s11bit, 10bitData + 1bitSign [-1024, 1023], 7bitFix */
} sharp_texture_adj_cfg;

typedef struct color_adj_point_cfg_t {
	u16	adj_point_x;		/* u10bit */
	u16	adj_point_y;		/* u10bit */
	u8	adj_scaling_coef;	/* u3bit, [0, 6] */
	u8	color_adj_tab[16];	/* u5bit, 4bitFix */
} sharp_color_adj_cfg;

typedef struct sharp_peaking_cfg {
	/* Peaking Ctrl(coring/gain/limitCtrl) Params */
	u8 peaking_ctrl_enable;					/* u1bit */
	s16 peaking_ctrl_idx_N0[SHP_PEAKING_BAND_NUM];		/* s11bit, 10bitData + 1bitSign, 0bitFix */
	s16 peaking_ctrl_idx_N1[SHP_PEAKING_BAND_NUM];		/* s11bit, 10bitData + 1bitSign, 0bitFix */
	s16 peaking_ctrl_idx_N2[SHP_PEAKING_BAND_NUM];		/* s11bit, 10bitData + 1bitSign, 0bitFix */
	s16 peaking_ctrl_idx_N3[SHP_PEAKING_BAND_NUM];		/* s11bit, 10bitData + 1bitSign, 0bitFix */
	s16 peaking_ctrl_idx_P0[SHP_PEAKING_BAND_NUM];		/* s11bit, 10bitData + 1bitSign, 0bitFix */
	s16 peaking_ctrl_idx_P1[SHP_PEAKING_BAND_NUM];		/* s11bit, 10bitData + 1bitSign, 0bitFix */
	s16 peaking_ctrl_idx_P2[SHP_PEAKING_BAND_NUM];		/* s11bit, 10bitData + 1bitSign, 0bitFix */
	s16 peaking_ctrl_idx_P3[SHP_PEAKING_BAND_NUM];		/* s11bit, 10bitData + 1bitSign, 0bitFix */
	s16 peaking_ctrl_value_N1[SHP_PEAKING_BAND_NUM];	/* s11bit, 10bitData + 1bitSign, 0bitFix */
	s16 peaking_ctrl_value_N2[SHP_PEAKING_BAND_NUM];	/* s11bit, 10bitData + 1bitSign, 0bitFix */
	s16 peaking_ctrl_value_N3[SHP_PEAKING_BAND_NUM];	/* s11bit, 10bitData + 1bitSign, 0bitFix */
	s16 peaking_ctrl_value_P1[SHP_PEAKING_BAND_NUM];	/* s11bit, 10bitData + 1bitSign, 0bitFix */
	s16 peaking_ctrl_value_P2[SHP_PEAKING_BAND_NUM];	/* s11bit, 10bitData + 1bitSign, 0bitFix */
	s16 peaking_ctrl_value_P3[SHP_PEAKING_BAND_NUM];	/* s11bit, 10bitData + 1bitSign, 0bitFix */
	u16 peaking_ctrl_ratio_P01[SHP_PEAKING_BAND_NUM];	/* u12bit, 12bitData, 10itFix */
	u16 peaking_ctrl_ratio_P12[SHP_PEAKING_BAND_NUM];	/* u12bit, 12bitData, 10itFix */
	u16 peaking_ctrl_ratio_P23[SHP_PEAKING_BAND_NUM];	/* u12bit, 12bitData, 10itFix */
	u16 peaking_ctrl_ratio_N01[SHP_PEAKING_BAND_NUM];	/* u12bit, 12bitData, 10itFix */
	u16 peaking_ctrl_ratio_N12[SHP_PEAKING_BAND_NUM];	/* u12bit, 12bitData, 10itFix */
	u16 peaking_ctrl_ratio_N23[SHP_PEAKING_BAND_NUM];	/* u12bit, 12bitData, 10itFix */

	/* Peaking ShootCtrl for Each Band */
	u8 peaking_shtctrl_enable;					/* u1bit */
	u8 peaking_shtctrl_delta_offset[SHP_PEAKING_BAND_NUM];		/* u8bit, [0, 255] */
	u8 peaking_shtctrl_alpha_over[SHP_PEAKING_BAND_NUM];		/* u7bit, 7bitFix */
	u8 peaking_shtctrl_alpha_under[SHP_PEAKING_BAND_NUM];		/* u7bit, 7bitFix */
	u8 peaking_shtctrl_alpha_over_unlimit[SHP_PEAKING_BAND_NUM];	/* u7bit, 7bitFix */
	u8 peaking_shtctrl_alpha_under_unlimit[SHP_PEAKING_BAND_NUM];	/* u7bit, 7bitFix */

	/* Peaking Edge Process */
	sharp_edge_proc_cfg edge_proc_cfg;

	/* Peaking Edgt ShootCtrl Process */
	sharp_edge_shoot_ctrl_cfg edge_shoot_ctrl_cfg;

	/* Peaking Gain */
	u16 peaking_gain;	/* u10bit, 7bitFix */
} sharp_peaking_cfg;

typedef struct sharp_shoot_ctrl_cfg {
	u16 shoot_ctrl_filt_radius;		/* u1bit, 0: radius=1, 1: radius=2 */
	u8 shoot_ctrl_delta_offset;		/* u8bit, [0, 255] */
	u16 shoot_ctrl_alpha_over;		/* u7bit, 7bitFix */
	u16 shoot_ctrl_alpha_under;		/* u7bit, 7bitFix */
	u16 shoot_ctrl_alpha_over_unlimit;	/* u7bit, 7bitFix */
	u16 shoot_ctrl_alpha_under_unlimit;	/* u7bit, 7bitFix */
} sharp_shoot_ctrl_cfg;

typedef struct sharp_band_core_H {
	s16 sharp_Reg_core_H0;	/* s5bit */
	s16 sharp_Reg_core_H1;	/* s7bit */
	s16 sharp_Reg_core_H2;	/* s9bit */
	s16 sharp_Reg_core_H3;	/* s10bit */
	s16 sharp_Reg_core_H4;	/* s11bit */
	s16 sharp_Reg_core_H5;	/* u10bit */
} sharp_band_core_H;

typedef struct sharp_band_core_V {
	s16 sharp_Reg_core_V0;	/* s4bit */
	s16 sharp_Reg_core_V1;	/* s4bit */
	s16 sharp_Reg_core_V2;	/* u4bit */
} sharp_band_core_V;

typedef struct ti_params {
	u16 slp1;		/* u9bit */
	u16 thr1;		/* u9bit */
	u16 radius;		/* u1bit */
	u16 noiseThrPos;	/* u10bitFix */
	u16 noiseThrNeg;	/* u10bitFix */
	u16 tiGain;		/* u5bit 4bitFix */
} sharp_ti_params;

typedef struct {
	u16 roi_xstart;	/* U12 */
	u16 roi_ystart;	/* U12 */
	u16 roi_xend;	/* U12 */
	u16 roi_yend;	/* U12 */
} sharp_roi_cfg;

typedef struct sharp_hw_params {
	u8 sharp_enable;	/* u1bit */

	/* Peaking Bands Filt Core */
	sharp_band_core_H sharp_H_core[4];	/* s11bit, 10bitFix */
	sharp_band_core_V sharp_V_core[3];	/* s4bit, 4bitFix */

	s16 sharp_Reg_USM_core[SHP_PADDING_RADIUS_V + 1];	/* s4bit, 4bitFix */
	u8 sharp_Reg_Diag_coef;					/* 3bit, 0bitFix */

	/* LCTI Process */
	u8 lti_enable;	/* u1bit */
	sharp_ti_params lti_h_cfg;
	sharp_ti_params lti_v_cfg;
	u8 cti_enable;	/* u1bit */
	sharp_ti_params cti_h_cfg;
	sharp_ti_params cti_v_cfg;

	/* Peaking Process */
	u8 peaking_enable;	/* u1bit */
	sharp_peaking_cfg peaking_cfg;

	/* ShootCtrl Process */
	u8 shoot_ctrl_enable;	/* u1bit */
	sharp_shoot_ctrl_cfg shoot_ctrl_cfg;

	/* GainCtrl */
	u8 gain_ctrl_enable;	/* u1bit */
	sharp_gain_ctrl_cfg gain_ctrl_cfg;

	/* ColorAdj */
	u8 color_adj_enable;	/* u1bit */
	sharp_color_adj_cfg color_adj_params[4];

	/* TextureAdj */
	u8 texture_adj_enable;	/* u1bit */
	sharp_texture_adj_cfg	texture_adj_cfg;

	/* ROI */
	u8 roi_en;		/* U1 */
	sharp_roi_cfg roi_cfg;	/* U12x4 */
} sharp_hw_params;

typedef union {
	struct {
		union {
			struct {
				unsigned int sw_sharp_enable: 1;
				unsigned int sw_lti_enable: 1;
				unsigned int sw_cti_enable: 1;
				unsigned int sw_peaking_enable: 1;
				unsigned int sw_peaking_ctrl_enable: 1;
				unsigned int sw_peaking_shoot_ctrl_enable: 1;
				unsigned int sw_edge_proc_enable: 1;
				unsigned int sw_shoot_ctrl_enable: 1;
				unsigned int sw_gain_ctrl_enable: 1;
				unsigned int sw_color_adj_enable: 1;
				unsigned int sw_texture_adj_enable: 1;
				unsigned int reserve_0: 1;
				unsigned int sw_ink_enable: 1;
				unsigned int reserve_1: 19;
			} bits;
			unsigned int u32;
		} sw_ctrl;
		union {
			struct {
				unsigned int reserve_0: 1;
				unsigned int sw_lti_gating_en: 1;
				unsigned int sw_cti_gating_en: 1;
				unsigned int sw_peaking_gating_en: 1;
				unsigned int sw_peaking_ctrl_gating_en: 1;
				unsigned int sw_peaking_shoot_ctrl_gating_en: 1;
				unsigned int sw_edge_proc_gating_en: 1;
				unsigned int sw_shoot_ctrl_gating_en: 1;
				unsigned int sw_gain_ctrl_gating_en: 1;
				unsigned int sw_color_adj_gating_en: 1;
				unsigned int sw_texture_adj_gating_en: 1;
				unsigned int reserve_1: 21;
			} bits;
			unsigned int u32;
		} sw_auto_gating_imd;
		union {
			struct {
				unsigned int sw_peaking_v00: 4;
				unsigned int sw_peaking_v01: 4;
				unsigned int sw_peaking_v02: 4;
				unsigned int sw_peaking_v10: 4;
				unsigned int sw_peaking_v11: 4;
				unsigned int sw_peaking_v12: 4;
				unsigned int reserve_0: 8;
			} bits;
			unsigned int u32;
		} sw_peaking_filter_coe0;
		union {
			struct {
				unsigned int sw_peaking_v20: 4;
				unsigned int sw_peaking_v21: 4;
				unsigned int sw_peaking_v22: 4;
				unsigned int sw_peaking_usm0: 4;
				unsigned int sw_peaking_usm1: 4;
				unsigned int sw_peaking_usm2: 4;
				unsigned int sw_diag_coef: 3;
				unsigned int reserve_0: 5;
			} bits;
			unsigned int u32;
		} sw_peaking_filter_coe1;
		union {
			struct {
				unsigned int sw_peaking_h00: 5;
				unsigned int reserve_0: 3;
				unsigned int sw_peaking_h01: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_peaking_h02: 9;
				unsigned int reserve_2: 7;
			} bits;
			unsigned int u32;
		} sw_peaking_filter_coe2;
		union {
			struct {
				unsigned int sw_peaking_h03: 10;
				unsigned int sw_peaking_h04: 11;
				unsigned int sw_peaking_h05: 10;
				unsigned int reserve_0: 1;
			} bits;
			unsigned int u32;
		} sw_peaking_filter_coe3;
		union {
			struct {
				unsigned int sw_peaking_h10: 5;
				unsigned int reserve_0: 3;
				unsigned int sw_peaking_h11: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_peaking_h12: 9;
				unsigned int reserve_2: 7;
			} bits;
			unsigned int u32;
		} sw_peaking_filter_coe4;
		union {
			struct {
				unsigned int sw_peaking_h13: 10;
				unsigned int sw_peaking_h14: 11;
				unsigned int sw_peaking_h15: 10;
				unsigned int reserve_0: 1;
			} bits;
			unsigned int u32;
		} sw_peaking_filter_coe5;
		union {
			struct {
				unsigned int sw_peaking_h20: 5;
				unsigned int reserve_0: 3;
				unsigned int sw_peaking_h21: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_peaking_h22: 9;
				unsigned int reserve_2: 7;
			} bits;
			unsigned int u32;
		} sw_peaking_filter_coe6;
		union {
			struct {
				unsigned int sw_peaking_h23: 10;
				unsigned int sw_peaking_h24: 11;
				unsigned int sw_peaking_h25: 10;
				unsigned int reserve_0: 1;
			} bits;
			unsigned int u32;
		} sw_peaking_filter_coe7;
		union {
			struct {
				unsigned int sw_peaking_h30: 5;
				unsigned int reserve_0: 3;
				unsigned int sw_peaking_h31: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_peaking_h32: 9;
				unsigned int reserve_2: 7;
			} bits;
			unsigned int u32;
		} sw_peaking_filter_coe8;
		union {
			struct {
				unsigned int sw_peaking_h33: 10;
				unsigned int sw_peaking_h34: 11;
				unsigned int sw_peaking_h35: 10;
				unsigned int reserve_0: 1;
			} bits;
			unsigned int u32;
		} sw_peaking_filter_coe9;
		union {
			struct {
				unsigned int sw_peaking0_idx_n0: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking0_idx_n1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking0_ctrl_coe0;
		union {
			struct {
				unsigned int sw_peaking0_idx_n2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking0_idx_n3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking0_ctrl_coe1;
		union {
			struct {
				unsigned int sw_peaking0_idx_p0: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking0_idx_p1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking0_ctrl_coe2;
		union {
			struct {
				unsigned int sw_peaking0_idx_p2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking0_idx_p3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking0_ctrl_coe3;
		union {
			struct {
				unsigned int sw_peaking0_value_n1: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking0_value_n2: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking0_ctrl_coe4;
		union {
			struct {
				unsigned int sw_peaking0_value_n3: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking0_value_p1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking0_ctrl_coe5;
		union {
			struct {
				unsigned int sw_peaking0_value_p2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking0_value_p3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking0_ctrl_coe6;
		union {
			struct {
				unsigned int sw_peaking0_ratio_n01: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_peaking0_ratio_n12: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_peaking0_ctrl_coe7;
		union {
			struct {
				unsigned int sw_peaking0_ratio_n23: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_peaking0_ratio_p01: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_peaking0_ctrl_coe8;
		union {
			struct {
				unsigned int sw_peaking0_ratio_p12: 12;
				unsigned int sw_peaking0_ratio_p23: 12;
				unsigned int sw_peaking0_shoot_delta_offset: 8;
			} bits;
			unsigned int u32;
		} sw_peaking0_ctrl_coe9;
		union {
			struct {
				unsigned int sw_peaking0_shoot_alpha_over: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_peaking0_shoot_alpha_under: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_peaking0_shoot_alpha_over_unlimit: 7;
				unsigned int reserve_2: 1;
				unsigned int sw_peaking0_shoot_alpha_under_unlimit: 7;
				unsigned int reserve_3: 1;
			} bits;
			unsigned int u32;
		} sw_peaking0_ctrl_coe10;
		union {
			struct {
				unsigned int sw_peaking1_idx_n0: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking1_idx_n1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking1_ctrl_coe0;
		union {
			struct {
				unsigned int sw_peaking1_idx_n2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking1_idx_n3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking1_ctrl_coe1;
		union {
			struct {
				unsigned int sw_peaking1_idx_p0: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking1_idx_p1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking1_ctrl_coe2;
		union {
			struct {
				unsigned int sw_peaking1_idx_p2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking1_idx_p3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking1_ctrl_coe3;
		union {
			struct {
				unsigned int sw_peaking1_value_n1: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking1_value_n2: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking1_ctrl_coe4;
		union {
			struct {
				unsigned int sw_peaking1_value_n3: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking1_value_p1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking1_ctrl_coe5;
		union {
			struct {
				unsigned int sw_peaking1_value_p2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking1_value_p3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking1_ctrl_coe6;
		union {
			struct {
				unsigned int sw_peaking1_ratio_n01: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_peaking1_ratio_n12: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_peaking1_ctrl_coe7;
		union {
			struct {
				unsigned int sw_peaking1_ratio_n23: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_peaking1_ratio_p01: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_peaking1_ctrl_coe8;
		union {
			struct {
				unsigned int sw_peaking1_ratio_p12: 12;
				unsigned int sw_peaking1_ratio_p23: 12;
				unsigned int sw_peaking1_shoot_delta_offset: 8;
			} bits;
			unsigned int u32;
		} sw_peaking1_ctrl_coe9;
		union {
			struct {
				unsigned int sw_peaking1_shoot_alpha_over: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_peaking1_shoot_alpha_under: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_peaking1_shoot_alpha_over_unlimit: 7;
				unsigned int reserve_2: 1;
				unsigned int sw_peaking1_shoot_alpha_under_unlimit: 7;
				unsigned int reserve_3: 1;
			} bits;
			unsigned int u32;
		} sw_peaking1_ctrl_coe10;
		union {
			struct {
				unsigned int sw_peaking2_idx_n0: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking2_idx_n1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking2_ctrl_coe0;
		union {
			struct {
				unsigned int sw_peaking2_idx_n2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking2_idx_n3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking2_ctrl_coe1;
		union {
			struct {
				unsigned int sw_peaking2_idx_p0: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking2_idx_p1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking2_ctrl_coe2;
		union {
			struct {
				unsigned int sw_peaking2_idx_p2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking2_idx_p3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking2_ctrl_coe3;
		union {
			struct {
				unsigned int sw_peaking2_value_n1: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking2_value_n2: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking2_ctrl_coe4;
		union {
			struct {
				unsigned int sw_peaking2_value_n3: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking2_value_p1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking2_ctrl_coe5;
		union {
			struct {
				unsigned int sw_peaking2_value_p2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking2_value_p3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking2_ctrl_coe6;
		union {
			struct {
				unsigned int sw_peaking2_ratio_n01: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_peaking2_ratio_n12: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_peaking2_ctrl_coe7;
		union {
			struct {
				unsigned int sw_peaking2_ratio_n23: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_peaking2_ratio_p01: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_peaking2_ctrl_coe8;
		union {
			struct {
				unsigned int sw_peaking2_ratio_p12: 12;
				unsigned int sw_peaking2_ratio_p23: 12;
				unsigned int sw_peaking2_shoot_delta_offset: 8;
			} bits;
			unsigned int u32;
		} sw_peaking2_ctrl_coe9;
		union {
			struct {
				unsigned int sw_peaking2_shoot_alpha_over: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_peaking2_shoot_alpha_under: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_peaking2_shoot_alpha_over_unlimit: 7;
				unsigned int reserve_2: 1;
				unsigned int sw_peaking2_shoot_alpha_under_unlimit: 7;
				unsigned int reserve_3: 1;
			} bits;
			unsigned int u32;
		} sw_peaking2_ctrl_coe10;
		union {
			struct {
				unsigned int sw_peaking3_idx_n0: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking3_idx_n1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking3_ctrl_coe0;
		union {
			struct {
				unsigned int sw_peaking3_idx_n2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking3_idx_n3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking3_ctrl_coe1;
		union {
			struct {
				unsigned int sw_peaking3_idx_p0: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking3_idx_p1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking3_ctrl_coe2;
		union {
			struct {
				unsigned int sw_peaking3_idx_p2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking3_idx_p3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking3_ctrl_coe3;
		union {
			struct {
				unsigned int sw_peaking3_value_n1: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking3_value_n2: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking3_ctrl_coe4;
		union {
			struct {
				unsigned int sw_peaking3_value_n3: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking3_value_p1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking3_ctrl_coe5;
		union {
			struct {
				unsigned int sw_peaking3_value_p2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking3_value_p3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking3_ctrl_coe6;
		union {
			struct {
				unsigned int sw_peaking3_ratio_n01: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_peaking3_ratio_n12: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_peaking3_ctrl_coe7;
		union {
			struct {
				unsigned int sw_peaking3_ratio_n23: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_peaking3_ratio_p01: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_peaking3_ctrl_coe8;
		union {
			struct {
				unsigned int sw_peaking3_ratio_p12: 12;
				unsigned int sw_peaking3_ratio_p23: 12;
				unsigned int sw_peaking3_shoot_delta_offset: 8;
			} bits;
			unsigned int u32;
		} sw_peaking3_ctrl_coe9;
		union {
			struct {
				unsigned int sw_peaking3_shoot_alpha_over: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_peaking3_shoot_alpha_under: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_peaking3_shoot_alpha_over_unlimit: 7;
				unsigned int reserve_2: 1;
				unsigned int sw_peaking3_shoot_alpha_under_unlimit: 7;
				unsigned int reserve_3: 1;
			} bits;
			unsigned int u32;
		} sw_peaking3_ctrl_coe10;
		union {
			struct {
				unsigned int sw_peaking4_idx_n0: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking4_idx_n1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking4_ctrl_coe0;
		union {
			struct {
				unsigned int sw_peaking4_idx_n2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking4_idx_n3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking4_ctrl_coe1;
		union {
			struct {
				unsigned int sw_peaking4_idx_p0: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking4_idx_p1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking4_ctrl_coe2;
		union {
			struct {
				unsigned int sw_peaking4_idx_p2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking4_idx_p3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking4_ctrl_coe3;
		union {
			struct {
				unsigned int sw_peaking4_value_n1: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking4_value_n2: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking4_ctrl_coe4;
		union {
			struct {
				unsigned int sw_peaking4_value_n3: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking4_value_p1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking4_ctrl_coe5;
		union {
			struct {
				unsigned int sw_peaking4_value_p2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking4_value_p3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking4_ctrl_coe6;
		union {
			struct {
				unsigned int sw_peaking4_ratio_n01: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_peaking4_ratio_n12: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_peaking4_ctrl_coe7;
		union {
			struct {
				unsigned int sw_peaking4_ratio_n23: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_peaking4_ratio_p01: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_peaking4_ctrl_coe8;
		union {
			struct {
				unsigned int sw_peaking4_ratio_p12: 12;
				unsigned int sw_peaking4_ratio_p23: 12;
				unsigned int sw_peaking4_shoot_delta_offset: 8;
			} bits;
			unsigned int u32;
		} sw_peaking4_ctrl_coe9;
		union {
			struct {
				unsigned int sw_peaking4_shoot_alpha_over: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_peaking4_shoot_alpha_under: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_peaking4_shoot_alpha_over_unlimit: 7;
				unsigned int reserve_2: 1;
				unsigned int sw_peaking4_shoot_alpha_under_unlimit: 7;
				unsigned int reserve_3: 1;
			} bits;
			unsigned int u32;
		} sw_peaking4_ctrl_coe10;
		union {
			struct {
				unsigned int sw_peaking5_idx_n0: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking5_idx_n1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking5_ctrl_coe0;
		union {
			struct {
				unsigned int sw_peaking5_idx_n2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking5_idx_n3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking5_ctrl_coe1;
		union {
			struct {
				unsigned int sw_peaking5_idx_p0: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking5_idx_p1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking5_ctrl_coe2;
		union {
			struct {
				unsigned int sw_peaking5_idx_p2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking5_idx_p3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking5_ctrl_coe3;
		union {
			struct {
				unsigned int sw_peaking5_value_n1: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking5_value_n2: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking5_ctrl_coe4;
		union {
			struct {
				unsigned int sw_peaking5_value_n3: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking5_value_p1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking5_ctrl_coe5;
		union {
			struct {
				unsigned int sw_peaking5_value_p2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking5_value_p3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking5_ctrl_coe6;
		union {
			struct {
				unsigned int sw_peaking5_ratio_n01: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_peaking5_ratio_n12: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_peaking5_ctrl_coe7;
		union {
			struct {
				unsigned int sw_peaking5_ratio_n23: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_peaking5_ratio_p01: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_peaking5_ctrl_coe8;
		union {
			struct {
				unsigned int sw_peaking5_ratio_p12: 12;
				unsigned int sw_peaking5_ratio_p23: 12;
				unsigned int sw_peaking5_shoot_delta_offset: 8;
			} bits;
			unsigned int u32;
		} sw_peaking5_ctrl_coe9;
		union {
			struct {
				unsigned int sw_peaking5_shoot_alpha_over: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_peaking5_shoot_alpha_under: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_peaking5_shoot_alpha_over_unlimit: 7;
				unsigned int reserve_2: 1;
				unsigned int sw_peaking5_shoot_alpha_under_unlimit: 7;
				unsigned int reserve_3: 1;
			} bits;
			unsigned int u32;
		} sw_peaking5_ctrl_coe10;
		union {
			struct {
				unsigned int sw_peaking6_idx_n0: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking6_idx_n1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking6_ctrl_coe0;
		union {
			struct {
				unsigned int sw_peaking6_idx_n2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking6_idx_n3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking6_ctrl_coe1;
		union {
			struct {
				unsigned int sw_peaking6_idx_p0: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking6_idx_p1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking6_ctrl_coe2;
		union {
			struct {
				unsigned int sw_peaking6_idx_p2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking6_idx_p3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking6_ctrl_coe3;
		union {
			struct {
				unsigned int sw_peaking6_value_n1: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking6_value_n2: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking6_ctrl_coe4;
		union {
			struct {
				unsigned int sw_peaking6_value_n3: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking6_value_p1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking6_ctrl_coe5;
		union {
			struct {
				unsigned int sw_peaking6_value_p2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking6_value_p3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking6_ctrl_coe6;
		union {
			struct {
				unsigned int sw_peaking6_ratio_n01: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_peaking6_ratio_n12: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_peaking6_ctrl_coe7;
		union {
			struct {
				unsigned int sw_peaking6_ratio_n23: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_peaking6_ratio_p01: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_peaking6_ctrl_coe8;
		union {
			struct {
				unsigned int sw_peaking6_ratio_p12: 12;
				unsigned int sw_peaking6_ratio_p23: 12;
				unsigned int sw_peaking6_shoot_delta_offset: 8;
			} bits;
			unsigned int u32;
		} sw_peaking6_ctrl_coe9;
		union {
			struct {
				unsigned int sw_peaking6_shoot_alpha_over: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_peaking6_shoot_alpha_under: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_peaking6_shoot_alpha_over_unlimit: 7;
				unsigned int reserve_2: 1;
				unsigned int sw_peaking6_shoot_alpha_under_unlimit: 7;
				unsigned int reserve_3: 1;
			} bits;
			unsigned int u32;
		} sw_peaking6_ctrl_coe10;
		union {
			struct {
				unsigned int sw_peaking7_idx_n0: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking7_idx_n1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking7_ctrl_coe0;
		union {
			struct {
				unsigned int sw_peaking7_idx_n2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking7_idx_n3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking7_ctrl_coe1;
		union {
			struct {
				unsigned int sw_peaking7_idx_p0: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking7_idx_p1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking7_ctrl_coe2;
		union {
			struct {
				unsigned int sw_peaking7_idx_p2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking7_idx_p3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking7_ctrl_coe3;
		union {
			struct {
				unsigned int sw_peaking7_value_n1: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking7_value_n2: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking7_ctrl_coe4;
		union {
			struct {
				unsigned int sw_peaking7_value_n3: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking7_value_p1: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking7_ctrl_coe5;
		union {
			struct {
				unsigned int sw_peaking7_value_p2: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_peaking7_value_p3: 11;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking7_ctrl_coe6;
		union {
			struct {
				unsigned int sw_peaking7_ratio_n01: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_peaking7_ratio_n12: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_peaking7_ctrl_coe7;
		union {
			struct {
				unsigned int sw_peaking7_ratio_n23: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_peaking7_ratio_p01: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_peaking7_ctrl_coe8;
		union {
			struct {
				unsigned int sw_peaking7_ratio_p12: 12;
				unsigned int sw_peaking7_ratio_p23: 12;
				unsigned int sw_peaking7_shoot_delta_offset: 8;
			} bits;
			unsigned int u32;
		} sw_peaking7_ctrl_coe9;
		union {
			struct {
				unsigned int sw_peaking7_shoot_alpha_over: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_peaking7_shoot_alpha_under: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_peaking7_shoot_alpha_over_unlimit: 7;
				unsigned int reserve_2: 1;
				unsigned int sw_peaking7_shoot_alpha_under_unlimit: 7;
				unsigned int reserve_3: 1;
			} bits;
			unsigned int u32;
		} sw_peaking7_ctrl_coe10;
		union {
			struct {
				unsigned int sw_peaking_gain: 10;
				unsigned int reserve_0: 2;
				unsigned int sw_nondir_thr: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_dir_cmp_ratio: 4;
				unsigned int sw_nondir_wgt_ratio: 5;
				unsigned int reserve_2: 3;
			} bits;
			unsigned int u32;
		} sw_peaking_ctrl0;
		union {
			struct {
				unsigned int sw_nondir_wgt_offset: 8;
				unsigned int sw_dir_cnt_thr: 4;
				unsigned int sw_dir_cnt_avg: 3;
				unsigned int reserve_0: 1;
				unsigned int sw_dir_cnt_offset: 4;
				unsigned int sw_diag_dir_thr: 7;
				unsigned int reserve_1: 5;
			} bits;
			unsigned int u32;
		} sw_peaking_ctrl1;
		union {
			struct {
				unsigned int sw_diag_adjgain_tab0: 4;
				unsigned int sw_diag_adjgain_tab1: 4;
				unsigned int sw_diag_adjgain_tab2: 4;
				unsigned int sw_diag_adjgain_tab3: 4;
				unsigned int sw_diag_adjgain_tab4: 4;
				unsigned int sw_diag_adjgain_tab5: 4;
				unsigned int sw_diag_adjgain_tab6: 4;
				unsigned int sw_diag_adjgain_tab7: 4;
			} bits;
			unsigned int u32;
		} sw_peaking_ctrl2;
		union {
			struct {
				unsigned int sw_edge_alpha_over_non: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_edge_alpha_under_non: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_edge_alpha_over_unlimit_non: 7;
				unsigned int reserve_2: 1;
				unsigned int sw_edge_alpha_under_unlimit_non: 7;
				unsigned int reserve_3: 1;
			} bits;
			unsigned int u32;
		} sw_peaking_ctrl3;
		union {
			struct {
				unsigned int sw_edge_alpha_over_v: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_edge_alpha_under_v: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_edge_alpha_over_unlimit_v: 7;
				unsigned int reserve_2: 1;
				unsigned int sw_edge_alpha_under_unlimit_v: 7;
				unsigned int reserve_3: 1;
			} bits;
			unsigned int u32;
		} sw_peaking_ctrl4;
		union {
			struct {
				unsigned int sw_edge_alpha_over_h: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_edge_alpha_under_h: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_edge_alpha_over_unlimit_h: 7;
				unsigned int reserve_2: 1;
				unsigned int sw_edge_alpha_under_unlimit_h: 7;
				unsigned int reserve_3: 1;
			} bits;
			unsigned int u32;
		} sw_peaking_ctrl5;
		union {
			struct {
				unsigned int sw_edge_alpha_over_d0: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_edge_alpha_under_d0: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_edge_alpha_over_unlimit_d0: 7;
				unsigned int reserve_2: 1;
				unsigned int sw_edge_alpha_under_unlimit_d0: 7;
				unsigned int reserve_3: 1;
			} bits;
			unsigned int u32;
		} sw_peaking_ctrl6;
		union {
			struct {
				unsigned int sw_edge_alpha_over_d1: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_edge_alpha_under_d1: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_edge_alpha_over_unlimit_d1: 7;
				unsigned int reserve_2: 1;
				unsigned int sw_edge_alpha_under_unlimit_d1: 7;
				unsigned int reserve_3: 1;
			} bits;
			unsigned int u32;
		} sw_peaking_ctrl7;
		union {
			struct {
				unsigned int sw_edge_delta_offset_non: 8;
				unsigned int sw_edge_delta_offset_v: 8;
				unsigned int sw_edge_delta_offset_h: 8;
				unsigned int reserve_0: 8;
			} bits;
			unsigned int u32;
		} sw_peaking_ctrl8;
		union {
			struct {
				unsigned int sw_edge_delta_offset_d0: 8;
				unsigned int sw_edge_delta_offset_d1: 8;
				unsigned int reserve_0: 16;
			} bits;
			unsigned int u32;
		} sw_peaking_ctrl9;
		union {
			struct {
				unsigned int sw_shoot_filt_radius: 1;
				unsigned int reserve_0: 3;
				unsigned int sw_shoot_delta_offset: 8;
				unsigned int sw_shoot_alpha_over: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_shoot_alpha_under: 7;
				unsigned int reserve_2: 5;
			} bits;
			unsigned int u32;
		} sw_shoot_ctrl0;
		union {
			struct {
				unsigned int sw_shoot_alpha_over_unlimit: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_shoot_alpha_under_unlimit: 7;
				unsigned int reserve_1: 17;
			} bits;
			unsigned int u32;
		} sw_shoot_ctrl1;
		union {
			struct {
				unsigned int sw_adp_idx0: 10;
				unsigned int sw_adp_idx1: 10;
				unsigned int sw_adp_idx2: 10;
				unsigned int reserve_0: 2;
			} bits;
			unsigned int u32;
		} sw_gain_ctrl0;
		union {
			struct {
				unsigned int sw_adp_idx3: 10;
				unsigned int reserve_0: 2;
				unsigned int sw_adp_gain0: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_adp_gain1: 7;
				unsigned int reserve_2: 5;
			} bits;
			unsigned int u32;
		} sw_gain_ctrl1;
		union {
			struct {
				unsigned int sw_adp_gain2: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_adp_gain3: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_adp_gain4: 7;
				unsigned int reserve_2: 9;
			} bits;
			unsigned int u32;
		} sw_gain_ctrl2;
		union {
			struct {
				unsigned int sw_adp_slp01: 11;
				unsigned int reserve_0: 1;
				unsigned int sw_adp_slp12: 11;
				unsigned int reserve_1: 9;
			} bits;
			unsigned int u32;
		} sw_gain_ctrl3;
		struct {
			unsigned int reserve_data[12];
		} reserve_reg_464_512;
		union {
			struct {
				unsigned int sw_adp_slp23: 11;
				unsigned int reserve_0: 1;
				unsigned int sw_adp_slp34: 11;
				unsigned int reserve_1: 9;
			} bits;
			unsigned int u32;
		} sw_gain_ctrl4;
		union {
			struct {
				unsigned int sw_adp_slp45: 11;
				unsigned int sw_var_idx0: 10;
				unsigned int sw_var_idx1: 10;
				unsigned int reserve_0: 1;
			} bits;
			unsigned int u32;
		} sw_gain_ctrl5;
		union {
			struct {
				unsigned int sw_var_idx2: 10;
				unsigned int reserve_0: 2;
				unsigned int sw_var_idx3: 10;
				unsigned int reserve_1: 2;
				unsigned int sw_var_gain0: 7;
				unsigned int reserve_2: 1;
			} bits;
			unsigned int u32;
		} sw_gain_ctrl6;
		union {
			struct {
				unsigned int sw_var_gain1: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_var_gain2: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_var_gain3: 7;
				unsigned int reserve_2: 1;
				unsigned int sw_var_gain4: 7;
				unsigned int reserve_3: 1;
			} bits;
			unsigned int u32;
		} sw_gain_ctrl7;
		union {
			struct {
				unsigned int sw_var_slp01: 11;
				unsigned int reserve_0: 1;
				unsigned int sw_var_slp12: 11;
				unsigned int reserve_1: 9;
			} bits;
			unsigned int u32;
		} sw_gain_ctrl8;
		union {
			struct {
				unsigned int sw_var_slp23: 11;
				unsigned int reserve_0: 1;
				unsigned int sw_var_slp34: 11;
				unsigned int reserve_1: 9;
			} bits;
			unsigned int u32;
		} sw_gain_ctrl9;
		union {
			struct {
				unsigned int sw_var_slp45: 11;
				unsigned int reserve_0: 5;
				unsigned int sw_lum_select: 2;
				unsigned int reserve_1: 2;
				unsigned int sw_lum_idx0: 10;
				unsigned int reserve_2: 2;
			} bits;
			unsigned int u32;
		} sw_gain_ctrl10;
		union {
			struct {
				unsigned int sw_lum_idx1: 10;
				unsigned int sw_lum_idx2: 10;
				unsigned int sw_lum_idx3: 10;
				unsigned int reserve_0: 2;
			} bits;
			unsigned int u32;
		} sw_gain_ctrl11;
		union {
			struct {
				unsigned int sw_lum_gain0: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_lum_gain1: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_lum_gain2: 7;
				unsigned int reserve_2: 1;
				unsigned int sw_lum_gain3: 7;
				unsigned int reserve_3: 1;
			} bits;
			unsigned int u32;
		} sw_gain_ctrl12;
		union {
			struct {
				unsigned int sw_lum_gain4: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_lum_slp01: 11;
				unsigned int reserve_1: 1;
				unsigned int sw_lum_slp12: 11;
				unsigned int reserve_2: 1;
			} bits;
			unsigned int u32;
		} sw_gain_ctrl13;
		union {
			struct {
				unsigned int sw_lum_slp23: 11;
				unsigned int reserve_0: 1;
				unsigned int sw_lum_slp34: 11;
				unsigned int reserve_1: 9;
			} bits;
			unsigned int u32;
		} sw_gain_ctrl14;
		union {
			struct {
				unsigned int sw_lum_slp45: 11;
				unsigned int reserve_0: 21;
			} bits;
			unsigned int u32;
		} sw_gain_ctrl15;
		union {
			struct {
				unsigned int sw_adj_point_x0: 10;
				unsigned int reserve_0: 2;
				unsigned int sw_adj_point_y0: 10;
				unsigned int reserve_1: 2;
				unsigned int sw_adj_scaling_coef0: 3;
				unsigned int reserve_2: 5;
			} bits;
			unsigned int u32;
		} sw_coloradj_ctrl0;
		union {
			struct {
				unsigned int sw_coloradj_tab0_0: 5;
				unsigned int sw_coloradj_tab0_1: 5;
				unsigned int sw_coloradj_tab0_2: 5;
				unsigned int sw_coloradj_tab0_3: 5;
				unsigned int sw_coloradj_tab0_4: 5;
				unsigned int sw_coloradj_tab0_5: 5;
				unsigned int reserve_0: 2;
			} bits;
			unsigned int u32;
		} sw_coloradj_ctrl1;
		union {
			struct {
				unsigned int sw_coloradj_tab0_6: 5;
				unsigned int sw_coloradj_tab0_7: 5;
				unsigned int sw_coloradj_tab0_8: 5;
				unsigned int sw_coloradj_tab0_9: 5;
				unsigned int sw_coloradj_tab0_10: 5;
				unsigned int sw_coloradj_tab0_11: 5;
				unsigned int reserve_0: 2;
			} bits;
			unsigned int u32;
		} sw_coloradj_ctrl2;
		union {
			struct {
				unsigned int sw_coloradj_tab0_12: 5;
				unsigned int sw_coloradj_tab0_13: 5;
				unsigned int sw_coloradj_tab0_14: 5;
				unsigned int sw_coloradj_tab0_15: 5;
				unsigned int reserve_0: 12;
			} bits;
			unsigned int u32;
		} sw_coloradj_ctrl3;
		union {
			struct {
				unsigned int sw_adj_point_x1: 10;
				unsigned int reserve_0: 2;
				unsigned int sw_adj_point_y1: 10;
				unsigned int reserve_1: 2;
				unsigned int sw_adj_scaling_coef1: 3;
				unsigned int reserve_2: 5;
			} bits;
			unsigned int u32;
		} sw_coloradj_ctrl4;
		union {
			struct {
				unsigned int sw_coloradj_tab1_0: 5;
				unsigned int sw_coloradj_tab1_1: 5;
				unsigned int sw_coloradj_tab1_2: 5;
				unsigned int sw_coloradj_tab1_3: 5;
				unsigned int sw_coloradj_tab1_4: 5;
				unsigned int sw_coloradj_tab1_5: 5;
				unsigned int reserve_0: 2;
			} bits;
			unsigned int u32;
		} sw_coloradj_ctrl5;
		union {
			struct {
				unsigned int sw_coloradj_tab1_6: 5;
				unsigned int sw_coloradj_tab1_7: 5;
				unsigned int sw_coloradj_tab1_8: 5;
				unsigned int sw_coloradj_tab1_9: 5;
				unsigned int sw_coloradj_tab1_10: 5;
				unsigned int sw_coloradj_tab1_11: 5;
				unsigned int reserve_0: 2;
			} bits;
			unsigned int u32;
		} sw_coloradj_ctrl6;
		union {
			struct {
				unsigned int sw_coloradj_tab1_12: 5;
				unsigned int sw_coloradj_tab1_13: 5;
				unsigned int sw_coloradj_tab1_14: 5;
				unsigned int sw_coloradj_tab1_15: 5;
				unsigned int reserve_0: 12;
			} bits;
			unsigned int u32;
		} sw_coloradj_ctrl7;
		union {
			struct {
				unsigned int sw_adj_point_x2: 10;
				unsigned int reserve_0: 2;
				unsigned int sw_adj_point_y2: 10;
				unsigned int reserve_1: 2;
				unsigned int sw_adj_scaling_coef2: 3;
				unsigned int reserve_2: 5;
			} bits;
			unsigned int u32;
		} sw_coloradj_ctrl8;
		union {
			struct {
				unsigned int sw_coloradj_tab2_0: 5;
				unsigned int sw_coloradj_tab2_1: 5;
				unsigned int sw_coloradj_tab2_2: 5;
				unsigned int sw_coloradj_tab2_3: 5;
				unsigned int sw_coloradj_tab2_4: 5;
				unsigned int sw_coloradj_tab2_5: 5;
				unsigned int reserve_0: 2;
			} bits;
			unsigned int u32;
		} sw_coloradj_ctrl9;
		union {
			struct {
				unsigned int sw_coloradj_tab2_6: 5;
				unsigned int sw_coloradj_tab2_7: 5;
				unsigned int sw_coloradj_tab2_8: 5;
				unsigned int sw_coloradj_tab2_9: 5;
				unsigned int sw_coloradj_tab2_10: 5;
				unsigned int sw_coloradj_tab2_11: 5;
				unsigned int reserve_0: 2;
			} bits;
			unsigned int u32;
		} sw_coloradj_ctrl10;
		union {
			struct {
				unsigned int sw_coloradj_tab2_12: 5;
				unsigned int sw_coloradj_tab2_13: 5;
				unsigned int sw_coloradj_tab2_14: 5;
				unsigned int sw_coloradj_tab2_15: 5;
				unsigned int reserve_0: 12;
			} bits;
			unsigned int u32;
		} sw_coloradj_ctrl11;
		union {
			struct {
				unsigned int sw_adj_point_x3: 10;
				unsigned int reserve_0: 2;
				unsigned int sw_adj_point_y3: 10;
				unsigned int reserve_1: 2;
				unsigned int sw_adj_scaling_coef3: 3;
				unsigned int reserve_2: 5;
			} bits;
			unsigned int u32;
		} sw_coloradj_ctrl12;
		union {
			struct {
				unsigned int sw_coloradj_tab3_0: 5;
				unsigned int sw_coloradj_tab3_1: 5;
				unsigned int sw_coloradj_tab3_2: 5;
				unsigned int sw_coloradj_tab3_3: 5;
				unsigned int sw_coloradj_tab3_4: 5;
				unsigned int sw_coloradj_tab3_5: 5;
				unsigned int reserve_0: 2;
			} bits;
			unsigned int u32;
		} sw_coloradj_ctrl13;
		union {
			struct {
				unsigned int sw_coloradj_tab3_6: 5;
				unsigned int sw_coloradj_tab3_7: 5;
				unsigned int sw_coloradj_tab3_8: 5;
				unsigned int sw_coloradj_tab3_9: 5;
				unsigned int sw_coloradj_tab3_10: 5;
				unsigned int sw_coloradj_tab3_11: 5;
				unsigned int reserve_0: 2;
			} bits;
			unsigned int u32;
		} sw_coloradj_ctrl14;
		union {
			struct {
				unsigned int sw_coloradj_tab3_12: 5;
				unsigned int sw_coloradj_tab3_13: 5;
				unsigned int sw_coloradj_tab3_14: 5;
				unsigned int sw_coloradj_tab3_15: 5;
				unsigned int reserve_0: 12;
			} bits;
			unsigned int u32;
		} sw_coloradj_ctrl15;
		union {
			struct {
				unsigned int sw_idxmode_select: 1;
				unsigned int sw_ymode_select: 2;
				unsigned int reserve_0: 1;
				unsigned int sw_tex_idx0: 10;
				unsigned int reserve_1: 2;
				unsigned int sw_tex_idx1: 10;
				unsigned int reserve_2: 6;
			} bits;
			unsigned int u32;
		} sw_texture_ctrl0;
		union {
			struct {
				unsigned int sw_tex_idx2: 10;
				unsigned int reserve_0: 2;
				unsigned int sw_tex_idx3: 10;
				unsigned int reserve_1: 2;
				unsigned int sw_tex_gain0: 7;
				unsigned int reserve_2: 1;
			} bits;
			unsigned int u32;
		} sw_texture_ctrl1;
		union {
			struct {
				unsigned int sw_tex_gain1: 7;
				unsigned int reserve_0: 1;
				unsigned int sw_tex_gain2: 7;
				unsigned int reserve_1: 1;
				unsigned int sw_tex_gain3: 7;
				unsigned int reserve_2: 1;
				unsigned int sw_tex_gain4: 7;
				unsigned int reserve_3: 1;
			} bits;
			unsigned int u32;
		} sw_texture_ctrl2;
		union {
			struct {
				unsigned int sw_tex_slp01: 11;
				unsigned int reserve_0: 1;
				unsigned int sw_tex_slp12: 11;
				unsigned int reserve_1: 9;
			} bits;
			unsigned int u32;
		} sw_texture_ctrl3;
		union {
			struct {
				unsigned int sw_tex_slp23: 11;
				unsigned int reserve_0: 1;
				unsigned int sw_tex_slp34: 11;
				unsigned int reserve_1: 9;
			} bits;
			unsigned int u32;
		} sw_texture_ctrl4;
		union {
			struct {
				unsigned int sw_tex_slp45: 11;
				unsigned int reserve_0: 21;
			} bits;
			unsigned int u32;
		} sw_texture_ctrl5;
		union {
			struct {
				unsigned int sw_ltih_radius: 1;
				unsigned int reserve_0: 3;
				unsigned int sw_ltih_slp1: 9;
				unsigned int reserve_1: 3;
				unsigned int sw_ltih_thr1: 9;
				unsigned int reserve_2: 7;
			} bits;
			unsigned int u32;
		} sw_lti_ctrl0;
		union {
			struct {
				unsigned int sw_ltih_noisethrneg: 10;
				unsigned int reserve_0: 2;
				unsigned int sw_ltih_noisethrpos: 10;
				unsigned int reserve_1: 2;
				unsigned int sw_ltih_tigain: 5;
				unsigned int reserve_2: 3;
			} bits;
			unsigned int u32;
		} sw_lti_ctrl1;
		union {
			struct {
				unsigned int sw_ltiv_radius: 1;
				unsigned int reserve_0: 3;
				unsigned int sw_ltiv_slp1: 9;
				unsigned int reserve_1: 3;
				unsigned int sw_ltiv_thr1: 9;
				unsigned int reserve_2: 7;
			} bits;
			unsigned int u32;
		} sw_lti_ctrl2;
		union {
			struct {
				unsigned int sw_ltiv_noisethrneg: 10;
				unsigned int reserve_0: 2;
				unsigned int sw_ltiv_noisethrpos: 10;
				unsigned int reserve_1: 2;
				unsigned int sw_ltiv_tigain: 5;
				unsigned int reserve_2: 3;
			} bits;
			unsigned int u32;
		} sw_lti_ctrl3;
		union {
			struct {
				unsigned int sw_ctih_radius: 1;
				unsigned int reserve_0: 3;
				unsigned int sw_ctih_slp1: 9;
				unsigned int reserve_1: 3;
				unsigned int sw_ctih_thr1: 9;
				unsigned int reserve_2: 7;
			} bits;
			unsigned int u32;
		} sw_cti_ctrl0;
		union {
			struct {
				unsigned int sw_ctih_noisethrneg: 10;
				unsigned int reserve_0: 2;
				unsigned int sw_ctih_noisethrpos: 10;
				unsigned int reserve_1: 2;
				unsigned int sw_ctih_tigain: 5;
				unsigned int reserve_2: 3;
			} bits;
			unsigned int u32;
		} sw_cti_ctrl1;
		union {
			struct {
				unsigned int sw_ctiv_radius: 1;
				unsigned int reserve_0: 3;
				unsigned int sw_ctiv_slp1: 9;
				unsigned int reserve_1: 3;
				unsigned int sw_ctiv_thr1: 9;
				unsigned int reserve_2: 7;
			} bits;
			unsigned int u32;
		} sw_cti_ctrl2;
		union {
			struct {
				unsigned int sw_ctiv_noisethrneg: 10;
				unsigned int reserve_0: 2;
				unsigned int sw_ctiv_noisethrpos: 10;
				unsigned int reserve_1: 2;
				unsigned int sw_ctiv_tigain: 5;
				unsigned int reserve_2: 3;
			} bits;
			unsigned int u32;
		} sw_cti_ctrl3;
		union {
			struct {
				unsigned int sw_debug_mode: 4;
				unsigned int reserve_0: 28;
			} bits;
			unsigned int u32;
		} sw_dbg_ctrl0;
		union {
			struct {
				unsigned int sw_roi_xstart: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_roi_ystart: 12;
				unsigned int reserve_1: 3;
				unsigned int sw_roi_en: 1;
			} bits;
			unsigned int u32;
		} sw_roi_ctrl0;
		union {
			struct {
				unsigned int sw_roi_xend: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_roi_yend: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_roi_ctrl1;
	} regs;
	unsigned int p_reg_addr[SHARP_FULL_REGS_LENGTH];
} sharp_full_regs;

typedef union {
	struct {
		union {
			struct {
				unsigned int sharp_en: 1;
				unsigned int shoot_ctrl_en: 1;
				unsigned int reserve_0: 30;
			} bits;
			unsigned int u32;
		} sw_enable_ctrl;
		union {
			struct {
				unsigned int shoot_ctrl_gating_en: 1;
				unsigned int reserve_0: 31;
			} bits;
			unsigned int u32;
		} sw_gating_ctrl;
		struct {
			unsigned int reserve_data[2];
		} reserve_reg_8_16;
		union {
			struct {
				unsigned int peaking_gain: 10;
				unsigned int reserve_0: 6;
				unsigned int coring_thr: 7;
				unsigned int reserve_1: 9;
			} bits;
			unsigned int u32;
		} sw_usm_ctrl;
		union {
			struct {
				unsigned int coef_A: 8;
				unsigned int coef_B: 8;
				unsigned int coef_C: 8;
				unsigned int reserve_0: 8;
			} bits;
			unsigned int u32;
		} sw_usm_coef;
		struct {
			unsigned int reserve_data[2];
		} reserve_reg_24_32;
		union {
			struct {
				unsigned int delta_offset: 8;
				unsigned int reserve_0: 24;
			} bits;
			unsigned int u32;
		} sw_shoot_ctrl_reg0;
		union {
			struct {
				unsigned int alpha_pos: 7;
				unsigned int reserve_0: 9;
				unsigned int alpha_pos_unlimit: 7;
				unsigned int reserve_1: 9;
			} bits;
			unsigned int u32;
		} sw_shoot_ctrl_reg1;
		union {
			struct {
				unsigned int alpha_neg: 7;
				unsigned int reserve_0: 9;
				unsigned int alpha_neg_unlimit: 7;
				unsigned int reserve_1: 9;
			} bits;
			unsigned int u32;
		} sw_shoot_ctrl_reg2;
		union {
			struct {
				unsigned int sw_roi_xstart: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_roi_ystart: 12;
				unsigned int reserve_1: 3;
				unsigned int sw_roi_en: 1;
			} bits;
			unsigned int u32;
		} sw_roi_ctrl0;
		union {
			struct {
				unsigned int sw_roi_xend: 12;
				unsigned int reserve_0: 4;
				unsigned int sw_roi_yend: 12;
				unsigned int reserve_1: 4;
			} bits;
			unsigned int u32;
		} sw_roi_ctrl1;
		union {
			struct {
				unsigned int sw_ink_mode: 4;
				unsigned int reserve_0: 27;
				unsigned int sw_ink_enable: 1;
			} bits;
			unsigned int u32;
		} sw_ink_ctrl0;
	} regs;
	unsigned int p_reg_addr[SHARP_LITE_REGS_LENGTH];
} sharp_lite_regs;

void rockchip_hwpq_sharp_cfg_init(rk_hwpq_shp_cfg *p_shp_cfg);
void rockchip_sharp_hw_params_init(const rk_hwpq_shp_cfg *p_shp_hal_cfg,
				   sharp_hw_params *p_sharp_reg);
void rockchip_sharp_full_regs_generate(const sharp_hw_params *p_sharp_reg,
				       sharp_full_regs *p_kernel_cfg);

#endif /* _ROCKCHIP_SHARP_REGS_H_ */
