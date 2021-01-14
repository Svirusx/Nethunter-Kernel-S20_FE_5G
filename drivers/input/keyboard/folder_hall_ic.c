#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/spinlock.h>
#include <linux/wakelock.h>
#include <linux/hall.h>
#include <linux/notifier.h>
#include <linux/sec_class.h>
#ifdef CONFIG_USB_HW_PARAM
#include <linux/usb_notify.h>
#endif

#define HALL_EVENT_REPORT_WORKQUEUE	0

extern struct device *hall_ic;

struct hall_drvdata {
	struct input_dev *input;
	int gpio_hall;
	int irq_hall;
	struct work_struct work;
	struct delayed_work hall_dwork;
	struct wake_lock hall_wake_lock;
};

static bool hall_value = 1;
static int is_probe_done = 0;

static BLOCKING_NOTIFIER_HEAD(hall_ic_notifier_list);

void hall_ic_register_notify(struct notifier_block *nb) {
	blocking_notifier_chain_register(&hall_ic_notifier_list, nb);
}
EXPORT_SYMBOL(hall_ic_register_notify);

void hall_ic_request_notify(struct notifier_block *nb) {
	if(!is_probe_done) {
		pr_info("%s(): didn't probe done: %d\n", __func__, is_probe_done);
		nb->notifier_call(nb, -1, NULL);
		return;
	}

	nb->notifier_call(nb, !hall_value, NULL);
}
EXPORT_SYMBOL(hall_ic_request_notify);

void hall_ic_unregister_notify(struct notifier_block *nb) {
	blocking_notifier_chain_unregister(&hall_ic_notifier_list, nb);
}
EXPORT_SYMBOL(hall_ic_unregister_notify);

static ssize_t sysfs_hall_value_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	if (!hall_value)
		snprintf(buf, 2, "0");
	else
		snprintf(buf, 2, "1");

	return strlen(buf);
}

static ssize_t sysfs_hall_value_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct hall_drvdata *ddata = NULL;
	char emulate_hall_status[255] = {0,};
	
	ddata = dev_get_drvdata(dev);
	if (ddata == NULL) {
		pr_err("ddata is NULL\n");
		return -EINVAL;
	}

	pr_info("%s():size = %zu\n", __func__, count);
	if (count > 255) {
		pr_err("%s size(%zu) is too long.\n", __func__, count);
		return -EINVAL;
	}

	sscanf(buf, "%s", emulate_hall_status);
	pr_info("%s():currnet hall_value = %s\n",	__func__, emulate_hall_status);
	if (!strncmp(buf, "OPEN", 4))
		hall_value = 1;
	else if (!strncmp(buf, "CLOSE", 5))
		hall_value = 0;
	else
		pr_err("%s : parameter wrong!\n", __func__);
	
	input_report_switch(ddata->input, SW_LID, !hall_value);
	input_sync(ddata->input);
		
	blocking_notifier_call_chain(&hall_ic_notifier_list, !hall_value, NULL);
	
	return count;
}
static DEVICE_ATTR(flip_status, 0444, sysfs_hall_value_show, sysfs_hall_value_store);

