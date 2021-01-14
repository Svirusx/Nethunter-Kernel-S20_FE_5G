// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/samsung/debug/sec_debug_summary.c
 *
 * COPYRIGHT(C) 2006-2019 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/of_fdt.h>
#include <linux/version.h>
#include <linux/platform_device.h>

#include <asm/stacktrace.h>
#include <asm/system_misc.h>

#include <linux/sec_debug.h>

#if defined(CONFIG_MSM_SMEM)
#include <soc/qcom/smem.h>
#else
#include <linux/soc/qcom/smem.h>
#endif

#include "sec_debug_internal.h"
#include "sec_debug_summary_extern.h"

static struct sec_debug_summary *secdbg_summary;
static struct sec_debug_summary_data_apss *secdbg_apss;

static char __sec_debug_arch_desc[128];
static char *sec_debug_arch_desc = __sec_debug_arch_desc;

static void __init summary_init_debug_arch_desc(void)
{
	const char *machine_name;

	machine_name = of_flat_dt_get_machine_name();
	if (!machine_name) {
		strlcpy(__sec_debug_arch_desc,
				"UNKNOWN SAMSUNG Device - machine_name is not detected",
				sizeof(__sec_debug_arch_desc));
		return;
	}

	snprintf(__sec_debug_arch_desc, sizeof(__sec_debug_arch_desc),
			"%s (DT)", machine_name);
}

#ifdef CONFIG_SEC_USER_RESET_DEBUG
static void __summary_save_dying_msg_for_user_reset_debug(const char *str,
		const void *pc, const void *lr)
{
	_kern_ex_info_t *p_ex_info;

	sec_debug_store_extc_idx(false);

	if (!sec_debug_reset_ex_info)
		return;

	p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
	if (p_ex_info->cpu == -1) {
		int slen;
		char *msg;

		p_ex_info->cpu = smp_processor_id();
		snprintf(p_ex_info->task_name,
			sizeof(p_ex_info->task_name), "%s", current->comm);
		p_ex_info->ktime = local_clock();
		snprintf(p_ex_info->pc,
			sizeof(p_ex_info->pc), "%pS", pc);
		snprintf(p_ex_info->lr,
			sizeof(p_ex_info->lr), "%pS", lr);

		slen = scnprintf(p_ex_info->panic_buf,
			sizeof(p_ex_info->panic_buf), "%s", str);

		msg = p_ex_info->panic_buf;

		if ((slen >= 1) && (msg[slen - 1] == '\n'))
			msg[slen - 1] = '\0';
	}
}
#else
static inline void __summary_save_dying_msg_for_user_reset_debug(
		const char *str, const void *pc, const void *lr)
{ }
#endif

#ifdef CONFIG_ARM64
#define PT_REGS_PC	pc
#define PT_REGS_LR	regs[30]
#define ARCH_INSTR_SIZE	0x4
#else
#define PT_REGS_PC	ARM_pc
#define PT_REGS_LR	ARM_lr
#define ARCH_INSTR_SIZE	0x4
#endif

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
unsigned long sec_delay_check __read_mostly = 1;
EXPORT_SYMBOL(sec_delay_check);
#endif

int sec_debug_summary_save_die_info(const char *str, struct pt_regs *regs)
{
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
	sec_delay_check = 0;
#endif
	if (!secdbg_apss)
		return -ENOMEM;

	snprintf(secdbg_apss->excp.pc_sym, sizeof(secdbg_apss->excp.pc_sym),
		"%pS", (void *)regs->PT_REGS_PC);
	snprintf(secdbg_apss->excp.lr_sym, sizeof(secdbg_apss->excp.lr_sym),
		"%pS", (void *)regs->PT_REGS_LR);

	__summary_save_dying_msg_for_user_reset_debug(str,
			(void *)regs->PT_REGS_PC, (void *)regs->PT_REGS_LR);

	return 0;
}

int sec_debug_summary_save_panic_info(const char *str, unsigned long caller)
{
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
	sec_delay_check = 0;
#endif
	if (!secdbg_apss)
		return -ENOMEM;

	snprintf(secdbg_apss->excp.panic_caller,
		sizeof(secdbg_apss->excp.panic_caller), "%pS", (void *)caller);
	snprintf(secdbg_apss->excp.panic_msg,
		sizeof(secdbg_apss->excp.panic_msg), "%s", str);
	snprintf(secdbg_apss->excp.thread,
		sizeof(secdbg_apss->excp.thread), "%s:%d", current->comm,
		task_pid_nr(current));

	__summary_save_dying_msg_for_user_reset_debug(str,
			(void *)(caller - ARCH_INSTR_SIZE), (void *)caller);

	return 0;
}

