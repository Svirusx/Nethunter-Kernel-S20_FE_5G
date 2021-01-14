
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

#include <linux/gpio.h>
#include "vl53l5_k_gpio_utils.h"
#include "vl53l5_k_logging.h"
#include "vl53l5_k_error_codes.h"

#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
static int get_power_enable(struct vl53l5_k_module_t *p_module)
{
	return vl53l5_k_get_gpio(&p_module->gpio.power_enable,
			&p_module->gpio.power_enable_owned, 1);
}

static void put_power_enable(struct vl53l5_k_module_t *p_module)
{
	vl53l5_k_put_gpio(&p_module->gpio.power_enable,
		 &p_module->gpio.power_enable_owned);
}

static int get_low_power(struct vl53l5_k_module_t *p_module)
{
	return vl53l5_k_get_gpio(&p_module->gpio.low_power,
			&p_module->gpio.low_power_owned, 1);
}

static void put_low_power(struct vl53l5_k_module_t *p_module)
{
	vl53l5_k_put_gpio(&p_module->gpio.low_power,
		 &p_module->gpio.low_power_owned);
}

static int get_comms_select(struct vl53l5_k_module_t *p_module)
{
	return vl53l5_k_get_gpio(&p_module->gpio.comms_select,
			&p_module->gpio.comms_select_owned, 1);
}

static void put_comms_select(struct vl53l5_k_module_t *p_module)
{
	vl53l5_k_put_gpio(&p_module->gpio.comms_select,
		 &p_module->gpio.comms_select_owned);
}
#endif
static int get_interrupt(struct vl53l5_k_module_t *p_module)
{
	return vl53l5_k_get_gpio(&p_module->gpio.interrupt,
			&p_module->gpio.interrupt_owned, 0);
}

static void put_interrupt(struct vl53l5_k_module_t *p_module)
{
	vl53l5_k_put_gpio(&p_module->gpio.interrupt,
		 &p_module->gpio.interrupt_owned);
}

int vl53l5_k_get_gpio(int *p_gpio, int *p_is_gpio_owned, int is_output)
{
	int status = 0;

	*p_is_gpio_owned = 0;
	if (*p_gpio == -1) {
		vl53l5_k_log_warning("Gpio is disabled");
		status = VL53L5_K_ERROR_GPIO_IS_DISABLED;
		goto no_gpio;
	}

	vl53l5_k_log_debug("Requesting gpio: %d", *p_gpio);
	status = gpio_request(*p_gpio, "vl53l5_k_pwren");
	if (status) {
		vl53l5_k_log_error("Request gpio failed gpio %d: %d",
				   *p_gpio, status);
		status = VL53L5_K_ERROR_GPIO_REQUEST_FAILED;
		goto request_failed;
	}

	if (is_output) {
		vl53l5_k_log_debug(
			"Setting gpio %d direction to output", *p_gpio);
		status = gpio_direction_output(*p_gpio, 0);
	} else {
		vl53l5_k_log_debug(
			"Setting gpio %d direction to input", *p_gpio);
		status = gpio_direction_input(*p_gpio);
	}

	if (status) {
		vl53l5_k_log_error("Set output direction failed gpio %d: %d",
				   *p_gpio, status);
		status = VL53L5_K_ERROR_GPIO_DIRECTION_SET_FAILED;
		goto direction_failed;
	}
	*p_is_gpio_owned = 1;

	return status;

direction_failed:
	gpio_free(*p_gpio);

request_failed:
no_gpio:
	return status;
}

void vl53l5_k_put_gpio(int *p_gpio, int *p_is_gpio_owned)
{
	if (*p_is_gpio_owned) {
		vl53l5_k_log_debug("Releasing gpio %d", *p_gpio);
		gpio_free(*p_gpio);
		*p_is_gpio_owned = 0;
	}
}

int vl53l5_k_set_gpio(int *p_gpio, uint8_t value)
{
	int status = STATUS_OK;
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	if (*p_gpio != -1) {
		gpio_set_value(*p_gpio, value);
	} else {
		vl53l5_k_log_error("Failed. Gpio not available");
		status = VL53L5_K_ERROR_ATTEMPT_TO_SET_DISABLED_GPIO;
	}
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	if (*p_gpio >= 0) {
		gpio_set_value(*p_gpio, value);
	} else {
		vl53l5_k_log_error("Failed. Gpio not available");
		status = STATUS_OK;
	}
#endif
	return status;
}

int vl53l5_k_assign_gpios(struct vl53l5_k_module_t *p_module)
{
	int status = 0;

	LOG_FUNCTION_START("");
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	if (p_module->gpio.power_enable != -1) {
		vl53l5_k_log_debug("Get power enable");
		status = get_power_enable(p_module);
		if (status) {
			vl53l5_k_log_debug("Failed to get power enable");
			goto no_pwren;
		}
	}

	if (p_module->gpio.low_power != -1) {
		vl53l5_k_log_debug("Get low power");
		status = get_low_power(p_module);
		if (status) {
			vl53l5_k_log_debug("Failed to get low power");
			goto no_lpn;
		}
	}

	if (p_module->gpio.comms_select != -1) {
		vl53l5_k_log_debug("Get comms select");
		status = get_comms_select(p_module);
		if (status) {
			vl53l5_k_log_debug("Failed to get comms select");
			goto no_csel;
		}
	}
#endif
	if (p_module->gpio.interrupt != -1) {
		vl53l5_k_log_debug("Get interrupt");
		status = get_interrupt(p_module);
		if (status) {
			vl53l5_k_log_debug("Failed to get interrupt");
			goto no_int;
		}
	}
	LOG_FUNCTION_END(status);
	return status;

no_int:
	put_interrupt(p_module);
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
no_csel:
	put_comms_select(p_module);
no_lpn:
	put_low_power(p_module);
no_pwren:
	put_power_enable(p_module);
#endif
	LOG_FUNCTION_END(status);
	return status;
}

void vl53l5_k_release_gpios(struct vl53l5_k_module_t *p_module)
{
	LOG_FUNCTION_START("");
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	vl53l5_k_log_debug("Release power enable");
	put_power_enable(p_module);

	vl53l5_k_log_debug("Release low power");
	put_low_power(p_module);

	vl53l5_k_log_debug("Release comms select");
	put_comms_select(p_module);
#endif
	vl53l5_k_log_debug("Release interrupt");
	put_interrupt(p_module);

	LOG_FUNCTION_END(0);
}
