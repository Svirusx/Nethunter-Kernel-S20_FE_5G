
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

#include "vl53l5_api_core.h"
#include "vl53l5_platform.h"
#include "vl53l5_tcdm_dump.h"

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L5_TRACE_MODULE_POWER_API, \
	level, VL53L5_TRACE_FUNCTION_ALL, ##__VA_ARGS__)

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L5_TRACE_MODULE_POWER_API, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L5_TRACE_MODULE_POWER_API, status, ##__VA_ARGS__)

#define STXP70_TEST_A0_8 0x06
#define PAGE_SELECT 0x7FFF
#define STXP70_CTRL 0x20
#define STXP70_STATUS 0x21
#define MCU_BYPASS 0x0C

#define UI_RAM_SIZE_BYTES  (12 * 1024)
#define PAGE_3_SIZE_BYTES  (32 * 1024)
#define PAGE_4_SIZE_BYTES  (32 * 1024)

#define GO1GO2_SECTIONS 6
uint16_t go1go2_registers[GO1GO2_SECTIONS][2] = {

	{0x0000, 0x0023},

	{0x0080, 0x0010},

	{0x0100, 0x0017},

	{0x0200, 0x00E8},

	{0x1000, 0x243F},

	{0x4000, 0x0B71}

};

#define CHECK_FOR_TIMEOUT(status, p_dev, start_ms, end_ms, timeout_ms) \
	(status = vl53l5_check_for_timeout(\
		(p_dev), start_ms, end_ms, timeout_ms))

#define VL53L5_BOOT_COMPLETION_POLLING_TIMEOUT_MS 500

static int32_t _write_byte(
	struct vl53l5_dev_handle_t *p_dev,
	uint16_t address, uint8_t value)
{
	return vl53l5_write_multi(p_dev, address, &value, 1);
}

static int32_t _read_byte(
	struct vl53l5_dev_handle_t *p_dev,
	uint16_t address, uint8_t *p_value)
{
	return vl53l5_read_multi(p_dev, address, p_value, 1);
}

static int32_t _set_page(
	struct vl53l5_dev_handle_t *p_dev, uint8_t page)
{
	int32_t status = STATUS_OK;

	status = _write_byte(p_dev, PAGE_SELECT, page);
	if (status < STATUS_OK) {
		goto exit;
	}

exit:
	return status;
}

static int32_t _read_page(
	struct vl53l5_dev_handle_t *p_dev,
	uint16_t page, uint32_t count, uint8_t *p_value)
{
	int32_t status = STATUS_OK;

	if (_set_page(p_dev, page) < STATUS_OK) {
		goto exit;
	}

	status = vl53l5_read_multi(p_dev, 0, p_value, count);
	if (status < STATUS_OK) {
		goto exit;
	}

exit:
	return status;
}

