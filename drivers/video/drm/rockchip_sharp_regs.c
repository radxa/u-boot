/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#include <linux/types.h>
#include <string.h>

#include "rockchip_sharp_regs.h"

#define MAX(a, b)		((a) > (b) ? (a) : (b))
#define MIN(a, b)		((a) < (b) ? (a) : (b))
#define CLIP(v, lo, hi)		((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

static s16 shp_round_div(int num, int den)
{
	if (num >= 0)
		return (2 * num + den) / (2 * den);

	return (2 * num - den) / (2 * den);
}

void rockchip_hwpq_sharp_cfg_init(rk_hwpq_shp_cfg *p_shp_cfg)
{
	int i;
	memset(p_shp_cfg, 0, sizeof(*p_shp_cfg));

	p_shp_cfg->sharp_enable = 1;

	/* ---- LTI/CTI (h/v), all disabled by default ---- */
	struct rkvop_cfg_lti_struct lti_def;
	memset(&lti_def, 0, sizeof(lti_def));
	lti_def.lti_enable = 0;
	lti_def.lti_radius = 1;
	lti_def.lti_slope = 100;
	lti_def.lti_thresold = 21;
	lti_def.lti_gain = 8;
	lti_def.lti_noise_thr_pos = 1023;
	lti_def.lti_noise_thr_neg = 1023;
	p_shp_cfg->sharp_lti_h_cfg = lti_def;
	p_shp_cfg->sharp_lti_v_cfg = lti_def;
	p_shp_cfg->sharp_cti_h_cfg = lti_def;
	p_shp_cfg->sharp_cti_v_cfg = lti_def;

	/* ---- peaking ---- */
	struct rkvop_cfg_peaking_struct *pk = &p_shp_cfg->sharp_peaking_cfg;
	pk->peaking_enable = 1;
	pk->peaking_gain = 300;
	pk->gain_enable = 1;
	pk->coring_enable = 1;
	pk->limit_ctrl_enable = 1;
	pk->shoot_adj_enable = 0;

	const u16 coring_zero_tmp[8] = {5, 5, 8, 5, 8, 5, 5, 7};
	const u16 coring_thr_tmp[8] = {40, 40, 40, 24, 26, 30, 26, 26};
	const u16 coring_ratio_tmp[8] = {1479, 1188, 1024, 1422, 1024, 1024, 1024, 1024};
	const u16 gain_pos_tmp[8] = {0, 512, 1024, 512, 1024, 0, 0, 0};
	const u16 gain_neg_tmp[8] = {0, 512, 1024, 512, 1024, 0, 0, 0};
	const u16 limit_pos0_tmp[8] = {64, 64, 64, 64, 64, 64, 64, 64};
	const u16 limit_pos1_tmp[8] = {120, 120, 120, 120, 120, 120, 120, 120};
	const u16 limit_neg0_tmp[8] = {64, 64, 64, 64, 64, 64, 64, 64};
	const u16 limit_neg1_tmp[8] = {120, 120, 120, 120, 120, 120, 120, 120};
	const u16 limit_ratio_tmp[8] = {128, 128, 128, 128, 128, 128, 128, 128};
	const u16 limit_bnd_pos_tmp[8] = {81, 131, 63, 81, 63, 63, 63, 63};
	const u16 limit_bnd_neg_tmp[8] = {81, 131, 63, 81, 63, 63, 63, 63};
	const u8 sht_delta_offset_tmp[8] = {32, 32, 32, 32, 32, 32, 32, 32};
	const u8 sht_alpha_over_tmp[8] = {8, 8, 8, 8, 8, 8, 8, 8};
	const u8 sht_alpha_under_tmp[8] = {64, 64, 64, 64, 64, 64, 64, 64};
	const u8 sht_alpha_over_unlimit_tmp[8] = {96, 96, 96, 96, 96, 96, 96, 96};
	const u8 sht_alpha_under_unlimit_tmp[8] = {96, 96, 96, 96, 96, 96, 96, 96};
	for (i = 0; i < 8; i++) {
		pk->coring_zero[i] = coring_zero_tmp[i];
		pk->coring_thr[i] = coring_thr_tmp[i];
		pk->coring_ratio[i] = coring_ratio_tmp[i];
		pk->gain_pos[i] = gain_pos_tmp[i];
		pk->gain_neg[i] = gain_neg_tmp[i];
		pk->limit_ctrl_pos0[i] = limit_pos0_tmp[i];
		pk->limit_ctrl_pos1[i] = limit_pos1_tmp[i];
		pk->limit_ctrl_neg0[i] = limit_neg0_tmp[i];
		pk->limit_ctrl_neg1[i] = limit_neg1_tmp[i];
		pk->limit_ctrl_bnd_pos[i] = limit_bnd_pos_tmp[i];
		pk->limit_ctrl_bnd_neg[i] = limit_bnd_neg_tmp[i];
		pk->limit_ctrl_ratio[i] = limit_ratio_tmp[i];
		pk->shoot_adj_delta_offset[i] = sht_delta_offset_tmp[i];
		pk->shoot_adj_alpha_over[i] = sht_alpha_over_tmp[i];
		pk->shoot_adj_alpha_under[i] = sht_alpha_under_tmp[i];
		pk->shoot_adj_alpha_over_unlimit[i] = sht_alpha_over_unlimit_tmp[i];
		pk->shoot_adj_alpha_under_unlimit[i] = sht_alpha_under_unlimit_tmp[i];
	}

	/* filter cores */
	const s16 filt_core_h0[6] = {1, 10, 45, 120, 210, 252};
	const s16 filt_core_h1[6] = {-9, -55, -119, -73, 128, 256};
	const s16 filt_core_h2[6] = {0, 0, 0, -256, 0, 512};
	const s16 filt_core_h3[6] = {0, 0, 0, 0, -256, 512};
	const s16 filt_core_v0[3] = {1, 4, 6};
	const s16 filt_core_v1[3] = {-4, 0, 8};
	const s16 filt_core_v2[3] = {0, -4, 8};
	const s16 filt_core_usm[3] = {1, 4, 6};
	pk->filter_cfg_diag_enh_coef = 6;
	for (i = 0; i < 6; i++) {
		pk->filter_cfg_filt_core_H0[i] = filt_core_h0[i];
		pk->filter_cfg_filt_core_H1[i] = filt_core_h1[i];
		pk->filter_cfg_filt_core_H2[i] = filt_core_h2[i];
		pk->filter_cfg_filt_core_H3[i] = filt_core_h3[i];
	}
	for (i = 0; i < 3; i++) {
		pk->filter_cfg_filt_core_V0[i] = filt_core_v0[i];
		pk->filter_cfg_filt_core_V1[i] = filt_core_v1[i];
		pk->filter_cfg_filt_core_V2[i] = filt_core_v2[i];
		pk->filter_cfg_filt_core_USM[i] = filt_core_usm[i];
	}

	/* edge process (disabled by default) */
	pk->edge_ctrl_enable = 0;
	pk->edge_ctrl_non_dir_thr = 64;
	pk->edge_ctrl_dir_cmp_ratio = 4;
	pk->edge_ctrl_non_dir_wgt_offset = 64;
	pk->edge_ctrl_non_dir_wgt_ratio = 16;
	pk->edge_ctrl_dir_cnt_thr = 2;
	pk->edge_ctrl_dir_cnt_avg = 3;
	pk->edge_ctrl_dir_cnt_offset = 2;
	pk->edge_ctrl_diag_dir_thr = 64;
	const u8 diag_adj_gain_tab_tmp[8] = {6, 7, 8, 9, 10, 11, 12, 13};
	for (i = 0; i < 8; i++)
		pk->edge_ctrl_diag_adj_gain_tab[i] = diag_adj_gain_tab_tmp[i];

	pk->edge_shoot_ctrl_enable = 0;
	pk->edge_shoot_ctrl_delta_offset_h = 4;
	pk->edge_shoot_ctrl_alpha_over_h = 8;
	pk->edge_shoot_ctrl_alpha_under_h = 16;
	pk->edge_shoot_ctrl_alpha_over_unlimit_h = 64;
	pk->edge_shoot_ctrl_alpha_under_unlimit_h = 112;
	pk->edge_shoot_ctrl_delta_offset_v = 4;
	pk->edge_shoot_ctrl_alpha_over_v = 8;
	pk->edge_shoot_ctrl_alpha_under_v = 16;
	pk->edge_shoot_ctrl_alpha_over_unlimit_v = 64;
	pk->edge_shoot_ctrl_alpha_under_unlimit_v = 112;
	pk->edge_shoot_ctrl_delta_offset_d0 = 4;
	pk->edge_shoot_ctrl_alpha_over_d0 = 16;
	pk->edge_shoot_ctrl_alpha_under_d0 = 16;
	pk->edge_shoot_ctrl_alpha_over_unlimit_d0 = 96;
	pk->edge_shoot_ctrl_alpha_under_unlimit_d0 = 96;
	pk->edge_shoot_ctrl_delta_offset_d1 = 4;
	pk->edge_shoot_ctrl_alpha_over_d1 = 16;
	pk->edge_shoot_ctrl_alpha_under_d1 = 16;
	pk->edge_shoot_ctrl_alpha_over_unlimit_d1 = 96;
	pk->edge_shoot_ctrl_alpha_under_unlimit_d1 = 96;
	pk->edge_shoot_ctrl_delta_offset_non = 4;
	pk->edge_shoot_ctrl_alpha_over_non = 8;
	pk->edge_shoot_ctrl_alpha_under_non = 8;
	pk->edge_shoot_ctrl_alpha_over_unlimit_non = 112;
	pk->edge_shoot_ctrl_alpha_under_unlimit_non = 112;

	/* ---- shootctrl ---- */
	struct rkvop_cfg_shootctrl_struct *sc = &p_shp_cfg->sharp_shootctrl_cfg;
	sc->shootctrl_enable = 1;
	sc->filter_radius = 1;
	sc->delta_offset = 0;
	sc->alpha_over = 32;
	sc->alpha_under = 32;
	sc->alpha_over_unlimit = 32;
	sc->alpha_under_unlimit = 32;

	/* ---- global gain (disabled) ---- */
	struct rkvop_cfg_global_gain_struct *gg = &p_shp_cfg->sharp_global_gain_cfg;
	gg->global_gain_enable = 0;
	gg->lum_mode = 0;
	const u16 lum_grd_tmp[6] = {0, 200, 300, 860, 960, 1023};
	const u8 lum_val_tmp[6] = {64, 64, 64, 64, 64, 64};
	const u16 adp_grd_tmp[6] = {0, 4, 60, 180, 300, 1023};
	const u8 adp_val_tmp[6] = {64, 64, 64, 64, 64, 64};
	const u16 var_grd_tmp[6] = {0, 39, 102, 209, 500, 1023};
	const u8 var_val_tmp[6] = {64, 64, 64, 64, 64, 64};
	for (i = 0; i < 6; i++) {
		gg->lum_grd[i] = lum_grd_tmp[i];
		gg->lum_val[i] = lum_val_tmp[i];
		gg->adp_grd[i] = adp_grd_tmp[i];
		gg->adp_val[i] = adp_val_tmp[i];
		gg->var_grd[i] = var_grd_tmp[i];
		gg->var_val[i] = var_val_tmp[i];
	}

	/* ---- color adj (disabled) ---- */
	struct rkvop_cfg_color_ctrl_struct *ca = &p_shp_cfg->sharp_color_adj_cfg;
	ca->color_ctrl_enable = 0;
	ca->p0_scaling_coef = 2;
	ca->p1_scaling_coef = 2;
	ca->p2_scaling_coef = 2;
	ca->p3_scaling_coef = 2;
	ca->p0_ctrl_point_u = 468;
	ca->p1_ctrl_point_u = 596;
	ca->p2_ctrl_point_u = 120;
	ca->p3_ctrl_point_u = 820;
	ca->p0_ctrl_point_v = 584;
	ca->p1_ctrl_point_v = 480;
	ca->p2_ctrl_point_v = 800;
	ca->p3_ctrl_point_v = 110;
	for (i = 0; i < 16; i++) {
		/* @note: set to all 16, ref by wjy 20260817 */
		ca->p0_ctrl_roll_tab[i] = 16;
		ca->p1_ctrl_roll_tab[i] = 16;
		ca->p2_ctrl_roll_tab[i] = 16;
		ca->p3_ctrl_roll_tab[i] = 16;
	}

	/* ---- texture adj (disabled) ---- */
	struct rkvop_cfg_texture_adj_struct *ta = &p_shp_cfg->sharp_texture_adj_cfg;
	ta->textureAdj_enable = 0;
	ta->y_mode_select = 1;
	ta->idx_mode_select = 1;
	const u16 tex_grd_tmp[6] = {0, 128, 256, 400, 600, 1023};
	const u8 tex_val_tmp[6] = {40, 60, 80, 100, 127, 127};
	for (i = 0; i < 6; i++) {
		ta->texture_grd[i] = tex_grd_tmp[i];
		ta->texture_val[i] = tex_val_tmp[i];
	}
}

