/*
 * LED driver for leds-rt8547
 *
 * Copyright (C) 2014 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/pwm.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/leds-rt8547.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif

extern struct class *camera_class; /*sys/class/camera*/
struct device *rt8547_led_dev;

struct rt8547_led_platform_data *global_rt8547data;
struct device *global_dev;

bool sysfs_flash_op;
bool flash_config_factory;

void rt8547_led_setGpio(int onoff)
{
	if (onoff) {
		gpio_direction_output(global_rt8547data->flash_control, 1);
		gpio_direction_output(global_rt8547data->flash_en, 1);
	} else {
		gpio_direction_output(global_rt8547data->flash_control, 0);
		gpio_direction_output(global_rt8547data->flash_en, 0);
	}
}

void rt8547_led_set_low_bit(void)
{
	gpio_direction_output(global_rt8547data->flash_control, 0);
	udelay(T_LONG);
	gpio_direction_output(global_rt8547data->flash_control, 1);
	udelay(T_SHORT);
}

void rt8547_led_set_high_bit(void)
{
	gpio_direction_output(global_rt8547data->flash_control, 0);
	udelay(T_SHORT);
	gpio_direction_output(global_rt8547data->flash_control, 1);
	udelay(T_LONG);
}

static int rt8547_led_set_bit(unsigned int bit)
{
	if (bit) {
		rt8547_led_set_high_bit();
	} else {
		rt8547_led_set_low_bit();
	}

	return 0;
}

static int rt8547_led_write_one_wire_data(unsigned int data, unsigned int len)
{
	int i = 0;

	for ( i = len-1; i >= 0; i--) {
		rt8547_led_set_bit((data >> i) & 0x01);
	}

	return 0;
}

static int rt8547_led_write_data(unsigned int regAddr, unsigned int data)
{
	int err = 0;

	gpio_direction_output(global_rt8547data->flash_control, 1);
	udelay(T_SOD);

	/* Write Slave Address */
	rt8547_led_write_one_wire_data(RT8547_SLAVE_ADDR, 8);

	/* Write Reg Address */
	rt8547_led_write_one_wire_data(regAddr, 3);

	/* Write Data */
	rt8547_led_write_one_wire_data(data, 8);

	/* Data End Condition */
	gpio_direction_output(global_rt8547data->flash_control, 0);
	udelay(T_SHORT);
	gpio_direction_output(global_rt8547data->flash_control, 1);
	udelay(T_EOD);

	return err;
}

