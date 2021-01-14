// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/samsung/debug/sec_test_node.c
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

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>

#include <linux/sec_debug.h>

#include "sec_debug_internal.h"

#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "sec_debug."

static long *g_allocated_phys_mem;
static long *g_allocated_virt_mem;

static int sec_alloc_virtual_mem(const char *val, const struct kernel_param *kp)
{
	long *mem;
	char *str = (char *) val;
	size_t size = (size_t)memparse(str, &str);

	if (size) {
		mem = vmalloc(size);
		if (mem) {
			pr_info("Allocated virtual memory of size: 0x%zx bytes\n",
					size);
			*mem = (long)g_allocated_virt_mem;
			g_allocated_virt_mem = mem;

			return 0;
		}

		pr_err("Failed to allocate virtual memory of size: 0x%zx bytes\n",
				size);

		return -ENOMEM;
	}

	pr_info("Invalid size: %s bytes\n", val);

	return -EAGAIN;
}
module_param_call(alloc_virtual_mem, &sec_alloc_virtual_mem, NULL, NULL, 0644);

static int sec_free_virtual_mem(const char *val, const struct kernel_param *kp)
{
	long *mem;
	char *str = (char *) val;
	size_t free_count = (size_t)memparse(str, &str);

	if (!free_count) {
		if (strncmp(val, "all", 4)) {
			free_count = 10;
		} else {
			pr_err("Invalid free count: %s\n", val);
			return -EAGAIN;
		}
	}

	if (free_count > 10)
		free_count = 10;

	if (!g_allocated_virt_mem) {
		pr_err("No virtual memory chunk to free.\n");
		return 0;
	}

	while (g_allocated_virt_mem && free_count--) {
		mem = (long *) *g_allocated_virt_mem;
		vfree(g_allocated_virt_mem);
		g_allocated_virt_mem = mem;
	}

	pr_info("Freed previously allocated virtual memory chunks.\n");

	if (g_allocated_virt_mem)
		pr_info("Still, some virtual memory chunks are not freed. Try again.\n");

	return 0;
}
module_param_call(free_virtual_mem, &sec_free_virtual_mem, NULL, NULL, 0644);

static int sec_alloc_physical_mem(const char *val,
				  const struct kernel_param *kp)
{
	long *mem;
	char *str = (char *) val;
	size_t size = (size_t)memparse(str, &str);

	if (size) {
		mem = kmalloc(size, GFP_KERNEL);
		if (mem) {
			pr_info("Allocated physical memory of size: 0x%zx bytes\n",
				size);
			*mem = (long) g_allocated_phys_mem;
			g_allocated_phys_mem = mem;

			return 0;
		}

		pr_err("Failed to allocate physical memory of size: 0x%zx bytes\n",
			       size);

		return -ENOMEM;
	}

	pr_info("Invalid size: %s bytes\n", val);

	return -EAGAIN;
}
module_param_call(alloc_physical_mem, &sec_alloc_physical_mem,
		NULL, NULL, 0644);

static int sec_free_physical_mem(const char *val, const struct kernel_param *kp)
{
	long *mem;
	char *str = (char *) val;
	size_t free_count = (size_t)memparse(str, &str);

	if (!free_count) {
		if (strncmp(val, "all", 4)) {
			free_count = 10;
		} else {
			pr_info("Invalid free count: %s\n", val);
			return -EAGAIN;
		}
	}

	if (free_count > 10)
		free_count = 10;

	if (!g_allocated_phys_mem) {
		pr_info("No physical memory chunk to free.\n");
		return 0;
	}

	while (g_allocated_phys_mem && free_count--) {
		mem = (long *) *g_allocated_phys_mem;
		kfree(g_allocated_phys_mem);
		g_allocated_phys_mem = mem;
	}

	pr_info("Freed previously allocated physical memory chunks.\n");

	if (g_allocated_phys_mem)
		pr_info("Still, some physical memory chunks are not freed. Try again.\n");

	return 0;
}
module_param_call(free_physical_mem, &sec_free_physical_mem, NULL, NULL, 0644);

