/*
 * LED driver - leds-ktd2692.c
 *
 * Copyright (C) 2020 Sunggeun Yim <sunggeun.yim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/pwm.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/leds-ktd2692.h>
#include <linux/time.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif

//#define DEBUG_LED_TIME
extern struct class *camera_class; /*sys/class/camera*/
struct device *ktd2692_led_dev;

struct ktd2692_platform_data *global_ktd2692data = NULL;
struct device *global_dev_kt;
bool sysfs_flash_op_kt;

void ktd2692_setGpio(int onoff)
{
	if (onoff) {
		if (gpio_is_valid(global_ktd2692data->flash_control))
			__gpio_set_value(global_ktd2692data->flash_control, 1);
		if (gpio_is_valid(global_ktd2692data->flash_en))
			__gpio_set_value(global_ktd2692data->flash_en, 1);
	} else {
		if (gpio_is_valid(global_ktd2692data->flash_control))
			__gpio_set_value(global_ktd2692data->flash_control, 0);
		if (gpio_is_valid(global_ktd2692data->flash_en))
			__gpio_set_value(global_ktd2692data->flash_en, 0);
	}
}

void ktd2692_set_low_bit(void)
{
#ifdef DEBUG_LED_TIME
	struct timeval start_low, end_low, start_high, end_high;
	unsigned long time_low, time_high;
#endif

#ifdef DEBUG_LED_TIME
	do_gettimeofday(&start_low);
#endif

	if (gpio_is_valid(global_ktd2692data->flash_control))
		__gpio_set_value(global_ktd2692data->flash_control, 0);
	udelay(T_L_LB);

#ifdef DEBUG_LED_TIME
	do_gettimeofday(&end_low);

	do_gettimeofday(&start_high);
#endif

	if (gpio_is_valid(global_ktd2692data->flash_control))
		__gpio_set_value(global_ktd2692data->flash_control, 1);
	udelay(T_H_LB);

#ifdef DEBUG_LED_TIME
	do_gettimeofday(&end_high);

	time_low = (end_low.tv_sec - start_low.tv_sec) * 1000000 + (end_low.tv_usec - start_low.tv_usec);
	time_high = (end_high.tv_sec - start_high.tv_sec) * 1000000 + (end_high.tv_usec - start_high.tv_usec);

	LED_INFO("[ta] LOW BIT: time_low(%lu) / time_high(%lu)\n", time_low, time_high);
	if (time_low <= (time_high*2)) 
		LED_ERROR("[ta] Low Bit: high pulse too long\n");
#endif
}

void ktd2692_set_high_bit(void)
{
#ifdef DEBUG_LED_TIME
	struct timeval start_low, end_low, start_high, end_high;
	unsigned long time_low, time_high;
#endif

#ifdef DEBUG_LED_TIME
	do_gettimeofday(&start_low);
#endif

	if (gpio_is_valid(global_ktd2692data->flash_control))
		__gpio_set_value(global_ktd2692data->flash_control, 0);
	udelay(T_L_HB);

#ifdef DEBUG_LED_TIME
	do_gettimeofday(&end_low);

	do_gettimeofday(&start_high);
#endif

	if (gpio_is_valid(global_ktd2692data->flash_control))
		__gpio_set_value(global_ktd2692data->flash_control, 1);
	udelay(T_H_HB);

#ifdef DEBUG_LED_TIME
	do_gettimeofday(&end_high);

	time_low = (end_low.tv_sec - start_low.tv_sec) * 1000000 + (end_low.tv_usec - start_low.tv_usec);
	time_high = (end_high.tv_sec - start_high.tv_sec) * 1000000 + (end_high.tv_usec - start_high.tv_usec);

	LED_INFO("[ta] HIGHT BIT: time_low(%lu) / time_high(%lu)\n", time_low, time_high);
	if ((time_low*2) >= time_high) 
		LED_ERROR("[ta] HIGH BIT: low pulse too long\n");
#endif
}

