
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
#include <linux/uaccess.h>
#include <linux/firmware.h>
#include <linux/string.h>
#include "vl53l5_k_ioctl_controls.h"
#include "vl53l5_k_driver_config.h"
#include "vl53l5_k_logging.h"
#include "vl53l5_k_error_codes.h"
#include "vl53l5_k_range_wait_handler.h"
#include "vl53l5_k_range_rotation.h"
#include "vl53l5_k_version.h"
#include "vl53l5_platform.h"
#include "vl53l5_api_ranging.h"
#include "vl53l5_api_power.h"
#include "vl53l5_api_core.h"
#include "vl53l5_api_range_decode.h"
#include "cal_size.h"
#ifdef VL53L5_TCDM_DUMP
#include "vl53l5_tcdm_dump.h"
#endif
#ifdef VL53L5_WORKER
#include "vl53l5_k_work_handler.h"
#endif
#ifdef VL53L5_INTERRUPT
#include "vl53l5_k_interrupt.h"
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
#include <linux/delay.h>

#define MAX_FAIL_COUNT 60 // 33 * 60 = 1980 msec
#endif

int p2p_bh[] = VL53L5_K_P2P_CAL_BLOCK_HEADERS;
int shape_bh[] = VL53L5_K_SHAPE_CAL_BLOCK_HEADERS;

static int _check_state(struct vl53l5_k_module_t *p_module,
		 enum vl53l5_k_state_preset expected_state)
{
	enum vl53l5_k_state_preset current_state;
	int is_state = STATUS_OK;

	current_state = p_module->state_preset;

	vl53l5_k_log_debug("current state: %i expected state: %i",
				current_state, expected_state);
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	if (current_state != expected_state)
		is_state = VL53L5_K_ERROR_DEVICE_STATE_INVALID;
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	if (current_state < expected_state) {
		vl53l5_k_log_error("current state: %i expected state: %i",
				current_state, expected_state);
		is_state = VL53L5_K_ERROR_DEVICE_STATE_INVALID;
	}
#endif //STM_VL53L5_SUPPORT_SEC_CODE
	return is_state;
}

static int _poll_for_new_data(struct vl53l5_k_module_t *p_module,
			      int sleep_time_ms,
			      int timeout_ms)
{
	int status = STATUS_OK;
	int data_ready = 0;
	int timeout_occurred = 0;

	LOG_FUNCTION_START("");

	p_module->polling_count = 0;
	status = vl53l5_get_tick_count(
		&p_module->stdev, &p_module->polling_start_time_ms);
	if (status != STATUS_OK)
		goto out;

	do {

		status = vl53l5_k_check_data_ready(p_module, &data_ready);
		if ((data_ready) || (status != STATUS_OK))
			goto out;

		status = vl53l5_k_check_for_timeout(p_module,
						    timeout_ms,
						    &timeout_occurred);
		if (status != STATUS_OK)
			goto out;

		if (timeout_occurred)
			goto out;

		status = vl53l5_wait_ms(
			&p_module->stdev, p_module->polling_sleep_time_ms);
		if (status != STATUS_OK)
			goto out;

	} while (1);

out:

	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}

	if (status == VL53L5_ERROR_TIME_OUT)
		status = VL53L5_K_ERROR_RANGE_POLLING_TIMEOUT;

	LOG_FUNCTION_END(status);
	return status;
}

static int _set_device_params(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;
	const char *fw_path;
	struct spi_device *spi;
	const struct vl53l5_fw_header_t *header;
	unsigned char *fw_data;
	const struct firmware *fw_entry;

	LOG_FUNCTION_START("");

	spi = p_module->spi_info.device;
	fw_path = p_module->firmware_name;

	vl53l5_k_log_debug("Req FW : %s", fw_path);
	status = request_firmware(&fw_entry, fw_path, &spi->dev);
	if (status) {
		vl53l5_k_log_error("FW %s not available", fw_path);
		goto out;
	}

	fw_data = (unsigned char *)fw_entry->data;
	header = (struct vl53l5_fw_header_t *)fw_data;

	vl53l5_k_log_debug("Bin config ver %i.%i",
			header->config_ver_major, header->config_ver_minor);

	switch (p_module->config_preset) {
	case VL53L5_CFG__REFSPAD:
		vl53l5_k_log_info(
		"REFSPAD offset: 0x%08x size: 0x%08x",
			header->cfg00_offset,
			header->cfg00_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->cfg00_offset],
			header->cfg00_size);
		break;
	case VL53L5_CFG__XTALK_GEN1_1000__8X8:
		vl53l5_k_log_info(
		"XTALK_GEN1_1000_8X8 offset: 0x%08x size: 0x%08x",
			header->cfg01_offset,
			header->cfg01_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->cfg01_offset],
			header->cfg01_size);
		break;
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	case VL53L5_CFG__XTALK_GEN1_600__8X8:
		vl53l5_k_log_info(
		"XTALK_GEN1_600_8X8 offset: 0x%08x size: 0x%08x",
			header->cfg02_offset,
			header->cfg02_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->cfg02_offset],
			header->cfg02_size);
		break;
	case VL53L5_CFG__XTALK_GEN2_600__8X8:
		vl53l5_k_log_info(
		"XTALK_GEN2_600_8X8 offset: 0x%08x size: 0x%08x",
			header->cfg03_offset,
			header->cfg03_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->cfg03_offset],
			header->cfg03_size);
		break;
#endif
	case VL53L5_CFG__XTALK_GEN2_300__8X8:
		vl53l5_k_log_info(
		"XTALK_GEN2_300_8X8 offset: 0x%08x size: 0x%08x",
			header->cfg04_offset,
			header->cfg04_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->cfg04_offset],
			header->cfg04_size);
		break;
	case VL53L5_CFG__OFFSET_1000__8X8:
		vl53l5_k_log_info(
		"OFFSET_1000_8X8 offset: 0x%08x size: 0x%08x",
			header->cfg05_offset,
			header->cfg05_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->cfg05_offset],
			header->cfg05_size);
		break;
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	case VL53L5_CFG__OFFSET_600__8X8:
		vl53l5_k_log_info(
		"OFFSET_600_8X8 offset: 0x%08x size: 0x%08x",
			header->cfg06_offset,
			header->cfg06_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->cfg06_offset],
			header->cfg06_size);
		break;
#endif
	case VL53L5_CFG__OFFSET_300__8X8:
		vl53l5_k_log_info(
		"OFFSET_300_8X8 offset: 0x%08x size: 0x%08x",
			header->cfg07_offset,
			header->cfg07_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->cfg07_offset],
			header->cfg07_size);
		break;
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	case VL53L5_CFG__BACK_TO_BACK__4X4__30HZ:
		vl53l5_k_log_info(
		"BACK_TO_BACK_4X4_30HZ offset: 0x%08x size: 0x%08x",
			header->cfg08_offset,
			header->cfg08_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->cfg08_offset],
			header->cfg08_size);
		break;
	case VL53L5_CFG__BACK_TO_BACK__4X4__15HZ:
		vl53l5_k_log_info(
		"BACK_TO_BACK_4X4_15HZ offset: 0x%08x size: 0x%08x",
			header->cfg09_offset,
			header->cfg09_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->cfg09_offset],
			header->cfg09_size);
		break;
	case VL53L5_CFG__BACK_TO_BACK__4X4__10HZ:
		vl53l5_k_log_info(
		"BACK_TO_BACK_4X4_10HZ offset: 0x%08x size: 0x%08x",
			header->cfg10_offset,
			header->cfg10_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->cfg10_offset],
			header->cfg10_size);
		break;
