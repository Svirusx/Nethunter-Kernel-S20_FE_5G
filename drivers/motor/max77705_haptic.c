/*
 * haptic motor driver for max77705_haptic.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "[VIB] " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/max77705.h>
#include <linux/mfd/max77705-private.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>

#define MOTOR_LRA                       (1<<7)
#define MOTOR_EN                        (1<<6)
#define EXT_PWM                         (0<<5)
#define DIVIDER_128                     (1<<1)
#define MRDBTMER_MASK                   (0x7)
#define MREN                            (1 << 3)
#define BIASEN                          (1 << 7)

struct max77705_haptic_drvdata {
	struct max77705_dev *max77705;
	struct i2c_client *i2c;
	struct max77705_haptic_pdata *pdata;
	struct delayed_work haptic_work;
	bool running;
};

static struct max77705_haptic_drvdata *max77705_g_hap_data;

static int max77705_haptic_i2c(void *data, bool en)
{
	struct max77705_haptic_drvdata *drvdata = (struct max77705_haptic_drvdata *)data;

	if (max77705_g_hap_data == NULL) {
		pr_info("[VIB]%s null reference\n", __func__);
		return -1;
	}

	max77705_update_reg(drvdata->i2c,
			MAX77705_PMIC_REG_MCONFIG,
			en ? 0xff : 0x0, MOTOR_LRA | MOTOR_EN);
	return 0;
}

#if defined(CONFIG_SS_VIBRATOR)
void max77705_vibtonz_en(bool en)
{
	if (max77705_g_hap_data == NULL)
		return;

	if (en) {
		if (max77705_g_hap_data->running)
			return;
		max77705_haptic_i2c(max77705_g_hap_data, true);
		max77705_g_hap_data->running = true;
	} else {
		if (!max77705_g_hap_data->running)
			return;
		max77705_haptic_i2c(max77705_g_hap_data, false);
		max77705_g_hap_data->running = false;
	}
}
EXPORT_SYMBOL(max77705_vibtonz_en);
#endif

static void max77705_haptic_init_reg(struct max77705_haptic_drvdata *drvdata)
{
	int ret;

	ret = max77705_update_reg(drvdata->i2c,
			MAX77705_PMIC_REG_MAINCTRL1, 0xff, BIASEN);
	if (ret)
		pr_err("i2c REG_BIASEN update error %d\n", ret);

	ret = max77705_update_reg(drvdata->i2c,
			MAX77705_PMIC_REG_MCONFIG, 0xff, MOTOR_LRA);
	if (ret)
		pr_err("i2c MOTOR_LRA update error %d\n", ret);

	ret = max77705_update_reg(drvdata->i2c,
			MAX77705_PMIC_REG_MCONFIG, 0xff, DIVIDER_128);
	if (ret)
		pr_err("i2c DIVIDER_128 update error %d\n", ret);
}

static void uvlo_haptic_init_reg(struct work_struct *work)
{
	struct max77705_haptic_drvdata *drvdata = container_of(work, struct max77705_haptic_drvdata, haptic_work.work);
	u8 reg_data;

	max77705_read_reg(drvdata->i2c,
			MAX77705_PMIC_REG_MCONFIG, &reg_data);
	pr_info("[VIB],before haptic reg init, data = %02x\n", reg_data);

	max77705_haptic_init_reg(drvdata);
	max77705_vibtonz_en(true);

	max77705_read_reg(drvdata->i2c,
			MAX77705_PMIC_REG_MCONFIG, &reg_data);
	pr_info("[VIB],after haptic reg init, data = %02x\n", reg_data);
}

static irqreturn_t max77705_haptic_irq(int irq, void *data)
{
	struct max77705_haptic_drvdata *drvdata = data;

	pr_info("%s: [VIB] UVLO INT occurred, init haptic reg\n", __func__);

	schedule_delayed_work(&drvdata->haptic_work, msecs_to_jiffies(1000));

	return IRQ_HANDLED;
}

#if defined(CONFIG_OF)
static struct max77705_haptic_pdata *of_max77705_haptic_dt(struct device *dev)
{
	struct device_node *np = dev->parent->of_node;
	struct max77705_haptic_pdata *pdata;

	pdata = kzalloc(sizeof(struct max77705_haptic_drvdata), GFP_KERNEL);
	if (pdata == NULL)
		return NULL;

	if (!of_property_read_u32(np, "haptic,mode", &pdata->mode)) {
		pr_info("[VIB] %s: mode reference fail\n", __func__);
		pdata->mode = 1;
	}
	if (!of_property_read_u32(np, "haptic,divisor", &pdata->divisor)) {
		pr_info("[VIB] %s: divisor reference fail\n", __func__);
		pdata->divisor = 128;
	}

	pr_info("[VIB] %s: mode: %d\n", __func__, pdata->mode);
	pr_info("[VIB] %s: divisor: %d\n", __func__, pdata->divisor);

	return pdata;
}
#endif

static int max77705_haptic_probe(struct platform_device *pdev)
{
	int error = 0;
	struct max77705_dev *max77705 = dev_get_drvdata(pdev->dev.parent);
	u8 reg_data;
	struct max77705_platform_data *max77705_pdata
		= dev_get_platdata(max77705->dev);
	struct max77705_haptic_pdata *pdata
		= max77705_pdata->haptic_data;
	struct max77705_haptic_drvdata *drvdata;
	int irq_base = max77705->pdata->irq_base;

#if defined(CONFIG_OF)
	if (pdata == NULL) {
		pdata = of_max77705_haptic_dt(&pdev->dev);
		if (unlikely(!pdata)) {
			pr_err("max77705-haptic : %s not found haptic dt!\n",
					__func__);
			return -1;
		}
	}
#else
	if (unlikely(!pdata)) {
		pr_err("%s: no pdata\n", __func__);
		return -ENODEV;
	}
#endif /* CONFIG_OF */

	drvdata = kzalloc(sizeof(struct max77705_haptic_drvdata), GFP_KERNEL);
	if (unlikely(!drvdata))
		goto err_alloc1;

	platform_set_drvdata(pdev, drvdata);
	drvdata->max77705 = max77705;
	drvdata->i2c = max77705->i2c;
	drvdata->pdata = pdata;
	max77705_g_hap_data = drvdata;

	INIT_DELAYED_WORK(&drvdata->haptic_work, uvlo_haptic_init_reg);

	max77705_haptic_init_reg(drvdata);
	max77705_read_reg(drvdata->i2c,
			MAX77705_PMIC_REG_MCONFIG, &reg_data);

	pdata->irq = irq_base + MAX77705_SYSTEM_IRQ_SYSUVLO_INT;
	error = request_threaded_irq(pdata->irq, NULL, max77705_haptic_irq,
				IRQF_NO_SUSPEND, "max77705_UVLO", drvdata);
	if (error < 0) {
		pr_err("%s: Failed to request IRQ #%d: %d\n",
				__func__, pdata->irq, error);
		pdata->irq = 0;
	}

	return 0;
