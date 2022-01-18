#ifndef __SEC_DEBUG_INDIRECT
#warning "samsung/debug/sec_debug.h is included directly."
#error "please include sec_debug.h instead of this file"
#endif

#ifndef __INDIRECT__SEC_DEBUG_H__
#define __INDIRECT__SEC_DEBUG_H__

#include <linux/init.h>

#include <asm/cacheflush.h>

#if defined(CONFIG_SEC_DEBUG_SCHED_LOG_IRQ_V2)
#define IRQ_ENTRY_V2	0x76324945
#define IRQ_EXIT_V2	0x76324958
#else
#define IRQ_ENTRY	0x4945
#define IRQ_EXIT	0x4958
#endif

#define SOFTIRQ_ENTRY	0x5345
#define SOFTIRQ_EXIT	0x5358

#define SCM_ENTRY	0x5555
#define SCM_EXIT	0x6666

#define SEC_DEBUG_MODEM_SEPARATE_EN  0xEAEAEAEA
#define SEC_DEBUG_MODEM_SEPARATE_DIS 0xDEADDEAD

#define RESET_EXTRA_INFO_SIZE	1024

enum sec_debug_upload_cause_t {
	UPLOAD_CAUSE_INIT = 0xCAFEBABE,
/* ++ KP : 0xC8xx_xxxx ++ */
	UPLOAD_CAUSE_KERNEL_PANIC = 0xC8000000,
	UPLOAD_CAUSE_USER_FAULT,
	UPLOAD_CAUSE_HSIC_DISCONNECTED,
	UPLOAD_CAUSE_BUS_HANG,
	UPLOAD_CAUSE_PF_WD_BITE,
	UPLOAD_CAUSE_PF_WD_INIT_FAIL,
	UPLOAD_CAUSE_PF_WD_RESTART_FAIL,
	UPLOAD_CAUSE_PF_WD_KICK_FAIL,
/* ++ SubSystem : 0xC8_5353_xx ++ */
	UPLOAD_CAUSE_SS_START = 0xC8535300,
	UPLOAD_CAUSE_CP_ERROR_FATAL = UPLOAD_CAUSE_SS_START,
	UPLOAD_CAUSE_MDM_ERROR_FATAL,
	UPLOAD_CAUSE_MDM_CRITICAL_FATAL,
	UPLOAD_CAUSE_MODEM_RST_ERR,
	UPLOAD_CAUSE_ADSP_ERROR_FATAL,
	UPLOAD_CAUSE_SLPI_ERROR_FATAL, /* 5 */
	UPLOAD_CAUSE_SPSS_ERROR_FATAL,
	UPLOAD_CAUSE_NPU_ERROR_FATAL,
	UPLOAD_CAUSE_CDSP_ERROR_FATAL,
	UPLOAD_CAUSE_SUBSYS_IF_TIMEOUT,
	UPLOAD_CAUSE_RIVA_RST_ERR, /* A */
	UPLOAD_CAUSE_LPASS_RST_ERR,
	UPLOAD_CAUSE_DSPS_RST_ERR,
	UPLOAD_CAUSE_PERIPHERAL_ERR,
	UPLOAD_CAUSE_SS_END = UPLOAD_CAUSE_PERIPHERAL_ERR,
/* -- SubSystem : 0xC8_5353_xx -- */
/* ++ Quest : 0xC8_5153_xx ++ */
	UPLOAD_CAUSE_QUEST_START = 0xC8515300,
	UPLOAD_CAUSE_QUEST_CRYPTO = UPLOAD_CAUSE_QUEST_START,
	UPLOAD_CAUSE_QUEST_ICACHE,
	UPLOAD_CAUSE_QUEST_CACHECOHERENCY,
	UPLOAD_CAUSE_QUEST_SUSPEND,
	UPLOAD_CAUSE_QUEST_VDDMIN,
	UPLOAD_CAUSE_QUEST_QMESADDR, /* 5 */
	UPLOAD_CAUSE_QUEST_QMESACACHE,
	UPLOAD_CAUSE_QUEST_PMIC,
	UPLOAD_CAUSE_QUEST_UFS,
	UPLOAD_CAUSE_QUEST_SDCARD,
	UPLOAD_CAUSE_QUEST_SENSOR, /* A */
	UPLOAD_CAUSE_QUEST_SENSORPROBE,
	UPLOAD_CAUSE_QUEST_NATURESCENE,
	UPLOAD_CAUSE_QUEST_A75G,
	UPLOAD_CAUSE_QUEST_Q65G,
	UPLOAD_CAUSE_QUEST_THERMAL, /* F */
	UPLOAD_CAUSE_QUEST_QDAF_FAIL, /* 10 */
	UPLOAD_CAUSE_QUEST_FAIL,
	UPLOAD_CAUSE_QUEST_DDR_TEST_MAIN,
	UPLOAD_CAUSE_QUEST_DDR_TEST_CAL,
	UPLOAD_CAUSE_QUEST_DDR_TEST_SMD,
	UPLOAD_CAUSE_SOD_RESULT,
	UPLOAD_CAUSE_QUEST_ZIP_UNZIP,
	UPLOAD_CAUSE_DRAM_SCAN,
	UPLOAD_CAUSE_QUEST_AOSSTHERMALDIFF,
	UPLOAD_CAUSE_QUEST_STRESSAPPTEST,
	UPLOAD_CAUSE_QUEST_END = UPLOAD_CAUSE_QUEST_STRESSAPPTEST,
/* --Quest : 0xC8_5153_xx -- */
/* -- KP : 0xC8xx_xxxx -- */
/* ++ TP ++ */
	UPLOAD_CAUSE_POWER_THERMAL_RESET = 0x54500000,
/* -- TP -- */
/* ++ DP ++ */
	UPLOAD_CAUSE_NON_SECURE_WDOG_BARK = 0x44500000,
/* -- DP -- */
/* ++ WP ++ */
	UPLOAD_CAUSE_NON_SECURE_WDOG_BITE = 0x57500000,
	UPLOAD_CAUSE_SECURE_WDOG_BITE,
/* -- WP -- */
/* ++ MP ++ */
/* Intended reset (MP) 0x4D50_xxxx */
	UPLOAD_CAUSE_MP_START = 0x4D500000,
	UPLOAD_CAUSE_POWER_LONG_PRESS = UPLOAD_CAUSE_MP_START,
	UPLOAD_CAUSE_FORCED_UPLOAD,
	UPLOAD_CAUSE_USER_FORCED_UPLOAD,
	UPLOAD_CAUSE_MP_END = UPLOAD_CAUSE_USER_FORCED_UPLOAD,
/* -- MP -- */
/* ++ PP ++ */
	UPLOAD_CAUSE_EDL_FORCED_UPLOAD = 0x50500000,
	UPLOAD_CAUSE_PM_OCP,
/* -- PP -- */
/* ++ SP -- */
	UPLOAD_CAUSE_SMPL = 0x53500000,
/* -- SP -- */
};

