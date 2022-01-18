/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <linux/skbuff.h>
#include <linux/netlink.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/sec_class.h>
#include "adsp.h"

static u8 msg_size[MSG_SENSOR_MAX] = {		
	MSG_ACCEL_MAX,
	MSG_GYRO_MAX,
	MSG_MAG_MAX,
	MSG_PRESSURE_MAX,
	MSG_LIGHT_MAX,
	MSG_PROX_MAX,
	MSG_TYPE_SIZE_ZERO,
	MSG_MOBEAM_MAX,
#ifdef CONFIG_SUPPORT_DUAL_6AXIS
	MSG_ACCEL_MAX,
	MSG_GYRO_MAX,
#endif
#ifdef CONFIG_SUPPORT_DUAL_OPTIC
	MSG_LIGHT_MAX,
	MSG_PROX_MAX,
#endif
#ifdef CONFIG_SUPPORT_SUB_MOBEAM
	MSG_MOBEAM_MAX,
#endif
	MSG_TYPE_SIZE_ZERO,  /* PHYSICAL_SENSOR_SYSFS */
	MSG_GYRO_TEMP_MAX,
#ifdef CONFIG_SUPPORT_DUAL_6AXIS
	MSG_GYRO_TEMP_MAX,
#endif
	MSG_PRESSURE_TEMP_MAX,
	MSG_TYPE_SIZE_ZERO,
#ifdef CONFIG_SUPPORT_VIRTUAL_OPTIC
	MSG_TYPE_SIZE_ZERO,
#endif
	MSG_TYPE_SIZE_ZERO,
#ifdef CONFIG_SUPPORT_AK0997X
	MSG_DIGITAL_HALL_MAX,
	MSG_DIGITAL_HALL_ANGLE_MAX,
	MSG_DIGITAL_HALL_ANGLE_MAX,
#endif
#ifdef CONFIG_SUPPORT_DUAL_DDI_COPR_FOR_LIGHT_SENSOR
	MSG_DDI_MAX,
#endif
#ifdef CONFIG_SUPPORT_LIGHT_MAIN2_SENSOR
	MSG_LIGHT_MAX,
#endif
	MSG_TYPE_SIZE_ZERO,
	MSG_TYPE_SIZE_ZERO
};

/* The netlink socket */
struct adsp_data *data;

DEFINE_MUTEX(factory_mutex);

#ifdef CONFIG_SUPPORT_BRIGHTNESS_NOTIFY_FOR_LIGHT_SENSOR
struct adsp_data* adsp_get_struct_data(void)
{
	return data;
}
#endif

/* Function used to send message to the user space */
int adsp_unicast(void *param, int param_size, u16 sensor_type,
	u32 portid, u16 msg_type)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	void *msg;
	int ret = -1;
	u16 nlmsg_type = (sensor_type << 8) | msg_type;

	if (data->restrict_mode && msg_type == MSG_TYPE_SET_ACCEL_MOTOR) {
		pr_err("[FACTORY] %s - restrict_mode\n", __func__);
		return ret;
	}

	data->ready_flag[msg_type] &= ~(1 << sensor_type);
	skb = nlmsg_new(param_size, GFP_KERNEL);
	if (!skb) {
		pr_err("[FACTORY] %s - nlmsg_new fail\n", __func__);
		return -ENOMEM;
	}

	nlh = nlmsg_put(skb, portid, 0, nlmsg_type, param_size, 0);
	if (nlh == NULL) {
		pr_err("[FACTORY] %s - nlmsg_put fail\n", __func__);
		nlmsg_free(skb);
		return -EMSGSIZE;
	}
	msg = nlmsg_data(nlh);
	memcpy(msg, param, param_size);
	NETLINK_CB(skb).dst_group = 0;
	ret = nlmsg_unicast(data->adsp_skt, skb, PID);
	if (ret != 0)
		pr_err("[FACTORY] %s - ret = %d\n", __func__, ret);

	return ret;
}

#if defined(CONFIG_SUPPORT_DEVICE_MODE) || defined(CONFIG_VBUS_NOTIFIER)
struct adsp_data* adsp_ssc_core_register(unsigned int type,
	struct device_attribute *attributes[])
{
	int ret = 0;

	data->sensor_attr[type] = attributes;
	ret = sensors_register(&data->sensor_device[type], data,
		data->sensor_attr[type], "ssc_core");

	data->sysfs_created[type] = true;
	pr_info("[FACTORY] %s - type:%u ptr:%pK\n",
		__func__, type, data->sensor_device[type]);

	return data;
}

struct adsp_data* adsp_ssc_core_unregister(unsigned int type)
{
	pr_info("[FACTORY] %s - type:%u ptr:%pK\n",
		__func__, type, data->sensor_device[type]);

