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
#include <linux/input/pogo_i2c_notifier.h>
#include <linux/sec_class.h>
#include "stm/stm32l0/stm32_pogo_i2c.h"

enum LID_POSITION {
	E_LID_0 = 1,
	E_LID_NORMAL = 2,
	E_LID_360 = 3,
};

enum LOGICAL_HALL_STATUS
{
	LOGICAL_HALL_OPEN = 0,
	LOGICAL_HALL_CLOSE = 1,
	LOGICAL_HALL_BACK = 2,
};

struct device *hall_logical;

struct hall_drvdata {
	struct input_dev 				*input;
	struct notifier_block			pogo_nb;
};

static int hall_logical_status = 0;
static int hall_backflip_status = 0;

static ssize_t hall_logical_detect_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	if (hall_logical_status == LOGICAL_HALL_OPEN)
		sprintf(buf, "OPEN\n");
	else if (hall_logical_status == LOGICAL_HALL_CLOSE)
		sprintf(buf, "CLOSE\n");
	else
		sprintf(buf, "BACK\n");

	return strlen(buf);
}
static DEVICE_ATTR(hall_logical_detect, 0444, hall_logical_detect_show, NULL);

static int hall_logical_open(struct input_dev *input)
{
	/* Report current state of buttons that are connected to GPIOs */
	input_report_switch(input, SW_FLIP, hall_logical_status);
	input_sync(input);

	return 0;
}

static void hall_logical_close(struct input_dev *input)
{
}

#ifdef CONFIG_OF
static int of_hall_data_parsing_dt(struct device *dev, struct hall_drvdata *ddata)
{
	return 0;
}
#endif

static int logical_hallic_notifier_handler(struct notifier_block *nb,
		unsigned long action, void *data)
{
	struct hall_drvdata *logical_hall_dev = container_of(nb, struct hall_drvdata, pogo_nb);
	struct pogo_data_struct pogo_data =  *(struct pogo_data_struct *)data;
	int hall_status;

	switch (action) {
	case POGO_NOTIFIER_ID_DETACHED:
		hall_logical_status = LOGICAL_HALL_OPEN;
		input_report_switch(logical_hall_dev->input, SW_FLIP, hall_logical_status);
		input_sync(logical_hall_dev->input);
		break;
	case POGO_NOTIFIER_EVENTID_HALL:
		if (pogo_data.size != 1) {
			pr_info("%s size is wrong. size=%d!\n", __func__, pogo_data.size);
			break; 
		}

		hall_status = *pogo_data.data;

		if (hall_status == E_LID_0) {
			hall_logical_status = LOGICAL_HALL_CLOSE;
			input_info(true, &logical_hall_dev->input->dev, "%s hall_status = %d (CLOSE)\n", __func__, hall_status);
			input_report_switch(logical_hall_dev->input, SW_FLIP, hall_logical_status);
			input_sync(logical_hall_dev->input);
		} else if (hall_status == E_LID_NORMAL) {
			hall_logical_status = LOGICAL_HALL_OPEN;
			hall_backflip_status = 0;
			input_info(true, &logical_hall_dev->input->dev, "%s hall_status = %d (NORMAL)\n", __func__, hall_status);
			input_report_switch(logical_hall_dev->input, SW_FLIP, hall_logical_status);
			input_report_switch(logical_hall_dev->input, SW_HALL_LOGICAL, hall_backflip_status);
			input_sync(logical_hall_dev->input);
		} else if (hall_status == E_LID_360) {
			hall_backflip_status = 1;
			input_info(true, &logical_hall_dev->input->dev, "%s hall_status = %d (BACK)\n", __func__, hall_status);
			input_report_switch(logical_hall_dev->input, SW_HALL_LOGICAL, hall_backflip_status);
			input_sync(logical_hall_dev->input);
		}
		
		break;
	};

	return NOTIFY_DONE;
}

