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


extern struct device *hall_ic;

struct hall_drvdata {
#ifdef CONFIG_WACOM_HALL_SUPPORT_COVER_DETECT
	struct input_dev *input;
#endif
	struct device *dev;
	struct work_struct work;
	struct delayed_work wacom_cover_dwork;
	struct wake_lock wacom_wake_lock;
	int gpio_wacom_cover;
	int irq_wacom_cover;
};

static bool hall_wacom_status = 1;

static ssize_t hall_wacom_detect_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	pr_info("%s : value=%d\n", __func__, hall_wacom_status);
	if (hall_wacom_status)
		sprintf(buf, "OPEN\n");
	else
		sprintf(buf, "CLOSE\n");

	return strlen(buf);
}
static DEVICE_ATTR(hall_wacom_detect, 0444, hall_wacom_detect_show, NULL);

static void wacom_cover_work(struct work_struct *work)
{
	struct hall_drvdata *ddata =
		container_of(work, struct hall_drvdata,
				wacom_cover_dwork.work);

	hall_wacom_status = gpio_get_value(ddata->gpio_wacom_cover);

	pr_info("keys:%s #1 : %d\n", __func__, hall_wacom_status);
#ifdef CONFIG_WACOM_HALL_SUPPORT_COVER_DETECT
	input_report_switch(ddata->input,
			SW_CERTIFYHALL, !hall_wacom_status);
	input_sync(ddata->input);
#endif
}

static void __wacom_cover_detect(struct hall_drvdata *ddata, bool wacom_status)
{
	cancel_delayed_work_sync(&ddata->wacom_cover_dwork);
#ifdef CONFIG_SEC_FACTORY
	schedule_delayed_work(&ddata->wacom_cover_dwork, HZ / 20);
#else
	if (wacom_status)	{
		wake_lock_timeout(&ddata->wacom_wake_lock, HZ * 5 / 100); /* 50ms */
		schedule_delayed_work(&ddata->wacom_cover_dwork, HZ * 1 / 100); /* 10ms */
	} else {
		wake_unlock(&ddata->wacom_wake_lock);
		schedule_delayed_work(&ddata->wacom_cover_dwork, 0);
	}
#endif
}

static irqreturn_t wacom_cover_detect(int irq, void *dev_id)
{
	bool wacom_status;
	struct hall_drvdata *ddata = dev_id;


	wacom_status = gpio_get_value(ddata->gpio_wacom_cover);

	pr_info("keys:%s wacom_status : %d\n",
		 __func__, wacom_status);

	__wacom_cover_detect(ddata, wacom_status);

	return IRQ_HANDLED;
}

#ifdef CONFIG_WACOM_HALL_SUPPORT_COVER_DETECT
static int wacom_hall_open(struct input_dev *input)
{
	struct hall_drvdata *ddata = input_get_drvdata(input);
	/* update the current status */
	schedule_delayed_work(&ddata->wacom_cover_dwork, HZ / 2);
	/* Report current state of buttons that are connected to GPIOs */
	input_sync(input);

	return 0;
}

static void wacom_hall_close(struct input_dev *input)
{
}
#endif

static void init_hall_ic_wacom_irq(struct hall_drvdata *ddata)
{
	int ret = 0;
	int irq = ddata->irq_wacom_cover;

	hall_wacom_status = gpio_get_value(ddata->gpio_wacom_cover);

	INIT_DELAYED_WORK(&ddata->wacom_cover_dwork, wacom_cover_work);

	ret =
		request_threaded_irq(
		irq, NULL,
		wacom_cover_detect,
		IRQF_TRIGGER_RISING |
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
		"wacom_cover", ddata);
	if (ret < 0) {
		pr_info("keys: failed to request wacom cover irq %d gpio %d\n",
			irq, ddata->gpio_wacom_cover);
	} else {
		pr_info("%s : success\n", __func__);
	}
}


#ifdef CONFIG_OF
static int of_hall_data_parsing_dt(struct device *dev, struct hall_drvdata *ddata)
{
	struct device_node *np = dev->of_node;
	int gpio;
	enum of_gpio_flags flags;

	gpio = of_get_named_gpio_flags(np, "hall,gpio_wacom_cover", 0, &flags);
	ddata->gpio_wacom_cover = gpio;

	gpio = gpio_to_irq(gpio);
	ddata->irq_wacom_cover = gpio;
	pr_info("%s : gpio_wacom_cover=%d , irq_wacom_cover=%d\n", 
		__func__, ddata->gpio_wacom_cover, ddata->irq_wacom_cover);

	return 0;
}
#endif

static int hall_wacom_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hall_drvdata *ddata;
#ifdef CONFIG_WACOM_HALL_SUPPORT_COVER_DETECT
	struct input_dev *input;
#endif
	int error;
	int wakeup = 0;

	pr_info("%s called", __func__);
	ddata = kzalloc(sizeof(struct hall_drvdata), GFP_KERNEL);
	if (!ddata) {
		dev_err(dev, "failed to allocate state\n");
		return -ENOMEM;
	}

