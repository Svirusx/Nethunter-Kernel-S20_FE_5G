/* abc_hub_cond.c
 *
 * Abnormal Behavior Catcher Hub Driver Sub Module Cond(Detecting Connection)
 *
 * Copyright (C) 2017 Samsung Electronics
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/of_gpio.h>
#include <linux/sec_class.h>
#include <linux/sti/abc_hub.h>

#define DEBUG_FOR_SECDETECT

#define SEC_CONN_PRINT(format, ...) pr_info("[ABC_COND] " format, ##__VA_ARGS__)
#define ABCEVENT_CONN_MAX_DEV_STRING 120

/* This value is used for checking gpio irq is enabled or not. */
static int detect_conn_enabled;

/**
 * Parse the device tree and get gpio number, irq type.
 * And then request gpios.
 */
int parse_cond_data(struct device *dev,
			struct abc_hub_platform_data *pdata,
			struct device_node *np)
{
	int i;
	int retval = 0;
	struct pinctrl *conn_pinctrl;
	struct pinctrl *pm_conn_pinctrl;

	SEC_CONN_PRINT("ABC_HUB_COND DT Parsing start.");

	pdata->cond_pdata.gpio_cnt = of_gpio_named_count(np, "sec,det_conn_gpios");
	SEC_CONN_PRINT("gpio cnt = : %d\n", pdata->cond_pdata.gpio_cnt);

	if (pdata->cond_pdata.gpio_cnt <= 0) {
		SEC_CONN_PRINT("of_gpio_named_count = 0.\n");
		return -ENODEV;
	}

	pdata->cond_pdata.gpio_total_cnt = pdata->cond_pdata.gpio_cnt;

	/* Setting pinctrl state to NO PULL */
	conn_pinctrl = devm_pinctrl_get_select(dev, "det_ap_connect");
	if (IS_ERR_OR_NULL(conn_pinctrl)) {
		SEC_CONN_PRINT("setting no pull failed.\n");
		return -ENODEV;
	}

	for (i = 0; i < pdata->cond_pdata.gpio_cnt; i++) {
		/*Get connector name*/
		of_property_read_string_index(np, "sec,det_conn_name", i, &pdata->cond_pdata.name[i]);

		/*Get connector gpio number*/
		pdata->cond_pdata.irq_gpio[i] = of_get_named_gpio(np, "sec,det_conn_gpios", i);
		SEC_CONN_PRINT("index[%d] : gpio number / name: %d / %s\n", i, pdata->cond_pdata.irq_gpio[i],
			       pdata->cond_pdata.name[i]);

		if (gpio_is_valid(pdata->cond_pdata.irq_gpio[i])) {
			retval = gpio_request_one(pdata->cond_pdata.irq_gpio[i], GPIOF_DIR_IN,
						  pdata->cond_pdata.name[i]);

			if (retval) {
				dev_err(dev, "%s: Unable to request %s int [%d]\n",
					__func__, pdata->cond_pdata.name[i], pdata->cond_pdata.irq_gpio[i]);
				return -EINVAL;
			}

#if defined(DEBUG_FOR_SECDETECT)
			SEC_CONN_PRINT("i = [%d] gpio level [%d] = %d\n", i, pdata->cond_pdata.irq_gpio[i],
				       gpio_get_value(pdata->cond_pdata.irq_gpio[i]));
#endif
			/*Filling the irq_number from this gpio.*/
			pdata->cond_pdata.irq_number[i] = gpio_to_irq(pdata->cond_pdata.irq_gpio[i]);
			pdata->cond_pdata.irq_type[i] = IRQ_TYPE_EDGE_BOTH;
		} else {
			dev_err(dev, "%s: Failed to get irq gpio.\n", __func__);
			return -EINVAL;
		}
	}

	/* Get pm_gpio */
	pdata->cond_pdata.gpio_pm_cnt = of_gpio_named_count(np, "sec,det_pm_conn_gpios");
	SEC_CONN_PRINT("pm gpio cnt = : %d\n", pdata->cond_pdata.gpio_pm_cnt);

	if (pdata->cond_pdata.gpio_pm_cnt <= 0) {
		SEC_CONN_PRINT("pm of_gpio_named_count = 0.\n");
		return 0;
	}

	/* Setting pinctrl state to NO PULL */
	pm_conn_pinctrl = devm_pinctrl_get_select(dev, "det_pm_connect");
	if (IS_ERR_OR_NULL(pm_conn_pinctrl)) {
		SEC_CONN_PRINT("pm setting no pull failed.\n");
		return -ENODEV;
	}

