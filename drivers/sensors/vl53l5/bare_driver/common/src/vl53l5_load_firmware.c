
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

#include "vl53l5_load_firmware.h"
#include "vl53l5_platform.h"
#include "vl53l5_rom_boot.h"
#include "vl53l5_register_utils.h"
#include "vl53l5_platform_log.h"
#include "vl53l5_dci_helpers.h"
#include "vl53l5_fw_checksum.h"
#include "dci_ui_memory_defs.h"
#include "vl53l5_platform_user_config.h"

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L5_TRACE_MODULE_LOAD_FIRMWARE, \
	level, VL53L5_TRACE_FUNCTION_ALL, ##__VA_ARGS__)

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L5_TRACE_MODULE_LOAD_FIRMWARE, fmt, \
			    ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L5_TRACE_MODULE_LOAD_FIRMWARE, status, \
			  ##__VA_ARGS__)

#define CHECK_FOR_TIMEOUT(status, p_dev, start_ms, end_ms, timeout_ms) \
	(status = vl53l5_check_for_timeout(\
		(p_dev), start_ms, end_ms, timeout_ms))

#define MAX_FW_FILE_SIZE 100000
#define WRITE_CHUNK_SIZE(p_dev) VL53L5_COMMS_BUFF_MAX_COUNT(p_dev)

static int32_t _write_byte(
	struct vl53l5_dev_handle_t *p_dev, uint16_t address, uint8_t value)
{
	return vl53l5_write_multi(p_dev, address, &value, 1);
}

static int32_t _read_byte(
	struct vl53l5_dev_handle_t *p_dev, uint16_t address, uint8_t *p_value)
{
	return vl53l5_read_multi(p_dev, address, p_value, 1);
}

