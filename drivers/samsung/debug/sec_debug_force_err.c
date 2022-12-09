// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/samsung/debug/sec_debug_force_err.c
 *
 * COPYRIGHT(C) 2017-2019 Samsung Electronics Co., Ltd. All Right Reserved.
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

#include <linux/version.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/preempt.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/gfp.h>

#include <linux/cpumask.h>
#include <linux/cpu.h>
#include <linux/irq.h>
#include <linux/slab.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
#include <linux/qcom_scm.h>
#else
#include <soc/qcom/scm.h>
#endif

#include <linux/sec_debug.h>

#include "sec_debug_internal.h"

#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "sec_debug."

/* timeout for dog bark/bite */
#define DELAY_TIME 20000

static int force_error(const char *val, const struct kernel_param *kp);
module_param_call(force_error, force_error, NULL, NULL, 0644);

static void __simulate_apps_wdog_bark(void)
{
	unsigned long time_out_jiffies;

	pr_emerg("Simulating apps watch dog bark\n");
	local_irq_disable();
	time_out_jiffies = jiffies + msecs_to_jiffies(DELAY_TIME);
	while (time_is_after_jiffies(time_out_jiffies))
		udelay(1);
	local_irq_enable();
	/* if we reach here, simulation failed */
	pr_emerg("Simulation of apps watch dog bark failed\n");
}

static void __simulate_apps_wdog_bite(void)
{
	unsigned long time_out_jiffies;
#ifdef CONFIG_HOTPLUG_CPU
	unsigned int cpu;

	for_each_online_cpu(cpu) {
		BUG_ON(cpu >= NR_CPUS);

		if (cpu == 0)
			continue;
		cpu_down(cpu);
	}
#endif
	pr_emerg("Simulating apps watch dog bite\n");
	local_irq_disable();
	time_out_jiffies = jiffies + msecs_to_jiffies(DELAY_TIME);
	while (time_is_after_jiffies(time_out_jiffies))
		udelay(1);
	local_irq_enable();
	/* if we reach here, simulation had failed */
	pr_emerg("Simualtion of apps watch dog bite failed\n");
}

#if defined(CONFIG_SEC_DEBUG_SEC_WDOG_BITE) || \
	defined(CONFIG_SEC_USER_RESET_DEBUG_TEST)
static void __simulate_secure_wdog_bite(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
	qcom_scm_sec_wdog_trigger();
#else
#define SCM_SVC_SEC_WDOG_TRIG	0x8
	struct scm_desc desc = {
		.args[0] = 0,
		.arginfo = SCM_ARGS(1),
	};

	scm_call2(SCM_SIP_FNID(SCM_SVC_BOOT,
			SCM_SVC_SEC_WDOG_TRIG), &desc);
#endif
	/* if we hit, scm_call has failed */
	pr_emerg("simulation of secure watch dog bite failed\n");
	__simulate_apps_wdog_bite();
}
#endif

#if defined(CONFIG_ARCH_MSM8226) || defined(CONFIG_ARCH_MSM8974)
/*
 * Misc data structures needed for simulating bus timeout in
 * camera
 */

#define HANG_ADDRESS 0xfda10000

struct clk_pair {
	const char *dev;
	const char *clk;
};

static struct clk_pair bus_timeout_camera_clocks_on[] = {
	/*
	 * gcc_mmss_noc_cfg_ahb_clk should be on but right
	 * now this clock is on by default and not accessable.
	 * Update this table if gcc_mmss_noc_cfg_ahb_clk is
	 * ever not enabled by default!
	 */
	{
		.dev = "fda0c000.qcom,cci",
		.clk = "camss_top_ahb_clk",
	},
	{
		.dev = "fda10000.qcom,vfe",
		.clk = "iface_clk",
	},
};

static struct clk_pair bus_timeout_camera_clocks_off[] = {
	{
		.dev = "fda10000.qcom,vfe",
		.clk = "camss_vfe_vfe_clk",
	}
};

static void bus_timeout_clk_access(struct clk_pair bus_timeout_clocks_off[],
				struct clk_pair bus_timeout_clocks_on[],
				int off_size, int on_size)
{
	size_t i;

	/*
	 * Yes, none of this cleans up properly but the goal here
	 * is to trigger a hang which is going to kill the rest of
	 * the system anyway
	 */

	for (i = 0; i < on_size; i++) {
		struct clk *this_clock;

		this_clock = clk_get_sys(bus_timeout_clocks_on[i].dev,
					bus_timeout_clocks_on[i].clk);
		if (!IS_ERR(this_clock))
			if (clk_prepare_enable(this_clock))
				pr_warn("Device %s: Clock %s not enabled",
					bus_timeout_clocks_on[i].clk,
					bus_timeout_clocks_on[i].dev);
	}

	for (i = 0; i < off_size; i++) {
		struct clk *this_clock;

		this_clock = clk_get_sys(bus_timeout_clocks_off[i].dev,
					 bus_timeout_clocks_off[i].clk);
		if (!IS_ERR(this_clock))
			clk_disable_unprepare(this_clock);
	}
}