void sec_debug_summary_set_timeout_subsys(const char *source, const char *dest)
{
	if (secdbg_apss == NULL)
		return;

	if (source)
		strlcpy(secdbg_apss->excp.timeout_subsys[0], source,
			sizeof(secdbg_apss->excp.timeout_subsys[0]));

	if (dest)
		strlcpy(secdbg_apss->excp.timeout_subsys[1], dest,
			sizeof(secdbg_apss->excp.timeout_subsys[1]));
}

#ifdef CONFIG_SEC_DEBUG_MDM_FILE_INFO
void sec_set_mdm_summary_info(char *str_buf)
{
	snprintf(secdbg_apss->mdmerr_info,
		sizeof(secdbg_apss->mdmerr_info), "%s", str_buf);
}
#endif

#ifdef CONFIG_SEC_DEBUG_VERBOSE_SUMMARY_HTML
unsigned int cpu_frequency[CONFIG_NR_CPUS];
unsigned int cpu_volt[CONFIG_NR_CPUS];
char cpu_state[CONFIG_NR_CPUS][VAR_NAME_MAX];

void sec_debug_save_cpu_freq_voltage(int cpu, int flag, unsigned long value)
{
	if (flag == SAVE_FREQ)
		cpu_frequency[cpu] = value;
	else if (flag == SAVE_VOLT)
		cpu_volt[cpu] = (unsigned int)value;
}
#endif

static uint32_t tzapps_start_addr;
static uint32_t tzapps_size;

void __deprecated sec_debug_summary_secure_app_addr_size(uint32_t addr,
		uint32_t size)
{
	tzapps_start_addr = addr;
	tzapps_size = size;
}

void sec_debug_summary_set_lpm_info_cci(uint64_t paddr)
{
#ifdef CONFIG_MSM_PM
	if (secdbg_apss) {
		pr_info("0x%llx\n", paddr);
		secdbg_apss->aplpm.p_cci = paddr;
	}
#endif
}

void *sec_debug_summary_get_modem(void)
{
	if (secdbg_summary)
		return (void *)&secdbg_summary->priv.modem;

	pr_warn("secdbg_summary is null.\n");
	return NULL;
}

struct sec_debug_summary_data_apss *sec_debug_summary_get_apss(void)
{
	if (secdbg_summary)
		return &secdbg_summary->priv.apss;

	pr_warn("secdbg_summary is null.\n");
	return NULL;
}

static void __init summary_add_info_mon(const char *name,
		unsigned int size, phys_addr_t pa)
{
	struct sec_debug_summary_simple_var_mon *info_mon;

	info_mon = &(secdbg_apss->info_mon);

	if (info_mon->idx >= ARRAY_SIZE(info_mon->var)) {
		pr_warn("index variable is out of bound!\n");
		return;
	}

	strlcpy(info_mon->var[info_mon->idx].name,
			name, sizeof(info_mon->var[0].name));
	info_mon->var[info_mon->idx].sizeof_type = size;
	info_mon->var[info_mon->idx].var_paddr = pa;

	info_mon->idx++;
}