enum sec_restart_reason_t {
	RESTART_REASON_NORMAL = 0x0,
	RESTART_REASON_BOOTLOADER = 0x77665500,
	RESTART_REASON_REBOOT = 0x77665501,
	RESTART_REASON_RECOVERY = 0x77665502,
	RESTART_REASON_RTC = 0x77665503,
	RESTART_REASON_DMVERITY_CORRUPTED = 0x77665508,
	RESTART_REASON_DMVERITY_ENFORCE = 0x77665509,
	RESTART_REASON_KEYS_CLEAR = 0x7766550a,
	RESTART_REASON_SEC_DEBUG_MODE = 0x776655ee,
	RESTART_REASON_RECOVERY_UPDATE = 0x776655cc,
	RESTART_REASON_END = 0xffffffff,
};

#define UPLOAD_MSG_USER_FAULT			"User Fault"
#define UPLOAD_MSG_CRASH_KEY			"Crash Key"
#define UPLOAD_MSG_USER_CRASH_KEY		"User Crash Key"
#define UPLOAD_MSG_LONG_KEY_PRESS		"Long Key Press"
#define UPLOAD_MSG_PF_WD_BITE			"Software Watchdog Timer expired"
#define UPLOAD_MSG_PF_WD_INIT_FAIL		"Platform Watchdog couldnot be initialized"
#define UPLOAD_MSG_PF_WD_RESTART_FAIL		"Platform Watchdog couldnot be restarted"
#define UPLOAD_MSG_PF_WD_KICK_FAIL		"Platform Watchdog can't update sync_cnt"

