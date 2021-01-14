/* Copyright (c) 2017, The Linux Foundation. All rights reserved.
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

#include <linux/module.h>
#include <linux/firmware.h>
#include <cam_sensor_cmn_header.h>
#include "cam_sensor_thread.h"
#include "cam_sensor_soc.h"
#include "cam_sensor_core.h"
#include "cam_sensor_util.h"
#include "cam_debug_util.h"

/**
 * cam_sensor_thread_func - create thread
 * @data:	  ctrl structure
 *
 * Returns success or failure
 */
static int cam_sensor_thread_func(void *data)
{
	int rc = 0;
	struct cam_sensor_ctrl_t *s_ctrl = NULL;
	uint32_t frame_cnt = 0;

	if (!data) {
		CAM_ERR(CAM_SENSOR, "Invalid Args");
		return -EINVAL;
	}

	CAM_INFO(CAM_SENSOR, "E");

	s_ctrl = (struct cam_sensor_ctrl_t *)data;
	s_ctrl->is_thread_started = true;

	while (true) {
		if (!s_ctrl->is_thread_started) {
			CAM_INFO(CAM_SENSOR, "Thread is stopped");
			break;
		}

		rc = camera_io_dev_read(&s_ctrl->io_master_info, 0x0005,
				&frame_cnt, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "[CNT_DBG] sensor_id 0x%x Failed to read frame_cnt",
				s_ctrl->sensordata->slave_info.sensor_id);
		}
		else {
			CAM_INFO(CAM_SENSOR, "[CNT_DBG] sensor_id 0x%x frame_cnt 0x%x",
				s_ctrl->sensordata->slave_info.sensor_id, frame_cnt);
		}
		msleep(33);
	}

	CAM_INFO(CAM_SENSOR, "X");

	return rc;
}

/**
 * cam_sensor_thread_create - create thread
 * @s_ctrl:  ctrl structure
 *
 * Returns success or failure
 */
int cam_sensor_thread_create(struct cam_sensor_ctrl_t *s_ctrl)
{
	int retries = 30;

	CAM_INFO(CAM_SENSOR, "E");

	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "Invalid Args");
		return -EINVAL;
	}

	if (s_ctrl->is_thread_started) {
		CAM_ERR(CAM_SENSOR, "Already started");
		return -EBUSY;
	}

	s_ctrl->is_thread_started = false;
	s_ctrl->sensor_thread = kthread_run(cam_sensor_thread_func, (void *)s_ctrl, "CAM_SENSOR");
	if (IS_ERR(s_ctrl->sensor_thread))
		return -EINVAL;

	while (s_ctrl->is_thread_started == false) {
		usleep_range(2000, 3000);
		if (retries < 0) {
			CAM_ERR(CAM_SENSOR, "Fail to start thread");
			break;
		}
		retries--;
	}

	CAM_INFO(CAM_SENSOR, "X");
	return 0;
}

/**
 * cam_sensor_thread_destroy - destroy thread
 * @s_ctrl:  ctrl structure
 *
 * Returns success or failure
 */
int cam_sensor_thread_destroy(struct cam_sensor_ctrl_t *s_ctrl)
{
	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "Invalid Args");
		return -EINVAL;
	}

	if (!s_ctrl->is_thread_started) {
		CAM_DBG(CAM_SENSOR, "Thread is not started");
		return 0;
	}

	CAM_INFO(CAM_SENSOR, "E");

	s_ctrl->is_thread_started = false;
	if (s_ctrl->sensor_thread) {
		kthread_stop(s_ctrl->sensor_thread);
		s_ctrl->sensor_thread = NULL;
	}

	CAM_INFO(CAM_SENSOR, "X");

	return 0;
}
