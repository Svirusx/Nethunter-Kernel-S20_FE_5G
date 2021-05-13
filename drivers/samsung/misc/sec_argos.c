/*
 * argos.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/pm_qos.h>
#include <linux/reboot.h>
#include <linux/of.h>
#include <linux/gfp.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/cpumask.h>
#include <linux/interrupt.h>

#if defined(CONFIG_ARCH_LAHAINA)
#define ARGOS_VOTING_DDR_CLK
#include <linux/interconnect.h>
#endif

#ifdef CONFIG_CPU_FREQ_LIMIT
#include <linux/cpufreq.h>
#include <linux/cpufreq_limit.h>

#ifndef CONFIG_CPU_FREQ_LIMIT_USERSPACE
#define DVFS_ARGOS_ID CFLM_ARGOS
int set_freq_limit(unsigned long id, unsigned int freq);
#endif

#else
#define DVFS_ARGOS_ID	0
int set_freq_limit(unsigned long id, unsigned int freq)
{
	pr_err("%s is not yet implemented\n", __func__);
	return 0;
}
#endif

#if defined(CONFIG_ARCH_LAHAINA)
#define ARGOS_VOTING_DDR_CLK
#endif

#define ARGOS_NAME "argos"
#define TYPE_SHIFT 4
#define TYPE_MASK_BIT ((1 << TYPE_SHIFT) - 1)

#define LOCK_RELEASE 	0
#define FREQ_UNLOCK		-1
#define SKIP_FREQ_UPDATE	0
#define FREQ_UPDATE 	1

#define CPU_UNLOCK_FREQ -1
#ifdef ARGOS_VOTING_DDR_CLK
#define DDR_UNLOCK_FREQ 0
#endif

//Refer to "include/dt-bindings/interconnect/qcom,lahaina.h"
#define MASTER_APPSS_PROC                              2
#define SLAVE_EBI1                             512
#define BUS_W 4        /* SM8350 DDR Voting('w' for DDR is 4) */
#define MHZ_TO_KBPS(mhz, w) ((uint64_t)mhz * 1000 * w)

static DEFINE_SPINLOCK(argos_irq_lock);
static DEFINE_SPINLOCK(argos_task_lock);
static DEFINE_SPINLOCK(argos_boost_list_lock);

enum {
	THRESHOLD,
	BIG_MIN_FREQ,
	BIG_MAX_FREQ,
	LITTLE_MIN_FREQ,
	LITTLE_MAX_FREQ,
	DDR_FREQ,
	RESERVED,
	TASK_AFFINITY_EN,
	IRQ_AFFINITY_EN,
	SCHED_BOOST_EN,
	ITEM_MAX,
};

enum {
	BOOST_CPU,
#ifdef ARGOS_VOTING_DDR_CLK
	BOOST_DDR,
#endif
	BOOST_MAX
};

struct boost_table {
	unsigned int items[ITEM_MAX];
};

struct argos_task_affinity {
	struct task_struct *p;
	struct cpumask *affinity_cpu_mask;
	struct cpumask *default_cpu_mask;
	struct list_head entry;
};

struct argos_irq_affinity {
	unsigned int irq;
	struct cpumask *affinity_cpu_mask;
	struct cpumask *default_cpu_mask;
	struct list_head entry;
};

struct argos {
	const char *desc;
	struct platform_device *pdev;
	struct boost_table *tables;
	int ntables;
	int prev_level;
	struct list_head task_affinity_list;
	bool task_hotplug_disable;
	struct list_head irq_affinity_list;
	bool irq_hotplug_disable;
	bool hmpboost_enable;
	bool argos_block;
	struct blocking_notifier_head argos_notifier;
	/* protect prev_level, qos, task/irq_hotplug_disable, hmpboost_enable */
	struct mutex level_mutex;
};

struct argos_platform_data {
	struct argos *devices;
	struct device *dev;
	int ndevice;
	struct notifier_block pm_qos_nfb;
	int *boost_list[BOOST_MAX];
	int boost_max[BOOST_MAX];
};