#endif
	case VL53L5_CFG__BACK_TO_BACK__8X8__15HZ:
		vl53l5_k_log_info(
		"BACK_TO_BACK_8X8_15HZ offset: 0x%08x size: 0x%08x",
			header->cfg11_offset,
			header->cfg11_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->cfg11_offset],
			header->cfg11_size);
		break;
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	case VL53L5_CFG__BACK_TO_BACK__8X8__10HZ:
		vl53l5_k_log_info(
		"BACK_TO_BACK_8X8_10HZ offset: 0x%08x size: 0x%08x",
			header->cfg12_offset,
			header->cfg12_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->cfg12_offset],
			header->cfg12_size);
		break;
	case VL53L5_CFG__TIMED__8X8__15HZ__5MS:
		vl53l5_k_log_info(
		"TIMED_8X8_15HZ_5MS offset: 0x%08x size: 0x%08x",
			header->cfg13_offset,
			header->cfg13_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->cfg13_offset],
			header->cfg13_size);
		break;
	case VL53L5_CFG__TIMED__8X8__10HZ__5MS:
		vl53l5_k_log_info(
		"TIMED_8X8_10HZ_5MS offset: 0x%08x size: 0x%08x",
			header->cfg14_offset,
			header->cfg14_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->cfg14_offset],
			header->cfg14_size);
		break;
	case VL53L5_CFG__TIMED__4X4__15HZ__5MS:
		vl53l5_k_log_info(
		"TIMED_4X4_15HZ_5MS offset: 0x%08x size: 0x%08x",
			header->cfg15_offset,
			header->cfg15_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->cfg15_offset],
			header->cfg15_size);
		break;
	case VL53L5_CAL__GENERIC_XTALK_SHAPE:
		vl53l5_k_log_info(
		"CAL_GENERIC_XTALK_SHAPE offset: 0x%08x size: 0x%08x",
			header->cal00_offset,
			header->cal00_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->cal00_offset],
			header->cal00_size);
		break;
	case VL53L5_CFG__STATIC__SS_0PC__VCSELCP_ON:
		vl53l5_k_log_info(
		"STATIC_SS_0PC_VCSELCP_ON offset: 0x%08x size: 0x%08x",
			header->sta00_offset,
			header->sta00_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->sta00_offset],
			header->sta00_size);
		break;
	case VL53L5_CFG__STATIC__SS_1PC__VCSELCP_ON:
		vl53l5_k_log_info(
		"STATIC_SS_1PC_VCSELCP_ON offset: 0x%08x size: 0x%08x",
			header->sta01_offset,
			header->sta01_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->sta01_offset],
			header->sta01_size);
		break;
	case VL53L5_CFG__STATIC__SS_2PC__VCSELCP_ON:
		vl53l5_k_log_info(
		"STATIC_SS_2PC_VCSELCP_ON offset: 0x%08x size: 0x%08x",
			header->sta02_offset,
			header->sta02_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->sta02_offset],
			header->sta02_size);
		break;
	case VL53L5_CFG__STATIC__SS_4PC__VCSELCP_ON:
		vl53l5_k_log_info(
		"STATIC_SS_4PC_VCSELCP_ON offset: 0x%08x size: 0x%08x",
			header->sta03_offset,
			header->sta03_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->sta03_offset],
			header->sta03_size);
		break;
	case VL53L5_CFG__STATIC__SS_0PC__VCSELCP_OFF:
		vl53l5_k_log_info(
		"STATIC_SS_0PC_VCSELCP_OFF offset: 0x%08x size: 0x%08x",
			header->sta04_offset,
			header->sta04_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->sta04_offset],
			header->sta04_size);
		break;
	case VL53L5_CFG__STATIC__SS_1PC__VCSELCP_OFF:
		vl53l5_k_log_info(
		"STATIC_SS_1PC_VCSELCP_OFF offset: 0x%08x size: 0x%08x",
			header->sta05_offset,
			header->sta05_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->sta05_offset],
			header->sta05_size);
		break;
	case VL53L5_CFG__STATIC__SS_2PC__VCSELCP_OFF:
		vl53l5_k_log_info(
		"STATIC_SS_2PC_VCSELCP_OFF offset: 0x%08x size: 0x%08x",
			header->sta06_offset,
			header->sta06_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->sta06_offset],
			header->sta06_size);
		break;
#endif
	case VL53L5_CFG__STATIC__SS_4PC__VCSELCP_OFF:
		vl53l5_k_log_info(
		"STATIC_SS_4PC_VCSELCP_OFF offset: 0x%08x size: 0x%08x",
			header->sta07_offset,
			header->sta07_size);
		status = vl53l5_set_device_parameters(
			&p_module->stdev,
			&fw_data[header->sta07_offset],
			header->sta07_size);
		break;
	default:
		vl53l5_k_log_error("Invalid preset: %i",
				   p_module->config_preset);
		status = VL53L5_K_ERROR_INVALID_CONFIG_PRESET;
	break;
	};

	vl53l5_k_log_debug("Release FW");
	release_firmware(fw_entry);
out:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}
	LOG_FUNCTION_END(status);
	return status;
}

static int _start_poll_stop(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	vl53l5_k_log_debug("Calibration vl53l5_start");
	status = vl53l5_start(&p_module->stdev, NULL);
	if (status != STATUS_OK) {
		vl53l5_k_log_error("start failed");
		goto out;
	}

	vl53l5_k_log_debug("Poll data");
	status = _poll_for_new_data(
		p_module, p_module->polling_sleep_time_ms,
		VL53L5_MAX_CALIBRATION_POLL_TIME_MS);
	if (status != STATUS_OK)
		goto out_stop;

	status = vl53l5_stop(&p_module->stdev, NULL);
	if (status != STATUS_OK) {
		vl53l5_k_log_error("stop failed");
		goto out;
	}

out:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}
	LOG_FUNCTION_END(status);
	return status;

out_stop:
	(void)vl53l5_stop(&p_module->stdev, NULL);
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}
	LOG_FUNCTION_END(status);
	return status;
}
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
static int _write_p2p_calibration(struct vl53l5_k_module_t *p_module)
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
static int _write_p2p_calibration(struct vl53l5_k_module_t *p_module, int flow)
#endif //STM_VL53L5_SUPPORT_SEC_CODE
{
	int status = STATUS_OK;
	int num_bh = VL53L5_K_P2P_BH_SZ;
	int *p_buffer[] = {&p2p_bh[0]};
	char buffer[VL53L5_K_DEVICE_INFO_SZ] = {0};

	LOG_FUNCTION_START("");

	status = _check_state(p_module, VL53L5_STATE_INITIALISED);
	if (status != STATUS_OK)
		goto out_state;

	status = vl53l5_get_version(&p_module->stdev,
				    &p_module->calibration.version);

	if (status != STATUS_OK)
		goto out;

	status = vl53l5_get_module_info(&p_module->stdev,
					&p_module->calibration.info);

	if (status != STATUS_OK)
		goto out;

	status = vl53l5_get_device_parameters(&p_module->stdev,
					      p_buffer[0],
					      num_bh);

	if (status != STATUS_OK)
		goto out;

	status = vl53l5_encode_device_info(&p_module->stdev,
					&p_module->calibration.info,
					&p_module->calibration.version,
					buffer);

	if (status < STATUS_OK)
		goto out;

#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	vl53l5_k_log_debug("filename %s", VL53L5_CAL_P2P_FILENAME);
	vl53l5_k_log_debug("file size %i", VL53L5_K_P2P_FILE_SIZE + 4);
	status = vl53l5_write_file(&p_module->stdev,
				p_module->stdev.host_dev.p_comms_buff,
				VL53L5_K_P2P_FILE_SIZE + 4,
				VL53L5_CAL_P2P_FILENAME,
				buffer,
				VL53L5_K_DEVICE_INFO_SZ);

	if (status < STATUS_OK) {
		vl53l5_k_log_error(
			"write file %s failed: %d ",
				VL53L5_CAL_P2P_FILENAME, status);
		goto out;
	}
#endif //STM_VL53L5_SUPPORT_LEGACY_CODE
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	if (flow == 1 || flow == 3) {
		vl53l5_k_log_debug("Write filename %s", VL53L5_CAL_P2P_FILENAME);
		vl53l5_k_log_debug("Write file size %i", VL53L5_K_P2P_FILE_SIZE + 4);
		status = vl53l5_write_file(&p_module->stdev,
					p_module->stdev.host_dev.p_comms_buff,
					VL53L5_K_P2P_FILE_SIZE + 4,
					VL53L5_CAL_P2P_FILENAME,
					buffer,
					VL53L5_K_DEVICE_INFO_SZ);

		if (status < STATUS_OK) {
			vl53l5_k_log_error(
			"write file %s fail-%d ",
				VL53L5_CAL_P2P_FILENAME, status);
			goto out;
		}
	} else {
		vl53l5_k_log_debug("Write filename %s", VL53L5_CAL_DATA_FILENAME);
		vl53l5_k_log_debug("Write file size %i", VL53L5_K_P2P_FILE_SIZE + 4);
		status = vl53l5_write_file(&p_module->stdev,
					p_module->stdev.host_dev.p_comms_buff,
					VL53L5_K_P2P_FILE_SIZE + 4,
					VL53L5_CAL_DATA_FILENAME,
					buffer,
					VL53L5_K_DEVICE_INFO_SZ);
		if (status < STATUS_OK) {
			vl53l5_k_log_error(
			"write file %s fail-%d ",
				VL53L5_CAL_DATA_FILENAME, status);
			goto out;
		}
	}
#endif //STM_VL53L5_SUPPORT_SEC_CODE
out:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}
out_state:
	LOG_FUNCTION_END(status);
	return status;
}