/* for sec debug level */
#define KERNEL_SEC_DEBUG_LEVEL_LOW	(0x574F4C44)
#define KERNEL_SEC_DEBUG_LEVEL_MID	(0x44494D44)
#define KERNEL_SEC_DEBUG_LEVEL_HIGH	(0x47494844)

#define ANDROID_DEBUG_LEVEL_LOW		0x4f4c
#define ANDROID_DEBUG_LEVEL_MID		0x494d
#define ANDROID_DEBUG_LEVEL_HIGH	0x4948

#define ANDROID_CP_DEBUG_ON		0x5500
#define ANDROID_CP_DEBUG_OFF		0x55ff

#define DUMP_SINK_TO_SDCARD 0x73646364
#define DUMP_SINK_TO_BOOTDEV 0x42544456
#define DUMP_SINK_TO_USB 0x0

#define CDSP_SIGNOFF_BLOCK 0x2377
#define CDSP_SIGNOFF_ON 0x7277

#define LOCAL_CONFIG_PRINT_EXTRA_INFO

typedef enum {
	USER_UPLOAD_CAUSE_MIN = 1,
	USER_UPLOAD_CAUSE_SMPL = USER_UPLOAD_CAUSE_MIN,	/* RESET_REASON_SMPL */
	USER_UPLOAD_CAUSE_WTSR,				/* RESET_REASON_WTSR */
	USER_UPLOAD_CAUSE_WATCHDOG,			/* RESET_REASON_WATCHDOG */
	USER_UPLOAD_CAUSE_PANIC,			/* RESET_REASON_PANIC */
	USER_UPLOAD_CAUSE_MANUAL_RESET,			/* RESET_REASON_MANUAL_RESET */
	USER_UPLOAD_CAUSE_POWER_RESET,			/* RESET_REASON_POWER_RESET */
	USER_UPLOAD_CAUSE_REBOOT,			/* RESET_REASON_REBOOT */
	USER_UPLOAD_CAUSE_BOOTLOADER_REBOOT,		/* RESET_REASON_BOOTLOADER_REBOOT */
	USER_UPLOAD_CAUSE_POWER_ON,			/* RESET_REASON_POWER_ON */
	USER_UPLOAD_CAUSE_THERMAL,			/* RESET_REASON_THERMAL_RESET */
	USER_UPLOAD_CAUSE_CP_CRASH,			/* RESET_REASON_CP_CRASH_RESET */
	USER_UPLOAD_CAUSE_UNKNOWN,			/* RESET_REASON_UNKNOWN */
	USER_UPLOAD_CAUSE_MAX = USER_UPLOAD_CAUSE_UNKNOWN,
} user_upload_cause_t;