#define ADD_VAR_TO_INFOMON(var)						\
	summary_add_info_mon(#var, sizeof(var), (uint64_t)__pa(&var))

#define ADD_STR_TO_INFOMON(pstr)					\
	summary_add_info_mon(#pstr, -1, (uint64_t)__pa(pstr))

static void __init summary_add_var_mon(char *name,
		unsigned int size, phys_addr_t pa)
{
	struct sec_debug_summary_simple_var_mon *var_mon;

	if (!secdbg_apss) {
		pr_warn("secdbg_apps is not ready!\n");
		return;
	}

	var_mon = &(secdbg_apss->var_mon);

	if (var_mon->idx >= ARRAY_SIZE(var_mon->var)) {
		pr_warn("index variable is out of bound!\n");
		return;
	}

	strlcpy(var_mon->var[var_mon->idx].name, name,
			sizeof(var_mon->var[0].name));
	var_mon->var[var_mon->idx].sizeof_type = size;
	var_mon->var[var_mon->idx].var_paddr = pa;

	var_mon->idx++;
}

#define ADD_VAR_TO_VARMON(var)						\
	summary_add_var_mon(#var, sizeof(var), __pa(&var))

#define ADD_STR_TO_VARMON(pstr)						\
	summary_add_var_mon(#pstr, -1, __pa(pstr))

#define VAR_NAME_MAX    30

#define ADD_ARRAY_TO_VARMON(arr, _index, _varname)			\
do {									\
	char name[VAR_NAME_MAX];					\
	char _strindex[5];						\
	snprintf(_strindex, ARRAY_SIZE(_strindex) - 2, "%c%zu%c",	\
			'_', _index, '\0');				\
	strlcpy(name, #_varname, ARRAY_SIZE(name));			\
	strlcat(name, _strindex, ARRAY_SIZE(name));			\
	summary_add_var_mon(name, sizeof(arr), __pa(&arr));		\
} while (0)

#define ADD_STR_ARRAY_TO_VARMON(pstrarr, _index, _varname)		\
do {									\
	char name[VAR_NAME_MAX];					\
	char _strindex[5];						\
	snprintf(_strindex, ARRAY_SIZE(_strindex) - 2, "%c%zu%c",	\
			'_', _index, '\0');				\
	strlcpy(name, #_varname, ARRAY_SIZE(name));			\
	strlcat(name, _strindex, ARRAY_SIZE(name));			\
	summary_add_var_mon(name, -1, __pa(&pstrarr));			\
} while (0)

#ifdef __SEC_DEBUG_SUMMARY_BUILD_ROOT
#define summary_set_build_root(s)	__summary_set_build_root(s)
#define __summary_set_build_root(s)	#s
static const char build_root[] =
		summary_set_build_root(__SEC_DEBUG_SUMMARY_BUILD_ROOT);
#else
static const char build_root[] = "UNKNOWN";
#endif

static void __init summary_init_infomon(void)
{
	ADD_STR_TO_INFOMON(build_root);
	ADD_STR_TO_INFOMON(linux_banner);

	summary_add_info_mon("Kernel cmdline", -1,
			IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP) ?
			__pa(erased_command_line) :
			__pa(saved_command_line));
	summary_add_info_mon("Hardware name", -1, __pa(sec_debug_arch_desc));
}

#define phys_addr_of_secdbg_paddr(__member)	\
	(secdbg_paddr + offsetof(struct sec_debug_log, __member))

#ifdef CONFIG_SEC_DEBUG_VERBOSE_SUMMARY_HTML
static void __init __summary_init_varmon_verbose(void)
{
	size_t i;

	for (i = 0; i < num_present_cpus(); i++) {
		ADD_STR_ARRAY_TO_VARMON(cpu_state[i], i, CPU_STAT_CORE);
		ADD_ARRAY_TO_VARMON(cpu_frequency[i], i, CPU_FREQ_CORE);
		ADD_ARRAY_TO_VARMON(cpu_volt[i], i, CPU_VOLT_CORE);
	}
}
#else
static void __init __summary_init_varmon_verbose(void) { }
#endif

static void __init summary_init_varmon(void)
{
	uint64_t last_pet_paddr;
	uint64_t last_ns_paddr;

	/* save paddrs of last_pet und last_ns */
	if (secdbg_paddr && secdbg_log) {
		last_pet_paddr = phys_addr_of_secdbg_paddr(last_pet);
		summary_add_var_mon("last_pet",
			sizeof((secdbg_log->last_pet)), last_pet_paddr);

		last_ns_paddr = phys_addr_of_secdbg_paddr(last_ns);
		summary_add_var_mon("last_ns",
				sizeof((secdbg_log->last_ns.counter)),
				last_ns_paddr);
	} else
		pr_emerg("**** secdbg_log or secdbg_paddr is not initialized ****\n");

#if defined(CONFIG_ARM) || defined(CONFIG_ARM64)
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)
	ADD_VAR_TO_VARMON(boot_reason);
	ADD_VAR_TO_VARMON(cold_boot);
#endif
#endif

	__summary_init_varmon_verbose();
}

static void __init summary_init_sched_log(void)
{
	struct sec_debug_summary_sched_log *sched_log = &(secdbg_apss->sched_log);

	sched_log->sched_idx_paddr = phys_addr_of_secdbg_paddr(sched[0].idx);
	sched_log->sched_buf_paddr = phys_addr_of_secdbg_paddr(sched[0].buf);
	sched_log->sched_struct_buf_sz = sizeof(struct sched_buf);
	sched_log->sched_struct_log_sz = sizeof(struct sched_log);
	sched_log->sched_array_cnt = SCHED_LOG_MAX;

	sched_log->irq_idx_paddr = phys_addr_of_secdbg_paddr(irq[0].idx);
	sched_log->irq_buf_paddr = phys_addr_of_secdbg_paddr(irq[0].buf);
	sched_log->irq_struct_buf_sz = sizeof(struct irq_buf);
	sched_log->irq_struct_log_sz = sizeof(struct irq_log);
	sched_log->irq_array_cnt = SCHED_LOG_MAX;

	sched_log->secure_idx_paddr = phys_addr_of_secdbg_paddr(secure[0].idx);
	sched_log->secure_buf_paddr = phys_addr_of_secdbg_paddr(secure[0].buf);
	sched_log->secure_struct_buf_sz = sizeof(struct secure_buf);
	sched_log->secure_struct_log_sz = sizeof(struct secure_log);
	sched_log->secure_array_cnt = TZ_LOG_MAX;

	sched_log->irq_exit_idx_paddr = phys_addr_of_secdbg_paddr(irq_exit[0].idx);
	sched_log->irq_exit_buf_paddr = phys_addr_of_secdbg_paddr(irq_exit[0].buf);
	sched_log->irq_exit_struct_buf_sz = sizeof(struct irq_exit_buf);
	sched_log->irq_exit_struct_log_sz = sizeof(struct irq_exit_log);
	sched_log->irq_exit_array_cnt = SCHED_LOG_MAX;

#ifdef CONFIG_SEC_DEBUG_MSG_LOG
	sched_log->msglog_idx_paddr = phys_addr_of_secdbg_paddr(secmsg[0].idx);
	sched_log->msglog_buf_paddr = phys_addr_of_secdbg_paddr(secmsg[0].buf);
	sched_log->msglog_struct_buf_sz = sizeof(struct secmsg_buf);
	sched_log->msglog_struct_log_sz = sizeof(struct secmsg_log);
	sched_log->msglog_array_cnt = MSG_LOG_MAX;
#endif
}

#ifdef CONFIG_QCOM_MEMORY_DUMP_V2
static struct sec_debug_summary_msm_dump_info msm_dump_info_cached;

void sec_debug_summary_bark_dump(void *s_cpu_data, void *cpu_buf,
		uint32_t cpu_ctx_size)
{
	msm_dump_info_cached.cpu_data_paddr = (uint64_t)virt_to_phys(s_cpu_data);
	msm_dump_info_cached.cpu_buf_paddr = (uint64_t)virt_to_phys(cpu_buf);
	msm_dump_info_cached.cpu_ctx_size = cpu_ctx_size;
	msm_dump_info_cached.offset = 0x10;	/* 16 bytes */
}

static void *get_wdog_regsave_paddr(void)
{
	return (void *)__pa(&(msm_dump_info_cached.cpu_buf_paddr));
}
#endif

static void __init summary_init_core_reg(void)
{
	struct sec_debug_summary_cpu_context *cpu_reg = &(secdbg_apss->cpu_reg);

	/* setup sec debug core reg info */
	cpu_reg->sec_debug_core_reg_paddr = virt_to_phys(&sec_debug_core_reg);

	pr_info("sec_debug_core_reg_paddr = 0x%llx\n",
				cpu_reg->sec_debug_core_reg_paddr);

#ifdef CONFIG_QCOM_MEMORY_DUMP_V2
	/* setup qc core reg info */
	memcpy(&(cpu_reg->msm_dump_info), &msm_dump_info_cached,
			sizeof(struct sec_debug_summary_msm_dump_info));

	if (!cpu_reg->msm_dump_info.cpu_data_paddr ||
	    !cpu_reg->msm_dump_info.cpu_buf_paddr)
		pr_warn("msm_dump_info is no initialized correctly!\n");
#endif
}

static void __init summary_init_avc_log(void)
{
#ifdef CONFIG_SEC_DEBUG_AVC_LOG
	secdbg_apss->avc_log.secavc_idx_paddr = phys_addr_of_secdbg_paddr(secavc[0].idx);
	secdbg_apss->avc_log.secavc_buf_paddr = phys_addr_of_secdbg_paddr(secavc[0].buf);
	secdbg_apss->avc_log.secavc_struct_sz = sizeof(struct secavc_buf);
	secdbg_apss->avc_log.secavc_array_cnt = AVC_LOG_MAX;
#endif
}

int sec_debug_summary_is_modem_separate_debug_ssr(void)
{
	if (IS_ENABLED(CONFIG_SEC_CP_SEPARATE_DEBUG))
		return SEC_DEBUG_MODEM_SEPARATE_EN;
	else
		return secdbg_summary->priv.modem.separate_debug;
}

#define SET_MEMBER_TYPE_INFO(PTR, TYPE, MEMBER) \
	{ \
		(PTR)->size = sizeof(((TYPE *)0)->MEMBER); \
		(PTR)->offset = offsetof(TYPE, MEMBER); \
	}

static void __init summary_init_task_info(void)
{
	secdbg_apss->task.stack_size = THREAD_SIZE;
	secdbg_apss->task.start_sp = THREAD_SIZE;
#ifdef CONFIG_VMAP_STACK
	secdbg_apss->task.irq_stack.pcpu_stack = (uint64_t)&irq_stack_ptr;
#else
	secdbg_apss->task.irq_stack.pcpu_stack = (uint64_t)&irq_stack;
#endif
	secdbg_apss->task.irq_stack.size = IRQ_STACK_SIZE;
	secdbg_apss->task.irq_stack.start_sp = IRQ_STACK_SIZE;

	secdbg_apss->task.ti.struct_size = sizeof(struct thread_info);
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ti.flags, struct thread_info, flags);
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ts.cpu, struct task_struct, cpu);
#if defined (CONFIG_CFP_ROPP) || defined(CONFIG_RKP_CFP_ROPP)
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ti.rrk, struct thread_info, rrk);
#endif

	secdbg_apss->task.ts.struct_size = sizeof(struct task_struct);
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ts.state, struct task_struct, state);
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ts.exit_state, struct task_struct,
					exit_state);
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ts.stack, struct task_struct, stack);
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ts.flags, struct task_struct, flags);
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ts.on_cpu, struct task_struct, on_cpu);
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ts.pid, struct task_struct, pid);
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ts.comm, struct task_struct, comm);
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ts.tasks_next, struct task_struct,
					tasks.next);
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ts.thread_group_next,
					struct task_struct, thread_group.next);
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ts.fp, struct task_struct,
					thread.cpu_context.fp);
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ts.sp, struct task_struct,
					thread.cpu_context.sp);
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ts.pc, struct task_struct,
					thread.cpu_context.pc);