int _write_shape_calibration(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;
	int num_bh = VL53L5_K_SHAPE_CAL_BLOCK_HEADERS_SZ;
	int *p_buffer[] = {&shape_bh[0]};
	char buffer[VL53L5_K_DEVICE_INFO_SZ] = {0};

	LOG_FUNCTION_START("");

	status = _check_state(p_module, VL53L5_STATE_INITIALISED);
	if (status != STATUS_OK)
		goto out_state;

	status = vl53l5_get_version(&p_module->stdev,
				    &p_module->calibration.version);

	if (status != STATUS_OK)
		goto out;

	status = vl53l5_get_module_info(&p_module->stdev,
					&p_module->calibration.info);

	if (status != STATUS_OK)
		goto out;

	status = vl53l5_get_device_parameters(&p_module->stdev,
					      p_buffer[0],
					      num_bh);

	if (status != STATUS_OK)
		goto out;

	status = vl53l5_encode_device_info(&p_module->stdev,
					&p_module->calibration.info,
					&p_module->calibration.version,
					buffer);

	if (status < STATUS_OK)
		goto out;

	vl53l5_k_log_debug("filename %s", VL53L5_CAL_SHAPE_FILENAME);
	vl53l5_k_log_debug("file size %i", VL53L5_K_SHAPE_FILE_SIZE + 4);
	status = vl53l5_write_file(&p_module->stdev,
				p_module->stdev.host_dev.p_comms_buff,
				VL53L5_K_SHAPE_FILE_SIZE + 4,
				VL53L5_CAL_SHAPE_FILENAME,
				buffer,
				VL53L5_K_DEVICE_INFO_SZ);

	if (status < STATUS_OK) {
		vl53l5_k_log_error(
			"write file %s failed: %d ",
				VL53L5_CAL_SHAPE_FILENAME, status);
		goto out;
	}

out:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}
out_state:
	LOG_FUNCTION_END(status);
	return status;
}

#if defined(STM_VL53L5_SUPPORT_LEGACY_CODE) && defined(VL53L5_TCDM_DUMP)
static void _tcdm_dump(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;
	char *p_data;
	int count = 0;
	int buff_size = 100000 * sizeof(char);

	LOG_FUNCTION_START("");

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
				VL53L5_TCDM_FILENAME,
				NULL,
				0);

	if (status < STATUS_OK) {
		vl53l5_k_log_error(
				"write file %s failed: %d ",
				VL53L5_TCDM_FILENAME, status);
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
}
#endif

int vl53l5_ioctl_init(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;
	int _comms_buff_count = sizeof(p_module->comms_buffer);
	const char *fw_path;
	struct spi_device *spi;
	const struct vl53l5_fw_header_t *header;
	unsigned char *fw_data;
	const struct firmware *fw_entry = NULL;

	LOG_FUNCTION_START("");

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	status = _check_state(p_module, VL53L5_STATE_PRESENT);
	if (status != STATUS_OK) {
		goto out_state;
	}
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	if (p_module->last_driver_error == VL53L5_PROBE_FAILED)
		return VL53L5_PROBE_FAILED;

	memset(&p_module->stdev, 0, sizeof(struct vl53l5_dev_handle_t));
	memset(&p_module->comms_buffer, 0, VL53L5_COMMS_BUFFER_SIZE_BYTES);

	vl53l5_k_power_onoff(p_module, p_module->iovdd_vreg, p_module->iovdd_vreg_name, 0);
	vl53l5_k_power_onoff(p_module, p_module->avdd_vreg, p_module->avdd_vreg_name, 0);
	msleep(20);
	vl53l5_k_power_onoff(p_module, p_module->avdd_vreg, p_module->avdd_vreg_name, 1);
	usleep_range(1000, 1100);
	vl53l5_k_power_onoff(p_module, p_module->iovdd_vreg, p_module->iovdd_vreg_name, 1);
	usleep_range(5000, 5100);
#endif
	if (!p_module->firmware_name) {
		vl53l5_k_log_error(
			"FW name not in dts");
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
		p_module->stdev.status_probe = -2;
#endif
		goto out;
	}

	spi = p_module->spi_info.device;
	fw_path = p_module->firmware_name;
	vl53l5_k_log_debug("Load FW : %s", fw_path);

	vl53l5_k_log_debug("Req FW");
	status = request_firmware(&fw_entry, fw_path, &spi->dev);
	if (status) {
		vl53l5_k_log_error("FW %s not available", fw_path);
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
		p_module->stdev.status_probe = -3;
#endif
		goto out;
	}

	fw_data = (unsigned char *)fw_entry->data;
	header = (struct vl53l5_fw_header_t *)fw_data;

	vl53l5_k_log_info("Bin FW ver %i.%i",
				header->fw_ver_major, header->fw_ver_minor);

	p_module->stdev.host_dev.p_fw_buff = &fw_data[header->fw_offset];
	p_module->stdev.host_dev.fw_buff_count = header->fw_size;

	p_module->last_driver_error = 0;

	VL53L5_ASSIGN_COMMS_BUFF(&p_module->stdev, p_module->comms_buffer,
				 _comms_buff_count);

	status = vl53l5_init(&p_module->stdev);
	if (status != STATUS_OK) {
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
		p_module->stdev.status_probe = -4;
#endif
		goto out_powerdown;
	}

	vl53l5_k_log_debug("Release FW");
	release_firmware(fw_entry);

	p_module->stdev.host_dev.p_fw_buff = NULL;
	p_module->stdev.host_dev.fw_buff_count = 0;

#ifdef VL53L5_INTERRUPT
	status = vl53l5_k_start_interrupt(p_module);
	if (status != STATUS_OK) {
		vl53l5_k_log_error("Failed to start interrupt: %d", status);
		goto out;
	}
	disable_irq(p_module->gpio.interrupt);
#endif

	p_module->state_preset = VL53L5_STATE_INITIALISED;
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (p_module->load_calibration == true
		|| p_module->read_p2p_cal_data == true) {
		vl53l5_ioctl_set_power_mode(p_module, NULL, VL53L5_POWER_STATE_HP_IDLE);
		usleep_range(5000, 5100);
		if (p_module->load_calibration == true) {
			vl53l5_ioctl_read_generic_shape(p_module);
			usleep_range(1000, 1100);
		}

		if (p_module->read_p2p_cal_data == true) {
			vl53l5_ioctl_read_p2p_calibration(p_module);
			usleep_range(1000, 1100);
		}
	} else {
		usleep_range(2000, 2100);
	}

	vl53l5_ioctl_set_device_parameters(p_module, NULL, VL53L5_CFG__BACK_TO_BACK__8X8__15HZ);
	usleep_range(1000, 1100);
	vl53l5_ioctl_set_device_parameters(p_module, NULL, VL53L5_CFG__STATIC__SS_4PC__VCSELCP_OFF);
	usleep_range(1000, 1100);
	p_module->stdev.last_dev_error = VL53L5_ERROR_NONE;
	p_module->last_driver_error = STATUS_OK;
	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
#endif
out:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
out_state:
#endif
	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	LOG_FUNCTION_END(status);
	return status;
out_powerdown:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}
	vl53l5_k_log_debug("Release FW");
	release_firmware(fw_entry);
	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	(void)vl53l5_ioctl_term(p_module);
	LOG_FUNCTION_END(status);
	return status;
}

