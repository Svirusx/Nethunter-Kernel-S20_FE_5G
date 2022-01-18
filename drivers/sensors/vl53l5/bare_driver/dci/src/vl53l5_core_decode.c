
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

#include "dci_size.h"
#include "dci_ui_size.h"
#include "ic_checkers_size.h"
#include "vl53l5_core_decode.h"
#include "vl53l5_core_enum_type.h"
#include "vl53l5_core_map_idx.h"
#include "vl53l5_core_dev_path.h"
#include "vl53l5_dci_utils.h"
#include "vl53l5_error_codes.h"
#include "vl53l5_platform_log.h"
#include "common_datatype_size.h"

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START( \
		VL53L5_TRACE_MODULE_DCI_DECODE, fmt, ##__VA_ARGS__)

#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END( \
		VL53L5_TRACE_MODULE_DCI_DECODE, status, ##__VA_ARGS__)

#define LOG_FUNCTION_END_FMT(status, ...) \
	_LOG_FUNCTION_END_FMT( \
		VL53L5_TRACE_MODULE_DCI_DECODE, status, ##__VA_ARGS__)
static int32_t _decode_dci_grp__map_version(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct dci_grp__map_version_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_MAP_VERSION_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->map__major =
		vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	buff_count += BYTE_2;

	pstruct->map__minor =
		vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	buff_count += BYTE_2;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _decode_dci_grp__silicon_temperature_data(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct dci_grp__silicon_temperature_data_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_SILICON_TEMPERATURE_DATA_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->silicon_temp_degc__start =
		vl53l5_decode_int8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->silicon_temp_degc__end =
		vl53l5_decode_int8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->silicon_temp__pad_0 =
		vl53l5_decode_int8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->silicon_temp__pad_1 =
		vl53l5_decode_int8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _decode_dci_grp__zone_cfg(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct dci_grp__zone_cfg_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_ZONE_CFG_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->zone__grid_cols =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->zone__grid_rows =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->zone__grid_x_ll =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->zone__grid_y_ll =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->zone__grid_x_pitch =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->zone__grid_y_pitch =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->zone__pad_0 =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->zone__pad_1 =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

int32_t vl53l5_core_decode_cmd(
	uint16_t idx,
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	switch (idx) {
	case DEV_MAP_VERSION_IDX:
		status =
		_decode_dci_grp__map_version(
			buffer_size,
			buffer,
			&VL53L5_MAP_VERSION(p_dev));
		break;

	case DEV_SILICON_TEMP_DATA_IDX:
		status =
		_decode_dci_grp__silicon_temperature_data(
			buffer_size,
			buffer,
			&VL53L5_SILICON_TEMP_DATA(p_dev));
		break;
	case DEV_ZONE_CFG_IDX:
		status =
		_decode_dci_grp__zone_cfg(
			buffer_size,
			buffer,
			&VL53L5_ZONE_CFG(p_dev));
		break;

	default:
		status = VL53L5_INVALID_IDX_DECODE_CMD_ERROR;
		break;
	}

	LOG_FUNCTION_END(status);
	return status;

}