static int32_t _check_fw_checksum(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;
	uint32_t checksum = 0;
	uint8_t data[4] = {0};
	uint16_t ui_addr = (uint16_t)(DCI_UI__FIRMWARE_CHECKSUM_IDX & 0xFFFF);

	LOG_FUNCTION_START("");

	data[0] = 0;
	status = vl53l5_read_multi(p_dev, ui_addr, data, 4);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_dci_swap_buffer_byte_ordering(data, 4);
	if (status < STATUS_OK)
		goto exit;

	checksum = (uint32_t)((data[3] << 24) |
			      (data[2] << 16) |
			      (data[1] << 8) |
			      data[0]);
	trace_print(VL53L5_TRACE_LEVEL_INFO,
		    "Expected Checksum: 0x%x Actual Checksum: 0x%x\n",
		    VL53L5_FW_CHECKSUM, checksum);
	if (checksum != VL53L5_FW_CHECKSUM) {
		status = VL53L5_ERROR_INIT_FW_CHECKSUM;
		goto exit;
	}

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _write_page(
	struct vl53l5_dev_handle_t *p_dev, uint16_t page_offset,
	uint32_t page_size, uint32_t max_chunk_size, uint32_t *p_write_count)
{
	int32_t status = STATUS_OK;
	uint32_t write_size = 0;
	uint32_t remainder_size = 0;
	uint8_t *p_write_buff = NULL;

	if ((page_offset + max_chunk_size) < page_size)
		write_size = max_chunk_size;
	else
		write_size = page_size - page_offset;

	if (*p_write_count > p_dev->host_dev.fw_buff_count) {

		p_write_buff = p_dev->host_dev.p_comms_buff;
		memset(p_write_buff, 0, write_size);
	} else {
		if ((p_dev->host_dev.fw_buff_count - *p_write_count)
				 < write_size) {

			p_write_buff = p_dev->host_dev.p_comms_buff;
			remainder_size =
				p_dev->host_dev.fw_buff_count - *p_write_count;
			memcpy(p_write_buff,
			       p_dev->host_dev.p_fw_buff + *p_write_count,
			       remainder_size);
			memset(p_write_buff + remainder_size,
			       0,
			       write_size - remainder_size);
		} else {

			p_write_buff =
				p_dev->host_dev.p_fw_buff + *p_write_count;
		}
	}

	status = vl53l5_write_multi(p_dev, page_offset, p_write_buff,
				    write_size);
	if (status < STATUS_OK)
		goto exit;

	(*p_write_count) += write_size;

exit:
	return status;
}

static int32_t _reset_mcu_and_wait_boot(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;
	uint8_t u_start[] = {0, 0, 0x42, 0};

	LOG_FUNCTION_START("");

	status = _write_byte(p_dev, 0x7FFF, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_write_multi(p_dev, 0x114, u_start, 4);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x0B, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x0C, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x0B, 0x01);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_wait_mcu_boot(p_dev, VL53L5_BOOT_STATE_HIGH,
				      0, VL53L5_MCU_BOOT_WAIT_DELAY);

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _enable_host_access_to_go1_async(
	struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;
	uint32_t start_time_ms   = 0;
	uint32_t current_time_ms = 0;
	uint8_t m_status = 0;
	uint8_t revision_id = 0;

	LOG_FUNCTION_START("");

	status = _write_byte(p_dev, 0x7FFF, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _read_byte(p_dev, 0x0001, &revision_id);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x7FFF, 0x02);
	if (status < STATUS_OK)
		goto exit;

	if (revision_id != 2) {
		status = VL53L5_UNKNOWN_SILICON_REVISION;
		trace_print(VL53L5_TRACE_LEVEL_ERRORS,
			"ERROR: Unknown silicon revision %d\n", revision_id);
		goto exit;
	}

	status = _write_byte(p_dev, 0x03, 0x0D);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x7FFF, 0x01);
	if (status < STATUS_OK)
		goto exit;

	m_status = 0;
	status = vl53l5_get_tick_count(p_dev, &start_time_ms);
	if (status < STATUS_OK)
		goto exit;

	while ((m_status & 0x10) == 0) {
		status = _read_byte(p_dev, 0x21, &m_status);
		if (status < STATUS_OK)
			goto exit;

		status = vl53l5_get_tick_count(p_dev, &current_time_ms);
		if (status < STATUS_OK)
			goto exit;

		CHECK_FOR_TIMEOUT(
			status, p_dev, start_time_ms, current_time_ms,
			VL53L5_BOOT_COMPLETION_POLLING_TIMEOUT_MS);
		if (status < STATUS_OK) {
			trace_print(
			VL53L5_TRACE_LEVEL_ERRORS,
			"ERROR: timeout waiting for mcu idle m_status %02x\n",
			m_status);
			status = VL53L5_ERROR_MCU_IDLE_TIMEOUT;
			goto exit;
		}
	}

	status = _write_byte(p_dev, 0x7FFF, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x0C, 0x01);
	if (status < STATUS_OK)
		goto exit;
exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _set_to_power_on_status(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	status = _write_byte(p_dev, 0x7FFF, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x101, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x102, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x4002, 0x01);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x4002, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x103, 0x01);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x400F, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x21A, 0x43);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x21A, 0x03);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x21A, 0x01);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x21A, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x219, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x21B, 0x00);
	if (status < STATUS_OK)
		goto exit;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _wake_up_mcu(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	status = _write_byte(p_dev, 0x7FFF, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x0C, 0x00);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x7FFF, 0x01);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x20, 0x07);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x20, 0x06);
	if (status < STATUS_OK)
		goto exit;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _download_fw_to_ram(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	uint16_t tdcm_offset = 0;
	uint8_t tdcm_page = 9;
	uint32_t tdcm_page_size = 0;
	uint32_t write_count = 0;

	LOG_FUNCTION_START("");

	for (tdcm_page = 9; tdcm_page < 12; tdcm_page++) {
		status = _write_byte(p_dev, 0x7FFF, tdcm_page);
		if (status < STATUS_OK)
			goto exit;

		if (tdcm_page == 9)
			tdcm_page_size = 0x8000;
		if (tdcm_page == 10)
			tdcm_page_size = 0x8000;
		if (tdcm_page == 11)
			tdcm_page_size = 0x5000;

		for (tdcm_offset = 0; tdcm_offset < tdcm_page_size;
				tdcm_offset += WRITE_CHUNK_SIZE(p_dev)) {
			status = _write_page(
				p_dev, tdcm_offset, tdcm_page_size,
				WRITE_CHUNK_SIZE(p_dev), &write_count);
			if (status != STATUS_OK)
				goto exit;
		}
	}

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _wait_for_boot_complete_before_fw_load(
	struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	status = _enable_host_access_to_go1_async(p_dev);
	if (status < STATUS_OK)
		goto exit;

	status = _set_to_power_on_status(p_dev);
	if (status < STATUS_OK)
		goto exit;

	status = _wake_up_mcu(p_dev);
	if (status < STATUS_OK)
		goto exit;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _wait_for_boot_complete_after_fw_load(
	struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	status = _enable_host_access_to_go1_async(p_dev);
	if (status < STATUS_OK)
		goto exit;

	status = _reset_mcu_and_wait_boot(p_dev);
	if (status < STATUS_OK)
		goto exit;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

int32_t vl53l5_load_firmware(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");
	trace_print(VL53L5_TRACE_LEVEL_INFO, "\n\n#### load_firmware ####\n\n");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}
	if (VL53L5_FW_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_FW_BUFF_NOT_FOUND;
		goto exit;
	}
	if (VL53L5_FW_BUFF_ISEMPTY(p_dev)) {
		status = VL53L5_ERROR_FW_BUFF_NOT_FOUND;
		goto exit;
	}
	if (VL53L5_COMMS_BUFF_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	status = _wait_for_boot_complete_before_fw_load(p_dev);
	if (status < STATUS_OK)
		goto exit;

	status = _download_fw_to_ram(p_dev);
	if (status < STATUS_OK)
		goto exit;

	status = _wait_for_boot_complete_after_fw_load(p_dev);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x7FFF, 0x02);
	if (status < STATUS_OK)
		goto exit;

	status = _check_fw_checksum(p_dev);
	if (status < STATUS_OK)
		goto exit;
exit:
	if (status < STATUS_OK)

		(void)_write_byte(p_dev, 0x7FFF, 0x02);

	LOG_FUNCTION_END(status);
	return status;
}