ssize_t rt8547_led_store(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	int value = 0;
	int ret = 0;
	int torch_step = 0;
	unsigned long flags = 0;
	sysfs_flash_op = false;

	if ((buf == NULL) || kstrtouint(buf, 10, &value)) {
		return -1;
	}

	global_rt8547data->sysfs_input_data = value;

	ret = gpio_request(global_rt8547data->flash_control, "rt8547_led_control");
	if (ret) {
		pr_err("%s: Failed to requeset rt8547_led_control\n", __func__);
		return -1;
	}

	ret = gpio_request(global_rt8547data->flash_en, "rt8547_led_en");
	if (ret) {
		gpio_free(global_rt8547data->flash_control);
		pr_err("%s: Failed to requeset rt8547_led_en\n", __func__);
		return -1;
	}

	if (value <= 0) {
		pr_info("%s: TORCH OFF. : E(%d)\n", __func__, value);
		sysfs_flash_op = false;
		global_rt8547data->mode_status = RT8547_DISABLES_TORCH_FLASH_MODE;

		spin_lock_irqsave(&global_rt8547data->int_lock, flags);
		rt8547_led_setGpio(0);
		spin_unlock_irqrestore(&global_rt8547data->int_lock, flags);

		pr_info("%s: TORCH OFF. : X(%d)\n", __func__, value);
	} else {
		pr_info("%s: TORCH ON. : E(%d)\n", __func__, value);
		sysfs_flash_op = true;
		global_rt8547data->mode_status = RT8547_ENABLE_TORCH_MODE;
		spin_lock_irqsave(&global_rt8547data->int_lock, flags);
		rt8547_led_write_data(RT8547_ADDR_LVP_SETTING, RT8547_3V);
		if (value == 100) {
			pr_info("%s: sysfs flash value %d\n", __func__, value);
			rt8547_led_write_data(RT8547_ADDR_CURRENT_SETTING,
				global_rt8547data->factory_current_value | RT8547_TORCH_SELECT);
		} else if (value == 200) {
			pr_info("%s: sysfs flash value %d\n", __func__, value);
			/* Factory mode Turn on Flash */
			flash_config_factory = true;
			rt8547_led_write_data(RT8547_ADDR_CURRENT_SETTING,
				global_rt8547data->factory_current_value | RT8547_TORCH_SELECT);
		} else if ((1001 <= value) && (value <= 2015)) {
			/* (value) 1001, 1002, 1004, 1006, 1009
			: (torch_step) 1(50mA), 3(100mA), 5(150mA), 6(175mA), 7(200mA) */
			if (value <= 1001)
				torch_step = 1;
			else if (value <= 1002)
				torch_step = 3;
			else if (value <= 1004)
				torch_step = 4;
			else if (value <= 1006)
				torch_step = 5;
			else if (value <= 1009)
				torch_step = 7;
			else if (value <= 1010)
				torch_step = 8;
			else if ((2001 <= value) && (value <= 2015))
				torch_step = value - 2000;
			else
				torch_step = 1;

			rt8547_led_write_data(RT8547_ADDR_CURRENT_SETTING,
				torch_step | RT8547_TORCH_SELECT);
			pr_info("%s: torch current enum(%d) : %d mA\n", __func__, torch_step, torch_step * 25 + 25);
		} else {
			rt8547_led_write_data(RT8547_ADDR_CURRENT_SETTING,
				global_rt8547data->torch_current_value | RT8547_TORCH_SELECT);
			pr_info("%s: global_rt8547data->torch_current_value(%d)\n", __func__,
				global_rt8547data->torch_current_value);
		}
		if (flash_config_factory)
		{
			rt8547_led_write_data(RT8547_ADDR_FLASH_CURRENT_LEVEL_TIMEOUT_SETTING,
				(RT8547_TIMEOUT_CURRENT_400mA << 5) | RT8547_FLASH_CURRENT_250mA);
			flash_config_factory = false;
		}
		else
		{
			rt8547_led_write_data(RT8547_ADDR_FLASH_CURRENT_LEVEL_TIMEOUT_SETTING,
				(RT8547_TIMEOUT_CURRENT_400mA << 5) | RT8547_FLASH_CURRENT_150mA);
		}

		rt8547_led_write_data(RT8547_ADDR_FLASH_TIMEOUT_SETTING, RT8547_TIMER_1216ms);

		gpio_direction_output(global_rt8547data->flash_en, 1);
		spin_unlock_irqrestore(&global_rt8547data->int_lock, flags);

		pr_info("%s: TORCH ON. : X(%d)\n", __func__, value);
	}

	gpio_free(global_rt8547data->flash_control);
	gpio_free(global_rt8547data->flash_en);

	return count;
}

EXPORT_SYMBOL(rt8547_led_mode_ctrl);

