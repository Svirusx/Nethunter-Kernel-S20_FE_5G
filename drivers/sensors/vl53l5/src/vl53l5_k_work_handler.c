
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

#include <linux/slab.h>
#include "vl53l5_k_work_handler.h"
#include "vl53l5_k_logging.h"
#include "vl53l5_k_driver_config.h"
#include "vl53l5_k_module_data.h"
#include "vl53l5_k_error_codes.h"
#include "vl53l5_k_error_converter.h"
#include "vl53l5_k_range_rotation.h"
#include "vl53l5_k_range_wait_handler.h"
#include "vl53l5_platform.h"
#include "vl53l5_api_ranging.h"
#include "vl53l5_api_core.h"

static int get_new_range_data(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	vl53l5_k_log_debug("Get data");

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	status = vl53l5_get_range_data(&p_module->stdev);

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (status == VL53L5_NO_NEW_RANGE_DATA_ERROR)
		status = STATUS_OK;
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
	p_module->range.is_valid = 1;
	p_module->range.count++;
	status = vl53l5_k_range_rotation(p_module->rotation_mode,
					 p_module->range_filter,
					 &p_module->range.data.core);

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (status != STATUS_OK)
		goto out;

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	vl53l5_k_wake_up_wait_list(p_module);

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);

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
	mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(status);
	return status;
}

static int run_range_loop(struct vl53l5_k_module_t *p_module,
			  int *p_data_ready)
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	if (p_module->state_preset != VL53L5_STATE_RANGING) {
		status = VL53L5_K_ERROR_DEVICE_NOT_RANGING;
		goto out;
	}

	if (!p_module->worker_is_running) {
		goto out;
	}

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = vl53l5_k_check_data_ready(p_module, p_data_ready);

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (status < STATUS_OK)
		goto out_killworker;

	if (*p_data_ready) {
		status = get_new_range_data(p_module);
		if (status < STATUS_OK)
			goto out_killworker;
		goto out;
	}

	status = vl53l5_k_check_for_timeout(p_module, VL53L5_MAX_POLL_TIME_MS,
					    &p_module->timeout_occurred);
	if (p_module->timeout_occurred) {

		if (status == VL53L5_ERROR_TIME_OUT)
			status = VL53L5_K_ERROR_WORKER_THREAD_TIMEOUT;
		goto out_killworker;
	}
	if (status != STATUS_OK)

		goto out_killworker;

out:
	if (status != STATUS_OK) {

		vl53l5_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		status = vl53l5_read_device_error(&p_module->stdev, status);

		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
		vl53l5_k_log_error("Failed: %d", status);
	}
	vl53l5_k_store_error(p_module, status);
	LOG_FUNCTION_END(status);
	return status;

out_killworker:

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	(void)vl53l5_k_cancel_worker(p_module);

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);

	if (status != STATUS_OK) {

		vl53l5_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		status = vl53l5_read_device_error(&p_module->stdev, status);

		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
		vl53l5_k_log_error("Failed: %d", status);
	}
	vl53l5_k_store_error(p_module, status);
	LOG_FUNCTION_END(status);
	return status;
}

static int schedule_worker(struct vl53l5_k_module_t *p_module, int is_new_start)
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	if (is_new_start)
		vl53l5_get_tick_count(
			&p_module->stdev, &p_module->polling_start_time_ms);

	status = schedule_delayed_work(&p_module->dwork,
		msecs_to_jiffies(p_module->polling_sleep_time_ms));

	LOG_FUNCTION_END(status);
	return status;
}

static void work_handler(struct work_struct *work)
{
	int status = 0;
	int got_new_data = 0;
	struct vl53l5_k_module_t *p_module;

	LOG_FUNCTION_START("");

	p_module = container_of(work, struct vl53l5_k_module_t,
				dwork.work);

	status = run_range_loop(p_module, &got_new_data);
	if (status != 0)

		vl53l5_k_store_error(p_module, status);

	if ((p_module->range_mode == VL53L5_RANGE_SERVICE_WORKER) &&
			(p_module->worker_is_running) && (status == 0)) {

		(void)schedule_worker(p_module, got_new_data);
	}

	LOG_FUNCTION_END(0);
}

int vl53l5_k_init_worker(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	INIT_DELAYED_WORK(&p_module->dwork, work_handler);

	LOG_FUNCTION_END(status);
	return status;
}

int vl53l5_k_start_worker(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	(void)schedule_worker(p_module, 1);

	p_module->worker_is_running = 1;
	p_module->timeout_occurred = 0;

	LOG_FUNCTION_END(status);
	return status;
}

int vl53l5_k_cancel_worker(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	vl53l5_k_wake_up_wait_list(p_module);

	vl53l5_k_log_debug("Cancelling delayed work");
	(void)cancel_delayed_work(&p_module->dwork);

	p_module->worker_is_running = 0;

	LOG_FUNCTION_END(status);
	return status;
}
