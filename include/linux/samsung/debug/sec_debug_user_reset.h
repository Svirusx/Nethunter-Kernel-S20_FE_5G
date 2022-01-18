#ifndef __SEC_DEBUG_USER_RESET_INDIRECT
#warning "samsung/debug/sec_debug_user_reset.h is included directly."
#error "please include sec_debug.h instead of this file"
#endif

#ifndef __INDIRECT__SEC_DEBUG_USER_RESET_H__
#define __INDIRECT__SEC_DEBUG_USER_RESET_H__

#include "sec_debug_user_reset_type.h"

#if IS_ENABLED(CONFIG_SEC_USER_RESET_DEBUG)
extern void sec_debug_store_additional_dbg(enum extra_info_dbg_type type, unsigned int value, const char *fmt, ...);

/* called @ kernel/panic.c */
/* called @ arch/arm64/mm/fault.c */
extern void sec_debug_store_extc_idx(bool prefix);

/* called @ lib/bug.c */
extern void sec_debug_store_bug_string(const char *file, unsigned bug_line);

/* called @ drivers/misc/samsung/sec_hw_param.c */
extern unsigned int sec_debug_get_reset_reason(void);

/* called @ drivers/misc/samsung/sec_hw_param.c */
extern int sec_debug_get_reset_write_cnt(void);

/* called @ drivers/misc/samsung/sec_hw_param.c */
extern char *sec_debug_get_reset_reason_str(unsigned int reason);

/* called @ drivers/misc/samsung/sec_hw_param.c */
extern struct debug_reset_header *get_debug_reset_header(void);

/* FIXME: this function is not referenced anywhere */
extern void __deprecated sec_debug_summary_modem_print(void);

extern rst_exinfo_t *sec_debug_reset_ex_info;

/* called @ arch/arm64/mm/fault.c */
static inline void sec_debug_store_pte(unsigned long addr, int idx)
{
	int cpu;
	_kern_ex_info_t *p_ex_info;

	if (!sec_debug_reset_ex_info)
		return;

	p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
	if (p_ex_info->cpu != -1)
		return;

	cpu = get_cpu();

	if(idx == 0)
		memset(&p_ex_info->fault[cpu].pte, 0,
				sizeof(p_ex_info->fault[cpu].pte));

	p_ex_info->fault[cpu].pte[idx] = addr;
	put_cpu();
}

/* called @ arch/arm64/mm/fault.c */
static inline void sec_debug_save_fault_info(unsigned int esr, const char *str,
			unsigned long var1, unsigned long var2)
{
	int cpu;
	_kern_ex_info_t *p_ex_info;

	if (!sec_debug_reset_ex_info)
		return;

	p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
	if (p_ex_info->cpu != -1)
		return;

	cpu = get_cpu();

	p_ex_info->fault[cpu].esr = esr;
	snprintf(p_ex_info->fault[cpu].str,
			sizeof(p_ex_info->fault[cpu].str),
			"%s", str);
	p_ex_info->fault[cpu].var1 = var1;
	p_ex_info->fault[cpu].var2 = var2;
	put_cpu();
}

/* called @ arch/arm64/kernel/traps.c */
static inline void sec_debug_save_badmode_info(int reason,
			const char *handler_str,
			unsigned int esr, const char *esr_str)
{
	_kern_ex_info_t *p_ex_info;

	if (!sec_debug_reset_ex_info)
		return;

	p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
	if (p_ex_info->cpu != -1)
		return;

	p_ex_info->badmode.reason = reason;
	snprintf(p_ex_info->badmode.handler_str,
			sizeof(p_ex_info->badmode.handler_str),
			"%s", handler_str);
	p_ex_info->badmode.esr = esr;
	snprintf(p_ex_info->badmode.esr_str,
			sizeof(p_ex_info->badmode.esr_str),
			"%s", esr_str);
}

/* called @ drivers/iommu/arm-smmu.c */
static inline void ___sec_debug_save_smmu_info(ex_info_smmu_t *smmu_info)
{
	_kern_ex_info_t *p_ex_info;

	if (!sec_debug_reset_ex_info)
		return;

	p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
	if (p_ex_info->cpu != -1)
		return;

	memcpy(&(p_ex_info->smmu), smmu_info, sizeof(ex_info_smmu_t));
}

/* called @ drivers/iommu/arm-smmu.c */
#define sec_debug_save_smmu_info_asf_fatal()				\
do {									\
	ex_info_smmu_t sec_dbg_smmu;					\
	memset(&sec_dbg_smmu, 0x0, sizeof(ex_info_smmu_t));		\
	snprintf(sec_dbg_smmu.dev_name, sizeof(sec_dbg_smmu.dev_name),	\
			"%s", dev_name(smmu->dev));			\
	sec_dbg_smmu.fsr = fsr;						\
	___sec_debug_save_smmu_info(&sec_dbg_smmu);			\
} while (0)

/* called @ drivers/iommu/arm-smmu.c */
#define sec_debug_save_smmu_info_fatal()				\
do {									\
	ex_info_smmu_t sec_dbg_smmu;					\
	memset(&sec_dbg_smmu, 0x0, sizeof(ex_info_smmu_t));		\
	snprintf(sec_dbg_smmu.dev_name, sizeof(sec_dbg_smmu.dev_name),	\
		"%s", dev_name(smmu->dev));				\
	sec_dbg_smmu.fsr = fsr;						\
	sec_dbg_smmu.fsynr0 = fsynr0;					\
	sec_dbg_smmu.fsynr1 = fsynr1;					\
	sec_dbg_smmu.iova = iova;					\
	sec_dbg_smmu.far = (unsigned long)iova;				\
	sec_dbg_smmu.cbndx = cfg->cbndx;				\
	sec_dbg_smmu.phys_soft = phys_soft;				\
	sec_dbg_smmu.sid = frsynra;					\
	___sec_debug_save_smmu_info(&sec_dbg_smmu);			\
} while (0)
#else /* CONFIG_SEC_USER_RESET_DEBUG */

static inline void sec_debug_store_extc_idx(bool prefix) {}
static inline void sec_debug_store_bug_string(const char *file, unsigned bug_line) {}
static inline void sec_debug_store_pte(unsigned long addr, int idx) {}
static inline void sec_dbg_save_fault_info(unsigned int esr, const char *str, unsigned long var1, unsigned long var2) {}
static inline void sec_debug_save_badmode_info(int reason, const char *handler_str,unsigned int esr, const char *esr_str) {}
static inline void sec_debug_save_smmu_info(ex_info_smmu_t *smmu_info) {}
static inline char *sec_debug_get_reset_reason_str(unsigned int reason) { return NULL; }
static inline unsigned int sec_debug_get_reset_reason(void) { return 0; }
static inline void sec_debug_save_fault_info(unsigned int esr, const char *str, unsigned long var1, unsigned long var2) {}
static inline struct debug_reset_header *get_debug_reset_header(void) { return NULL; }
static inline int sec_debug_get_reset_write_cnt(void) { return 0; }
#define sec_debug_save_smmu_info_fatal()
#define sec_debug_save_smmu_info_asf_fatal()
static inline void sec_debug_summary_modem_print(void) {}
#endif /* CONFIG_SEC_USER_RESET_DEBUG */

#endif /* __INDIRECT__SEC_DEBUG_USER_RESET_H__ */
