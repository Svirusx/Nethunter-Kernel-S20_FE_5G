#ifndef __SEC_KCOMPAT_H__
#define __SEC_KCOMPAT_H__

#include <linux/version.h>

#if IS_ENABLED(CONFIG_MSM_SMEM) && IS_ENABLED(CONFIG_QCOM_SMEM)
#error "CONFIG_MSM_SMEM and CONFIG_QCOM_SMEM can not be enabled at the same time"
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)

#define timer_setup(__timer, __fn, __data)	setup_timer(__timer, __fn, __data)

#if IS_ENABLED(CONFIG_MSM_SMEM)

#if defined(QCOM_SMEM_HOST_ANY)
#undef QCOM_SMEM_HOST_ANY
#endif
#define QCOM_SMEM_HOST_ANY	SMEM_ANY_HOST_FLAG

void *qcom_smem_get(unsigned host, unsigned item, size_t *size);
extern phys_addr_t qcom_smem_virt_to_phys(void *p);
#endif /* CONFIG_MSM_SMEM */

#endif /* KERNEL_VERSION(4,14,0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0)

extern unsigned long *bitmap_alloc(unsigned int nbits, gfp_t flags);
extern unsigned long *bitmap_zalloc(unsigned int nbits, gfp_t flags);
	
#endif /* KERNEL_VERSION(4,19,0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)

void qcom_scm_disable_sdi(void);

#endif /* KERNEL_VERSION(5,4,0) */

#endif /* __SEC_KCOMPAT_H__ */
