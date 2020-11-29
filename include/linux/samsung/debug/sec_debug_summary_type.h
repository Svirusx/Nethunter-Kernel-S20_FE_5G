#ifndef __SEC_DEBUG_SUMMARY_TYPE_H__
#define __SEC_DEBUG_SUMMARY_TYPE_H__

#define SEC_DEBUG_SUMMARY_MAGIC0 0xFFFFFFFF
#define SEC_DEBUG_SUMMARY_MAGIC1 0x5ECDEB6
#define SEC_DEBUG_SUMMARY_MAGIC2 0x14F014F0
 /* high word : major version
  * low word : minor version
  * minor version changes should not affect the Boot Loader behavior
  */
#define SEC_DEBUG_SUMMARY_MAGIC3 0x00060000

struct core_reg_info {
	char name[12];
	uint64_t value;
};

struct sec_debug_summary_excp {
	char type[16];
	char task[16];
	char file[32];
	int line;
	char msg[256];
	struct core_reg_info core_reg[64];
};

struct sec_debug_summary_excp_apss {
	char pc_sym[64];
	char lr_sym[64];
	char panic_caller[64];
	char panic_msg[128];
	char thread[32];
	char timeout_subsys[2][16];
};

struct sec_debug_summary_log {
	uint64_t idx_paddr;
	uint64_t log_paddr;
	uint64_t size;
};

struct sec_debug_summary_kernel_log {
	uint64_t first_idx_paddr;
	uint64_t next_idx_paddr;
	uint64_t log_paddr;
	uint64_t size_paddr;
};

struct rgb_bit_info {
	unsigned char r_off;
	unsigned char r_len;
	unsigned char g_off;
	unsigned char g_len;
	unsigned char b_off;
	unsigned char b_len;
	unsigned char a_off;
	unsigned char a_len;
};

struct var_info {
	char name[32];
	int sizeof_type;
	uint64_t var_paddr;
}__attribute__((aligned(32)));
struct sec_debug_summary_simple_var_mon {
	int idx;
	struct var_info var[32];
};

struct sec_debug_summary_fb {
	uint64_t fb_paddr;
	int xres;
	int yres;
	int bpp;
	struct rgb_bit_info rgb_bitinfo;
};

struct sec_debug_summary_sched_log {
	uint64_t sched_idx_paddr;
	uint64_t sched_buf_paddr;
	unsigned int sched_struct_buf_sz;
	unsigned int sched_struct_log_sz;
	unsigned int sched_array_cnt;
	uint64_t irq_idx_paddr;
	uint64_t irq_buf_paddr;
	unsigned int irq_struct_buf_sz;
	unsigned int irq_struct_log_sz;
	unsigned int irq_array_cnt;
	uint64_t secure_idx_paddr;
	uint64_t secure_buf_paddr;
	unsigned int secure_struct_buf_sz;
	unsigned int secure_struct_log_sz;
	unsigned int secure_array_cnt;
	uint64_t irq_exit_idx_paddr;
	uint64_t irq_exit_buf_paddr;
	unsigned int irq_exit_struct_buf_sz;
	unsigned int irq_exit_struct_log_sz;
	unsigned int irq_exit_array_cnt;
	uint64_t msglog_idx_paddr;
	uint64_t msglog_buf_paddr;
	unsigned int msglog_struct_buf_sz;
	unsigned int msglog_struct_log_sz;
	unsigned int msglog_array_cnt;
};

struct __log_struct_info {
	unsigned int buffer_offset;
	unsigned int w_off_offset;
	unsigned int head_offset;
	unsigned int size_offset;
	unsigned int size_t_typesize;
};
struct __log_data {
	uint64_t log_paddr;
	uint64_t buffer_paddr;
};
struct sec_debug_summary_logger_log_info {
	struct __log_struct_info stinfo;
	struct __log_data main;
	struct __log_data system;
	struct __log_data events;
	struct __log_data radio;
};
struct sec_debug_summary_data {
	unsigned int magic;
	char name[16];
	char state[16];
	struct sec_debug_summary_log log;
	struct sec_debug_summary_excp excp;
	struct sec_debug_summary_simple_var_mon var_mon;
};

struct sec_debug_summary_data_modem {
	unsigned int magic;
	char name[16];
	char state[16];
	struct sec_debug_summary_log log;
	struct sec_debug_summary_excp excp;
	struct sec_debug_summary_simple_var_mon var_mon;
	unsigned int separate_debug;
};

struct sec_debug_summary_avc_log {
	uint64_t secavc_idx_paddr;
	uint64_t secavc_buf_paddr;
	uint64_t secavc_struct_sz;
	uint64_t secavc_array_cnt;
};

struct sec_debug_summary_ksyms {
	uint32_t magic;
	uint32_t kallsyms_all;
	uint64_t addresses_pa;
	uint64_t names_pa;
	uint64_t num_syms;
	uint64_t token_table_pa;
	uint64_t token_index_pa;
	uint64_t markers_pa;
	struct ksect {
		uint64_t sinittext;
		uint64_t einittext;
		uint64_t stext;
		uint64_t etext;
		uint64_t end;
	} sect;
	uint64_t relative_base;
	uint64_t offsets_pa;
};

struct basic_type_int {
	uint64_t pa;	/* physical address of the variable */
	uint32_t size;	/* size of basic type. eg sizeof(unsigned long) goes here */
	uint32_t count;	/* for array types */
};

struct member_type {
	uint16_t size;
	uint16_t offset;
};

typedef struct member_type member_type_int;
typedef struct member_type member_type_long;
typedef struct member_type member_type_longlong;
typedef struct member_type member_type_ptr;
typedef struct member_type member_type_str;

