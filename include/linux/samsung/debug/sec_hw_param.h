#ifndef __SEC_HW_PARAM_INDIRECT
#warning "samsung/debug/sec_hw_param.h is included directly."
#error "please include sec_debug.h instead of this file"
#endif

#ifndef __INDIRECT__SEC_HW_PARAM_H__
#define __INDIRECT__SEC_HW_PARAM_H__

#ifdef CONFIG_SEC_DEBUG
/* called @ drivers/battery_v2/sec_battery.c */
extern void battery_last_dcvs(int cap, int volt, int temp, int curr);
#else
static inline void battery_last_dcvs(int cap, int volt, int temp, int curr) {}
#endif

#endif /* __INDIRECT__SEC_HW_PARAM_H__ */