#ifdef CONFIG_SCHED_INFO
	/* sched_info */
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ts.sched_info__pcount,
					struct task_struct, sched_info.pcount);
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ts.sched_info__run_delay,
					struct task_struct,
					sched_info.run_delay);
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ts.sched_info__last_arrival,
					struct task_struct,
					sched_info.last_arrival);
	SET_MEMBER_TYPE_INFO(&secdbg_apss->task.ts.sched_info__last_queued,
					struct task_struct,
					sched_info.last_queued);
#endif

	secdbg_apss->task.init_task = (uint64_t)&init_task;
#if defined (CONFIG_CFP_ROPP) || defined(CONFIG_RKP_CFP_ROPP)
	secdbg_apss->task.ropp.magic = 0x50504F52;
#else
	secdbg_apss->task.ropp.magic = 0x0;
#endif
}

static void __init summary_init_kernel_constant(void)
{
	struct sec_debug_summary_kconst *kconst = &(secdbg_apss->kconst);

	kconst->nr_cpus = num_possible_cpus();
	kconst->per_cpu_offset.pa = virt_to_phys(__per_cpu_offset);
	kconst->per_cpu_offset.size = sizeof(__per_cpu_offset[0]);
	kconst->per_cpu_offset.count = ARRAY_SIZE(__per_cpu_offset);
	kconst->phys_offset = PHYS_OFFSET;
	kconst->page_offset = PAGE_OFFSET;
	kconst->va_bits = VA_BITS;
	kconst->kimage_vaddr = kimage_vaddr;
	kconst->kimage_voffset = kimage_voffset;
	kconst->swapper_pg_dir_paddr = __pa_symbol(swapper_pg_dir);
	kconst->vmap_stack = IS_ENABLED(CONFIG_VMAP_STACK) ? 1 : 0;
}

