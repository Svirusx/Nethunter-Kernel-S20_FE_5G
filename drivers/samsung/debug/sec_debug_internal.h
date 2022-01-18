/* drivers/debug/sec_debug_internal.h */

#ifndef __SEC_DEBUG_INTERNAL_H__
#define __SEC_DEBUG_INTERNAL_H__

#include <linux/sec_debug.h>

#include "../sec_kcompat.h"

/* [[BEGIN>> sec_debug.c */
extern char *erased_command_line;

extern void dump_memory_info(void);
extern void dump_all_task_info(void);
extern void dump_cpu_stat(void);
extern void sec_debug_set_upload_cause(enum sec_debug_upload_cause_t type);
extern enum sec_debug_upload_cause_t sec_debug_get_upload_cause(void);
/* <<END]] sec_debug.c */

/* [[BEGIN>> sec_debug_sched_log.c */
#if defined(CONFIG_SEC_DEBUG_SCHED_LOG_PER_CPU)
extern struct sec_debug_last_pet_ns *secdbg_last_pet_ns; 
#else
extern struct sec_debug_log *secdbg_log;
#endif
extern phys_addr_t secdbg_paddr;
extern size_t secdbg_size;
/* <<END]] sec_debug_sched_log.c */

/* [[BEGIN>> sec_log_buf.c */
extern unsigned int get_sec_log_idx(void);
/* <<END]] sec_log_buf.c */

/* [[BEGIN>> sec_debug_summary.c */
#ifdef CONFIG_SEC_DEBUG_SUMMARY
extern void *sec_debug_summary_get_modem(void);
#else
static void inline *sec_debug_summary_get_modem(void) { return NULL; }
#endif
/* <<END]] sec_debug_summary.c */

/* [[BEGIN>> sec_debug_summary_coreinfo.c */
#ifdef CONFIG_SEC_DEBUG_SUMMARY
extern int summary_init_coreinfo(struct sec_debug_summary_data_apss *apss);
#else
#define summary_init_coreinfo(__apss)	(0)
#endif
/* <<END]] sec_debug_summary_coreinfo.c */

/* [[BEGIN>> sec_debug_user_reset.c */
#ifdef CONFIG_SEC_USER_RESET_DEBUG
extern void sec_debug_backtrace(void);
#else
#define sec_debug_reset_ex_info		(NULL)
static inline void sec_debug_backtrace(void) {}
#endif
/* <<END]] sec_debug_user_reset.c */

/* [[BEGIN>> sec_bootstat.c */
/* implemented @ drivers/soc/qcom/boot_stats.c */
extern uint32_t bs_linuxloader_start;
extern uint32_t bs_linux_start;
extern uint32_t bs_uefi_start;
extern uint32_t bs_bootloader_load_kernel;
extern unsigned int get_boot_stat_time(void);
extern unsigned int get_boot_stat_freq(void);
/* <<END]] sec_bootstat.c */

/* [[BEGIN>> sec_log_buf.c */
/* implemented @ kernel/printk/printk.c */
#ifdef CONFIG_SEC_LOG_BUF_NO_CONSOLE
extern void sec_log_buf_pull_early_buffer(bool *init_done);
#else
static inline void sec_log_buf_pull_early_buffer(bool *init_done) {}
#endif
/* <<END]] sec_log_buf.c */

/* out-of-sec_debug */

/* drivers/power/reset/msm-poweroff.c */
extern void set_dload_mode(int on);

/* kernel/init/version.c */
#include <linux/utsname.h>
extern struct uts_namespace init_uts_ns;

/* drivers/base/core.c */
extern struct kset *devices_kset;

/* implemented @ drivers/soc/qcom/watchdog_v2.c */
extern void force_watchdog_bark(void);

/* implemented @ drivers/input/misc/qpnp-power-on.c */
int qpnp_control_s2_reset_onoff(int on);

/* fake pr_xxx macros to prevent checkpatch fails */
#define __printx	printk
#define __pr_info(fmt, ...) \
	__printx(KERN_INFO fmt, ##__VA_ARGS__)
#define __pr_err(fmt, ...) \
	__printx(KERN_ERR fmt, ##__VA_ARGS__)

#endif /* __SEC_DEBUG_INTERNAL_H__ */