	if (data->sysfs_created[type]) {
		sensors_unregister(data->sensor_device[type],
			data->sensor_attr[type]);
		data->sysfs_created[type] = false;
	} else {
		pr_info("[FACTORY] %s: skip type %u\n", __func__, type);
	}
	return data;
}
#endif

int adsp_factory_register(unsigned int type,
	struct device_attribute *attributes[])
{
	int ret = 0;
	char *dev_name;

	switch (type) {
	case MSG_ACCEL:
		dev_name = "accelerometer_sensor";
		break;
	case MSG_GYRO:
		dev_name = "gyro_sensor";
		break;
#ifdef CONFIG_SUPPORT_DUAL_6AXIS
	case MSG_ACCEL_SUB:
		dev_name = "sub_accelerometer_sensor";
		break;
	case MSG_GYRO_SUB:
		dev_name = "sub_gyro_sensor";
		break;
#endif
	case MSG_MAG:
		dev_name = "magnetic_sensor";
		break;
	case MSG_PRESSURE:
		dev_name = "barometer_sensor";
		break;
	case MSG_LIGHT:
		dev_name = "light_sensor";
		break;
	case MSG_PROX:
		dev_name = "proximity_sensor";
		break;
#ifdef CONFIG_SUPPORT_DUAL_OPTIC
	case MSG_LIGHT_SUB:
		dev_name = "sub_light_sensor";
		break;
	case MSG_PROX_SUB:
		dev_name = "sub_proximity_sensor";
		break;
#endif
	case MSG_SSC_CORE:
		dev_name = "ssc_core";
		break;
	case MSG_HH_HOLE:
		dev_name = "hidden_hole";
		break;
#ifdef CONFIG_SUPPORT_HIDDEN_HOLE_SUB
	case MSG_HH_HOLE_SUB:
		dev_name = "hidden_hole_sub";
		break;
#endif
#ifdef CONFIG_SUPPORT_AK0997X
	case MSG_DIGITAL_HALL:
		dev_name = "digital_hall";
		break;
#endif
	default:
		dev_name = "unknown_sensor";
		break;
	}

	data->sensor_attr[type] = attributes;
	ret = sensors_register(&data->sensor_device[type], data,
		data->sensor_attr[type], dev_name);

	data->sysfs_created[type] = true;
	pr_info("[FACTORY] %s - type:%u ptr:%pK\n",
		__func__, type, data->sensor_device[type]);

	return ret;
}

int adsp_factory_unregister(unsigned int type)
{
	pr_info("[FACTORY] %s - type:%u ptr:%pK\n",
		__func__, type, data->sensor_device[type]);

	if (data->sysfs_created[type]) {
		sensors_unregister(data->sensor_device[type],
			data->sensor_attr[type]);
		data->sysfs_created[type] = false;
	} else {
		pr_info("[FACTORY] %s: skip type %u\n", __func__, type);
	}
	return 0;
}

int get_prox_raw_data(int *raw_data, int *offset)
{
	uint8_t cnt = 0;

	mutex_lock(&data->prox_factory_mutex);
	adsp_unicast(NULL, 0, MSG_PROX, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << MSG_PROX) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_PROX);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		mutex_unlock(&data->prox_factory_mutex);
		return -1;
	}

	*raw_data = data->msg_buf[MSG_PROX][0];
	*offset = data->msg_buf[MSG_PROX][1];
	mutex_unlock(&data->prox_factory_mutex);

	return 0;
}

#ifdef CONFIG_SUPPORT_DUAL_OPTIC
int get_sub_prox_raw_data(int *raw_data, int *offset)
{
	uint8_t cnt = 0;

	mutex_lock(&data->prox_factory_mutex);
	adsp_unicast(NULL, 0, MSG_PROX_SUB, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << MSG_PROX_SUB) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_PROX_SUB);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		mutex_unlock(&data->prox_factory_mutex);
		return -1;
	}

	*raw_data = data->msg_buf[MSG_PROX_SUB][0];
	*offset = data->msg_buf[MSG_PROX_SUB][1];
	mutex_unlock(&data->prox_factory_mutex);

	return 0;
}
#endif

int get_accel_raw_data(int32_t *raw_data)
{
	uint8_t cnt = 0;

	adsp_unicast(NULL, 0, MSG_ACCEL, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << MSG_ACCEL) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_ACCEL);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return -1;
	}

	memcpy(raw_data, &data->msg_buf[MSG_ACCEL][0], sizeof(int32_t) * 3);

	return 0;
}

#ifdef CONFIG_SUPPORT_DUAL_6AXIS
int get_sub_accel_raw_data(int32_t *raw_data)
{
	uint8_t cnt = 0;

	adsp_unicast(NULL, 0, MSG_ACCEL_SUB, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << MSG_ACCEL_SUB) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_ACCEL_SUB);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return -1;
	}

	memcpy(raw_data, &data->msg_buf[MSG_ACCEL_SUB][0], sizeof(int32_t) * 3);

	return 0;
}
#endif