static void __init summary_set_kallsyms_info(void)
{
	struct sec_debug_summary_ksyms *ksyms = &(secdbg_apss->ksyms);

	if (!IS_ENABLED(CONFIG_KALLSYMS_BASE_RELATIVE)) {
		ksyms->addresses_pa = __pa(kallsyms_addresses);
		ksyms->relative_base = 0x0;
		ksyms->offsets_pa = 0x0;
	} else {
		ksyms->addresses_pa = 0x0;
		ksyms->relative_base = (uint64_t)kallsyms_relative_base;
		ksyms->offsets_pa = __pa(kallsyms_offsets);
	}

	ksyms->names_pa = __pa(kallsyms_names);
	ksyms->num_syms = kallsyms_num_syms;
	ksyms->token_table_pa = __pa(kallsyms_token_table);
	ksyms->token_index_pa = __pa(kallsyms_token_index);
	ksyms->markers_pa = __pa(kallsyms_markers);

	ksyms->sect.sinittext = (uintptr_t)_sinittext;
	ksyms->sect.einittext = (uintptr_t)_einittext;
	ksyms->sect.stext = (uintptr_t)_stext;
	ksyms->sect.etext = (uintptr_t)_etext;
	ksyms->sect.end = (uintptr_t)_end;

	ksyms->kallsyms_all = IS_ENABLED(CONFIG_KALLSYMS_ALL) ? 1 : 0;
	ksyms->magic = SEC_DEBUG_SUMMARY_MAGIC1;
}

