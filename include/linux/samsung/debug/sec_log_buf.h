#ifndef __SEC_LOG_BUF_INDIRECT
#warning "samsung/debug/sec_log_buf.h is included directly."
#error "please include sec_debug.h instead of this file"
#endif

#ifndef __INDIRECT__SEC_LOG_BUF_H__
#define __INDIRECT__SEC_LOG_BUF_H__

#define SEC_LOG_MAGIC		0x4d474f4c	/* "LOGM" */

struct sec_log_buf {
	uint32_t boot_cnt;
	uint32_t magic;
	uint32_t idx;
	uint32_t prev_idx;
	char buf[];
};

#ifdef CONFIG_SEC_LOG_BUF_NO_CONSOLE
/* called @ kernel/printk/printk.c */
void sec_log_buf_write(const char *s, unsigned int count);
#else
static inline void sec_log_buf_write(const char *s, unsigned int count) {}
#endif

#endif /* __INDIRECT__SEC_LOG_BUF_H__ */
