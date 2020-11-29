/*
 * @file hdm.c
 * @brief HDM Support
 * Copyright (c) 2019, Samsung Electronics Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#if defined(CONFIG_ARCH_QCOM)
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/resource.h>
#endif
#include <linux/hdm.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/uh.h>
#include <linux/sec_class.h>
#if defined(CONFIG_ARCH_QCOM)
#include <linux/sched/signal.h>
#include <soc/qcom/secure_buffer.h>
#include <soc/qcom/qtee_shmbridge.h>
#endif

#if defined(CONFIG_ARCH_EXYNOS)
#include <linux/sec_ext.h>
#include "tz_hdm.h"
#endif

#include "hdm_log.h"

//extern ssize_t bbd_control(const char *buf, ssize_t len);
//extern void hdm_wifi_if_close(void);

int hdm_log_level = HDM_LOG_LEVEL;
void hdm_printk(int level, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	if (hdm_log_level < level)
		return;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	printk("%s %pV", TAG, &vaf);

	va_end(args);
}

#if defined(CONFIG_ARCH_EXYNOS)
static TEEC_UUID uuid = {
	.timeLow = 0x0,
	.timeMid = 0x0,
	.timeHiAndVersion = 0x0,
	.clockSeqAndNode = {0x0, 0x0, 0x54, 0x41, 0x2d, 0x48, 0x44, 0x4d},
};

TEEC_Result tz_call(uint8_t* data, size_t len, unsigned long* mode)
{
	TEEC_Context context;
	TEEC_Session session;
	TEEC_Operation operation;
	TEEC_Result result = TEEC_SUCCESS;
	tciMessage_t* sendmsg = NULL;
	tciMessage_t* rspmsg = NULL;

	hdm_info("%s begin\n", __func__);

	sendmsg = kmalloc(sizeof(tciMessage_t), GFP_KERNEL);
	rspmsg = kmalloc(sizeof(tciMessage_t), GFP_KERNEL);

	sendmsg->jws_message.len = len;
	memcpy(sendmsg->jws_message.data, data, len);

	hdm_info("%s sendmsg->jws_message.len = %d\n", __func__,
	       sendmsg->jws_message.len);

	operation.params[0].tmpref.buffer = sendmsg;
	operation.params[0].tmpref.size = sizeof(tciMessage_t);

	operation.params[1].tmpref.buffer = rspmsg;
	operation.params[1].tmpref.size = sizeof(tciMessage_t);

	result = TEEC_InitializeContext(NULL, &context);
	if (result != TEEC_SUCCESS)
		goto out;
	hdm_info("%s TEEC_InitializeContext OK\n", __func__);

	result = TEEC_OpenSession(&context, &session, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, NULL);
	if (result != TEEC_SUCCESS)
		goto finalize_context;
	hdm_info("%s TEEC_OpenSession OK\n", __func__);

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT,
						TEEC_MEMREF_TEMP_OUTPUT,
						TEEC_NONE, TEEC_NONE);

	result = TEEC_InvokeCommand(&session, CMD_STORE_APPLY_POLICY,
				    &operation, NULL);
	if (result != TEEC_SUCCESS)
		goto close_session;
	hdm_info("%s TEEC_InvokeCommand OK\n", __func__);

	//memcpy(mode, rspmsg->data, 1);
	*mode = rspmsg->jws_message.policy_value;
	hdm_info("%s rspmsg->jws_message.len = %d\n", __func__,
		 rspmsg->jws_message.len);
	hdm_info("%s rspmsg->jws_message.policy_value = %d\n", __func__,
	       rspmsg->jws_message.policy_value);
	hdm_info("%s mode = %d\n", __func__, *mode);

close_session:
	TEEC_CloseSession(&session);
finalize_context:
	TEEC_FinalizeContext(&context);
out:
	hdm_info("%s end, result=0x%x\n", __func__, result);

	return result;
}
#endif

/* this function handles shutting off of subsystem specified
 * by cmd_id
 */

int hdm_prot[HDM_DEVICE_MAX] = {0,};
int hdm_cmd[HDM_DEVICE_MAX] = {0,};

