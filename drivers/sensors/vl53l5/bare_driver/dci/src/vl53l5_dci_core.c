
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

#include "vl53l5_dci_core.h"
#include "vl53l5_dci_helpers.h"
#include "vl53l5_dci_utils.h"
#include "vl53l5_driver_dev_path.h"
#include "vl53l5_globals.h"
#include "vl53l5_platform.h"
#include "vl53l5_platform_log.h"
#include "vl53l5_platform_user_config.h"
#include "vl53l5_error_codes.h"
#include "dci_luts.h"

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L5_TRACE_MODULE_DCI, \
	level, VL53L5_TRACE_FUNCTION_ALL, ##__VA_ARGS__)

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L5_TRACE_MODULE_DCI, fmt, ##__VA_ARGS__)

#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L5_TRACE_MODULE_DCI, status, ##__VA_ARGS__)

#define CHECK_FOR_TIMEOUT(status, p_dev, start_ms, end_ms, timeout_ms) \
	(status = vl53l5_check_for_timeout(\
		(p_dev), start_ms, end_ms, timeout_ms))

#define CMD_STATUS_COMPLETE(p_dev) \
	(VL53L5_CMD_INFO(p_dev).cmd__rtn_command_status \
		== DCI_CMD_STATUS__COMPLETED)

#define CMD_STATUS_ERROR(p_dev) \
	(VL53L5_CMD_INFO(p_dev).cmd__rtn_command_status \
		== DCI_CMD_STATUS__ERROR)

#define TRANS_ID_MATCH(p_dev, trans_id) \
	(VL53L5_CMD_INFO(p_dev).cmd__rtn_transaction_id == trans_id)

#define DCI_COMMAND_FOOTER_SIZE 4
#define DCI_END_OF_DATA_FOOTER_SIZE 4
#define DCI_END_OF_DATA_TYPE 0xf
#define DCI_COMMAND_STATUS_SIZE 4

#define UI_COMMAND_WRITE_END (DCI_UI__COMMAND_WRITE__END_IDX & 0xFFFF)
#define UI_COMMAND_READ_END \
	(UI_COMMAND_WRITE_END - DCI_UI__COMMAND_FOOTER__SIZE_BYTES)

#define UI_STATUS_START (DCI_UI__COMMAND_INFO__START_IDX & 0xFFFF)

#define CMD_STATUS_TIMEOUT_MS 1000

static int32_t _encode_end_data_footer(
	uint8_t *p_buff, uint32_t *p_count);

static void _encode_command_footer(
	uint8_t *p_buff, uint32_t *p_count,
	uint8_t command_id, uint8_t transaction_id);

static int32_t _write_comms_buffer_to_command_ui(
	struct vl53l5_dev_handle_t *p_dev);

static int32_t _read_command_ui_to_comms_buffer(
	struct vl53l5_dev_handle_t *p_dev);

static int32_t _poll_command_status_block(struct vl53l5_dev_handle_t *p_dev,
	uint32_t trans_id, uint32_t timeout_ms, uint32_t start_time_ms,
	bool check_trans_id, bool check_cmd_status);

static int32_t _get_command_status(struct vl53l5_dev_handle_t *p_dev);

