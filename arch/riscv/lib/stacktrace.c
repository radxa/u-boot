// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026 Rockchip Electronics Co., Ltd.
 */

#include <common.h>
#include <config.h>
#include <stacktrace.h>
#include <timestamp.h>
#include <version.h>
#include <version_string.h>
#include <asm/global_data.h>
#include <asm/ptrace.h>

DECLARE_GLOBAL_DATA_PTR;

void show_stacktrace_header(void)
{
	printf("=== Backtrace ===\n");
#if defined(CONFIG_TPL_BUILD)
	printf("U-Boot TPL " PLAIN_VERSION " (" U_BOOT_DATE " - " U_BOOT_TIME ")\n");
#elif defined(CONFIG_SPL_BUILD)
	printf("U-Boot SPL " PLAIN_VERSION " (" U_BOOT_DATE " - " U_BOOT_TIME ")\n");
#else
	printf("%s\n", version_string);
#endif
}

struct stackframe {
	ulong fp;
	ulong pc;
};

static ulong get_stack_top(void)
{
	/*
	 * SPL: gd->start_addr_sp is not initialized if:
	 * CONFIG_SPL_SKIP_RELOCATE=y && !CONFIG_IS_ENABLED(SYS_REPORT_STACK_F_USAGE)
	 */
	ulong stack_top = gd ? gd->start_addr_sp : 0;

	if (!stack_top) {
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_STACK)
		stack_top = CONFIG_SPL_STACK;
#elif defined(CONFIG_TPL_BUILD) && defined(CONFIG_TPL_STACK)
		stack_top = CONFIG_TPL_STACK;
#elif defined(SYS_INIT_SP_ADDR)
		stack_top = SYS_INIT_SP_ADDR;
#endif
	}

	return stack_top;
}

static __always_inline ulong read_fp(void)
{
	ulong val;

	asm volatile("mv %0, s0" : "=r" (val));
	return val;
}

static __always_inline ulong read_ra(void)
{
	ulong val;

	asm volatile("mv %0, ra" : "=r" (val));
	return val;
}

static __always_inline ulong read_pc(void)
{
	ulong val;

	asm volatile("auipc %0, 0" : "=r" (val));
	return val;
}

static __always_inline ulong read_sp(void)
{
	ulong val;

	asm volatile("mv %0, sp" : "=r" (val));
	return val;
}

static bool walk_stackframe(struct stackframe *frame, ulong stack_bottom,
				    ulong stack_top)
{
	ulong next_fp;
	ulong fp = frame->fp;
	ulong *frame_data = (ulong *)fp;

	if (fp & (sizeof(ulong) - 1) ||
	    fp < stack_bottom + 2 * sizeof(ulong) || fp > stack_top)
		return false;

	next_fp = frame_data[-2];
	if (next_fp <= fp)
		return false;

	frame->pc = frame_data[-1];
	frame->fp = next_fp;

	return true;
}

static ulong relocate_address(ulong address)
{
	if (gd && gd->flags & GD_FLG_RELOC)
		return address - gd->reloc_off;

	return address;
}

void dump_core_stack(struct pt_regs *regs)
{
	ulong stack_top = get_stack_top();
	ulong stack_bottom = stack_top ? stack_top - CONFIG_STACK_SIZE : 0;
	struct stackframe frame = {
		.fp = regs->s0,
		.pc = regs->sepc,
	};
#if defined(CONFIG_TPL_BUILD)
	const char *build = "tpl";
#elif defined(CONFIG_SPL_BUILD)
	const char *build = "spl";
#else
	const char *build = "";
#endif

	printf("\nCall trace:\n");
	printf("  PC:\t[< " REG_FMT " >]\n", relocate_address(frame.pc));
	printf("  LR:\t[< " REG_FMT " >]\n", relocate_address(regs->ra));

	printf("\nStack:\n");
	do {
		printf("\t[< " REG_FMT " >]\n", relocate_address(frame.pc));
	} while (stack_top && walk_stackframe(&frame, stack_bottom, stack_top));

	printf("\nCopy info from \"=== Backtrace ===...\" to a file(eg. dump.txt), and run\n"
	       "command in your U-Boot project: "
	       "./scripts/stacktrace.sh dump.txt %s\n\n", build);
}

void dump_stack(void)
{
	/*
	 * Do not zero-initialize 'regs = {0}': memset() may be called before read_ra(),
	 * overwriting ra with the return address inside dump_stack().
	 * dump_core_stack() only reads the fields initialized below.
	 */
	struct pt_regs regs;

	regs.s0 = read_fp();
	regs.ra = read_ra(); /* lr */
	regs.sp = read_sp();
	regs.sepc = read_pc();

	show_stacktrace_header();
	dump_core_stack(&regs);
}
