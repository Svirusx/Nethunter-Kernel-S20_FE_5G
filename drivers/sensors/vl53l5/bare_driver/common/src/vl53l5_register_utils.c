
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

#include "vl53l5_register_utils.h"
#include "vl53l5_rom_boot.h"
#include "vl53l5_error_codes.h"
#include "vl53l5_platform.h"

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

#define PAGE_SELECT 0x7FFF

#define GO2_STATUS_0 0x6
#define GO2_STATUS_1 0x7

#define GPIO_LOW 0
#define GPIO_HIGH 1

#define REGULATOR_DISABLE 0
#define REGULATOR_ENABLE 1

#define REGDVDD1V1__INDEX 0x000F
#define XSHUT_CTRL 0x0009

#define DEFAULT_PAGE 2
#define GO2_PAGE 0

#define REGULATOR_REGISTER_OR_MASK(return_mask, lp_reg_enable, hp_reg_enable)\
do {\
	return_mask = 0;\
	return_mask |= (lp_reg_enable ? 0x00 : 0x02);\
	return_mask |= (hp_reg_enable ? 0x00 : 0x01);\
} while (0)

#define REGULATOR_REGISTER_AND_MASK(return_mask)\
	(return_mask = 0xfc)

#define MASK_XSHUT_REGISTER(reg_val, value)\
do {\
	reg_val &= 0xf8;\
	reg_val |= (value ? 0x04 : 0x02);\
} while (0)

#define BOOT_STATE_MATCHED(p_dev, state) \
	(((state == VL53L5_BOOT_STATE_HIGH) && MCU_BOOT_COMPLETE(p_dev)) || \
	((state == VL53L5_BOOT_STATE_LOW) && MCU_BOOT_NOT_COMPLETE(p_dev)))

int32_t vl53l5_register_read_modify_write(
	struct vl53l5_dev_handle_t *p_dev, uint16_t addr,
	uint8_t first_and_mask, uint8_t second_or_mask)
{
	int32_t status = STATUS_OK;
	uint8_t reg_val = 0;

	status = vl53l5_read_multi(p_dev, addr, &reg_val, 1);
	if (status != STATUS_OK)
		goto out;

	reg_val &= first_and_mask;
	reg_val |= second_or_mask;

	status = vl53l5_write_multi(p_dev, addr, &reg_val, 1);
out:
	return status;
}

int32_t vl53l5_set_page(struct vl53l5_dev_handle_t *p_dev, uint8_t page)
{
	int32_t status = STATUS_OK;

	status = vl53l5_write_multi(p_dev, PAGE_SELECT, &page, 1);

	return status;
}

int32_t vl53l5_set_regulators(
	struct vl53l5_dev_handle_t *p_dev,
	uint8_t lp_reg_enable,
	uint8_t hp_reg_enable)
{
	int32_t status = STATUS_OK;
	uint16_t reg_index = 0;
	uint8_t and_mask = 0;
	uint8_t or_mask = 0;

	REGULATOR_REGISTER_AND_MASK(and_mask);

	REGULATOR_REGISTER_OR_MASK(or_mask, lp_reg_enable, hp_reg_enable);

	reg_index = REGDVDD1V1__INDEX;
	status = vl53l5_register_read_modify_write(
		p_dev, reg_index, and_mask, or_mask);

	return status;
}

int32_t vl53l5_set_xshut_bypass(
	struct vl53l5_dev_handle_t *p_dev, uint8_t state)
{
	int32_t status = STATUS_OK;
	uint16_t reg_index = 0;
	uint8_t and_mask = 0;
	uint8_t or_mask = 0;

	reg_index = 0x09;

	if (state == 1) {
		and_mask = 0xff;
		or_mask = 0x01;
	} else {
		and_mask = 0xfe;
		or_mask = 0x00;
	}

	status = vl53l5_register_read_modify_write(
		p_dev, reg_index, and_mask, or_mask);

	return status;
}

int32_t vl53l5_set_manual_xshut_state(
	struct vl53l5_dev_handle_t *p_dev, uint8_t state)
{
	int32_t status = STATUS_OK;
	uint16_t reg_index = 0;
	uint8_t reg_val = 0;

