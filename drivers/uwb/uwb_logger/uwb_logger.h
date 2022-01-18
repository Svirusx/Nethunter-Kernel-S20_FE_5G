#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/ktime.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
#include <linux/sched/clock.h>
#else
#include <linux/sched.h>
#endif


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
#define sec_input_proc_ops(ops_name, ops_owner, read_fn)	\
	const struct proc_ops ops_name = {						\
		.proc_read = read_fn,								\
	}
#else
#define sec_input_proc_ops(ops_name, ops_owner, read_fn)	\
	const struct file_operations ops_name = {				\
		.owner = ops_owner,									\
		.read = read_fn,									\
	}
#endif

#ifndef _UWB_LOGGER_H_
#define _UWB_LOGGER_H_

#ifdef CONFIG_SEC_UWB_LOGGER

#define UWB_LOG_ERR(fmt, ...) \
	do { \
		pr_err(fmt, ##__VA_ARGS__); \
		uwb_logger_print(fmt, ##__VA_ARGS__); \
	} while (0)
#define UWB_LOG_INFO(fmt, ...) \
	do { \
		pr_info(fmt, ##__VA_ARGS__); \
		uwb_logger_print(fmt, ##__VA_ARGS__); \
	} while (0)
#define UWB_LOG_DBG(fmt, ...)		pr_debug(sec-uwb, ##__VA_ARGS__)
#define UWB_LOG_REC(fmt, ...)		uwb_logger_print(fmt, ##__VA_ARGS__)

void uwb_logger_set_max_count(int count);
void uwb_logger_print(const char *fmt, ...);
void uwb_print_hex_dump(void *buf, void *pref, uint32_t size);
int uwb_logger_init(void);

#else /*CONFIG_SEC_UWB_LOGGER*/

#define UWB_LOG_ERR(fmt, ...)		pr_err(fmt, ##__VA_ARGS__)
#define UWB_LOG_INFO(fmt, ...)		pr_info(fmt, ##__VA_ARGS__)
#define UWB_LOG_DBG(fmt, ...)		pr_debug(fmt, ##__VA_ARGS__)
#define UWB_LOG_REC(fmt, ...)		do { } while (0)

#define uwb_print_hex_dump(a, b, c)	do { } while (0)
#define uwb_logger_init()		do { } while (0)
#define uwb_logger_set_max_count(a)	do { } while (0)
#endif

#endif