void rockchip_sharp_hw_params_init(const rk_hwpq_shp_cfg *p_shp_hal_cfg,
				   sharp_hw_params *p_sharp_reg)
{
	int ii;
	p_sharp_reg->sharp_enable = p_shp_hal_cfg->sharp_enable;
	p_sharp_reg->lti_enable = (p_shp_hal_cfg->sharp_lti_h_cfg.lti_enable) ||
		(p_shp_hal_cfg->sharp_lti_v_cfg.lti_enable);

	p_sharp_reg->lti_h_cfg.radius = p_shp_hal_cfg->sharp_lti_h_cfg.lti_radius;
	p_sharp_reg->lti_h_cfg.noiseThrNeg = p_shp_hal_cfg->sharp_lti_h_cfg.lti_noise_thr_neg;
	p_sharp_reg->lti_h_cfg.noiseThrPos = p_shp_hal_cfg->sharp_lti_h_cfg.lti_noise_thr_pos;
	p_sharp_reg->lti_h_cfg.slp1 = p_shp_hal_cfg->sharp_lti_h_cfg.lti_slope;
	p_sharp_reg->lti_h_cfg.thr1 = p_shp_hal_cfg->sharp_lti_h_cfg.lti_thresold;
	p_sharp_reg->lti_h_cfg.tiGain = p_shp_hal_cfg->sharp_lti_h_cfg.lti_gain;

	p_sharp_reg->lti_v_cfg.radius = p_shp_hal_cfg->sharp_lti_v_cfg.lti_radius;
	p_sharp_reg->lti_v_cfg.noiseThrNeg = p_shp_hal_cfg->sharp_lti_v_cfg.lti_noise_thr_neg;
	p_sharp_reg->lti_v_cfg.noiseThrPos = p_shp_hal_cfg->sharp_lti_v_cfg.lti_noise_thr_pos;
	p_sharp_reg->lti_v_cfg.slp1 = p_shp_hal_cfg->sharp_lti_v_cfg.lti_slope;
	p_sharp_reg->lti_v_cfg.thr1 = p_shp_hal_cfg->sharp_lti_v_cfg.lti_thresold;
	p_sharp_reg->lti_v_cfg.tiGain = p_shp_hal_cfg->sharp_lti_v_cfg.lti_gain;

	if ((p_sharp_reg->lti_enable == 1) && (p_shp_hal_cfg->sharp_lti_h_cfg.lti_enable == 0))
		p_sharp_reg->lti_h_cfg.tiGain = 0;

	if ((p_sharp_reg->lti_enable == 1) && (p_shp_hal_cfg->sharp_lti_v_cfg.lti_enable == 0))
		p_sharp_reg->lti_v_cfg.tiGain = 0;

	p_sharp_reg->cti_enable = (p_shp_hal_cfg->sharp_cti_h_cfg.lti_enable) ||
		(p_shp_hal_cfg->sharp_cti_v_cfg.lti_enable);

	p_sharp_reg->cti_h_cfg.radius = p_shp_hal_cfg->sharp_cti_h_cfg.lti_radius;
	p_sharp_reg->cti_h_cfg.noiseThrNeg = p_shp_hal_cfg->sharp_cti_h_cfg.lti_noise_thr_neg;
	p_sharp_reg->cti_h_cfg.noiseThrPos = p_shp_hal_cfg->sharp_cti_h_cfg.lti_noise_thr_pos;
	p_sharp_reg->cti_h_cfg.slp1 = p_shp_hal_cfg->sharp_cti_h_cfg.lti_slope;
	p_sharp_reg->cti_h_cfg.thr1 = p_shp_hal_cfg->sharp_cti_h_cfg.lti_thresold;
	p_sharp_reg->cti_h_cfg.tiGain = p_shp_hal_cfg->sharp_cti_h_cfg.lti_gain;

	p_sharp_reg->cti_v_cfg.radius = p_shp_hal_cfg->sharp_cti_v_cfg.lti_radius;
	p_sharp_reg->cti_v_cfg.noiseThrNeg = p_shp_hal_cfg->sharp_cti_v_cfg.lti_noise_thr_neg;
	p_sharp_reg->cti_v_cfg.noiseThrPos = p_shp_hal_cfg->sharp_cti_v_cfg.lti_noise_thr_pos;
	p_sharp_reg->cti_v_cfg.slp1 = p_shp_hal_cfg->sharp_cti_v_cfg.lti_slope;
	p_sharp_reg->cti_v_cfg.thr1 = p_shp_hal_cfg->sharp_cti_v_cfg.lti_thresold;
	p_sharp_reg->cti_v_cfg.tiGain = p_shp_hal_cfg->sharp_cti_v_cfg.lti_gain;

	if ((p_sharp_reg->cti_enable == 1) && (p_shp_hal_cfg->sharp_cti_h_cfg.lti_enable == 0))
		p_sharp_reg->cti_h_cfg.tiGain = 0;

	if ((p_sharp_reg->cti_enable == 1) && (p_shp_hal_cfg->sharp_cti_v_cfg.lti_enable == 0))
		p_sharp_reg->cti_v_cfg.tiGain = 0;

	/* Peaking SW cfg */
	p_sharp_reg->peaking_enable = p_shp_hal_cfg->sharp_peaking_cfg.peaking_enable;
	p_sharp_reg->peaking_cfg.peaking_ctrl_enable =
		(p_shp_hal_cfg->sharp_peaking_cfg.coring_enable) ||
		(p_shp_hal_cfg->sharp_peaking_cfg.gain_enable) ||
		(p_shp_hal_cfg->sharp_peaking_cfg.limit_ctrl_enable);

	/* Peaking Ctrl sw-process */
	for (ii = 0; ii < SHP_PEAKING_BAND_NUM; ii++) {
		s16 coring_ratio, coring_zero, coring_thr, gain_ctrl_pos, gain_ctrl_neg,
			limit_ctrl_p0, limit_ctrl_p1, limit_ctrl_n0, limit_ctrl_n1,
			limit_ctrl_ratio;
		if (p_shp_hal_cfg->sharp_peaking_cfg.coring_enable == 1) {
			coring_ratio = p_shp_hal_cfg->sharp_peaking_cfg.coring_ratio[ii];
			coring_zero = p_shp_hal_cfg->sharp_peaking_cfg.coring_zero[ii];
			coring_thr = p_shp_hal_cfg->sharp_peaking_cfg.coring_thr[ii];
		} else {
			coring_ratio = 1024;
			coring_zero = 0;
			coring_thr = 0;
		}
		if (p_shp_hal_cfg->sharp_peaking_cfg.gain_enable == 1) {
			gain_ctrl_pos = p_shp_hal_cfg->sharp_peaking_cfg.gain_pos[ii];
			gain_ctrl_neg = p_shp_hal_cfg->sharp_peaking_cfg.gain_neg[ii];
		} else {
			gain_ctrl_pos = 1024;
			gain_ctrl_neg = 1024;
		}
		if (p_shp_hal_cfg->sharp_peaking_cfg.limit_ctrl_enable == 1) {
			limit_ctrl_p0 = p_shp_hal_cfg->sharp_peaking_cfg.limit_ctrl_pos0[ii];
			limit_ctrl_p1 = p_shp_hal_cfg->sharp_peaking_cfg.limit_ctrl_pos1[ii];
			limit_ctrl_n0 = p_shp_hal_cfg->sharp_peaking_cfg.limit_ctrl_neg0[ii];
			limit_ctrl_n1 = p_shp_hal_cfg->sharp_peaking_cfg.limit_ctrl_neg1[ii];
			limit_ctrl_ratio = p_shp_hal_cfg->sharp_peaking_cfg.limit_ctrl_ratio[ii];
		} else {
			limit_ctrl_p0 = 1023;
			limit_ctrl_p1 = 1023;
			limit_ctrl_n0 = 1023;
			limit_ctrl_n1 = 1023;
			limit_ctrl_ratio = 1024;
		}

		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P0[ii] = coring_zero;
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N0[ii] = -coring_zero;
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P1[ii] = coring_thr;
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N1[ii] = -coring_thr;
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P2[ii] = limit_ctrl_p0;
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N2[ii] = -limit_ctrl_n0;
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P3[ii] = limit_ctrl_p1;
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N3[ii] = -limit_ctrl_n1;

		u16 ratio_pos_tmp = (coring_ratio * gain_ctrl_pos + 512) >> 10;
		u16 ratio_neg_tmp = (coring_ratio * gain_ctrl_neg + 512) >> 10;
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P1[ii] =
			((ratio_pos_tmp * (coring_thr - coring_zero) + 512) >> 10);
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N1[ii] =
			-((ratio_neg_tmp * (coring_thr - coring_zero) + 512) >> 10);
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P01[ii] = ratio_pos_tmp;
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N01[ii] = ratio_neg_tmp;

		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P12[ii] = gain_ctrl_pos;
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N12[ii] = gain_ctrl_neg;

		u16 peaking_ctrl_add_tmp;
		peaking_ctrl_add_tmp = (gain_ctrl_pos * (limit_ctrl_p0 - coring_thr) + 512) >> 10;
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P2[ii] =
			p_sharp_reg->peaking_cfg.peaking_ctrl_value_P1[ii] + peaking_ctrl_add_tmp;
		peaking_ctrl_add_tmp = (gain_ctrl_neg * (limit_ctrl_n0 - coring_thr) + 512) >> 10;
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N2[ii] =
			p_sharp_reg->peaking_cfg.peaking_ctrl_value_N1[ii] - peaking_ctrl_add_tmp;

		ratio_pos_tmp = (limit_ctrl_ratio * gain_ctrl_pos + 512) >> 10;
		ratio_neg_tmp = (limit_ctrl_ratio * gain_ctrl_neg + 512) >> 10;

		peaking_ctrl_add_tmp =
			(ratio_pos_tmp * (limit_ctrl_p1 - limit_ctrl_p0) + 512) >> 10;
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P3[ii] =
			p_sharp_reg->peaking_cfg.peaking_ctrl_value_P2[ii] + peaking_ctrl_add_tmp;
		peaking_ctrl_add_tmp =
			(ratio_neg_tmp * (limit_ctrl_n1 - limit_ctrl_n0) + 512) >> 10;
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N3[ii] =
			p_sharp_reg->peaking_cfg.peaking_ctrl_value_N2[ii] - peaking_ctrl_add_tmp;
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P23[ii] = ratio_pos_tmp;
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N23[ii] = ratio_neg_tmp;

		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N0[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N0[ii], -1024, 1023);
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N1[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N1[ii], -1024, 1023);
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N2[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N2[ii], -1024, 1023);
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N3[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N3[ii], -1024, 1023);
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P0[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P0[ii], -1024, 1023);
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P1[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P1[ii], -1024, 1023);
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P2[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P2[ii], -1024, 1023);
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P3[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P3[ii], -1024, 1023);

		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N1[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_value_N1[ii], -1024, 1023);
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N2[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_value_N2[ii], -1024, 1023);
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N3[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_value_N3[ii], -1024, 1023);
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P1[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_value_P1[ii], -1024, 1023);
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P2[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_value_P2[ii], -1024, 1023);
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P3[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_value_P3[ii], -1024, 1023);

		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P01[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P01[ii], 0, 4095);
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P12[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P12[ii], 0, 4095);
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P23[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P23[ii], 0, 4095);
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N01[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N01[ii], 0, 4095);
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N12[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N12[ii], 0, 4095);
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N23[ii] =
			CLIP(p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N23[ii], 0, 4095);
	}

	/* peaking shoot adj params */
	p_sharp_reg->peaking_cfg.peaking_shtctrl_enable =
		p_shp_hal_cfg->sharp_peaking_cfg.shoot_adj_enable;
	memcpy(p_sharp_reg->peaking_cfg.peaking_shtctrl_delta_offset,
	       p_shp_hal_cfg->sharp_peaking_cfg.shoot_adj_delta_offset,
	       sizeof(p_sharp_reg->peaking_cfg.peaking_shtctrl_delta_offset));
	memcpy(p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over,
	       p_shp_hal_cfg->sharp_peaking_cfg.shoot_adj_alpha_over,
	       sizeof(p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over));
	memcpy(p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under,
	       p_shp_hal_cfg->sharp_peaking_cfg.shoot_adj_alpha_under,
	       sizeof(p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under));
	memcpy(p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over_unlimit,
	       p_shp_hal_cfg->sharp_peaking_cfg.shoot_adj_alpha_over_unlimit,
	       sizeof(p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over_unlimit));
	memcpy(p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under_unlimit,
	       p_shp_hal_cfg->sharp_peaking_cfg.shoot_adj_alpha_under_unlimit,
	       sizeof(p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under_unlimit));

	/* peaking gain */
	p_sharp_reg->peaking_cfg.peaking_gain = p_shp_hal_cfg->sharp_peaking_cfg.peaking_gain;

	/* shoot_ctrl params */
	p_sharp_reg->shoot_ctrl_enable = p_shp_hal_cfg->sharp_shootctrl_cfg.shootctrl_enable;
	p_sharp_reg->shoot_ctrl_cfg.shoot_ctrl_filt_radius =
		p_shp_hal_cfg->sharp_shootctrl_cfg.filter_radius;
	p_sharp_reg->shoot_ctrl_cfg.shoot_ctrl_delta_offset =
		p_shp_hal_cfg->sharp_shootctrl_cfg.delta_offset;
	p_sharp_reg->shoot_ctrl_cfg.shoot_ctrl_alpha_over =
		p_shp_hal_cfg->sharp_shootctrl_cfg.alpha_over;
	p_sharp_reg->shoot_ctrl_cfg.shoot_ctrl_alpha_under =
		p_shp_hal_cfg->sharp_shootctrl_cfg.alpha_under;
	p_sharp_reg->shoot_ctrl_cfg.shoot_ctrl_alpha_over_unlimit =
		p_shp_hal_cfg->sharp_shootctrl_cfg.alpha_over_unlimit;
	p_sharp_reg->shoot_ctrl_cfg.shoot_ctrl_alpha_under_unlimit =
		p_shp_hal_cfg->sharp_shootctrl_cfg.alpha_under_unlimit;

	/* hardware Reg Config */
	/* Filter Core Config */
	p_sharp_reg->sharp_H_core[0].sharp_Reg_core_H0 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H0[0];
	p_sharp_reg->sharp_H_core[0].sharp_Reg_core_H1 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H0[1];
	p_sharp_reg->sharp_H_core[0].sharp_Reg_core_H2 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H0[2];
	p_sharp_reg->sharp_H_core[0].sharp_Reg_core_H3 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H0[3];
	p_sharp_reg->sharp_H_core[0].sharp_Reg_core_H4 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H0[4];
	p_sharp_reg->sharp_H_core[0].sharp_Reg_core_H5 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H0[5];

	p_sharp_reg->sharp_H_core[1].sharp_Reg_core_H0 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H1[0];
	p_sharp_reg->sharp_H_core[1].sharp_Reg_core_H1 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H1[1];
	p_sharp_reg->sharp_H_core[1].sharp_Reg_core_H2 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H1[2];
	p_sharp_reg->sharp_H_core[1].sharp_Reg_core_H3 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H1[3];
	p_sharp_reg->sharp_H_core[1].sharp_Reg_core_H4 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H1[4];
	p_sharp_reg->sharp_H_core[1].sharp_Reg_core_H5 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H1[5];

	p_sharp_reg->sharp_H_core[2].sharp_Reg_core_H0 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H2[0];
	p_sharp_reg->sharp_H_core[2].sharp_Reg_core_H1 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H2[1];
	p_sharp_reg->sharp_H_core[2].sharp_Reg_core_H2 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H2[2];
	p_sharp_reg->sharp_H_core[2].sharp_Reg_core_H3 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H2[3];
	p_sharp_reg->sharp_H_core[2].sharp_Reg_core_H4 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H2[4];
	p_sharp_reg->sharp_H_core[2].sharp_Reg_core_H5 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H2[5];

	p_sharp_reg->sharp_H_core[3].sharp_Reg_core_H0 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H3[0];
	p_sharp_reg->sharp_H_core[3].sharp_Reg_core_H1 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H3[1];
	p_sharp_reg->sharp_H_core[3].sharp_Reg_core_H2 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H3[2];
	p_sharp_reg->sharp_H_core[3].sharp_Reg_core_H3 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H3[3];
	p_sharp_reg->sharp_H_core[3].sharp_Reg_core_H4 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H3[4];
	p_sharp_reg->sharp_H_core[3].sharp_Reg_core_H5 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_H3[5];

	p_sharp_reg->sharp_V_core[0].sharp_Reg_core_V0 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_V0[0];
	p_sharp_reg->sharp_V_core[0].sharp_Reg_core_V1 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_V0[1];
	p_sharp_reg->sharp_V_core[0].sharp_Reg_core_V2 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_V0[2];

	p_sharp_reg->sharp_V_core[1].sharp_Reg_core_V0 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_V1[0];
	p_sharp_reg->sharp_V_core[1].sharp_Reg_core_V1 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_V1[1];
	p_sharp_reg->sharp_V_core[1].sharp_Reg_core_V2 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_V1[2];

	p_sharp_reg->sharp_V_core[2].sharp_Reg_core_V0 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_V2[0];
	p_sharp_reg->sharp_V_core[2].sharp_Reg_core_V1 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_V2[1];
	p_sharp_reg->sharp_V_core[2].sharp_Reg_core_V2 =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_V2[2];

	memcpy(p_sharp_reg->sharp_Reg_USM_core,
	       p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_USM,
	       sizeof(p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_filt_core_USM));

	p_sharp_reg->sharp_Reg_Diag_coef =
		p_shp_hal_cfg->sharp_peaking_cfg.filter_cfg_diag_enh_coef;

	/* Peaking Ctrl Config */

	/* Edge Process Config */
	p_sharp_reg->peaking_cfg.edge_proc_cfg.edge_proc_enable =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_ctrl_enable;
	p_sharp_reg->peaking_cfg.edge_proc_cfg.non_dir_thr =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_ctrl_non_dir_thr;
	p_sharp_reg->peaking_cfg.edge_proc_cfg.dir_cmp_ratio =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_ctrl_dir_cmp_ratio;
	p_sharp_reg->peaking_cfg.edge_proc_cfg.non_dir_wgt_offset =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_ctrl_non_dir_wgt_offset;
	p_sharp_reg->peaking_cfg.edge_proc_cfg.non_dir_wgt_ratio =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_ctrl_non_dir_wgt_ratio;	/* 1.0f, 4bitFix */

	p_sharp_reg->peaking_cfg.edge_proc_cfg.dir_cnt_thr =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_ctrl_dir_cnt_thr;	/* 1.0f, 4bitFix */
	p_sharp_reg->peaking_cfg.edge_proc_cfg.dir_cnt_avg =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_ctrl_dir_cnt_avg;	/* 1.0f, 4bitFix */
	p_sharp_reg->peaking_cfg.edge_proc_cfg.dir_cnt_offset =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_ctrl_dir_cnt_offset;	/* 1.0f, 4bitFix */

	p_sharp_reg->peaking_cfg.edge_proc_cfg.diag_dir_thr =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_ctrl_diag_dir_thr;
	memcpy(p_sharp_reg->peaking_cfg.edge_proc_cfg.diag_adj_gain_tab,
	       p_shp_hal_cfg->sharp_peaking_cfg.edge_ctrl_diag_adj_gain_tab,
	       sizeof(p_sharp_reg->peaking_cfg.edge_proc_cfg.diag_adj_gain_tab));

	/* Edge Shoot Ctrl Config */
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.edge_shoot_ctrl_enable =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_enable;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.delta_offset_h =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_delta_offset_h;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_h =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_over_h;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_h =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_under_h;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_unlimit_h =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_over_unlimit_h;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_unlimit_h =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_under_unlimit_h;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.delta_offset_v =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_delta_offset_v;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_v =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_over_v;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_v =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_under_v;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_unlimit_v =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_over_unlimit_v;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_unlimit_v =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_under_unlimit_v;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.delta_offset_d0 =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_delta_offset_d0;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_d0 =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_over_d0;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_d0 =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_under_d0;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_unlimit_d0 =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_over_unlimit_d0;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_unlimit_d0 =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_under_unlimit_d0;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.delta_offset_d1 =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_delta_offset_d1;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_d1 =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_over_d1;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_d1 =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_under_d1;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_unlimit_d1 =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_over_unlimit_d1;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_unlimit_d1 =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_under_unlimit_d1;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.delta_offset_non =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_delta_offset_non;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_non =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_over_non;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_non =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_under_non;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_unlimit_non =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_over_unlimit_non;
	p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_unlimit_non =
		p_shp_hal_cfg->sharp_peaking_cfg.edge_shoot_ctrl_alpha_under_unlimit_non;

	/* GainCtrl Config */
	p_sharp_reg->gain_ctrl_enable = p_shp_hal_cfg->sharp_global_gain_cfg.global_gain_enable;
	p_sharp_reg->gain_ctrl_cfg.adp_idx0 = p_shp_hal_cfg->sharp_global_gain_cfg.adp_grd[1];
	p_sharp_reg->gain_ctrl_cfg.adp_idx1 = p_shp_hal_cfg->sharp_global_gain_cfg.adp_grd[2];
	p_sharp_reg->gain_ctrl_cfg.adp_idx2 = p_shp_hal_cfg->sharp_global_gain_cfg.adp_grd[3];
	p_sharp_reg->gain_ctrl_cfg.adp_idx3 = p_shp_hal_cfg->sharp_global_gain_cfg.adp_grd[4];

	p_sharp_reg->gain_ctrl_cfg.adp_gain0 = p_shp_hal_cfg->sharp_global_gain_cfg.adp_val[0];
	p_sharp_reg->gain_ctrl_cfg.adp_gain1 = p_shp_hal_cfg->sharp_global_gain_cfg.adp_val[1];
	p_sharp_reg->gain_ctrl_cfg.adp_gain2 = p_shp_hal_cfg->sharp_global_gain_cfg.adp_val[2];
	p_sharp_reg->gain_ctrl_cfg.adp_gain3 = p_shp_hal_cfg->sharp_global_gain_cfg.adp_val[3];
	p_sharp_reg->gain_ctrl_cfg.adp_gain4 = p_shp_hal_cfg->sharp_global_gain_cfg.adp_val[4];
	/* p_sharp_reg->gain_ctrl_cfg.adp_gain5 = pq_sharp_gain_ctrl_params.gain_ctrl_adp_gain2; */

	p_sharp_reg->gain_ctrl_cfg.adp_slp01 = shp_round_div(
		128 * (p_sharp_reg->gain_ctrl_cfg.adp_gain1 - p_sharp_reg->gain_ctrl_cfg.adp_gain0),
		MAX((p_sharp_reg->gain_ctrl_cfg.adp_idx0 - 0), 1));
	p_sharp_reg->gain_ctrl_cfg.adp_slp12 = shp_round_div(
		128 * (p_sharp_reg->gain_ctrl_cfg.adp_gain2 - p_sharp_reg->gain_ctrl_cfg.adp_gain1),
		MAX((p_sharp_reg->gain_ctrl_cfg.adp_idx1 -
		      p_sharp_reg->gain_ctrl_cfg.adp_idx0), 1));
	p_sharp_reg->gain_ctrl_cfg.adp_slp23 = shp_round_div(
		128 * (p_sharp_reg->gain_ctrl_cfg.adp_gain3 - p_sharp_reg->gain_ctrl_cfg.adp_gain2),
		MAX((p_sharp_reg->gain_ctrl_cfg.adp_idx2 -
		      p_sharp_reg->gain_ctrl_cfg.adp_idx1), 1));
	p_sharp_reg->gain_ctrl_cfg.adp_slp34 = shp_round_div(
		128 * (p_sharp_reg->gain_ctrl_cfg.adp_gain4 - p_sharp_reg->gain_ctrl_cfg.adp_gain3),
		MAX((p_sharp_reg->gain_ctrl_cfg.adp_idx3 -
		      p_sharp_reg->gain_ctrl_cfg.adp_idx2), 1));
	p_sharp_reg->gain_ctrl_cfg.adp_slp45 = shp_round_div(
		128 * (p_shp_hal_cfg->sharp_global_gain_cfg.adp_val[5] -
		        p_sharp_reg->gain_ctrl_cfg.adp_gain4),
		MAX((SHP_GAINCTRL_LUTLENGTH - 1 - p_sharp_reg->gain_ctrl_cfg.adp_idx3), 1));

	p_sharp_reg->gain_ctrl_cfg.adp_slp01 =
		CLIP(p_sharp_reg->gain_ctrl_cfg.adp_slp01, -1024, 1023);
	p_sharp_reg->gain_ctrl_cfg.adp_slp12 =
		CLIP(p_sharp_reg->gain_ctrl_cfg.adp_slp12, -1024, 1023);
	p_sharp_reg->gain_ctrl_cfg.adp_slp23 =
		CLIP(p_sharp_reg->gain_ctrl_cfg.adp_slp23, -1024, 1023);
	p_sharp_reg->gain_ctrl_cfg.adp_slp34 =
		CLIP(p_sharp_reg->gain_ctrl_cfg.adp_slp34, -1024, 1023);
	p_sharp_reg->gain_ctrl_cfg.adp_slp45 =
		CLIP(p_sharp_reg->gain_ctrl_cfg.adp_slp45, -1024, 1023);

	p_sharp_reg->gain_ctrl_cfg.var_idx0 = p_shp_hal_cfg->sharp_global_gain_cfg.var_grd[1];
	p_sharp_reg->gain_ctrl_cfg.var_idx1 = p_shp_hal_cfg->sharp_global_gain_cfg.var_grd[2];
	p_sharp_reg->gain_ctrl_cfg.var_idx2 = p_shp_hal_cfg->sharp_global_gain_cfg.var_grd[3];
	p_sharp_reg->gain_ctrl_cfg.var_idx3 = p_shp_hal_cfg->sharp_global_gain_cfg.var_grd[4];

	p_sharp_reg->gain_ctrl_cfg.var_gain0 = p_shp_hal_cfg->sharp_global_gain_cfg.var_val[0];
	p_sharp_reg->gain_ctrl_cfg.var_gain1 = p_shp_hal_cfg->sharp_global_gain_cfg.var_val[1];
	p_sharp_reg->gain_ctrl_cfg.var_gain2 = p_shp_hal_cfg->sharp_global_gain_cfg.var_val[2];
	p_sharp_reg->gain_ctrl_cfg.var_gain3 = p_shp_hal_cfg->sharp_global_gain_cfg.var_val[3];
	p_sharp_reg->gain_ctrl_cfg.var_gain4 = p_shp_hal_cfg->sharp_global_gain_cfg.var_val[4];
	/* p_sharp_reg->gain_ctrl_cfg.var_gain5 = pq_sharp_gain_ctrl_params.gain_ctrl_var_gain2; */

	p_sharp_reg->gain_ctrl_cfg.var_slp01 = shp_round_div(
		128 * (p_sharp_reg->gain_ctrl_cfg.var_gain1 - p_sharp_reg->gain_ctrl_cfg.var_gain0),
		MAX((p_sharp_reg->gain_ctrl_cfg.var_idx0 - 0), 1));
	p_sharp_reg->gain_ctrl_cfg.var_slp12 = shp_round_div(
		128 * (p_sharp_reg->gain_ctrl_cfg.var_gain2 - p_sharp_reg->gain_ctrl_cfg.var_gain1),
		MAX((p_sharp_reg->gain_ctrl_cfg.var_idx1 -
		      p_sharp_reg->gain_ctrl_cfg.var_idx0), 1));
	p_sharp_reg->gain_ctrl_cfg.var_slp23 = shp_round_div(
		128 * (p_sharp_reg->gain_ctrl_cfg.var_gain3 - p_sharp_reg->gain_ctrl_cfg.var_gain2),
		MAX((p_sharp_reg->gain_ctrl_cfg.var_idx2 -
		      p_sharp_reg->gain_ctrl_cfg.var_idx1), 1));
	p_sharp_reg->gain_ctrl_cfg.var_slp34 = shp_round_div(
		128 * (p_sharp_reg->gain_ctrl_cfg.var_gain4 - p_sharp_reg->gain_ctrl_cfg.var_gain3),
		MAX((p_sharp_reg->gain_ctrl_cfg.var_idx3 -
		      p_sharp_reg->gain_ctrl_cfg.var_idx2), 1));
	p_sharp_reg->gain_ctrl_cfg.var_slp45 = shp_round_div(
		128 * (p_shp_hal_cfg->sharp_global_gain_cfg.var_val[5] -
		        p_sharp_reg->gain_ctrl_cfg.var_gain4),
		MAX((SHP_GAINCTRL_LUTLENGTH - 1 - p_sharp_reg->gain_ctrl_cfg.var_idx3), 1));

	p_sharp_reg->gain_ctrl_cfg.var_slp01 =
		CLIP(p_sharp_reg->gain_ctrl_cfg.var_slp01, -1024, 1023);
	p_sharp_reg->gain_ctrl_cfg.var_slp12 =
		CLIP(p_sharp_reg->gain_ctrl_cfg.var_slp12, -1024, 1023);
	p_sharp_reg->gain_ctrl_cfg.var_slp23 =
		CLIP(p_sharp_reg->gain_ctrl_cfg.var_slp23, -1024, 1023);
	p_sharp_reg->gain_ctrl_cfg.var_slp34 =
		CLIP(p_sharp_reg->gain_ctrl_cfg.var_slp34, -1024, 1023);
	p_sharp_reg->gain_ctrl_cfg.var_slp45 =
		CLIP(p_sharp_reg->gain_ctrl_cfg.var_slp45, -1024, 1023);

	/* Luma Gain Ctrl */
	p_sharp_reg->gain_ctrl_cfg.lum_select = p_shp_hal_cfg->sharp_global_gain_cfg.lum_mode;
	p_sharp_reg->gain_ctrl_cfg.lum_idx0 = p_shp_hal_cfg->sharp_global_gain_cfg.lum_grd[1];
	p_sharp_reg->gain_ctrl_cfg.lum_idx1 = p_shp_hal_cfg->sharp_global_gain_cfg.lum_grd[2];
	p_sharp_reg->gain_ctrl_cfg.lum_idx2 = p_shp_hal_cfg->sharp_global_gain_cfg.lum_grd[3];
	p_sharp_reg->gain_ctrl_cfg.lum_idx3 = p_shp_hal_cfg->sharp_global_gain_cfg.lum_grd[4];

	p_sharp_reg->gain_ctrl_cfg.lum_gain0 = p_shp_hal_cfg->sharp_global_gain_cfg.lum_val[0];
	p_sharp_reg->gain_ctrl_cfg.lum_gain1 = p_shp_hal_cfg->sharp_global_gain_cfg.lum_val[1];
	p_sharp_reg->gain_ctrl_cfg.lum_gain2 = p_shp_hal_cfg->sharp_global_gain_cfg.lum_val[2];
	p_sharp_reg->gain_ctrl_cfg.lum_gain3 = p_shp_hal_cfg->sharp_global_gain_cfg.lum_val[3];
	p_sharp_reg->gain_ctrl_cfg.lum_gain4 = p_shp_hal_cfg->sharp_global_gain_cfg.lum_val[4];
	/* p_sharp_reg->gain_ctrl_cfg.lum_gain5 = pq_sharp_gain_ctrl_params.gain_ctrl_lum_gain2; */

	p_sharp_reg->gain_ctrl_cfg.lum_slp01 = shp_round_div(
		128 * (p_sharp_reg->gain_ctrl_cfg.lum_gain1 - p_sharp_reg->gain_ctrl_cfg.lum_gain0),
		MAX((p_sharp_reg->gain_ctrl_cfg.lum_idx0 - 0), 1));
	p_sharp_reg->gain_ctrl_cfg.lum_slp12 = shp_round_div(
		128 * (p_sharp_reg->gain_ctrl_cfg.lum_gain2 - p_sharp_reg->gain_ctrl_cfg.lum_gain1),
		MAX((p_sharp_reg->gain_ctrl_cfg.lum_idx1 -
		      p_sharp_reg->gain_ctrl_cfg.lum_idx0), 1));
	p_sharp_reg->gain_ctrl_cfg.lum_slp23 = shp_round_div(
		128 * (p_sharp_reg->gain_ctrl_cfg.lum_gain3 - p_sharp_reg->gain_ctrl_cfg.lum_gain2),
		MAX((p_sharp_reg->gain_ctrl_cfg.lum_idx2 -
		      p_sharp_reg->gain_ctrl_cfg.lum_idx1), 1));
	p_sharp_reg->gain_ctrl_cfg.lum_slp34 = shp_round_div(
		128 * (p_sharp_reg->gain_ctrl_cfg.lum_gain4 - p_sharp_reg->gain_ctrl_cfg.lum_gain3),
		MAX((p_sharp_reg->gain_ctrl_cfg.lum_idx3 -
		      p_sharp_reg->gain_ctrl_cfg.lum_idx2), 1));
	p_sharp_reg->gain_ctrl_cfg.lum_slp45 = shp_round_div(
		128 * (p_shp_hal_cfg->sharp_global_gain_cfg.lum_val[5] -
		        p_sharp_reg->gain_ctrl_cfg.lum_gain4),
		MAX((SHP_GAINCTRL_LUTLENGTH - 1 - p_sharp_reg->gain_ctrl_cfg.lum_idx3), 1));

	p_sharp_reg->gain_ctrl_cfg.lum_slp01 =
		CLIP(p_sharp_reg->gain_ctrl_cfg.lum_slp01, -1024, 1023);
	p_sharp_reg->gain_ctrl_cfg.lum_slp12 =
		CLIP(p_sharp_reg->gain_ctrl_cfg.lum_slp12, -1024, 1023);
	p_sharp_reg->gain_ctrl_cfg.lum_slp23 =
		CLIP(p_sharp_reg->gain_ctrl_cfg.lum_slp23, -1024, 1023);
	p_sharp_reg->gain_ctrl_cfg.lum_slp34 =
		CLIP(p_sharp_reg->gain_ctrl_cfg.lum_slp34, -1024, 1023);
	p_sharp_reg->gain_ctrl_cfg.lum_slp45 =
		CLIP(p_sharp_reg->gain_ctrl_cfg.lum_slp45, -1024, 1023);

	/* Color Adjust */
	p_sharp_reg->color_adj_enable = p_shp_hal_cfg->sharp_color_adj_cfg.color_ctrl_enable;
	p_sharp_reg->color_adj_params[0].adj_point_x =
		p_shp_hal_cfg->sharp_color_adj_cfg.p0_ctrl_point_u;
	p_sharp_reg->color_adj_params[0].adj_point_y =
		p_shp_hal_cfg->sharp_color_adj_cfg.p0_ctrl_point_v;
	p_sharp_reg->color_adj_params[0].adj_scaling_coef =
		p_shp_hal_cfg->sharp_color_adj_cfg.p0_scaling_coef;
	memcpy(p_sharp_reg->color_adj_params[0].color_adj_tab,
	       p_shp_hal_cfg->sharp_color_adj_cfg.p0_ctrl_roll_tab,
	       sizeof(p_shp_hal_cfg->sharp_color_adj_cfg.p0_ctrl_roll_tab));

	p_sharp_reg->color_adj_params[1].adj_point_x =
		p_shp_hal_cfg->sharp_color_adj_cfg.p1_ctrl_point_u;
	p_sharp_reg->color_adj_params[1].adj_point_y =
		p_shp_hal_cfg->sharp_color_adj_cfg.p1_ctrl_point_v;
	p_sharp_reg->color_adj_params[1].adj_scaling_coef =
		p_shp_hal_cfg->sharp_color_adj_cfg.p1_scaling_coef;
	memcpy(p_sharp_reg->color_adj_params[1].color_adj_tab,
	       p_shp_hal_cfg->sharp_color_adj_cfg.p1_ctrl_roll_tab,
	       sizeof(p_shp_hal_cfg->sharp_color_adj_cfg.p0_ctrl_roll_tab));

	p_sharp_reg->color_adj_params[2].adj_point_x =
		p_shp_hal_cfg->sharp_color_adj_cfg.p2_ctrl_point_u;
	p_sharp_reg->color_adj_params[2].adj_point_y =
		p_shp_hal_cfg->sharp_color_adj_cfg.p2_ctrl_point_v;
	p_sharp_reg->color_adj_params[2].adj_scaling_coef =
		p_shp_hal_cfg->sharp_color_adj_cfg.p2_scaling_coef;
	memcpy(p_sharp_reg->color_adj_params[2].color_adj_tab,
	       p_shp_hal_cfg->sharp_color_adj_cfg.p2_ctrl_roll_tab,
	       sizeof(p_shp_hal_cfg->sharp_color_adj_cfg.p0_ctrl_roll_tab));

	p_sharp_reg->color_adj_params[3].adj_point_x =
		p_shp_hal_cfg->sharp_color_adj_cfg.p3_ctrl_point_u;
	p_sharp_reg->color_adj_params[3].adj_point_y =
		p_shp_hal_cfg->sharp_color_adj_cfg.p3_ctrl_point_v;
	p_sharp_reg->color_adj_params[3].adj_scaling_coef =
		p_shp_hal_cfg->sharp_color_adj_cfg.p3_scaling_coef;
	memcpy(p_sharp_reg->color_adj_params[3].color_adj_tab,
	       p_shp_hal_cfg->sharp_color_adj_cfg.p3_ctrl_roll_tab,
	       sizeof(p_shp_hal_cfg->sharp_color_adj_cfg.p0_ctrl_roll_tab));

	/* Texture Adjust */
	p_sharp_reg->texture_adj_enable = p_shp_hal_cfg->sharp_texture_adj_cfg.textureAdj_enable;
	/* 0: use direct_wgt, 1: use abs(highFreq) */
	p_sharp_reg->texture_adj_cfg.idx_mode_select =
		p_shp_hal_cfg->sharp_texture_adj_cfg.idx_mode_select;
	/* 0: use Yin+Hfreq, 1: use Yin, 2: use Mean3x3 of Yin, 3: use Mean5x5 of Yin */
	p_sharp_reg->texture_adj_cfg.Y_mode_select =
		p_shp_hal_cfg->sharp_texture_adj_cfg.y_mode_select;
	p_sharp_reg->texture_adj_cfg.tex_idx0 = p_shp_hal_cfg->sharp_texture_adj_cfg.texture_grd[1];
	p_sharp_reg->texture_adj_cfg.tex_idx1 = p_shp_hal_cfg->sharp_texture_adj_cfg.texture_grd[2];
	p_sharp_reg->texture_adj_cfg.tex_idx2 = p_shp_hal_cfg->sharp_texture_adj_cfg.texture_grd[3];
	p_sharp_reg->texture_adj_cfg.tex_idx3 = p_shp_hal_cfg->sharp_texture_adj_cfg.texture_grd[4];

	p_sharp_reg->texture_adj_cfg.tex_gain0 =
		p_shp_hal_cfg->sharp_texture_adj_cfg.texture_val[0];
	p_sharp_reg->texture_adj_cfg.tex_gain1 =
		p_shp_hal_cfg->sharp_texture_adj_cfg.texture_val[1];
	p_sharp_reg->texture_adj_cfg.tex_gain2 =
		p_shp_hal_cfg->sharp_texture_adj_cfg.texture_val[2];
	p_sharp_reg->texture_adj_cfg.tex_gain3 =
		p_shp_hal_cfg->sharp_texture_adj_cfg.texture_val[3];
	p_sharp_reg->texture_adj_cfg.tex_gain4 =
		p_shp_hal_cfg->sharp_texture_adj_cfg.texture_val[4];

	p_sharp_reg->texture_adj_cfg.tex_slp01 = shp_round_div(
		128 * (p_sharp_reg->texture_adj_cfg.tex_gain1 -
		        p_sharp_reg->texture_adj_cfg.tex_gain0),
		MAX((p_sharp_reg->texture_adj_cfg.tex_idx0 - 0), 1));
	p_sharp_reg->texture_adj_cfg.tex_slp12 = shp_round_div(
		128 * (p_sharp_reg->texture_adj_cfg.tex_gain2 -
		        p_sharp_reg->texture_adj_cfg.tex_gain1),
		MAX((p_sharp_reg->texture_adj_cfg.tex_idx1 -
		      p_sharp_reg->texture_adj_cfg.tex_idx0), 1));
	p_sharp_reg->texture_adj_cfg.tex_slp23 = shp_round_div(
		128 * (p_sharp_reg->texture_adj_cfg.tex_gain3 -
		        p_sharp_reg->texture_adj_cfg.tex_gain2),
		MAX((p_sharp_reg->texture_adj_cfg.tex_idx2 -
		      p_sharp_reg->texture_adj_cfg.tex_idx1), 1));
	p_sharp_reg->texture_adj_cfg.tex_slp34 = shp_round_div(
		128 * (p_sharp_reg->texture_adj_cfg.tex_gain4 -
		        p_sharp_reg->texture_adj_cfg.tex_gain3),
		MAX((p_sharp_reg->texture_adj_cfg.tex_idx3 -
		      p_sharp_reg->texture_adj_cfg.tex_idx2), 1));
	p_sharp_reg->texture_adj_cfg.tex_slp45 = shp_round_div(
		128 * (p_shp_hal_cfg->sharp_texture_adj_cfg.texture_val[5] -
		        p_sharp_reg->texture_adj_cfg.tex_gain4),
		MAX((SHP_GAINCTRL_LUTLENGTH - 1 - p_sharp_reg->texture_adj_cfg.tex_idx3), 1));

	p_sharp_reg->texture_adj_cfg.tex_slp01 =
		CLIP(p_sharp_reg->texture_adj_cfg.tex_slp01, -1024, 1023);
	p_sharp_reg->texture_adj_cfg.tex_slp12 =
		CLIP(p_sharp_reg->texture_adj_cfg.tex_slp12, -1024, 1023);
	p_sharp_reg->texture_adj_cfg.tex_slp23 =
		CLIP(p_sharp_reg->texture_adj_cfg.tex_slp23, -1024, 1023);
	p_sharp_reg->texture_adj_cfg.tex_slp34 =
		CLIP(p_sharp_reg->texture_adj_cfg.tex_slp34, -1024, 1023);
	p_sharp_reg->texture_adj_cfg.tex_slp45 =
		CLIP(p_sharp_reg->texture_adj_cfg.tex_slp45, -1024, 1023);
}

