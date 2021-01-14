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

#if !defined(CONFIG_SENSORS_HALL)
#include <linux/sec_class.h>
struct device *hall_ic;
EXPORT_SYMBOL(hall_ic);
#else
extern struct device *hall_ic;
#endif

struct certify_hall_drvdata {
	struct input_dev *input;
	int gpio_certify_cover;
	int irq_certify_cover;
	struct work_struct work;
	struct delayed_work certify_cover_dwork;
	struct wake_lock certify_wake_lock;
};

static bool certify_cover;

static ssize_t certify_hall_detect_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	if (!certify_cover)
		sprintf(buf, "OPEN\n");
	else
		sprintf(buf, "CLOSE\n");

	return strlen(buf);
}
static DEVICE_ATTR(certify_hall_detect, 0444, certify_hall_detect_show, NULL);

#ifdef CONFIG_SEC_FACTORY
static void certify_cover_work(struct work_struct *work)
{
	bool first, second;
	struct certify_hall_drvdata *ddata =
		container_of(work, struct certify_hall_drvdata,
				certify_cover_dwork.work);

	first = !gpio_get_value(ddata->gpio_certify_cover);

	pr_info("keys:%s #1 : %d\n", __func__, first);

	msleep(50);

	second = !gpio_get_value(ddata->gpio_certify_cover);

	pr_info("keys:%s #2 : %d\n", __func__, second);

	if (first == second) {
		certify_cover = first;
		input_report_switch(ddata->input, SW_CERTIFYHALL, certify_cover);
		input_sync(ddata->input);
	}
}
#else
static void certify_cover_work(struct work_struct *work)
{
	bool first;
	struct certify_hall_drvdata *ddata =
		container_of(work, struct certify_hall_drvdata,
				certify_cover_dwork.work);

	first = !gpio_get_value(ddata->gpio_certify_cover);

	pr_info("keys:%s #1 : %d\n", __func__, first);

	certify_cover = first;
	input_report_switch(ddata->input,
			SW_CERTIFYHALL, certify_cover);
	input_sync(ddata->input);
}
#endif

static void __certify_cover_detect(struct certify_hall_drvdata *ddata, bool flip_certify_status)
{
	cancel_delayed_work_sync(&ddata->certify_cover_dwork);
#ifdef CONFIG_SEC_FACTORY
	schedule_delayed_work(&ddata->certify_cover_dwork, HZ / 20);
#else
	if (flip_certify_status)	{
		wake_lock_timeout(&ddata->certify_wake_lock, HZ * 5 / 100); /* 50ms */
		schedule_delayed_work(&ddata->certify_cover_dwork, HZ * 1 / 100); /* 10ms */
	} else {
		wake_unlock(&ddata->certify_wake_lock);
		schedule_delayed_work(&ddata->certify_cover_dwork, 0);
	}
#endif
}

static irqreturn_t certify_cover_detect(int irq, void *dev_id)
{
	bool flip_certify_status;
	struct certify_hall_drvdata *ddata = dev_id;

	flip_certify_status = !gpio_get_value(ddata->gpio_certify_cover);

	pr_info("keys:%s flip_certify_status : %d\n",
		 __func__, flip_certify_status);

	__certify_cover_detect(ddata, flip_certify_status);

	return IRQ_HANDLED;
}

static int certify_hall_open(struct input_dev *input)
{
	struct certify_hall_drvdata *ddata = input_get_drvdata(input);
	/* update the current status */
	schedule_delayed_work(&ddata->certify_cover_dwork, HZ / 2);
	/* Report current state of buttons that are connected to GPIOs */
	input_sync(input);

	return 0;
}

static void certify_hall_close(struct input_dev *input)
{
}


static void init_certify_hall_ic_irq(struct input_dev *input)
{
	struct certify_hall_drvdata *ddata = input_get_drvdata(input);

	int ret = 0;
	int irq = ddata->irq_certify_cover;

	certify_cover = !gpio_get_value(ddata->gpio_certify_cover);

	INIT_DELAYED_WORK(&ddata->certify_cover_dwork, certify_cover_work);

	ret =
		request_threaded_irq(
		irq, NULL,
		certify_cover_detect,
		IRQF_TRIGGER_RISING |
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
		"certify_cover", ddata);
	if (ret < 0) {
		pr_info("keys: failed to certify request flip cover irq %d gpio %d\n",
			irq, ddata->gpio_certify_cover);
	} else {
		pr_info("%s : success\n", __func__);
	}
}

