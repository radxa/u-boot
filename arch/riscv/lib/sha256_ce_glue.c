// SPDX-License-Identifier: GPL-2.0-only
/*
 * sha256_ce_glue.c - SHA-256 secure hash using RISCV Vector Extensions
 *
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd
 */

#include <u-boot/sha256.h>

extern void sha256_transform_zvknha_or_zvknhb_zvkb(uint32_t state[8], uint8_t const *src,
						   uint32_t blocks);

void sha256_process(sha256_context *ctx, const unsigned char *data,
		    unsigned int blocks)
{
	if (!blocks)
		return;

	sha256_transform_zvknha_or_zvknhb_zvkb(ctx->state, data, blocks);
}