int vl53l5_ioctl_term(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = _check_state(p_module, VL53L5_STATE_RANGING);
	if (status != STATUS_OK) {
		status = _check_state(p_module, VL53L5_STATE_INITIALISED);
		if (status != STATUS_OK)
			goto out_state;
	} else {
		status = vl53l5_stop(&p_module->stdev,
				     &p_module->ranging_flags);
		if (status != STATUS_OK)
			goto out;
	}

#ifdef VL53L5_INTERRUPT
	disable_irq(p_module->gpio.interrupt);
	status = vl53l5_k_stop_interrupt(p_module);
	if (status != STATUS_OK) {
		vl53l5_k_log_error(
			"Failed to stop interrupt: %d", status);
		goto out;
	}
#endif
#ifdef VL53L5_WORKER
	status = vl53l5_k_cancel_worker(p_module);
	if (status != STATUS_OK) {
		vl53l5_k_log_error(
			"Failed to cancel worker: %d", status);
		goto out;
	}
#endif

	status = vl53l5_term(&p_module->stdev);
	if (status != STATUS_OK)
		goto out;

	p_module->state_preset = VL53L5_STATE_PRESENT;
out:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}
out_state:

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	LOG_FUNCTION_END(status);
	return status;
}

int vl53l5_ioctl_get_version(struct vl53l5_k_module_t *p_module, void __user *p)
{
	int status = STATUS_OK;
	struct vl53l5_version_t *p_version = NULL;

	LOG_FUNCTION_START("");

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = _check_state(p_module, VL53L5_STATE_INITIALISED);
	if (status != STATUS_OK)
		goto out_state;

	p_version = kzalloc(sizeof(struct vl53l5_version_t), GFP_KERNEL);
	if (p_version == NULL) {
		vl53l5_k_log_error("Allocate Failed");
		status = VL53L5_K_ERROR_FAILED_TO_ALLOCATE_VERSION;
		goto out;
	}

	status = vl53l5_get_version(&p_module->stdev, p_version);
	if (status != STATUS_OK)
		goto out_free;

	vl53l5_k_log_info(
		"Driver Ver: %d.%d.%d.%d",
		VL53L5_K_VER_MAJOR,
		VL53L5_K_VER_MINOR,
		VL53L5_K_VER_BUILD,
		VL53L5_K_VER_REVISION);
	vl53l5_k_log_info(
		"API Ver: %d.%d.%d.%d",
		p_version->driver.ver_major,
		p_version->driver.ver_minor,
		p_version->driver.ver_build,
		p_version->driver.ver_revision);
	vl53l5_k_log_info(
		"FW Ver : %d.%d.%d.%d",
		p_version->firmware.ver_major,
		p_version->firmware.ver_minor,
		p_version->firmware.ver_build,
		p_version->firmware.ver_revision);

	status = copy_to_user(p, p_version, sizeof(struct vl53l5_version_t));
	if (status != STATUS_OK) {
		status = VL53L5_K_ERROR_FAILED_TO_COPY_VERSION;
		goto out;
	}
out:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}
out_free:
	kfree(p_version);
out_state:

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	LOG_FUNCTION_END(status);
	return status;
}

#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
int vl53l5_ioctl_get_module_info(struct vl53l5_k_module_t *p_module,
				 void __user *p)
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
int vl53l5_ioctl_get_module_info(struct vl53l5_k_module_t *p_module,
				 void __user *p, void *internal_p)
#endif
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = _check_state(p_module, VL53L5_STATE_INITIALISED);
	if (status != STATUS_OK)
		goto out_state;

	status = vl53l5_get_module_info(&p_module->stdev, &p_module->m_info);
	if (status != STATUS_OK)
		goto out;
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	vl53l5_k_log_info("FGC: %s", (char *)p_module->m_info.fgc);
#endif
	vl53l5_k_log_info("ID : %x.%x",
			  p_module->m_info.module_id_hi,
			  p_module->m_info.module_id_lo);

#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	status = copy_to_user(p, &p_module->m_info,
				sizeof(struct vl53l5_module_info_t));
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	if (p != NULL)
		status = copy_to_user(p, &p_module->m_info, sizeof(struct vl53l5_module_info_t));
	else
		status = STATUS_OK;
#endif //STM_VL53L5_SUPPORT_SEC_CODE

	if (status != STATUS_OK) {
		status = VL53L5_K_ERROR_FAILED_TO_COPY_MODULE_INFO;
		goto out;
	}

out:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}

out_state:

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	LOG_FUNCTION_END(status);
	return status;
}

#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
int vl53l5_ioctl_start(struct vl53l5_k_module_t *p_module, void __user *p)
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
int vl53l5_ioctl_start(struct vl53l5_k_module_t *p_module, void __user *p, int mode_control)
#endif
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	if (p_module->state_preset == VL53L5_STATE_RANGING)
		return status;

	if (mode_control) {
		vl53l5_ioctl_set_power_mode(p_module, NULL, VL53L5_POWER_STATE_HP_IDLE);
		usleep_range(5000, 5100);
		vl53l5_ioctl_set_device_parameters(p_module, NULL, VL53L5_CFG__BACK_TO_BACK__8X8__15HZ);
		usleep_range(2000, 2100);
	}
#endif
	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = _check_state(p_module, VL53L5_STATE_INITIALISED);

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (status != STATUS_OK)
		goto out_state;

	if (p != NULL) {

		vl53l5_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		status = copy_from_user(
			&p_module->ranging_flags, p,
			sizeof(struct vl53l5_ranging_mode_flags_t));

		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
		if (status != STATUS_OK) {
			status = VL53L5_K_ERROR_FAILED_TO_COPY_FLAGS;
			goto out;
		}
	}

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = vl53l5_start(&p_module->stdev, &p_module->ranging_flags);

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (status != STATUS_OK)
		goto out;

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	p_module->range.count = 0;
	p_module->range.is_valid = 0;
	p_module->polling_start_time_ms = 0;

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);

#ifdef VL53L5_INTERRUPT

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	enable_irq(p_module->gpio.interrupt);
	p_module->irq_is_active = true;

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
#endif

#ifdef VL53L5_WORKER

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = vl53l5_k_start_worker(p_module);

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (status != STATUS_OK) {
		vl53l5_k_log_error(
			"Failed to schedule worker: %d", status);
		goto out;
	}
#endif

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	p_module->state_preset = VL53L5_STATE_RANGING;

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);

out:
	if (status != STATUS_OK) {

		vl53l5_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
	}
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	else {
		if (p_module->last_driver_error != STATUS_OK)
			vl53l5_k_log_error("last driver error %d", p_module->last_driver_error);
		p_module->last_driver_error = status;
		p_module->failed_count = 0;
	}
#endif
out_state:
	LOG_FUNCTION_END(status);
	return status;
}

