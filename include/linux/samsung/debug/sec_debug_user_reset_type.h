#ifndef __SEC_DEBUG_USER_RESET_TYPE_H__
#define __SEC_DEBUG_USER_RESET_TYPE_H__

enum extra_info_dbg_type {
	DBG_0_RESERBED= 0,
	DBG_1_UFS_ERR,
	DBG_2_RESERVED,
	DBG_3_RESERVED,
	DBG_4_RESERVED,
	DBG_5_RESERVED,
	DBG_MAX,
};

#define EXINFO_UPLOAD_CAUSE_STR_SIZE		64
#define EXINFO_RPM_LOG_SIZE			50
#define EXINFO_TZ_LOG_SIZE			40
#define EXINFO_HYP_LOG_SIZE			460

typedef struct {
	unsigned int esr;
	char str[52];
	u64 var1;
	u64 var2;
	u64 pte[6];
} ex_info_fault_t;

extern ex_info_fault_t ex_info_fault[NR_CPUS];

typedef struct {
	char dev_name[24];
	u32 fsr;
	u32 fsynr0;
	u32 fsynr1;
	unsigned long iova;
	unsigned long far;
	char mas_name[24];
	u8 cbndx;
	phys_addr_t phys_soft;
	phys_addr_t phys_atos;
	u32 sid;
} ex_info_smmu_t;

typedef struct {
	int reason;
	char handler_str[24];
	unsigned int esr;
	char esr_str[32];
} ex_info_badmode_t;

typedef struct {
	u64 ktime;
	u32 extc_idx;
	u32 upload_cause;
	int cpu;
	char task_name[TASK_COMM_LEN];
	char bug_buf[64];
	char panic_buf[64];
	ex_info_smmu_t smmu;
	ex_info_fault_t fault[NR_CPUS];
	ex_info_badmode_t badmode;
	char pc[64];
	char lr[64];
	char ufs_err[23];
	u32 lpm_state[NR_CPUS];
	u64 lr_val[NR_CPUS];
	u64 pc_val[NR_CPUS];
	u32 pko;
	char backtrace[0];
} _kern_ex_info_t;

typedef struct {
	u32 magic;
	u32 reserved;
	char str[EXINFO_UPLOAD_CAUSE_STR_SIZE];
} upload_cause_exinfo_t;

typedef struct {
	u64 nsec;
	u32 arg[4];
	char msg[EXINFO_RPM_LOG_SIZE];
} __rpm_log_t;

typedef struct {
		u32 magic;
		u32 ver;
		u32 nlog;
		__rpm_log_t log[5];
} _rpm_ex_info_t;

typedef struct {
	_rpm_ex_info_t info;
} rpm_exinfo_t;

typedef struct {
	u8 cpu_status[NR_CPUS];
	char msg[EXINFO_TZ_LOG_SIZE];
} tz_exinfo_t;

typedef struct {
	u32 esr;
	u32 ear0;
	u32 esr_sdi;
	u32 ear0_sdi;
} pimem_exinfo_t;

typedef struct {
	s64 cpu[NR_CPUS];
} cpu_stuck_exinfo_t ;

typedef struct {
	int s2_fault_counter;
	char msg[EXINFO_HYP_LOG_SIZE];
} hyp_exinfo_t;

#define EXINFO_UPLOAD_CAUSE_DATA_SIZE		sizeof(upload_cause_exinfo_t)
#define EXINFO_RPM_DATA_SIZE			sizeof(rpm_exinfo_t)
#define EXINFO_TZ_DATA_SIZE			sizeof(tz_exinfo_t)
#define EXINFO_PIMEM_DATA_SIZE			sizeof(pimem_exinfo_t)
#define EXINFO_CPU_STUCK_DATA_SIZE		sizeof(cpu_stuck_exinfo_t)
#define EXINFO_HYP_DATA_SIZE			sizeof(hyp_exinfo_t)
#define EXINFO_SUBSYS_DATA_SIZE			(EXINFO_UPLOAD_CAUSE_DATA_SIZE + EXINFO_RPM_DATA_SIZE + EXINFO_TZ_DATA_SIZE + EXINFO_PIMEM_DATA_SIZE + EXINFO_CPU_STUCK_DATA_SIZE + EXINFO_HYP_DATA_SIZE)
#define EXINFO_KERNEL_SPARE_SIZE		(2048 - EXINFO_SUBSYS_DATA_SIZE)
#define EXINFO_KERNEL_DEFAULT_SIZE		2048

