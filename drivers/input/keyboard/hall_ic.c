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
#include <linux/sec_class.h>

#if 0 //defined(CONFIG_SEC_WINNERLTE_PROJECT)
#define EMULATE_HALL_IC
#endif

struct device *hall_ic;
EXPORT_SYMBOL(hall_ic);

struct hall_drvdata {
	struct input_dev *input;
	int gpio_flip_cover;
#if defined(EMULATE_HALL_IC)	
	int gpio_flip_cover_key1;
	int gpio_flip_cover_key2;
	int emulated_hall_ic_status;
#endif	
	int irq_flip_cover;
	bool flip_cover;
	struct work_struct work;
	struct delayed_work flip_cover_dwork;
	struct wake_lock flip_wake_lock;
};

static bool flip_cover = 1;

static ssize_t hall_detect_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	if (flip_cover)
		sprintf(buf, "CLOSE\n");
	else
		sprintf(buf, "OPEN\n");

	return strlen(buf);
}
static DEVICE_ATTR(hall_detect, 0444, hall_detect_show, NULL);

#ifdef CONFIG_SEC_FACTORY
static void flip_cover_work(struct work_struct *work)
{
	bool first, second;
	struct hall_drvdata *ddata =
		container_of(work, struct hall_drvdata,
				flip_cover_dwork.work);

	first = !gpio_get_value(ddata->gpio_flip_cover);

	pr_info("keys:%s #1 : %d\n", __func__, first);

	msleep(50);

	second = !gpio_get_value(ddata->gpio_flip_cover);

	pr_info("keys:%s #2 : %d\n", __func__, second);

	if (first == second) {
		flip_cover = first;
		input_report_switch(ddata->input, SW_FLIP, flip_cover);
		input_sync(ddata->input);
	}
}
#else
static void flip_cover_work(struct work_struct *work)
{
	bool first;
	struct hall_drvdata *ddata =
		container_of(work, struct hall_drvdata,
				flip_cover_dwork.work);

#if !defined(EMULATE_HALL_IC)
	ddata->flip_cover = !gpio_get_value(ddata->gpio_flip_cover);
	first = !gpio_get_value(ddata->gpio_flip_cover);

	pr_info("keys:%s #1 : %d\n", __func__, first);

	flip_cover = first;
	input_report_switch(ddata->input,
			SW_FLIP, ddata->flip_cover);
	input_sync(ddata->input);
#else
	ddata->flip_cover = !gpio_get_value(ddata->gpio_flip_cover_key1) & !gpio_get_value(ddata->gpio_flip_cover_key2);
	first = !gpio_get_value(ddata->gpio_flip_cover_key1) & !gpio_get_value(ddata->gpio_flip_cover_key2);	
	pr_info("keys:%s #1 key1=%d,key2=%d : %d\n", __func__, gpio_get_value(ddata->gpio_flip_cover_key1),gpio_get_value(ddata->gpio_flip_cover_key2),first);	
	if(flip_cover != first) {
		if(first)
		{		
			ddata->emulated_hall_ic_status = !ddata->emulated_hall_ic_status;
			pr_info("keys:%s #1 : %d , %d\n", __func__, first, ddata->emulated_hall_ic_status);	
			input_report_switch(ddata->input,
					SW_FLIP, ddata->emulated_hall_ic_status);
			input_sync(ddata->input);
		}
		flip_cover = first;
	}
	schedule_delayed_work(&ddata->flip_cover_dwork, msecs_to_jiffies(1000));
#endif

}
#endif

static void __flip_cover_detect(struct hall_drvdata *ddata, bool flip_status)
{
	cancel_delayed_work_sync(&ddata->flip_cover_dwork);
#ifdef CONFIG_SEC_FACTORY
	schedule_delayed_work(&ddata->flip_cover_dwork, HZ / 20);
#else
	if (!flip_status)	{
		wake_lock_timeout(&ddata->flip_wake_lock, HZ * 5 / 100); /* 50ms */
		schedule_delayed_work(&ddata->flip_cover_dwork, HZ * 1 / 100); /* 10ms */
	} else {
		wake_unlock(&ddata->flip_wake_lock);
		schedule_delayed_work(&ddata->flip_cover_dwork, 0);
	}
#endif
}

static irqreturn_t flip_cover_detect(int irq, void *dev_id)
{
	bool flip_status;
	struct hall_drvdata *ddata = dev_id;

#if !defined(EMULATE_HALL_IC)
	flip_status = !gpio_get_value(ddata->gpio_flip_cover);
#else
	flip_status = gpio_get_value(ddata->gpio_flip_cover_key1) & gpio_get_value(ddata->gpio_flip_cover_key2);
#endif


	pr_info("keys:%s flip_status : %d\n",
		 __func__, flip_status);


	__flip_cover_detect(ddata, flip_status);

	return IRQ_HANDLED;
}