	reg_index = XSHUT_CTRL;
	reg_val = 0;
	status = vl53l5_read_multi(p_dev, reg_index, &reg_val, 1);
	if (status != STATUS_OK)
		goto exit;

	MASK_XSHUT_REGISTER(reg_val, state);

	status = vl53l5_write_multi(p_dev, reg_index, &reg_val, 1);

exit:
	return status;
}

int32_t vl53l5_wait_mcu_boot(
	struct vl53l5_dev_handle_t *p_dev, enum vl53l5_boot_state state,
	uint32_t timeout_ms, uint32_t wait_time_ms)
{
	int32_t status = STATUS_OK;
	uint32_t start_time_ms   = 0;
	uint32_t current_time_ms = 0;

	status = vl53l5_get_tick_count(p_dev, &start_time_ms);
	if (status != STATUS_OK)
		goto exit;

	if (timeout_ms == 0)
		timeout_ms = VL53L5_BOOT_COMPLETION_POLLING_TIMEOUT_MS;

	if (wait_time_ms > timeout_ms)
		wait_time_ms = timeout_ms;

	VL53L5_GO2_STATUS_0(p_dev).bytes = 0;
	VL53L5_GO2_STATUS_1(p_dev).bytes = 0;

	do {

		status = vl53l5_read_multi(p_dev, GO2_STATUS_0,
			&VL53L5_GO2_STATUS_0(p_dev).bytes, 1);
		if (status != STATUS_OK)
			goto exit;

		if (HW_TRAP(p_dev)) {

			status = vl53l5_read_multi(p_dev, GO2_STATUS_1,
					&VL53L5_GO2_STATUS_1(p_dev).bytes, 1);
			if (status != STATUS_OK)
				goto exit;

			if (VL53L5_GO2_STATUS_1(p_dev).bytes)
				status = VL53L5_ERROR_MCU_ERROR_HW_STATE;
			else

				status =
				    VL53L5_ERROR_FALSE_MCU_ERROR_POWER_STATE;
			goto exit;
		}

		if (state == VL53L5_BOOT_STATE_ERROR) {
			if (MCU_ERROR(p_dev)) {
				status = VL53L5_ERROR_MCU_ERROR_WAIT_STATE;
				goto exit;
			}
		} else {
			if (BOOT_STATE_MATCHED(p_dev, state))
				goto exit_error;
		}

		status = vl53l5_get_tick_count(p_dev, &current_time_ms);
		if (status != STATUS_OK)
			goto exit;

		CHECK_FOR_TIMEOUT(
			status, p_dev, start_time_ms, current_time_ms,
			timeout_ms);

		if (status != STATUS_OK) {
			status = VL53L5_ERROR_BOOT_COMPLETE_TIMEOUT;
			goto exit;
		}

		if (wait_time_ms) {
			status = vl53l5_wait_ms(p_dev, wait_time_ms);
			if (status != STATUS_OK)
				goto exit;
		}
	} while (1);

exit_error:
	if (MCU_ERROR(p_dev)) {
		(void)vl53l5_read_multi(p_dev, GO2_STATUS_1,
					&VL53L5_GO2_STATUS_1(p_dev).bytes, 1);
		if (MCU_NVM_PROGRAMMED(p_dev))
			status = VL53L5_ERROR_MCU_NVM_NOT_PROGRAMMED;
		else
			status = VL53L5_ERROR_MCU_ERROR_WAIT_STATE;
	}

exit:
	return status;
}

int32_t vl53l5_check_device_booted(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	VL53L5_GO2_STATUS_1(p_dev).bytes = 0;

	status = vl53l5_set_page(p_dev, GO2_PAGE);
	if (status != STATUS_OK)
		goto exit;

	status = vl53l5_read_multi(p_dev, GO2_STATUS_1,
			&VL53L5_GO2_STATUS_1(p_dev).bytes, 1);
	if (status != STATUS_OK)
		goto exit_page;

	if (MCU_FIRST_BOOT_COMPLETE(p_dev))
		p_dev->host_dev.device_booted = true;
	else
		p_dev->host_dev.device_booted = false;

exit_page:
	(void)vl53l5_set_page(p_dev, DEFAULT_PAGE);
exit:
	return status;
}