#ifdef CONFIG_SEC_FACTORY
int get_mag_raw_data(int32_t *raw_data)
{
	uint8_t cnt = 0;

	adsp_unicast(NULL, 0, MSG_MAG, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << MSG_MAG) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_MAG);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return -1;
	}

	memcpy(raw_data, &data->msg_buf[MSG_MAG][0], sizeof(int32_t) * 3);

	return 0;
}
#endif

#ifdef CONFIG_SUPPORT_MOBEAM
void adsp_mobeam_register(struct device_attribute *attributes[])
{
	int i;

	data->mobeam_device = sec_device_create(0, data, "sec_barcode_emul");

	for (i = 0; attributes[i] != NULL; i++)	{
		if (device_create_file(data->mobeam_device, attributes[i]) < 0)
			pr_err("%s fail to create %d", __func__, i);
	}
}

void adsp_mobeam_unregister(struct device_attribute *attributes[])
{
	int i;

	for (i = 0; attributes[i] != NULL; i++)
		device_remove_file(data->mobeam_device, attributes[i]);
}
#ifdef CONFIG_SUPPORT_SUB_MOBEAM
void adsp_sub_mobeam_register(struct device_attribute *attributes[])
{
	int i;

	data->sub_mobeam_device = sec_device_create(0, data, "sec_sub_barcode_emul");

	for (i = 0; attributes[i] != NULL; i++)	{
		if (device_create_file(data->sub_mobeam_device, attributes[i]) < 0)
			pr_err("%s fail to create %d", __func__, i);
	}
}

void adsp_sub_mobeam_unregister(struct device_attribute *attributes[])
{
	int i;

	for (i = 0; attributes[i] != NULL; i++)
		device_remove_file(data->sub_mobeam_device, attributes[i]);
}
#endif
#endif

#ifdef CONFIG_SUPPORT_AK0997X
int get_hall_angle_data(int32_t *raw_data)
{
	uint8_t cnt = 0;

	mutex_lock(&data->digital_hall_mutex);
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
                return -1;
	}

	pr_info("[FACTORY] %s - st %d/%d, akm %d/%d, lf %d/%d, hall %d/%d/%d(uT)\n",
		__func__, data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][1],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][2],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][3],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][4],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][5],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][6],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][7],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][8]);

	*raw_data = data->msg_buf[MSG_DIGITAL_HALL_ANGLE][2];
	mutex_unlock(&data->digital_hall_mutex);

	return 0;
}
#endif

static int process_received_msg(struct sk_buff *skb, struct nlmsghdr *nlh)
{
	u16 sensor_type = nlh->nlmsg_type >> 8;
	u16 msg_type = nlh->nlmsg_type & 0xff;

	/* check the boundary to prevent memory attack */
	if (msg_type >= MSG_TYPE_MAX || sensor_type >= MSG_SENSOR_MAX ||
		nlh->nlmsg_len - (int32_t)sizeof(struct nlmsghdr) >
	    	sizeof(int32_t) * msg_size[sensor_type]) {
		pr_err("[FACTORY] %s %d, %d, %d\n", __func__, msg_type, sensor_type, nlh->nlmsg_len);
		return 0;
	}

	if (sensor_type == MSG_FACTORY_INIT_CMD) {
		accel_factory_init_work();
#ifdef CONFIG_SUPPORT_DUAL_6AXIS
		sub_accel_factory_init_work();
#endif
#ifdef CONFIG_SUPPORT_HIDDEN_HOLE
		hidden_hole_init_work();
#endif
#ifdef CONFIG_SUPPORT_HIDDEN_HOLE_SUB
		hidden_hole_sub_init_work();
#endif
#ifdef CONFIG_SUPPORT_DEVICE_MODE
		sns_device_mode_init_work();
#endif
#if defined(CONFIG_SUPPORT_DUAL_OPTIC) && defined(CONFIG_SUPPORT_DEVICE_MODE)
		sns_flip_init_work();
#endif
#ifdef CONFIG_SUPPORT_AMS_PROX_CALIBRATION
		prox_cal_init_work(data);
#endif
#ifdef CONFIG_SUPPORT_SSC_AOD_RECT
		light_rect_init_work();
#endif
#ifdef CONFIG_SUPPORT_AMS_LIGHT_CALIBRATION
		light_cal_init_work(data);
#endif
#ifdef CONFIG_SUPPORT_PROX_POWER_ON_CAL
		prox_factory_init_work();
#endif
#ifdef CONFIG_VBUS_NOTIFIER
		sns_vbus_init_work();
#endif
#ifdef CONFIG_SUPPORT_AK0997X
		digital_hall_factory_auto_cal_init_work();
#endif
		return 0;
	}

	memcpy(data->msg_buf[sensor_type],
		(int32_t *)NLMSG_DATA(nlh),
		nlh->nlmsg_len - (int32_t)sizeof(struct nlmsghdr));
#ifdef CONFIG_SUPPORT_LIGHT_DUALIZATION
	if (sensor_type == MSG_LIGHT && msg_type == MSG_TYPE_ST_SHOW_DATA)
	{
		light_set_name_vendor(data->msg_buf[MSG_LIGHT][0]);
	}
#endif
	data->ready_flag[msg_type] |= 1 << sensor_type;

	return 0;
}

