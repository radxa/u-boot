/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <console.h>
#include <hang.h>
#include <asm/arch-rockchip/smccc.h>
#include <asm/io.h>
#ifdef CONFIG_ARM
#include <asm/psci.h>
#ifdef CONFIG_ARM_CPU_SUSPEND
#include <asm/suspend.h>
#endif
#endif
#include <linux/io.h>

#define SIZE_PAGE(n)	((n) << 12)

#ifdef CONFIG_ARM
#ifdef CONFIG_ARM64
#define ARM_PSCI_1_0_SYSTEM_SUSPEND	ARM_PSCI_1_0_FN64_SYSTEM_SUSPEND
#define ARM_PSCI_0_2_CPU_ON		ARM_PSCI_0_2_FN64_CPU_ON
#define ARM_PSCI_0_2_CPU_OFF		ARM_PSCI_0_2_FN_CPU_OFF
#else
#define ARM_PSCI_1_0_SYSTEM_SUSPEND	ARM_PSCI_1_0_FN_SYSTEM_SUSPEND
#define ARM_PSCI_0_2_CPU_ON		ARM_PSCI_0_2_FN_CPU_ON
#define ARM_PSCI_0_2_CPU_OFF		ARM_PSCI_0_2_FN_CPU_OFF
#endif

static struct arm_smccc_res __invoke_sip_fn_smc6(unsigned long function_id,
						 unsigned long arg0,
						 unsigned long arg1,
						 unsigned long arg2,
						 unsigned long arg3,
						 unsigned long arg4,
						 unsigned long arg5)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, arg0, arg1, arg2, arg3, arg4, arg5, 0, &res);

	return res;
}

static struct arm_smccc_res __invoke_sip_fn_smc(unsigned long function_id,
						unsigned long arg0,
						unsigned long arg1,
						unsigned long arg2)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, arg0, arg1, arg2, 0, 0, 0, 0, &res);
	return res;
}

#ifdef CONFIG_ARM_CPU_SUSPEND
int psci_system_suspend(unsigned long unused)
{
	struct arm_smccc_res res;

	res = __invoke_sip_fn_smc(ARM_PSCI_1_0_SYSTEM_SUSPEND,
				  virt_to_phys(cpu_resume), 0, 0);
	return res.a0;
}
#endif

#else /* CONFIG_RISCV */
static int sbi_ext_vendor_id;

/*
 * RISC-V: map ARM SMC function IDs to Rockchip vendor SBI FIDs.
 * SBI returns {error, value} (2 registers); ARM SMC returns {a0-a3}
 * (4 registers).  We map error->a0, value->a1; a2/a3 are always 0 on
 * RISC-V.
 */
static inline ulong sip_fid_to_sbi(ulong function_id)
{
	return function_id & ~0xff000000;
}

static struct arm_smccc_res sbi_sip_ecall(int ext, int fid,
					  unsigned long arg0,
					  unsigned long arg1,
					  unsigned long arg2,
					  unsigned long arg3,
					  unsigned long arg4,
					  unsigned long arg5)
{
	struct arm_smccc_res res;

	register uintptr_t a0 asm ("a0") = (uintptr_t)(arg0);
	register uintptr_t a1 asm ("a1") = (uintptr_t)(arg1);
	register uintptr_t a2 asm ("a2") = (uintptr_t)(arg2);
	register uintptr_t a3 asm ("a3") = (uintptr_t)(arg3);
	register uintptr_t a4 asm ("a4") = (uintptr_t)(arg4);
	register uintptr_t a5 asm ("a5") = (uintptr_t)(arg5);
	register uintptr_t a6 asm ("a6") = (uintptr_t)(fid);
	register uintptr_t a7 asm ("a7") = (uintptr_t)(ext);
	asm volatile ("ecall"
		      : "+r" (a0), "+r" (a1)
		      : "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6), "r" (a7)
		      : "memory");
	res.a0 = a0;
	res.a1 = a1;
	res.a2 = a2;
	res.a3 = a3;

	return res;
}

static struct arm_smccc_res __invoke_sip_fn_smc6(unsigned long function_id,
						 unsigned long arg0,
						 unsigned long arg1,
						 unsigned long arg2,
						 unsigned long arg3,
						 unsigned long arg4,
						 unsigned long arg5)
{
	struct arm_smccc_res res;

	if (!sbi_ext_vendor_id) {
		long vid;

		if (sbi_get_mvendorid(&vid)) {
			printf("RISC-V: No machine vendor ID!");
			hang();
		}
		sbi_ext_vendor_id = SBI_EXT_VENDOR_START + vid;
	}

	res = sbi_sip_ecall(sbi_ext_vendor_id, sip_fid_to_sbi(function_id),
			    arg0, arg1, arg2, arg3, arg4, arg5);
	return res;
}

static struct arm_smccc_res __invoke_sip_fn_smc(unsigned long function_id,
						unsigned long arg0,
						unsigned long arg1,
						unsigned long arg2)
{
	return __invoke_sip_fn_smc6(function_id, arg0, arg1, arg2, 0, 0, 0);
}
#endif