typedef union {
	_kern_ex_info_t info;
	char ksize[EXINFO_KERNEL_DEFAULT_SIZE + EXINFO_KERNEL_SPARE_SIZE];
} kern_exinfo_t ;

#define RPM_EX_INFO_MAGIC 0x584D5052

typedef struct {
	kern_exinfo_t kern_ex_info;
	upload_cause_exinfo_t uc_ex_info;
	rpm_exinfo_t rpm_ex_info;
	tz_exinfo_t tz_ex_info;
	pimem_exinfo_t pimem_info;
	cpu_stuck_exinfo_t cpu_stuck_info;
	hyp_exinfo_t hyp_ex_info;
} rst_exinfo_t;

/* rst_exinfo_t sec_debug_reset_ex_info; */

#define SEC_DEBUG_EX_INFO_SIZE	(sizeof(rst_exinfo_t))		/* 4KB */

enum debug_reset_header_state {
	DRH_STATE_INIT=0,
	DRH_STATE_VALID,
	DRH_STATE_INVALID,
	DRH_STATE_MAX,
};

struct debug_reset_header {
	uint32_t magic;
	uint32_t write_times;
	uint32_t read_times;
	uint32_t ap_klog_idx;
	uint32_t summary_size;
	uint32_t stored_tzlog;
	uint32_t fac_write_times;
	uint32_t auto_comment_size;
	uint32_t reset_history_valid;
	uint32_t reset_history_cnt;
};

#define TZ_DIAG_LOG_MAGIC 0x747a6461 /* tzda */

/* copy from drivers/firmware/qcom/tz_log.c */
#define TZBSP_MAX_CPU_COUNT			0x08
#define TZBSP_DIAG_NUM_OF_VMID			16
#define TZBSP_DIAG_VMID_DESC_LEN		7
#define TZBSP_DIAG_INT_NUM			64
#define TZBSP_MAX_INT_DESC			16

#define TZBSP_AES_256_ENCRYPTED_KEY_SIZE	256
#define TZBSP_NONCE_LEN				12
#define TZBSP_TAG_LEN				16

enum tz_boot_info_cpu_status_type {
	TZ_BOOT_INFO_NONE = 0,
	RUNNING,
	POWER_COLLAPSED,
	WARM_BOOTING,
	INVALID_WARM_ENTRY_EXIT_COUNT,
	INVALID_WARM_TERM_ENTRY_EXIT_COUNT,
};

/*
 * VMID Table
 */
struct tzdbg_vmid_t {
	uint8_t vmid; /* Virtual Machine Identifier */
	uint8_t desc[TZBSP_DIAG_VMID_DESC_LEN];	/* ASCII Text */
};

/*
 * Boot Info Table
 */
struct tzdbg_boot_info_t {
	uint32_t wb_entry_cnt;	/* Warmboot entry CPU Counter */
	uint32_t wb_exit_cnt;	/* Warmboot exit CPU Counter */
	uint32_t pc_entry_cnt;	/* Power Collapse entry CPU Counter */
	uint32_t pc_exit_cnt;	/* Power Collapse exit CPU counter */
	uint32_t warm_jmp_addr;	/* Last Warmboot Jump Address */
	uint32_t spare;	/* Reserved for future use. */
};

/*
 * Boot Info Table for 64-bit
 */
struct tzdbg_boot_info64_t {
	uint32_t wb_entry_cnt;  /* Warmboot entry CPU Counter */
	uint32_t wb_exit_cnt;   /* Warmboot exit CPU Counter */
	uint32_t pc_entry_cnt;  /* Power Collapse entry CPU Counter */
	uint32_t pc_exit_cnt;   /* Power Collapse exit CPU counter */
	uint32_t psci_entry_cnt;/* PSCI syscall entry CPU Counter */
	uint32_t psci_exit_cnt;   /* PSCI syscall exit CPU Counter */
	uint64_t warm_jmp_addr; /* Last Warmboot Jump Address */
	uint32_t warm_jmp_instr; /* Last Warmboot Jump Address Instruction */
};

/*
 * Reset Info Table
 */
struct tzdbg_reset_info_t {
	uint32_t reset_type;	/* Reset Reason */
	uint32_t reset_cnt;	/* Number of resets occurred/CPU */
};

/*
 * Interrupt Info Table
 */
