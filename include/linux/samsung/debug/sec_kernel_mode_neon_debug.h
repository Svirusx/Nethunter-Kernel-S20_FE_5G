#ifndef __SEC_KERNEL_MODE_NEON_DEBUG_INDIRECT
#warning "samsung/debug/sec_kernel_mode_neon_debug.h is included directly."
#error "please include sec_debug.h instead of this file"
#endif

#ifndef __INDIRECT__SEC_KERNEL_MODE_NEON_DEBUG_H__
#define __INDIRECT__SEC_KERNEL_MODE_NEON_DEBUG_H__

#ifdef CONFIG_KERNEL_MODE_NEON_DEBUG
/* called @ arch/arm64/kernel/fpsimd.c */
extern void (*fpsimd_context_check)(struct task_struct *next);

#else
static inline void fpsimd_context_check(struct task_struct *next) {}
#endif

#endif /* __INDIRECT__SEC_KERNEL_MODE_NEON_DEBUG_H__ */