static struct argos_platform_data *argos_pdata;
static int boost_unlock_freq[BOOST_MAX] = {
CPU_UNLOCK_FREQ
#ifdef ARGOS_VOTING_DDR_CLK
, DDR_UNLOCK_FREQ
#endif
};
#ifdef ARGOS_VOTING_DDR_CLK
struct icc_path *path_argos_bw;
int argos_icc_register = 0;
#endif
static int argos_find_index(const char *label)
{
	int i;
	int dev_num = -1;

	if (!argos_pdata) {
		pr_err("argos not initialized\n");
		return -1;
	}

	for (i = 0; i < argos_pdata->ndevice; i++)
		if (strcmp(argos_pdata->devices[i].desc, label) == 0)
			dev_num = i;
	return dev_num;
}

int sec_argos_register_notifier(struct notifier_block *n, char *label)
{
	struct blocking_notifier_head *cnotifier;
	int dev_num;

	dev_num = argos_find_index(label);

	if (dev_num < 0) {
		pr_err("No match found for label: %d", dev_num);
		return -ENODEV;
	}

	cnotifier = &argos_pdata->devices[dev_num].argos_notifier;

	if (!cnotifier) {
		pr_err("argos notifier not found(dev_num:%d)\n", dev_num);
		return -ENXIO;
	}

	pr_info("%pf(dev_num:%d)\n", n->notifier_call, dev_num);

	return blocking_notifier_chain_register(cnotifier, n);
}
EXPORT_SYMBOL(sec_argos_register_notifier);

int sec_argos_unregister_notifier(struct notifier_block *n, char *label)
{
	struct blocking_notifier_head *cnotifier;
	int dev_num;

	dev_num = argos_find_index(label);

	if (dev_num < 0) {
		pr_err("No match found for label: %d", dev_num);
		return -ENODEV;
	}

	cnotifier = &argos_pdata->devices[dev_num].argos_notifier;

	if (!cnotifier) {
		pr_err("argos notifier not found(dev_num:%d)\n", dev_num);
		return -ENXIO;
	}

	pr_info("%pf(dev_num:%d)\n", n->notifier_call, dev_num);

	return blocking_notifier_chain_unregister(cnotifier, n);
}
EXPORT_SYMBOL(sec_argos_unregister_notifier);

static int argos_task_affinity_setup(struct task_struct *p, int dev_num,
				     struct cpumask *affinity_cpu_mask,
				     struct cpumask *default_cpu_mask)
{
	struct argos_task_affinity *this;
	struct list_head *head;

	if (!argos_pdata) {
		pr_err("argos not initialized\n");
		return -ENXIO;
	}

	if (dev_num < 0 || dev_num >= argos_pdata->ndevice) {
		pr_err("dev_num:%d should be dev_num:0 ~ %d in boundary\n",
		       dev_num, argos_pdata->ndevice - 1);
		return -EINVAL;
	}

	head = &argos_pdata->devices[dev_num].task_affinity_list;

	this = kzalloc(sizeof(*this), GFP_ATOMIC);
	if (!this)
		return -ENOMEM;

	this->p = p;
	this->affinity_cpu_mask = affinity_cpu_mask;
	this->default_cpu_mask = default_cpu_mask;

	spin_lock(&argos_task_lock);
	list_add(&this->entry, head);
	spin_unlock(&argos_task_lock);

	return 0;
}

int argos_task_affinity_setup_label(struct task_struct *p, const char *label,
				    struct cpumask *affinity_cpu_mask,
				    struct cpumask *default_cpu_mask)
{
	int dev_num;

	dev_num = argos_find_index(label);

	return argos_task_affinity_setup(p, dev_num, affinity_cpu_mask,
					 default_cpu_mask);
}

static int argos_irq_affinity_setup(unsigned int irq, int dev_num,
				    struct cpumask *affinity_cpu_mask,
				    struct cpumask *default_cpu_mask)
{
	struct argos_irq_affinity *this;
	struct list_head *head;

	if (!argos_pdata) {
		pr_err("argos not initialized\n");
		return -ENXIO;
	}

	if (dev_num < 0 || dev_num >= argos_pdata->ndevice) {
		pr_err("dev_num:%d should be dev_num:0 ~ %d in boundary\n",
		       dev_num, argos_pdata->ndevice - 1);
		return -EINVAL;
	}

	head = &argos_pdata->devices[dev_num].irq_affinity_list;

	this = kzalloc(sizeof(*this), GFP_ATOMIC);
	if (!this)
		return -ENOMEM;

