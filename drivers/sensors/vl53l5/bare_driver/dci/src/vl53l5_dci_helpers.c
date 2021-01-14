
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

#include "vl53l5_dci_helpers.h"
#include "vl53l5_dci_utils.h"
#include "vl53l5_globals.h"
#include "vl53l5_platform.h"
#include "vl53l5_platform_log.h"
#include "vl53l5_error_codes.h"
#include "dci_luts.h"
#include "dci_ui_structs.h"

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L5_TRACE_MODULE_DCI, \
	level, VL53L5_TRACE_FUNCTION_ALL, ##__VA_ARGS__)

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L5_TRACE_MODULE_DCI, fmt, ##__VA_ARGS__)

#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L5_TRACE_MODULE_DCI, status, ##__VA_ARGS__)

#define PAGE_SWITCH_TYPE 0x0c

#define FIRST_NON_DATA_TYPE PAGE_SWITCH_TYPE

static int32_t _calculate_block_header_size_field(
	uint8_t type, uint32_t size_in_bytes, uint16_t *p_size)
{

	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	if (type > 0xF) {
		status = VL53L5_ERROR_INVALID_BH_ENCODE;
		goto exit;
	}

	if (type == DCI_BH__P_TYPE__CONSEC_PARAMS_LIST) {

		*p_size = (uint16_t)size_in_bytes;
	} else if (type < FIRST_NON_DATA_TYPE) {

		*p_size = (uint16_t)(size_in_bytes / type);
	} else if (type == DCI_BH__P_TYPE__END_OF_DATA) {

		*p_size = 0;
	} else {

		status = VL53L5_ERROR_INVALID_BH_ENCODE;
		goto exit;
	}

	if (*p_size > 0xFFF) {
		status = VL53L5_ERROR_INVALID_BH_SIZE;
		goto exit;
	}

exit:
	LOG_FUNCTION_END(status);
	return status;
}

int32_t vl53l5_dci_encode_block_header(
	uint8_t *p_buffer,
	uint32_t max_buffer_size,
	uint8_t type,
	uint32_t block_byte_size,
	uint16_t idx)
{
	int32_t status = VL53L5_ERROR_NONE;
	uint16_t b_size__p_rep = 0;
	struct dci_ui__packed_data__bh_t block_header = {0};

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_buffer)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (max_buffer_size < 4) {
		status = VL53L5_MAX_BUFFER_SIZE_REACHED;
		goto exit;
	}

	status = _calculate_block_header_size_field(
		type, block_byte_size, &b_size__p_rep);
	if (status < STATUS_OK)
		goto exit;

	block_header.packed_data__bh.p_type = (uint32_t)(type & 0xF);
	block_header.packed_data__bh.b_size__p_rep =
		(uint32_t)(b_size__p_rep & 0x0FFF);
	block_header.packed_data__bh.b_idx__p_idx =
		(uint32_t)idx;

	vl53l5_encode_uint32_t(
		(uint32_t)block_header.packed_data__bh.bytes, 4, p_buffer);

exit:
	LOG_FUNCTION_END(status);
	return status;
}

int32_t vl53l5_dci_decode_block_header(
	uint8_t *p_buffer,
	uint32_t max_buffer_size,
	uint8_t *p_type,
	uint32_t *p_block_byte_size,
	uint16_t *p_idx)
{
	int32_t status = VL53L5_ERROR_NONE;
	struct dci_ui__packed_data__bh_t block_header = {0};

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_buffer)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_ISNULL(p_type)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_ISNULL(p_idx)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_ISNULL(p_block_byte_size)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (max_buffer_size < 4) {
		status = VL53L5_MAX_BUFFER_SIZE_REACHED;
		goto exit;
	}

	block_header.packed_data__bh.bytes =
		vl53l5_decode_uint32_t(4, p_buffer);

	*p_type = block_header.packed_data__bh.p_type;
	*p_idx = block_header.packed_data__bh.b_idx__p_idx;

	if (block_header.packed_data__bh.p_type ==
		DCI_BH__P_TYPE__CONSEC_PARAMS_LIST) {

		*p_block_byte_size =
		    (uint32_t)block_header.packed_data__bh.b_size__p_rep;
	} else if (block_header.packed_data__bh.p_type < FIRST_NON_DATA_TYPE) {

		*p_block_byte_size =
		    (uint32_t)block_header.packed_data__bh.b_size__p_rep *
		    (uint32_t)block_header.packed_data__bh.p_type;
	} else {

		*p_block_byte_size = 0;
	}

exit:
	LOG_FUNCTION_END(status);
	return status;
}

int32_t vl53l5_dci_swap_buffer_byte_ordering(
	uint8_t *p_buffer, uint32_t buffer_count)
{
	uint8_t temp_char;
	uint32_t block_index = 0;
	int32_t status = VL53L5_ERROR_NONE;

	if (VL53L5_ISNULL(p_buffer)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	if ((buffer_count % 4) != 0) {
		status = VL53L5_BYTE_SWAP_FAIL;
		goto exit;
	}

	while (block_index < buffer_count) {
		temp_char = *p_buffer;
		*p_buffer = *(p_buffer + 3);
		*(p_buffer + 3) = temp_char;
		temp_char = *(p_buffer + 1);
		*(p_buffer + 1) = *(p_buffer + 2);
		*(p_buffer + 2) = temp_char;
		block_index += 4;
		p_buffer += 4;
	}

exit:
	return status;
}