static int ktd2692_set_bit(unsigned int bit)
{
	if (bit) {
		ktd2692_set_high_bit();
	} else {
		ktd2692_set_low_bit();
	}
	return 0;
}

static int ktd2692_write_data(unsigned data)
{
	int err = 0;
	unsigned int bit = 0;

	/* Data Start Condition */
	if (gpio_is_valid(global_ktd2692data->flash_control))
		__gpio_set_value(global_ktd2692data->flash_control, 1);
	udelay(T_SOD_KT);

	/* BIT 7*/
	bit = ((data>> 7) & 0x01);
	ktd2692_set_bit(bit);

	/* BIT 6 */
	bit = ((data>> 6) & 0x01);
	ktd2692_set_bit(bit);

	/* BIT 5*/
	bit = ((data>> 5) & 0x01);
	ktd2692_set_bit(bit);

	/* BIT 4 */
	bit = ((data>> 4) & 0x01);
	ktd2692_set_bit(bit);

	/* BIT 3*/
	bit = ((data>> 3) & 0x01);
	ktd2692_set_bit(bit);

	/* BIT 2 */
	bit = ((data>> 2) & 0x01);
	ktd2692_set_bit(bit);

	/* BIT 1*/
	bit = ((data>> 1) & 0x01);
	ktd2692_set_bit(bit);

	/* BIT 0 */
	bit = ((data>> 0) & 0x01);
	ktd2692_set_bit(bit);

	if (gpio_is_valid(global_ktd2692data->flash_control))
		__gpio_set_value(global_ktd2692data->flash_control, 0);
	udelay(T_EOD_L);

	/* Data End Condition */
	if (gpio_is_valid(global_ktd2692data->flash_control))
		__gpio_set_value(global_ktd2692data->flash_control, 1);
	udelay(T_EOD_H);

	return err;
}