#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
int vl53l5_ioctl_stop(struct vl53l5_k_module_t *p_module, void __user *p)
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
int vl53l5_ioctl_stop(struct vl53l5_k_module_t *p_module, void __user *p, int mode_control)
#endif
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	if (p_module->state_preset == VL53L5_STATE_LOW_POWER)
		return status;

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = _check_state(p_module, VL53L5_STATE_INITIALISED);
#endif
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = _check_state(p_module, VL53L5_STATE_RANGING);
#endif

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (status != STATUS_OK)
		goto out_state;

	if (p != NULL) {

		vl53l5_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		status = copy_from_user(
			&p_module->ranging_flags, p,
			sizeof(struct vl53l5_ranging_mode_flags_t));

		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
		if (status != STATUS_OK) {
			status = VL53L5_K_ERROR_FAILED_TO_COPY_FLAGS;
			goto out;
		}
	}

#ifdef VL53L5_INTERRUPT

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	disable_irq(p_module->gpio.interrupt);
	p_module->irq_is_active = false;

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
#endif

#ifdef VL53L5_WORKER

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	status = vl53l5_k_cancel_worker(p_module);

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (status != STATUS_OK) {
		vl53l5_k_log_error(
			"Failed to cancel worker: %d", status);
		goto out;
	}
#endif

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = vl53l5_stop(&p_module->stdev, &p_module->ranging_flags);

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (status != STATUS_OK)
		goto out;

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	p_module->range.count = 0;
	p_module->range.is_valid = 0;
	p_module->polling_start_time_ms = 0;
	p_module->state_preset = VL53L5_STATE_INITIALISED;

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);

#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	if (mode_control) {
		usleep_range(5000, 5100);
		vl53l5_ioctl_set_power_mode(p_module, NULL, VL53L5_POWER_STATE_LP_IDLE_COMMS);
	}
#endif

out:
	if (status != STATUS_OK) {

		vl53l5_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
	}
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	else {
		vl53l5_k_log_info("last info %d, %d", p_module->last_driver_error, p_module->stdev.last_dev_error);
	}
#endif
out_state:
	LOG_FUNCTION_END(status);
	return status;
}

int vl53l5_ioctl_get_range(struct vl53l5_k_module_t *p_module, void __user *p)
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = _check_state(p_module, VL53L5_STATE_RANGING);

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	if (status != STATUS_OK)
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	if ((status != STATUS_OK) || (p_module->last_driver_error != STATUS_OK))
#endif
		goto out_state;

	if (p_module->range_mode == VL53L5_RANGE_SERVICE_DEFAULT) {

		vl53l5_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		status = vl53l5_get_range_data(&p_module->stdev);

		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
		if (status == VL53L5_NO_NEW_RANGE_DATA_ERROR ||
			status == VL53L5_TOO_HIGH_AMBIENT_WARNING)
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
		p_module->range.count++;
		p_module->range.is_valid = 1;
		status = vl53l5_k_range_rotation(p_module->rotation_mode,
						 p_module->range_filter,
						 &p_module->range.data.core);

		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
		if (status != STATUS_OK)
			goto out;
	}

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	status = copy_to_user(
		p, &p_module->range.data, sizeof(struct vl53l5_range_data_t));
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	if (p != NULL) {
		if (status == STATUS_OK) {
#ifdef CONFIG_SENSORS_VL53L5_SUPPORT_UAPI
			memcpy(p_module->af_range_data.range_data, p_module->range.data.core.per_tgt_results.median_range_mm, sizeof(p_module->af_range_data.range_data));
			memcpy(p_module->af_range_data.status, p_module->range.data.core.per_tgt_results.target_status, sizeof(p_module->af_range_data.status));
			status = copy_to_user(
				p, &p_module->af_range_data, sizeof(struct range_sensor_data_t));
#else
			status = copy_to_user(
				p, &p_module->range.data, sizeof(struct vl53l5_range_data_t));
#endif
		}
	}
	else {
		if (status == STATUS_OK) {
#ifdef CONFIG_SENSORS_VL53L5_SUPPORT_UAPI
			memcpy(p_module->af_range_data.range_data, p_module->range.data.core.per_tgt_results.median_range_mm, sizeof(p_module->af_range_data.range_data));
			memcpy(p_module->af_range_data.status, p_module->range.data.core.per_tgt_results.target_status, sizeof(p_module->af_range_data.status));
#endif
			memcpy(&p_module->range_data, &p_module->range.data, sizeof(p_module->range_data));
		}
	}
#endif //STM_VL53L5_SUPPORT_SEC_CODE

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (status != STATUS_OK) {
		status = VL53L5_K_ERROR_FAILED_TO_COPY_RANGE_DATA;
		goto out;
	}

out:
	if (status != STATUS_OK) {

		vl53l5_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		status = vl53l5_read_device_error(&p_module->stdev, status);

		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
	}
#if defined(VL53L5_INTERRUPT) || defined(VL53L5_WORKER)
	else if (p_module->last_driver_error != STATUS_OK)
		status = p_module->last_driver_error;
	if (status != STATUS_OK) {
#endif
#ifdef VL53L5_INTERRUPT

		vl53l5_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		vl53l5_k_log_debug("Disable IRQ");
		disable_irq(p_module->gpio.interrupt);
		p_module->irq_is_active = false;

		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
#endif
#ifdef VL53L5_WORKER

		vl53l5_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		vl53l5_k_log_debug("Disable worker thread");
		(void)vl53l5_k_cancel_worker(p_module);

		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
#endif
#if defined(VL53L5_INTERRUPT) || defined(VL53L5_WORKER)
	}
#endif
	if (status != STATUS_OK) {
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
		vl53l5_k_log_error("Failed: %d", status);
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
		if (p_module->failed_count == MAX_FAIL_COUNT)
			vl53l5_k_log_error("Failed: %d", status);
#endif

#if defined(VL53L5_TCDM_DUMP)
		if (status < VL53L5_K_ERROR_NOT_DEFINED) {
			vl53l5_k_log_info("TCDM Dump");

			vl53l5_k_log_debug("Lock");
			mutex_lock(&p_module->mutex);
			_tcdm_dump(p_module);
			vl53l5_k_log_debug("Unlock");
			mutex_unlock(&p_module->mutex);
		}
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
		p_module->last_driver_error = status;
#endif
	}
out_state:
	LOG_FUNCTION_END(status);
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	if (status != STATUS_OK) {
		p_module->failed_count++;
		if (p_module->failed_count > MAX_FAIL_COUNT) {
			int m_status;

			p_module->failed_count = 0;
			m_status = vl53l5_ioctl_init(p_module);
			if (m_status == STATUS_OK) {
				m_status = vl53l5_ioctl_start(p_module, NULL, 1);
				if (m_status != STATUS_OK)
					vl53l5_k_log_error("Start Failed %d", m_status);
				else
					msleep(60);
			} else
				vl53l5_k_log_error("Reset Failed %d", m_status);
		}
	}
#endif
	return status;
}

#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
int vl53l5_ioctl_blocking_get_range(struct vl53l5_k_module_t *p_module,
				    void __user *p)
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = _check_state(p_module, VL53L5_STATE_RANGING);

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (status != STATUS_OK)
		goto out_state;

	if (p_module->range_mode == VL53L5_RANGE_SERVICE_DEFAULT) {

		vl53l5_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		status = _poll_for_new_data(
			p_module, p_module->polling_sleep_time_ms,
			VL53L5_MAX_POLL_TIME_MS);

		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
		if (status != STATUS_OK) {
			if ((status == VL53L5_K_ERROR_DEVICE_NOT_INITIALISED) ||
			    (status == VL53L5_K_ERROR_RANGE_POLLING_TIMEOUT))
				status = VL53L5_K_ERROR_DEVICE_NOT_RANGING;
			goto out;
		}

		vl53l5_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
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
		p_module->polling_count = 0;
		p_module->range.is_valid = 1;

		status = vl53l5_k_range_rotation(p_module->rotation_mode,
						 p_module->range_filter,
						 &p_module->range.data.core);

		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
		if (status != STATUS_OK)
			goto out;

	}