int psci_cpu_off(uint32_t state)
{
	struct arm_smccc_res res;

#ifdef CONFIG_ARM
	arm_smccc_smc(ARM_PSCI_0_2_CPU_OFF, state, 0, 0, 0, 0, 0, 0, &res);
#else
	res = sbi_sip_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_STOP, 0, 0, 0, 0, 0, 0);
#endif
	return res.a0;
}

int psci_cpu_on_ctx(unsigned long cpuid, unsigned long entry_point,
		    unsigned long context)
{
	struct arm_smccc_res res;

#ifdef CONFIG_ARM
	arm_smccc_smc(ARM_PSCI_0_2_CPU_ON, cpuid, entry_point, context,
		      0, 0, 0, 0, &res);
#else
	res = sbi_sip_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_START,
			    cpuid, entry_point, context, 0, 0, 0);
#endif
	return res.a0;
}

int psci_cpu_on(unsigned long cpuid, unsigned long entry_point)
{
	return psci_cpu_on_ctx(cpuid, entry_point, 0);
}

int sip_smc_set_suspend_mode(unsigned long ctrl,
			     unsigned long config1,
			     unsigned long config2)
{
	struct arm_smccc_res res;

	res = __invoke_sip_fn_smc(SIP_SUSPEND_MODE, ctrl, config1, config2);
	return res.a0;
}

int sip_smc_remotectl_config(unsigned long func, unsigned long data)
{
	struct arm_smccc_res res;

	res = __invoke_sip_fn_smc(SIP_REMOTECTL_CFG, func, data, 0);
	return res.a0;
}

int sip_smc_access_mem_os_reg(unsigned long func, unsigned long id,
			      unsigned long *val)
{
	struct arm_smccc_res res;

	if (val == NULL)
		return SIP_RET_INVALID_PARAMS;

	res = __invoke_sip_fn_smc(SIP_ACCESS_MEM_OS_REG, func, id, *val);

	if (func == RK_MEM_OS_REG_READ)
		*val = res.a1;

	return res.a0;
}

int sip_smc_amp_cfg(unsigned long func, unsigned long arg0, unsigned long arg1,
		    unsigned long arg2)
{
	struct arm_smccc_res res;

	res = __invoke_sip_fn_smc6(SIP_AMP_CFG, func, arg0, arg1, arg2, 0, 0);
	return res.a0;
}

struct arm_smccc_res sip_smc_dram(unsigned long arg0,
				  unsigned long arg1,
				  unsigned long arg2)
{
	return __invoke_sip_fn_smc(SIP_DRAM_CONFIG, arg0, arg1, arg2);
}

struct arm_smccc_res sip_smc_request_share_mem(unsigned long page_num,
					       share_page_type_t page_type)
{
	struct arm_smccc_res res;
	unsigned long share_mem_phy;

	res = __invoke_sip_fn_smc(SIP_SHARE_MEM, page_num, page_type, 0);
	if (IS_SIP_ERROR(res.a0))
		goto error;

	share_mem_phy = res.a1;
	res.a1 = (unsigned long)ioremap(share_mem_phy, SIZE_PAGE(page_num));

error:
	return res;
}

struct arm_smccc_res sip_smc_secure_reg_read(unsigned long addr_phy)
{
	struct arm_smccc_res res;

	res = __invoke_sip_fn_smc(SIP_ACCESS_REG, 0, addr_phy, SECURE_REG_RD);
	return res;
}

int sip_smc_secure_reg_write(unsigned long addr_phy, unsigned long val)
{
	struct arm_smccc_res res;

	res = __invoke_sip_fn_smc(SIP_ACCESS_REG, val, addr_phy, SECURE_REG_WR);
	return res.a0;
}

int sip_smc_hdcp_config(unsigned long func, unsigned long arg1, unsigned long arg2)
{
	struct arm_smccc_res res;

	res = __invoke_sip_fn_smc(SIP_HDCP_CONFIG, func, arg1, arg2);
	return res.a0;
}

struct arm_smccc_res sip_smc_get_sip_version(void)
{
	return __invoke_sip_fn_smc(SIP_SIP_VERSION, 0, 0, 0);
}

/*
 * OP-TEE works both for kernel 3.10 and 4.4, and these two kernels have
 * different sip implement that 3.10 uses SIP_IMPLEMENT_V1 and 4.4 uses
 * SIP_IMPLEMENT_V2. So we should tell OP-TEE the current rockchip sip
 * version(default SIP_IMPLEMENT_V1) before use.
 */
int sip_smc_set_sip_version(unsigned long version)
{
	struct arm_smccc_res res;

	res = __invoke_sip_fn_smc(SIP_SIP_VERSION, version, SECURE_REG_WR, 0);
	if (IS_SIP_ERROR(res.a0)) {
		printf("%s: set rockchip sip version v%ld failed\n",
		       __func__, version);
		return res.a0;
	}

	return 0;
}

int sip_smc_mcu_config(unsigned long mcu_id, unsigned long func, unsigned long arg2)
{
	struct arm_smccc_res res;

	res = __invoke_sip_fn_smc(SIP_MCU_CFG, mcu_id, func, arg2);
	return res.a0;
}
