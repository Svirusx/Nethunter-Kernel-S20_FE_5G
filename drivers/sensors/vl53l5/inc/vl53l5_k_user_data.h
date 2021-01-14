
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

#ifndef VL53L5_K_IOCTL_DATA_H_
#define VL53L5_K_IOCTL_DATA_H_

#include "vl53l5_k_user_def.h"
#include "vl53l5_api_ranging.h"
#include "vl53l5_api_range_decode.h"
#include "vl53l5_api_core.h"

struct vl53l5_k_raw_buffer_data_t {

	uint8_t buffer[VL53L5_K_CONFIG_DATA_MAX_SIZE];

	uint32_t count;
};

struct vl53l5_k_get_parameters_data_t {
	struct vl53l5_k_raw_buffer_data_t raw_data;
	uint32_t headers[VL53L5_K_MAX_NUM_GET_HEADERS];
	uint32_t header_count;
};
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
struct vl53l5_k_error_info_t {

	int32_t last_driver_error;
};
#endif
enum vl53l5_k_config_preset {
	VL53L5_CFG__REFSPAD = 0,
	VL53L5_CFG__XTALK_GEN1_1000__8X8,
	VL53L5_CFG__XTALK_GEN1_600__8X8,
	VL53L5_CFG__XTALK_GEN2_600__8X8,
	VL53L5_CFG__XTALK_GEN2_300__8X8,
	VL53L5_CFG__OFFSET_1000__8X8,
	VL53L5_CFG__OFFSET_600__8X8,
	VL53L5_CFG__OFFSET_300__8X8,
	VL53L5_CFG__BACK_TO_BACK__4X4__30HZ,
	VL53L5_CFG__BACK_TO_BACK__4X4__15HZ,
	VL53L5_CFG__BACK_TO_BACK__4X4__10HZ,
	VL53L5_CFG__BACK_TO_BACK__8X8__15HZ,
	VL53L5_CFG__BACK_TO_BACK__8X8__10HZ,
	VL53L5_CFG__TIMED__8X8__15HZ__5MS,
	VL53L5_CFG__TIMED__8X8__10HZ__5MS,
	VL53L5_CFG__TIMED__4X4__15HZ__5MS,
	VL53L5_CAL__GENERIC_XTALK_SHAPE,
	VL53L5_CFG__STATIC__SS_0PC__VCSELCP_ON,
	VL53L5_CFG__STATIC__SS_1PC__VCSELCP_ON,
	VL53L5_CFG__STATIC__SS_2PC__VCSELCP_ON,
	VL53L5_CFG__STATIC__SS_4PC__VCSELCP_ON,
	VL53L5_CFG__STATIC__SS_0PC__VCSELCP_OFF,
	VL53L5_CFG__STATIC__SS_1PC__VCSELCP_OFF,
	VL53L5_CFG__STATIC__SS_2PC__VCSELCP_OFF,
	VL53L5_CFG__STATIC__SS_4PC__VCSELCP_OFF,
};

struct vl53l5_fw_header_t {
	unsigned int fw_ver_major;
	unsigned int fw_ver_minor;
	unsigned int config_ver_major;
	unsigned int config_ver_minor;
	unsigned int fw_offset;
	unsigned int fw_size;
	unsigned int cfg00_offset;
	unsigned int cfg00_size;
	unsigned int cfg01_offset;
	unsigned int cfg01_size;
	unsigned int cfg02_offset;
	unsigned int cfg02_size;
	unsigned int cfg03_offset;
	unsigned int cfg03_size;
	unsigned int cfg04_offset;
	unsigned int cfg04_size;
	unsigned int cfg05_offset;
	unsigned int cfg05_size;
	unsigned int cfg06_offset;
	unsigned int cfg06_size;
	unsigned int cfg07_offset;
	unsigned int cfg07_size;
	unsigned int cfg08_offset;
	unsigned int cfg08_size;
	unsigned int cfg09_offset;
	unsigned int cfg09_size;
	unsigned int cfg10_offset;
	unsigned int cfg10_size;
	unsigned int cfg11_offset;
	unsigned int cfg11_size;
	unsigned int cfg12_offset;
	unsigned int cfg12_size;
	unsigned int cfg13_offset;
	unsigned int cfg13_size;
	unsigned int cfg14_offset;
	unsigned int cfg14_size;
	unsigned int cfg15_offset;
	unsigned int cfg15_size;
	unsigned int cal00_offset;
	unsigned int cal00_size;
	unsigned int sta00_offset;
	unsigned int sta00_size;
	unsigned int sta01_offset;
	unsigned int sta01_size;
	unsigned int sta02_offset;
	unsigned int sta02_size;
	unsigned int sta03_offset;
	unsigned int sta03_size;
	unsigned int sta04_offset;
	unsigned int sta04_size;
	unsigned int sta05_offset;
	unsigned int sta05_size;
	unsigned int sta06_offset;
	unsigned int sta06_size;
	unsigned int sta07_offset;
	unsigned int sta07_size;
};

enum vl53l5_k_rotation_select {
	VL53L5_K_ROTATION_SEL__NONE = 0,
	VL53L5_K_ROTATION_SEL__CW_90,
	VL53L5_K_ROTATION_SEL__CW_180,
	VL53L5_K_ROTATION_SEL__CW_270,
	VL53L5_K_ROTATION_SEL__MIRROR_X,
	VL53L5_K_ROTATION_SEL__MIRROR_Y
};

