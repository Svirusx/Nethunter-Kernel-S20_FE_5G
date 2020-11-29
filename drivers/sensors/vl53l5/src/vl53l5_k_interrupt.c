
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
#include <linux/irq.h>
#include <linux/gpio.h>
#include "vl53l5_k_interrupt.h"
#include "vl53l5_k_driver_config.h"
#include "vl53l5_k_range_rotation.h"
#include "vl53l5_k_logging.h"
#include "vl53l5_k_range_wait_handler.h"
#include "vl53l5_k_error_converter.h"
#include "vl53l5_api_ranging.h"

static int _interrupt_get_range_data(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	disable_irq(p_module->gpio.interrupt);

	status = vl53l5_get_range_data(&p_module->stdev);

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (status != STATUS_OK)
		goto out;

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = vl53l5_decode_range_data(&p_module->stdev,
					  &p_module->range.data);

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (status != STATUS_OK)
		goto out;

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	p_module->range.count++;
	p_module->range.is_valid = 1;
	p_module->polling_count = 0;

	status = vl53l5_k_range_rotation(p_module->rotation_mode,
					p_module->range_filter,
					&p_module->range.data.core);

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (status != STATUS_OK)
		goto out;

out:
	if (status != STATUS_OK) {

		vl53l5_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		status = vl53l5_read_device_error(&p_module->stdev, status);

		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
		vl53l5_k_log_error("Failed: %d", status);
	}

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	vl53l5_k_store_error(p_module, status);

	enable_irq(p_module->gpio.interrupt);

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(status);

	return status;
}

static irqreturn_t irq_default_primary_handler(int irq, void *dev_id)
{
	return IRQ_WAKE_THREAD;
}

static irqreturn_t vl53l5_interrupt_handler(int irq, void *dev_id)
{
	int status = STATUS_OK;
	irqreturn_t irq_ret = IRQ_NONE;
	struct vl53l5_k_module_t *p_module = (struct vl53l5_k_module_t *)dev_id;

	status = _interrupt_get_range_data(p_module);
	if (status == STATUS_OK) {
		vl53l5_k_wake_up_wait_list(p_module);
		vl53l5_k_log_debug("Interrupt handled");
		irq_ret = IRQ_HANDLED;
	} else {

		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_store_error(p_module, status);
		vl53l5_k_log_debug("Interrupt not handled");
		irq_ret = IRQ_NONE;
	}

	return irq_ret;
}

int vl53l5_k_start_interrupt(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;
	const char *p_dev_name = p_module->name;

	LOG_FUNCTION_START("");

	if (p_module->gpio.interrupt < 0) {

		status = VL53L5_K_ERROR_DEVICE_INTERRUPT_NOT_OWNED;
		goto out;
	}
	p_module->irq_val = gpio_to_irq(p_module->gpio.interrupt);
	status = request_threaded_irq(p_module->irq_val,
				      irq_default_primary_handler,
				      vl53l5_interrupt_handler,
				      IRQF_TRIGGER_FALLING,
				      p_dev_name,
				      p_module);

	if (status) {

		vl53l5_k_log_error("Unable to assign IRQ: %d",
				   p_module->irq_val);
		goto out;
	} else {
		vl53l5_k_log_debug("IRQ %d now assigned",
				  p_module->irq_val);
		p_module->gpio.interrupt_owned = 1;
	}

	p_module->irq_is_running = true;

out:
	LOG_FUNCTION_END(status);
	return status;
}

int vl53l5_k_stop_interrupt(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	disable_irq(p_module->gpio.interrupt);

	free_irq(p_module->irq_val, p_module);
	vl53l5_k_log_debug("IRQ %d now free", p_module->irq_val);
	p_module->irq_val = 0;
	p_module->irq_is_running = false;

	LOG_FUNCTION_END(status);
	return status;
}
