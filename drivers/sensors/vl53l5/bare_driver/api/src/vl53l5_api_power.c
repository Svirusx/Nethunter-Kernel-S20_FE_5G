
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

#include "vl53l5_api_power.h"
#include "vl53l5_api_core.h"
#include "vl53l5_checks.h"
#include "vl53l5_register_utils.h"
#include "vl53l5_load_firmware.h"
#include "vl53l5_globals.h"
#include "vl53l5_trans.h"
#include "vl53l5_dci_core.h"
#include "vl53l5_dci_utils.h"
#include "vl53l5_platform.h"
#include "vl53l5_platform_log.h"
#include "vl53l5_platform_user_config.h"
#include "vl53l5_error_codes.h"

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L5_TRACE_MODULE_API, \
	level, VL53L5_TRACE_FUNCTION_ALL, ##__VA_ARGS__)
#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L5_TRACE_MODULE_API, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L5_TRACE_MODULE_API, status, ##__VA_ARGS__)
#define LOG_FUNCTION_END_FMT(status, fmt, ...) \
	_LOG_FUNCTION_END_FMT(VL53L5_TRACE_MODULE_API, status, fmt,\
			      ##__VA_ARGS__)
#define LOG_FUNCTION_END_FLUSH(status, ...) \
	do { \
	_LOG_FUNCTION_END(VL53L5_TRACE_MODULE_API, status, ##__VA_ARGS__);\
	_FLUSH_TRACE_TO_OUTPUT();\
	} while (0)

#define GPIO_LOW 0
#define GPIO_HIGH 1

#define REGULATOR_DISABLE 0
#define REGULATOR_ENABLE 1

#define DEFAULT_PAGE 2
#define GO2_PAGE 0

static int32_t _set_power_to_hp_idle(struct vl53l5_dev_handle_t *p_dev);

static int32_t _set_power_to_lp_idle_comms(struct vl53l5_dev_handle_t *p_dev);

static int32_t _set_power_to_ulp_idle(struct vl53l5_dev_handle_t *p_dev);

static int32_t _go_to_hp_idle(struct vl53l5_dev_handle_t *p_dev);

static int32_t _go_to_lp_idle_comms(struct vl53l5_dev_handle_t *p_dev);

static int32_t _go_to_ulp_idle(struct vl53l5_dev_handle_t *p_dev);

int32_t vl53l5_set_power_mode(
	struct vl53l5_dev_handle_t *p_dev,
	enum vl53l5_power_states power_mode)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	if (VL53L5_ISNULL(p_dev)) {
		status = VL53L5_ERROR_INVALID_PARAMS;
		goto exit;
	}

	if ((VL53L5_CHECK_POWER_STATE_OFF(p_dev) &&
			(MCU_FIRST_BOOT_NOT_COMPLETE(p_dev))) ||
			VL53L5_CHECK_POWER_STATE_RANGING(p_dev)) {
		status = VL53L5_ERROR_POWER_STATE;
		goto exit;
	}

	switch (power_mode) {
	case VL53L5_POWER_STATE_HP_IDLE:
		if (p_dev->host_dev.power_state
				!= VL53L5_POWER_STATE_HP_IDLE)
			status = _set_power_to_hp_idle(p_dev);
		break;
	case VL53L5_POWER_STATE_LP_IDLE_COMMS:
		if (p_dev->host_dev.power_state
				!= VL53L5_POWER_STATE_LP_IDLE_COMMS)
			status = _set_power_to_lp_idle_comms(p_dev);
		break;

	case VL53L5_POWER_STATE_ULP_IDLE:
		if (p_dev->host_dev.power_state
				!= VL53L5_POWER_STATE_ULP_IDLE)
			status = _set_power_to_ulp_idle(p_dev);
		break;
	case VL53L5_POWER_STATE_OFF:

	case VL53L5_POWER_STATE_RANGING:

	default:
		status = VL53L5_ERROR_POWER_STATE;
		break;
	}

exit:
	LOG_FUNCTION_END_FLUSH(status);
	return status;
}

static int32_t _go_to_hp_idle(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	if (VL53L5_CHECK_POWER_STATE_ULP_IDLE(p_dev)) {

		status = vl53l5_set_manual_xshut_state(p_dev, GPIO_HIGH);
		if (status < STATUS_OK)
			goto exit;

		status = vl53l5_set_regulators(p_dev, REGULATOR_ENABLE,
					 REGULATOR_ENABLE);
		if (status < STATUS_OK)
			goto exit;

	} else if (VL53L5_CHECK_POWER_STATE_LP_IDLE_COMMS(p_dev)) {

		status = vl53l5_set_manual_xshut_state(p_dev, GPIO_HIGH);
		if (status < STATUS_OK)
			goto exit;
	} else if (VL53L5_CHECK_POWER_STATE_OFF(p_dev) &&
		   p_dev->host_dev.device_booted == true) {

		status = vl53l5_set_manual_xshut_state(p_dev, GPIO_HIGH);
		if (status < STATUS_OK)
			goto exit;
	} else {
		status = VL53L5_ERROR_POWER_STATE;
		goto exit;
	}

	status = vl53l5_wait_mcu_boot(p_dev, VL53L5_BOOT_STATE_HIGH,
				      0, VL53L5_HP_IDLE_WAIT_DELAY);
	if (status < STATUS_OK)
		goto exit;

	p_dev->host_dev.power_state = VL53L5_POWER_STATE_HP_IDLE;

exit:
	return status;
}

static int32_t _go_to_lp_idle_comms(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	status = vl53l5_set_manual_xshut_state(p_dev, GPIO_LOW);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_wait_mcu_boot(p_dev, VL53L5_BOOT_STATE_LOW,
				      0, VL53L5_LP_IDLE_WAIT_DELAY);
	if (status < STATUS_OK)
		goto exit;

	p_dev->host_dev.power_state = VL53L5_POWER_STATE_LP_IDLE_COMMS;

exit:
	return status;
}

static int32_t _go_to_ulp_idle(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	status = _go_to_lp_idle_comms(p_dev);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_set_regulators(p_dev, REGULATOR_DISABLE,
				       REGULATOR_DISABLE);
	if (status < STATUS_OK)
		goto exit;

	p_dev->host_dev.version_match.map_version_match = false;

	p_dev->host_dev.power_state = VL53L5_POWER_STATE_ULP_IDLE;

exit:
	return status;
}

static int32_t _set_power_to_hp_idle(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;
	enum vl53l5_power_states current_state = p_dev->host_dev.power_state;

	bool comms_on = true;

	if (VL53L5_CHECK_POWER_STATE_HP_IDLE(p_dev))
		goto exit;

	if (comms_on) {

		status = vl53l5_set_page(p_dev, GO2_PAGE);
		if (status < STATUS_OK)
			goto exit_page_changed;
	}

	status = _go_to_hp_idle(p_dev);

exit_page_changed:

	if (status != STATUS_OK) {
		(void)vl53l5_set_page(p_dev, DEFAULT_PAGE);
		goto exit;
	} else {
		status = vl53l5_set_page(p_dev, DEFAULT_PAGE);
		if (status != STATUS_OK)
			goto exit;
	}

	if (current_state == VL53L5_POWER_STATE_ULP_IDLE) {
		if (!VL53L5_FW_BUFF_ISNULL(p_dev)) {
			status = vl53l5_load_firmware(p_dev);
			if (status != STATUS_OK)
				goto exit;
		}
	}
	if (current_state == VL53L5_POWER_STATE_ULP_IDLE ||
		p_dev->host_dev.device_booted == true) {
		status = vl53l5_check_map_version(p_dev);
		if (status != STATUS_OK)
			goto exit;
	}

exit:
	return status;
}

static int32_t _set_power_to_lp_idle_comms(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;
	enum vl53l5_power_states current_state = p_dev->host_dev.power_state;

	bool comms_on = true;

	if (VL53L5_CHECK_POWER_STATE_LP_IDLE_COMMS(p_dev))
		goto exit;

	if (current_state == VL53L5_POWER_STATE_ULP_IDLE) {
		status = _set_power_to_hp_idle(p_dev);
		if (status < STATUS_OK)
			goto exit;
	}

	if (comms_on) {

		status = vl53l5_set_page(p_dev, GO2_PAGE);
		if (status < STATUS_OK)
			goto exit;
	}
	if (!VL53L5_CHECK_POWER_STATE_HP_IDLE(p_dev)) {

		status = _go_to_hp_idle(p_dev);
		if (status < STATUS_OK)
			goto exit_page_changed;

	}

	status = _go_to_lp_idle_comms(p_dev);

exit_page_changed:

	if (status < STATUS_OK)
		(void)vl53l5_set_page(p_dev, DEFAULT_PAGE);
	else
		status = vl53l5_set_page(p_dev, DEFAULT_PAGE);

exit:
	return status;
}

static int32_t _set_power_to_ulp_idle(struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = VL53L5_ERROR_NONE;

	bool comms_on = true;

	if (VL53L5_CHECK_POWER_STATE_ULP_IDLE(p_dev))
		goto exit;

	if (comms_on) {

		status = vl53l5_set_page(p_dev, GO2_PAGE);
		if (status < STATUS_OK)
			goto exit;
	}

	if (!VL53L5_CHECK_POWER_STATE_HP_IDLE(p_dev)) {

		status = _go_to_hp_idle(p_dev);
		if (status < STATUS_OK)
			goto exit_page_changed;
	}

	status = _go_to_ulp_idle(p_dev);
	if (status < STATUS_OK)
		goto exit_page_changed;

exit_page_changed:

	if (status < STATUS_OK)
		(void)vl53l5_set_page(p_dev, DEFAULT_PAGE);
	else
		status = vl53l5_set_page(p_dev, DEFAULT_PAGE);

exit:
	return status;
}