	pdata->cond_pdata.gpio_total_cnt = pdata->cond_pdata.gpio_total_cnt + pdata->cond_pdata.gpio_pm_cnt;
#if defined(DEBUG_FOR_SECDETECT)
	SEC_CONN_PRINT("gpio_total_count = %d", pdata->cond_pdata.gpio_total_cnt);
#endif

	for (i = pdata->cond_pdata.gpio_cnt; i < pdata->cond_pdata.gpio_total_cnt; i++) {
		/*Get connector name*/
		of_property_read_string_index(np, "sec,det_pm_conn_name", i - pdata->cond_pdata.gpio_cnt,
					      &pdata->cond_pdata.name[i]);

		/*Get connector gpio number*/
		pdata->cond_pdata.irq_gpio[i] = of_get_named_gpio(np, "sec,det_pm_conn_gpios",
								  i - pdata->cond_pdata.gpio_cnt);
		SEC_CONN_PRINT("index[%d] : pm gpio number / name: %d / %s\n", i, pdata->cond_pdata.irq_gpio[i],
			       pdata->cond_pdata.name[i]);
		if (gpio_is_valid(pdata->cond_pdata.irq_gpio[i])) {
			SEC_CONN_PRINT("i = [%d], gpio level [%d] = %d\n", i, pdata->cond_pdata.irq_gpio[i],
				       gpio_get_value(pdata->cond_pdata.irq_gpio[i]));
			SEC_CONN_PRINT("gpio irq gpio = [%d], irq = [%d]\n", pdata->cond_pdata.irq_gpio[i],
				       gpio_to_irq(pdata->cond_pdata.irq_gpio[i]));
		} else {
			SEC_CONN_PRINT("pm gpio[%d] invalid\n", i - pdata->cond_pdata.gpio_cnt);
			return -EINVAL;
		}

		pdata->cond_pdata.irq_number[i] = gpio_to_irq(pdata->cond_pdata.irq_gpio[i]);
		pdata->cond_pdata.irq_type[i] = IRQ_TYPE_EDGE_BOTH;
	}
	SEC_CONN_PRINT("ABC_HUB_COND DT Parsing done.");

	return 1;
}

/**
 * Send an ABC_event about given gpio pin number.
 */
void send_ABC_event_by_num(int num, struct abc_hub_info *pinfo, int level)
{
	char ABC_event_dev_str[ABCEVENT_CONN_MAX_DEV_STRING];

	/*Send ABC Event Data*/
	if (level == 1) {
		sprintf(ABC_event_dev_str, "MODULE=cond@ERROR=%s", pinfo->pdata->cond_pdata.name[num]);
		abc_hub_send_event(ABC_event_dev_str);

		SEC_CONN_PRINT("send ABC_event pin[%d]:CONNECTOR_NAME=%s,CONNECTOR_TYPE=HIGH_LEVEL.\n",
			       num, pinfo->pdata->cond_pdata.name[num]);
	} else if (level == 0)
		SEC_CONN_PRINT("not send ABC_event pin[%d]:CONNECTOR_NAME=%s,CONNECTOR_TYPE=LOW_LEVEL.\n",
			       num, pinfo->pdata->cond_pdata.name[num]);
}

/**
 * Check and send ABC_event from irq handler.
 */
void check_and_send_ABC_event_irq(int irq, struct abc_hub_info *pinfo, int type)
{
	int i;

	/*Send ABCEvent Data*/
	for (i = 0; i < pinfo->pdata->cond_pdata.gpio_total_cnt; i++) {
		if (irq == pinfo->pdata->cond_pdata.irq_number[i]) {
			/* if Rising or falling edge occurs, check level one more.*/
			/* and if it still level high send ABC_event.*/
			usleep_range(1 * 1000, 1 * 1000);
			if (gpio_get_value(pinfo->pdata->cond_pdata.irq_gpio[i]))
				send_ABC_event_by_num(i, pinfo, 1);

			SEC_CONN_PRINT("%s status changed.\n", pinfo->pdata->cond_pdata.name[i]);
		}
	}
}

/**
 * This is IRQ handler function which is
 * called when the connector pin state change.
 */
static irqreturn_t detect_conn_interrupt_handler(int irq, void *handle)
{
	int type;
	struct abc_hub_info *pinfo = handle;

	if (detect_conn_enabled != 0) {
		SEC_CONN_PRINT("%s\n", __func__);

		type = irq_get_trigger_type(irq);
		check_and_send_ABC_event_irq(irq, pinfo, type);
	}

	return IRQ_HANDLED;
}

