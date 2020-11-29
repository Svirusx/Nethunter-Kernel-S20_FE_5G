/*
 * sec_quest_bps.c
 *
 *  Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *  dongmin choi <dm.choi@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/io.h>
#include <linux/sec_class.h>
#include <linux/sec_quest_bps.h>
#include <linux/fs.h>
#include <linux/sec_quest.h>
#include <linux/sec_quest_param.h>
#include <linux/sec_quest_bps_classifier.h>

extern struct bps_info bps_envs;
static bool bps_info_initialized = false;
static int bps_info_table[SEC_BPS_MAX_ID];

static struct device_attribute sec_bps_attrs[] = {
	SEC_BPS_ATTR(rbpsp),
	SEC_BPS_ATTR(rbpsf),
	SEC_BPS_ATTR(n1bps),
	SEC_BPS_ATTR(n2bps),
	SEC_BPS_ATTR(n3bps),
	SEC_BPS_ATTR(n4bps),
};

static int __init get_bps_rbpsp(char *str)
{
	int rbpsp;

	get_option(&str, &rbpsp);
	bps_info_table[SEC_BPS_RBPSP] = rbpsp;

	return 0;
}
early_param("bps.rbpsp", get_bps_rbpsp);

static int __init get_bps_rbpsf(char *str)
{
	int rbpsf;

	get_option(&str, &rbpsf);
	bps_info_table[SEC_BPS_RBPSF] = rbpsf;

	return 0;
}
early_param("bps.rbpsf", get_bps_rbpsf);

static int __init get_bps_n1bps(char *str)
{
	int n1bps;

	get_option(&str, &n1bps);
	bps_info_table[SEC_BPS_N1BPS] = n1bps;

	return 0;
}
early_param("bps.n1bps", get_bps_n1bps);

static int __init get_bps_n2bps(char *str)
{
	int n2bps;

	get_option(&str, &n2bps);
	bps_info_table[SEC_BPS_N2BPS] = n2bps;

	return 0;
}
early_param("bps.n2bps", get_bps_n2bps);

static int __init get_bps_n3bps(char *str)
{
	int n3bps;

	get_option(&str, &n3bps);
	bps_info_table[SEC_BPS_N3BPS] = n3bps;

	return 0;
}
early_param("bps.n3bps", get_bps_n3bps);

static int __init get_bps_n4bps(char *str)
{
	int n4bps;

	get_option(&str, &n4bps);
	bps_info_table[SEC_BPS_N4BPS] = n4bps;

	return 0;
}
early_param("bps.n4bps", get_bps_n4bps);

static void sec_print_bps_info(struct bps_info *bfo, char *info)
{
	printk("%s:%s rbps_magic:0x%x, rbpsp:%d, rbpsf:%d, n1bps:%d, n2bps:%d, n3bps:%d, n4bps:%d\n",
		__func__, info, bfo->rbps_magic, bfo->rbpsp, bfo->rbpsf,
		bfo->n1bps, bfo->n2bps, bfo->n3bps, bfo->n4bps);
	printk("%s:%s fac info:%d\n", __func__, info, bfo->fac_info);
}

static void sec_bps_param_init(void)
{
	printk("%s: initialize\n", __func__);

	quest_load_param_quest_bps_data();

	bps_info_initialized = true;

	sec_print_bps_info(&bps_envs, "init");
}

static void sec_bps_param_update(int idx, int value)
{
	printk("%s: idx = %d, val = %d\n", __func__, idx, value);

	if (bps_info_initialized == false) {
		printk("%s: skip it's not initialized!\n", __func__);
		return;
	}

	sec_print_bps_info(&bps_envs, "before");

	switch(idx) {
	case SEC_BPS_RBPSP:
		printk("%s: SEC_BPS_RBPSP, val = %d\n", __func__, bps_envs.rbpsp);
		bps_envs.rbpsp = value;
		printk("%s: SEC_BPS_RBPSP, val = %d\n", __func__, bps_envs.rbpsp);
		break;
	case SEC_BPS_RBPSF:
		printk("%s: SEC_BPS_RBPSF, val = %d\n", __func__, bps_envs.rbpsf);
		bps_envs.rbpsf = value;
		printk("%s: SEC_BPS_RBPSF, val = %d\n", __func__, bps_envs.rbpsf);
		break;
	case SEC_BPS_N1BPS:
		printk("%s: SEC_BPS_N1BPS, val = %d\n", __func__, bps_envs.n1bps);
		bps_envs.n1bps = value;
		printk("%s: SEC_BPS_N1BPS, val = %d\n", __func__, bps_envs.n1bps);
		break;
	case SEC_BPS_N2BPS:
		printk("%s: SEC_BPS_N2BPS, val = %d\n", __func__, bps_envs.n2bps);
		bps_envs.n2bps = value;
		printk("%s: SEC_BPS_N2BPS, val = %d\n", __func__, bps_envs.n2bps);
		break;
	case SEC_BPS_N3BPS:
		printk("%s: SEC_BPS_N3BPS, val = %d\n", __func__, bps_envs.n3bps);
		bps_envs.n3bps = value;
		printk("%s: SEC_BPS_N3BPS, val = %d\n", __func__, bps_envs.n3bps);
		break;
	case SEC_BPS_N4BPS:
		printk("%s: SEC_BPS_N4BPS, val = %d\n", __func__, bps_envs.n4bps);
		bps_envs.n4bps = value;
		printk("%s: SEC_BPS_N4BPS, val = %d\n", __func__, bps_envs.n4bps);
		break;
	case SEC_BPS_FACINFO:
		if (value > bps_envs.fac_info && value <= SEC_BPS_INFO_FAC_STEP_ASSEMBLED) {
			printk("%s: SEC_BPS_FACINFO, val = %d\n", __func__, bps_envs.fac_info);
			bps_envs.fac_info = value;
			printk("%s: SEC_BPS_FACINFO, val = %d\n", __func__, bps_envs.fac_info);
		} else {
			printk("%s: SEC_BPS_FACINFO, skip update cur[%d], upd[%d]\n",
				__func__, bps_envs.fac_info, value);
		}
		break;
	}

	if (idx != SEC_BPS_FACINFO)
		bps_envs.rbps_magic = QUEST_BPS_INFO_MAGIC;

	quest_sync_param_quest_bps_data();

	sec_print_bps_info(&bps_envs, "after");
}

static void sec_bps_param_work(struct work_struct *work)
{
	struct sec_bps_param *bps_data =
		container_of(work, struct sec_bps_param, sec_bps_work);

	printk("%s: idx = %d, val = %d\n", __func__, bps_data->idx, bps_data->val);

	if (bps_info_initialized == false)
		sec_bps_param_init();

	sec_bps_param_update(bps_data->idx, bps_data->val);

	printk("%s: done\n", __func__);
}

ssize_t sec_bps_show_attrs(struct device *dev,
							struct device_attribute *attr, char *buf)
{
	const ptrdiff_t offset = attr - sec_bps_attrs;
	int i = 0;

	switch (offset) {
	case SEC_BPS_RBPSP:
		printk("%s: SEC_BPS_RBPSP\n", __func__);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", bps_info_table[SEC_BPS_RBPSP]);
		break;
	case SEC_BPS_RBPSF:
		printk("%s: SEC_BPS_RBPSF\n", __func__);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", bps_info_table[SEC_BPS_RBPSF]);
		break;
	case SEC_BPS_N1BPS:
		printk("%s: SEC_BPS_N1BPS\n", __func__);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", bps_info_table[SEC_BPS_N1BPS]);
		break;
	case SEC_BPS_N2BPS:
		printk("%s: SEC_BPS_N2BPS\n", __func__);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", bps_info_table[SEC_BPS_N2BPS]);
		break;
	case SEC_BPS_N3BPS:
		printk("%s: SEC_BPS_N3BPS\n", __func__);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", bps_info_table[SEC_BPS_N3BPS]);
		break;
	case SEC_BPS_N4BPS:
		printk("%s: SEC_BPS_N4BPS\n", __func__);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", bps_info_table[SEC_BPS_N4BPS]);
		break;
	default:
		printk("%s: fail! \n", __func__);
		i = -EINVAL;
		break;
	}
	return i;
}

ssize_t sec_bps_store_attrs(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count)
{
	const ptrdiff_t offset = attr - sec_bps_attrs;
	int ret, val;

	ret = kstrtoint(buf, 0, &val);
	if (ret < 0)
		return -EINVAL;

	printk("%s: ret = %d, val = %d\n", __func__, ret, val);

	switch (offset) {
	case SEC_BPS_RBPSP:
		printk("%s: SEC_BPS_RBPSP\n", __func__);
		sec_bps_param_data.idx = offset;
		sec_bps_param_data.val = val;
		schedule_work(&sec_bps_param_data.sec_bps_work);
		break;
	case SEC_BPS_RBPSF:
		printk("%s: SEC_BPS_RBPSF\n", __func__);
		sec_bps_param_data.idx = offset;
		sec_bps_param_data.val = val;
		schedule_work(&sec_bps_param_data.sec_bps_work);
		break;
	case SEC_BPS_N1BPS:
		printk("%s: SEC_BPS_N1BPS\n", __func__);
		sec_bps_param_data.idx = offset;
		sec_bps_param_data.val = val;
		schedule_work(&sec_bps_param_data.sec_bps_work);
		break;
	case SEC_BPS_N2BPS:
		printk("%s: SEC_BPS_N2BPS\n", __func__);
		sec_bps_param_data.idx = offset;
		sec_bps_param_data.val = val;
		schedule_work(&sec_bps_param_data.sec_bps_work);
		break;
	case SEC_BPS_N3BPS:
		printk("%s: SEC_BPS_N3BPS\n", __func__);
		sec_bps_param_data.idx = offset;
		sec_bps_param_data.val = val;
		schedule_work(&sec_bps_param_data.sec_bps_work);
		break;
	case SEC_BPS_N4BPS:
		printk("%s: SEC_BPS_N4BPS\n", __func__);
		sec_bps_param_data.idx = offset;
		sec_bps_param_data.val = val;
		schedule_work(&sec_bps_param_data.sec_bps_work);
		break;
	default:
		printk("%s: fail! \n", __func__);
		break;
	}

	return count;
}

static int sec_bps_create_attrs(struct device *dev)
{
	int i;
	int rc;

	for (i = 0; i < ARRAY_SIZE(sec_bps_attrs); i++) {
		printk("%s: create %dth file\n", __func__, i+1);
		rc = device_create_file(dev, &sec_bps_attrs[i]);
		if (rc)
			goto create_attr_fail;
	}
	goto create_attr_success;

create_attr_fail:
	while (i--) {
		printk("%s: remove %dth file\n", __func__, i);
		device_remove_file(dev, &sec_bps_attrs[i]);
	}
create_attr_success:
	return rc;
}

static ssize_t store_bps_fac_info(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret, val;

	printk("%s: ++", __func__);

	/* read var and convert to int */
	ret = kstrtoint(buf, 0, &val);
	if (ret < 0)
		return -EINVAL;

	printk("%s: val = %d", __func__, val);

	sec_bps_param_data.idx = SEC_BPS_FACINFO;
	sec_bps_param_data.val = val;
	schedule_work(&sec_bps_param_data.sec_bps_work);

	return count;
}

