
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

#ifndef _VL53L5_GLOBALS_H_
#define _VL53L5_GLOBALS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vl53l5_types.h"
#include "vl53l5_results_config.h"
#include "dci_ui_memory_defs.h"
#include "dci_size.h"
#include "dci_ui_size.h"

#define VL53L5_ISNULL(ptr) ((ptr) == NULL)

#define VL53L5_CAL_DEV_ISNULL(p_dev)\
	((p_dev)->host_dev.pcalibration_dev == NULL)

#define VL53L5_COMMS_BUFF_ISNULL(p_dev)\
	((p_dev)->host_dev.p_comms_buff == NULL)

#define VL53L5_COMMS_BUFF_ISEMPTY(p_dev)\
	((p_dev)->host_dev.comms_buff_count == 0)

#define VL53L5_COMMS_BUFF(p_dev)\
	((p_dev)->host_dev.p_comms_buff)

#define VL53L5_COMMS_BUFF_COUNT(p_dev)\
	((p_dev)->host_dev.comms_buff_count)

#define VL53L5_COMMS_BUFF_MAX_COUNT(p_dev)\
	((p_dev)->host_dev.comms_buff_max_count)

#define VL53L5_CHECK_COMMS_BUFF_AVAILABLE_BYTES(p_dev, required_bytes)\
	((VL53L5_COMMS_BUFF_COUNT(p_dev) + required_bytes) <\
		VL53L5_COMMS_BUFF_MAX_COUNT(p_dev))

#define VL53L5_COMMS_BUFF_NEXT_BYTE(p_dev)\
	(&VL53L5_COMMS_BUFF(p_dev)[VL53L5_COMMS_BUFF_COUNT(p_dev)])

#define VL53L5_FW_BUFF_ISNULL(p_dev)\
	((p_dev)->host_dev.p_fw_buff == NULL)

#define VL53L5_FW_BUFF_ISEMPTY(p_dev)\
	((p_dev)->host_dev.fw_buff_count == 0)

#define VL53L5_RESET_COMMS_BUFF(p_dev)\
	((p_dev)->host_dev.comms_buff_count = 0)

#define VL53L5_GET_VERSION_CHECK_STATUS(p_dev)\
	(((p_dev)->host_dev.version_match.map_version_match == true) ? \
	 STATUS_OK : VL53L5_VER_CHECK_NOT_PERFORMED)

#define VL53L5_DEV_RANGE_UI_SIZE_BYTES \
	((DCI_UI__COMMAND_INFO__START_IDX & 0xFFFF) - \
	(DCI_UI__DEVICE_INFO__START_IDX & 0xFFFF))

#define VL53L5_MIN_FW_ERROR ((int32_t) 0xfe000000)

#define VL53L5_MAX_FW_ERROR ((int32_t) 0xfeffffff)

#define VL53L5_MAX_FW_STATUS_1_WARNING ((uint8_t) 0x7f)

#define STATUS_OK 0

#define VL53L5_MAX_RANGE_UI_SIZE_BYTES \
	((VL53L5_DEV_RANGE_UI_SIZE_BYTES > VL53L5_RESULTS_TOTAL_SIZE_BYTES) ? \
	VL53L5_RESULTS_TOTAL_SIZE_BYTES : VL53L5_DEV_RANGE_UI_SIZE_BYTES)

#define VL53L5_MAX_CMD_UI_SIZE_BYTES \
	((DCI_UI__COMMAND_INFO__START_IDX - DCI_UI__RANGE_DATA__START_IDX) \
	& 0xffff)

#define	VL53L5_DEV_INFO_BLOCK_SZ DCI_UI__DEVICE_INFO__SIZE_BYTES

#define	VL53L5_HEADER_FOOTER_BLOCK_SZ DCI_UI__DEVICE_INFO__SIZE_BYTES

#define VL53L5_CONFIG_HEADER_SIZE \
	(2 * VL53L5_DCI_UI_PACKED_DATA_BH_SZ + \
	VL53L5_MAP_VERSION_SZ + \
	VL53L5_CFG_INFO_SZ)

#define VL53L5_CALIBRATION_HEADER_SIZE \
	(VL53L5_DCI_UI_PACKED_DATA_BH_SZ + VL53L5_MAP_VERSION_SZ)

#define VL53L5_MAX_CONF_CAL_SIZE_BYTES 1764

#define VL53L5_MAX_APPEND_SIZE_BYTES 20

#define VL53L5_COMMS_BUFFER_SIZE_BYTES \
	((VL53L5_MAX_CONF_CAL_SIZE_BYTES + VL53L5_MAX_APPEND_SIZE_BYTES >\
		VL53L5_MAX_RANGE_UI_SIZE_BYTES) ?\
	 (VL53L5_MAX_CONF_CAL_SIZE_BYTES + VL53L5_MAX_APPEND_SIZE_BYTES) :\
	 VL53L5_MAX_RANGE_UI_SIZE_BYTES)

#define VL53L5_DEBUG_BUFFER_SIZE_BYTES VL53L5_DEV_RANGE_UI_SIZE_BYTES

#define VL53L5_MAX_INPUT_DATA_LENGTH \
	((VL53L5_COMMS_BUFFER_SIZE_BYTES > VL53L5_MAX_CMD_UI_SIZE_BYTES) ?\
	 VL53L5_MAX_CMD_UI_SIZE_BYTES - 8 : VL53L5_COMMS_BUFFER_SIZE_BYTES - 8)

#define BYTE_1 1
#define BYTE_2 2
#define BYTE_3 3
#define BYTE_4 4
#define BYTE_8 8

#define MAX_NUM_RANGE_RETURNS  0

#ifdef __cplusplus
}
#endif

#endif