static void factory_receive_skb(struct sk_buff *skb)
{
	struct netlink_ext_ack extack = {};
	struct nlmsghdr *nlh;
	int len;
	int err;

	nlh = (struct nlmsghdr *)skb->data;
	len = skb->len;
	while (NLMSG_OK(nlh, len)) {
		err = process_received_msg(skb, nlh);
		/* if err or if this message says it wants a response */
		if (err || (nlh->nlmsg_flags & NLM_F_ACK))
			netlink_ack(skb, nlh, err, &extack);
		nlh = NLMSG_NEXT(nlh, len);
	}
}

/* Receive messages from netlink socket. */
static void factory_test_result_receive(struct sk_buff *skb)
{
	mutex_lock(&factory_mutex);
	factory_receive_skb(skb);
	mutex_unlock(&factory_mutex);
}

struct netlink_kernel_cfg netlink_cfg = {
	.input = factory_test_result_receive,
};

static int __init factory_adsp_init(void)
{
	int i;

	pr_info("[FACTORY] %s\n", __func__);
	data = kzalloc(sizeof(*data), GFP_KERNEL);

	for (i = 0; i < MSG_SENSOR_MAX; i++) {
		if (msg_size[i] > 0)
		  data->msg_buf[i] = kzalloc(sizeof(int32_t) * msg_size[i],
			  GFP_KERNEL);
	}

	data->adsp_skt = netlink_kernel_create(&init_net,
		NETLINK_ADSP_FAC, &netlink_cfg);

	for (i = 0; i < MSG_SENSOR_MAX; i++)
		data->sysfs_created[i] = false;
	for (i = 0; i < MSG_TYPE_MAX; i++)
		data->ready_flag[i] = 0;

	data->restrict_mode = false;
	mutex_init(&data->accel_factory_mutex);
	mutex_init(&data->prox_factory_mutex);
	mutex_init(&data->light_factory_mutex);
	mutex_init(&data->remove_sysfs_mutex);
#ifdef CONFIG_SUPPORT_AK0997X
	mutex_init(&data->digital_hall_mutex);
#endif
#ifdef CONFIG_SUPPORT_AMS_LIGHT_CALIBRATION
	INIT_DELAYED_WORK(&data->light_cal_work, light_cal_read_work_func);
#endif
#if defined(CONFIG_SEC_C1Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT)
	INIT_DELAYED_WORK(&data->light_copr_debug_work,
		light_copr_debug_work_func);
#endif
#ifdef CONFIG_SUPPORT_BRIGHTNESS_NOTIFY_FOR_LIGHT_SENSOR
	INIT_WORK(&data->light_br_work, light_brightness_work_func);
#endif
	pr_info("[FACTORY] %s: Timer Init\n", __func__);
	return 0;
}

static void __exit factory_adsp_exit(void)
{
	int i;

	mutex_destroy(&data->accel_factory_mutex);
	mutex_destroy(&data->prox_factory_mutex);
	mutex_destroy(&data->light_factory_mutex);
	mutex_destroy(&data->remove_sysfs_mutex);
#ifdef CONFIG_SUPPORT_AK0997X
	mutex_destroy(&data->digital_hall_mutex);
#endif
#ifdef CONFIG_SUPPORT_AMS_LIGHT_CALIBRATION
	cancel_delayed_work_sync(&data->light_cal_work);
#endif
#if defined(CONFIG_SEC_C1Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT)
	cancel_delayed_work_sync(&data->light_copr_debug_work);
#endif
#ifdef CONFIG_SUPPORT_BRIGHTNESS_NOTIFY_FOR_LIGHT_SENSOR
	cancel_work_sync(&data->light_br_work);
#endif

	for (i = 0; i < MSG_SENSOR_MAX; i++)
		kfree(data->msg_buf[i]);
	pr_info("[FACTORY] %s\n", __func__);
}

module_init(factory_adsp_init);
module_exit(factory_adsp_exit);
MODULE_DESCRIPTION("Support for factory test sensors (adsp)");
MODULE_LICENSE("GPL");
