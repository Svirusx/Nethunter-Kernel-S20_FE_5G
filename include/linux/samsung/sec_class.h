#ifndef __SEC_CLASS_H__
#define __SEC_CLASS_H__

#include <linux/version.h>

#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
extern struct device *sec_dev_get_by_name(const char *name);
extern void sec_device_destroy(dev_t devt);
extern struct device *___sec_device_create(void *drvdata, const char *fmt);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0)
#define sec_device_create(__devt, __drvdata, __fmt)			\
	___sec_device_create(__drvdata, __fmt)
#else
#define sec_device_create(__drvdata, __fmt)				\
	___sec_device_create(__drvdata, __fmt)
#endif
#else /* CONFIG_DRV_SAMSUNG */
static inline struct device *sec_dev_get_by_name(const char *name) { return NULL; }
static inline void sec_device_destroy(dev_t devt) {};
#define sec_device_create(...)	(NULL)
#endif /* CONFIG_DRV_SAMSUNG */

#endif	/* __SEC_CLASS_H__ */