ssize_t ktd2692_store(struct device *dev,struct device_attribute *attr, const char *buf,size_t count)
{
	int value = 0;
	int brightness_value = 0;
	int ret = 0;
	unsigned long flags = 0;
	int torch_intensity = -1;
	sysfs_flash_op_kt = 0;

	if ((buf == NULL) || kstrtouint(buf, 10, &value)) {
		LED_INFO("sysfs value :(%d)\n", value);
		return -1;
	}

	LED_INFO("sysfs value :(%d)\n", value);
	
	if (global_ktd2692data == NULL) {
		LED_ERROR("KTD2692(%s) global_ktd2692data is not initialized.\n", __func__);
		return -EFAULT;
	}

	LED_INFO("KTD2692-STORE BEFORE:(%d) (%d) (%d)\n", value,buf,global_ktd2692data->sysfs_input_data);
	global_ktd2692data->sysfs_input_data = value;
	LED_INFO("KTD2692-STORE AFTER:(%d) (%d)\n", value,global_ktd2692data->sysfs_input_data);

	if (gpio_is_valid(global_ktd2692data->flash_control)) {
		ret = gpio_request(global_ktd2692data->flash_control, "ktd2692_led_control");
		if (ret) {
			LED_ERROR("Failed to request ktd2692_led_mode_ctrl\n");
			return -1;
		}
	}
	if (gpio_is_valid(global_ktd2692data->flash_en)) {
		ret = gpio_request(global_ktd2692data->flash_en, "ktd2692_led_en");
		if (ret) {
			gpio_free(global_ktd2692data->flash_control);
			LED_ERROR("Failed to request ktd2692_led_en\n");
		}
	}

	if (value <= 0) {
		sysfs_flash_op_kt = false;

		LED_INFO("KTD2692-TORCH OFF. : E(%d)\n", value);

		global_ktd2692data->mode_status = KTD2692_DISABLES_TORCH_FLASH_MODE;
		spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
		ktd2692_write_data(global_ktd2692data->mode_status|
							KTD2692_ADDR_VIDEO_FLASHMODE_CONTROL);
		spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);

		ktd2692_setGpio(0);
		global_ktd2692data->is_torch_enable = false;
		LED_INFO("KTD2692-TORCH OFF. : X(%d)\n", value);
	} else {
		sysfs_flash_op_kt = true;

		global_ktd2692data->mode_status = KTD2692_ENABLE_TORCH_MODE;
		global_ktd2692data->is_torch_enable = true;
		spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
		ktd2692_write_data(global_ktd2692data->LVP_Voltage|
							KTD2692_ADDR_LVP_SETTING);
		LED_INFO("KTD2692-TORCH ON VALUE. : X (%d) (%d)\n", value,global_ktd2692data->sysfs_input_data);
#if 0	/* use the internel defualt setting */
		ktd2692_write_data(global_ktd2692data->flash_timeout|
							KTD2692_ADDR_FLASH_TIMEOUT_SETTING);
		LED_INFO("KTD2692_ADDR_FLASH_TIMEOUT_SETTING");
#endif
		if (value == 100 || value == 1) {/* value 1 is getting used for turning on torch using side key */
			LED_INFO("KTD2692-TORCH ON : E(%d), current(%d)\n", value, global_ktd2692data->factory_torch_current_value);
			ktd2692_write_data(global_ktd2692data->factory_torch_current_value|
								KTD2692_ADDR_VIDEO_CURRENT_SETTING);
			LED_INFO("KTD2692-TORCH ON : X(%d), current(%d)\n", value, global_ktd2692data->factory_torch_current_value);
		} else if (1001 <= value && value <= 1010) {
			LED_INFO("KTD2692-TORCH ON. : E(%d)\n", value);
			brightness_value = value - 1001;
			if (global_ktd2692data->torch_table[brightness_value] != 0) {
				torch_intensity = KTD2692_CAL_VIDEO_CURRENT(KTD2692_TORCH_STEP_LEVEL_CURRENT(global_ktd2692data->torch_table[brightness_value], global_ktd2692data->max_current),
					global_ktd2692data->max_current);
			}
			if (torch_intensity < 0) {
				LED_INFO("KTD2692-force to set as default : %d\n", global_ktd2692data->factory_torch_current_value);
				torch_intensity = global_ktd2692data->factory_torch_current_value;
			}
			ktd2692_write_data(torch_intensity|
								KTD2692_ADDR_VIDEO_CURRENT_SETTING);
			LED_INFO("KTD2692-TORCH ON. : X(%d)\n", value);
		} else {
			LED_INFO(" KTD2692-FLASH ON : E(%d), current(%d)\n", value, global_ktd2692data->flash_current_value);
			global_ktd2692data->mode_status = KTD2692_ENABLE_FLASH_MODE;
			/*ktd2692_write_data(global_ktd2692data->flash_current_value|
								KTD2692_ADDR_FLASH_CURRENT_SETTING);*/
			if (gpio_is_valid(global_ktd2692data->flash_en))
				__gpio_set_value(global_ktd2692data->flash_en, 1);
			
			LED_INFO(" KTD2692-FLASH ON. : X(%d), current(%d)\n", value, global_ktd2692data->flash_current_value);
		}
		ktd2692_write_data(global_ktd2692data->mode_status|
							KTD2692_ADDR_VIDEO_FLASHMODE_CONTROL);
		spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);
	}

	if (gpio_is_valid(global_ktd2692data->flash_control))
		gpio_free(global_ktd2692data->flash_control);
	if (gpio_is_valid(global_ktd2692data->flash_en))
		gpio_free(global_ktd2692data->flash_en);
	return count;
}
EXPORT_SYMBOL(ktd2692_store);