static int32_t host_go1_async_access(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;
	uint8_t m_status = 0;
	uint32_t start_time_ms   = 0;
	uint32_t current_time_ms = 0;

	LOG_FUNCTION_START("");

	if (_set_page(p_dev, 1) < STATUS_OK) {
		goto exit;
	}

	status = _write_byte(p_dev, STXP70_TEST_A0_8, 0x01);
	if (status < STATUS_OK) {
		goto exit;
	}

	m_status = 0;
	status = vl53l5_get_tick_count(p_dev, &start_time_ms);
	if (status < STATUS_OK)
		goto exit;

	while (m_status != 0x04) {
		status = _read_byte(p_dev, STXP70_STATUS, &m_status);
		if (status < STATUS_OK) {
			goto exit;
		}

		status = vl53l5_get_tick_count(p_dev, &current_time_ms);
		if (status < STATUS_OK)
			goto exit;

		CHECK_FOR_TIMEOUT(
			status, p_dev, start_time_ms, current_time_ms,
			VL53L5_BOOT_COMPLETION_POLLING_TIMEOUT_MS);
		if (status < STATUS_OK) {
			goto exit;
		}
	}

	if (_set_page(p_dev, 0) < STATUS_OK) {
		goto exit;
	}

	status = _read_byte(p_dev, PAGE_SELECT, &m_status);
	if (status < STATUS_OK) {
		goto exit;
	}

	status = _write_byte(p_dev, MCU_BYPASS, 0x01);
	if (status < STATUS_OK) {
		goto exit;
	}

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t mcu_go1_async_access(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;
	uint8_t m_status = 0;
	uint32_t start_time_ms   = 0;
	uint32_t current_time_ms = 0;

	LOG_FUNCTION_START("");

	if (_set_page(p_dev, 0) < STATUS_OK) {
		goto exit;
	}

	status = _write_byte(p_dev, MCU_BYPASS, 0x00);
	if (status < STATUS_OK) {
		goto exit;
	}

	if (_set_page(p_dev, 1) < STATUS_OK) {
		goto exit;
	}

	status = _write_byte(p_dev, STXP70_CTRL, 0x07);
	if (status < STATUS_OK) {
		goto exit;
	}

	status = _write_byte(p_dev, STXP70_CTRL, 0x06);
	if (status < STATUS_OK) {
		goto exit;
	}

	m_status = 0;
	status = vl53l5_get_tick_count(p_dev, &start_time_ms);
	if (status < STATUS_OK)
		goto exit;

	while (m_status != 0x00) {
		status = _read_byte(p_dev, STXP70_STATUS, &m_status);
		if (status < STATUS_OK) {
			goto exit;
		}

		status = vl53l5_get_tick_count(p_dev, &current_time_ms);
		if (status < STATUS_OK)
			goto exit;

		CHECK_FOR_TIMEOUT(
			status, p_dev, start_time_ms, current_time_ms,
			VL53L5_BOOT_COMPLETION_POLLING_TIMEOUT_MS);
		if (status < STATUS_OK) {
			goto exit;
		}
	}

exit:
	(void)_set_page(p_dev, 2);

	LOG_FUNCTION_END(status);
	return status;
}

static int32_t dump_tcdm_state(struct vl53l5_dev_handle_t *p_dev,
			       uint8_t *buffer,
			       uint32_t *pbytes_written)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	*pbytes_written = 0;

	status = _read_page(p_dev, 3, PAGE_3_SIZE_BYTES, buffer);
	if (status < STATUS_OK) {
		goto exit;
	}
	*pbytes_written += PAGE_3_SIZE_BYTES;

	status = _read_page(p_dev, 4, PAGE_4_SIZE_BYTES, buffer + PAGE_3_SIZE_BYTES);
	if (status < STATUS_OK) {
		goto exit;
	}
	*pbytes_written += PAGE_4_SIZE_BYTES;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

int32_t vl53l5_dump_tcdm(struct vl53l5_dev_handle_t *p_dev, uint8_t *buffer, uint32_t *count)
{
	int32_t status = STATUS_OK;
	uint32_t bytes_written = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	status = host_go1_async_access(p_dev);
	if (status < STATUS_OK)
		goto exit;

	status = dump_tcdm_state(p_dev, p_buff, &bytes_written);
	if (status < STATUS_OK)
		goto exit;
	*count += bytes_written;

	status = mcu_go1_async_access(p_dev);
	if (status < STATUS_OK)
		goto exit;

exit:
	LOG_FUNCTION_END(status);
	return status;
}


#ifdef STM_VL53L5_SUPPORT_SEC_CODE
void _tcdm_dump(struct vl53l5_k_module_t *p_module)
{
#ifdef VL53L5_TCDM_DUMP
	int status = STATUS_OK;
	char *p_data;
	int count = 0;
	int buff_size = 100000 * sizeof(char);
	char filepath[100] ={0,};
	
	LOG_FUNCTION_START("");

	sprintf(filepath,"/data/log/vl53l5_tcdm_%x.bin",p_module->last_driver_error);
	vl53l5_k_log_info("filepath: %s",filepath);
	p_data = kzalloc(buff_size, GFP_KERNEL);
	if (p_data == NULL) {
		vl53l5_k_log_error("Allocate failed");
		goto out;
	}

	status = vl53l5_dump_tcdm(&p_module->stdev, p_data, &count);

	if (status < STATUS_OK)
		goto out_free;

	status = vl53l5_write_file(&p_module->stdev,
				p_data,
				count,
				filepath,
				NULL,
				0);

	if (status < STATUS_OK) {
		vl53l5_k_log_error(
				"write file %s failed: %d ",
				filepath, status);
		goto out_free;
	}
out_free:
	kfree(p_data);
out:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("failed %i", status);
	}
	LOG_FUNCTION_END(status);
#endif
}
#endif //STM_VL53L5_SUPPORT_SEC_CODE