int64_t rt8547_led_mode_ctrl(int state, int value)
{
	int ret = 0;
	unsigned long flags = 0;

	if (sysfs_flash_op == true) {
		pr_info("%s: sysfs_flash_op is enabled \n", __func__);
		return 0;
	}

	ret = gpio_request(global_rt8547data->flash_control, "rt8547_led_control");
	if (ret) {
		pr_err("%s: Failed to requeset rt8547_led_control\n", __func__);
		return ret;
	}

	ret = gpio_request(global_rt8547data->flash_en, "rt8547_led_en");
	if (ret) {
		gpio_free(global_rt8547data->flash_control);
		pr_err("%s: Failed to requeset rt8547_led_en\n", __func__);
		return ret;
	}

	switch(state) {
		case RT8547_ENABLE_PRE_FLASH_MODE:
			/* FlashLight Mode Pre Flash */
			pr_info("%s: Pre Flash ON E(%d)\n", __func__, state);
			global_rt8547data->mode_status = RT8547_ENABLE_PRE_FLASH_MODE;
			spin_lock_irqsave(&global_rt8547data->int_lock, flags);
			rt8547_led_write_data(RT8547_ADDR_LVP_SETTING, global_rt8547data->LVP_Voltage);
			rt8547_led_write_data(RT8547_ADDR_CURRENT_SETTING,
								global_rt8547data->pre_current_value|RT8547_TORCH_SELECT);
			rt8547_led_write_data(RT8547_ADDR_FLASH_CURRENT_LEVEL_TIMEOUT_SETTING,
				(RT8547_TIMEOUT_CURRENT_400mA << 5) | RT8547_FLASH_CURRENT_150mA);
			spin_unlock_irqrestore(&global_rt8547data->int_lock, flags);

			pr_info("%s: Pre Flash ON X(%d)\n", __func__, state);
			break;
		case RT8547_ENABLE_TORCH_MODE:
			/* FlashLight Mode TORCH */
			pr_info("%s: TORCH ON E(%d)\n", __func__, state);
			global_rt8547data->mode_status = RT8547_ENABLE_TORCH_MODE;
			spin_lock_irqsave(&global_rt8547data->int_lock, flags);
			if(value == 0)
			{
				rt8547_led_write_data(RT8547_ADDR_LVP_SETTING, global_rt8547data->LVP_Voltage);
				rt8547_led_write_data(RT8547_ADDR_CURRENT_SETTING,
							global_rt8547data->torch_current_value|RT8547_TORCH_SELECT);
				rt8547_led_write_data(RT8547_ADDR_FLASH_CURRENT_LEVEL_TIMEOUT_SETTING,
					(RT8547_TIMEOUT_CURRENT_400mA << 5) | RT8547_FLASH_CURRENT_150mA);
			}
			else
			{
				rt8547_led_write_data(RT8547_ADDR_LVP_SETTING, global_rt8547data->LVP_Voltage);
				rt8547_led_write_data(RT8547_ADDR_CURRENT_SETTING, value|RT8547_TORCH_SELECT);
				rt8547_led_write_data(RT8547_ADDR_FLASH_CURRENT_LEVEL_TIMEOUT_SETTING,
					(RT8547_TIMEOUT_CURRENT_400mA << 5) | RT8547_FLASH_CURRENT_150mA);
				rt8547_led_write_data(RT8547_ADDR_FLASH_TIMEOUT_SETTING, RT8547_TIMER_1216ms);
				gpio_direction_output(global_rt8547data->flash_en, 1);
			}
			spin_unlock_irqrestore(&global_rt8547data->int_lock, flags);
			pr_info("%s: TORCH ON X(%d)\n", __func__, state);
			break;
		case RT8547_ENABLE_FLASH_MODE:
			/* FlashLight Mode Flash */
			pr_info("%s: FLASH ON E(%d)\n", __func__, state);
			global_rt8547data->mode_status = RT8547_ENABLE_FLASH_MODE;
			spin_lock_irqsave(&global_rt8547data->int_lock, flags);
			if(value == 0)
			{
				rt8547_led_write_data(RT8547_ADDR_LVP_SETTING, global_rt8547data->LVP_Voltage); // LVP setting
				rt8547_led_write_data(RT8547_ADDR_CURRENT_SETTING, RT8547_STROBE_SELECT); // Strobe select
				rt8547_led_write_data(RT8547_ADDR_FLASH_CURRENT_LEVEL_TIMEOUT_SETTING,
					(RT8547_TIMEOUT_CURRENT_400mA << 5) | global_rt8547data->flash_current_value);
			}
			{
				rt8547_led_write_data(RT8547_ADDR_LVP_SETTING, global_rt8547data->LVP_Voltage); // LVP setting
				rt8547_led_write_data(RT8547_ADDR_CURRENT_SETTING, RT8547_STROBE_SELECT); // Strobe select
				rt8547_led_write_data(RT8547_ADDR_FLASH_CURRENT_LEVEL_TIMEOUT_SETTING,
					(RT8547_TIMEOUT_CURRENT_400mA << 5) | value);
				rt8547_led_write_data(RT8547_ADDR_FLASH_TIMEOUT_SETTING, RT8547_TIMER_1216ms);

				gpio_direction_output(global_rt8547data->flash_en, 1);
			}
			spin_unlock_irqrestore(&global_rt8547data->int_lock, flags);
			pr_info("%s: FLASH ON X(%d)\n", __func__, state);
			break;
		case RT8547_DISABLES_TORCH_FLASH_MODE:
		default:
			/* FlashLight Mode OFF */
			pr_info("%s: FLASH OFF E(%d)\n", __func__, state);
			global_rt8547data->mode_status = RT8547_DISABLES_TORCH_FLASH_MODE;
			spin_lock_irqsave(&global_rt8547data->int_lock, flags);
			rt8547_led_setGpio(0);
			spin_unlock_irqrestore(&global_rt8547data->int_lock, flags);

			pr_info("%s: FLASH OFF X(%d)\n", __func__, state);
			break;
	}

	gpio_free(global_rt8547data->flash_en);
	gpio_free(global_rt8547data->flash_control);


	return ret;
}