	this->irq = irq;
	this->affinity_cpu_mask = affinity_cpu_mask;
	this->default_cpu_mask = default_cpu_mask;

	spin_lock(&argos_irq_lock);
	list_add(&this->entry, head);
	spin_unlock(&argos_irq_lock);

	return 0;
}

int argos_irq_affinity_setup_label(unsigned int irq, const char *label,
				   struct cpumask *affinity_cpu_mask,
				   struct cpumask *default_cpu_mask)
{
	int dev_num;

	dev_num = argos_find_index(label);

	return argos_irq_affinity_setup(irq, dev_num, affinity_cpu_mask,
			default_cpu_mask);
}

int argos_task_affinity_apply(int dev_num, bool enable)
{
	struct argos_task_affinity *this;
	struct list_head *head;
	int result = 0;
	struct cpumask *mask;
	bool *hotplug_disable;

	head = &argos_pdata->devices[dev_num].task_affinity_list;
	hotplug_disable = &argos_pdata->devices[dev_num].task_hotplug_disable;

	if (list_empty(head)) {
		pr_debug("task_affinity_list is empty\n");
		return result;
	}

	list_for_each_entry(this, head, entry) {
		if (enable) {
			if (!*hotplug_disable)
				*hotplug_disable = true;

			mask = this->affinity_cpu_mask;
		} else {
			if (*hotplug_disable)
				*hotplug_disable = false;

			mask = this->default_cpu_mask;
		}

		result = set_cpus_allowed_ptr(this->p, mask);

		pr_info("%s affinity %s to cpu_mask:0x%X\n",
			this->p->comm,
			(enable ? "enable" : "disable"),
			(int)*mask->bits);
	}

	return result;
}

int argos_irq_affinity_apply(int dev_num, bool enable)
{
	struct argos_irq_affinity *this;
	struct list_head *head;
	int result = 0;
	struct cpumask *mask;
	bool *hotplug_disable;

	head = &argos_pdata->devices[dev_num].irq_affinity_list;
	hotplug_disable = &argos_pdata->devices[dev_num].irq_hotplug_disable;

	if (list_empty(head)) {
		pr_debug("irq_affinity_list is empty\n");
		return result;
	}

	list_for_each_entry(this, head, entry) {
		if (enable) {
			if (!*hotplug_disable)
				*hotplug_disable = true;

			mask = this->affinity_cpu_mask;
		} else {
			if (*hotplug_disable)
				*hotplug_disable = false;

			mask = this->default_cpu_mask;
		}

		result = irq_set_affinity(this->irq, mask);

		pr_info("irq%d affinity %s to cpu_mask:0x%X\n",
			this->irq, (enable ? "enable" : "disable"),
			(int)*mask->bits);
	}

	return result;
}

int argos_hmpboost_apply(int dev_num, bool enable)
{
	bool *hmpboost_enable;

	hmpboost_enable = &argos_pdata->devices[dev_num].hmpboost_enable;

	if (enable) {
		/* disable -> enable */
		if (!*hmpboost_enable) {
			*hmpboost_enable = true;
			pr_info("hmp boost enable [%d]\n", dev_num);
		}
	} else {
		/* enable -> disable */
		if (*hmpboost_enable) {
			*hmpboost_enable = false;
			pr_info("hmp boost disable [%d]\n", dev_num);
		}
	}

	return 0;
}

static int find_max(int dev_type, int *freq, int ndevice){
	int i, max = boost_unlock_freq[dev_type];
	for (i = 0; i < ndevice; i++){
		if(freq[i] > max) max = freq[i];
	}
	
	return max;
}

static int check_update_freq(int boost_type, int dev_type, int target)
{
	int ret = SKIP_FREQ_UPDATE, new_max, prev;

	spin_lock(&argos_boost_list_lock);

	prev = argos_pdata->boost_list[boost_type][dev_type];
	argos_pdata->boost_list[boost_type][dev_type] = target;
	
	new_max = find_max(boost_type, argos_pdata->boost_list[boost_type], \
		argos_pdata->ndevice);
	spin_unlock(&argos_boost_list_lock);
	
	if(new_max > argos_pdata->boost_max[boost_type] \
		|| (prev == argos_pdata->boost_max[boost_type] \
			&& new_max != argos_pdata->boost_max[boost_type])){
		argos_pdata->boost_max[boost_type] = new_max;
		ret = FREQ_UPDATE;
	}
	return ret;
}

