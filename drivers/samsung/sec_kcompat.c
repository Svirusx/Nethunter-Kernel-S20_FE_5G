// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/samsung/sec_kcompat.c
 *
 * COPYRIGHT(C) 2019 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/bitmap.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>

#if IS_ENABLED(CONFIG_MSM_SMEM)
#include <soc/qcom/smem.h>
#endif

#include "sec_kcompat.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
#if IS_ENABLED(CONFIG_MSM_SMEM)
void * __weak qcom_smem_get(unsigned host, unsigned item, size_t *size)
{
	void * ret;
	unsigned int size_tmp = 0;

	ret = smem_get_entry(item, &size_tmp, SMEM_APPS, host);
	*size = (size_t)size_tmp;

	return ret;
}

phys_addr_t __weak qcom_smem_virt_to_phys(void *p)
{
	return smem_virt_to_phys(p) & 0xFFFFFFFFULL;
}
#endif /* CONFIG_MSM_SMEM */
#endif /* KERNEL_VERSION(4,14,0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0)
unsigned long * __weak bitmap_alloc(unsigned int nbits, gfp_t flags)
{
	return kmalloc_array(BITS_TO_LONGS(nbits), sizeof(unsigned long),
			flags);
}

unsigned long * __weak bitmap_zalloc(unsigned int nbits, gfp_t flags)
{
	return bitmap_alloc(nbits, flags | __GFP_ZERO);
}
#endif /* KERNEL_VERSION(4,19,0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)

#include <soc/qcom/scm.h>

#define SCM_WDOG_DEBUG_BOOT_PART 0x9

void __weak qcom_scm_disable_sdi(void)
{
	int ret;
	struct scm_desc desc = {
		.args[0] = 1,
		.args[1] = 0,
		.arginfo = SCM_ARGS(2),
	};

	/* Needed to bypass debug image on some chips */
	ret = scm_call2_atomic(SCM_SIP_FNID(SCM_SVC_BOOT,
			  SCM_WDOG_DEBUG_BOOT_PART), &desc);
	if (ret)
		pr_err("Failed to disable wdog debug: %d\n", ret);
}
#endif

MODULE_DESCRIPTION("sec-kcompat");
MODULE_LICENSE("GPL v2");