int32_t ktd2692_led_mode_ctrl(int state, u32 intensity)
{
	int ret = 0;
	unsigned long flags = 0;

	if (global_ktd2692data == NULL) {
		LED_ERROR("KTD2692(%s) global_ktd2692data is not initialized.\n", __func__);
		return -EFAULT;
	}

	if (sysfs_flash_op_kt) {
		pr_warn("%s : The camera led control is not allowed"
			"because sysfs led control already used it\n", __FUNCTION__);
		return 0; //no error
	}

	if (gpio_is_valid(global_ktd2692data->flash_control)) {
		ret = gpio_request(global_ktd2692data->flash_control, "ktd2692_led_control");
		if (ret) {
			LED_ERROR("Failed to request ktd2692_led_mode_ctrl\n");
			return -1;
		}
	}

	switch(state) {
		case 0:
			/* FlashLight Mode OFF */
			if (gpio_is_valid(global_ktd2692data->flash_en)) {
				ret = gpio_request(global_ktd2692data->flash_en, "ktd2692_led_en");
				if (ret) {
					gpio_free(global_ktd2692data->flash_control);
					LED_ERROR("Failed to request ktd2692_led_en\n");
				}
			}
			LED_INFO("KTD2692-FLASH OFF E(%d)\n", state);
			global_ktd2692data->mode_status = KTD2692_DISABLES_TORCH_FLASH_MODE;
			spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
			ktd2692_write_data(global_ktd2692data->mode_status|
								KTD2692_ADDR_VIDEO_FLASHMODE_CONTROL);
			spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);

			ktd2692_setGpio(0);
			global_ktd2692data->is_torch_enable = false;
			if (gpio_is_valid(global_ktd2692data->flash_en))
				gpio_free(global_ktd2692data->flash_en);
			LED_INFO("KTD2692-FLASH OFF X(%d)\n", state);
			break;
		case 1:
			/* FlashLight Mode TORCH */
			/*if (gpio_is_valid(global_ktd2692data->flash_en)) 
				ret = gpio_request(global_ktd2692data->flash_en, "ktd2692_led_en");

				if (ret) {
					gpio_free(global_ktd2692data->flash_control);
					LED_ERROR("Failed to request ktd2692_led_en\n");
				}
			
			if (global_ktd2692data->is_torch_enable == true) {
				LED_INFO("KTD2692-TORCH is already ON\n");
				return 0;
			}*/
			
			global_ktd2692data->mode_status = KTD2692_ENABLE_TORCH_MODE;
			LED_INFO("KTD2692 global video current [%d] [%d] \n", state, global_ktd2692data->video_current_value );
			spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
			ktd2692_write_data(global_ktd2692data->LVP_Voltage|
								KTD2692_ADDR_LVP_SETTING);
			if (intensity > 0) {
				ktd2692_write_data((uint8_t)(KTD2692_VIDEO_CURRENT(intensity, KTD2692_MAX_CURRENT)) |
									KTD2692_ADDR_VIDEO_CURRENT_SETTING);
				LED_INFO("[%s] : Intensity(%u) Current-Index (%d)\n", __func__, intensity,
						 KTD2692_VIDEO_CURRENT(intensity, KTD2692_MAX_CURRENT));
			} else {
				ktd2692_write_data(global_ktd2692data->video_current_value|
									KTD2692_ADDR_VIDEO_CURRENT_SETTING);
				LED_INFO("[%s] : Intensity(%u) Current-Index (%d)\n", __func__, intensity,
						 KTD2692_VIDEO_CURRENT(intensity, KTD2692_MAX_CURRENT))	;				
			}
			
			ktd2692_write_data(global_ktd2692data->mode_status|
								KTD2692_ADDR_VIDEO_FLASHMODE_CONTROL);
										
			/*if (gpio_is_valid(global_ktd2692data->flash_en))
				gpio_free(global_ktd2692data->flash_en);*/
			spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);
			LED_INFO("KTD2692-TORCH ON X(%d)\n", state);
			break;
		case 2:
			/* FlashLight Mode Flash */
			/*if (gpio_is_valid(global_ktd2692data->flash_en)) {
				ret = gpio_request(global_ktd2692data->flash_en, "ktd2692_led_en");
				LED_INFO("KTD2692-FLASH gpio enabled  (%d)\n",ret );
				if (ret) {
					gpio_free(global_ktd2692data->flash_control);
					LED_ERROR("Failed to request ktd2692_led_en\n");
				}
			}*/
			LED_INFO("KTD2692-FLASH ON E(%d)\n", state);
			global_ktd2692data->mode_status = KTD2692_ENABLE_FLASH_MODE;
			spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
			ktd2692_write_data(global_ktd2692data->LVP_Voltage|
								KTD2692_ADDR_LVP_SETTING);
			/*if (intensity > 0) {
				ktd2692_write_data((uint8_t)(KTD2692_FLASH_CURRENT(intensity, KTD2692_MAX_CURRENT))|
									KTD2692_ADDR_FLASH_CURRENT_SETTING);
				LED_INFO("[%s] : Intensity(%u) Current-Index (%d)\n", __func__, intensity,
						 KTD2692_FLASH_CURRENT(intensity, KTD2692_MAX_CURRENT));
			} else {
				ktd2692_write_data(global_ktd2692data->flash_current_value|
									KTD2692_ADDR_FLASH_CURRENT_SETTING);
			}*/

			ktd2692_write_data(global_ktd2692data->mode_status|
								KTD2692_ADDR_VIDEO_FLASHMODE_CONTROL);
			if (gpio_is_valid(global_ktd2692data->flash_en))
				__gpio_set_value(global_ktd2692data->flash_en, 1);
			/*if (gpio_is_valid(global_ktd2692data->flash_en))
				gpio_free(global_ktd2692data->flash_en);*/
			spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);
			LED_INFO("KTD2692-FLASH ON X(%d)\n", state);
			break;
		case 3:
			/* FlashLight Mode Flicker */
			LED_INFO("KTD2692-FLASH ON E(%d)\n", state);
			/*global_ktd2692data->mode_status = KTD2692_ENABLE_FLASH_MODE;*/
			spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
			ktd2692_write_data(global_ktd2692data->LVP_Voltage|
								KTD2692_ADDR_LVP_SETTING);
			if (intensity > 0) {
				ktd2692_write_data((uint8_t)(KTD2692_FLASH_CURRENT(intensity, KTD2692_MAX_CURRENT))|
									KTD2692_ADDR_FLASH_CURRENT_SETTING);
				LED_INFO("[%s] : Intensity(%u) Current-Index (%d)\n", __func__, intensity,
						 KTD2692_FLASH_CURRENT(intensity, KTD2692_MAX_CURRENT));
			} else {
				ktd2692_setGpio(0);
			}

			/*ktd2692_write_data(global_ktd2692data->mode_status|
								KTD2692_ADDR_VIDEO_FLASHMODE_CONTROL);*/
			spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);
			LED_INFO("KTD2692-FLASH ON X(%d)\n", state);
			break;
		default:
			/* FlashLight Mode OFF */
			if (gpio_is_valid(global_ktd2692data->flash_en)) {
				ret = gpio_request(global_ktd2692data->flash_en, "ktd2692_led_en");
				if (ret) {
					gpio_free(global_ktd2692data->flash_control);
					LED_ERROR("Failed to request ktd2692_led_en\n");
				}
			}
			LED_INFO("KTD2692-FLASH OFF E(%d)\n", state);
			global_ktd2692data->mode_status = KTD2692_DISABLES_TORCH_FLASH_MODE;
			spin_lock_irqsave(&global_ktd2692data->int_lock, flags);
			ktd2692_write_data(global_ktd2692data->mode_status|
								KTD2692_ADDR_VIDEO_FLASHMODE_CONTROL);
			spin_unlock_irqrestore(&global_ktd2692data->int_lock, flags);

			ktd2692_setGpio(0);
			if (gpio_is_valid(global_ktd2692data->flash_en))
				gpio_free(global_ktd2692data->flash_en);
			LED_INFO("KTD2692-FLASH OFF X(%d)\n", state);
			break;
	}

	if (gpio_is_valid(global_ktd2692data->flash_control)){
		LED_INFO("KTD2692 free flash_ctrl");
		gpio_free(global_ktd2692data->flash_control);
	}
	return ret;
}
EXPORT_SYMBOL(ktd2692_led_mode_ctrl);

