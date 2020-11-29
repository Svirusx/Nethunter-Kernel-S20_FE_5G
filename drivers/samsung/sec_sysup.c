/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * Samsung TN debugging code
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#include <linux/sec_param.h>
#include <linux/sec_class.h>

#define SEC_EDTBO_FILENAME		"/spu/edtbo/edtbo.img"
#define EDTBO_FIEMAP_MAGIC 		0x7763
#define EDTBO_FIEMAP_COUNT 		128

static unsigned long edtbo_ver = 0;

static int __init init_sysup_edtbo(char *str)
{
	edtbo_ver = simple_strtoul(str, NULL, 10);

	return 0;
}
early_param("androidboot.edtbo", init_sysup_edtbo);

static ssize_t sec_edtbo_update_store(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   const char *buf, size_t size)
{
	struct fiemap *pfiemap;
	struct fiemap_extent_info fieinfo = { 0, };
	struct file *fp;
	mm_segment_t old_fs;
	struct inode *inode;
	u64 len;
	int update, error;
	u32 allocsize = 0;
	u32 fiemap_magic = EDTBO_FIEMAP_MAGIC;

	error = sscanf(buf, "%d", &update);
	if (error < 0 || update != 1)
		return -EINVAL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(SEC_EDTBO_FILENAME, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		pr_err("file open error\n");
		error = -ENOENT;
		goto fp_err;
	}

	inode = file_inode(fp);
	if (!inode->i_op->fiemap) {
		error = -EOPNOTSUPP;
		goto open_err;
	}

	allocsize = EDTBO_FIEMAP_COUNT * sizeof(struct fiemap_extent) + offsetof(struct fiemap, fm_extents);
	pfiemap = kmalloc(allocsize, GFP_KERNEL | __GFP_ZERO);
	if (!pfiemap) {
		error = -ENOMEM;
		goto open_err;
	}

	pfiemap->fm_length = ULONG_MAX;
	pfiemap->fm_extent_count = EDTBO_FIEMAP_COUNT;

	error = fiemap_check_ranges(inode->i_sb, pfiemap->fm_start, pfiemap->fm_length, &len);
	if (error)
		goto alloc_err;

	fieinfo.fi_flags = pfiemap->fm_flags;
	fieinfo.fi_extents_max = pfiemap->fm_extent_count;
	fieinfo.fi_extents_start = pfiemap->fm_extents;

	error = inode->i_op->fiemap(inode, &fieinfo, pfiemap->fm_start, len);
	if (error)
		goto alloc_err;

	pfiemap->fm_flags = fieinfo.fi_flags;
	pfiemap->fm_mapped_extents = fieinfo.fi_extents_mapped;

	if (!pfiemap->fm_mapped_extents) {
		error = -EFAULT;
		goto alloc_err;
	}

	sec_set_param(param_index_fiemap_result, pfiemap);
	sec_set_param(param_index_fiemap_update, &fiemap_magic);

	error = size;

alloc_err:
	if (pfiemap)
		kfree(pfiemap);
open_err:
	filp_close(fp, NULL);
fp_err:
	set_fs(old_fs);

	return error;
}

static ssize_t sec_edtbo_version_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", edtbo_ver);
}

static struct kobj_attribute sec_sysup_edtbo_update_attr =
	__ATTR(edtbo_update, 0220, NULL, sec_edtbo_update_store);

static struct kobj_attribute sec_sysup_edtbo_version_attr =
	__ATTR(edtbo_version, 0440, sec_edtbo_version_show, NULL);

static struct attribute *sec_sysup_attributes[] = {
	&sec_sysup_edtbo_update_attr.attr,
	&sec_sysup_edtbo_version_attr.attr,
	NULL,
};

static struct attribute_group sec_sysup_attr_group = {
	.attrs = sec_sysup_attributes,
};

static int __init sec_sysup_init(void)
{
	int ret = 0;
	struct device *dev;

	pr_info("start\n");
 
	dev = ___sec_device_create(NULL, "sec_sysup");
	if(!dev)
		pr_err("sec device create failed!\n");

	ret = sysfs_create_group(&dev->kobj, &sec_sysup_attr_group);
	if (ret)
		pr_err("could not create sysfs node\n");

	return 0;
}

static void __exit sec_sysup_exit(void)
{
	pr_info("exit\n");
}

module_init(sec_sysup_init);
module_exit(sec_sysup_exit);