#ifdef CONFIG_OF
static int of_certify_hall_data_parsing_dt(struct device *dev, struct certify_hall_drvdata *ddata)
{
	struct device_node *np = dev->of_node;
	int gpio;
	enum of_gpio_flags flags;

	gpio = of_get_named_gpio_flags(np, "certify_hall,gpio_certify_cover", 0, &flags);
	ddata->gpio_certify_cover = gpio;

	gpio = gpio_to_irq(gpio);
	ddata->irq_certify_cover = gpio;

	return 0;
}
#endif

static int certify_hall_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct certify_hall_drvdata *ddata;
	struct input_dev *input;
	int error;
	int wakeup = 0;

	pr_info("%s start\n", __func__);

	ddata = kzalloc(sizeof(struct certify_hall_drvdata), GFP_KERNEL);
	if (!ddata)
		return -ENOMEM;

#ifdef CONFIG_OF
	if (dev->of_node) {
		error = of_certify_hall_data_parsing_dt(dev, ddata);
		if (error < 0) {
			pr_info("%s : fail to get the dt (HALL)\n", __func__);
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

	wake_lock_init(&ddata->certify_wake_lock, WAKE_LOCK_SUSPEND,
		"flip wake lock");

	platform_set_drvdata(pdev, ddata);
	input_set_drvdata(input, ddata);

	input->name = "certify_hall";
	input->phys = "certify_hall";
	input->dev.parent = &pdev->dev;

	input->evbit[0] |= BIT_MASK(EV_SW);
	input_set_capability(input, EV_SW, SW_CERTIFYHALL);

	input->open = certify_hall_open;
	input->close = certify_hall_close;

	/* Enable auto repeat feature of Linux input subsystem */
	__set_bit(EV_REP, input->evbit);

	init_certify_hall_ic_irq(input);

	if (ddata->gpio_certify_cover != 0) {
#if !defined(CONFIG_SENSORS_HALL)
		hall_ic = sec_device_create(ddata, "hall_ic");
#endif
		error = device_create_file(hall_ic, &dev_attr_certify_hall_detect);
		if (error < 0) {
			pr_err("Failed to create device file(%s)!, error: %d\n",
				dev_attr_certify_hall_detect.attr.name, error);
		}
	}

	error = input_register_device(input);
	if (error) {
		dev_err(dev, "Unable to register input device, error: %d\n",
			error);
		goto fail1;
	}

	device_init_wakeup(&pdev->dev, wakeup);

	pr_info("%s end\n", __func__);
	return 0;

 fail1:
	kfree(ddata);

	return error;
}

static int certify_hall_remove(struct platform_device *pdev)
{
	struct certify_hall_drvdata *ddata = platform_get_drvdata(pdev);
	struct input_dev *input = ddata->input;

	pr_info("%s start\n", __func__);

	device_init_wakeup(&pdev->dev, 0);

	input_unregister_device(input);

	wake_lock_destroy(&ddata->certify_wake_lock);

	kfree(ddata);

	return 0;
}

#if defined(CONFIG_OF)
static const struct of_device_id hall_dt_ids[] = {
	{ .compatible = "certify_hall" },
	{ },
};
MODULE_DEVICE_TABLE(of, hall_dt_ids);
#endif /* CONFIG_OF */

#ifdef CONFIG_PM_SLEEP
static int certify_hall_suspend(struct device *dev)
{
	struct certify_hall_drvdata *ddata = dev_get_drvdata(dev);
	struct input_dev *input = ddata->input;

	pr_info("%s start\n", __func__);

	enable_irq_wake(ddata->irq_certify_cover);

	if (device_may_wakeup(dev)) {
		enable_irq_wake(ddata->irq_certify_cover);
	} else {
		mutex_lock(&input->mutex);
		if (input->users)
			certify_hall_close(input);
		mutex_unlock(&input->mutex);
	}

	return 0;
}

static int certify_hall_resume(struct device *dev)
{
	struct certify_hall_drvdata *ddata = dev_get_drvdata(dev);
	struct input_dev *input = ddata->input;

	pr_info("%s start\n", __func__);
	input_sync(input);
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(hall_pm_ops, certify_hall_suspend, certify_hall_resume);

static struct platform_driver certify_hall_device_driver = {
	.probe		= certify_hall_probe,
	.remove		= certify_hall_remove,
	.driver		= {
		.name	= "certify_hall",
		.owner	= THIS_MODULE,
		.pm	= &hall_pm_ops,
#if defined(CONFIG_OF)
		.of_match_table	= hall_dt_ids,
#endif /* CONFIG_OF */
	}
};

static int __init certify_hall_init(void)
{
	pr_info("%s start\n", __func__);
	return platform_driver_register(&certify_hall_device_driver);
}

static void __exit certify_hall_exit(void)
{
	pr_info("%s start\n", __func__);
	platform_driver_unregister(&certify_hall_device_driver);
}

late_initcall(certify_hall_init);
module_exit(certify_hall_exit);
