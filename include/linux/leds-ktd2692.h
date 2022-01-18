/*
 * leds-ir-ktd2692.h - Flash-led driver for KTD 2692
 *
 * Copyright (C) 2020 Samsung Electronics
 * Sunggeun Yim <sunggeun.yim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LEDS_KTD2692_H__
#define __LEDS_KTD2692_H__

#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>

#define ktd2692_NAME "leds-ktd2692"

#define KTD2692_MAX_CURRENT 1500
#define KTD2692_FLASH_DEFAULT_CURRENT	1200	/* 1.2A */
#define KTD2692_VIDEO_DEFAULT_CURRENT	175	/* 175mA */
#define KTD2692_FACTORY_TORCH_CURRENT 240	/* 240mA */

#define TORCH_STEP 10

#define KTD2692_CAL_FLASH_CURRENT(mA, max) (((((((mA)*16)*10)/(max))+5)/10)-1)
#define KTD2692_CAL_VIDEO_CURRENT(mA, max) ((((((((mA)*16)*3)*10)/(max))+5)/10)-1)
#define KTD2692_FLASH_CURRENT(mA, max) ((KTD2692_CAL_FLASH_CURRENT((int)mA, max)) > 0) ? (KTD2692_CAL_FLASH_CURRENT(mA, max) & 0x1f) : 0
#define KTD2692_VIDEO_CURRENT(mA, max) ((KTD2692_CAL_VIDEO_CURRENT((int)mA, max)) > 0) ? (KTD2692_CAL_VIDEO_CURRENT(mA, max) & 0x1f) : 0

#define KTD2692_TORCH_STEP_LEVEL_CURRENT(n,max) (((((max)/16)/3) + 1)*(n))

#define LED_ERROR(x, ...) printk(KERN_ERR "%s : " x, __func__, ##__VA_ARGS__)
#define LED_INFO(x, ...) printk(KERN_INFO "%s : " x, __func__, ##__VA_ARGS__)
#define LED_CHECK_ERR_GOTO(x, out, fmt, ...) \
	if (unlikely((x) < 0)) { \
		printk(KERN_ERR fmt, ##__VA_ARGS__); \
		goto out; \
	}

#define DT_READ_U32(node, key, value) do {\
			pprop = key; \
			temp = 0; \
			if (of_property_read_u32((node), key, &temp)) \
				pr_warn("%s: no property in the node.\n", pprop);\
			(value) = temp; \
		} while (0)

#define KTD2692_ADDR_LVP_SETTING	0x00
#define KTD2692_ADDR_FLASH_TIMEOUT_SETTING	0x20
#define KTD2692_ADDR_MIN_CURRENT_SETTING	0x40
#define KTD2692_ADDR_VIDEO_CURRENT_SETTING	0x60
#define KTD2692_ADDR_FLASH_CURRENT_SETTING	0x80
#define KTD2692_ADDR_VIDEO_FLASHMODE_CONTROL	0xA0

#define T_L_LB		(80)	/* us */
#define T_H_LB		(5)				/* us*/
#define T_H_HB		(80)	/* us */
#define T_L_HB		(5)				/* us*/
#define T_SOD_KT		(200)			/* us */
#define T_EOD_L		(25)				/* us */
#define T_EOD_H		(400)			/* us */

