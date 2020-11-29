
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

#include "vl53l5_commands.h"
#include "vl53l5_globals.h"
#include "vl53l5_trans.h"
#include "vl53l5_platform_log.h"
#include "vl53l5_error_codes.h"
#include "vl53l5_dci_core.h"
#include "vl53l5_dci_utils.h"
#include "vl53l5_core_map_bh.h"

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L5_TRACE_MODULE_DCI, \
	level, VL53L5_TRACE_FUNCTION_ALL, ##__VA_ARGS__)

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L5_TRACE_MODULE_DCI, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L5_TRACE_MODULE_DCI, status, ##__VA_ARGS__)

#define BLOCK_HEADER_SIZE 4

#define COMMAND_HAS_RETURN(command_id) \
	((command_id == DCI_CMD_ID__GET_PARMS) ||\
		(command_id == DCI_CMD_ID__START_RANGE))

int32_t vl53l5_encode_block_headers(
	struct vl53l5_dev_handle_t *p_dev,
	uint32_t *p_block_headers,
	uint32_t num_block_headers,
	bool encode_version)
{
	int32_t status = VL53L5_ERROR_NONE;
	uint32_t num_bh = num_block_headers;
	uint32_t i = 0;

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_ISNULL(p_block_headers)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (num_block_headers == 0) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	if (encode_version)
		num_bh++;

	if ((num_bh * BLOCK_HEADER_SIZE) >
			VL53L5_COMMS_BUFF_MAX_COUNT(p_dev)) {
		status = VL53L5_MAX_BUFFER_SIZE_REACHED;
		goto exit;
	}

	VL53L5_RESET_COMMS_BUFF(p_dev);

	if (encode_version) {
		vl53l5_encode_uint32_t(
			VL53L5_MAP_VERSION_BH, BLOCK_HEADER_SIZE,
			VL53L5_COMMS_BUFF_NEXT_BYTE(p_dev));
		VL53L5_COMMS_BUFF_COUNT(p_dev) += BLOCK_HEADER_SIZE;
	}

	for (i = 0; i < num_block_headers; i++) {

		if ((p_block_headers[i] == VL53L5_MAP_VERSION_BH) &&
				(encode_version))
			continue;

		vl53l5_encode_uint32_t(
			p_block_headers[i], BLOCK_HEADER_SIZE,
			VL53L5_COMMS_BUFF_NEXT_BYTE(p_dev));
		VL53L5_COMMS_BUFF_COUNT(p_dev) += BLOCK_HEADER_SIZE;
	}

exit:
	return status;
}

int32_t vl53l5_execute_command(
	struct vl53l5_dev_handle_t *p_dev,
	uint8_t command_id)
{
	int32_t status = VL53L5_ERROR_NONE;
	uint8_t trans_id = 0;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	trans_id = VL53L5_GET_NEXT_TRANS_ID(p_dev);
	status = vl53l5_dci_write_command(
		p_dev, command_id, trans_id);
	if (status != STATUS_OK)
		goto exit;

	status = vl53l5_dci_poll_command_status(p_dev, trans_id, 0);
	if (status != STATUS_OK)
		goto exit;

	if (COMMAND_HAS_RETURN(command_id))

		status = vl53l5_dci_read_command(p_dev);

exit:
	LOG_FUNCTION_END(status);
	return status;
}