#if defined(VL53L5_INTERRUPT) || defined(VL53L5_WORKER)
	else {

		status = vl53l5_k_sleep_for_data(p_module);
		if (status != STATUS_OK)
			goto out;
	}
#endif

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	status = copy_to_user(p, &p_module->range.data,
			      sizeof(struct vl53l5_range_data_t));

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (status != STATUS_OK) {
		status = VL53L5_K_ERROR_FAILED_TO_COPY_RANGE_DATA;
		goto out;
	}
out:
	if (status != STATUS_OK) {

		vl53l5_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		status = vl53l5_read_device_error(&p_module->stdev, status);

		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
	}
#if defined(VL53L5_INTERRUPT) || defined(VL53L5_WORKER)
	else if (p_module->last_driver_error != STATUS_OK)
		status = p_module->last_driver_error;
	if (status != STATUS_OK) {
#endif
#ifdef VL53L5_INTERRUPT

		vl53l5_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		vl53l5_k_log_debug("Disable interrupt");
		disable_irq(p_module->gpio.interrupt);
		p_module->irq_is_active = false;

		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
#endif
#ifdef VL53L5_WORKER

		vl53l5_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		vl53l5_k_log_debug("Disable worker thread");
		(void)vl53l5_k_cancel_worker(p_module);

		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
#endif
#if defined(VL53L5_INTERRUPT) || defined(VL53L5_WORKER)
	}
#endif
	if (status != STATUS_OK) {
		vl53l5_k_log_error("Failed: %d", status);
#if defined(STM_VL53L5_SUPPORT_LEGACY_CODE) && defined(VL53L5_TCDM_DUMP)
		if (status < VL53L5_K_ERROR_NOT_DEFINED) {
			vl53l5_k_log_info("TCDM Dump");

			vl53l5_k_log_debug("Lock");
			mutex_lock(&p_module->mutex);
			_tcdm_dump(p_module);

			vl53l5_k_log_debug("Unlock");
			mutex_unlock(&p_module->mutex);
		}
#endif
	}
out_state:
	LOG_FUNCTION_END(status);
	return status;
}
#endif // STM_VL53L5_SUPPORT_LEGACY_CODE

#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
int vl53l5_ioctl_set_device_parameters(struct vl53l5_k_module_t *p_module,
				       void __user *p)
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
int vl53l5_ioctl_set_device_parameters(struct vl53l5_k_module_t *p_module,
				       void __user *p,  int config)
#endif
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = _check_state(p_module, VL53L5_STATE_INITIALISED);
	if (status != STATUS_OK)
		goto out_state;

#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	status = copy_from_user(&p_module->config_preset, p,
				sizeof(int));
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	if (p != NULL) {
		status = copy_from_user(&p_module->config_preset, p,
				sizeof(int));
	} else {
		p_module->config_preset = config;
		status = STATUS_OK;
	}
#endif
	if (status != STATUS_OK) {
		status = VL53L5_K_ERROR_FAILED_TO_COPY_CONFIG_PRESET;
		goto out;
	}

	status = _set_device_params(p_module);

out:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}

out_state:
	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	LOG_FUNCTION_END(status);
	return status;
}

int vl53l5_ioctl_get_device_parameters(struct vl53l5_k_module_t *p_module,
				       void __user *p)
{
	int status;
	struct vl53l5_k_get_parameters_data_t *p_get_data;
	int count = 0;

	LOG_FUNCTION_START("");

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = _check_state(p_module, VL53L5_STATE_INITIALISED);
	if (status != STATUS_OK)
		goto out_state;

	p_get_data = kzalloc(
		sizeof(struct vl53l5_k_get_parameters_data_t), GFP_KERNEL);
	if (p_get_data == NULL) {
		vl53l5_k_log_error("Allocate Failed");
		status = VL53L5_K_ERROR_FAILED_TO_ALLOCATE_PARAMETER_DATA;
		goto out;
	}

	status = copy_from_user(
		p_get_data, p, sizeof(struct vl53l5_k_get_parameters_data_t));
	if (status != STATUS_OK) {
		status = VL53L5_K_ERROR_FAILED_TO_COPY_PARAMETER_DATA;
		goto out_free;
	}

	status = vl53l5_get_device_parameters(&p_module->stdev,
					      p_get_data->headers,
					      p_get_data->header_count);
	if (status != STATUS_OK)
		goto out;

	count = (p_module->stdev.host_dev.comms_buff_count - 4);

	memcpy(p_get_data->raw_data.buffer,
	       p_module->stdev.host_dev.p_comms_buff,
	       count);
	p_get_data->raw_data.count = count;

	status = copy_to_user(
		p, p_get_data, sizeof(struct vl53l5_k_get_parameters_data_t));

out:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}
out_free:
	kfree(p_get_data);
out_state:

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	LOG_FUNCTION_END(status);
	return status;
}
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
int vl53l5_ioctl_get_error_info(struct vl53l5_k_module_t *p_module,
				void __user *p)
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	status = copy_to_user(p, &p_module->last_driver_error, sizeof(int));
	if (status != STATUS_OK) {
		status = VL53L5_K_ERROR_FAILED_TO_COPY_ERROR_INFO;
		goto out;
	}

	p_module->last_driver_error = STATUS_OK;
out:

	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(status);
	return status;
}
#endif
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
int vl53l5_ioctl_set_power_mode(struct vl53l5_k_module_t *p_module,
				void __user *p)
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
int vl53l5_ioctl_set_power_mode(struct vl53l5_k_module_t *p_module,
				void __user *p, enum vl53l5_power_states state)
#endif
{
	int status = STATUS_OK;
	enum vl53l5_k_state_preset current_state;
	const char *fw_path;
	struct spi_device *spi;
	const struct vl53l5_fw_header_t *header;
	unsigned char *fw_data;
	const struct firmware *fw_entry = NULL;
	enum vl53l5_power_states current_power_state;

	LOG_FUNCTION_START("");

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	current_state = p_module->state_preset;
	current_power_state = p_module->power_state;

	if (current_state < VL53L5_STATE_LOW_POWER) {
		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
		status = VL53L5_K_ERROR_DEVICE_STATE_INVALID;
		goto out_state;
	}

	vl53l5_k_log_debug("Current state : %i", p_module->power_state);

	if (p_module->power_state == VL53L5_POWER_STATE_ULP_IDLE) {
		spi = p_module->spi_info.device;
		fw_path = p_module->firmware_name;
		vl53l5_k_log_debug("Load FW : %s", fw_path);

		vl53l5_k_log_debug("Req FW");
		status = request_firmware(&fw_entry, fw_path, &spi->dev);
		if (status) {
			vl53l5_k_log_error("FW %s not available",
						fw_path);
			goto out;
		}

		fw_data = (unsigned char *)fw_entry->data;
		header = (struct vl53l5_fw_header_t *)fw_data;

		vl53l5_k_log_info("Bina FW ver %i.%i",
				header->fw_ver_major, header->fw_ver_minor);

		p_module->stdev.host_dev.p_fw_buff =
						&fw_data[header->fw_offset];
		p_module->stdev.host_dev.fw_buff_count = header->fw_size;
	}

#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	status = copy_from_user(&p_module->power_state, p,
				sizeof(enum vl53l5_power_states));
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	if (p != NULL) {
		status = copy_from_user(&p_module->power_state, p,
					sizeof(enum vl53l5_power_states));
	} else {
		p_module->power_state = state;
		status = STATUS_OK;
	}