#if IS_ENABLED(CONFIG_SEC_DEBUG)
enum pon_restart_reason {
/**********************************************/
/* Following values came from qpnp-power-on.h */
/**********************************************/
	PON_RESTART_REASON_UNKNOWN		= 0x00,
	PON_RESTART_REASON_RECOVERY		= 0x01,
	PON_RESTART_REASON_BOOTLOADER		= 0x02,
	PON_RESTART_REASON_RTC			= 0x03,
	PON_RESTART_REASON_DMVERITY_CORRUPTED	= 0x04,
	PON_RESTART_REASON_DMVERITY_ENFORCE	= 0x05,
	PON_RESTART_REASON_KEYS_CLEAR		= 0x06,
#if (CONFIG_SEC_QPNP_PON_SPARE_BITS == 6)
	/* 32 ~ 63 for OEMs/ODMs secific features */
	PON_RESTART_REASON_OEM_MIN		= 0x20,
	PON_RESTART_REASON_OEM_MAX		= 0x3f,
#endif
/**********************************************/
/*       Followings are Samsung values        */
/**********************************************/
#if (CONFIG_SEC_QPNP_PON_SPARE_BITS == 6)
	/* TODO: DUMP_SINK feature is not used MSM8953 families */
	PON_RESTART_REASON_DUMP_SINK_BOOTDEV	= 0x07,
	PON_RESTART_REASON_DUMP_SINK_SDCARD	= 0x08,
	PON_RESTART_REASON_DUMP_SINK_USB	= 0x09,

	/* TODO : QUEST feature is not used MSM8953 families */
#if IS_ENABLED(CONFIG_SEC_QUEST)
	PON_RESTART_REASON_QUEST_UEFI_START	= 0x0A,
#endif
	PON_RESTART_REASON_FORCE_UPLOAD_ON	= 0x10,
	PON_RESTART_REASON_FORCE_UPLOAD_OFF	= 0x11,
#endif /* CONFIG_SEC_QPNP_PON_SPARE_BITS == 6 */
	PON_RESTART_REASON_CP_CRASH		= 0x12,
	PON_RESTART_REASON_MANUAL_RESET = 0x13,
	PON_RESTART_REASON_NORMALBOOT		= 0x14,
	PON_RESTART_REASON_DOWNLOAD		= 0x15,
	PON_RESTART_REASON_NVBACKUP		= 0x16,
	PON_RESTART_REASON_NVRESTORE		= 0x17,
	PON_RESTART_REASON_NVERASE		= 0x18,
	PON_RESTART_REASON_NVRECOVERY		= 0x19,
#if IS_ENABLED(CONFIG_SEC_PERIPHERAL_SECURE_CHK)
	PON_RESTART_REASON_SECURE_CHECK_FAIL	= 0x1A,
#endif
	PON_RESTART_REASON_WATCH_DOG		= 0x1B,
	PON_RESTART_REASON_KERNEL_PANIC		= 0x1C,
	PON_RESTART_REASON_THERMAL		= 0x1D,
	PON_RESTART_REASON_POWER_RESET		= 0x1E,
	PON_RESTART_REASON_WTSR			= 0x1F,
	/***********************************************/
	PON_RESTART_REASON_RORY_START		= 0x20,
	/* here is reserved for rory download. */
	/* don't use betwwen PON_RESTART_REASON_RORY_START */
	/*   & PON_RESTART_REASON_RORY_END */
	PON_RESTART_REASON_RORY_END		= 0x2A,
	/***********************************************/
	PON_RESTART_REASON_MULTICMD = 0x2B,
	PON_RESTART_REASON_CROSS_FAIL = 0x2C,
	PON_RESTART_REASON_LIMITED_DRAM_SETTING = 0x2D,
	PON_RESTART_REASON_SLT_COMPLETE = 0x2F,
	PON_RESTART_REASON_DBG_LOW		= 0x30,
	PON_RESTART_REASON_DBG_MID		= 0x31,
	PON_RESTART_REASON_DBG_HIGH		= 0x32,
	PON_RESTART_REASON_CP_DBG_ON		= 0x33,
	PON_RESTART_REASON_CP_DBG_OFF		= 0x34,
	PON_RESTART_REASON_CP_MEM_RESERVE_ON	= 0x35,
	PON_RESTART_REASON_CP_MEM_RESERVE_OFF	= 0x36,
	PON_RESTART_REASON_FIRMWAREUPDATE	= 0x37,
#if IS_ENABLED(CONFIG_MUIC_SUPPORT_RUSTPROOF)
	/* SWITCHSEL_MODE use 0x38 ~ 0x3D */
	PON_RESTART_REASON_SWITCHSEL		= 0x38, /* SET_SWITCHSEL_MODE */
	PON_RESTART_REASON_RUSTPROOF		= 0x3D, /* SET_RUSTPROOF_MODE */
#endif
	PON_RESTART_REASON_MBS_MEM_RESERVE_ON	= 0x3E,
	PON_RESTART_REASON_MBS_MEM_RESERVE_OFF	= 0x3F,
#if (CONFIG_SEC_QPNP_PON_SPARE_BITS == 6)
	PON_RESTART_REASON_MAX			= 0x40
#else
#if IS_ENABLED(CONFIG_SEC_ABC)
	PON_RESTART_REASON_USER_DRAM_TEST	= 0x40,
#endif
#if IS_ENABLED(CONFIG_SEC_QUEST)
	PON_RESTART_REASON_QUEST_UEFI_START = 0x41,
#endif
#if IS_ENABLED(CONFIG_SEC_QUEST_UEFI_ENHANCEMENT)
	PON_RESTART_REASON_QUEST_NMCHECKER_START	= 0x42,
	PON_RESTART_REASON_QUEST_NMCHECKER_SMD_START	= 0x43,
#endif
#if IS_ENABLED(CONFIG_SEC_DDR_SKP)
	PON_RESTART_REASON_QUEST_DRAM_START	= 0x44,
	PON_RESTART_REASON_QUEST_DRAM_FAIL	= 0x45,
	PON_RESTART_REASON_QUEST_DRAM_TRAINIG_FAIL	= 0x46,
#endif
	PON_RESTART_REASON_FORCE_UPLOAD_ON	= 0x48,
	PON_RESTART_REASON_FORCE_UPLOAD_OFF	= 0x49,
	PON_RESTART_REASON_DUMP_SINK_BOOTDEV = 0x4D,
	PON_RESTART_REASON_DUMP_SINK_SDCARD = 0x4E,
	PON_RESTART_REASON_DUMP_SINK_USB = 0x4F,
#if IS_ENABLED(CONFIG_SEC_QUEST)
	PON_RESTART_REASON_QUEST_REWORK	= 0x50,
#endif
#if IS_ENABLED(CONFIG_SEC_QUEST_UEFI_USER)
	PON_RESTART_REASON_QUEST_QUEFI_USER_START = 0x51,
	PON_RESTART_REASON_QUEST_SUEFI_USER_START = 0x52,
#endif
 	PON_RESTART_REASON_RECOVERY_UPDATE = 0x60,
 	PON_RESTART_REASON_BOTA_COMPLETE = 0x61,
	PON_RESTART_REASON_CDSP_BLOCK = 0x62,
	PON_RESTART_REASON_CDSP_ON = 0x63,
	PON_RESTART_REASON_MAX			= 0x80
#endif
};
#endif /* CONFIG_SEC_DEBUG */

