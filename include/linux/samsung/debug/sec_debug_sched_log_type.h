#ifndef __SEC_DEBUG_SCHED_LOG_TYPE_H__
#define __SEC_DEBUG_SCHED_LOG_TYPE_H__

#if !defined(__KERNEL__) && !defined(TASK_COMM_LEN)
#define TASK_COMM_LEN		16
#endif

struct irq_buf {
	u64 time;
	unsigned int irq;
	void *fn;
	char *name;
	int en;
	int preempt_count;
	union {
		void *context;
		irq_hw_number_t hwirq;
	};
	pid_t pid;
	unsigned int entry_exit;
};

struct secure_buf {
	u64 time;
	u32 svc_id, cmd_id;
	pid_t pid;
};

struct irq_exit_buf {
	unsigned int irq;
	u64 time;
	u64 end_time;
	u64 elapsed_time;
	pid_t pid;
};

struct sched_buf {
	u64 time;
	union {
		char comm[TASK_COMM_LEN];
		u64 addr;
	};
	pid_t pid;
	struct task_struct *pTask;
	char prev_comm[TASK_COMM_LEN];
	int prio;
	pid_t prev_pid;
	int prev_prio;
	int prev_state;
};

struct timer_buf {
	u64 time;
	unsigned int type;
	int int_lock;
	void *fn;
	pid_t pid;
};

struct secmsg_buf {
	u64 time;
	char msg[64];
	void *caller0;
	void *caller1;
	char *task;
};

struct secavc_buf {
	char msg[256];
};

struct dcvs_buf {
	u64 time;
	int cpu_no;
	unsigned int prev_freq;
	unsigned int new_freq;
};

struct fuelgauge_debug {
	u64 time;
	unsigned int voltage;
	unsigned short soc;
	unsigned short charging_status;
};

enum {
	CLUSTER_POWER_TYPE,
	CPU_POWER_TYPE,
	CLOCK_RATE_TYPE,
};

struct cluster_power {
	char *name;
	int index;
	unsigned long sync_cpus;
	unsigned long child_cpus;
	bool from_idle;
	int entry_exit;
};

struct cpu_power {
	int index;
	int entry_exit;
	bool success;
};

struct clock_rate {
	char *name;
	u64 state;
	u64 cpu_id;
	int complete;
};

struct power_buf {
	u64 time;
	pid_t pid;
	unsigned int type;
	union {
		struct cluster_power cluster;
		struct cpu_power cpu;
		struct clock_rate clk_rate;
	};
};

#ifdef __KERNEL__

#define SCHED_LOG_MAX			512

#define FG_LOG_MAX			128

struct irq_log {
	unsigned int idx;
	struct irq_buf buf[SCHED_LOG_MAX];
} ____cacheline_aligned_in_smp;

struct irq_exit_log {
	unsigned int idx;
	struct irq_exit_buf buf[SCHED_LOG_MAX];
} ____cacheline_aligned_in_smp;

struct sched_log {
	unsigned int idx;
	struct sched_buf buf[SCHED_LOG_MAX];
} ____cacheline_aligned_in_smp;

struct timer_log {
	unsigned int idx;
	struct timer_buf buf[SCHED_LOG_MAX];
} ____cacheline_aligned_in_smp;

#define TZ_LOG_MAX			64
struct secure_log {
	unsigned int idx;
	struct secure_buf buf[TZ_LOG_MAX];
} ____cacheline_aligned_in_smp;

#define MSG_LOG_MAX			1024
struct secmsg_log {
	unsigned int idx;
	struct secmsg_buf buf[MSG_LOG_MAX];
} ____cacheline_aligned_in_smp;

#define AVC_LOG_MAX			256
struct secavc_log {
	unsigned int idx;
	struct secavc_buf buf[AVC_LOG_MAX];
} ____cacheline_aligned_in_smp;

#define DCVS_LOG_MAX			256
struct dcvs_debug {
	unsigned int idx;
	struct dcvs_buf buf[DCVS_LOG_MAX];
} ____cacheline_aligned_in_smp;

#define POWER_LOG_MAX			256
struct power_log {
	unsigned int idx;
	struct power_buf buf[POWER_LOG_MAX];
} ____cacheline_aligned_in_smp;

#if defined(CONFIG_SEC_DEBUG_SCHED_LOG_PER_CPU)
struct sec_debug_log {
	struct sched_log sched;
	struct irq_log irq;

#ifdef CONFIG_SEC_DEBUG_MSG_LOG
	struct secmsg_log secmsg;
#endif

#ifdef CONFIG_SEC_DEBUG_POWER_LOG
	struct power_log pwr;
#endif
};

struct sec_debug_last_pet_ns {
	/* zwei variables -- last_pet und last_ns */
	unsigned long long last_pet;
	atomic64_t last_ns;
};

#else
struct sec_debug_log {
	struct sched_log sched[NR_CPUS];
	struct irq_log irq[NR_CPUS];
	struct secure_log secure[NR_CPUS];
	struct irq_exit_log irq_exit[NR_CPUS];
	struct timer_log timer[NR_CPUS];

#ifdef CONFIG_SEC_DEBUG_MSG_LOG
	struct secmsg_log secmsg[NR_CPUS];
#endif

#ifdef CONFIG_SEC_DEBUG_AVC_LOG
	struct secavc_log secavc[NR_CPUS];
#endif

#ifdef CONFIG_SEC_DEBUG_DCVS_LOG
	struct dcvs_debug dcvs[NR_CPUS];
#endif

#ifdef CONFIG_SEC_DEBUG_POWER_LOG
	struct power_log pwr[NR_CPUS];
#endif

#ifdef CONFIG_SEC_DEBUG_FUELGAUGE_LOG
	unsigned int fg_log_idx;
	struct fuelgauge_debug fg_log[FG_LOG_MAX];
#endif

	/* zwei variables -- last_pet und last_ns */
	unsigned long long last_pet;
	atomic64_t last_ns;
};
#endif

#endif /* __KERNEL */

#endif /* __SEC_DEBUG_SCHED_LOG_TYPE_H__ */