struct struct_thread_info {
	uint32_t struct_size;
	member_type_long flags;
	member_type_ptr task;
	member_type_int cpu;
	member_type_long rrk;
};

struct struct_task_struct {
	uint32_t struct_size;
	member_type_long state;
	member_type_long exit_state;
	member_type_ptr stack;
	member_type_int flags;
	member_type_int on_cpu;
	member_type_int cpu;
	member_type_int pid;
	member_type_str comm;
	member_type_ptr tasks_next;
	member_type_ptr thread_group_next;
	member_type_long fp;
	member_type_long sp;
	member_type_long pc;
	member_type_long sched_info__pcount;
	member_type_longlong sched_info__run_delay;
	member_type_longlong sched_info__last_arrival;
	member_type_longlong sched_info__last_queued;
};

struct ropp_info {
	uint64_t magic;
	uint64_t master_key_pa;
	uint64_t master_key_val;
};

struct irq_stack_info {
	uint64_t pcpu_stack; /* IRQ_STACK_PTR(0) */
	uint64_t size; /* IRQ_STACK_SIZE */
	uint64_t start_sp; /* IRQ_STACK_START_SP */
};

struct sec_debug_summary_task {
	uint64_t stack_size; /* THREAD_SIZE */
	uint64_t start_sp; /* TRHEAD_START_SP */
	struct struct_thread_info ti;
	struct struct_task_struct ts;
	uint64_t init_task;
	struct irq_stack_info irq_stack;
	struct ropp_info ropp;
};

struct rtb_state_struct {
	uint32_t struct_size;
	member_type_int rtb_phys;
	member_type_int nentries;
	member_type_int size;
	member_type_int enabled;
	member_type_int initialized;
	member_type_int step_size;
};

struct rtb_entry_struct {
	uint32_t struct_size;
	member_type_int log_type;
	member_type_int idx;
	member_type_int caller;
	member_type_int data;
	member_type_int timestamp;
	member_type_int cycle_count;
};

struct sec_debug_summary_iolog {
	uint64_t rtb_state_pa;
	struct rtb_state_struct rtb_state;
	struct rtb_entry_struct rtb_entry;
	uint64_t rtb_pcpu_idx_pa;
};

struct sec_debug_summary_kconst {
	uint64_t nr_cpus;
	struct basic_type_int per_cpu_offset;
	uint64_t phys_offset;
	uint64_t page_offset;
	uint64_t va_bits;
	uint64_t kimage_vaddr;
	uint64_t kimage_voffset;
	uint64_t swapper_pg_dir_paddr;
	int vmap_stack;
};


struct sec_debug_summary_msm_dump_info {
	uint64_t cpu_data_paddr;
	uint64_t cpu_buf_paddr;
	uint32_t cpu_ctx_size; //2048
	uint32_t offset; //0x10

	//this_cpu_offset = cpu_ctx_size*cpu+offset
	// x0 = *((unsigned long long *)(cpu_buf_paddr + this_cpu_offset) + 0x0)
	// x1 = *((unsigned long long *)(cpu_buf_paddr + this_cpu_offset) + 0x1)
	// ...
};

struct sec_debug_summary_cpu_context {
	uint64_t sec_debug_core_reg_paddr;
	struct sec_debug_summary_msm_dump_info msm_dump_info;
};

struct struct_aplpm_state {
	uint64_t p_cci;
	uint32_t num_clusters;
	uint64_t p_cluster;
	uint32_t dstate_offset;
	uint64_t p_runqueues;
	uint32_t cstate_offset;
};

struct sec_debug_summary_coreinfo {
	uint64_t coreinfo_data;
	uint64_t coreinfo_size;
	uint64_t coreinfo_note;
};

struct sec_debug_summary_data_apss {
	char name[16];
	char state[16];
	char mdmerr_info[128];
	int nr_cpus;
	struct sec_debug_summary_kernel_log log;
	struct sec_debug_summary_excp_apss excp;
	struct sec_debug_summary_simple_var_mon var_mon;
	struct sec_debug_summary_simple_var_mon info_mon;
	struct sec_debug_summary_fb fb_info;
	struct sec_debug_summary_sched_log sched_log;
	struct sec_debug_summary_logger_log_info logger_log;
	struct sec_debug_summary_avc_log avc_log;
	union {
		struct msm_dump_data ** tz_core_dump;
		uint64_t _tz_core_dump;
	};
	struct sec_debug_summary_ksyms ksyms;
	struct sec_debug_summary_kconst kconst;
	struct sec_debug_summary_iolog iolog;
	struct sec_debug_summary_cpu_context cpu_reg;
	struct sec_debug_summary_task task;
	struct struct_aplpm_state aplpm;
	uint64_t msm_memdump_paddr;
	uint64_t dump_sink_paddr;
	struct sec_debug_summary_coreinfo coreinfo;
};

struct sec_debug_summary_private {
	struct sec_debug_summary_data_apss apss;
	struct sec_debug_summary_data rpm;
	struct sec_debug_summary_data_modem modem;
	struct sec_debug_summary_data dsps;
};

struct sec_debug_summary {
	unsigned int magic[4];
	union {
		struct sec_debug_summary_data_apss *apss;
		uint64_t _apss;
	};
	union {
		struct sec_debug_summary_data *rpm;
		uint64_t _rpm;
	};

	union {
		struct sec_debug_summary_data_modem *modem;
		uint64_t _modem;
	};
	union {
		struct sec_debug_summary_data *dsps;
		uint64_t _dsps;
	};

	uint64_t secure_app_start_addr;
	uint64_t secure_app_size;
	struct sec_debug_summary_private priv;
};

#endif	/* __SEC_DEBUG_SUMMARY_TYPE_H__ */
