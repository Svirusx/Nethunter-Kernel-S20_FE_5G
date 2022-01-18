/*
 * driver/misc/samsung/sec_class.c
 *
 * COPYRIGHT(C) 2014-2018 Samsung Electronics Co., Ltd. All Right Reserved.
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

#include <linux/device.h>
#include <linux/err.h>
#include <linux/sec_class.h>
#include <linux/module.h>

/* CAUTION : Do not be declared as external sec_class  */
static struct class *sec_class;
static atomic_t sec_dev;

static int sec_class_match_device_by_name(struct device *dev, const void *data)
{
	const char *name = data;

	return sysfs_streq(name, dev_name(dev));
}

struct device *sec_dev_get_by_name(const char *name)
{
	return class_find_device(sec_class, NULL, name,
			sec_class_match_device_by_name);
}
EXPORT_SYMBOL(sec_dev_get_by_name);

struct device *___sec_device_create(void *drvdata, const char *fmt)
{
	struct device *dev;

	if (unlikely(!sec_class)) {
		pr_err("Not yet created class(sec)!\n");
		BUG();
	}

	if (IS_ERR(sec_class)) {
		pr_err("Failed to create class(sec) %ld\n", PTR_ERR(sec_class));
		BUG();
	}

	dev = device_create(sec_class, NULL, atomic_inc_return(&sec_dev),
			drvdata, "%s", fmt);
	if (IS_ERR(dev))
		pr_err("Failed to create device %s %ld\n", fmt, PTR_ERR(dev));
	else
		pr_debug("%s : %d\n", fmt, dev->devt);

	return dev;
}
EXPORT_SYMBOL(___sec_device_create);

void sec_device_destroy(dev_t devt)
{
	if (unlikely(!devt)) {
		pr_err("Not allowed to destroy dev\n");
	} else {
		pr_info("%d\n", devt);
		device_destroy(sec_class, devt);
	}
}
EXPORT_SYMBOL_GPL(sec_device_destroy);

static int __init sec_class_create(void)
{
	sec_class = class_create(THIS_MODULE, "sec");
	if (unlikely(IS_ERR(sec_class))) {
		pr_err("Failed to create class(sec) %ld\n", PTR_ERR(sec_class));
		return PTR_ERR(sec_class);
	}
	return 0;
}

arch_initcall(sec_class_create);

MODULE_DESCRIPTION("sec-class");
MODULE_LICENSE("GPL v2");
