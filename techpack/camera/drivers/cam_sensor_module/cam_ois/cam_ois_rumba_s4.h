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
#ifndef _CAM_OIS_RUMBA_S4_H_
#define _CAM_OIS_RUMBA_S4_H_

#include "cam_ois_dev.h"

struct cam_ois_sinewave_t {
	int sin_x;
	int sin_y;
};

void cam_ois_offset_test(struct cam_ois_ctrl_t *o_ctrl,
	long *raw_data_x, long *raw_data_y, bool is_need_cal);
int cam_ois_get_offset_data(struct cam_ois_ctrl_t *o_ctrl,
	long *raw_data_x, long *raw_data_y);
void cam_ois_get_selftest_data(struct cam_ois_ctrl_t *o_ctrl,
	long *raw_data_x, long *raw_data_y);
int cam_ois_gyro_sensor_calibration(struct cam_ois_ctrl_t *o_ctrl,
	long *raw_data_x, long *raw_data_y);
uint32_t cam_ois_self_test(struct cam_ois_ctrl_t *o_ctrl);
bool cam_ois_sine_wavecheck(struct cam_ois_ctrl_t *o_ctrl, int threshold,
	struct cam_ois_sinewave_t *sinewave, int *result, int num_of_module);
int cam_ois_check_fw(struct cam_ois_ctrl_t *o_ctrl);
int cam_ois_wait_idle(struct cam_ois_ctrl_t *o_ctrl, int retries);
int cam_ois_init(struct cam_ois_ctrl_t *o_ctrl);
int cam_ois_i2c_write(struct cam_ois_ctrl_t *o_ctrl, uint32_t addr, uint32_t data,
    enum camera_sensor_i2c_type addr_type,
	enum camera_sensor_i2c_type data_type);
int32_t cam_ois_set_debug_info(struct cam_ois_ctrl_t *o_ctrl, uint16_t mode);
int cam_ois_get_ois_mode(struct cam_ois_ctrl_t *o_ctrl, uint16_t *mode);
int cam_ois_set_ois_mode(struct cam_ois_ctrl_t *o_ctrl, uint16_t mode);
int cam_ois_set_shift(struct cam_ois_ctrl_t *o_ctrl);
int cam_ois_set_ggfadeup(struct cam_ois_ctrl_t *o_ctrl, uint32_t value);
int cam_ois_set_ggfadedown(struct cam_ois_ctrl_t *o_ctrl, uint32_t value);
int cam_ois_shift_calibration(struct cam_ois_ctrl_t *o_ctrl, uint16_t af_position, uint16_t subdev_id);
int cam_ois_fixed_aperture(struct cam_ois_ctrl_t *o_ctrl);
int cam_ois_rumba_init(struct cam_ois_ctrl_t *o_ctrl);
void cam_ois_reset(void *ctrl);
#endif/* _CAM_OIS_RUMBA_S4_H_ */