static void summary_set_lpm_info_runqueues(void)
{
	secdbg_apss->aplpm.p_runqueues = virt_to_phys((void *)&runqueues);
#ifdef CONFIG_SCHED_WALT
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0)
	secdbg_apss->aplpm.cstate_offset = offsetof(struct rq, cstate);
#endif
#endif
}

#ifdef CONFIG_SCHED_WALT
int __weak get_num_clusters(void)
{
	return num_clusters;
}
#endif

static void __init summary_set_lpm_info_cluster(void)
{
#ifdef CONFIG_SCHED_WALT
	secdbg_apss->aplpm.num_clusters = get_num_clusters();
	secdbg_apss->aplpm.p_cluster = virt_to_phys((void *)sched_cluster);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0)
	secdbg_apss->aplpm.dstate_offset = offsetof(struct sched_cluster, dstate);
#endif
#endif
}

static void __init sec_debug_summary_external_init(void)
{
	summary_set_kallsyms_info();
	summary_set_lpm_info_cluster();
	summary_set_lpm_info_runqueues();

	sec_debug_summary_set_klog_info(secdbg_apss);
	sec_debug_summary_set_msm_memdump_info(secdbg_apss);
#if IS_ENABLED(CONFIG_QCOM_RTB)
	sec_debug_summary_set_rtb_info(secdbg_apss);
#endif
	summary_init_coreinfo(secdbg_apss);
}

#if defined(CONFIG_QCOM_SMEM)
static int __init __qcom_smem_alloc_secdbg_summary(size_t size)
{
	int err;

	/* set summary address in smem for other subsystems to see */
	err = qcom_smem_alloc(QCOM_SMEM_HOST_ANY, SMEM_ID_VENDOR2, size);
	if (err && err != -EEXIST) {
		pr_err("smem alloc failed! (%d)\n", err);
		return err;
	}

	secdbg_summary = qcom_smem_get(QCOM_SMEM_HOST_ANY,
			SMEM_ID_VENDOR2, &size);
	if (!secdbg_summary || size < sizeof(struct sec_debug_summary)) {
		pr_err("smem get failed!\n");
		return -ENOMEM;
	}

	return 0;
}
#else
static inline int __qcom_smem_alloc_secdbg_summary(size_t size) { return -ENODEV; }
#endif

#if defined(CONFIG_MSM_SMEM)
static int __init __msm_smem_alloc_secdbg_summary(size_t size)
{
	secdbg_summary = smem_alloc(SMEM_ID_VENDOR2, size, 0, SMEM_ANY_HOST_FLAG);
	if (!secdbg_summary) {
		pr_err("smem alloc failed!\n");
		return -ENOMEM;
	}

	return 0;
}
#else
static inline int __msm_smem_alloc_secdbg_summary(size_t size) { return -ENODEV; }
#endif

static int __init __smem_alloc_secdbg_summary(size_t size)
{
	int err;

	err = __qcom_smem_alloc_secdbg_summary(size);
	if (unlikely(err) && IS_ENABLED(CONFIG_MSM_SMEM))
		err = __msm_smem_alloc_secdbg_summary(size);

	return err;
}