#define VL53L5_K__TGT_OP_FILTER_EN__NOUPDATE \
	(1U << (unsigned int)DCI_TARGET_STATUS__NOUPDATE)
#define VL53L5_K__TGT_OP_FILTER_EN__MSRCNOTARGET \
	(1U << (unsigned int)DCI_TARGET_STATUS__MSRCNOTARGET)
#define VL53L5_K__TGT_OP_FILTER_EN__RANGEPHASECHECK \
	(1U << (unsigned int)DCI_TARGET_STATUS__RANGEPHASECHECK)
#define VL53L5_K__TGT_OP_FILTER_EN__SIGMATHRESHOLDCHECK \
	(1U << (unsigned int)DCI_TARGET_STATUS__SIGMATHRESHOLDCHECK)
#define VL53L5_K__TGT_OP_FILTER_EN__PHASECONSISTENCY \
	(1U << (unsigned int)DCI_TARGET_STATUS__PHASECONSISTENCY)
#define VL53L5_K__TGT_OP_FILTER_EN__RANGECOMPLETE \
	(1U << (unsigned int)DCI_TARGET_STATUS__RANGECOMPLETE)
#define VL53L5_K__TGT_OP_FILTER_EN__RANGECOMPLETE_NO_WRAP_CHECK \
	(1U << (unsigned int)DCI_TARGET_STATUS__RANGECOMPLETE_NO_WRAP_CHECK)
#define VL53L5_K__TGT_OP_FILTER_EN__EVENTCONSISTENCY \
	(1U << (unsigned int)DCI_TARGET_STATUS__EVENTCONSISTENCY)
#define VL53L5_K__TGT_OP_FILTER_EN__MINSIGNALEVENTCHECK \
	(1U << (unsigned int)DCI_TARGET_STATUS__MINSIGNALEVENTCHECK)
#define VL53L5_K__TGT_OP_FILTER_EN__RANGECOMPLETE_MERGED_PULSE \
	(1U << (unsigned int)DCI_TARGET_STATUS__RANGECOMPLETE_MERGED_PULSE)
#define VL53L5_K__TGT_OP_FILTER_EN__PREV_RANGE_NO_TARGETS \
	(1U << (unsigned int)DCI_TARGET_STATUS__PREV_RANGE_NO_TARGETS)
#define VL53L5_K__TGT_OP_FILTER_EN__RANGEIGNORETHRESHOLD \
	(1U << (unsigned int)DCI_TARGET_STATUS__RANGEIGNORETHRESHOLD)
#define VL53L5_K__TGT_OP_FILTER_EN__TARGETDUETOBLUR \
	(1U << (unsigned int)DCI_TARGET_STATUS__TARGETDUETOBLUR)

#define VL53L5_K__TGT_OP_FILTER_FLAGS__REMOVE_BLURRED \
	VL53L5_K__TGT_OP_FILTER_EN__RANGEPHASECHECK | \
	VL53L5_K__TGT_OP_FILTER_EN__SIGMATHRESHOLDCHECK | \
	VL53L5_K__TGT_OP_FILTER_EN__PHASECONSISTENCY | \
	VL53L5_K__TGT_OP_FILTER_EN__RANGECOMPLETE | \
	VL53L5_K__TGT_OP_FILTER_EN__RANGECOMPLETE_NO_WRAP_CHECK | \
	VL53L5_K__TGT_OP_FILTER_EN__MINSIGNALEVENTCHECK | \
	VL53L5_K__TGT_OP_FILTER_EN__RANGECOMPLETE_MERGED_PULSE | \
	VL53L5_K__TGT_OP_FILTER_EN__PREV_RANGE_NO_TARGETS

#define VL53L5_K__TGT_OP_FILTER_FLAGS__VALID_ONLY  \
	VL53L5_K__TGT_OP_FILTER_EN__RANGECOMPLETE | \
	VL53L5_K__TGT_OP_FILTER_EN__RANGECOMPLETE_NO_WRAP_CHECK | \
	VL53L5_K__TGT_OP_FILTER_EN__RANGECOMPLETE_MERGED_PULSE

#define VL53L5_K__TGT_OP_FILTER_FLAGS__ALL \
	VL53L5_K__TGT_OP_FILTER_EN__MSRCNOTARGET | \
	VL53L5_K__TGT_OP_FILTER_EN__RANGEPHASECHECK | \
	VL53L5_K__TGT_OP_FILTER_EN__SIGMATHRESHOLDCHECK | \
	VL53L5_K__TGT_OP_FILTER_EN__PHASECONSISTENCY | \
	VL53L5_K__TGT_OP_FILTER_EN__RANGECOMPLETE | \
	VL53L5_K__TGT_OP_FILTER_EN__RANGECOMPLETE_NO_WRAP_CHECK | \
	VL53L5_K__TGT_OP_FILTER_EN__EVENTCONSISTENCY | \
	VL53L5_K__TGT_OP_FILTER_EN__MINSIGNALEVENTCHECK | \
	VL53L5_K__TGT_OP_FILTER_EN__RANGECOMPLETE_MERGED_PULSE | \
	VL53L5_K__TGT_OP_FILTER_EN__PREV_RANGE_NO_TARGETS | \
	VL53L5_K__TGT_OP_FILTER_EN__RANGEIGNORETHRESHOLD | \
	VL53L5_K__TGT_OP_FILTER_EN__TARGETDUETOBLUR

#define VL53L5_K__TGT_OP_FILTER_FLAGS \
	VL53L5_K__TGT_OP_FILTER_FLAGS__VALID_ONLY

#endif