int32_t rt8547_led_set_torch(int curr)
{
	int ret = 0;
	unsigned long flags = 0;

	if (curr == -1) {
		spin_lock_irqsave(&global_rt8547data->int_lock, flags);
		rt8547_led_setGpio(0);
		spin_unlock_irqrestore(&global_rt8547data->int_lock, flags);

		LED_INFO("RT8547-FLASH OFF X(%d)\n", curr);
		return 0;
	}

	ret = gpio_request(global_rt8547data->flash_control, "rt8547_led_control");
	if (ret) {
		LED_ERROR("Failed to request rt8547_led_mode_ctrl\n");
		return ret;
	}

	/* FlashLight Mode TORCH for flicker sensor */
	LED_INFO("RT8547-FLICKERTEST ON E(%d)\n", curr);

	spin_lock_irqsave(&global_rt8547data->int_lock, flags);
	rt8547_led_write_data(RT8547_ADDR_LVP_SETTING, global_rt8547data->LVP_Voltage);
	rt8547_led_write_data(RT8547_ADDR_CURRENT_SETTING,
			curr|RT8547_TORCH_SELECT);
	rt8547_led_write_data(RT8547_ADDR_FLASH_CURRENT_LEVEL_TIMEOUT_SETTING,
		(RT8547_TIMEOUT_CURRENT_400mA << 5) | RT8547_FLASH_CURRENT_150mA);
	spin_unlock_irqrestore(&global_rt8547data->int_lock, flags);
	LED_INFO("RT8547-FLICKERTEST ON X(%d)\n", curr);

	gpio_free(global_rt8547data->flash_control);

	return ret;
}

ssize_t rt8547_led_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", global_rt8547data->sysfs_input_data);
}

static DEVICE_ATTR(rear_flash, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
	rt8547_led_show, rt8547_led_store);
static DEVICE_ATTR(rear_torch_flash, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
	rt8547_led_show, rt8547_led_store);

static int rt8547_led_parse_dt(struct device *dev,
                                struct rt8547_led_platform_data *pdata)
{
	struct device_node *dnode = dev->of_node;
	u32 buffer = 0;
	int ret = 0;

	/* Defulat Value */
	pdata->LVP_Voltage = RT8547_3V;
	pdata->flash_timeout = RT8547_TIMER_992ms;
	pdata->timeout_current_value = RT8547_TIMEOUT_CURRENT_350mA;
	pdata->flash_current_value = RT8547_FLASH_CURRENT_1500mA;
	pdata->torch_current_value = RT8547_TORCH_CURRENT_200mA;
	pdata->pre_current_value = RT8547_TORCH_CURRENT_275mA;
	pdata->factory_current_value = RT8547_TORCH_CURRENT_400mA;
	pdata->mode_status = RT8547_DISABLES_TORCH_FLASH_MODE;

	/* get gpio */
	pdata->flash_control = of_get_named_gpio(dnode, "flash_control", 0);
	if (!gpio_is_valid(pdata->flash_control)) {
		pr_err("%s: failed to get flash_control\n", __func__);
		return -1;
	}

	pdata->flash_en = of_get_named_gpio(dnode, "flash_en", 0);
	if (!gpio_is_valid(pdata->flash_en)) {
		pr_err("%s: failed to get flash_en\n", __func__);
		return -1;
	}

	/* get flash current value */
	if (of_property_read_u32(dnode, "flash_current", &buffer) == 0) {
		pr_info("%s: flash_current = <%d><%d>\n", __func__,buffer,RT8547_FLASH_CURRENT(buffer)&0x1F);
		pdata->flash_current_value = RT8547_FLASH_CURRENT(buffer)&0x1F;
	}

	/* get torch current value */
	if (of_property_read_u32(dnode, "torch_current", &buffer) == 0) {
		pr_info("%s: torch_current = <%d><%d>\n", __func__,buffer, RT8547_TORCH_CURRENT(buffer)&0x0F);
		pdata->torch_current_value = RT8547_TORCH_CURRENT(buffer)&0x0F;
	}

	/* get factory current value */
	if (of_property_read_u32(dnode, "factory_current", &buffer) == 0) {
		pr_info("%s: factory_current = <%d><%d>\n", __func__,buffer, RT8547_TORCH_CURRENT(buffer)&0x0F);
		pdata->factory_current_value = RT8547_TORCH_CURRENT(buffer)&0x0F;
	}

	/* get pre current value */
	if (of_property_read_u32(dnode, "pre_current", &buffer) == 0) {
		pr_info("%s: pre_current = <%d><%d>\n", __func__,buffer, RT8547_TORCH_CURRENT(buffer)&0x0F);
		pdata->pre_current_value = RT8547_TORCH_CURRENT(buffer)&0x0F;
	}

	return ret;
}