static int hall_logical_probe(struct platform_device *pdev)
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

	input = input_allocate_device();
	if (!input) {
		dev_err(dev, "failed to allocate state\n");
		error = -ENOMEM;
		goto fail1;
	}

	ddata->input = input;

	platform_set_drvdata(pdev, ddata);
	input_set_drvdata(input, ddata);

	input->name = "hall_logical";
	input->phys = "hall_logical";
	input->dev.parent = &pdev->dev;

	input->evbit[0] |= BIT_MASK(EV_SW);
	input_set_capability(input, EV_SW, SW_FLIP);
	input_set_capability(input, EV_SW, SW_HALL_LOGICAL);

	input->open = hall_logical_open;
	input->close = hall_logical_close;

	/* Enable auto repeat feature of Linux input subsystem */
	__set_bit(EV_REP, input->evbit);

	hall_logical = sec_device_create(ddata, "hall_logical");
	if (IS_ERR(hall_logical)) {
		dev_err(dev, "%s: failed to create device for the sysfs\n",__func__);
	}

	error = device_create_file(hall_logical, &dev_attr_hall_logical_detect);
	if (error < 0) {
		pr_err("Failed to create device file(%s)!, error: %d\n",
		dev_attr_hall_logical_detect.attr.name, error);
	}

	error = input_register_device(input);
	if (error) {
		dev_err(dev, "Unable to register input device, error: %d\n",
			error);
		goto fail1;
	}

	device_init_wakeup(&pdev->dev, wakeup);

	pogo_notifier_register(&ddata->pogo_nb,
			logical_hallic_notifier_handler, POGO_NOTIFY_DEV_HALLIC);

	input_report_switch(input, SW_FLIP, hall_logical_status);
	input_report_switch(input, SW_HALL_LOGICAL, hall_backflip_status);
	input_info(true, dev, "%s hall_status = %d backflip_status = %d\n", __func__, hall_logical_status, hall_backflip_status);

	pr_info("%s end", __func__);
	return 0;

 fail1:
	kfree(ddata);

	return error;
}

static int hall_logical_remove(struct platform_device *pdev)
{
	struct hall_drvdata *ddata = platform_get_drvdata(pdev);
	struct input_dev *input = ddata->input;

	pr_info("%s start\n", __func__);

	device_init_wakeup(&pdev->dev, 0);

	input_unregister_device(input);
	pogo_notifier_unregister(&ddata->pogo_nb);

	kfree(ddata);

	return 0;
}

#if defined(CONFIG_OF)
static const struct of_device_id hall_logical_dt_ids[] = {
	{ .compatible = "hall_logical" },
	{ },
};
MODULE_DEVICE_TABLE(of, hall_logical_dt_ids);
#endif /* CONFIG_OF */

#ifdef CONFIG_PM_SLEEP
static int hall_logical_suspend(struct device *dev)
{
	struct hall_drvdata *ddata = dev_get_drvdata(dev);
	struct input_dev *input = ddata->input;

	pr_info("%s start\n", __func__);

	if (device_may_wakeup(dev)) {
	} else {
		mutex_lock(&input->mutex);
		if (input->users)
			hall_logical_close(input);
		mutex_unlock(&input->mutex);
	}

	return 0;
}

static int hall_logical_resume(struct device *dev)
{
	struct hall_drvdata *ddata = dev_get_drvdata(dev);
	struct input_dev *input = ddata->input;

	pr_info("%s start\n", __func__);
	input_sync(input);
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(hall_logical_pm_ops, hall_logical_suspend, hall_logical_resume);

static struct platform_driver hall_device_driver = {
	.probe		= hall_logical_probe,
	.remove		= hall_logical_remove,
	.driver		= {
		.name	= "hall_logical",
		.owner	= THIS_MODULE,
		.pm	= &hall_logical_pm_ops,
#if defined(CONFIG_OF)
		.of_match_table	= hall_logical_dt_ids,
#endif /* CONFIG_OF */
	}
};

int hall_logical_init(void)
{
	pr_info("%s start\n", __func__);
	return platform_driver_probe(&hall_device_driver, hall_logical_probe);
}

void hall_logical_exit(void)
{
	pr_info("%s start\n", __func__);
	platform_driver_unregister(&hall_device_driver);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jjuny79.kim <jjuny79.kim@samsung.com>");
MODULE_DESCRIPTION("Hall IC common driver for GPIOs");
MODULE_ALIAS("platform:gpio-keys");
