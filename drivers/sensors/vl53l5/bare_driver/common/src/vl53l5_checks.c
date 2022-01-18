
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

#include "vl53l5_checks.h"
#include "vl53l5_commands.h"
#include "vl53l5_register_utils.h"
#include "vl53l5_globals.h"
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

#define TEST_NVM_REG_INDEX 0x21

#define PAGE_SELECT_INDEX 0x7fff

static int32_t _test_end_block(struct vl53l5_dev_handle_t *p_dev)
{
	uint8_t *p_buff = NULL;
	int32_t status = VL53L5_ERROR_NONE;
	const uint32_t end_of_data_footer_index = 8;

	p_buff = VL53L5_COMMS_BUFF(p_dev);

	if ((p_buff[end_of_data_footer_index] & 0x0f) !=
			DCI_BH__P_TYPE__END_OF_DATA)
		status = VL53L5_DCI_END_BLOCK_ERROR;

	return status;
}

int32_t vl53l5_test_map_version(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = VL53L5_ERROR_NONE;
	uint8_t *p_buff = NULL;
	uint16_t map_major = 0;
	uint16_t map_minor = 0;
	uint32_t block_header = 0;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	p_buff = VL53L5_COMMS_BUFF(p_dev);

	block_header = vl53l5_decode_uint32_t(4, p_buff);
	if (block_header != VL53L5_MAP_VERSION_BH) {
		status = VL53L5_VERSION_IDX_NOT_PRESENT;
		goto exit;
	}
	p_buff += 4;

	map_major = vl53l5_decode_uint16_t(2, p_buff);
	p_buff += 2;

	map_minor = vl53l5_decode_uint16_t(2, p_buff);

	trace_print(
		VL53L5_TRACE_LEVEL_INFO, "Map version: %u.%u\n",
		map_major, map_minor);

	if ((map_major != MAP_VERSION_MAJOR) ||
	    (map_minor != MAP_VERSION_MINOR)) {
		status = VL53L5_DCI_VERSION_ERROR;
		goto exit;
	}

exit:
	LOG_FUNCTION_END(status);
	return status;
}

int32_t vl53l5_check_map_version(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = VL53L5_ERROR_NONE;
	uint32_t block_headers[1] = {
		VL53L5_MAP_VERSION_BH
	};

	LOG_FUNCTION_START("");

	p_dev->host_dev.version_match.map_version_match = false;

	status = vl53l5_encode_block_headers(p_dev, block_headers, 1, false);
	if (status != VL53L5_ERROR_NONE)
		goto exit;

	status = vl53l5_execute_command(p_dev, DCI_CMD_ID__GET_PARMS);
	if (status != VL53L5_ERROR_NONE)
		goto exit;

	status = _test_end_block(p_dev);
	if (status != VL53L5_ERROR_NONE)
		goto exit;

	status = vl53l5_test_map_version(p_dev);
	if (status != VL53L5_ERROR_NONE)
		goto exit;

	p_dev->host_dev.version_match.map_version_match = true;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