/* LVP_SETTING */
enum ktd2692_LVPsetting_t {
	KTD2692_DISABLE_LVP = 0x00,
	KTD2692_3_2V,
	KTD2692_3_3V,
	KTD2692_3_4V,
	KTD2692_3_5V,
	KTD2692_3_6V,
	KTD2692_3_7V,
	KTD2692_3_8V,
};
/* FLASH_TIMEOUT_SETTING */
enum ktd2692_timer_t {
	KTD2692_DISABLE_TIMER = 0x00,
	KTD2692_TIMER_262ms,
	KTD2692_TIMER_524ms,
	KTD2692_TIMER_786ms,
	KTD2692_TIMER_1049ms,	/* default */
	KTD2692_TIMER_1311ms,
	KTD2692_TIMER_1573ms,
	KTD2692_TIMER_1835ms,
};
/* MIN. CURRENT SETTING FOR TIMER OPERATING */
enum ktd2692_min_current_t {
	KTD2692_MIN_CURRENT_90mA = 0x00,
	KTD2692_MIN_CURRENT_120mA,
	KTD2692_MIN_CURRENT_150mA,
	KTD2692_MIN_CURRENT_180mA,
	KTD2692_MIN_CURRENT_210mA,
	KTD2692_MIN_CURRENT_240mA,
	KTD2692_MIN_CURRENT_270mA,
	KTD2692_MIN_CURRENT_300mA,
};
/* VIDEO CURRENT SETTING */
enum ktd2692_video_current_t {
	KTD2692_VIDEO_CURRENT1 = 0x00,
	KTD2692_VIDEO_CURRENT2,
	KTD2692_VIDEO_CURRENT3,
	KTD2692_VIDEO_CURRENT4,
	KTD2692_VIDEO_CURRENT5,
	KTD2692_VIDEO_CURRENT6,
	KTD2692_VIDEO_CURRENT7,
	KTD2692_VIDEO_CURRENT8,
	KTD2692_VIDEO_CURRENT9,
	KTD2692_VIDEO_CURRENT10,
	KTD2692_VIDEO_CURRENT11,
	KTD2692_VIDEO_CURRENT12,
	KTD2692_VIDEO_CURRENT13,
	KTD2692_VIDEO_CURRENT14,
	KTD2692_VIDEO_CURRENT15,
	KTD2692_VIDEO_CURRENT16,
};
/* FLASH CURRENT SETTING */
enum ktd2692_flash_current_t {
	KTD2692_FLASH_CURRENT1 = 0x00,
	KTD2692_FLASH_CURRENT2,
	KTD2692_FLASH_CURRENT3,
	KTD2692_FLASH_CURRENT4,
	KTD2692_FLASH_CURRENT5,
	KTD2692_FLASH_CURRENT6,
	KTD2692_FLASH_CURRENT7,
	KTD2692_FLASH_CURRENT8,
	KTD2692_FLASH_CURRENT9,
	KTD2692_FLASH_CURRENT10,
	KTD2692_FLASH_CURRENT11,
	KTD2692_FLASH_CURRENT12,
	KTD2692_FLASH_CURRENT13,
	KTD2692_FLASH_CURRENT14,
	KTD2692_FLASH_CURRENT15,
	KTD2692_FLASH_CURRENT16,
};
/* VIDEO/FLASH MODE CONTROL */
enum ktd2692_mode_control_t {
	KTD2692_DISABLES_TORCH_FLASH_MODE = 0x00,
	KTD2692_ENABLE_TORCH_MODE,
	KTD2692_ENABLE_FLASH_MODE,
	KTD2692_FLICKER_FLASH_MODE,
};
/* TORCH CURRENT INDEX */
enum ktd2692_torch_current_index_t {
	KTD2692_TORCH_CURRENT1,
	KTD2692_TORCH_CURRENT2,
	KTD2692_TORCH_CURRENT3,
	KTD2692_TORCH_CURRENT4,
	KTD2692_TORCH_CURRENT5,
	KTD2692_TORCH_CURRENT6,
	KTD2692_TORCH_CURRENT7,
	KTD2692_TORCH_CURRENT8,
	KTD2692_TORCH_CURRENT9,
	KTD2692_TORCH_CURRENT10,
	KTD2692_TORCH_CURRENT_MAX,
};

struct ktd2692_platform_data {
	spinlock_t int_lock;
	int sysfs_input_data;
	int flash_control;
	int flash_en;
	int torch_table_enable;
	int torch_table[TORCH_STEP];
	bool is_torch_enable;
	struct workqueue_struct *wqueue;
	struct work_struct	ktd269_work;
	u32 max_current;
	enum ktd2692_LVPsetting_t LVP_Voltage;
	enum ktd2692_timer_t flash_timeout;
	enum ktd2692_min_current_t min_current_value;
	enum ktd2692_flash_current_t flash_current_value;
	enum ktd2692_video_current_t video_current_value;
	enum ktd2692_video_current_t factory_torch_current_value;
	enum ktd2692_mode_control_t mode_status;
};

struct ktd2692_torch_current_level {
	u32 index;
	u32 intensity;
};

extern ssize_t ktd2692_store(struct device *dev,struct device_attribute *attr, const char *buf,size_t count);
extern ssize_t ktd2692_show(struct device *dev,struct device_attribute *attr, char *buf);


//extern ssize_t ktd2692_store(const char *buf);
//extern ssize_t ktd2692_show(char *buf);

extern int32_t ktd2692_led_mode_ctrl(int state, u32 intensity);
#endif
