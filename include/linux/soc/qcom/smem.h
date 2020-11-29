/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __QCOM_SMEM_H__
#define __QCOM_SMEM_H__

#define QCOM_SMEM_HOST_ANY -1

int qcom_smem_alloc(unsigned host, unsigned item, size_t size);
void *qcom_smem_get(unsigned host, unsigned item, size_t *size);

int qcom_smem_get_free_space(unsigned host);

phys_addr_t qcom_smem_virt_to_phys(void *p);

#ifdef CONFIG_SEC_DEBUG
#define SMEM_ID_VENDOR0		134
#define SMEM_ID_VENDOR1		135
#define SMEM_ID_VENDOR2		136
#endif

#endif