static int hall_open(struct input_dev *input)
{
	struct hall_drvdata *ddata = input_get_drvdata(input);
	/* update the current status */
	schedule_delayed_work(&ddata->flip_cover_dwork, HZ / 2);
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
	int irq = ddata->irq_flip_cover;

#if !defined(EMULATE_HALL_IC)
	flip_cover = !gpio_get_value(ddata->gpio_flip_cover);
#else
	flip_cover = 0;
#endif

	INIT_DELAYED_WORK(&ddata->flip_cover_dwork, flip_cover_work);

	ret =
		request_threaded_irq(
		irq, NULL,
		flip_cover_detect,
		IRQF_TRIGGER_RISING |
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
		"flip_cover", ddata);
	if (ret < 0) {
		pr_info("keys: failed to request flip cover irq %d gpio %d\n",
			irq, ddata->gpio_flip_cover);
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

	gpio = of_get_named_gpio_flags(np, "hall,gpio_flip_cover", 0, &flags);
	ddata->gpio_flip_cover = gpio;

	gpio = gpio_to_irq(gpio);
	ddata->irq_flip_cover = gpio;
	pr_info("%s : gpio_flip_cover=%d , irq_flip_cover=%d\n", __func__, ddata->gpio_flip_cover, ddata->irq_flip_cover);

#if defined(EMULATE_HALL_IC)
	gpio = of_get_named_gpio_flags(np, "hall,gpio_flip_cover_key1", 0, &flags);
	ddata->gpio_flip_cover_key1 = gpio;
	
	gpio = of_get_named_gpio_flags(np, "hall,gpio_flip_cover_key2", 0, &flags);
	ddata->gpio_flip_cover_key2 = gpio;
	pr_info("%s : cover_key1=%d , cover_key2=%d\n", __func__, ddata->gpio_flip_cover_key1, ddata->gpio_flip_cover_key2);
#endif
	return 0;
}
#endif

static int hall_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hall_drvdata *ddata;
	struct input_dev *input;
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

#if defined(EMULATE_HALL_IC)
	ddata->emulated_hall_ic_status = 0;
#endif

	input = input_allocate_device();
	if (!input) {
		dev_err(dev, "failed to allocate state\n");
		error = -ENOMEM;
		goto fail1;
	}

	ddata->input = input;

	wake_lock_init(&ddata->flip_wake_lock, WAKE_LOCK_SUSPEND,
		"flip wake lock");

	platform_set_drvdata(pdev, ddata);
	input_set_drvdata(input, ddata);

	input->name = "hall";
	input->phys = "hall";
	input->dev.parent = &pdev->dev;

	input->evbit[0] |= BIT_MASK(EV_SW);
	input_set_capability(input, EV_SW, SW_FLIP);

	input->open = hall_open;
	input->close = hall_close;

	/* Enable auto repeat feature of Linux input subsystem */
	__set_bit(EV_REP, input->evbit);

	init_hall_ic_irq(input);

	if (ddata->gpio_flip_cover != 0) {
		hall_ic = sec_device_create(ddata, "hall_ic");

		error = device_create_file(hall_ic, &dev_attr_hall_detect);
		if (error < 0) {
			pr_err("Failed to create device file(%s)!, error: %d\n",
			dev_attr_hall_detect.attr.name, error);
		}
	}

	error = input_register_device(input);
	if (error) {
		dev_err(dev, "Unable to register input device, error: %d\n",
			error);
		goto fail1;
	}

	device_init_wakeup(&pdev->dev, wakeup);

	pr_info("%s end", __func__);
	return 0;

 fail1:
	kfree(ddata);

	return error;
}

static int hall_remove(struct platform_device *pdev)
{
	struct hall_drvdata *ddata = platform_get_drvdata(pdev);
	struct input_dev *input = ddata->input;

	pr_info("%s start\n", __func__);

	device_init_wakeup(&pdev->dev, 0);

	input_unregister_device(input);

	wake_lock_destroy(&ddata->flip_wake_lock);

	kfree(ddata);

	return 0;
}

#if defined(CONFIG_OF)
static const struct of_device_id hall_dt_ids[] = {
	{ .compatible = "hall" },
	{ },
};
MODULE_DEVICE_TABLE(of, hall_dt_ids);
#endif /* CONFIG_OF */

#ifdef CONFIG_PM_SLEEP
static int hall_suspend(struct device *dev)
{
	struct hall_drvdata *ddata = dev_get_drvdata(dev);
	struct input_dev *input = ddata->input;

	pr_info("%s start\n", __func__);

	enable_irq_wake(ddata->irq_flip_cover);

	if (device_may_wakeup(dev)) {
		enable_irq_wake(ddata->irq_flip_cover);
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

	pr_info("%s start\n", __func__);
	input_sync(input);
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(hall_pm_ops, hall_suspend, hall_resume);

static struct platform_driver hall_device_driver = {
	.probe		= hall_probe,
	.remove		= hall_remove,
	.driver		= {
		.name	= "hall",
		.owner	= THIS_MODULE,
		.pm	= &hall_pm_ops,
#if defined(CONFIG_OF)
		.of_match_table	= hall_dt_ids,
#endif /* CONFIG_OF */
	}
};

static int __init hall_init(void)
{
	pr_info("%s start\n", __func__);
	return platform_driver_register(&hall_device_driver);
}

static void __exit hall_exit(void)
{
	pr_info("%s start\n", __func__);
	platform_driver_unregister(&hall_device_driver);
}

late_initcall(hall_init);
module_exit(hall_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phil Blundell <pb@handhelds.org>");
MODULE_DESCRIPTION("Keyboard driver for GPIOs");
MODULE_ALIAS("platform:gpio-keys");