#ifdef CONFIG_SEC_FACTORY
static void hall_value_work(struct work_struct *work)
{
	bool first,second;
	struct hall_drvdata *ddata =
		container_of(work, struct hall_drvdata,
				hall_dwork.work);
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif

	first = gpio_get_value(ddata->gpio_hall);

	printk("[keys] %s hall_status #1 : %d (%s)\n", __func__,
						first, first ? "open" : "close");

	usleep_range(50000, 50000); /* 50ms */

	second = gpio_get_value(ddata->gpio_hall);

	printk("[keys] %s hall_status #2 : %d (%s)\n", __func__,
						second, second ? "open" : "close");

	if (first == second) {
		hall_value = first;

#if defined(CONFIG_USB_HW_PARAM)
		if (!hall_value && o_notify)
			inc_hw_param(o_notify, USB_HALL_FOLDING_COUNT);
#endif
		input_report_switch(ddata->input, SW_LID, !hall_value);
		input_sync(ddata->input);

		/* foder open : 0, close : 1 */
		blocking_notifier_call_chain(&hall_ic_notifier_list, !hall_value, NULL);
	}
}
#if !HALL_EVENT_REPORT_WORKQUEUE
static void hall_value_report(struct hall_drvdata *ddata)
{
	bool first,second;
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif
	first = gpio_get_value(ddata->gpio_hall);

	printk("[keys] %s hall_status #1 : %d (%s)\n", __func__,
						first, first ? "open" : "close");

	usleep_range(50000, 50000); /* 50ms */

	second = gpio_get_value(ddata->gpio_hall);

	printk("[keys] %s hall_status #2 : %d (%s)\n", __func__,
						second, second ? "open" : "close");

	if (first == second) {
		hall_value = first;
#if defined(CONFIG_USB_HW_PARAM)
		if (!hall_value && o_notify)
			inc_hw_param(o_notify, USB_HALL_FOLDING_COUNT);
#endif
		input_report_switch(ddata->input, SW_LID, !hall_value);
		input_sync(ddata->input);

		/* foder open : 0, close : 1 */
		blocking_notifier_call_chain(&hall_ic_notifier_list, !hall_value, NULL);
	}
}
#endif
#else
static void hall_close(struct input_dev *input);
static void hall_value_work(struct work_struct *work)
{
#define HALL_COMPARISONS 6
	struct hall_drvdata *ddata =
				container_of(work, struct hall_drvdata,
                                hall_dwork.work);
	bool comp_val[HALL_COMPARISONS]={0};
	int i;

#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif
	comp_val[0] = gpio_get_value(ddata->gpio_hall);

	printk("[keys] %s hall_status : %d (%s)\n",
		__func__, comp_val[0], comp_val[0] ? "open" : "close");

	for (i = 1; i < HALL_COMPARISONS; i++){
		usleep_range(6000, 6000); /* 6 ms */
		comp_val[i] = gpio_get_value(ddata->gpio_hall);
		if (comp_val[i] != comp_val[0]){
			pr_err("%s : Value is not same!\n", __func__);
			goto out;
		}
	}

	hall_value = comp_val[0];
#if defined(CONFIG_USB_HW_PARAM)
	if (!hall_value && o_notify)
		inc_hw_param(o_notify, USB_HALL_FOLDING_COUNT);
#endif
	printk("[keys] hall ic reported value: %d (%s)\n",
			hall_value, hall_value ? "open" : "close");

	input_report_switch(ddata->input, SW_LID, !hall_value);
	input_sync(ddata->input);

	/* foder open : 0, close : 1 */
	blocking_notifier_call_chain(&hall_ic_notifier_list, !hall_value, NULL);

out:
	hall_close(ddata->input);
} /* Noise Problem - Hall defence WA code */
#if !HALL_EVENT_REPORT_WORKQUEUE
static void hall_value_report(struct hall_drvdata *ddata)
{
#define HALL_COMPARISONS 6
	bool comp_val[HALL_COMPARISONS]={0};
	int i;
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif		
	comp_val[0] = gpio_get_value(ddata->gpio_hall);

	printk("[keys] %s hall_status : %d (%s)\n",
		__func__, comp_val[0], comp_val[0] ? "open" : "close");

	for (i = 1; i < HALL_COMPARISONS; i++){
		usleep_range(3000, 3000); /* 6 ms */
		comp_val[i] = gpio_get_value(ddata->gpio_hall);
		if (comp_val[i] != comp_val[0]){
			pr_err("%s : Value is not same!\n", __func__);
			goto out;
		}
	}

	hall_value = comp_val[0];
	printk("[keys] hall ic reported value: %d (%s)\n",
			hall_value, hall_value ? "open" : "close");
#if defined(CONFIG_USB_HW_PARAM)
	if (!hall_value && o_notify)
		inc_hw_param(o_notify, USB_HALL_FOLDING_COUNT);
#endif
	input_report_switch(ddata->input, SW_LID, !hall_value);
	input_sync(ddata->input);

	/* foder open : 0, close : 1 */
	blocking_notifier_call_chain(&hall_ic_notifier_list, !hall_value, NULL);

out:
	hall_close(ddata->input);
}
#endif
#endif