int32_t vl53l5_dci_write_command(
	struct vl53l5_dev_handle_t *p_dev,
	uint8_t command_id,
	uint8_t transaction_id)
{
	int32_t status = VL53L5_ERROR_NONE;
	uint32_t available_count = 0;
	uint32_t required_count = 0;
	uint8_t *p_buff = 0;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	if (command_id > DCI_CMD_ID__STOP_RANGE) {
		status = VL53L5_INVALID_CMD_ID;
		goto exit;
	}

	available_count = VL53L5_COMMS_BUFF_MAX_COUNT(p_dev)
		- VL53L5_COMMS_BUFF_COUNT(p_dev);

	required_count = DCI_COMMAND_FOOTER_SIZE;
	if (VL53L5_COMMS_BUFF_MAX_COUNT(p_dev))
		required_count += DCI_END_OF_DATA_FOOTER_SIZE;

	if (required_count > available_count) {
		status = VL53L5_MAX_BUFFER_SIZE_REACHED;
		goto exit;
	} else if ((VL53L5_COMMS_BUFF_COUNT(p_dev) + required_count) >
			 VL53L5_MAX_CMD_UI_SIZE_BYTES) {
		status = VL53L5_DATA_EXCEEDS_CMD_BUFFER_SIZE;
		goto exit;
	}

	p_buff = VL53L5_COMMS_BUFF(p_dev) + VL53L5_COMMS_BUFF_COUNT(p_dev);

	if (VL53L5_COMMS_BUFF_COUNT(p_dev)) {
		status = _encode_end_data_footer(
			p_buff, &VL53L5_COMMS_BUFF_COUNT(p_dev));
		p_buff += DCI_END_OF_DATA_FOOTER_SIZE;
		if (status != STATUS_OK)
			goto exit;
	}

	_encode_command_footer(
		p_buff, &VL53L5_COMMS_BUFF_COUNT(p_dev), command_id,
		transaction_id);

	status = vl53l5_dci_swap_buffer_byte_ordering(
		VL53L5_COMMS_BUFF(p_dev), VL53L5_COMMS_BUFF_COUNT(p_dev));
	if (status < STATUS_OK)
		goto exit;

	status = _write_comms_buffer_to_command_ui(p_dev);

exit:
	LOG_FUNCTION_END(status);
	return status;
}