#endif
	if (status != STATUS_OK) {
		status = VL53L5_K_ERROR_FAILED_TO_COPY_POWER_MODE;
		goto out;
	}

	vl53l5_k_log_debug("Req state : %i", p_module->power_state);

	status = vl53l5_set_power_mode(&p_module->stdev,
				       p_module->power_state);
	if (status == VL53L5_ERROR_POWER_STATE) {
		vl53l5_k_log_error("Invalid state %i", p_module->power_state);

		p_module->power_state = current_power_state;
		(void)vl53l5_set_power_mode(&p_module->stdev,
						p_module->power_state);
		status = VL53L5_K_ERROR_INVALID_POWER_STATE;
		goto out;
	}
	else if (status != STATUS_OK)
		goto out;

	switch (p_module->power_state) {
	case VL53L5_POWER_STATE_ULP_IDLE:
	case VL53L5_POWER_STATE_LP_IDLE_COMMS:
		p_module->state_preset = VL53L5_STATE_LOW_POWER;
		break;
	case VL53L5_POWER_STATE_HP_IDLE:
		p_module->state_preset = VL53L5_STATE_INITIALISED;
		break;
	default:
		vl53l5_k_log_error("Invalid state %i", p_module->power_state);

		p_module->state_preset = current_state;
		status = VL53L5_K_ERROR_INVALID_POWER_STATE;
		break;
	}

out:
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	if (p_module->stdev.host_dev.p_fw_buff != NULL) {
		vl53l5_k_log_debug("Release FW");
		release_firmware(fw_entry);
	}
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	if (fw_entry != NULL) {
		vl53l5_k_log_debug("Release Firmware");
		release_firmware(fw_entry);
	}
#endif
	p_module->stdev.host_dev.p_fw_buff = NULL;
	p_module->stdev.host_dev.fw_buff_count = 0;
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
#endif
out_state:
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
#endif
	LOG_FUNCTION_END(status);
	return status;
}

int vl53l5_ioctl_poll_data_ready(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;

	LOG_FUNCTION_START("");

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	status = _check_state(p_module, VL53L5_STATE_RANGING);
	if (status != STATUS_OK)
		goto out_state;

	status = _poll_for_new_data(
		p_module, p_module->polling_sleep_time_ms,
		VL53L5_MAX_CALIBRATION_POLL_TIME_MS);

	if (status != STATUS_OK) {
		if ((status == VL53L5_K_ERROR_DEVICE_NOT_INITIALISED) ||
			(status == VL53L5_K_ERROR_RANGE_POLLING_TIMEOUT))
			status = VL53L5_K_ERROR_DEVICE_NOT_RANGING;
		goto out;
	}
out:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}
out_state:
	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(status);
	return status;
}

int vl53l5_ioctl_read_p2p_calibration(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;
	unsigned int num_bh = VL53L5_K_P2P_BH_SZ;
	unsigned int *p_buffer[] = {&p2p_bh[0]};
	unsigned char *p_buff = NULL;
	unsigned int file_size = VL53L5_K_P2P_FILE_SIZE + 4 +
					VL53L5_K_DEVICE_INFO_SZ;

	LOG_FUNCTION_START("");

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	status = _check_state(p_module, VL53L5_STATE_INITIALISED);
	if (status != STATUS_OK) {
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
		if(!p_module->stdev.status_cal) p_module->stdev.status_cal = -10;
#endif
		goto out_state;
	}

	vl53l5_k_log_info("Read filename %s", VL53L5_CAL_P2P_FILENAME);
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	vl53l5_k_log_debug("Read file size %i", file_size);
	status = vl53l5_read_file(&p_module->stdev,
				p_module->stdev.host_dev.p_comms_buff,
				file_size,
				VL53L5_CAL_P2P_FILENAME);

	if (status < STATUS_OK) {
		vl53l5_k_log_error(
			"read file %s failed: %d ",
			VL53L5_CAL_P2P_FILENAME, status);
		goto out;
	}
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	vl53l5_k_log_info("Read filename %s", VL53L5_CAL_DATA_FILENAME);
	status = vl53l5_read_file(&p_module->stdev,
				p_module->stdev.host_dev.p_comms_buff,
				file_size,
				VL53L5_CAL_DATA_FILENAME);

	if (status < STATUS_OK) {
		vl53l5_k_log_error(
			"read file %s fail-%d ",
			VL53L5_CAL_DATA_FILENAME, status);
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
		if(!p_module->stdev.status_cal) p_module->stdev.status_cal = -11;
#endif
		goto out;
	}
#endif

	status = vl53l5_decode_calibration_data(
			&p_module->stdev,
			&p_module->calibration.cal_data,
			&p_module->calibration.info,
			&p_module->calibration.version,
			p_buffer[0],
			num_bh,
			VL53L5_K_P2P_FILE_SIZE + 4);

	if (status != STATUS_OK) {
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
		if(!p_module->stdev.status_cal) p_module->stdev.status_cal = -12;
#endif
		goto out;
	}
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	vl53l5_k_log_info("FGC: %s", (char *)p_module->calibration.info.fgc);
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	vl53l5_k_log_info("CAL ID : %u.%u",
			  p_module->calibration.info.module_id_hi,
			  p_module->calibration.info.module_id_lo);
#endif
	if ((p_module->calibration.info.module_id_hi
			== p_module->m_info.module_id_hi)
			&& (p_module->calibration.info.module_id_lo
			== p_module->m_info.module_id_lo)) {
		p_buff = p_module->stdev.host_dev.p_comms_buff;
		p_buff += VL53L5_K_DEVICE_INFO_SZ;
		status = vl53l5_set_device_parameters(&p_module->stdev, p_buff,
						VL53L5_K_P2P_FILE_SIZE);

		if (status != STATUS_OK) {
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
			if(!p_module->stdev.status_cal) p_module->stdev.status_cal = -13;
#endif			
			goto out;
		}
	} else {
		vl53l5_k_log_error("device and cal id was not matched %u.%u ",
			  p_module->m_info.module_id_hi,
			  p_module->m_info.module_id_lo);
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
		status = VL53L5_CONFIG_ERROR_INVALID_VERSION;
		if(!p_module->stdev.status_cal) p_module->stdev.status_cal = -14;
#endif
	}

out:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}
out_state:
	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(status);
	return status;
}

int vl53l5_ioctl_read_shape_calibration(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;
	unsigned int num_bh = VL53L5_K_SHAPE_CAL_BLOCK_HEADERS_SZ;
	unsigned int *p_buffer[] = {&shape_bh[0]};
	unsigned char *p_buff = NULL;
	unsigned int file_size = VL53L5_K_SHAPE_FILE_SIZE + 4 + VL53L5_K_DEVICE_INFO_SZ;

	LOG_FUNCTION_START("");

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	status = _check_state(p_module, VL53L5_STATE_INITIALISED);
	if (status != STATUS_OK)
		goto out_state;

	vl53l5_k_log_debug("Read filename %s", VL53L5_CAL_SHAPE_FILENAME);
	vl53l5_k_log_debug("Read file size %i", file_size);
	status = vl53l5_read_file(&p_module->stdev,
				p_module->stdev.host_dev.p_comms_buff,
				file_size,
				VL53L5_CAL_SHAPE_FILENAME);

	if (status < STATUS_OK) {
		vl53l5_k_log_error(
			"read file %s failed: %d ",
			VL53L5_CAL_SHAPE_FILENAME, status);
		goto out;
	}

	status = vl53l5_decode_calibration_data(
			&p_module->stdev,
			&p_module->calibration.cal_data,
			&p_module->calibration.info,
			&p_module->calibration.version,
			p_buffer[0],
			num_bh,
			VL53L5_K_SHAPE_FILE_SIZE + 4);

	if (status != STATUS_OK)
		goto out;

	p_buff = p_module->stdev.host_dev.p_comms_buff;
	p_buff += VL53L5_K_DEVICE_INFO_SZ;
	status = vl53l5_set_device_parameters(&p_module->stdev, p_buff,
					      VL53L5_K_SHAPE_FILE_SIZE);

	if (status != STATUS_OK)
		goto out;
out:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}
out_state:
	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(status);
	return status;
}