#if IS_ENABLED(CONFIG_SEC_DEBUG)

#include <asm/sec_debug.h>

DECLARE_PER_CPU(struct sec_debug_core_t, sec_debug_core_reg);
DECLARE_PER_CPU(struct sec_debug_mmu_reg_t, sec_debug_mmu_reg);
DECLARE_PER_CPU(enum sec_debug_upload_cause_t, sec_debug_upload_cause);

extern bool sec_debug_is_enabled(void);
extern unsigned int sec_debug_level(void);
extern void (*sec_nvmem_pon_write)(u8 pon_rr);

static __always_inline void sec_debug_strcpy_task_comm(char *dst, char *src)
{
#if (TASK_COMM_LEN == 16)
	*(unsigned __int128 *)dst = *(unsigned __int128 *)src;
#else
	memcpy(dst, src, TASK_COMM_LEN);
#endif
}

/* called @ arch/arm64/kernel/smp.c */
static inline void sec_debug_save_context(void)
{
	unsigned long flags;
	unsigned int cpu = smp_processor_id();

	local_irq_save(flags);
	sec_debug_save_mmu_reg(&per_cpu(sec_debug_mmu_reg, cpu));
	sec_debug_save_core_reg(&per_cpu(sec_debug_core_reg, cpu));
	pr_emerg("(%s) context saved(CPU:%d)\n", __func__, cpu);
	local_irq_restore(flags);
	flush_cache_all();
}