int32_t vl53l5_dci_read_command(
	struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	status = _read_command_ui_to_comms_buffer(p_dev);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_dci_swap_buffer_byte_ordering(
		VL53L5_COMMS_BUFF(p_dev), VL53L5_COMMS_BUFF_COUNT(p_dev));
	if (status < STATUS_OK)
		goto exit;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

int32_t vl53l5_dci_poll_command_status(
	struct vl53l5_dev_handle_t *p_dev,
	uint32_t trans_id,
	uint32_t timeout_ms)
{
	int32_t status = VL53L5_ERROR_NONE;
	uint32_t start_time_ms   = 0;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	if (timeout_ms == 0)

		timeout_ms = CMD_STATUS_TIMEOUT_MS;

	status = vl53l5_get_tick_count(p_dev, &start_time_ms);
	if (status < STATUS_OK)
		goto exit;

	status = _poll_command_status_block(p_dev, trans_id, timeout_ms,
		start_time_ms, true, false);
	if (status != STATUS_OK)
		goto exit;

	status = _poll_command_status_block(p_dev, trans_id, timeout_ms,
		start_time_ms, true, true);

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _encode_end_data_footer(
	uint8_t *p_buff, uint32_t *p_count)
{
	int32_t status = VL53L5_ERROR_NONE;

	status = vl53l5_dci_encode_block_header(
		p_buff, *p_count + DCI_END_OF_DATA_FOOTER_SIZE,
		DCI_END_OF_DATA_TYPE, 0, 0);
	if (status == VL53L5_ERROR_NONE)
		(*p_count) += DCI_END_OF_DATA_FOOTER_SIZE;

	return status;
}

static void _encode_command_footer(
	uint8_t *p_buff, uint32_t *p_count,
	uint8_t command_id, uint8_t transaction_id)
{
	struct dci_ui__cmd_footer_t ui_cmd_footer = {0};

	ui_cmd_footer.cmd_footer.cmd__ip_data_size = *p_count;

	ui_cmd_footer.cmd_footer.cmd__ip_command_id = command_id;

	ui_cmd_footer.cmd_footer.cmd__ip_transaction_id = transaction_id;

	vl53l5_encode_uint32_t(
		ui_cmd_footer.cmd_footer.bytes, 4, p_buff);

	(*p_count) += DCI_COMMAND_FOOTER_SIZE;
}

static int32_t _write_comms_buffer_to_command_ui(
	struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = VL53L5_ERROR_NONE;
	uint16_t start_index = (uint16_t)((UI_COMMAND_WRITE_END
				- VL53L5_COMMS_BUFF_COUNT(p_dev)) + 1);

	status = vl53l5_write_multi(
		p_dev, start_index, VL53L5_COMMS_BUFF(p_dev),
		VL53L5_COMMS_BUFF_COUNT(p_dev));

	return status;
}

static int32_t _read_command_ui_to_comms_buffer(
	struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = VL53L5_ERROR_NONE;
	uint16_t start_index = 0;

	VL53L5_RESET_COMMS_BUFF(p_dev);

	if (VL53L5_CMD_INFO(p_dev).cmd__rtn_data_size >
			VL53L5_COMMS_BUFF_MAX_COUNT(p_dev)) {
		status = VL53L5_MAX_BUFFER_SIZE_REACHED;
		goto exit;
	}

	if (VL53L5_CMD_INFO(p_dev).cmd__rtn_data_size >
			VL53L5_MAX_CMD_UI_SIZE_BYTES) {
		status = VL53L5_DATA_EXCEEDS_CMD_BUFFER_SIZE;
		goto exit;
	}

	if ((VL53L5_CMD_INFO(p_dev).cmd__rtn_data_size % 4) != 0) {

		status = VL53L5_BYTE_SWAP_FAIL;
		goto exit;
	}

	start_index = (uint16_t)((UI_COMMAND_READ_END
		- VL53L5_CMD_INFO(p_dev).cmd__rtn_data_size) + 1);
	status = vl53l5_read_multi(
		p_dev, start_index, VL53L5_COMMS_BUFF(p_dev),
		VL53L5_CMD_INFO(p_dev).cmd__rtn_data_size);
	if (status != STATUS_OK)
		goto exit;

	VL53L5_COMMS_BUFF_COUNT(p_dev) =
		VL53L5_CMD_INFO(p_dev).cmd__rtn_data_size;

exit:
	return status;
}

static int32_t _get_command_status(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = VL53L5_ERROR_NONE;

	VL53L5_RESET_COMMS_BUFF(p_dev);

	status = vl53l5_read_multi(
		p_dev, UI_STATUS_START, VL53L5_COMMS_BUFF(p_dev),
		DCI_COMMAND_STATUS_SIZE);
	if (status < STATUS_OK)
		goto exit;

	VL53L5_COMMS_BUFF_COUNT(p_dev) = DCI_COMMAND_STATUS_SIZE;

	status = vl53l5_dci_swap_buffer_byte_ordering(
		VL53L5_COMMS_BUFF(p_dev), VL53L5_COMMS_BUFF_COUNT(p_dev));
	if (status < STATUS_OK)
		goto exit;

	VL53L5_CMD_INFO(p_dev).bytes = vl53l5_decode_uint32_t(
		VL53L5_COMMS_BUFF_COUNT(p_dev), VL53L5_COMMS_BUFF(p_dev));

exit:
	return status;
}

static int32_t _poll_command_status_block(struct vl53l5_dev_handle_t *p_dev,
	uint32_t trans_id, uint32_t timeout_ms, uint32_t start_time_ms,
	bool check_trans_id, bool check_cmd_status)
{
	int32_t status = STATUS_OK;
	uint32_t current_time_ms = 0;
	bool success = false;

	LOG_FUNCTION_START("");

	do {

		status = _get_command_status(p_dev);
		if (status < STATUS_OK)
			goto exit;

		success = ((!check_trans_id || TRANS_ID_MATCH(p_dev, trans_id))
			&& (!check_cmd_status || CMD_STATUS_COMPLETE(p_dev)));
		if (success)
			break;

		if (check_cmd_status && CMD_STATUS_ERROR(p_dev)) {
			status = VL53L5_DCI_CMD_STATUS_ERROR;
			goto exit;
		}

		status = vl53l5_get_tick_count(p_dev, &current_time_ms);
		if (status < STATUS_OK)
			goto exit;

		CHECK_FOR_TIMEOUT(
			status,
			p_dev,
			start_time_ms,
			current_time_ms,
			timeout_ms);
		if (status < STATUS_OK) {
			status = VL53L5_ERROR_CMD_STATUS_TIMEOUT;
			goto exit;
		}

		status = vl53l5_wait_ms(
			p_dev, VL53L5_DEFAULT_COMMS_POLLING_DELAY_MS);
		if (status < STATUS_OK)
			goto exit;
	} while (1);

exit:
	LOG_FUNCTION_END(status);
	return status;
}