int vl53l5_perform_calibration(struct vl53l5_k_module_t *p_module, int flow)
{
	int status = STATUS_OK;
	int file_status = STATUS_OK;
	int refspad = 0;
	int xtalk = 0;
#ifdef VL53L5_OFFSET_CAL
	int offset = 0;
#endif

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	status = _check_state(p_module, VL53L5_STATE_INITIALISED);
	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (status != STATUS_OK)
		goto out_state;

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	switch (flow) {
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	case 0:
		refspad = VL53L5_CFG__REFSPAD;
		xtalk = VL53L5_CFG__XTALK_GEN2_600__8X8;
#ifdef VL53L5_OFFSET_CAL
		offset = VL53L5_CFG__OFFSET_600__8X8;
#endif
		break;
	case 1:
		refspad = VL53L5_CFG__REFSPAD;
		xtalk = VL53L5_CFG__XTALK_GEN1_600__8X8;
#ifdef VL53L5_OFFSET_CAL
		offset = VL53L5_CFG__OFFSET_600__8X8;
#endif
		break;
#endif
	case 2:
		refspad = VL53L5_CFG__REFSPAD;
		xtalk = VL53L5_CFG__XTALK_GEN2_300__8X8;
#ifdef VL53L5_OFFSET_CAL
		offset = VL53L5_CFG__OFFSET_300__8X8;
#endif
		break;
	case 3:
		refspad = VL53L5_CFG__REFSPAD;
		xtalk = VL53L5_CFG__XTALK_GEN1_1000__8X8;
#ifdef VL53L5_OFFSET_CAL
		offset = VL53L5_CFG__OFFSET_1000__8X8;
#endif
		break;
	default:
		vl53l5_k_log_error("Invalid cal flow: %d", flow);
		status = VL53L5_K_ERROR_INVALID_CALIBRATION_FLOW;
		goto out;
		break;
	};

	p_module->config_preset = refspad;
	status = _set_device_params(p_module);

	if (status != STATUS_OK)
		goto out;

	status = _start_poll_stop(p_module);

	if (status != STATUS_OK) {
		vl53l5_k_log_error("Refspad Failed");
		goto out;
	}

	p_module->config_preset = xtalk;
	status = _set_device_params(p_module);

	if (status != STATUS_OK)
		goto out;

	status = _start_poll_stop(p_module);

	if (status != STATUS_OK) {
		vl53l5_k_log_error("Xtalk Failed");
		goto out;
	}
#ifdef VL53L5_OFFSET_CAL

	p_module->config_preset = offset;
	status = _set_device_params(p_module);

	if (status != STATUS_OK)
		goto out;

	status = _start_poll_stop(p_module);

	if (status != STATUS_OK) {
		vl53l5_k_log_error("Offset Failed");
		goto out;
	}
#endif

	if (flow == 1 || flow == 3) {

		file_status = _write_shape_calibration(p_module);
		if (file_status != STATUS_OK)
			goto out;
	}
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
	file_status = _write_p2p_calibration(p_module);
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
	file_status = _write_p2p_calibration(p_module, flow);
#endif
	if (file_status != STATUS_OK)
		goto out;

out:
	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
	if (status != STATUS_OK) {

		vl53l5_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_debug("Unlock");
		mutex_unlock(&p_module->mutex);
		vl53l5_k_log_error("Failed: %d", status);
#if defined(VL53L5_TCDM_DUMP)
		if (status < VL53L5_K_ERROR_NOT_DEFINED) {
			vl53l5_k_log_info("TCDM Dump");

			vl53l5_k_log_debug("Lock");
			mutex_lock(&p_module->mutex);
			_tcdm_dump(p_module);
			vl53l5_k_log_debug("Unlock");
			mutex_unlock(&p_module->mutex);
		}
#endif
	}
	if (status == STATUS_OK && file_status != STATUS_OK)
		status = file_status;

out_state:
	LOG_FUNCTION_END(status);
	return status;
}

int vl53l5_ioctl_read_generic_shape(struct vl53l5_k_module_t *p_module)
{
	int status = STATUS_OK;
	int num_bh = VL53L5_K_SHAPE_CAL_BLOCK_HEADERS_SZ;
	int *p_buffer[] = {&shape_bh[0]};

	LOG_FUNCTION_START("");

	vl53l5_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	status = _check_state(p_module, VL53L5_STATE_INITIALISED);
	if (status != STATUS_OK) {
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
		if(!p_module->stdev.status_cal) p_module->stdev.status_cal = -5;
#endif
		goto out_state;
	}

	vl53l5_k_log_info("Read filename %s", VL53L5_GENERIC_SHAPE_FILENAME);
	vl53l5_k_log_debug("Read file size %i", VL53L5_K_SHAPE_FILE_SIZE + 4);
	status = vl53l5_read_file(&p_module->stdev,
				p_module->stdev.host_dev.p_comms_buff,
				VL53L5_K_SHAPE_FILE_SIZE + 4,
				VL53L5_GENERIC_SHAPE_FILENAME);

	if (status < STATUS_OK) {
		vl53l5_k_log_error(
			"read file %s failed: %d ",
			VL53L5_GENERIC_SHAPE_FILENAME, status);
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
		if(!p_module->stdev.status_cal) p_module->stdev.status_cal = -6;
#endif
		goto out;
	}

	status = vl53l5_decode_calibration_data(
			&p_module->stdev,
			&p_module->calibration.cal_data,
			NULL,
			NULL,
			p_buffer[0],
			num_bh,
			VL53L5_K_SHAPE_FILE_SIZE + 4);

	if (status != STATUS_OK) {
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
		if(!p_module->stdev.status_cal) p_module->stdev.status_cal = -7;
#endif
		goto out;
	}

	status = vl53l5_set_device_parameters(
		&p_module->stdev, p_module->stdev.host_dev.p_comms_buff,
		VL53L5_K_SHAPE_FILE_SIZE);

	if (status != STATUS_OK) {
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
		if(!p_module->stdev.status_cal) p_module->stdev.status_cal = -8;
#endif
		goto out;
	}

out:
	if (status != STATUS_OK) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
		vl53l5_k_log_error("Failed: %d", status);
	}
out_state:
	vl53l5_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(status);
	return status;
}
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
int vl53l5_k_power_onoff(struct vl53l5_k_module_t *data,
    struct regulator *vdd_vreg, const char *vdd_reg_name, bool on)
{
	int ret = 0;
	int voltage = 0;
	int reg_enabled = 0;

	if (vdd_reg_name) {
		if (vdd_vreg == NULL) {
			vdd_vreg = regulator_get(NULL, vdd_reg_name);
			if (IS_ERR(vdd_vreg)) {
				vdd_vreg = NULL;
				vl53l5_k_log_error("failed vdd_reg %s\n", vdd_reg_name);
			}
		}
	}

	if (vdd_vreg) {
		voltage = regulator_get_voltage(vdd_vreg);
		reg_enabled = regulator_is_enabled(vdd_vreg);
		vl53l5_k_log_info("vdd_reg reg_enabled=%d voltage=%d\n", reg_enabled, voltage);

		if (on) {
			if (reg_enabled == 0) {
				ret = regulator_enable(vdd_vreg);
				if (ret) {
					vl53l5_k_log_error("dvdd reg enable fail\n");
					return ret;
				}
			}
		} else {
			if (reg_enabled == 1) {
				ret = regulator_disable(vdd_vreg);
				if (ret) {
					vl53l5_k_log_error("dvdd reg disable fail\n");
					return ret;
				}
			}
		}
	}

	vl53l5_k_log_info("%s\n", on ? "on" : "off");

	return ret;
}
#endif
