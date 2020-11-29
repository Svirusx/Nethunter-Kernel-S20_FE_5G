
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

#include "vl53l5_rom_boot.h"
#include "vl53l5_error_codes.h"
#include "vl53l5_platform.h"
#include "vl53l5_register_utils.h"

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L5_TRACE_MODULE_POWER_API, \
	level, VL53L5_TRACE_FUNCTION_ALL, ##__VA_ARGS__)

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L5_TRACE_MODULE_POWER_API, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L5_TRACE_MODULE_POWER_API, status, ##__VA_ARGS__)

#define CHECK_FOR_TIMEOUT(status, p_dev, start_ms, end_ms, timeout_ms) \
	(status = vl53l5_check_for_timeout(\
		(p_dev), start_ms, end_ms, timeout_ms))

#define DEFAULT_PAGE 2
#define GO2_PAGE 0

static int32_t _write_byte(
	struct vl53l5_dev_handle_t *p_dev, uint16_t address, uint8_t value)
{
	return vl53l5_write_multi(p_dev, address, &value, 1);
}

static int32_t _check_rom_firmware_boot_cut_1_2(
	struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	status = vl53l5_wait_mcu_boot(p_dev, VL53L5_BOOT_STATE_HIGH, 0, 0);
	if (status < STATUS_OK)
		goto exit;

	status = _write_byte(p_dev, 0x000E, 0x01);

exit:
	return status;
}

int32_t vl53l5_check_rom_firmware_boot(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;
	uint8_t device_id = 0;
	uint8_t revision_id = 0;

	LOG_FUNCTION_START("");

	status = vl53l5_set_page(p_dev, GO2_PAGE);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_read_multi(p_dev, 0x00, &device_id, 1);
	if (status < STATUS_OK)
		goto exit_change_page;

	status = vl53l5_read_multi(p_dev, 0x01, &revision_id, 1);
	if (status < STATUS_OK)
		goto exit_change_page;

	trace_print(VL53L5_TRACE_LEVEL_DEBUG,
		"device_id (0x%02X), revision_id (0x%02X)\n", device_id,
		revision_id);
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	vl53l5_k_log_info("device_id (0x%02X), revision_id (0x%02X)\n", device_id, revision_id);
	p_dev->id_device = device_id;
	p_dev->id_revision = revision_id;
#endif
	if ((device_id == 0xF0) && (revision_id == 0x02)) {
		trace_print(VL53L5_TRACE_LEVEL_INFO, "Silicon cut 1.2\n");
		trace_print(VL53L5_TRACE_LEVEL_DEBUG,
			    "Running cut 1.2 rom boot check\n");
		status = _check_rom_firmware_boot_cut_1_2(p_dev);
		if (status < STATUS_OK)
			goto exit_change_page;
	} else {
		trace_print(VL53L5_TRACE_LEVEL_ERRORS,
			    "Unsupported device type\n");

		status = VL53L5_UNKNOWN_SILICON_REVISION;
		goto exit_change_page;
	}

exit_change_page:

	if (status != STATUS_OK)
		(void)vl53l5_set_page(p_dev, DEFAULT_PAGE);
	else
		status = vl53l5_set_page(p_dev, DEFAULT_PAGE);

exit:
	LOG_FUNCTION_END(status);
	return status;
}