static void simulate_bus_hang(void)
{
	/* This simulates bus timeout on camera */
	int ret;
	uint32_t dummy_value;
	uint32_t address = HANG_ADDRESS;
	void *hang_address;
	struct regulator *r;
	unsigned long time_out_jiffies;

	/* simulate */
	hang_address = ioremap(address, SZ_4K);
	r = regulator_get(NULL, "gdsc_vfe");
	ret = IS_ERR(r);
	if (!ret)
		regulator_enable(r);
	else
		pr_emerg("Unable to get regulator reference\n");

	bus_timeout_clk_access(bus_timeout_camera_clocks_off,
				bus_timeout_camera_clocks_on,
				ARRAY_SIZE(bus_timeout_camera_clocks_off),
				ARRAY_SIZE(bus_timeout_camera_clocks_on));

	dummy_value = readl_relaxed(hang_address);
	time_out_jiffies = jiffies + msecs_to_jiffies(DELAY_TIME);
	while (time_is_after_jiffies(time_out_jiffies))
		udelay(1);
	/* if we hit here, test had failed */
	pr_emerg("Bus timeout test failed...0x%x\n", dummy_value);
	iounmap(hang_address);
}
#else /* defined(CONFIG_ARCH_MSM8226) || defined(CONFIG_ARCH_MSM8974) */
static void simulate_bus_hang(void)
{
	void __iomem *p;

	pr_emerg("Generating Bus Hang!\n");
	p = ioremap_nocache(0xFC4B8000, 32);
	*(unsigned int *)p = *(unsigned int *)p;
	mb();	/* memory barriar to generate bus hang */
	pr_info("*p = %x\n", *(unsigned int *)p);
	pr_emerg("Clk may be enabled.Try again if it reaches here!\n");
}
#endif /* defined(CONFIG_ARCH_MSM8226) || defined(CONFIG_ARCH_MSM8974) */

static void __simulate_dabort(void)
{
	char *buf = NULL;
	*buf = 0x0;
}

static void __simulate_pabort(void)
{
	((void (*)(void))NULL)();
}

static void __simulate_undef(void)
{
	BUG();
}

static void __simulate_dblfree(void)
{
	unsigned int *ptr = kmalloc(sizeof(unsigned int), GFP_KERNEL);

	kfree(ptr);
	msleep(1000);
	kfree(ptr);
}

static void __simulate_danglingref(void)
{
	unsigned int *ptr = kmalloc(sizeof(unsigned int), GFP_KERNEL);

	kfree(ptr);
	*ptr = 0x1234;
}

static void __simulate_lowmem(void)
{
	size_t i;

	for (i = 0; kmalloc(128 * 1024, GFP_KERNEL); i++)
		;
	pr_emerg("Allocated %zu KB!\n", i * 128);
}

static void __simulate_memcorrupt(void)
{
	unsigned int *ptr = kmalloc(sizeof(unsigned int), GFP_KERNEL);

	*ptr++ = 4;
	*ptr = 2;
	panic("MEMORY CORRUPTION");
}

#ifdef CONFIG_FREE_PAGES_RDONLY
static void __check_page_read(void)
{
	struct page *page = alloc_pages(GFP_ATOMIC, 0);
	unsigned int *ptr = (unsigned int *)page_address(page);

	pr_emerg("Test with RD page config.");
	__free_pages(page, 0);
	*ptr = 0x12345678;
}
#endif

static int force_error(const char *val, const struct kernel_param *kp)
{
	size_t i;
	struct __magic {
		const char *val;
		const char *msg;
		void (*func)(void);
	} magic[] = {
		{ "appdogbark",
			"Generating an apps wdog bark!",
			&__simulate_apps_wdog_bark },
		{ "appdogbite",
			"Generating an apps wdog bite!",
			&__simulate_apps_wdog_bite },
		{ "dabort",
			"Generating a data abort exception!",
			&__simulate_dabort },
		{ "pabort",
			"Generating a data abort exception!",
			&__simulate_pabort },
		{ "undef",
			"Generating a undefined instruction exception!",
			&__simulate_undef },
		{ "bushang",
			"Generating a Bus Hang!",
			&simulate_bus_hang },
		{ "dblfree",
			NULL,
			&__simulate_dblfree },
		{ "danglingref",
			NULL,
			&__simulate_danglingref },
		{ "lowmem",
			"Allocating memory until failure!",
			&__simulate_lowmem },
		{ "memcorrupt",
			NULL,
			&__simulate_memcorrupt },
#ifdef CONFIG_SEC_DEBUG_SEC_WDOG_BITE
		{ "secdogbite",
			"simulating secure watch dog bite!",
			&__simulate_secure_wdog_bite },
#endif
#ifdef CONFIG_SEC_USER_RESET_DEBUG_TEST
		{ "KP",
			"Generating a data abort exception!",
			&__simulate_dabort },
		{ "DP",
			NULL,
			&force_watchdog_bark },
		{ "WP",
			"simulating secure watch dog bite!",
			&__simulate_secure_wdog_bite },
#endif
#ifdef CONFIG_FREE_PAGES_RDONLY
		{ "pageRDcheck",
			NULL,
			&__check_page_read },
#endif
	};

	pr_emerg("!!!WARN forced error : %s\n", val);

	for (i = 0; i < ARRAY_SIZE(magic); i++) {
		size_t len = strlen(magic[i].val);

		if (strncmp(val, magic[i].val, len))
			continue;

		if (magic[i].msg != NULL)
			pr_emerg("%s\n", magic[i].msg);

		magic[i].func();
		return 0;
	}

	pr_emerg("No such error defined for now!\n");
	return 0;
}
