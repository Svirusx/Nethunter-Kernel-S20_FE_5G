
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

#ifndef VL53L5_API_CORE_H_
#define VL53L5_API_CORE_H_

#include "vl53l5_platform_user_data.h"
#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define VL53L5_ASSIGN_FW_BUFF(p_dev, fw_buff_ptr, count) \
do {\
	(p_dev)->host_dev.p_fw_buff = (fw_buff_ptr);\
	(p_dev)->host_dev.fw_buff_count = (count);\
} while (0)

#define VL53L5_ASSIGN_COMMS_BUFF(p_dev, comms_buff_ptr, max_count) \
do {\
	(p_dev)->host_dev.p_comms_buff = (comms_buff_ptr);\
	(p_dev)->host_dev.comms_buff_max_count = (max_count);\
} while (0)

#define VL53L5_FGC_STRING_LENGTH 19

struct vl53l5_version_t {
	struct {

		uint16_t ver_major;

		uint16_t ver_minor;

		uint16_t ver_build;

		uint16_t ver_revision;
	} driver;
	struct {

		uint16_t ver_major;

		uint16_t ver_minor;

		uint16_t ver_build;

		uint16_t ver_revision;
	} firmware;
};

struct vl53l5_module_info_t {

	uint8_t fgc[VL53L5_FGC_STRING_LENGTH];

	uint32_t module_id_lo;

	uint32_t module_id_hi;
};

struct vl53l5_ranging_mode_flags_t {

	uint8_t timed_interrupt : 1;

	uint8_t timed_retention : 1;
};

int32_t vl53l5_init(struct vl53l5_dev_handle_t *p_dev);

int32_t vl53l5_term(struct vl53l5_dev_handle_t *p_dev);

int32_t vl53l5_get_version(
	struct vl53l5_dev_handle_t *p_dev,
	struct vl53l5_version_t *p_version);

int32_t vl53l5_get_module_info(
	struct vl53l5_dev_handle_t *p_dev,
	struct vl53l5_module_info_t *p_module_info);

int32_t vl53l5_read_device_error(
	struct vl53l5_dev_handle_t *p_dev,
	int32_t latest_status);

int32_t vl53l5_start(
	struct vl53l5_dev_handle_t *p_dev,
	struct vl53l5_ranging_mode_flags_t *p_flags);

int32_t vl53l5_stop(
	struct vl53l5_dev_handle_t *p_dev,
	struct vl53l5_ranging_mode_flags_t *p_flags);

int32_t vl53l5_get_device_parameters(
	struct vl53l5_dev_handle_t *p_dev,
	uint32_t *p_block_headers,
	uint32_t num_block_headers);

int32_t vl53l5_set_device_parameters(
	struct vl53l5_dev_handle_t *p_dev,
	uint8_t *p_buff,
	uint32_t buff_count);

int32_t vl53l5_decode_calibration_data(
	struct vl53l5_dev_handle_t *p_dev,
	struct vl53l5_calibration_dev_t *p_data,
	struct vl53l5_module_info_t *p_module_info,
	struct vl53l5_version_t *p_version,
	uint32_t *bh_array,
	uint32_t num_bh,
	uint32_t data_size);

int32_t vl53l5_encode_device_info(
	struct vl53l5_dev_handle_t *p_dev,
	struct vl53l5_module_info_t *p_module_info,
	struct vl53l5_version_t *p_version,
	uint8_t *buffer);

#ifdef __cplusplus
}
#endif

#endif