static void __hall_value_detect(struct hall_drvdata *ddata, bool flip_status)
{
	cancel_delayed_work_sync(&ddata->hall_dwork);
#ifdef CONFIG_SEC_FACTORY
#if HALL_EVENT_REPORT_WORKQUEUE
	schedule_delayed_work(&ddata->hall_dwork, HZ / 20);
#else
	msleep(10);
	hall_value_report(ddata);
#endif
#else
	if (flip_status) {
		wake_lock_timeout(&ddata->hall_wake_lock, HZ * 5 / 100); /* 50ms */
#if HALL_EVENT_REPORT_WORKQUEUE
		schedule_delayed_work(&ddata->hall_dwork, HZ * 1 / 100); /* 10ms */
#else
		msleep(10);
		hall_value_report(ddata);
#endif
	} else {
		wake_unlock(&ddata->hall_wake_lock);
#if HALL_EVENT_REPORT_WORKQUEUE
		schedule_delayed_work(&ddata->hall_dwork, 0);
#else
		hall_value_report(ddata);
#endif
	}
#endif
}

static irqreturn_t hall_value_detect(int irq, void *dev_id)
{
	bool hall_status;
	struct hall_drvdata *ddata = dev_id;

	hall_status = gpio_get_value(ddata->gpio_hall);

	pr_debug("[keys] %s hall_status : %d\n", __func__, hall_status);

	if ( (system_state == SYSTEM_POWER_OFF) || (system_state == SYSTEM_RESTART) ) {
		pr_debug("[keys] %s don't need to work hall irq\n", __func__);
		return IRQ_HANDLED;
	}
	__hall_value_detect(ddata, hall_status);

	return IRQ_HANDLED;
}

static int hall_open(struct input_dev *input)
{
	struct hall_drvdata *ddata = input_get_drvdata(input);
	/* update the current status */
	schedule_delayed_work(&ddata->hall_dwork, HZ / 2);
	/* Report current state of buttons that are connected to GPIOs */
	input_sync(input);

	return 0;
}

static void hall_close(struct input_dev *input)
{
}

static void init_hall_ic_irq(struct input_dev *input)
{
	struct hall_drvdata *ddata = input_get_drvdata(input);

	int ret = 0;
	int irq = ddata->irq_hall;

	hall_value = gpio_get_value(ddata->gpio_hall);

	INIT_DELAYED_WORK(&ddata->hall_dwork, hall_value_work);

	ret =
		request_threaded_irq(
		irq, NULL,
		hall_value_detect,
		IRQF_TRIGGER_RISING |
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
		"hall_value", ddata);
	if (ret < 0) {
		printk(KERN_ERR "[keys] failed to request hall ic irq %d gpio %d\n", irq, ddata->gpio_hall);
	} else {
		pr_info("%s : success\n", __func__);
	}
}

#ifdef CONFIG_OF
static int of_hall_data_parsing_dt(struct device *dev,struct hall_drvdata *ddata)
{
	struct device_node *np_hall= dev->of_node;
	int gpio;
	enum of_gpio_flags flags;

	gpio = of_get_named_gpio_flags(np_hall, "folder_hall,gpio_folder", 0, &flags);
	if (gpio < 0) {
		pr_info("[keys] %s: fail to get hall_value \n", __func__ );
		return -EINVAL;
	}
	ddata->gpio_hall = gpio;

	gpio = gpio_to_irq(gpio);
	if (gpio < 0) {
		pr_info("[keys] %s: fail to return irq corresponding gpio \n", __func__ );
		return -EINVAL;
	}
	ddata->irq_hall = gpio;

	return 0;
}
#endif

static int hall_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hall_drvdata *ddata;
	struct input_dev *input;
	int error = 0;
	int wakeup = 0;

	pr_info("folder_hall_probe - start\n");

	ddata = kzalloc(sizeof(struct hall_drvdata), GFP_KERNEL);
	if (!ddata) {
		dev_err(dev, "failed to allocate state\n");
		return -ENOMEM;
	}

#ifdef CONFIG_OF
	if (dev->of_node) {
		error = of_hall_data_parsing_dt(dev, ddata);
		if (error < 0) {
			pr_info("[keys] %s : fail to get the dt (FOLDER HALL)\n", __func__);
			goto fail1;
		}
	}