static ssize_t show_bps_fac_info(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	printk("%s: ++", __func__);

	/* check if param successfully read */
	if (bps_info_initialized == false) {
		printk("%s: skip show bps fac info : not initialized!\n", __func__);
		return sprintf(buf, "NG\n");
	}

	printk("%s: fac info [%d]", __func__, bps_envs.fac_info);

	if (bps_envs.fac_info == SEC_BPS_INFO_FAC_STEP_ASSEMBLED)
		return sprintf(buf, "ASSEMBLED\n");
	else if (bps_envs.fac_info == SEC_BPS_INFO_FAC_STEP_SEMI_ASSEMBLED)
		return sprintf(buf, "SEMI-ASSEMBLED\n");
	else if (bps_envs.fac_info == SEC_BPS_INFO_FAC_STEP_PBA)
		return sprintf(buf, "SMD\n");
	else
 		return sprintf(buf, "FAIL\n");
}

static DEVICE_ATTR(factory, S_IRUGO | S_IWUSR, show_bps_fac_info, store_bps_fac_info);

static int __init sec_bps_init(void)
{
	int ret = 0;
	struct device *sec_bps;

	printk("%s: ++\n", __func__);

	sec_bps = sec_device_create(NULL, "sec_bps");
	if (IS_ERR(sec_bps)) {
		pr_err("%s: failed to create device (sec_bps)\n", __func__);
		return PTR_ERR(sec_bps);
	}

	ret = sec_bps_create_attrs(sec_bps);
	if (ret) {
		pr_err("%s: failed to create attrs (sec_bps)\n", __func__);
		goto err_create_bps_sysfs;
	}

	ret = device_create_file(sec_bps, &dev_attr_factory);
	if (ret) {
		pr_err("%s: failed to create factory attrs (sec_bps)\n", __func__);
		goto err_create_bps_sysfs;
	} else
		printk("%s: support factory bps sys file\n", __func__);

	INIT_WORK(&sec_bps_param_data.sec_bps_work, sec_bps_param_work);

	printk("%s: --\n", __func__);
	return 0;

err_create_bps_sysfs:
	sec_device_destroy(sec_bps->devt);

	return ret;
}

static void __exit sec_bps_exit(void)
{
	printk("%s: exit\n", __func__);
}

module_init(sec_bps_init);
module_exit(sec_bps_exit);