#ifdef CONFIG_OF
	if (dev->of_node) {
		error = of_hall_data_parsing_dt(dev, ddata);
		if (error < 0) {
			pr_info("%s : fail to get the dt (HALL)\n", __func__);
			goto fail1;
		}
	}
#endif

#ifdef CONFIG_WACOM_HALL_SUPPORT_COVER_DETECT
	input = input_allocate_device();
	if (!input) {
		dev_err(dev, "failed to allocate state\n");
		error = -ENOMEM;
		goto fail1;
	}

	ddata->input = input;

	input_set_drvdata(input, ddata);

	input->name = "wacom_hall";
	input->phys = "wacom_hall";
	input->dev.parent = &pdev->dev;

	input->evbit[0] |= BIT_MASK(EV_SW);
	input_set_capability(input, EV_SW, SW_CERTIFYHALL);

	input->open = wacom_hall_open;
	input->close = wacom_hall_close;

	/* Enable auto repeat feature of Linux input subsystem */
	__set_bit(EV_REP, input->evbit);

	error = input_register_device(input);
	if (error) {
		dev_err(dev, "Unable to register input device, error: %d\n",
			error);
		goto fail1;
	}
#endif
	wake_lock_init(&ddata->wacom_wake_lock, WAKE_LOCK_SUSPEND,
		"hall wacom wake lock");

	platform_set_drvdata(pdev, ddata);

	init_hall_ic_wacom_irq(ddata);

	error = device_create_file(hall_ic, &dev_attr_hall_wacom_detect);
	if (error < 0) {
		pr_err("Failed to create device file(%s)!, error: %d\n",
		dev_attr_hall_wacom_detect.attr.name, error);
	}

	device_init_wakeup(&pdev->dev, wakeup);

	pr_info("%s end", __func__);
	return 0;

fail1:
	kfree(ddata);

	return error;
}

static int hall_wacom_remove(struct platform_device *pdev)
{
	struct hall_drvdata *ddata = platform_get_drvdata(pdev);
#ifdef CONFIG_WACOM_HALL_SUPPORT_COVER_DETECT
	struct input_dev *input = ddata->input;
#endif
	pr_info("%s start\n", __func__);

	device_init_wakeup(&pdev->dev, 0);

	wake_lock_destroy(&ddata->wacom_wake_lock);
#ifdef CONFIG_WACOM_HALL_SUPPORT_COVER_DETECT
	input_unregister_device(input);
#endif
	kfree(ddata);

	return 0;
}

#if defined(CONFIG_OF)
static const struct of_device_id hall_wacom_dt_ids[] = {
	{ .compatible = "hall_wacom" },
	{ },
};
MODULE_DEVICE_TABLE(of, hall_wacom_dt_ids);
#endif /* CONFIG_OF */

#ifdef CONFIG_PM_SLEEP
static int hall_wacom_suspend(struct device *dev)
{
	struct hall_drvdata *ddata = dev_get_drvdata(dev);
#ifdef CONFIG_WACOM_HALL_SUPPORT_COVER_DETECT
	struct input_dev *input = ddata->input;
#endif
	pr_info("%s start\n", __func__);

#ifdef CONFIG_WACOM_HALL_SUPPORT_COVER_DETECT
	if (device_may_wakeup(dev)) {
		enable_irq_wake(ddata->irq_wacom_cover);
	} else {
		mutex_lock(&input->mutex);
		if (input->users)
			wacom_hall_close(input);
		mutex_unlock(&input->mutex);
	}
#else
	enable_irq_wake(ddata->irq_wacom_cover);
#endif

	return 0;
}

static int hall_wacom_resume(struct device *dev)
{
#ifdef CONFIG_WACOM_HALL_SUPPORT_COVER_DETECT
	struct hall_drvdata *ddata = dev_get_drvdata(dev);
	struct input_dev *input = ddata->input;
#endif

	pr_info("%s start\n", __func__);
#ifdef CONFIG_WACOM_HALL_SUPPORT_COVER_DETECT
	input_sync(input);
#endif
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(hall_wacom_pm_ops, hall_wacom_suspend, hall_wacom_resume);

static struct platform_driver hall_device_driver = {
	.probe		= hall_wacom_probe,
	.remove		= hall_wacom_remove,
	.driver		= {
		.name	= "hall_wacom",
		.owner	= THIS_MODULE,
		.pm	= &hall_wacom_pm_ops,
#if defined(CONFIG_OF)
		.of_match_table	= hall_wacom_dt_ids,
#endif /* CONFIG_OF */
	}
};

static int __init hall_wacom_init(void)
{
	pr_info("%s start\n", __func__);
	return platform_driver_register(&hall_device_driver);
}

static void __exit hall_wacom_exit(void)
{
	pr_info("%s start\n", __func__);
	platform_driver_unregister(&hall_device_driver);
}

late_initcall(hall_wacom_init);
module_exit(hall_wacom_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jjuny79.kim <jjuny79.kim@samsung.com>");
MODULE_DESCRIPTION("Hall IC wacom driver for GPIOs");
MODULE_ALIAS("platform:gpio-keys");