#endif

	input = input_allocate_device();
	if (!input) {
		dev_err(dev, "failed to allocate state\n");
		error = -ENOMEM;
		goto fail1;
	}

	ddata->input = input;

	wake_lock_init(&ddata->hall_wake_lock, WAKE_LOCK_SUSPEND,
		"folder hall wake lock");


	platform_set_drvdata(pdev, ddata);
	input_set_drvdata(input, ddata);

	input->name = "folder_hall";
	input->phys = "folder_hall";
	input->dev.parent = &pdev->dev;

	input->evbit[0] |= BIT_MASK(EV_SW);

	input_set_capability(input, EV_SW, SW_LID);

	input->open = hall_open;
	input->close = hall_close;

	/* Enable auto repeat feature of Linux input subsystem */
	__set_bit(EV_REP, input->evbit);

	init_hall_ic_irq(input);

	error = device_create_file(hall_ic, &dev_attr_flip_status);
	if (error < 0) {
		pr_err("Failed to create device file(%s) !\n", dev_attr_flip_status.attr.name);
		goto fail2;
	}

	error = input_register_device(input);
	if (error) {
		dev_err(dev, "Unable to register input device, error: %d\n",
			error);
		goto fail3;
	}

	schedule_delayed_work(&ddata->hall_dwork, HZ / 2);
	/* Report current state of buttons that are connected to GPIOs */
	input_sync(input);

	device_init_wakeup(&pdev->dev, wakeup);
	dev_set_drvdata(dev, ddata);

	is_probe_done = 1;

	pr_info("folder_hall_probe - end\n");

	return 0;
fail3:
	device_remove_file(dev, &dev_attr_flip_status);
fail2:
	platform_set_drvdata(pdev, NULL);
	wake_lock_destroy(&ddata->hall_wake_lock);
	input_free_device(input);
fail1:
	kfree(ddata);

	return error;
}

static int hall_remove(struct platform_device *pdev)
{
	struct hall_drvdata *ddata = platform_get_drvdata(pdev);
	struct input_dev *input = ddata->input;

	printk("[keys] %s\n", __func__);
	device_remove_file(&pdev->dev, &dev_attr_flip_status);

	device_init_wakeup(&pdev->dev, 0);

	input_unregister_device(input);

	wake_lock_destroy(&ddata->hall_wake_lock);

	kfree(ddata);

	return 0;
}

#if defined(CONFIG_OF)
static struct of_device_id hall_dt_ids[] = {
	{ .compatible = "folder_hall" },
	{ },
};
MODULE_DEVICE_TABLE(of, hall_dt_ids);
#endif /* CONFIG_OF */

#ifdef CONFIG_PM_SLEEP
static int hall_suspend(struct device *dev)
{
	struct hall_drvdata *ddata = dev_get_drvdata(dev);
	struct input_dev *input = ddata->input;
	bool status;

	printk("[keys] %s\n", __func__);
	status = gpio_get_value(ddata->gpio_hall);
	printk("[keys] %s hall_status : %d (%s)\n", __func__, status, status ? "open" : "close");

	enable_irq_wake(ddata->irq_hall);

	if (device_may_wakeup(dev)) {
		enable_irq_wake(ddata->irq_hall);
	} else {
		mutex_lock(&input->mutex);
		if (input->users)
			hall_close(input);
		mutex_unlock(&input->mutex);
	}

	return 0;
}

static int hall_resume(struct device *dev)
{
	struct hall_drvdata *ddata = dev_get_drvdata(dev);
	struct input_dev *input = ddata->input;
	bool status;

	printk("[keys] %s\n", __func__);
	status = gpio_get_value(ddata->gpio_hall);
	printk("[keys] %s flip_status : %d (%s)\n",
			__func__, status, status ? "open" : "close");
	input_sync(input);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(hall_pm_ops, hall_suspend, hall_resume);

static struct platform_driver hall_device_driver = {
	.probe		= hall_probe,
	.remove		= hall_remove,
	.driver		= {
		.name	= "folder_hall",
		.owner	= THIS_MODULE,
		.pm	= &hall_pm_ops,
#if defined(CONFIG_OF)
		.of_match_table	= hall_dt_ids,
#endif /* CONFIG_OF */
	}
};

static int __init hall_init(void)
{
	printk("[keys] %s\n", __func__);
	return platform_driver_register(&hall_device_driver);
}

static void __exit hall_exit(void)
{
	printk("[keys] %s\n", __func__);
	platform_driver_unregister(&hall_device_driver);
}

late_initcall(hall_init);
module_exit(hall_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("FOLDER Hall IC driver for GPIOs");
