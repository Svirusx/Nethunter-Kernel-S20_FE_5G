
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
#include "vl53l5_k_module_data.h"
#include "vl53l5_k_error_codes.h"
#include "vl53l5_k_logging.h"
#include "vl53l5_k_range_wait_handler.h"
#include "vl53l5_platform.h"

#if defined(VL53L5_INTERRUPT) || defined(VL53L5_WORKER)
struct vl53l5_k_wait_list {
	struct list_head list;
	pid_t pid;
};

static int add_reader(pid_t pid, struct list_head *head)
{
	struct vl53l5_k_wait_list *new_wait_list;

	new_wait_list = kzalloc(sizeof(struct vl53l5_k_wait_list), GFP_KERNEL);
	if (!new_wait_list)
		return VL53L5_K_ERROR_FAILED_TO_ALLOCATE_WORKER_WAIT_LIST;

	new_wait_list->pid = pid;
	list_add(&new_wait_list->list, head);

	vl53l5_k_log_debug("Added pid %d", pid);

	return 0;
}

static void empty_and_free_list(struct list_head *head)
{
	struct vl53l5_k_wait_list *wait_list;
	struct vl53l5_k_wait_list *tmp;

	list_for_each_entry_safe(wait_list, tmp, head, list) {
		list_del(&wait_list->list);
		kfree(wait_list);
	}
}

static bool is_pid_in_list(pid_t pid, struct list_head *head)
{
	struct vl53l5_k_wait_list *wait_list;

	list_for_each_entry(wait_list, head, list)
	if (wait_list->pid == pid)
		return true;

	return false;
}

static bool is_new_data_for_me(struct vl53l5_k_module_t *p_module, pid_t pid)
{
	return p_module->range.is_valid &&
	       !is_pid_in_list(pid, &p_module->reader_list);
}

static bool sleep_for_data_condition(struct vl53l5_k_module_t *p_module,
				     pid_t pid)
{
	bool res;

	res = is_new_data_for_me(p_module, pid);

#ifdef VL53L5_WORKER
	if (!res) {
		res |= !p_module->worker_is_running;
	}
#endif
#ifdef VL53L5_INTERRUPT
	if (!res) {
		res |= !p_module->irq_is_running;
	}
#endif

	return res;
}

static int sleep_for_data(struct vl53l5_k_module_t *p_module, pid_t pid)
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");
	status = wait_event_killable(p_module->wait_queue,
				sleep_for_data_condition(p_module, pid));
	if (status != STATUS_OK)
		status = VL53L5_K_ERROR_SLEEP_FOR_DATA_INTERRUPTED;

	LOG_FUNCTION_END(status);
	return status;
}

void vl53l5_k_wake_up_wait_list(struct vl53l5_k_module_t *p_module)
{
	LOG_FUNCTION_START("");

	empty_and_free_list(&p_module->reader_list);
	wake_up(&p_module->wait_queue);

	LOG_FUNCTION_END(0);
}

int vl53l5_k_sleep_for_data(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;
	pid_t pid = current->pid;

	LOG_FUNCTION_START("");

#ifdef VL53L5_INTERRUPT
	if (!p_module->irq_is_running) {
		status = VL53L5_K_ERROR_DEVICE_NOT_RANGING;
		goto out;
	}
#endif
#ifdef VL53L5_WORKER
	if (!p_module->worker_is_running) {
		status = VL53L5_K_ERROR_DEVICE_NOT_RANGING;
		goto out;
	}
#endif

	if (!is_new_data_for_me(p_module, pid))
		status = sleep_for_data(p_module, pid);
	if (status != STATUS_OK)
		goto out;

#ifdef VL53L5_INTERRUPT
	if (!p_module->irq_is_running) {
		status = VL53L5_K_ERROR_DEVICE_NOT_RANGING;
		goto out;
	}
#endif
#ifdef VL53L5_WORKER
	if (!p_module->worker_is_running) {
		if (p_module->timeout_occurred)
			status = VL53L5_K_ERROR_WORKER_THREAD_TIMEOUT;
		goto out;
	}
#endif

	status = add_reader(pid, &p_module->reader_list);
out:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

int vl53l5_k_check_data_ready(struct vl53l5_k_module_t *p_module, int *p_ready)
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	p_module->polling_count++;

	status = vl53l5_check_data_ready(&p_module->stdev);
	if (status == STATUS_OK) {
		*p_ready = 1;
	} else {
		*p_ready = 0;
		if (status == VL53L5_NO_NEW_RANGE_DATA_ERROR)
			status = 0;
		else

			status = vl53l5_read_device_error(&p_module->stdev,
							  status);
	}

	if (status != STATUS_OK)
		status = vl53l5_read_device_error(&p_module->stdev, status);

	LOG_FUNCTION_END(status);
	return status;
}

int vl53l5_k_check_for_timeout(struct vl53l5_k_module_t *p_module,
			      int timeout_ms,
			      int *p_timeout_occurred)
{
	int status = STATUS_OK;
	unsigned int current_time_ms = 0;

	LOG_FUNCTION_START("");

	status = vl53l5_get_tick_count(&p_module->stdev, &current_time_ms);
	if (status != STATUS_OK)
		goto out;

	status = vl53l5_check_for_timeout(
		&p_module->stdev, p_module->polling_start_time_ms,
		current_time_ms, timeout_ms);
	if (status == VL53L5_ERROR_TIME_OUT)
		*p_timeout_occurred = 1;
	else
		*p_timeout_occurred = 0;

out:

	if (status != STATUS_OK)
		status = vl53l5_read_device_error(&p_module->stdev, status);

	LOG_FUNCTION_END(status);
	return status;
}
