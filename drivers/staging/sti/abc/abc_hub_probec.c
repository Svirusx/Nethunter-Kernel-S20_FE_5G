/* abc_hub_probec.c
 *
 * Abnormal Behavior Catcher Hub Driver Sub Module(Probe fail Check)
 *
 * Copyright (C) 2017 Samsung Electronics
 *
 * Sangsu Ha <sangsu.ha@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/sti/abc_hub.h>

int probec_fail_cnt;
struct list_head probec_fail_list;

void abc_hub_remove_exp_list(struct sub_probec_pdata *probec_pdata){
	struct abc_hub_probec_entry *probec_fail;
	struct abc_hub_probec_entry *tmp;
	int i;

	for (i = 0; i < probec_pdata->exp_cnt; i++) {
		list_for_each_entry_safe(probec_fail, tmp, &probec_fail_list, node) {
			if (strcmp(probec_fail->probec_fail_str, probec_pdata->exp_name[i]) == 0) {
				list_del(&probec_fail->node);
				kfree(probec_fail);
				probec_fail_cnt--;
			}
		}
	}
}

static void abc_hub_probec_work_func(struct work_struct *work)
{
	struct sub_probec_pdata *probec_pdata = container_of(work, struct sub_probec_pdata, probec_work.work);
	struct abc_hub_probec_entry *probec_fail;
	char event_str[100] = {0,};

	if (probec_fail_cnt > 0) {
		abc_hub_remove_exp_list(probec_pdata);
		
		if (probec_fail_cnt > 0) {
			list_for_each_entry(probec_fail, &probec_fail_list, node)
				pr_info("%s: [fail_list] %s\n", __func__, probec_fail->probec_fail_str);
			sprintf(event_str, "MODULE=probec@ERROR=probe_fail@EXT_LOG=fail_cnt(%d)", probec_fail_cnt);
			abc_hub_send_event(event_str);
			return;
		}
	}

	pr_info("%s: There is no fail list\n", __func__);
}

int parse_probec_data(struct device *dev,
		     struct abc_hub_platform_data *pdata,
		     struct device_node *np)
{
	struct device_node *probec_np;
	int i;

	probec_np = of_find_node_by_name(np, "probec");

	pdata->probec_pdata.exp_cnt = of_property_count_strings(probec_np, "probec,exception_list");
	if (pdata->probec_pdata.exp_cnt <= 0) {
		dev_info(dev, "There is no exception list\n");
		return 0;
	}
	dev_info(dev, "exception list cnt : %d\n", pdata->probec_pdata.exp_cnt);

	for (i = 0; i < pdata->probec_pdata.exp_cnt; i++) {
		of_property_read_string_index(probec_np, "probec,exception_list", i, &pdata->probec_pdata.exp_name[i]);
		dev_info(dev, "exp_name[%d] : %s\n", i, pdata->probec_pdata.exp_name[i]);
	}

	return 0;
}

void abc_hub_probec_enable(struct device *dev, int enable)
{
	/* common sequence */
	struct abc_hub_info *pinfo = dev_get_drvdata(dev);

	pinfo->pdata->probec_pdata.enabled = enable;

	/* custom sequence */
	pr_info("%s: enable(%d)\n", __func__, enable);
	if (enable == ABC_HUB_ENABLED)
		queue_delayed_work(pinfo->pdata->probec_pdata.workqueue,
				   &pinfo->pdata->probec_pdata.probec_work, msecs_to_jiffies(2000));
}

int abc_hub_probec_init(struct device *dev)
{
	struct abc_hub_info *pinfo = dev_get_drvdata(dev);

	/* list init */
	pinfo->pdata->probec_pdata.probec_fail_cnt_p = &probec_fail_cnt;
	pinfo->pdata->probec_pdata.probec_fail_list_p = &probec_fail_list;

	/* delayed work init */
	INIT_DELAYED_WORK(&pinfo->pdata->probec_pdata.probec_work, abc_hub_probec_work_func);

	pinfo->pdata->probec_pdata.workqueue = create_singlethread_workqueue("probec_wq");
	if (!pinfo->pdata->probec_pdata.workqueue) {
		pr_err("%s: fail\n", __func__);
		return -1;
	}

	pr_info("%s: success\n", __func__);
	return 0;
}

void abc_hub_probec_add_fail_list(const char *name)
{
	struct abc_hub_probec_entry *probec_fail;
	static bool is_first = true;

	if (is_first) {
		INIT_LIST_HEAD(&probec_fail_list);
		probec_fail_cnt = 0;
		is_first = false;
	}

	if (probec_fail_cnt < PROBEC_FAIL_MAX) {
		probec_fail = kzalloc(sizeof(*probec_fail), GFP_KERNEL);
		if (probec_fail) {
			strlcpy(probec_fail->probec_fail_str, name, sizeof(probec_fail->probec_fail_str));
			list_add_tail(&probec_fail->node, &probec_fail_list);
			probec_fail_cnt++;
		} else {
			pr_err("%s: failed to allocate probec_fail_cnt\n", __func__);
		}
	}
}
EXPORT_SYMBOL(abc_hub_probec_add_fail_list);

MODULE_DESCRIPTION("Samsung ABC Hub Sub Module(probec) Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