static void argos_freq_lock(int type, int level)
{
	unsigned int big_min_freq, little_min_freq;
	int target_freq, need_update;
	struct boost_table *t = &argos_pdata->devices[type].tables[level];
	const char *cname;

	cname = argos_pdata->devices[type].desc;

	if(level != FREQ_UNLOCK){
		t = &argos_pdata->devices[type].tables[level];
		big_min_freq = t->items[BIG_MIN_FREQ];
		little_min_freq = t->items[LITTLE_MIN_FREQ];
	}
	
	if(level != FREQ_UNLOCK)
		target_freq = (big_min_freq > little_min_freq) ?
				big_min_freq : little_min_freq;	
	else
		target_freq = boost_unlock_freq[BOOST_CPU];

	need_update = check_update_freq(BOOST_CPU, type, target_freq);
	if(need_update != SKIP_FREQ_UPDATE){
		pr_info("update cpu freq %d\n", argos_pdata->boost_max[BOOST_CPU]);
		set_freq_limit(DVFS_ARGOS_ID, argos_pdata->boost_max[BOOST_CPU]);
	}
#ifdef ARGOS_VOTING_DDR_CLK	
	if(level != FREQ_UNLOCK)
		target_freq = t->items[DDR_FREQ];
	else
		target_freq = boost_unlock_freq[BOOST_DDR];
	
	need_update = check_update_freq(BOOST_DDR, type, target_freq);
	if(need_update != SKIP_FREQ_UPDATE){
		pr_info("update ddr freq %d\n", argos_pdata->boost_max[BOOST_DDR]);
		icc_set_bw(path_argos_bw, 0, MHZ_TO_KBPS(argos_pdata->boost_max[BOOST_DDR], BUS_W));
	}
#endif
}

void argos_block_enable(char *req_name, bool set)
{
	int dev_num;
	struct argos *cnode;

	dev_num = argos_find_index(req_name);

	if (dev_num < 0) {
		pr_err("No match found for label: %s", req_name);
		return;
	}

	cnode = &argos_pdata->devices[dev_num];

	if (set) {
		cnode->argos_block = true;
		mutex_lock(&cnode->level_mutex);
		argos_freq_lock(dev_num, FREQ_UNLOCK);
		argos_task_affinity_apply(dev_num, 0);
		argos_irq_affinity_apply(dev_num, 0);
		argos_hmpboost_apply(dev_num, 0);
		cnode->prev_level = -1;
		mutex_unlock(&cnode->level_mutex);
	} else {
		cnode->argos_block = false;
	}
	pr_info("req_name:%s block:%d\n",
		req_name, cnode->argos_block);
}