ssize_t ktd2692_show (struct device *dev,struct device_attribute *attr, char *buf)
{
	LED_INFO("KTD2692-SHOW:(%d) (%d)\n", buf,global_ktd2692data->sysfs_input_data);
	return sprintf(buf, "%d\n", global_ktd2692data->sysfs_input_data);
}
EXPORT_SYMBOL(ktd2692_show);

static DEVICE_ATTR(rear_flash, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
	ktd2692_show, ktd2692_store);
	
static DEVICE_ATTR(rear_torch_flash, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
	ktd2692_show, ktd2692_store);

static int ktd2692_parse_dt(struct device *dev,
                                struct ktd2692_platform_data *pdata)
{
	struct device_node *dnode = dev->of_node;
	u32 buffer = 0;
	int ret = 0;
	u32 torch_table_enable = 0;

	/* Defulat Value */
	pdata->LVP_Voltage = KTD2692_DISABLE_LVP;
	pdata->flash_timeout = KTD2692_TIMER_1049ms;	/* default */
	pdata->min_current_value = KTD2692_MIN_CURRENT_240mA;
	pdata->flash_current_value = KTD2692_FLASH_CURRENT(KTD2692_FLASH_DEFAULT_CURRENT, KTD2692_MAX_CURRENT);
	pdata->video_current_value = KTD2692_VIDEO_CURRENT(KTD2692_VIDEO_DEFAULT_CURRENT, KTD2692_MAX_CURRENT);
	pdata->factory_torch_current_value = KTD2692_VIDEO_CURRENT(KTD2692_FACTORY_TORCH_CURRENT, KTD2692_MAX_CURRENT);
	pdata->mode_status = KTD2692_DISABLES_TORCH_FLASH_MODE;

