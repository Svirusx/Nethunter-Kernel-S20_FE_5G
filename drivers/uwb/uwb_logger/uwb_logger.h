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
#define UWB_LOG_DBG(fmt, ...)		pr_debug(fmt, ##__VA_ARGS__)
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