static int hdm_ctrl_camera_prev(int cmd)
{
	if(cmd) {
		//block
		hdm_prot[HDM_DEVICE_CAMERA] = 1;
	}
	else {
		//unblock
	}
	return 0;
}

static int hdm_ctrl_mmc_prev(int cmd)
{
	if(cmd) {
		//block
		hdm_prot[HDM_DEVICE_MMC] = 1;
	}
	else {
		//unblock
	}
	return 0;
}

static int hdm_ctrl_wifi_prev(int cmd)
{
	if(cmd) {
		//block
		hdm_prot[HDM_DEVICE_WIFI] = 1;
		//hdm_wifi_if_close();
	}
	else {
		//unblock
	}
	return 0;
}


static int hdm_ctrl_bluetooth_prev(int cmd)
{
	if(cmd) {
		//block
		hdm_prot[HDM_DEVICE_BLUETOOTH] = 1;
	}
	else {
		//unblock
	}
	return 0;
}


static int hdm_ctrl_usb_prev(int cmd)
{
	if(cmd) {
		//block
		hdm_prot[HDM_DEVICE_USB] = 1;
	}
	else {
		//unblock
	}
	return 0;
}



static int hdm_ctrl_camera_next(int cmd)
{
	if(cmd) {
		//block
	}
	else {
		//unblock
		hdm_prot[HDM_DEVICE_CAMERA] = 0;
	}
	return 0;
}

static int hdm_ctrl_mmc_next(int cmd)
{
	if(cmd) {
		//block
	}
	else {
		//unblock
		hdm_prot[HDM_DEVICE_MMC] = 0;
	}
	return 0;
}

static int hdm_ctrl_wifi_next(int cmd)
{
	if(cmd) {
		//block
	}
	else {
		//unblock
		hdm_prot[HDM_DEVICE_WIFI] = 0;
	}
	return 0;
}


static int hdm_ctrl_bluetooth_next(int cmd)
{
	if(cmd) {
		//block
	}
	else {
		//unblock
		hdm_prot[HDM_DEVICE_BLUETOOTH] = 0;
	}
	return 0;
}


static int hdm_ctrl_usb_next(int cmd)
{
	if(cmd) {
		//block
	}
	else {
		//unblock
		hdm_prot[HDM_DEVICE_USB] = 0;
	}
	return 0;
}

const hdm_device_ctrl_t hdm_device_prev_ctrl_table[] = {
	[HDM_DEVICE_CAMERA] = { hdm_ctrl_camera_prev },
	[HDM_DEVICE_MMC] = { hdm_ctrl_mmc_prev },
	[HDM_DEVICE_USB] = { hdm_ctrl_usb_prev },
	[HDM_DEVICE_WIFI] = { hdm_ctrl_wifi_prev },
	[HDM_DEVICE_BLUETOOTH] = { hdm_ctrl_bluetooth_prev },
};

const hdm_device_ctrl_t hdm_device_next_ctrl_table[] = {
	[HDM_DEVICE_CAMERA] = { hdm_ctrl_camera_next },
	[HDM_DEVICE_MMC] = { hdm_ctrl_mmc_next },
	[HDM_DEVICE_USB] = { hdm_ctrl_usb_next },
	[HDM_DEVICE_WIFI] = { hdm_ctrl_wifi_next },
	[HDM_DEVICE_BLUETOOTH] = { hdm_ctrl_bluetooth_next },
};

int hdm_pre_setting(int mode)
{
	int dev_num;
	hdm_device_control_handler_t dc_prev_hdlr = NULL;

	for(dev_num = 0; dev_num < HDM_DEVICE_MAX; ++dev_num) {
		hdm_cmd[dev_num] = mode & (1 << dev_num)? HDM_PROTECT : HDM_ALLOW;
		dc_prev_hdlr = hdm_device_prev_ctrl_table[dev_num].fn;
		if(dc_prev_hdlr == NULL) {
			return -1;
		}
		dc_prev_hdlr(hdm_cmd[dev_num]);
	}
	return 0;
}