static int rt8547_led_probe(struct platform_device *pdev)
{
	struct rt8547_led_platform_data *pdata;
	int ret = 0;

	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			pr_err("%s: failed to allocate driver data\n", __func__);
			return -ENOMEM;
		}
		ret = rt8547_led_parse_dt(&pdev->dev, pdata);
		if (ret < 0) {
			pr_err("%s: not found torch dt! ret[%d]\n",
					 __func__, ret);
			kfree(pdata);
			return ret;
		}
	} else {
		pdata = pdev->dev.platform_data;
		if (pdata == NULL) {
			pr_err("%s: no platform data for this led is found\n", __func__);
			return -EFAULT;
		}
	}

	global_rt8547data = pdata;
	global_dev = &pdev->dev;

	pr_info("%s: RT8547_LED Probe\n", __func__);

	gpio_request_one(global_rt8547data->flash_control, GPIOF_OUT_INIT_LOW, "rt8547_led_control");
	gpio_request_one(global_rt8547data->flash_en, GPIOF_OUT_INIT_LOW, "rt8547_led_en");
	gpio_free(global_rt8547data->flash_control);
	gpio_free(global_rt8547data->flash_en);

	if (camera_class == NULL) {
		camera_class = class_create(THIS_MODULE, "camera");
		if (IS_ERR(camera_class))
			pr_err("%s: failed to create device cam_dev_rear!\n", __func__);
	}
	
	if (camera_class != NULL) {
		rt8547_led_dev = device_create(camera_class, NULL, 0, NULL, "flash");
		if (IS_ERR(rt8547_led_dev)) {
			pr_err("%s: Failed to create device(flash)!\n", __func__);
			return -ENODEV;
		}

		if (device_create_file(rt8547_led_dev, &dev_attr_rear_flash) < 0) {
			pr_err("%s: failed to create device file, %s\n",
					__func__, dev_attr_rear_flash.attr.name);
			return -ENOENT;
		}
		if (device_create_file(rt8547_led_dev, &dev_attr_rear_torch_flash) < 0) {
			pr_err("%s: failed to create device file, %s\n",
					__func__, dev_attr_rear_torch_flash.attr.name);
			return -ENOENT;
		}
	}
	spin_lock_init(&pdata->int_lock);

	return 0;
}
static int rt8547_led_remove(struct platform_device *pdev)
{
	device_remove_file(rt8547_led_dev, &dev_attr_rear_flash);
	device_remove_file(rt8547_led_dev, &dev_attr_rear_torch_flash);
	device_destroy(camera_class, 0);
	class_destroy(camera_class);

	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id cam_flash_dt_match[] = {
	{ .compatible = "qcom,rt8547-fled",},
	{},
};
/*MODULE_DEVICE_TABLE(of, cam_flash_dt_match);*/
#endif

static struct platform_driver rt8547_led_driver = {
	.probe		= rt8547_led_probe,
	.remove		= rt8547_led_remove,
	.driver		= {
		.name	= "rt8547-led",
		.owner	= THIS_MODULE,
#if defined(CONFIG_OF)
		.of_match_table = cam_flash_dt_match,
#endif
		.suppress_bind_attrs = true,
	},
};

#if defined(CONFIG_SEC_R8Q_PROJECT)
static unsigned int system_rev __read_mostly;

static int __init sec_hw_rev_setup(char *p)
{
	int ret;

	ret = kstrtouint(p, 0, &system_rev);
	if (unlikely(ret < 0)) {
		pr_info("%s:androidboot.revision is malformed %s ",__func__, p);
		return -EINVAL;
	}

	pr_info("%s:androidboot.revision %x \n", __func__,system_rev);

	return 0;
}
early_param("androidboot.revision", sec_hw_rev_setup);

static unsigned int sec_hw_rev(void)
{
	return system_rev;
}
#endif 

static int __init rt8547_led_driver_init(void)
{
	int rc = 0;
	#if defined(CONFIG_SEC_R8Q_PROJECT)
	unsigned int board_rev = sec_hw_rev();
	if(board_rev < 8)
	{
		return platform_driver_register(&rt8547_led_driver);
	}
	#else
		return platform_driver_register(&rt8547_led_driver);
	#endif

	if (rc < 0) {
		pr_info("%s: platform_driver_register Failed: rc = %d",
			__func__, rc);
		return rc;
	}
	return rc;
}

static void __exit rt8547_led_driver_exit(void)
{
	platform_driver_unregister(&rt8547_led_driver);
}

module_init(rt8547_led_driver_init);
module_exit(rt8547_led_driver_exit);

MODULE_DESCRIPTION("RT8547 flash LED driver");
MODULE_LICENSE("GPL");