	/* get gpio */
	pdata->flash_control = of_get_named_gpio(dnode, "flash_control", 0);
	if (!gpio_is_valid(pdata->flash_control)) {
		dev_err(dev, "failed to get flash_control\n");
		return -1;
	}

	/* get gpio */
	pdata->flash_en = of_get_named_gpio(dnode, "flash_en", 0);
	if (!gpio_is_valid(pdata->flash_en)) {
		dev_err(dev, "failed to get flash_enable\n");
	}

	/* get max current value */
	if (of_property_read_u32(dnode, "max_current", &buffer) == 0) {
		dev_info(dev, "max_current = <%d>\n",
			buffer);
		pdata->max_current = buffer;
	}

	/* get flash current value */
	if (of_property_read_u32(dnode, "flash_current", &buffer) == 0) {
		dev_info(dev, "flash_current = <%d><%d>\n",
			buffer, KTD2692_FLASH_CURRENT(buffer, pdata->max_current));
		pdata->flash_current_value = KTD2692_FLASH_CURRENT(buffer, pdata->max_current);
	}

	/* get vdieo current value */
	if (of_property_read_u32(dnode, "video_current", &buffer) == 0) {
		dev_info(dev, "video_current = <%d><%d>\n",
			buffer, KTD2692_VIDEO_CURRENT(buffer, pdata->max_current));
		pdata->video_current_value = KTD2692_VIDEO_CURRENT(buffer, pdata->max_current);
	}