int hdm_post_setting(int mode)
{
	int dev_num;
	hdm_device_control_handler_t dc_next_hdlr = NULL;

	for(dev_num = 0; dev_num < HDM_DEVICE_MAX; ++dev_num) {
		hdm_cmd[dev_num] = mode & (1 << dev_num)? HDM_PROTECT : HDM_ALLOW;
		dc_next_hdlr = hdm_device_next_ctrl_table[dev_num].fn;
		if(dc_next_hdlr == NULL) {
			return -1;
		}
		dc_next_hdlr(hdm_cmd[dev_num]);
	}

	return 0;
}

static ssize_t show_hdm_policy(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	int idx = 0;
	int i = 0;
	for (i = HDM_DEVICE_MAX-1; i >= 0; i--) {
		idx += sprintf(buf+idx, "%d ", hdm_cmd[i]);

	}
	hdm_info("%s %s\n", __func__, buf);
	return strlen(buf);
}

static ssize_t store_hdm_policy(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long mode = HDM_CMD_MAX;
	int c, p; 

	if (count == 0) {
		hdm_err("%s count = 0\n", __func__);
		goto error;
	}

	if (kstrtoul(buf, 0, &mode)) {
		goto error;
	};

	if (mode > HDM_CMD_MAX) {
		hdm_err("%s command size max fail. %d\n", __func__, mode);
		goto error;
	}
	hdm_info("%s: command id: 0x%x\n", __func__, (int)mode);

	c = (int)(mode & HDM_C_BITMASK);
	p = (int)(mode & HDM_P_BITMASK);

	hdm_info("%s m:0x%x c:0x%x p:0x%x\n", __func__, (int)mode, c ,p);	
	switch(c) {
	case HDM_KERNEL_PRE:
		hdm_pre_setting(p);
		break;
	case HDM_KERNEL_POST:
		hdm_post_setting(p);
		break;
#if defined(CONFIG_ARCH_QCOM)
#if defined(CONFIG_UH)
	case HDM_HYP_CALL:
		uh_call(UH_APP_HDM, 9, 0, p, 0, 0);

		break;
#endif
#endif
	default:
		goto error;
	}
error:
	return count;
}
static DEVICE_ATTR(hdm_policy, 0660, show_hdm_policy, store_hdm_policy);

#if defined(CONFIG_HDM_DEBUG)
static ssize_t show_hdm_test(struct device *dev, struct device_attribute *attr, char *buf)
{
	int idx = 0;
	int i = 0;
	for (i = HDM_DEVICE_MAX-1; i >= 0; i--) {
		idx += sprintf(buf+idx, "%d ", hdm_cmd[i]);

	}
	hdm_info("%s %s\n", __func__, buf);
	return strlen(buf);
}

static ssize_t store_hdm_test(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	unsigned long mode = 0xFFFFF;
	size_t len = count;
#if defined(CONFIG_UH)
	unsigned long command = 0;
	int i = 0;
#endif

	if (count == 0) {
		hdm_err("%s count = 0\n", __func__);
		goto error;
	}

	/* In case the buffer is a string (the case when using echo), we should remove the string terminator */
	if (buf[count] == '\0') {
		len = count - 1;
	}

	if (kstrtoul(buf, 0, &mode)) {
		goto error;
	};

	if (mode > HDM_CMD_MAX) {
		hdm_err("%s command size max fail. %d\n", __func__, mode);
		goto error;
	}
	hdm_err("%s command id: 0x%x\n", __func__, (int)mode);

	if (mode & HDM_KERNEL_PRE) {
		hdm_pre_setting(mode & HDM_P_BITMASK);
	} else if (mode & HDM_KERNEL_POST) {
		hdm_post_setting(mode & HDM_P_BITMASK);
		return count;
	}

#if defined(CONFIG_ARCH_EXYNOS)
	hdm_info("%s lsi uh call is supposed to be done here\n", __func__);
	hdm_info("%s mode : 0x%x\n", __func__, mode);

#if defined(CONFIG_UH)
	for (i = 0; i < HDM_DEVICE_MAX; i++) {
		if (mode & (1<<i)) {
			switch(i) {
			case HDM_DEVICE_CAMERA:
				hdm_info("%s CAMERA uh_call\n", __func__);
				command = APP_HDM_EVENT_CAMERA;
				break;
			case HDM_DEVICE_MMC:
				hdm_info("%s MMC uh_call\n", __func__);
				command = APP_HDM_EVENT_MMC;
				break;
			case HDM_DEVICE_USB:
				hdm_info("%s USB uh_call\n", __func__);
				command = APP_HDM_EVENT_USB;
				break;
			case HDM_DEVICE_WIFI:
				hdm_info("%s WIFI uh_call\n", __func__);
				command = APP_HDM_EVENT_WIFI;
				break;
			case HDM_DEVICE_BLUETOOTH:
				hdm_info("%s BT uh_call\n", __func__);
				command = APP_HDM_EVENT_BLUETOOTH;
				break;
			default:
				hdm_info("%s wrong cmd\n", __func__);
				break;
			}
			uh_call(UH_APP_HDM, command, 0, 0, 0, 0);
		}
	}
#else
	hdm_info("%s uh is not enabled yet\n", __func__);
#endif
#endif

#if defined(CONFIG_ARCH_QCOM)
	hdm_info("%s qc smc/hyp call is supposed to be done here\n", __func__);
	hdm_info("%s mode : 0x%x\n", __func__, mode);
#if defined(CONFIG_UH)
	hdm_info("%s i : %d, command : %d\n", __func__, i, command);
#endif
#endif

error:
	return count;
}
static DEVICE_ATTR(hdm_test, 0660, show_hdm_test, store_hdm_test);
#endif

