/*
 * Copyright (C) 2017 Rockchip Electronic Co.,Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <linux/linkage.h>
#include <asm/assembler.h>

#if !defined(__aarch64__)
/* ARM32 (AArch32) */
ENTRY(spl_optee_entry)
    mov lr, r1
    mov pc, r3
ENDPROC(spl_optee_entry)
#else
/* ARM64 (AArch64) */
ENTRY(spl_optee_entry)
    mov x30, x1
    br  x3
ENDPROC(spl_optee_entry)
#endif