err_alloc1:
	kfree(drvdata);
	return error;
}

static int max77705_haptic_remove(struct platform_device *pdev)
{
	struct max77705_haptic_drvdata *drvdata
		= platform_get_drvdata(pdev);

	max77705_haptic_i2c(drvdata, false);
	kfree(drvdata);
	return 0;
}

static int max77705_haptic_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int max77705_haptic_resume(struct platform_device *pdev)
{
	return 0;
}

static void max77705_haptic_shutdown(struct platform_device *pdev)
{
	struct max77705_haptic_drvdata *drvdata
		= platform_get_drvdata(pdev);

	pr_info("[VIB] %s : Disable Haptic\n", __func__);
	max77705_haptic_i2c(drvdata, false);
}

static struct platform_driver max77705_haptic_driver = {
	.probe		= max77705_haptic_probe,
	.remove		= max77705_haptic_remove,
	.suspend	= max77705_haptic_suspend,
	.resume		= max77705_haptic_resume,
	.shutdown	= max77705_haptic_shutdown,
	.driver = {
		.name	= "max77705-haptic",
		.owner	= THIS_MODULE,
	},
};

static int __init max77705_haptic_init(void)
{
	return platform_driver_register(&max77705_haptic_driver);
}
module_init(max77705_haptic_init);

static void __exit max77705_haptic_exit(void)
{
	platform_driver_unregister(&max77705_haptic_driver);
}
module_exit(max77705_haptic_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("max77705 haptic driver");