/**
 * Enable all gpio pin IRQs which is from Device Tree.
 */
int abc_detect_conn_irq_enable(struct abc_hub_info *pinfo, bool enable, int pin)
{
	int retval = 0;
	int i;

	if (enable) {
		if (pin < pinfo->pdata->cond_pdata.gpio_total_cnt) {
			retval = request_threaded_irq(pinfo->pdata->cond_pdata.irq_number[pin], NULL,
						      detect_conn_interrupt_handler,
						      pinfo->pdata->cond_pdata.irq_type[pin] | IRQF_ONESHOT,
						      pinfo->pdata->cond_pdata.name[pin], pinfo);

			if (retval) {
				SEC_CONN_PRINT("%s: Failed to request threaded irq %d.\n",
					       __func__, retval);

				return retval;
			}

#if defined(DEBUG_FOR_SECDETECT)
			SEC_CONN_PRINT("%s: Succeeded to request threaded irq %d: irq_num[%d], type[%x],name[%s].\n",
				       __func__, retval, pinfo->pdata->cond_pdata.irq_number[pin],
				       pinfo->pdata->cond_pdata.irq_type[pin], pinfo->pdata->cond_pdata.name[pin]);
#endif
			pinfo->pdata->cond_pdata.irq_enabled[pin] = true;
		}
	} else {
		for (i = 0; i < pinfo->pdata->cond_pdata.gpio_total_cnt; i++) {
			if (pinfo->pdata->cond_pdata.irq_enabled[i]) {
				disable_irq(pinfo->pdata->cond_pdata.irq_number[i]);
				free_irq(pinfo->pdata->cond_pdata.irq_number[i], pinfo);
			}
		}
		detect_conn_enabled = 0;
	}

	return retval;
}

/**
 * Enable abc hub cond driver.
 * Firstly, check gpio level for all pins and send abc event if gpio value is true.
 * Secondly, enable edge irq so that this driver can send abc event while drivers is on.
 */
void abc_hub_cond_enable(struct device *dev, int enable)
{
	/* common sequence */
	struct abc_hub_info *pinfo = dev_get_drvdata(dev);
	struct abc_hub_platform_data *pdata = pinfo->pdata;
	int ret;
	int i;

	pdata->cond_pdata.enabled = enable;

	/* implement enable sequence */
	if (enable == ABC_HUB_ENABLED) {
		SEC_CONN_PRINT("ABC_HUB_COND_ENABLED.");
		for (i = 0; i < pdata->cond_pdata.gpio_total_cnt; i++) {
			if (!(detect_conn_enabled & (1 << i))) {
				detect_conn_enabled |= (1 << i);

				SEC_CONN_PRINT("gpio level [%d] = %d\n", pdata->cond_pdata.irq_gpio[i],
					       gpio_get_value(pdata->cond_pdata.irq_gpio[i]));
				/*get level value of the gpio pin.*/
				/*if there's gpio low pin, send ABC_event*/
				if (gpio_get_value(pdata->cond_pdata.irq_gpio[i]))
					send_ABC_event_by_num(i, pinfo, 1);
				else
					send_ABC_event_by_num(i, pinfo, 0);

				/*Enable interrupt.*/
				ret = abc_detect_conn_irq_enable(pinfo, true, i);

				if (ret < 0) {
					SEC_CONN_PRINT("gpio[%d] Interrupt not enabled.\n",
						       pdata->cond_pdata.irq_gpio[i]);
					return;
				}
			}
		}
	} else if (enable == ABC_HUB_DISABLED) {
		SEC_CONN_PRINT("ABC_HUB_COND_DISANABLED.");
		/*Disable interrupt.*/
		ret = abc_detect_conn_irq_enable(pinfo, false, 0);
	}
}

int abc_hub_cond_init(struct device *dev)
{
	/* implement init sequence : return 0 if success, return -1 if fail */
	// struct abc_hub_info *pinfo = dev_get_drvdata(dev);

	return 0;
}

int abc_hub_cond_suspend(struct device *dev)
{
	/* implement suspend sequence */

	return 0;
}

int abc_hub_cond_resume(struct device *dev)
{
	/* implement resume sequence */

	return 0;
}

MODULE_DESCRIPTION("Samsung ABC Hub Sub Module1(cond) Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
