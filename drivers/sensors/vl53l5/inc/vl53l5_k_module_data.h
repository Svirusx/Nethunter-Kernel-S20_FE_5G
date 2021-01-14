
/*
* Copyright (c) 2020, STMicroelectronics - All Rights Reserved
*
* This file is part of VL53L5 Kernel Driver and is dual licensed,
* either 'STMicroelectronics Proprietary license'
* or 'BSD 3-clause "New" or "Revised" License' , at your option.
*
********************************************************************************
*
* 'STMicroelectronics Proprietary license'
*
********************************************************************************
*
* License terms: STMicroelectronics Proprietary in accordance with licensing
* terms at www.st.com/sla0081
*
* STMicroelectronics confidential
* Reproduction and Communication of this document is strictly prohibited unless
* specifically authorized in writing by STMicroelectronics.
*
*
********************************************************************************
*
* Alternatively, VL53L5 Kernel Driver may be distributed under the terms of
* 'BSD 3-clause "New" or "Revised" License', in which case the following
* provisions apply instead of the ones mentioned above :
*
********************************************************************************
*
* License terms: BSD 3-clause "New" or "Revised" License.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its contributors
* may be used to endorse or promote products derived from this software
* without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*
********************************************************************************
*
*/

#ifndef VL53L5_K_MODULE_DATA_H_
#define VL53L5_K_MODULE_DATA_H_

#include <linux/miscdevice.h>
#include "vl53l5_k_user_api.h"
#include "vl53l5_platform_user_data.h"
#include "vl53l5_k_state_def.h"
#include "vl53l5_k_spi_def.h"
#include "vl53l5_k_gpio_def.h"
#include "vl53l5_k_driver_config.h"
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
#include <linux/regulator/consumer.h>
#ifdef CONFIG_SENSORS_VL53L5_SUPPORT_UAPI
#include <uapi/linux/sensor/range_sensor.h>
#endif

#include <linux/sensor/sensors_core.h> //for sec dump
#endif
struct vl53l5_k_module_t {
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	struct device *factory_device;
#endif
	int id;

	char name[64];

	struct miscdevice miscdev;

	struct vl53l5_dev_handle_t stdev;

	const char *firmware_name;
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	const char *genshape_name;
#endif
	int comms_type;

	int config_preset;

	struct mutex mutex;

	struct vl53l5_k_spi_data_t spi_info;

	struct vl53l5_k_gpio_settings_t gpio;

	char comms_buffer[VL53L5_COMMS_BUFFER_SIZE_BYTES];

	enum vl53l5_k_state_preset state_preset;

	struct vl53l5_ranging_mode_flags_t ranging_flags;

	enum vl53l5_power_states power_state;

	enum vl53l5_range_servicing_mode range_mode;

	enum vl53l5_k_rotation_select rotation_mode;

	unsigned int range_filter;

	struct range_t {

		int count;

		bool is_valid;

		struct vl53l5_range_data_t data;
	} range;

	struct calibration_data_t {

		struct vl53l5_version_t version;

		struct vl53l5_module_info_t info;

		struct vl53l5_calibration_dev_t cal_data;
	} calibration;

	int polling_count;

	unsigned int polling_start_time_ms;

	unsigned int polling_sleep_time_ms;

	bool irq_is_active;

	bool irq_is_running;

	int irq_val;

	int is_enabled;

	int worker_is_running;

	int timeout_occurred;

	struct delayed_work dwork;

	struct list_head reader_list;

	wait_queue_head_t wait_queue;

	int32_t last_driver_error;

	struct vl53l5_module_info_t m_info;

#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	const char *avdd_vreg_name;
	const char *iovdd_vreg_name;

	struct regulator *avdd_vreg;
	struct regulator *iovdd_vreg;
	enum vl53l5_k_rotation_select fac_rotation_mode;

	int32_t min;
	int32_t avg;
	int32_t max;
	int32_t data[64];
	int8_t enabled;
	int8_t test_mode;
	int8_t failed_count;
	struct vl53l5_range_data_t range_data;
#ifdef CONFIG_SENSORS_VL53L5_SUPPORT_UAPI
	struct range_sensor_data_t 	af_range_data;
#endif

	bool load_calibration;
	bool read_p2p_cal_data;
	bool read_data_valid;
	bool suspend_state;

	int32_t max_targets_per_zone;
	int32_t number_of_zones;
	int32_t print_counter;
	uint32_t force_suspend_count;
	struct notifier_block dump_nb;  //for sec dump
	struct pinctrl_state *pinctrl_vddoff;
	struct pinctrl *pinctrl;
#endif
};

#endif