#if defined(CONFIG_ARCH_QCOM)
static uint64_t qseelog_shmbridge_handle;
static int __init __hdm_init_of(void)
{
	struct device_node *node;
	struct resource r;
	int ret;
	phys_addr_t addr;
	u64 size;

	uint32_t ns_vmids[] = {VMID_HLOS_FREE};
	uint32_t ns_vm_perms[] = {PERM_READ | PERM_WRITE};
	uint32_t ns_vm_nums = 1;
	
	int src_vmids[1] = {VMID_HLOS};
	int dest_vmids[1] = {VMID_HLOS_FREE};
	int dest_perms[1] = {PERM_READ | PERM_WRITE};

	hdm_info("%s start\n", __func__);

	node = of_find_node_by_name(NULL, "samsung,sec_hdm");
	if(!node) {
		hdm_err("failed of_find_node_by_name\n");
		return -ENODEV;
	}
	
	node = of_parse_phandle(node, "memory-region", 0);
	if(!node) {
		hdm_err("no memory-region specified\n");
		return -EINVAL;
	}


	ret = of_address_to_resource(node, 0, &r);
	if(ret) {
		hdm_err("failed of_address_to_resource\n");
		return ret;
	}

	addr = r.start;
	size = resource_size(&r);

	ret = hyp_assign_phys(addr, size, src_vmids, 1, dest_vmids, dest_perms, 1);
	if (ret) {
		hdm_err("%s: failed to assign, rc:%d\n", __func__, ret);
	}

	ret = qtee_shmbridge_register(addr, size, ns_vmids, ns_vm_perms, 
			ns_vm_nums, PERM_READ | PERM_WRITE, &qseelog_shmbridge_handle);
	if (ret) {
		hdm_err("%s: failed to bridge_register, rc:%d\n", __func__, ret);
	}

	hdm_info("%s done\n", __func__);
	return 0;
}
#endif

static int __init hdm_test_init(void)
{
	struct device *dev;
#if defined(CONFIG_ARCH_QCOM)
	int err;
#endif
	dev = sec_device_create(NULL, "hdm");
	WARN_ON(!dev);
	if (IS_ERR(dev)) {
		hdm_err("%s Failed to create devce\n", __func__);
	}

	if (device_create_file(dev, &dev_attr_hdm_policy) < 0) {
		hdm_err("%s Failed to create device file\n", __func__);
	}

#if defined(CONFIG_HDM_DEBUG)
	if (device_create_file(dev, &dev_attr_hdm_test) < 0) {
		hdm_err("%s Failed to create device file\n", __func__);
	}
#endif

#if defined(CONFIG_ARCH_QCOM)
	err = __hdm_init_of();
	if (err)
		return err;
#endif

	hdm_info("%s end\n", __func__);
	return 0;
}

module_init(hdm_test_init);
