#ifndef __SEC_DEBUG_H__
#define __SEC_DEBUG_H__

#include <linux/ftrace.h>

#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
#include <linux/sched/clock.h>
#include <linux/sched/debug.h>
#include <linux/sched/stat.h>
#include <linux/sched/task.h>
#endif

#define __SEC_DEBUG_INDIRECT
#include <linux/samsung/debug/sec_debug.h>
#undef __SEC_DEBUG_INDIRECT

#define __SEC_DEBUG_SCHED_LOG_INDIRECT
#include <linux/samsung/debug/sec_debug_sched_log.h>
#undef __SEC_DEBUG_SCHED_LOG_INDIRECT

#define __SEC_DEBUG_SUMMARY_INDIRECT
#include <linux/samsung/debug/sec_debug_summary.h>
#undef __SEC_DEBUG_SUMMARY_INDIRECT

#define __SEC_DEBUG_USER_RESET_INDIRECT
#include <linux/samsung/debug/sec_debug_user_reset.h>
#undef __SEC_DEBUG_USER_RESET_INDIRECT

#define __SEC_DEBUG_PARTITION_INDIRECT
#include <linux/samsung/debug/sec_debug_partition.h>
#undef __SEC_DEBUG_USER_RESET_INDIRECT

#define __SEC_HW_PARAM_INDIRECT
#include <linux/samsung/debug/sec_hw_param.h>
#undef __SEC_HW_PARAM_INDIRECT

#define __SEC_KERNEL_MODE_NEON_DEBUG_INDIRECT
#include <linux/samsung/debug/sec_kernel_mode_neon_debug.h>
#undef __SEC_KERNEL_MODE_NEON_DEBUG_INDIRECT

#define __SEC_LOG_BUF_INDIRECT
#include <linux/samsung/debug/sec_log_buf.h>
#undef __SEC_LOG_BUF_INDIRECT

#define __SEC_INIT_LOG_INDIRECT
#include <linux/samsung/debug/sec_init_log.h>
#undef __SEC_INIT_LOG_INDIRECT

#define __SEC_SLUB_DEBUG_INDIRECT
#include <linux/samsung/debug/sec_slub_debug.h>
#undef __SEC_SLUB_DEBUG_INDIRECT

#endif /* __SEC_DEBUG_H__ */

