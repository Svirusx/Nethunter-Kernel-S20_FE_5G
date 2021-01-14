#ifndef __SEC_INIT_LOG_INDIRECT
#warning "samsung/debug/sec_init_log.h is included directly."
#error "please include sec_debug.h instead of this file"
#endif

#ifndef __INDIRECT__SEC_INIT_LOG_H__
#define __INDIRECT__SEC_INIT_LOG_H__

#ifdef CONFIG_SEC_LOG_BUF_NO_CONSOLE
/* called @ kernel/printk/printk.c */
void sec_init_log_buf_write(const char *s, unsigned int count);
#else
static inline void sec_init_log_buf_write(const char *s, unsigned int count) {}
#endif

#endif /* __INDIRECT__SEC_INIT_LOG_H__ */