void rockchip_sharp_full_regs_generate(const sharp_hw_params *p_sharp_reg,
				       sharp_full_regs *p_kernel_cfg)
{
	p_kernel_cfg->regs.sw_ctrl.bits.sw_texture_adj_enable = p_sharp_reg->texture_adj_enable;
	p_kernel_cfg->regs.sw_ctrl.bits.sw_color_adj_enable = p_sharp_reg->color_adj_enable;
	p_kernel_cfg->regs.sw_ctrl.bits.sw_gain_ctrl_enable = p_sharp_reg->gain_ctrl_enable;
	p_kernel_cfg->regs.sw_ctrl.bits.sw_shoot_ctrl_enable = p_sharp_reg->shoot_ctrl_enable;
	p_kernel_cfg->regs.sw_ctrl.bits.sw_edge_proc_enable =
		p_sharp_reg->peaking_cfg.edge_proc_cfg.edge_proc_enable;
	p_kernel_cfg->regs.sw_ctrl.bits.sw_peaking_shoot_ctrl_enable =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_enable;
	p_kernel_cfg->regs.sw_ctrl.bits.sw_peaking_ctrl_enable =
		p_sharp_reg->peaking_cfg.peaking_ctrl_enable;
	p_kernel_cfg->regs.sw_ctrl.bits.sw_peaking_enable = p_sharp_reg->peaking_enable;
	p_kernel_cfg->regs.sw_ctrl.bits.sw_cti_enable = p_sharp_reg->cti_enable;
	p_kernel_cfg->regs.sw_ctrl.bits.sw_lti_enable = p_sharp_reg->lti_enable;
	p_kernel_cfg->regs.sw_ctrl.bits.sw_sharp_enable = p_sharp_reg->sharp_enable;
	p_kernel_cfg->regs.sw_peaking_filter_coe0.bits.sw_peaking_v12 =
		p_sharp_reg->sharp_V_core[1].sharp_Reg_core_V2;
	p_kernel_cfg->regs.sw_peaking_filter_coe0.bits.sw_peaking_v11 =
		p_sharp_reg->sharp_V_core[1].sharp_Reg_core_V1;
	p_kernel_cfg->regs.sw_peaking_filter_coe0.bits.sw_peaking_v10 =
		p_sharp_reg->sharp_V_core[1].sharp_Reg_core_V0;
	p_kernel_cfg->regs.sw_peaking_filter_coe0.bits.sw_peaking_v02 =
		p_sharp_reg->sharp_V_core[0].sharp_Reg_core_V2;
	p_kernel_cfg->regs.sw_peaking_filter_coe0.bits.sw_peaking_v01 =
		p_sharp_reg->sharp_V_core[0].sharp_Reg_core_V1;
	p_kernel_cfg->regs.sw_peaking_filter_coe0.bits.sw_peaking_v00 =
		p_sharp_reg->sharp_V_core[0].sharp_Reg_core_V0;
	p_kernel_cfg->regs.sw_peaking_filter_coe1.bits.sw_diag_coef =
		p_sharp_reg->sharp_Reg_Diag_coef;
	p_kernel_cfg->regs.sw_peaking_filter_coe1.bits.sw_peaking_usm2 =
		p_sharp_reg->sharp_Reg_USM_core[2];
	p_kernel_cfg->regs.sw_peaking_filter_coe1.bits.sw_peaking_usm1 =
		p_sharp_reg->sharp_Reg_USM_core[1];
	p_kernel_cfg->regs.sw_peaking_filter_coe1.bits.sw_peaking_usm0 =
		p_sharp_reg->sharp_Reg_USM_core[0];
	p_kernel_cfg->regs.sw_peaking_filter_coe1.bits.sw_peaking_v22 =
		p_sharp_reg->sharp_V_core[2].sharp_Reg_core_V2;
	p_kernel_cfg->regs.sw_peaking_filter_coe1.bits.sw_peaking_v21 =
		p_sharp_reg->sharp_V_core[2].sharp_Reg_core_V1;
	p_kernel_cfg->regs.sw_peaking_filter_coe1.bits.sw_peaking_v20 =
		p_sharp_reg->sharp_V_core[2].sharp_Reg_core_V0;
	p_kernel_cfg->regs.sw_peaking_filter_coe2.bits.sw_peaking_h02 =
		p_sharp_reg->sharp_H_core[0].sharp_Reg_core_H2;
	p_kernel_cfg->regs.sw_peaking_filter_coe2.bits.sw_peaking_h01 =
		p_sharp_reg->sharp_H_core[0].sharp_Reg_core_H1;
	p_kernel_cfg->regs.sw_peaking_filter_coe2.bits.sw_peaking_h00 =
		p_sharp_reg->sharp_H_core[0].sharp_Reg_core_H0;
	p_kernel_cfg->regs.sw_peaking_filter_coe3.bits.sw_peaking_h05 =
		p_sharp_reg->sharp_H_core[0].sharp_Reg_core_H5;
	p_kernel_cfg->regs.sw_peaking_filter_coe3.bits.sw_peaking_h04 =
		p_sharp_reg->sharp_H_core[0].sharp_Reg_core_H4;
	p_kernel_cfg->regs.sw_peaking_filter_coe3.bits.sw_peaking_h03 =
		p_sharp_reg->sharp_H_core[0].sharp_Reg_core_H3;
	p_kernel_cfg->regs.sw_peaking_filter_coe4.bits.sw_peaking_h12 =
		p_sharp_reg->sharp_H_core[1].sharp_Reg_core_H2;
	p_kernel_cfg->regs.sw_peaking_filter_coe4.bits.sw_peaking_h11 =
		p_sharp_reg->sharp_H_core[1].sharp_Reg_core_H1;
	p_kernel_cfg->regs.sw_peaking_filter_coe4.bits.sw_peaking_h10 =
		p_sharp_reg->sharp_H_core[1].sharp_Reg_core_H0;
	p_kernel_cfg->regs.sw_peaking_filter_coe5.bits.sw_peaking_h15 =
		p_sharp_reg->sharp_H_core[1].sharp_Reg_core_H5;
	p_kernel_cfg->regs.sw_peaking_filter_coe5.bits.sw_peaking_h14 =
		p_sharp_reg->sharp_H_core[1].sharp_Reg_core_H4;
	p_kernel_cfg->regs.sw_peaking_filter_coe5.bits.sw_peaking_h13 =
		p_sharp_reg->sharp_H_core[1].sharp_Reg_core_H3;
	p_kernel_cfg->regs.sw_peaking_filter_coe6.bits.sw_peaking_h22 =
		p_sharp_reg->sharp_H_core[2].sharp_Reg_core_H2;
	p_kernel_cfg->regs.sw_peaking_filter_coe6.bits.sw_peaking_h21 =
		p_sharp_reg->sharp_H_core[2].sharp_Reg_core_H1;
	p_kernel_cfg->regs.sw_peaking_filter_coe6.bits.sw_peaking_h20 =
		p_sharp_reg->sharp_H_core[2].sharp_Reg_core_H0;
	p_kernel_cfg->regs.sw_peaking_filter_coe7.bits.sw_peaking_h25 =
		p_sharp_reg->sharp_H_core[2].sharp_Reg_core_H5;
	p_kernel_cfg->regs.sw_peaking_filter_coe7.bits.sw_peaking_h24 =
		p_sharp_reg->sharp_H_core[2].sharp_Reg_core_H4;
	p_kernel_cfg->regs.sw_peaking_filter_coe7.bits.sw_peaking_h23 =
		p_sharp_reg->sharp_H_core[2].sharp_Reg_core_H3;
	p_kernel_cfg->regs.sw_peaking_filter_coe8.bits.sw_peaking_h32 =
		p_sharp_reg->sharp_H_core[3].sharp_Reg_core_H2;
	p_kernel_cfg->regs.sw_peaking_filter_coe8.bits.sw_peaking_h31 =
		p_sharp_reg->sharp_H_core[3].sharp_Reg_core_H1;
	p_kernel_cfg->regs.sw_peaking_filter_coe8.bits.sw_peaking_h30 =
		p_sharp_reg->sharp_H_core[3].sharp_Reg_core_H0;
	p_kernel_cfg->regs.sw_peaking_filter_coe9.bits.sw_peaking_h35 =
		p_sharp_reg->sharp_H_core[3].sharp_Reg_core_H5;
	p_kernel_cfg->regs.sw_peaking_filter_coe9.bits.sw_peaking_h34 =
		p_sharp_reg->sharp_H_core[3].sharp_Reg_core_H4;
	p_kernel_cfg->regs.sw_peaking_filter_coe9.bits.sw_peaking_h33 =
		p_sharp_reg->sharp_H_core[3].sharp_Reg_core_H3;
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe0.bits.sw_peaking0_idx_n1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N1[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe0.bits.sw_peaking0_idx_n0 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N0[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe1.bits.sw_peaking0_idx_n3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N3[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe1.bits.sw_peaking0_idx_n2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N2[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe2.bits.sw_peaking0_idx_p1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P1[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe2.bits.sw_peaking0_idx_p0 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P0[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe3.bits.sw_peaking0_idx_p3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P3[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe3.bits.sw_peaking0_idx_p2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P2[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe4.bits.sw_peaking0_value_n2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N2[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe4.bits.sw_peaking0_value_n1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N1[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe5.bits.sw_peaking0_value_p1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P1[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe5.bits.sw_peaking0_value_n3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N3[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe6.bits.sw_peaking0_value_p3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P3[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe6.bits.sw_peaking0_value_p2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P2[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe7.bits.sw_peaking0_ratio_n12 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N12[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe7.bits.sw_peaking0_ratio_n01 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N01[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe8.bits.sw_peaking0_ratio_p01 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P01[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe8.bits.sw_peaking0_ratio_n23 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N23[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe9.bits.sw_peaking0_shoot_delta_offset =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_delta_offset[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe9.bits.sw_peaking0_ratio_p23 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P23[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe9.bits.sw_peaking0_ratio_p12 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P12[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe10.bits.sw_peaking0_shoot_alpha_under_unlimit =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under_unlimit[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe10.bits.sw_peaking0_shoot_alpha_over_unlimit =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over_unlimit[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe10.bits.sw_peaking0_shoot_alpha_under =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under[0];
	p_kernel_cfg->regs.sw_peaking0_ctrl_coe10.bits.sw_peaking0_shoot_alpha_over =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over[0];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe0.bits.sw_peaking1_idx_n1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N1[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe0.bits.sw_peaking1_idx_n0 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N0[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe1.bits.sw_peaking1_idx_n3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N3[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe1.bits.sw_peaking1_idx_n2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N2[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe2.bits.sw_peaking1_idx_p1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P1[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe2.bits.sw_peaking1_idx_p0 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P0[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe3.bits.sw_peaking1_idx_p3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P3[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe3.bits.sw_peaking1_idx_p2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P2[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe4.bits.sw_peaking1_value_n2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N2[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe4.bits.sw_peaking1_value_n1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N1[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe5.bits.sw_peaking1_value_p1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P1[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe5.bits.sw_peaking1_value_n3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N3[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe6.bits.sw_peaking1_value_p3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P3[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe6.bits.sw_peaking1_value_p2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P2[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe7.bits.sw_peaking1_ratio_n12 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N12[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe7.bits.sw_peaking1_ratio_n01 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N01[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe8.bits.sw_peaking1_ratio_p01 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P01[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe8.bits.sw_peaking1_ratio_n23 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N23[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe9.bits.sw_peaking1_shoot_delta_offset =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_delta_offset[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe9.bits.sw_peaking1_ratio_p23 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P23[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe9.bits.sw_peaking1_ratio_p12 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P12[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe10.bits.sw_peaking1_shoot_alpha_under_unlimit =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under_unlimit[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe10.bits.sw_peaking1_shoot_alpha_over_unlimit =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over_unlimit[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe10.bits.sw_peaking1_shoot_alpha_under =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under[1];
	p_kernel_cfg->regs.sw_peaking1_ctrl_coe10.bits.sw_peaking1_shoot_alpha_over =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over[1];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe0.bits.sw_peaking2_idx_n1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N1[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe0.bits.sw_peaking2_idx_n0 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N0[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe1.bits.sw_peaking2_idx_n3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N3[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe1.bits.sw_peaking2_idx_n2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N2[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe2.bits.sw_peaking2_idx_p1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P1[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe2.bits.sw_peaking2_idx_p0 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P0[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe3.bits.sw_peaking2_idx_p3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P3[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe3.bits.sw_peaking2_idx_p2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P2[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe4.bits.sw_peaking2_value_n2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N2[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe4.bits.sw_peaking2_value_n1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N1[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe5.bits.sw_peaking2_value_p1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P1[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe5.bits.sw_peaking2_value_n3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N3[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe6.bits.sw_peaking2_value_p3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P3[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe6.bits.sw_peaking2_value_p2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P2[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe7.bits.sw_peaking2_ratio_n12 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N12[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe7.bits.sw_peaking2_ratio_n01 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N01[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe8.bits.sw_peaking2_ratio_p01 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P01[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe8.bits.sw_peaking2_ratio_n23 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N23[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe9.bits.sw_peaking2_shoot_delta_offset =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_delta_offset[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe9.bits.sw_peaking2_ratio_p23 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P23[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe9.bits.sw_peaking2_ratio_p12 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P12[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe10.bits.sw_peaking2_shoot_alpha_under_unlimit =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under_unlimit[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe10.bits.sw_peaking2_shoot_alpha_over_unlimit =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over_unlimit[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe10.bits.sw_peaking2_shoot_alpha_under =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under[2];
	p_kernel_cfg->regs.sw_peaking2_ctrl_coe10.bits.sw_peaking2_shoot_alpha_over =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over[2];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe0.bits.sw_peaking3_idx_n1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N1[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe0.bits.sw_peaking3_idx_n0 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N0[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe1.bits.sw_peaking3_idx_n3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N3[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe1.bits.sw_peaking3_idx_n2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N2[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe2.bits.sw_peaking3_idx_p1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P1[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe2.bits.sw_peaking3_idx_p0 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P0[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe3.bits.sw_peaking3_idx_p3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P3[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe3.bits.sw_peaking3_idx_p2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P2[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe4.bits.sw_peaking3_value_n2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N2[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe4.bits.sw_peaking3_value_n1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N1[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe5.bits.sw_peaking3_value_p1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P1[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe5.bits.sw_peaking3_value_n3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N3[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe6.bits.sw_peaking3_value_p3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P3[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe6.bits.sw_peaking3_value_p2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P2[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe7.bits.sw_peaking3_ratio_n12 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N12[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe7.bits.sw_peaking3_ratio_n01 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N01[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe8.bits.sw_peaking3_ratio_p01 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P01[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe8.bits.sw_peaking3_ratio_n23 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N23[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe9.bits.sw_peaking3_shoot_delta_offset =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_delta_offset[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe9.bits.sw_peaking3_ratio_p23 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P23[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe9.bits.sw_peaking3_ratio_p12 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P12[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe10.bits.sw_peaking3_shoot_alpha_under_unlimit =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under_unlimit[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe10.bits.sw_peaking3_shoot_alpha_over_unlimit =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over_unlimit[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe10.bits.sw_peaking3_shoot_alpha_under =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under[3];
	p_kernel_cfg->regs.sw_peaking3_ctrl_coe10.bits.sw_peaking3_shoot_alpha_over =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over[3];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe0.bits.sw_peaking4_idx_n1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N1[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe0.bits.sw_peaking4_idx_n0 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N0[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe1.bits.sw_peaking4_idx_n3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N3[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe1.bits.sw_peaking4_idx_n2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N2[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe2.bits.sw_peaking4_idx_p1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P1[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe2.bits.sw_peaking4_idx_p0 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P0[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe3.bits.sw_peaking4_idx_p3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P3[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe3.bits.sw_peaking4_idx_p2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P2[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe4.bits.sw_peaking4_value_n2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N2[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe4.bits.sw_peaking4_value_n1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N1[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe5.bits.sw_peaking4_value_p1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P1[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe5.bits.sw_peaking4_value_n3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N3[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe6.bits.sw_peaking4_value_p3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P3[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe6.bits.sw_peaking4_value_p2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P2[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe7.bits.sw_peaking4_ratio_n12 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N12[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe7.bits.sw_peaking4_ratio_n01 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N01[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe8.bits.sw_peaking4_ratio_p01 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P01[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe8.bits.sw_peaking4_ratio_n23 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N23[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe9.bits.sw_peaking4_shoot_delta_offset =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_delta_offset[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe9.bits.sw_peaking4_ratio_p23 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P23[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe9.bits.sw_peaking4_ratio_p12 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P12[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe10.bits.sw_peaking4_shoot_alpha_under_unlimit =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under_unlimit[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe10.bits.sw_peaking4_shoot_alpha_over_unlimit =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over_unlimit[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe10.bits.sw_peaking4_shoot_alpha_under =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under[4];
	p_kernel_cfg->regs.sw_peaking4_ctrl_coe10.bits.sw_peaking4_shoot_alpha_over =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over[4];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe0.bits.sw_peaking5_idx_n1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N1[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe0.bits.sw_peaking5_idx_n0 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N0[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe1.bits.sw_peaking5_idx_n3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N3[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe1.bits.sw_peaking5_idx_n2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N2[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe2.bits.sw_peaking5_idx_p1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P1[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe2.bits.sw_peaking5_idx_p0 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P0[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe3.bits.sw_peaking5_idx_p3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P3[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe3.bits.sw_peaking5_idx_p2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P2[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe4.bits.sw_peaking5_value_n2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N2[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe4.bits.sw_peaking5_value_n1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N1[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe5.bits.sw_peaking5_value_p1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P1[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe5.bits.sw_peaking5_value_n3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N3[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe6.bits.sw_peaking5_value_p3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P3[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe6.bits.sw_peaking5_value_p2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P2[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe7.bits.sw_peaking5_ratio_n12 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N12[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe7.bits.sw_peaking5_ratio_n01 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N01[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe8.bits.sw_peaking5_ratio_p01 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P01[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe8.bits.sw_peaking5_ratio_n23 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N23[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe9.bits.sw_peaking5_shoot_delta_offset =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_delta_offset[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe9.bits.sw_peaking5_ratio_p23 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P23[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe9.bits.sw_peaking5_ratio_p12 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P12[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe10.bits.sw_peaking5_shoot_alpha_under_unlimit =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under_unlimit[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe10.bits.sw_peaking5_shoot_alpha_over_unlimit =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over_unlimit[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe10.bits.sw_peaking5_shoot_alpha_under =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under[5];
	p_kernel_cfg->regs.sw_peaking5_ctrl_coe10.bits.sw_peaking5_shoot_alpha_over =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over[5];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe0.bits.sw_peaking6_idx_n1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N1[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe0.bits.sw_peaking6_idx_n0 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N0[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe1.bits.sw_peaking6_idx_n3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N3[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe1.bits.sw_peaking6_idx_n2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N2[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe2.bits.sw_peaking6_idx_p1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P1[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe2.bits.sw_peaking6_idx_p0 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P0[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe3.bits.sw_peaking6_idx_p3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P3[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe3.bits.sw_peaking6_idx_p2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P2[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe4.bits.sw_peaking6_value_n2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N2[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe4.bits.sw_peaking6_value_n1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N1[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe5.bits.sw_peaking6_value_p1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P1[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe5.bits.sw_peaking6_value_n3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N3[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe6.bits.sw_peaking6_value_p3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P3[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe6.bits.sw_peaking6_value_p2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P2[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe7.bits.sw_peaking6_ratio_n12 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N12[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe7.bits.sw_peaking6_ratio_n01 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N01[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe8.bits.sw_peaking6_ratio_p01 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P01[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe8.bits.sw_peaking6_ratio_n23 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N23[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe9.bits.sw_peaking6_shoot_delta_offset =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_delta_offset[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe9.bits.sw_peaking6_ratio_p23 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P23[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe9.bits.sw_peaking6_ratio_p12 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P12[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe10.bits.sw_peaking6_shoot_alpha_under_unlimit =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under_unlimit[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe10.bits.sw_peaking6_shoot_alpha_over_unlimit =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over_unlimit[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe10.bits.sw_peaking6_shoot_alpha_under =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under[6];
	p_kernel_cfg->regs.sw_peaking6_ctrl_coe10.bits.sw_peaking6_shoot_alpha_over =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over[6];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe0.bits.sw_peaking7_idx_n1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N1[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe0.bits.sw_peaking7_idx_n0 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N0[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe1.bits.sw_peaking7_idx_n3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N3[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe1.bits.sw_peaking7_idx_n2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_N2[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe2.bits.sw_peaking7_idx_p1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P1[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe2.bits.sw_peaking7_idx_p0 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P0[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe3.bits.sw_peaking7_idx_p3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P3[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe3.bits.sw_peaking7_idx_p2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_idx_P2[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe4.bits.sw_peaking7_value_n2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N2[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe4.bits.sw_peaking7_value_n1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N1[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe5.bits.sw_peaking7_value_p1 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P1[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe5.bits.sw_peaking7_value_n3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_N3[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe6.bits.sw_peaking7_value_p3 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P3[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe6.bits.sw_peaking7_value_p2 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_value_P2[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe7.bits.sw_peaking7_ratio_n12 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N12[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe7.bits.sw_peaking7_ratio_n01 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N01[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe8.bits.sw_peaking7_ratio_p01 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P01[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe8.bits.sw_peaking7_ratio_n23 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_N23[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe9.bits.sw_peaking7_shoot_delta_offset =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_delta_offset[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe9.bits.sw_peaking7_ratio_p23 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P23[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe9.bits.sw_peaking7_ratio_p12 =
		p_sharp_reg->peaking_cfg.peaking_ctrl_ratio_P12[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe10.bits.sw_peaking7_shoot_alpha_under_unlimit =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under_unlimit[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe10.bits.sw_peaking7_shoot_alpha_over_unlimit =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over_unlimit[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe10.bits.sw_peaking7_shoot_alpha_under =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_under[7];
	p_kernel_cfg->regs.sw_peaking7_ctrl_coe10.bits.sw_peaking7_shoot_alpha_over =
		p_sharp_reg->peaking_cfg.peaking_shtctrl_alpha_over[7];
	p_kernel_cfg->regs.sw_peaking_ctrl0.bits.sw_nondir_wgt_ratio =
		p_sharp_reg->peaking_cfg.edge_proc_cfg.non_dir_wgt_ratio;
	p_kernel_cfg->regs.sw_peaking_ctrl0.bits.sw_dir_cmp_ratio =
		p_sharp_reg->peaking_cfg.edge_proc_cfg.dir_cmp_ratio;
	p_kernel_cfg->regs.sw_peaking_ctrl0.bits.sw_nondir_thr =
		p_sharp_reg->peaking_cfg.edge_proc_cfg.non_dir_thr;
	p_kernel_cfg->regs.sw_peaking_ctrl0.bits.sw_peaking_gain =
		p_sharp_reg->peaking_cfg.peaking_gain;
	p_kernel_cfg->regs.sw_peaking_ctrl1.bits.sw_diag_dir_thr =
		p_sharp_reg->peaking_cfg.edge_proc_cfg.diag_dir_thr;
	p_kernel_cfg->regs.sw_peaking_ctrl1.bits.sw_dir_cnt_offset =
		p_sharp_reg->peaking_cfg.edge_proc_cfg.dir_cnt_offset;
	p_kernel_cfg->regs.sw_peaking_ctrl1.bits.sw_dir_cnt_avg =
		p_sharp_reg->peaking_cfg.edge_proc_cfg.dir_cnt_avg;
	p_kernel_cfg->regs.sw_peaking_ctrl1.bits.sw_dir_cnt_thr =
		p_sharp_reg->peaking_cfg.edge_proc_cfg.dir_cnt_thr;
	p_kernel_cfg->regs.sw_peaking_ctrl1.bits.sw_nondir_wgt_offset =
		p_sharp_reg->peaking_cfg.edge_proc_cfg.non_dir_wgt_offset;
	p_kernel_cfg->regs.sw_peaking_ctrl2.bits.sw_diag_adjgain_tab7 =
		p_sharp_reg->peaking_cfg.edge_proc_cfg.diag_adj_gain_tab[7];
	p_kernel_cfg->regs.sw_peaking_ctrl2.bits.sw_diag_adjgain_tab6 =
		p_sharp_reg->peaking_cfg.edge_proc_cfg.diag_adj_gain_tab[6];
	p_kernel_cfg->regs.sw_peaking_ctrl2.bits.sw_diag_adjgain_tab5 =
		p_sharp_reg->peaking_cfg.edge_proc_cfg.diag_adj_gain_tab[5];
	p_kernel_cfg->regs.sw_peaking_ctrl2.bits.sw_diag_adjgain_tab4 =
		p_sharp_reg->peaking_cfg.edge_proc_cfg.diag_adj_gain_tab[4];
	p_kernel_cfg->regs.sw_peaking_ctrl2.bits.sw_diag_adjgain_tab3 =
		p_sharp_reg->peaking_cfg.edge_proc_cfg.diag_adj_gain_tab[3];
	p_kernel_cfg->regs.sw_peaking_ctrl2.bits.sw_diag_adjgain_tab2 =
		p_sharp_reg->peaking_cfg.edge_proc_cfg.diag_adj_gain_tab[2];
	p_kernel_cfg->regs.sw_peaking_ctrl2.bits.sw_diag_adjgain_tab1 =
		p_sharp_reg->peaking_cfg.edge_proc_cfg.diag_adj_gain_tab[1];
	p_kernel_cfg->regs.sw_peaking_ctrl2.bits.sw_diag_adjgain_tab0 =
		p_sharp_reg->peaking_cfg.edge_proc_cfg.diag_adj_gain_tab[0];
	p_kernel_cfg->regs.sw_peaking_ctrl3.bits.sw_edge_alpha_under_unlimit_non =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_unlimit_non;
	p_kernel_cfg->regs.sw_peaking_ctrl3.bits.sw_edge_alpha_over_unlimit_non =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_unlimit_non;
	p_kernel_cfg->regs.sw_peaking_ctrl3.bits.sw_edge_alpha_under_non =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_non;
	p_kernel_cfg->regs.sw_peaking_ctrl3.bits.sw_edge_alpha_over_non =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_non;
	p_kernel_cfg->regs.sw_peaking_ctrl4.bits.sw_edge_alpha_under_unlimit_v =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_unlimit_v;
	p_kernel_cfg->regs.sw_peaking_ctrl4.bits.sw_edge_alpha_over_unlimit_v =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_unlimit_v;
	p_kernel_cfg->regs.sw_peaking_ctrl4.bits.sw_edge_alpha_under_v =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_v;
	p_kernel_cfg->regs.sw_peaking_ctrl4.bits.sw_edge_alpha_over_v =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_v;
	p_kernel_cfg->regs.sw_peaking_ctrl5.bits.sw_edge_alpha_under_unlimit_h =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_unlimit_h;
	p_kernel_cfg->regs.sw_peaking_ctrl5.bits.sw_edge_alpha_over_unlimit_h =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_unlimit_h;
	p_kernel_cfg->regs.sw_peaking_ctrl5.bits.sw_edge_alpha_under_h =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_h;
	p_kernel_cfg->regs.sw_peaking_ctrl5.bits.sw_edge_alpha_over_h =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_h;
	p_kernel_cfg->regs.sw_peaking_ctrl6.bits.sw_edge_alpha_under_unlimit_d0 =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_unlimit_d0;
	p_kernel_cfg->regs.sw_peaking_ctrl6.bits.sw_edge_alpha_over_unlimit_d0 =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_unlimit_d0;
	p_kernel_cfg->regs.sw_peaking_ctrl6.bits.sw_edge_alpha_under_d0 =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_d0;
	p_kernel_cfg->regs.sw_peaking_ctrl6.bits.sw_edge_alpha_over_d0 =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_d0;
	p_kernel_cfg->regs.sw_peaking_ctrl7.bits.sw_edge_alpha_under_unlimit_d1 =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_unlimit_d1;
	p_kernel_cfg->regs.sw_peaking_ctrl7.bits.sw_edge_alpha_over_unlimit_d1 =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_unlimit_d1;
	p_kernel_cfg->regs.sw_peaking_ctrl7.bits.sw_edge_alpha_under_d1 =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_under_d1;
	p_kernel_cfg->regs.sw_peaking_ctrl7.bits.sw_edge_alpha_over_d1 =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.alpha_over_d1;
	p_kernel_cfg->regs.sw_peaking_ctrl8.bits.sw_edge_delta_offset_h =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.delta_offset_h;
	p_kernel_cfg->regs.sw_peaking_ctrl8.bits.sw_edge_delta_offset_v =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.delta_offset_v;
	p_kernel_cfg->regs.sw_peaking_ctrl8.bits.sw_edge_delta_offset_non =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.delta_offset_non;
	p_kernel_cfg->regs.sw_peaking_ctrl9.bits.sw_edge_delta_offset_d1 =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.delta_offset_d1;
	p_kernel_cfg->regs.sw_peaking_ctrl9.bits.sw_edge_delta_offset_d0 =
		p_sharp_reg->peaking_cfg.edge_shoot_ctrl_cfg.delta_offset_d0;
	p_kernel_cfg->regs.sw_shoot_ctrl0.bits.sw_shoot_alpha_under =
		p_sharp_reg->shoot_ctrl_cfg.shoot_ctrl_alpha_under;
	p_kernel_cfg->regs.sw_shoot_ctrl0.bits.sw_shoot_alpha_over =
		p_sharp_reg->shoot_ctrl_cfg.shoot_ctrl_alpha_over;
	p_kernel_cfg->regs.sw_shoot_ctrl0.bits.sw_shoot_delta_offset =
		p_sharp_reg->shoot_ctrl_cfg.shoot_ctrl_delta_offset;
	p_kernel_cfg->regs.sw_shoot_ctrl0.bits.sw_shoot_filt_radius =
		p_sharp_reg->shoot_ctrl_cfg.shoot_ctrl_filt_radius;
	p_kernel_cfg->regs.sw_shoot_ctrl1.bits.sw_shoot_alpha_under_unlimit =
		p_sharp_reg->shoot_ctrl_cfg.shoot_ctrl_alpha_under_unlimit;
	p_kernel_cfg->regs.sw_shoot_ctrl1.bits.sw_shoot_alpha_over_unlimit =
		p_sharp_reg->shoot_ctrl_cfg.shoot_ctrl_alpha_over_unlimit;
	p_kernel_cfg->regs.sw_gain_ctrl0.bits.sw_adp_idx2 = p_sharp_reg->gain_ctrl_cfg.adp_idx2;
	p_kernel_cfg->regs.sw_gain_ctrl0.bits.sw_adp_idx1 = p_sharp_reg->gain_ctrl_cfg.adp_idx1;
	p_kernel_cfg->regs.sw_gain_ctrl0.bits.sw_adp_idx0 = p_sharp_reg->gain_ctrl_cfg.adp_idx0;
	p_kernel_cfg->regs.sw_gain_ctrl1.bits.sw_adp_gain1 = p_sharp_reg->gain_ctrl_cfg.adp_gain1;
	p_kernel_cfg->regs.sw_gain_ctrl1.bits.sw_adp_gain0 = p_sharp_reg->gain_ctrl_cfg.adp_gain0;
	p_kernel_cfg->regs.sw_gain_ctrl1.bits.sw_adp_idx3 = p_sharp_reg->gain_ctrl_cfg.adp_idx3;
	p_kernel_cfg->regs.sw_gain_ctrl2.bits.sw_adp_gain4 = p_sharp_reg->gain_ctrl_cfg.adp_gain4;
	p_kernel_cfg->regs.sw_gain_ctrl2.bits.sw_adp_gain3 = p_sharp_reg->gain_ctrl_cfg.adp_gain3;
	p_kernel_cfg->regs.sw_gain_ctrl2.bits.sw_adp_gain2 = p_sharp_reg->gain_ctrl_cfg.adp_gain2;
	p_kernel_cfg->regs.sw_gain_ctrl3.bits.sw_adp_slp12 = p_sharp_reg->gain_ctrl_cfg.adp_slp12;
	p_kernel_cfg->regs.sw_gain_ctrl3.bits.sw_adp_slp01 = p_sharp_reg->gain_ctrl_cfg.adp_slp01;
	p_kernel_cfg->regs.sw_gain_ctrl4.bits.sw_adp_slp34 = p_sharp_reg->gain_ctrl_cfg.adp_slp34;
	p_kernel_cfg->regs.sw_gain_ctrl4.bits.sw_adp_slp23 = p_sharp_reg->gain_ctrl_cfg.adp_slp23;
	p_kernel_cfg->regs.sw_gain_ctrl5.bits.sw_var_idx1 = p_sharp_reg->gain_ctrl_cfg.var_idx1;
	p_kernel_cfg->regs.sw_gain_ctrl5.bits.sw_var_idx0 = p_sharp_reg->gain_ctrl_cfg.var_idx0;
	p_kernel_cfg->regs.sw_gain_ctrl5.bits.sw_adp_slp45 = p_sharp_reg->gain_ctrl_cfg.adp_slp45;
	p_kernel_cfg->regs.sw_gain_ctrl6.bits.sw_var_gain0 = p_sharp_reg->gain_ctrl_cfg.var_gain0;
	p_kernel_cfg->regs.sw_gain_ctrl6.bits.sw_var_idx3 = p_sharp_reg->gain_ctrl_cfg.var_idx3;
	p_kernel_cfg->regs.sw_gain_ctrl6.bits.sw_var_idx2 = p_sharp_reg->gain_ctrl_cfg.var_idx2;
	p_kernel_cfg->regs.sw_gain_ctrl7.bits.sw_var_gain4 = p_sharp_reg->gain_ctrl_cfg.var_gain4;
	p_kernel_cfg->regs.sw_gain_ctrl7.bits.sw_var_gain3 = p_sharp_reg->gain_ctrl_cfg.var_gain3;
	p_kernel_cfg->regs.sw_gain_ctrl7.bits.sw_var_gain2 = p_sharp_reg->gain_ctrl_cfg.var_gain2;
	p_kernel_cfg->regs.sw_gain_ctrl7.bits.sw_var_gain1 = p_sharp_reg->gain_ctrl_cfg.var_gain1;
	p_kernel_cfg->regs.sw_gain_ctrl8.bits.sw_var_slp12 = p_sharp_reg->gain_ctrl_cfg.var_slp12;
	p_kernel_cfg->regs.sw_gain_ctrl8.bits.sw_var_slp01 = p_sharp_reg->gain_ctrl_cfg.var_slp01;
	p_kernel_cfg->regs.sw_gain_ctrl9.bits.sw_var_slp34 = p_sharp_reg->gain_ctrl_cfg.var_slp34;
	p_kernel_cfg->regs.sw_gain_ctrl9.bits.sw_var_slp23 = p_sharp_reg->gain_ctrl_cfg.var_slp23;
	p_kernel_cfg->regs.sw_gain_ctrl10.bits.sw_lum_idx0 = p_sharp_reg->gain_ctrl_cfg.lum_idx0;
	p_kernel_cfg->regs.sw_gain_ctrl10.bits.sw_lum_select =
		p_sharp_reg->gain_ctrl_cfg.lum_select;
	p_kernel_cfg->regs.sw_gain_ctrl10.bits.sw_var_slp45 = p_sharp_reg->gain_ctrl_cfg.var_slp45;
	p_kernel_cfg->regs.sw_gain_ctrl11.bits.sw_lum_idx3 = p_sharp_reg->gain_ctrl_cfg.lum_idx3;
	p_kernel_cfg->regs.sw_gain_ctrl11.bits.sw_lum_idx2 = p_sharp_reg->gain_ctrl_cfg.lum_idx2;
	p_kernel_cfg->regs.sw_gain_ctrl11.bits.sw_lum_idx1 = p_sharp_reg->gain_ctrl_cfg.lum_idx1;
	p_kernel_cfg->regs.sw_gain_ctrl12.bits.sw_lum_gain3 = p_sharp_reg->gain_ctrl_cfg.lum_gain3;
	p_kernel_cfg->regs.sw_gain_ctrl12.bits.sw_lum_gain2 = p_sharp_reg->gain_ctrl_cfg.lum_gain2;
	p_kernel_cfg->regs.sw_gain_ctrl12.bits.sw_lum_gain1 = p_sharp_reg->gain_ctrl_cfg.lum_gain1;
	p_kernel_cfg->regs.sw_gain_ctrl12.bits.sw_lum_gain0 = p_sharp_reg->gain_ctrl_cfg.lum_gain0;
	p_kernel_cfg->regs.sw_gain_ctrl13.bits.sw_lum_slp12 = p_sharp_reg->gain_ctrl_cfg.lum_slp12;
	p_kernel_cfg->regs.sw_gain_ctrl13.bits.sw_lum_slp01 = p_sharp_reg->gain_ctrl_cfg.lum_slp01;
	p_kernel_cfg->regs.sw_gain_ctrl13.bits.sw_lum_gain4 = p_sharp_reg->gain_ctrl_cfg.lum_gain4;
	p_kernel_cfg->regs.sw_gain_ctrl14.bits.sw_lum_slp34 = p_sharp_reg->gain_ctrl_cfg.lum_slp34;
	p_kernel_cfg->regs.sw_gain_ctrl14.bits.sw_lum_slp23 = p_sharp_reg->gain_ctrl_cfg.lum_slp23;
	p_kernel_cfg->regs.sw_gain_ctrl15.bits.sw_lum_slp45 = p_sharp_reg->gain_ctrl_cfg.lum_slp45;
	p_kernel_cfg->regs.sw_coloradj_ctrl0.bits.sw_adj_scaling_coef0 =
		p_sharp_reg->color_adj_params[0].adj_scaling_coef;
	p_kernel_cfg->regs.sw_coloradj_ctrl0.bits.sw_adj_point_y0 =
		p_sharp_reg->color_adj_params[0].adj_point_y;
	p_kernel_cfg->regs.sw_coloradj_ctrl0.bits.sw_adj_point_x0 =
		p_sharp_reg->color_adj_params[0].adj_point_x;
	p_kernel_cfg->regs.sw_coloradj_ctrl1.bits.sw_coloradj_tab0_5 =
		p_sharp_reg->color_adj_params[0].color_adj_tab[5];
	p_kernel_cfg->regs.sw_coloradj_ctrl1.bits.sw_coloradj_tab0_4 =
		p_sharp_reg->color_adj_params[0].color_adj_tab[4];
	p_kernel_cfg->regs.sw_coloradj_ctrl1.bits.sw_coloradj_tab0_3 =
		p_sharp_reg->color_adj_params[0].color_adj_tab[3];
	p_kernel_cfg->regs.sw_coloradj_ctrl1.bits.sw_coloradj_tab0_2 =
		p_sharp_reg->color_adj_params[0].color_adj_tab[2];
	p_kernel_cfg->regs.sw_coloradj_ctrl1.bits.sw_coloradj_tab0_1 =
		p_sharp_reg->color_adj_params[0].color_adj_tab[1];
	p_kernel_cfg->regs.sw_coloradj_ctrl1.bits.sw_coloradj_tab0_0 =
		p_sharp_reg->color_adj_params[0].color_adj_tab[0];
	p_kernel_cfg->regs.sw_coloradj_ctrl2.bits.sw_coloradj_tab0_11 =
		p_sharp_reg->color_adj_params[0].color_adj_tab[11];
	p_kernel_cfg->regs.sw_coloradj_ctrl2.bits.sw_coloradj_tab0_10 =
		p_sharp_reg->color_adj_params[0].color_adj_tab[10];
	p_kernel_cfg->regs.sw_coloradj_ctrl2.bits.sw_coloradj_tab0_9 =
		p_sharp_reg->color_adj_params[0].color_adj_tab[9];
	p_kernel_cfg->regs.sw_coloradj_ctrl2.bits.sw_coloradj_tab0_8 =
		p_sharp_reg->color_adj_params[0].color_adj_tab[8];
	p_kernel_cfg->regs.sw_coloradj_ctrl2.bits.sw_coloradj_tab0_7 =
		p_sharp_reg->color_adj_params[0].color_adj_tab[7];
	p_kernel_cfg->regs.sw_coloradj_ctrl2.bits.sw_coloradj_tab0_6 =
		p_sharp_reg->color_adj_params[0].color_adj_tab[6];
	p_kernel_cfg->regs.sw_coloradj_ctrl3.bits.sw_coloradj_tab0_15 =
		p_sharp_reg->color_adj_params[0].color_adj_tab[15];
	p_kernel_cfg->regs.sw_coloradj_ctrl3.bits.sw_coloradj_tab0_14 =
		p_sharp_reg->color_adj_params[0].color_adj_tab[14];
	p_kernel_cfg->regs.sw_coloradj_ctrl3.bits.sw_coloradj_tab0_13 =
		p_sharp_reg->color_adj_params[0].color_adj_tab[13];
	p_kernel_cfg->regs.sw_coloradj_ctrl3.bits.sw_coloradj_tab0_12 =
		p_sharp_reg->color_adj_params[0].color_adj_tab[12];
	p_kernel_cfg->regs.sw_coloradj_ctrl4.bits.sw_adj_scaling_coef1 =
		p_sharp_reg->color_adj_params[1].adj_scaling_coef;
	p_kernel_cfg->regs.sw_coloradj_ctrl4.bits.sw_adj_point_y1 =
		p_sharp_reg->color_adj_params[1].adj_point_y;
	p_kernel_cfg->regs.sw_coloradj_ctrl4.bits.sw_adj_point_x1 =
		p_sharp_reg->color_adj_params[1].adj_point_x;
	p_kernel_cfg->regs.sw_coloradj_ctrl5.bits.sw_coloradj_tab1_5 =
		p_sharp_reg->color_adj_params[1].color_adj_tab[5];
	p_kernel_cfg->regs.sw_coloradj_ctrl5.bits.sw_coloradj_tab1_4 =
		p_sharp_reg->color_adj_params[1].color_adj_tab[4];
	p_kernel_cfg->regs.sw_coloradj_ctrl5.bits.sw_coloradj_tab1_3 =
		p_sharp_reg->color_adj_params[1].color_adj_tab[3];
	p_kernel_cfg->regs.sw_coloradj_ctrl5.bits.sw_coloradj_tab1_2 =
		p_sharp_reg->color_adj_params[1].color_adj_tab[2];
	p_kernel_cfg->regs.sw_coloradj_ctrl5.bits.sw_coloradj_tab1_1 =
		p_sharp_reg->color_adj_params[1].color_adj_tab[1];
	p_kernel_cfg->regs.sw_coloradj_ctrl5.bits.sw_coloradj_tab1_0 =
		p_sharp_reg->color_adj_params[1].color_adj_tab[0];
	p_kernel_cfg->regs.sw_coloradj_ctrl6.bits.sw_coloradj_tab1_11 =
		p_sharp_reg->color_adj_params[1].color_adj_tab[11];
	p_kernel_cfg->regs.sw_coloradj_ctrl6.bits.sw_coloradj_tab1_10 =
		p_sharp_reg->color_adj_params[1].color_adj_tab[10];
	p_kernel_cfg->regs.sw_coloradj_ctrl6.bits.sw_coloradj_tab1_9 =
		p_sharp_reg->color_adj_params[1].color_adj_tab[9];
	p_kernel_cfg->regs.sw_coloradj_ctrl6.bits.sw_coloradj_tab1_8 =
		p_sharp_reg->color_adj_params[1].color_adj_tab[8];
	p_kernel_cfg->regs.sw_coloradj_ctrl6.bits.sw_coloradj_tab1_7 =
		p_sharp_reg->color_adj_params[1].color_adj_tab[7];
	p_kernel_cfg->regs.sw_coloradj_ctrl6.bits.sw_coloradj_tab1_6 =
		p_sharp_reg->color_adj_params[1].color_adj_tab[6];
	p_kernel_cfg->regs.sw_coloradj_ctrl7.bits.sw_coloradj_tab1_15 =
		p_sharp_reg->color_adj_params[1].color_adj_tab[15];
	p_kernel_cfg->regs.sw_coloradj_ctrl7.bits.sw_coloradj_tab1_14 =
		p_sharp_reg->color_adj_params[1].color_adj_tab[14];
	p_kernel_cfg->regs.sw_coloradj_ctrl7.bits.sw_coloradj_tab1_13 =
		p_sharp_reg->color_adj_params[1].color_adj_tab[13];
	p_kernel_cfg->regs.sw_coloradj_ctrl7.bits.sw_coloradj_tab1_12 =
		p_sharp_reg->color_adj_params[1].color_adj_tab[12];
	p_kernel_cfg->regs.sw_coloradj_ctrl8.bits.sw_adj_scaling_coef2 =
		p_sharp_reg->color_adj_params[2].adj_scaling_coef;
	p_kernel_cfg->regs.sw_coloradj_ctrl8.bits.sw_adj_point_y2 =
		p_sharp_reg->color_adj_params[2].adj_point_y;
	p_kernel_cfg->regs.sw_coloradj_ctrl8.bits.sw_adj_point_x2 =
		p_sharp_reg->color_adj_params[2].adj_point_x;
	p_kernel_cfg->regs.sw_coloradj_ctrl9.bits.sw_coloradj_tab2_5 =
		p_sharp_reg->color_adj_params[2].color_adj_tab[5];
	p_kernel_cfg->regs.sw_coloradj_ctrl9.bits.sw_coloradj_tab2_4 =
		p_sharp_reg->color_adj_params[2].color_adj_tab[4];
	p_kernel_cfg->regs.sw_coloradj_ctrl9.bits.sw_coloradj_tab2_3 =
		p_sharp_reg->color_adj_params[2].color_adj_tab[3];
	p_kernel_cfg->regs.sw_coloradj_ctrl9.bits.sw_coloradj_tab2_2 =
		p_sharp_reg->color_adj_params[2].color_adj_tab[2];
	p_kernel_cfg->regs.sw_coloradj_ctrl9.bits.sw_coloradj_tab2_1 =
		p_sharp_reg->color_adj_params[2].color_adj_tab[1];
	p_kernel_cfg->regs.sw_coloradj_ctrl9.bits.sw_coloradj_tab2_0 =
		p_sharp_reg->color_adj_params[2].color_adj_tab[0];
	p_kernel_cfg->regs.sw_coloradj_ctrl10.bits.sw_coloradj_tab2_11 =
		p_sharp_reg->color_adj_params[2].color_adj_tab[11];
	p_kernel_cfg->regs.sw_coloradj_ctrl10.bits.sw_coloradj_tab2_10 =
		p_sharp_reg->color_adj_params[2].color_adj_tab[10];
	p_kernel_cfg->regs.sw_coloradj_ctrl10.bits.sw_coloradj_tab2_9 =
		p_sharp_reg->color_adj_params[2].color_adj_tab[9];
	p_kernel_cfg->regs.sw_coloradj_ctrl10.bits.sw_coloradj_tab2_8 =
		p_sharp_reg->color_adj_params[2].color_adj_tab[8];
	p_kernel_cfg->regs.sw_coloradj_ctrl10.bits.sw_coloradj_tab2_7 =
		p_sharp_reg->color_adj_params[2].color_adj_tab[7];
	p_kernel_cfg->regs.sw_coloradj_ctrl10.bits.sw_coloradj_tab2_6 =
		p_sharp_reg->color_adj_params[2].color_adj_tab[6];
	p_kernel_cfg->regs.sw_coloradj_ctrl11.bits.sw_coloradj_tab2_15 =
		p_sharp_reg->color_adj_params[2].color_adj_tab[15];
	p_kernel_cfg->regs.sw_coloradj_ctrl11.bits.sw_coloradj_tab2_14 =
		p_sharp_reg->color_adj_params[2].color_adj_tab[14];
	p_kernel_cfg->regs.sw_coloradj_ctrl11.bits.sw_coloradj_tab2_13 =
		p_sharp_reg->color_adj_params[2].color_adj_tab[13];
	p_kernel_cfg->regs.sw_coloradj_ctrl11.bits.sw_coloradj_tab2_12 =
		p_sharp_reg->color_adj_params[2].color_adj_tab[12];
	p_kernel_cfg->regs.sw_coloradj_ctrl12.bits.sw_adj_scaling_coef3 =
		p_sharp_reg->color_adj_params[3].adj_scaling_coef;
	p_kernel_cfg->regs.sw_coloradj_ctrl12.bits.sw_adj_point_y3 =
		p_sharp_reg->color_adj_params[3].adj_point_y;
	p_kernel_cfg->regs.sw_coloradj_ctrl12.bits.sw_adj_point_x3 =
		p_sharp_reg->color_adj_params[3].adj_point_x;
	p_kernel_cfg->regs.sw_coloradj_ctrl13.bits.sw_coloradj_tab3_5 =
		p_sharp_reg->color_adj_params[3].color_adj_tab[5];
	p_kernel_cfg->regs.sw_coloradj_ctrl13.bits.sw_coloradj_tab3_4 =
		p_sharp_reg->color_adj_params[3].color_adj_tab[4];
	p_kernel_cfg->regs.sw_coloradj_ctrl13.bits.sw_coloradj_tab3_3 =
		p_sharp_reg->color_adj_params[3].color_adj_tab[3];
	p_kernel_cfg->regs.sw_coloradj_ctrl13.bits.sw_coloradj_tab3_2 =
		p_sharp_reg->color_adj_params[3].color_adj_tab[2];
	p_kernel_cfg->regs.sw_coloradj_ctrl13.bits.sw_coloradj_tab3_1 =
		p_sharp_reg->color_adj_params[3].color_adj_tab[1];
	p_kernel_cfg->regs.sw_coloradj_ctrl13.bits.sw_coloradj_tab3_0 =
		p_sharp_reg->color_adj_params[3].color_adj_tab[0];
	p_kernel_cfg->regs.sw_coloradj_ctrl14.bits.sw_coloradj_tab3_11 =
		p_sharp_reg->color_adj_params[3].color_adj_tab[11];
	p_kernel_cfg->regs.sw_coloradj_ctrl14.bits.sw_coloradj_tab3_10 =
		p_sharp_reg->color_adj_params[3].color_adj_tab[10];
	p_kernel_cfg->regs.sw_coloradj_ctrl14.bits.sw_coloradj_tab3_9 =
		p_sharp_reg->color_adj_params[3].color_adj_tab[9];
	p_kernel_cfg->regs.sw_coloradj_ctrl14.bits.sw_coloradj_tab3_8 =
		p_sharp_reg->color_adj_params[3].color_adj_tab[8];
	p_kernel_cfg->regs.sw_coloradj_ctrl14.bits.sw_coloradj_tab3_7 =
		p_sharp_reg->color_adj_params[3].color_adj_tab[7];
	p_kernel_cfg->regs.sw_coloradj_ctrl14.bits.sw_coloradj_tab3_6 =
		p_sharp_reg->color_adj_params[3].color_adj_tab[6];
	p_kernel_cfg->regs.sw_coloradj_ctrl15.bits.sw_coloradj_tab3_15 =
		p_sharp_reg->color_adj_params[3].color_adj_tab[15];
	p_kernel_cfg->regs.sw_coloradj_ctrl15.bits.sw_coloradj_tab3_14 =
		p_sharp_reg->color_adj_params[3].color_adj_tab[14];
	p_kernel_cfg->regs.sw_coloradj_ctrl15.bits.sw_coloradj_tab3_13 =
		p_sharp_reg->color_adj_params[3].color_adj_tab[13];
	p_kernel_cfg->regs.sw_coloradj_ctrl15.bits.sw_coloradj_tab3_12 =
		p_sharp_reg->color_adj_params[3].color_adj_tab[12];
	p_kernel_cfg->regs.sw_texture_ctrl0.bits.sw_tex_idx1 =
		p_sharp_reg->texture_adj_cfg.tex_idx1;
	p_kernel_cfg->regs.sw_texture_ctrl0.bits.sw_tex_idx0 =
		p_sharp_reg->texture_adj_cfg.tex_idx0;
	p_kernel_cfg->regs.sw_texture_ctrl0.bits.sw_ymode_select =
		p_sharp_reg->texture_adj_cfg.Y_mode_select;
	p_kernel_cfg->regs.sw_texture_ctrl0.bits.sw_idxmode_select =
		p_sharp_reg->texture_adj_cfg.idx_mode_select;
	p_kernel_cfg->regs.sw_texture_ctrl1.bits.sw_tex_gain0 =
		p_sharp_reg->texture_adj_cfg.tex_gain0;
	p_kernel_cfg->regs.sw_texture_ctrl1.bits.sw_tex_idx3 =
		p_sharp_reg->texture_adj_cfg.tex_idx3;
	p_kernel_cfg->regs.sw_texture_ctrl1.bits.sw_tex_idx2 =
		p_sharp_reg->texture_adj_cfg.tex_idx2;
	p_kernel_cfg->regs.sw_texture_ctrl2.bits.sw_tex_gain4 =
		p_sharp_reg->texture_adj_cfg.tex_gain4;
	p_kernel_cfg->regs.sw_texture_ctrl2.bits.sw_tex_gain3 =
		p_sharp_reg->texture_adj_cfg.tex_gain3;
	p_kernel_cfg->regs.sw_texture_ctrl2.bits.sw_tex_gain2 =
		p_sharp_reg->texture_adj_cfg.tex_gain2;
	p_kernel_cfg->regs.sw_texture_ctrl2.bits.sw_tex_gain1 =
		p_sharp_reg->texture_adj_cfg.tex_gain1;
	p_kernel_cfg->regs.sw_texture_ctrl3.bits.sw_tex_slp12 =
		p_sharp_reg->texture_adj_cfg.tex_slp12;
	p_kernel_cfg->regs.sw_texture_ctrl3.bits.sw_tex_slp01 =
		p_sharp_reg->texture_adj_cfg.tex_slp01;
	p_kernel_cfg->regs.sw_texture_ctrl4.bits.sw_tex_slp34 =
		p_sharp_reg->texture_adj_cfg.tex_slp34;
	p_kernel_cfg->regs.sw_texture_ctrl4.bits.sw_tex_slp23 =
		p_sharp_reg->texture_adj_cfg.tex_slp23;
	p_kernel_cfg->regs.sw_texture_ctrl5.bits.sw_tex_slp45 =
		p_sharp_reg->texture_adj_cfg.tex_slp45;
	p_kernel_cfg->regs.sw_lti_ctrl0.bits.sw_ltih_thr1 = p_sharp_reg->lti_h_cfg.thr1;
	p_kernel_cfg->regs.sw_lti_ctrl0.bits.sw_ltih_slp1 = p_sharp_reg->lti_h_cfg.slp1;
	p_kernel_cfg->regs.sw_lti_ctrl0.bits.sw_ltih_radius = p_sharp_reg->lti_h_cfg.radius;
	p_kernel_cfg->regs.sw_lti_ctrl1.bits.sw_ltih_tigain = p_sharp_reg->lti_h_cfg.tiGain;
	p_kernel_cfg->regs.sw_lti_ctrl1.bits.sw_ltih_noisethrpos =
		p_sharp_reg->lti_h_cfg.noiseThrPos;
	p_kernel_cfg->regs.sw_lti_ctrl1.bits.sw_ltih_noisethrneg =
		p_sharp_reg->lti_h_cfg.noiseThrNeg;
	p_kernel_cfg->regs.sw_lti_ctrl2.bits.sw_ltiv_thr1 = p_sharp_reg->lti_v_cfg.thr1;
	p_kernel_cfg->regs.sw_lti_ctrl2.bits.sw_ltiv_slp1 = p_sharp_reg->lti_v_cfg.slp1;
	p_kernel_cfg->regs.sw_lti_ctrl2.bits.sw_ltiv_radius = p_sharp_reg->lti_v_cfg.radius;
	p_kernel_cfg->regs.sw_lti_ctrl3.bits.sw_ltiv_tigain = p_sharp_reg->lti_v_cfg.tiGain;
	p_kernel_cfg->regs.sw_lti_ctrl3.bits.sw_ltiv_noisethrpos =
		p_sharp_reg->lti_v_cfg.noiseThrPos;
	p_kernel_cfg->regs.sw_lti_ctrl3.bits.sw_ltiv_noisethrneg =
		p_sharp_reg->lti_v_cfg.noiseThrNeg;
	p_kernel_cfg->regs.sw_cti_ctrl0.bits.sw_ctih_thr1 = p_sharp_reg->cti_h_cfg.thr1;
	p_kernel_cfg->regs.sw_cti_ctrl0.bits.sw_ctih_slp1 = p_sharp_reg->cti_h_cfg.slp1;
	p_kernel_cfg->regs.sw_cti_ctrl0.bits.sw_ctih_radius = p_sharp_reg->cti_h_cfg.radius;
	p_kernel_cfg->regs.sw_cti_ctrl1.bits.sw_ctih_tigain = p_sharp_reg->cti_h_cfg.tiGain;
	p_kernel_cfg->regs.sw_cti_ctrl1.bits.sw_ctih_noisethrpos =
		p_sharp_reg->cti_h_cfg.noiseThrPos;
	p_kernel_cfg->regs.sw_cti_ctrl1.bits.sw_ctih_noisethrneg =
		p_sharp_reg->cti_h_cfg.noiseThrNeg;
	p_kernel_cfg->regs.sw_cti_ctrl2.bits.sw_ctiv_thr1 = p_sharp_reg->cti_v_cfg.thr1;
	p_kernel_cfg->regs.sw_cti_ctrl2.bits.sw_ctiv_slp1 = p_sharp_reg->cti_v_cfg.slp1;
	p_kernel_cfg->regs.sw_cti_ctrl2.bits.sw_ctiv_radius = p_sharp_reg->cti_v_cfg.radius;
	p_kernel_cfg->regs.sw_cti_ctrl3.bits.sw_ctiv_tigain = p_sharp_reg->cti_v_cfg.tiGain;
	p_kernel_cfg->regs.sw_cti_ctrl3.bits.sw_ctiv_noisethrpos =
		p_sharp_reg->cti_v_cfg.noiseThrPos;
	p_kernel_cfg->regs.sw_cti_ctrl3.bits.sw_ctiv_noisethrneg =
		p_sharp_reg->cti_v_cfg.noiseThrNeg;
}
