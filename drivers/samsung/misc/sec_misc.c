// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/samsung/misc/sec_misc.c
 *
 * COPYRIGHT(C) 2006-2019 Samsung Electronics Co., Ltd. All Right Reserved.
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

#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/blkdev.h>
#include <linux/of.h>
#include <linux/sec_class.h>

/* TODO: This is a default reigister address of 'QFPROM JTAG' used before
 * 'SM8250' is introdued.
 * Keep this variable for the backward compatibility and in later SoCs,
 * "qfprom_jtag,reg" should be defined in the "samsung,sec_misc" ndoe.
 */
#define QFPROM_CORR_PTE_ROW0_LSB_ID		0x00784130

/* TODO: This value is only valid for SM8150. This variable and some additional
 * properties should be placed in the 'samsung,sec_misc" node.
 *   - msm_power_dou,reg
 *   - msm_power_dou,revision_bit_nr
 *   - msm_power_dou,revision_bit_shift
 *   - msm_power_dou,bins_nr
 *   - msm_power_dou,bins_shift
 */
#define POWER_DOU_ADDR				0x0078013C

static struct device *sec_misc_dev;

union qfprom_jtag_reg {
	uint32_t raw;
	struct {
		uint32_t jtag_id:20;
		uint32_t feature_id:8;
		uint32_t reserved_0:4;
	};
};

static union qfprom_jtag_reg *__qfprom_jtag_reg_get_instance(void)
{
	static union qfprom_jtag_reg qfprom_jtag;
	struct device_node *np;
	uint32_t reg_phys = QFPROM_CORR_PTE_ROW0_LSB_ID;
	void *reg_virt;

	if (likely(qfprom_jtag.raw))
		return &qfprom_jtag;

	np = of_find_node_by_name(NULL, "samsung,sec_misc");
	if (np) {
		uint32_t reg_phys_dt;

		if (!of_property_read_u32(np, "qfprom_jtag,reg", &reg_phys_dt))
			reg_phys = reg_phys_dt;
	}

	reg_virt = ioremap_nocache((phys_addr_t)reg_phys, SZ_4K);
	if (!reg_virt)
		return NULL;

	qfprom_jtag.raw = readl_relaxed(reg_virt);
	iounmap(reg_virt);

	return &qfprom_jtag;
}

static ssize_t msm_feature_id_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	union qfprom_jtag_reg *qfprom_jtag = __qfprom_jtag_reg_get_instance();

	if (!qfprom_jtag) {
		pr_warn("could not map FEATURE_ID address\n");
		return scnprintf(buf, PAGE_SIZE,
				"could not map FEATURE_ID address\n");
	}

	if (!qfprom_jtag->raw)
		return scnprintf(buf, PAGE_SIZE,
				"Feature ID is not supported!!\n");

	pr_debug("FEATURE_ID : 0x%08x\n", qfprom_jtag->feature_id);

	return scnprintf(buf, PAGE_SIZE, "%02u\n", qfprom_jtag->feature_id);
}
static DEVICE_ATTR_RO(msm_feature_id);
/*  End of Feature ID */

#ifdef CONFIG_SEC_FACTORY
static char *cpr_iddq_info;
static int __init cpr_iddq_info_setup(char *str)
{
	if (str)
		cpr_iddq_info = str;
	pr_err("sec_misc: can't get cpr_iddq_info\n");
	return 0;
}
__setup("QCOM.CPR_IDDQ_INFO=", cpr_iddq_info_setup);

static ssize_t msm_apc_avs_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (cpr_iddq_info) {
		pr_debug("msm cpr_iddq_info : %s\n", cpr_iddq_info);
		return scnprintf(buf, PAGE_SIZE, "%s\n", cpr_iddq_info);
	}

	return scnprintf(buf, PAGE_SIZE, "Not Support!\n");
}

static DEVICE_ATTR_RO(msm_apc_avs_info);

struct msm_power_dou {
	uint32_t raw;
	uint32_t revision_bit;
	uint32_t bins;
};

#define __get_bit_field(__raw, __nr, __shift)				\
	((__raw) >> (__shift)) & ((0x1 << (__nr)) - 0x1)

static struct msm_power_dou *__msm_power_dou_get_instance(void)
{
	static struct msm_power_dou power_dou;
	struct device_node *np;
	uint32_t reg_phys = POWER_DOU_ADDR;
	void *reg_virt;
	uint32_t nr;
	uint32_t shift;