static int _sec_debug_summary_init(void)
{
	int err;
	size_t size = sizeof(struct sec_debug_summary);

	pr_info("SMEM_ID_VENDOR2=0x%x size=0x%lx\n",
		(unsigned int)SMEM_ID_VENDOR2, size);

	err = __smem_alloc_secdbg_summary(size);
	if (err) {
		pr_err("return with error[%d]", err);
		return err;
	}

	memset_io(secdbg_summary, 0x0, size);

	secdbg_summary->_apss =
		qcom_smem_virt_to_phys(&secdbg_summary->priv.apss);
	secdbg_summary->_rpm =
		qcom_smem_virt_to_phys(&secdbg_summary->priv.rpm);
	secdbg_summary->_modem =
		qcom_smem_virt_to_phys(&secdbg_summary->priv.modem);
	secdbg_summary->_dsps =
		qcom_smem_virt_to_phys(&secdbg_summary->priv.dsps);

	pr_info("apss(%llx) rpm(%llx) modem(%llx) dsps(%llx)\n",
			secdbg_summary->_apss,
			secdbg_summary->_rpm,
			secdbg_summary->_modem,
			secdbg_summary->_dsps);

	/* FIXME: 'sec_debug_summary_secure_app_addr_size' is not called
	 * anywhere. It seems that, following two lines are deprecated.
	 */
	secdbg_summary->secure_app_start_addr = tzapps_start_addr;
	secdbg_summary->secure_app_size = tzapps_size;

	secdbg_apss = &secdbg_summary->priv.apss;

	strlcpy(secdbg_apss->name, "APSS", sizeof(secdbg_apss->name));
	strlcpy(secdbg_apss->state, "Init", sizeof(secdbg_apss->state));
	secdbg_apss->nr_cpus = num_present_cpus();
	secdbg_apss->dump_sink_paddr = get_pa_dump_sink();
	secdbg_apss->tz_core_dump = get_wdog_regsave_paddr();

	summary_init_debug_arch_desc();
	summary_init_infomon();
	summary_init_varmon();
	summary_init_sched_log();
	summary_init_avc_log();
	summary_init_core_reg();
	summary_init_kernel_constant();
	summary_init_task_info();

	/* TODO: initialize debug_summary data using external symbols.
	 * Plz see 'sec_debug_summary_external.h' file to manage
	 * which files and symbols are used at here.
	 */
	sec_debug_summary_external_init();

	/* fill magic nubmer last to ensure data integrity when the magic
	 * numbers are written
	 */
	secdbg_summary->magic[0] = SEC_DEBUG_SUMMARY_MAGIC0;
	secdbg_summary->magic[1] = SEC_DEBUG_SUMMARY_MAGIC1;
	secdbg_summary->magic[2] = SEC_DEBUG_SUMMARY_MAGIC2;
	secdbg_summary->magic[3] = SEC_DEBUG_SUMMARY_MAGIC3;

	return 0;
}

#ifdef CONFIG_SEC_DEBUG_SUMMARY_DRIVER
static int sec_debug_summary_probe(struct platform_device *pdev)
{
	int err = _sec_debug_summary_init();

	if (err) {
		pr_err("return with error[%d]", err);
		return err;
	}
	
	platform_set_drvdata(pdev, secdbg_summary);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id sec_debug_summary_dt_ids[] = {
	{ .compatible = "samsung,sec-debug-summary" },
	{ }
};
MODULE_DEVICE_TABLE(of, sec_debug_summary_dt_ids);
#endif /* CONFIG_OF */

struct platform_driver sec_debug_summary_driver = {
	.probe		= sec_debug_summary_probe,
	.driver		= {
		.name 	= "sec_debug_summary",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = sec_debug_summary_dt_ids,
#endif
	},
};

static int __init sec_debug_summary_init(void)
{
	int err;

	err = platform_driver_register(&sec_debug_summary_driver);
	if (err)
		pr_err("Failed to register sec_debug_summary platform driver: %d\n", err);

	return 0;
}
subsys_initcall_sync(sec_debug_summary_init);

static void __exit sec_debug_summary_exit(void)
{
	platform_driver_unregister(&sec_debug_summary_driver);
}
module_exit(sec_debug_summary_exit);
#else
static int __init sec_debug_summary_init(void)
{
	return _sec_debug_summary_init();
}
subsys_initcall_sync(sec_debug_summary_init);
#endif