	/* get factory torch current value */
	if (of_property_read_u32(dnode, "factory_torch_current", &buffer) == 0) {
		dev_info(dev, "factory_torch_current = <%d><%d>\n",
			buffer, KTD2692_VIDEO_CURRENT(buffer, pdata->max_current));
		pdata->factory_torch_current_value = KTD2692_VIDEO_CURRENT(buffer, pdata->max_current);
	}

	ret = of_property_read_u32(dnode, "torch_table_enable", &torch_table_enable);
	if (ret) {
		pr_info("%s failed to get a torch_table_enable\n", __func__);
	}
	if (torch_table_enable == 1) {
		pdata->torch_table_enable = torch_table_enable;
		ret = of_property_read_u32_array(dnode, "torch_table", pdata->torch_table, TORCH_STEP);
	} else {
		pdata->torch_table_enable = 0;
	}

	return ret;
}

static int ktd2692_probe(struct platform_device *pdev)
{
	struct ktd2692_platform_data *pdata;
	int ret = 0;

	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}
		ret = ktd2692_parse_dt(&pdev->dev, pdata);
		if (ret < 0) {
			return -EFAULT;
		}
	} else {
	pdata = pdev->dev.platform_data;
		if (pdata == NULL) {
			return -EFAULT;
		}
	}

	sysfs_flash_op_kt = 0; //default off


	if (camera_class == NULL) {
		camera_class = class_create(THIS_MODULE, "camera");
		if (IS_ERR(camera_class))
			pr_err("%s: failed to create device cam_dev_rear!\n", __func__);
	}
	
	
	if (camera_class != NULL) {
		ktd2692_led_dev = device_create(camera_class, NULL, 0, NULL, "flash");
		if (IS_ERR(ktd2692_led_dev)) {
			pr_err("%s: Failed to create device(flash)!\n", __func__);
			return -ENODEV;
		}

	if (device_create_file(ktd2692_led_dev, &dev_attr_rear_flash) < 0) {
			pr_err("%s: failed to create device file, %s\n",
					__func__, dev_attr_rear_flash.attr.name);
			return -ENOENT;
	}
	

	if (device_create_file(ktd2692_led_dev, &dev_attr_rear_torch_flash) < 0) {
			pr_err("%s: failed to create device file, %s\n",
					__func__, dev_attr_rear_torch_flash.attr.name);
			return -ENOENT;
	}
	}


	global_ktd2692data = pdata;
	global_dev_kt = &pdev->dev;

	LED_INFO("KTD2692_LED Probe\n");

	global_ktd2692data->is_torch_enable = false;

	spin_lock_init(&pdata->int_lock);

	return 0;
}
static int ktd2692_remove(struct platform_device *pdev)
{

	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id ktd2692_dt_ids[] = {
	{ .compatible = "qcom,ktd2692",},
	{},
};
/*MODULE_DEVICE_TABLE(of, ktd2692_dt_ids);*/
#endif

static struct platform_driver ktd2692_driver = {
	.driver = {
		   .name = ktd2692_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = ktd2692_dt_ids,
#endif
		   },
	.probe = ktd2692_probe,
	.remove = ktd2692_remove,
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

static int __init ktd2692_init(void)
{
	int rc = 0;
	
	#if defined(CONFIG_SEC_R8Q_PROJECT)
	unsigned int board_rev = sec_hw_rev();
	if(board_rev >= 8)
	{
		return platform_driver_register(&ktd2692_driver);
	}
	#else
		return platform_driver_register(&ktd2692_driver);
	#endif

	if (rc < 0) {
		pr_info("%s: platform_driver_register Failed: rc = %d",
			__func__, rc);
		return rc;
	}
	return rc;
}

static void __exit ktd2692_exit(void)
{
	platform_driver_unregister(&ktd2692_driver);
}

module_init(ktd2692_init);
module_exit(ktd2692_exit);

MODULE_DESCRIPTION("KTD2692 driver");
MODULE_LICENSE("GPL");