	if (likely(power_dou.raw))
		return &power_dou;

	np = of_find_node_by_name(NULL, "samsung,sec_misc");
	if (np) {
		uint32_t reg_phys_dt;

		if (!of_property_read_u32(np, "msm_power_dou,reg", &reg_phys_dt))
			reg_phys = reg_phys_dt;
	}

	reg_virt = ioremap_nocache((phys_addr_t)reg_phys, SZ_4K);
	if (!reg_virt)
		return NULL;

	power_dou.raw = readl_relaxed(reg_virt);
	iounmap(reg_virt);

	/* revision_bit: default - nr = 1 / shift = 31 */
	if (of_property_read_u32(np, "msm_power_dou,revision_bit_nr", &nr))
		nr = 1;
	if (of_property_read_u32(np, "msm_power_dou,revision_bit_shift", &shift))
		shift = 31;
	power_dou.revision_bit = __get_bit_field(power_dou.raw, nr, shift);

	/* bins: default - nr = 2 / shift = 29 */
	if (of_property_read_u32(np, "msm_power_dou,bins_nr", &nr))
		nr = 2;
	if (of_property_read_u32(np, "msm_power_dou,bins_shift", &shift))
		shift = 29;
	power_dou.bins = __get_bit_field(power_dou.raw, nr, shift);

	return &power_dou;
}

static ssize_t msm_dour_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct msm_power_dou *power_dou = __msm_power_dou_get_instance();

	if (!power_dou) {
		pr_err("could not map POWER_DOU address\n");
		return scnprintf(buf, PAGE_SIZE,
				"could not map POWER_DOU address\n");
	}

	if (!power_dou->raw)
		return scnprintf(buf, PAGE_SIZE,
				"POWER DOU is not supported!!\n");

	pr_debug("POWER DOU REVISION BIT : 0x%08x\n", power_dou->revision_bit);

	return scnprintf(buf, PAGE_SIZE, "%02u\n", power_dou->revision_bit);
}

static DEVICE_ATTR_RO(msm_dour);

static ssize_t msm_doub_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct msm_power_dou *power_dou = __msm_power_dou_get_instance();

	if (!power_dou) {
		pr_err("could not map POWER_DOU address\n");
		return scnprintf(buf, PAGE_SIZE,
				"could not map POWER_DOU address\n");
	}

	if (!power_dou->raw)
		return scnprintf(buf, PAGE_SIZE,
				"POWER DOU is not supported!!\n");

	pr_debug("POWER DOU BINS : 0x%08x\n", power_dou->bins);

	return scnprintf(buf, PAGE_SIZE, "%02u\n", power_dou->bins);
}

static DEVICE_ATTR_RO(msm_doub);
#endif

static struct attribute *sec_misc_attrs[] = {
	&dev_attr_msm_feature_id.attr,
#ifdef CONFIG_SEC_FACTORY
	&dev_attr_msm_apc_avs_info.attr,
	&dev_attr_msm_dour.attr,
	&dev_attr_msm_doub.attr,
#endif
	NULL,
};

static const struct attribute_group sec_misc_attr_group = {
	.attrs = sec_misc_attrs,
};

static const struct file_operations sec_misc_fops = {
	.owner = THIS_MODULE,
};

static struct miscdevice sec_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "sec_misc",
	.fops = &sec_misc_fops,
};

static int __init sec_misc_init(void)
{
	int ret;

	ret = misc_register(&sec_misc_device);
	if (unlikely(ret < 0)) {
		pr_err("misc_register failed!\n");
		goto failed_register_misc;
	}

	sec_misc_dev = ___sec_device_create(NULL, "sec_misc");
	if (unlikely(IS_ERR(sec_misc_dev))) {
		pr_err("failed to create device!\n");
		ret = PTR_ERR(sec_misc_dev);
		goto failed_create_device;
	}

	ret = sysfs_create_group(&sec_misc_dev->kobj, &sec_misc_attr_group);
	if (unlikely(ret)) {
		pr_err("Failed to create device files!\n");
		goto failed_create_device_file;
	}

	return 0;

failed_create_device_file:
failed_create_device:
	misc_deregister(&sec_misc_device);
failed_register_misc:
	return ret;
}

module_init(sec_misc_init);

/* Module information */
MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("Samsung misc. driver");
MODULE_LICENSE("GPL");