/* called @ drivers/power/reset/msm-poweroff.c */
extern void sec_debug_update_dload_mode(const int restart_mode, const int in_panic);

/* called @ drivers/power/reset/msm-poweroff.c */
extern void sec_debug_update_restart_reason(const char *cmd, const int in_panic, const int restart_mode);

/* called @ drivers/soc/qcom/watchdog_v2.c */
extern void sec_debug_prepare_for_wdog_bark_reset(void);

/* implemented @ drivers/soc/qcom/watchdog_v2.c */
/* called @ kernel/panic.c */
extern void emerg_pet_watchdog(void);

/* called @ init/main.c */
extern char *sec_debug_get_erased_command_line(void);

/* called @ drivers/debug/sec_debug_summary.c */
extern uint64_t get_pa_dump_sink(void);

/* FIXME: this is only for SAMSUNG internal */
/* called @ drivers/misc/samsung/sec_hw_param.c */
extern void sec_debug_upload_cause_str(enum sec_debug_upload_cause_t type, char *str, size_t len);

/* FIXME: this function is not referenced anywhere */
extern void __deprecated sec_debug_print_model(struct seq_file *m, const char *cpu_name);

/* FIXME: this function is not referenced anywhere */
extern void __deprecated sec_debug_set_thermal_upload(void);

#else /* CONFIG_SEC_DEBUG */

static inline bool sec_debug_is_enabled(void) { return false; }
static inline unsigned int sec_debug_level(void) { return 0; }
static inline void sec_debug_strcpy_task_comm(char *dst, char *src) {}
static inline void sec_debug_save_context(void) {}
static inline void sec_debug_update_dload_mode(const int restart_mode, const int in_panic) {}
static inline void sec_debug_update_restart_reason(const char *cmd, const int in_panic, const int restart_mode) {}
static inline void sec_debug_prepare_for_wdog_bark_reset(void) {}
static inline void emerg_pet_watchdog(void) {}
static inline char *sec_debug_get_erased_command_line(void) { return boot_command_line; }
static inline uint64_t get_pa_dump_sink(void) { return 0ULL; };
static inline void sec_debug_upload_cause_str(enum sec_debug_upload_cause_t type, char *str, size_t len) {}

/* FIXME: __deprected */
static inline void sec_debug_print_model(struct seq_file *m, const char *cpu_name) {}
static inline void sec_debug_set_thermal_upload(void) {}

#endif /* CONFIG_SEC_DEBUG */

#if IS_ENABLED(CONFIG_SEC_PERIPHERAL_SECURE_CHK)
/* called @ drivers/soc/qcom/peripheral-loader.c */
extern void sec_peripheral_secure_check_fail(void);
extern void sec_modem_loading_fail_to_bootloader(void);
#else
static inline void sec_peripheral_secure_check_fail(void) {}
static inline void sec_modem_loading_fail_to_bootloader(void) {}
#endif /* CONFIG_SEC_DEBUG */

#ifdef CONFIG_SEC_SSR_DEBUG_LEVEL_CHK
/* called @ drivers/soc/qcom/subsystem_restart.c */
extern int sec_debug_is_enabled_for_ssr(void);
#else
static inline int sec_debug_is_enabled_for_ssr(void) { return 0; }
#endif /* CONFIG_SEC_SSR_DEBUG_LEVEL_CHK */

#if IS_ENABLED(CONFIG_SEC_DEBUG_PWDT)
/* called @ drivers/soc/qcom/watchdog_v2.c */
extern void sec_debug_check_pwdt(void);
#else
static inline void sec_debug_check_pwdt(void) {}
#endif

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
extern unsigned long sec_delay_check;
#endif
#endif	/* __INDIRECT__SEC_DEBUG_H__ */
