// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
 */

#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <asm/arch_timer.h>
#include "rpmh_master_stat.h"
#include <linux/rtc.h>
#include <soc/qcom/subsystem_restart.h>

#define FATAL_MAX 5
#define ERROR_MAX (FATAL_MAX * 2)
#define DT_NAME_MAX 32

enum subsystem_clients {
	ADSP = 0,
	CDSP,
	SLPI,
	CLIENT_MAX
};

enum subsystem_ssr_status {
	SSR_DISABLE = 0,
	SSR_ENABLE,
};

struct subsystem_client_config {
	char *subsystem_name;
	int max_error_count; /*this entry == this exit*/
	int max_fatal_count; /*this entry == this exit == prev exit*/
	bool enable_ssr;
};

struct subsystem_client_config subsystem_conf[] = {
	{"adsp", ERROR_MAX, FATAL_MAX, SSR_DISABLE},
	{"cdsp", ERROR_MAX, FATAL_MAX, SSR_DISABLE},
	{"slpi", ERROR_MAX, FATAL_MAX, SSR_DISABLE},
};

static struct subsystem_debug_info {
	char *name;
	uint64_t entry_duration;
	uint64_t prev_duration;
	uint32_t error_count;
	uint32_t fatal_count;
	uint32_t ssr_count;
} subsystem_debug[CLIENT_MAX];

static bool subsystem_is_expected_state(struct subsystem_debug_info *debug_info)
{
	extern bool voice_activated;

	if ((!strncmp(debug_info->name, "adsp", 4)
			&& voice_activated)) {
		pr_info("%s: skip duration check(%d)\n", __func__,
			voice_activated);
		return true;
	} else
		return false;
}

static void subsystem_detect_sleep_error(
	struct subsystem_debug_info *debug_info, uint64_t duration)
{
	/* exceptional condition for not increase error count */
	if (subsystem_is_expected_state(debug_info))
		return;
	/* error detected if exit duration is same as entry */
	if (duration == debug_info->entry_duration) {
		debug_info->error_count++;
		/* increase fatal count in case of same as prev exit */
		if (duration == debug_info->prev_duration)
			debug_info->fatal_count++;

		pr_info("%s : error count %d, fatal count %d, ssr count %d\n",
			debug_info->name, debug_info->error_count,
			debug_info->fatal_count, debug_info->ssr_count);
	} else
		debug_info->error_count = debug_info->fatal_count = 0;
}

void subsystem_update_sleep_time(char *annotation, char *rpmh_master_name,
	uint64_t accumulated_duration)
{
	struct subsystem_debug_info *debug_info = NULL;
	int i;

	if (!annotation || !rpmh_master_name) {
		pr_err("%s parameters are not valid\n", __func__);
		return;
	}

	for (i = 0; i < CLIENT_MAX; i++) {
		if (!strcasecmp(rpmh_master_name,
			subsystem_conf[i].subsystem_name)) {
			debug_info = &subsystem_debug[i];
			break;
		}
	}
	if (debug_info == NULL)
		return;

	if (!strcmp(annotation, "entry"))
		debug_info->entry_duration = accumulated_duration;
	else if (!strcmp(annotation, "exit")) {
		subsystem_detect_sleep_error(debug_info, accumulated_duration);
		debug_info->prev_duration = accumulated_duration;
	} else
		pr_err("annotation error (%s)\n", annotation);
}
EXPORT_SYMBOL(subsystem_update_sleep_time);

void subsystem_monitor_sleep_issue(void)
{
	struct subsystem_debug_info *debug_info;
	struct subsys_device *subsys_dev;
	int i, rc = 0;

	for (i = 0; i < CLIENT_MAX; i++) {
		debug_info = &subsystem_debug[i];

		if (debug_info->error_count >
			subsystem_conf[i].max_error_count ||
		    debug_info->fatal_count >
			subsystem_conf[i].max_fatal_count) {
			pr_info("%s: restart %s by intention\n", __func__,
				debug_info->name);

			subsys_dev = find_subsys_device(debug_info->name);

			if (!subsys_dev) {
				pr_err("unable to find %s dev\n",
					debug_info->name);
				return;
			}

			if (subsystem_conf[i].enable_ssr &&
			    debug_info->ssr_count < subsystem_conf[i].max_error_count) {
				subsys_set_fssr(subsys_dev,
					subsystem_conf[i].enable_ssr);
				debug_info->ssr_count++;
			}
			/* subsystem_restart_dev has worker queue to handle */
			rc = subsystem_restart_dev(subsys_dev);
			if (rc) {
				pr_err("%s : subsystem_restart_dev failed\n",
					debug_info->name);
				return;
			}
			debug_info->error_count = debug_info->fatal_count = 0;
		}
	}
}
EXPORT_SYMBOL(subsystem_monitor_sleep_issue);

static int subsystem_sleep_debug_probe(struct platform_device *pdev)
{
	struct subsystem_debug_info *debug_info;
	char name[DT_NAME_MAX] = {'\0',};
	int i, rc = 0;

	dev_info(&pdev->dev, "%s\n", __func__);

	for (i = 0; i < CLIENT_MAX; i++) {
		debug_info = &subsystem_debug[i];
		debug_info->name = subsystem_conf[i].subsystem_name;

		sprintf(name, "subsystem-debug,%s-error-max",
			subsystem_conf[i].subsystem_name);
		rc = of_property_read_u32(pdev->dev.of_node, name,
			&subsystem_conf[i].max_error_count);
		if (rc)
			pr_debug("%s : unable to find %s\n", __func__, name);

		sprintf(name, "subsystem-debug,%s-fatal-max",
			subsystem_conf[i].subsystem_name);
		rc = of_property_read_u32(pdev->dev.of_node, name,
			&subsystem_conf[i].max_fatal_count);
		if (rc)
			pr_debug("%s : unable to find %s\n", __func__, name);

		sprintf(name, "subsystem-debug,%s-ssr-enable",
			subsystem_conf[i].subsystem_name);

		if (of_property_read_bool(pdev->dev.of_node, name))
			subsystem_conf[i].enable_ssr = SSR_ENABLE; 

		pr_info("%s:max error count %d, max fatal count %d, ssr is %s\n",
			subsystem_conf[i].subsystem_name,
			subsystem_conf[i].max_error_count,
			subsystem_conf[i].max_fatal_count,
			subsystem_conf[i].enable_ssr ? "enabled" : "disabled");
	}
	return 0;
}

static int subsystem_sleep_debug_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "%s\n", __func__);
	return 0;
}

static const struct of_device_id subsystem_debug_table[] = {
	{.compatible = "samsung,subsystem-sleep-debug"},
	{},
};

static struct platform_driver subsystem_sleep_debug_driver = {
	.probe	= subsystem_sleep_debug_probe,
	.remove = subsystem_sleep_debug_remove,
	.driver = {
		.name = "subsystem_sleep_debug",
		.of_match_table = subsystem_debug_table,
	},
};
module_platform_driver(subsystem_sleep_debug_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("DSP Sleep Debug driver");
