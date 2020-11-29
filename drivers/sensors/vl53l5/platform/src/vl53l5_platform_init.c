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
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
/**
 * @file   vl53l5_platform_init.c
 * @brief  Customer implemented platform interface
 *
 */

#include "vl53l5_platform_init.h"
#include "vl53l5_platform.h"
#include "vl53l5_k_user_data.h"
#include "vl53l5_k_module_data.h"


int32_t vl53l5_platform_init(struct vl53l5_dev_handle_t *pdev)
{
	int32_t status = STATUS_OK;
	struct vl53l5_k_module_t *p_module;

	p_module = (struct vl53l5_k_module_t *)
		container_of(pdev, struct vl53l5_k_module_t, stdev);

	status = vl53l5_gpio_low_power_control(pdev, GPIO_LOW);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_gpio_power_enable(pdev, GPIO_LOW);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_gpio_comms_select(pdev, GPIO_LOW);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_wait_us(pdev, 1000);
	if (status < STATUS_OK)
		goto exit;

	if (p_module->comms_type == VL53L5_K_COMMS_TYPE_SPI) {
		status = vl53l5_gpio_comms_select(pdev, GPIO_HIGH);
		if (status < VL53L5_ERROR_NONE)
			goto exit;
	}

	status = vl53l5_gpio_low_power_control(pdev, GPIO_HIGH);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_gpio_power_enable(pdev, GPIO_HIGH);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_wait_us(pdev, 200);
	if (status < STATUS_OK)
		goto exit;

exit:
	return status;
}

int32_t vl53l5_platform_terminate(struct vl53l5_dev_handle_t *pdev)
{
	int32_t status = STATUS_OK;

	status = vl53l5_gpio_comms_select(pdev, GPIO_LOW);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_gpio_low_power_control(pdev, GPIO_LOW);
	if (status < STATUS_OK)
		goto exit;

	status = vl53l5_gpio_power_enable(pdev, GPIO_LOW);
	if (status < STATUS_OK)
		goto exit;

exit:
	return status;
}
#endif