static int argos_cpuidle_reboot_notifier(struct notifier_block *this,
					 unsigned long event, void *_cmd)
{
	switch (event) {
	case SYSTEM_POWER_OFF:
	case SYS_RESTART:
		pr_info("called\n");
		pm_qos_remove_notifier(PM_QOS_NETWORK_THROUGHPUT,
				       &argos_pdata->pm_qos_nfb);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block argos_cpuidle_reboot_nb = {
	.notifier_call = argos_cpuidle_reboot_notifier,
};

#ifdef ARGOS_VOTING_DDR_CLK	
static void get_icc_path(void)
{
	struct device *dev = argos_pdata->dev;
	int bus_ret = 0;
	
	path_argos_bw = icc_get(dev, MASTER_APPSS_PROC, SLAVE_EBI1);
	if (IS_ERR(path_argos_bw)) {
		bus_ret = PTR_ERR(path_argos_bw);
		dev_err(dev, "Failed to get path_argos_bw. ret=%d\n", bus_ret);
		if (bus_ret != -EPROBE_DEFER)
			dev_err(dev, "Failed to get icc path. ret=%d\n", bus_ret);
	} else {
		dev_info(dev, "Success to get path_argos_bw.\n");
		argos_icc_register = 1;
	}

}
#endif

static int argos_pm_qos_notify(struct notifier_block *nfb,
			       unsigned long speedtype, void *arg)

{
	int type, level, prev_level;
	unsigned long speed;
	bool argos_blocked;
	struct argos *cnode;

	type = (speedtype & TYPE_MASK_BIT) - 1;
	if (type < 0 || type > argos_pdata->ndevice) {
		pr_err("There is no type for devices type[%d], ndevice[%d]\n",
		       type, argos_pdata->ndevice);
		return NOTIFY_BAD;
	}
	
#ifdef ARGOS_VOTING_DDR_CLK	
	if (argos_icc_register == 0){
		get_icc_path();
	}
#endif

	speed = speedtype >> TYPE_SHIFT;
	cnode = &argos_pdata->devices[type];

	prev_level = cnode->prev_level;

	pr_debug("name:%s, speed:%ldMbps\n", cnode->desc, speed);

	argos_blocked = cnode->argos_block;

	/* Find proper level */
	for (level = 0; level < cnode->ntables; level++) {
		struct boost_table *t = &cnode->tables[level];

		if (speed < t->items[THRESHOLD]) {
			break;
		} else if (argos_pdata->devices[type].ntables == level) {
			level++;
			break;
		}
	}

	/* decrease 1 level to match proper table */
	level--;
	if (!argos_blocked) {
		if (level != prev_level) {
			if (mutex_trylock(&cnode->level_mutex) == 0) {
			/*
			 * If the mutex is already locked, it means this argos
			 * is being blocked or is handling another change.
			 * We don't need to wait.
			 */
				pr_warn("skip name:%s, speed:%ldMbps, prev level:%d, request level:%d\n",
					cnode->desc, speed, prev_level, level);
				goto out;
			}
			pr_info("name:%s, speed:%ldMbps, prev level:%d, request level:%d\n",
				cnode->desc, speed, prev_level, level);

			if (level == FREQ_UNLOCK) {
				if (cnode->argos_notifier.head) {
					pr_debug("Call argos notifier(%s lev:%d)\n",
						 cnode->desc, level);
					blocking_notifier_call_chain(&cnode->argos_notifier,
								     speed, NULL);
				}
				argos_freq_lock(type, FREQ_UNLOCK);
				argos_task_affinity_apply(type, 0);
				argos_irq_affinity_apply(type, 0);
				argos_hmpboost_apply(type, 0);
			} else {
				unsigned int enable_flag;

				argos_freq_lock(type, level);
				/* FIXME should control affinity and hmp boost */

				enable_flag = argos_pdata->devices[type].tables[level].items[TASK_AFFINITY_EN];
				argos_task_affinity_apply(type, enable_flag);
				enable_flag = argos_pdata->devices[type].tables[level].items[IRQ_AFFINITY_EN];
				argos_irq_affinity_apply(type, enable_flag);
				enable_flag =
					argos_pdata->devices[type].tables[level].items[SCHED_BOOST_EN];
				argos_hmpboost_apply(type, enable_flag);

				if (cnode->argos_notifier.head) {
					pr_debug("Call argos notifier(%s lev:%d)\n",
						 cnode->desc, level);
					blocking_notifier_call_chain(&cnode->argos_notifier,
								     speed, NULL);
				}
			}

			cnode->prev_level = level;
			mutex_unlock(&cnode->level_mutex);
		} else {
			pr_debug("same level (%d) is requested", level);
		}
	}
out:
	return NOTIFY_OK;
}

#ifdef CONFIG_OF
static int argos_parse_dt(struct device *dev)
{
	struct argos_platform_data *pdata = dev->platform_data;
	struct argos *cnode;
	struct device_node *np, *cnp;
	int device_count = 0, num_level = 0;
	int retval = 0, i, j;

	np = dev->of_node;
	pdata->ndevice = of_get_child_count(np);
	if (!pdata->ndevice)
		return -ENODEV;

	pdata->devices = devm_kzalloc(dev, sizeof(struct argos) * pdata->ndevice, GFP_KERNEL);
	if (!pdata->devices)
		return -ENOMEM;

	for_each_child_of_node(np, cnp) {
		cnode = &pdata->devices[device_count];
		cnode->desc = of_get_property(cnp, "net_boost,label", NULL);
		if (of_property_read_u32(cnp, "net_boost,table_size", &num_level)) {
			dev_err(dev, "Failed to get table size: node not exist\n");
			retval = -EINVAL;
			goto err_out;
		}
		cnode->ntables = num_level;

		/* Allocation for freq and time table */
		if (!cnode->tables) {
			cnode->tables = devm_kzalloc(dev,
						     sizeof(struct boost_table) * cnode->ntables, GFP_KERNEL);
			if (!cnode->tables) {
				retval = -ENOMEM;
				goto err_out;
			}
		}

		/* Get and add frequency and time table */
		for (i = 0; i < num_level; i++) {
			for (j = 0; j < ITEM_MAX; j++) {
				retval = of_property_read_u32_index(cnp, "net_boost,table",
								    i * ITEM_MAX + j, &cnode->tables[i].items[j]);
				if (retval) {
					dev_err(dev, "Failed to get property\n");
					retval = -EINVAL;
					goto err_out;
				}
			}
		}

		INIT_LIST_HEAD(&cnode->task_affinity_list);
		INIT_LIST_HEAD(&cnode->irq_affinity_list);
		cnode->task_hotplug_disable = false;
		cnode->irq_hotplug_disable = false;
		cnode->hmpboost_enable = false;
		cnode->argos_block = false;
		cnode->prev_level = -1;
		mutex_init(&cnode->level_mutex);

		BLOCKING_INIT_NOTIFIER_HEAD(&cnode->argos_notifier);

		device_count++;
	}

	return 0;

err_out:
	return retval;
}
#endif

static int argos_probe(struct platform_device *pdev)
{
	int i, j, ret = 0;
	struct argos_platform_data *pdata;

	pr_info("Start probe\n");
	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev,
				     sizeof(struct argos_platform_data),
				     GFP_KERNEL);

		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate platform data\n");
			return -ENOMEM;
		}

		pdev->dev.platform_data = pdata;

		ret = argos_parse_dt(&pdev->dev);

		if (ret) {
			dev_err(&pdev->dev, "Failed to parse dt data\n");
			return ret;
		}
		pr_info("parse dt done\n");
		
		for(i = 0; i < BOOST_MAX; i++){
			pdata->boost_list[i] = devm_kzalloc(&pdev->dev, sizeof(int) * pdata->ndevice, GFP_KERNEL);
			if (!pdata->boost_list[i]) {
				dev_err(&pdev->dev, "Failed to allocate boosting frequency list\n");
				return -ENOMEM;
			}

			for (j = 0; j < pdata->ndevice; j++){
				pdata->boost_list[i][j] = boost_unlock_freq[i];	
			}
			pdata->boost_max[i] = boost_unlock_freq[i];
		}
	} else {
		pdata = pdev->dev.platform_data;
	}

	if (!pdata) {
		dev_err(&pdev->dev, "There are no platform data\n");
		return -EINVAL;
	}

	if (!pdata->ndevice || !pdata->devices) {
		dev_err(&pdev->dev, "There are no devices\n");
		return -EINVAL;
	}
	

	pdata->pm_qos_nfb.notifier_call = argos_pm_qos_notify;
	pm_qos_add_notifier(PM_QOS_NETWORK_THROUGHPUT, &pdata->pm_qos_nfb);
	register_reboot_notifier(&argos_cpuidle_reboot_nb);
	argos_pdata = pdata;
	argos_pdata->dev = &pdev->dev;
	platform_set_drvdata(pdev, pdata);

	return 0;
}

static int argos_remove(struct platform_device *pdev)
{
	struct argos_platform_data *pdata = platform_get_drvdata(pdev);

	if (!pdata || !argos_pdata)
		return 0;
	pm_qos_remove_notifier(PM_QOS_NETWORK_THROUGHPUT, &pdata->pm_qos_nfb);
	unregister_reboot_notifier(&argos_cpuidle_reboot_nb);
#ifdef ARGOS_VOTING_DDR_CLK	
	if (argos_icc_register == 1)
		icc_put(path_argos_bw);
#endif
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id argos_dt_ids[] = {
	{ .compatible = "samsung,argos"},
	{ }
};
#endif

static struct platform_driver argos_driver = {
	.driver = {
		.name = ARGOS_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table	= of_match_ptr(argos_dt_ids),
#endif
	},
	.probe = argos_probe,
	.remove = argos_remove
};

static int __init argos_init(void)
{
	return platform_driver_register(&argos_driver);
}

static void __exit argos_exit(void)
{
	return platform_driver_unregister(&argos_driver);
}
late_initcall(argos_init);
module_exit(argos_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SAMSUNG Electronics");
MODULE_DESCRIPTION("ARGOS DEVICE");