struct tzdbg_int_t {
	/*
	 * Type of Interrupt/exception
	 */
	uint16_t int_info;
	/*
	 * Availability of the slot
	 */
	uint8_t avail;
	/*
	 * Reserved for future use
	 */
	uint8_t spare;
	/*
	 * Interrupt # for IRQ and FIQ
	 */
	uint32_t int_num;
	/*
	 * ASCII text describing type of interrupt e.g:
	 * Secure Timer, EBI XPU. This string is always null terminated,
	 * supporting at most TZBSP_MAX_INT_DESC characters.
	 * Any additional characters are truncated.
	 */
	uint8_t int_desc[TZBSP_MAX_INT_DESC];
	uint32_t int_count[TZBSP_MAX_CPU_COUNT]; /* # of times seen per CPU */
};

/*
 * Interrupt Info Table used in tz version >=4.X
 */
struct tzdbg_int_t_tz40 {
	uint16_t int_info;
	uint8_t avail;
	uint8_t spare;
	uint32_t int_num;
	uint8_t int_desc[TZBSP_MAX_INT_DESC];
	uint32_t int_count[TZBSP_MAX_CPU_COUNT]; /* uint32_t in TZ ver >= 4.x*/
};

/* warm boot reason for cores */
struct tzbsp_diag_wakeup_info_t {
	/* Wake source info : APCS_GICC_HPPIR */
	uint32_t HPPIR;
	/* Wake source info : APCS_GICC_AHPPIR */
	uint32_t AHPPIR;
};

/*
 * Log ring buffer position
 */
struct tzdbg_log_pos_t {
	uint16_t wrap;
	uint16_t offset;
};

 /*
  * Log ring buffer
  */
struct tzdbg_log_t {
	struct tzdbg_log_pos_t	log_pos;
	/* open ended array to the end of the 4K IMEM buffer */
	uint8_t					log_buf[];
};

#define TZBSP_DIAG_VERSION_V9_2     0x00090002

struct tzdbg_log_pos_v9_2_t {
	uint32_t wrap;
	uint32_t offset;
};

struct tzdbg_log_v9_2_t {
	struct tzdbg_log_pos_v9_2_t       log_pos;
	/* open ended array to the end of the 4K IMEM buffer */
	uint8_t                                 log_buf[];
};

/*
 * Diagnostic Table
 * Note: This is the reference data structure for tz diagnostic table
 * supporting TZBSP_MAX_CPU_COUNT, the real diagnostic data is directly
 * copied into buffer from i/o memory.
 */
struct tzdbg_t {
	uint32_t magic_num;
	uint32_t version;
	/*
	 * Number of CPU's
	 */
	uint32_t cpu_count;
	/*
	 * Offset of VMID Table
	 */
	uint32_t vmid_info_off;
	/*
	 * Offset of Boot Table
	 */
	uint32_t boot_info_off;
	/*
	 * Offset of Reset info Table
	 */
	uint32_t reset_info_off;
	/*
	 * Offset of Interrupt info Table
	 */
	uint32_t int_info_off;
	/*
	 * Ring Buffer Offset
	 */
	uint32_t ring_off;
	/*
	 * Ring Buffer Length
	 */
	uint32_t ring_len;

	/* Offset for Wakeup info */
	uint32_t wakeup_info_off;

	/*
	 * VMID to EE Mapping
	 */
	struct tzdbg_vmid_t vmid_info[TZBSP_DIAG_NUM_OF_VMID];
	/*
	 * Boot Info
	 */
	struct tzdbg_boot_info64_t  boot_info[TZBSP_MAX_CPU_COUNT];
	/*
	 * Reset Info
	 */
	struct tzdbg_reset_info_t reset_info[TZBSP_MAX_CPU_COUNT];
	uint32_t num_interrupts;
	struct tzdbg_int_t  int_info[TZBSP_DIAG_INT_NUM];

	/* Wake up info */
	struct tzbsp_diag_wakeup_info_t  wakeup_info[TZBSP_MAX_CPU_COUNT];

	uint8_t key[TZBSP_AES_256_ENCRYPTED_KEY_SIZE];

	uint8_t nonce[TZBSP_NONCE_LEN];

	uint8_t tag[TZBSP_TAG_LEN];

	/*
	 * We need at least 2K for the ring buffer
	 */
	struct tzdbg_log_t ring_buffer;	/* TZ Ring Buffer */
};

#endif /* __SEC_DEBUG_USER_RESET_TYPE_H__ */