static int dbg_set_cpu_affinity(const char *val, const struct kernel_param *kp)
{
	char *endptr;
	pid_t pid;
	int cpu;
	struct cpumask mask;
	long ret;

	pid = (pid_t)memparse(val, &endptr);
	if (*endptr != '@') {
		pr_info("invalid input strin: %s\n", val);
		return -EINVAL;
	}

	cpu = (int)memparse(++endptr, &endptr);
	cpumask_clear(&mask);
	cpumask_set_cpu(cpu, &mask);
	pr_info("Setting %d cpu affinity to cpu%d\n", pid, cpu);

	ret = sched_setaffinity(pid, &mask);
	pr_info("sched_setaffinity returned %ld\n", ret);

	return 0;
}
module_param_call(setcpuaff, &dbg_set_cpu_affinity, NULL, NULL, 0644);

static unsigned int enable_user = 1;
module_param_named(enable_user, enable_user, uint, 0644);

static void sec_user_fault_dump(void)
{
	if (sec_debug_is_enabled() && enable_user)
		panic(UPLOAD_MSG_USER_FAULT);
}

static ssize_t sec_user_fault_write(struct file *file,
		const char __user *buffer, size_t count, loff_t *offs)
{
	char buf[100];

	if (count > sizeof(buf) - 1)
		return -EINVAL;

	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	buf[count] = '\0';
	if (!strncmp(buf, "dump_user_fault", strlen("dump_user_fault")))
		sec_user_fault_dump();

	return count;
}

static const struct file_operations sec_user_fault_proc_fops = {
	.write = sec_user_fault_write,
};

static int __init sec_debug_user_fault_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("user_fault", 0220, NULL,
			&sec_user_fault_proc_fops);
	if (!entry)
		return -ENOMEM;

	return 0;
}
device_initcall(sec_debug_user_fault_init);

static char ap_serial_from_cmdline[20];

static int __init ap_serial_setup(char *str)
{
	snprintf(ap_serial_from_cmdline,
		 sizeof(ap_serial_from_cmdline), "%s", &str[2]);
	return 1;
}
__setup("androidboot.ap_serial=", ap_serial_setup);

static struct bus_type chip_id_subsys = {
	.name = "chip-id",
	.dev_name = "chip-id",
};

static ssize_t ap_svc_serial_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(ap_serial_from_cmdline),
			"%s\n", ap_serial_from_cmdline);
}

static struct kobj_attribute sysfs_SVC_AP_attr =
	__ATTR(SVC_AP, 0444, ap_svc_serial_show, NULL);

static ssize_t ap_serial_show(struct device *dev,
		struct device_attribute *dev_attr, char *buf)
{
	return snprintf(buf, sizeof(ap_serial_from_cmdline),
			"%s\n", ap_serial_from_cmdline);
}

static struct device_attribute chipid_unique_id_attr =
	__ATTR(unique_id, 0444, ap_serial_show, NULL);

static struct attribute *chip_id_attrs[] = {
	&chipid_unique_id_attr.attr,
	NULL,
};

static struct attribute_group chip_id_attr_group = {
	.attrs = chip_id_attrs,
};

static const struct attribute_group *chip_id_attr_groups[] = {
	&chip_id_attr_group,
	NULL,
};

static int sec_debug_ap_serial_init(void)
{
	int ret;
	struct kernfs_node *svc_sd;
	struct kobject *svc;
	struct kobject *AP;

	/* create /sys/devices/system/chip-id/unique_id */
	ret = subsys_system_register(&chip_id_subsys, chip_id_attr_groups);
	if (ret)
		pr_err("Failed to register subsystem-%s\n",
						chip_id_subsys.name);

	/* To find /sys/devices/svc/ */
	svc = kobject_create_and_add("svc", &devices_kset->kobj);
	if (IS_ERR_OR_NULL(svc)) {
		svc_sd = sysfs_get_dirent(devices_kset->kobj.sd, "svc");
		if (!svc_sd) {
			pr_err("Failed to create sys/devices/svc\n");
			return -ENOENT;
		}
		svc = (struct kobject *)svc_sd->priv;
	}

	/* create /sys/devices/svc/AP */
	AP = kobject_create_and_add("AP", svc);
	if (IS_ERR_OR_NULL(AP)) {
		pr_err("Failed to create sys/devices/svc/AP\n");
		goto error_create_AP;
	}

	/* create /sys/devices/svc/AP/SVC_AP */
	ret = sysfs_create_file(AP, &sysfs_SVC_AP_attr.attr);
	if (ret) {
		pr_err("sysfs create fail-%s\n", sysfs_SVC_AP_attr.attr.name);
		goto error_create_sysfs;
	}

	return 0;

error_create_sysfs:
	kobject_put(AP);
error_create_AP:
	kobject_put(svc);

	return -ENOENT;
}
arch_initcall_sync(sec_debug_ap_serial_init);
