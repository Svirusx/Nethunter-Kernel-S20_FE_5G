#ifndef __SEC_DEBUG_SUMMARY_INDIRECT
#warning "samsung/debug/sec_debug_sched_log.h is included directly."
#error "please include sec_debug.h instead of this file"
#endif

#ifndef __INDIRECT__SEC_DEBUG_SUMMARY_H__
#define __INDIRECT__SEC_DEBUG_SUMMARY_H__

#include "sec_debug_summary_type.h"

#if IS_ENABLED(CONFIG_SEC_DEBUG_SUMMARY)

#include "sec_debug_summary_coreinfo.h"

/* called @ drivers/soc/qcom/watchdog_v2.c */
extern void sec_debug_summary_bark_dump(void *s_cpu_data, void *cpu_buf,
		uint32_t cpu_ctx_size);

/* called @ arch/arm64/kernel/traps.c */
int sec_debug_summary_save_die_info(const char *str, struct pt_regs *regs);

/* called @ drivers/debug/sec_debug_summary.c */
int sec_debug_summary_save_panic_info(const char *str, unsigned long caller);

/* called @ drivers/soc/qcom/memory_dump_v2.c */
struct sec_debug_summary_data_apss *sec_debug_summary_get_apss(void);

/* called @ drivers/soc/qcom/subsystem_restart.c */
extern int sec_debug_summary_is_modem_separate_debug_ssr(void);
extern void sec_debug_summary_set_timeout_subsys(const char *source, const char *dest);

/* called @ drivers/cpuidle/lpm-levels.c */
extern void sec_debug_summary_set_lpm_info_cci(uint64_t phy_addr);

/* FIXME: this function is not referenced anywhere */
extern void __deprecated sec_debug_summary_secure_app_addr_size(uint32_t addr, uint32_t size);
#else /* CONFIG_SEC_DEBUG_SUMMARY */
static inline void sec_debug_summary_bark_dump(void *s_cpu_data, void *cpu_buf, uint32_t cpu_ctx_size) { }
static inline int sec_debug_summary_save_die_info(const char *str, struct pt_regs *regs) { return 0; }
static inline int sec_debug_summary_save_panic_info(const char *str, unsigned long caller) { return 0; }
static inline struct sec_debug_summary_data_apss *sec_debug_summary_get_apss(void) { return NULL; }
static inline int sec_debug_summary_is_modem_separate_debug_ssr(void) { return SEC_DEBUG_MODEM_SEPARATE_DIS; }
static inline void sec_debug_summary_secure_app_addr_size(uint32_t addr, uint32_t size) { }
static inline void sec_debug_summary_set_lpm_info_cci(uint64_t phy_addr) { }
static inline void sec_debug_summary_set_timeout_subsys(const char *source, const char *dest) { }
#endif /* CONFIG_SEC_DEBUG_SUMMARY */


#ifdef CONFIG_SEC_DEBUG_VERBOSE_SUMMARY_HTML
enum {
	SAVE_FREQ = 0,
	SAVE_VOLT
};

extern unsigned int cpu_frequency[CONFIG_NR_CPUS];
extern unsigned int cpu_volt[CONFIG_NR_CPUS];
extern char cpu_state[CONFIG_NR_CPUS][VAR_NAME_MAX];

/* FIXME: this function is not referenced anywhere */
extern void sec_debug_save_cpu_freq_voltage(int cpu, int flag, unsigned long value);
#else
static inline void sec_debug_save_cpu_freq_voltage(int cpu, int flag, unsigned long value) { }
#endif	/* CONFIG_SEC_DEBUG_VERBOSE_SUMMARY_HTML */


#ifdef CONFIG_SEC_DEBUG_MDM_FILE_INFO
/* FIXME: this function is not referenced anywhere */
extern void sec_modify_restart_level_mdm(int value);

/* FIXME: this function is not referenced anywhere */
extern void sec_set_mdm_summary_info(char *str_buf);
#else
static inline void sec_modify_restart_level_mdm(int value) { }
static inline void sec_set_mdm_summary_info(char *str_buf) { }
#endif	/* CONFIG_SEC_DEBUG_MDM_FILE_INFO */

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
extern unsigned long sec_delay_check;
#endif

#endif	/* __INDIRECT__SEC_DEBUG_SUMMARY_H__ */
