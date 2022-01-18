
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

#include "dci_size.h"
#include "dci_ui_size.h"
#include "common_datatype_size.h"
#include "dyn_xtalk_size.h"
#include "ic_checkers_size.h"
#include "vl53l5_results_decode.h"
#include "vl53l5_results_enum_type.h"
#include "vl53l5_results_map_idx.h"
#include "vl53l5_results_dev_path.h"
#include "vl53l5_dci_utils.h"
#include "vl53l5_error_codes.h"
#include "vl53l5_platform_log.h"

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START( \
		VL53L5_TRACE_MODULE_DCI_DECODE, fmt, ##__VA_ARGS__)

#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END( \
		VL53L5_TRACE_MODULE_DCI_DECODE, status, ##__VA_ARGS__)

#define LOG_FUNCTION_END_FMT(status, ...) \
	_LOG_FUNCTION_END_FMT( \
		VL53L5_TRACE_MODULE_DCI_DECODE, status, ##__VA_ARGS__)
#ifdef VL53L5_META_DATA_ON
static int32_t _decode_dci_grp__buf_meta_data(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_meta_data_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_BUF_META_DATA_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->time_stamp =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->device_status =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->transaction_id =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->buffer_id =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->stream_count =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->silicon_temp_degc =
		vl53l5_decode_int8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->buf_meta_data__pad_0 =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->buf_meta_data__pad_1 =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->buf_meta_data__pad_2 =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_COMMON_DATA_ON
static int32_t _decode_dci_grp__rng_common_data(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_common_data_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_RNG_COMMON_DATA_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->wrap_dmax_mm =
		vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	buff_count += BYTE_2;

	pstruct->rng__no_of_zones =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->rng__max_targets_per_zone =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_RNG_TIMING_DATA_ON
static int32_t _decode_dci_grp__rng_timing_data(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct dci_grp__rng_timing_data_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_RNG_TIMING_DATA_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->rng__integration_time_us =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->rng__total_periods_elapsed =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->rng__blanking_time_us =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_AMB_RATE_KCPS_PER_SPAD_ON
static int32_t _decode_dci_grp__rng_per_zone_data_amb_rate_kcps_per_spad(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_per_zone_results_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_RPZD_AMB_RATE_KCPS_PER_SPAD_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->amb_rate_kcps_per_spad[i] =
			vl53l5_decode_uint32_t(BYTE_4, p_buff);
		p_buff += BYTE_4;
		buff_count += BYTE_4;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_EFFECTIVE_SPAD_COUNT_ON
static int32_t _decode_dci_grp__rng_per_zone_data_rng__effective_spad_count(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_per_zone_results_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_RPZD_RNG_EFFECTIVE_SPAD_COUNT_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->rng__effective_spad_count[i] =
			vl53l5_decode_uint32_t(BYTE_4, p_buff);
		p_buff += BYTE_4;
		buff_count += BYTE_4;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_AMB_DMAX_MM_ON
static int32_t _decode_dci_grp__rng_per_zone_data_amb_dmax_mm(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_per_zone_results_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_RPZD_AMB_DMAX_MM_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->amb_dmax_mm[i] =
			vl53l5_decode_uint16_t(BYTE_2, p_buff);
		p_buff += BYTE_2;
		buff_count += BYTE_2;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_SILICON_TEMP_DEGC_START_ON
static int32_t _decode_dci_grp__rng_per_zone_data_silicon_temp_degc__start(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_per_zone_results_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_RPZD_SILICON_TEMP_DEGC_START_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->silicon_temp_degc__start[i] =
			vl53l5_decode_int8_t(BYTE_1, p_buff);
		p_buff += BYTE_1;
		buff_count += BYTE_1;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_SILICON_TEMP_DEGC_END_ON
static int32_t _decode_dci_grp__rng_per_zone_data_silicon_temp_degc__end(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_per_zone_results_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_RPZD_SILICON_TEMP_DEGC_END_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->silicon_temp_degc__end[i] =
			vl53l5_decode_int8_t(BYTE_1, p_buff);
		p_buff += BYTE_1;
		buff_count += BYTE_1;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_NO_OF_TARGETS_ON
static int32_t _decode_dci_grp__rng_per_zone_data_rng__no_of_targets(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_per_zone_results_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_RPZD_RNG_NO_OF_TARGETS_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->rng__no_of_targets[i] =
			vl53l5_decode_uint8_t(BYTE_1, p_buff);
		p_buff += BYTE_1;
		buff_count += BYTE_1;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_ZONE_ID_ON
static int32_t _decode_dci_grp__rng_per_zone_data_rng__zone_id(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_per_zone_results_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_RPZD_RNG_ZONE_ID_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->rng__zone_id[i] =
			vl53l5_decode_uint8_t(BYTE_1, p_buff);
		p_buff += BYTE_1;
		buff_count += BYTE_1;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_SEQUENCE_IDX_ON
static int32_t _decode_dci_grp__rng_per_zone_data_rng__sequence_idx(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_per_zone_results_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_RPZD_RNG_SEQUENCE_IDX_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->rng__sequence_idx[i] =
			vl53l5_decode_uint8_t(BYTE_1, p_buff);
		p_buff += BYTE_1;
		buff_count += BYTE_1;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_PEAK_RATE_KCPS_PER_SPAD_ON
static int32_t _decode_dci_grp__rng_per_target_data_peak_rate_kcps_per_spad(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_per_tgt_results_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_RPTD_PEAK_RATE_KCPS_PER_SPAD_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->peak_rate_kcps_per_spad[i] =
			vl53l5_decode_uint32_t(BYTE_4, p_buff);
		p_buff += BYTE_4;
		buff_count += BYTE_4;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_MEDIAN_PHASE_ON
static int32_t _decode_dci_grp__rng_per_target_data_median_phase(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_per_tgt_results_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_RPTD_MEDIAN_PHASE_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->median_phase[i] =
			vl53l5_decode_uint32_t(BYTE_4, p_buff);
		p_buff += BYTE_4;
		buff_count += BYTE_4;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_RATE_SIGMA_KCPS_PER_SPAD_ON
static int32_t _decode_dci_grp__rng_per_target_data_rate_sigma_kcps_per_spad(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_per_tgt_results_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_RPTD_RATE_SIGMA_KCPS_PER_SPAD_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->rate_sigma_kcps_per_spad[i] =
			vl53l5_decode_uint32_t(BYTE_4, p_buff);
		p_buff += BYTE_4;
		buff_count += BYTE_4;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_RANGE_SIGMA_MM_ON
static int32_t _decode_dci_grp__rng_per_target_data_range_sigma_mm(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_per_tgt_results_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_RPTD_RANGE_SIGMA_MM_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->range_sigma_mm[i] =
			vl53l5_decode_uint16_t(BYTE_2, p_buff);
		p_buff += BYTE_2;
		buff_count += BYTE_2;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_MEDIAN_RANGE_MM_ON
static int32_t _decode_dci_grp__rng_per_target_data_median_range_mm(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_per_tgt_results_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_RPTD_MEDIAN_RANGE_MM_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->median_range_mm[i] =
			vl53l5_decode_int16_t(BYTE_2, p_buff);
		p_buff += BYTE_2;
		buff_count += BYTE_2;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_MIN_RANGE_DELTA_MM_ON
static int32_t _decode_dci_grp__rng_per_target_data_min_range_delta_mm(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_per_tgt_results_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_RPTD_MIN_RANGE_DELTA_MM_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->min_range_delta_mm[i] =
			vl53l5_decode_uint8_t(BYTE_1, p_buff);
		p_buff += BYTE_1;
		buff_count += BYTE_1;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_MAX_RANGE_DELTA_MM_ON
static int32_t _decode_dci_grp__rng_per_target_data_max_range_delta_mm(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_per_tgt_results_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_RPTD_MAX_RANGE_DELTA_MM_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->max_range_delta_mm[i] =
			vl53l5_decode_uint8_t(BYTE_1, p_buff);
		p_buff += BYTE_1;
		buff_count += BYTE_1;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_TARGET_REFLECTANCE_EST_PC_ON
static int32_t _decode_dci_grp__rng_per_target_data_target_reflectance_est_pc(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_per_tgt_results_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_RPTD_TARGET_REFLECTANCE_EST_PC_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->target_reflectance_est_pc[i] =
			vl53l5_decode_uint8_t(BYTE_1, p_buff);
		p_buff += BYTE_1;
		buff_count += BYTE_1;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_TARGET_STATUS_ON
static int32_t _decode_dci_grp__rng_per_target_data_target_status(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_range_per_tgt_results_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_RPTD_TARGET_STATUS_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->target_status[i] =
			vl53l5_decode_uint8_t(BYTE_1, p_buff);
		p_buff += BYTE_1;
		buff_count += BYTE_1;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_REF_CHANNEL_DATA_ON
static int32_t _decode_dci_grp__ref_channel_data(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct dci_grp__ref_channel_data_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_REF_CHANNEL_DATA_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->amb_rate_kcps_per_spad =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->rng__effective_spad_count =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->amb_dmax_mm =
		vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	buff_count += BYTE_2;

	pstruct->silicon_temp_degc__start =
		vl53l5_decode_int8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->silicon_temp_degc__end =
		vl53l5_decode_int8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->rng__no_of_targets =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->rng__zone_id =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->rng__sequence_idx =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->ref_channel_data__pad_0 =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_REF_TARGET_DATA_ON
static int32_t _decode_dci_grp__ref_target_data(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct dci_grp__ref_target_data_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_REF_TARGET_DATA_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->peak_rate_kcps_per_spad =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->median_phase =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->rate_sigma_kcps_per_spad =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->range_sigma_mm =
		vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	buff_count += BYTE_2;

	pstruct->median_range_mm =
		vl53l5_decode_int16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	buff_count += BYTE_2;

	pstruct->min_range_delta_mm =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->max_range_delta_mm =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->target_reflectance_est_pc =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->target_status =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_ZONE_THRESH_STATUS_BYTES_ON
static int32_t _decode_dci_grp__zone_thresh_status_array_zone_thresh_status_bytes(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct dci_grp__zone_thresh_status_array_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_ZTSA_ZONE_THRESH_STATUS_BYTES_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->zone_thresh_status_bytes[i] =
			vl53l5_decode_uint8_t(BYTE_1, p_buff);
		p_buff += BYTE_1;
		buff_count += BYTE_1;
		i++;
	}

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

#ifdef VL53L5_DYN_XTALK_OP_PERSISTENT_DATA_ON
static int32_t _decode_dci_grp__dyn_xtalk_persistent_data(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct dci_grp__dyn_xtalk_persistent_data_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_DYN_XTALK_PERSISTENT_DATA_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->dyn_xt__dyn_xtalk_grid_maximum_kcps_per_spad =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->dyn_xt__dyn_xtalk_grid_max_sigma_kcps_per_spad =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->dyn_xt__new_max_xtalk_sigma_kcps_per_spad =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->dyn_xt__calibration_gain =
		vl53l5_decode_int32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->temp_comp__temp_gain =
		vl53l5_decode_int32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->dyn_xt__nb_samples =
		vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	buff_count += BYTE_2;

	pstruct->dyn_xt__desired_samples =
		vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	buff_count += BYTE_2;

	pstruct->dyn_xt__accumulating =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->dyn_xt__grid_ready =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->dyn_xt__persist_spare_1 =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->dyn_xt__persist_spare_2 =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}
#endif

int32_t vl53l5_results_decode_cmd(
	uint16_t idx,
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	switch (idx) {
#ifdef VL53L5_META_DATA_ON
	case DEV_RNG_META_DATA_IDX:
		status =
		_decode_dci_grp__buf_meta_data(
			buffer_size,
			buffer,
			&VL53L5_META_DATA(p_dev));
		break;
#endif

#ifdef VL53L5_COMMON_DATA_ON
	case DEV_RNG_COMMON_DATA_IDX:
		status =
		_decode_dci_grp__rng_common_data(
			buffer_size,
			buffer,
			&VL53L5_COMMON_DATA(p_dev));
		break;
#endif

#ifdef VL53L5_RNG_TIMING_DATA_ON
	case DEV_RNG_TIMING_DATA_IDX:
		status =
		_decode_dci_grp__rng_timing_data(
			buffer_size,
			buffer,
			&VL53L5_RNG_TIMING_DATA(p_dev));
		break;
#endif

#ifdef VL53L5_AMB_RATE_KCPS_PER_SPAD_ON
	case DEV_RNG_PER_ZONE_DATA_AMB_RATE_KCPS_PER_SPAD_IDX:
		status =
		_decode_dci_grp__rng_per_zone_data_amb_rate_kcps_per_spad(
			buffer_size,
			buffer,
			&VL53L5_PER_ZONE_RESULTS(p_dev));
		break;
#endif

#ifdef VL53L5_EFFECTIVE_SPAD_COUNT_ON
	case DEV_RNG_PER_ZONE_DATA_RNG__EFFECTIVE_SPAD_COUNT_IDX:
		status =
		_decode_dci_grp__rng_per_zone_data_rng__effective_spad_count(
			buffer_size,
			buffer,
			&VL53L5_PER_ZONE_RESULTS(p_dev));
		break;
#endif

#ifdef VL53L5_AMB_DMAX_MM_ON
	case DEV_RNG_PER_ZONE_DATA_AMB_DMAX_MM_IDX:
		status =
		_decode_dci_grp__rng_per_zone_data_amb_dmax_mm(
			buffer_size,
			buffer,
			&VL53L5_PER_ZONE_RESULTS(p_dev));
		break;
#endif

#ifdef VL53L5_SILICON_TEMP_DEGC_START_ON
	case DEV_RNG_PER_ZONE_DATA_SILICON_TEMP_DEGC__START_IDX:
		status =
		_decode_dci_grp__rng_per_zone_data_silicon_temp_degc__start(
			buffer_size,
			buffer,
			&VL53L5_PER_ZONE_RESULTS(p_dev));
		break;
#endif

#ifdef VL53L5_SILICON_TEMP_DEGC_END_ON
	case DEV_RNG_PER_ZONE_DATA_SILICON_TEMP_DEGC__END_IDX:
		status =
		_decode_dci_grp__rng_per_zone_data_silicon_temp_degc__end(
			buffer_size,
			buffer,
			&VL53L5_PER_ZONE_RESULTS(p_dev));
		break;
#endif

#ifdef VL53L5_NO_OF_TARGETS_ON
	case DEV_RNG_PER_ZONE_DATA_RNG__NO_OF_TARGETS_IDX:
		status =
		_decode_dci_grp__rng_per_zone_data_rng__no_of_targets(
			buffer_size,
			buffer,
			&VL53L5_PER_ZONE_RESULTS(p_dev));
		break;
#endif

#ifdef VL53L5_ZONE_ID_ON
	case DEV_RNG_PER_ZONE_DATA_RNG__ZONE_ID_IDX:
		status =
		_decode_dci_grp__rng_per_zone_data_rng__zone_id(
			buffer_size,
			buffer,
			&VL53L5_PER_ZONE_RESULTS(p_dev));
		break;
#endif

#ifdef VL53L5_SEQUENCE_IDX_ON
	case DEV_RNG_PER_ZONE_DATA_RNG__SEQUENCE_IDX_IDX:
		status =
		_decode_dci_grp__rng_per_zone_data_rng__sequence_idx(
			buffer_size,
			buffer,
			&VL53L5_PER_ZONE_RESULTS(p_dev));
		break;
#endif

#ifdef VL53L5_PEAK_RATE_KCPS_PER_SPAD_ON
	case DEV_RNG_PER_TARGET_DATA_PEAK_RATE_KCPS_PER_SPAD_IDX:
		status =
		_decode_dci_grp__rng_per_target_data_peak_rate_kcps_per_spad(
			buffer_size,
			buffer,
			&VL53L5_PER_TGT_RESULTS(p_dev));
		break;
#endif

#ifdef VL53L5_MEDIAN_PHASE_ON
	case DEV_RNG_PER_TARGET_DATA_MEDIAN_PHASE_IDX:
		status =
		_decode_dci_grp__rng_per_target_data_median_phase(
			buffer_size,
			buffer,
			&VL53L5_PER_TGT_RESULTS(p_dev));
		break;
#endif

#ifdef VL53L5_RATE_SIGMA_KCPS_PER_SPAD_ON
	case DEV_RNG_PER_TARGET_DATA_RATE_SIGMA_KCPS_PER_SPAD_IDX:
		status =
		_decode_dci_grp__rng_per_target_data_rate_sigma_kcps_per_spad(
			buffer_size,
			buffer,
			&VL53L5_PER_TGT_RESULTS(p_dev));
		break;
#endif

#ifdef VL53L5_RANGE_SIGMA_MM_ON
	case DEV_RNG_PER_TARGET_DATA_RANGE_SIGMA_MM_IDX:
		status =
		_decode_dci_grp__rng_per_target_data_range_sigma_mm(
			buffer_size,
			buffer,
			&VL53L5_PER_TGT_RESULTS(p_dev));
		break;
#endif

#ifdef VL53L5_MEDIAN_RANGE_MM_ON
	case DEV_RNG_PER_TARGET_DATA_MEDIAN_RANGE_MM_IDX:
		status =
		_decode_dci_grp__rng_per_target_data_median_range_mm(
			buffer_size,
			buffer,
			&VL53L5_PER_TGT_RESULTS(p_dev));
		break;
#endif

#ifdef VL53L5_MIN_RANGE_DELTA_MM_ON
	case DEV_RNG_PER_TARGET_DATA_MIN_RANGE_DELTA_MM_IDX:
		status =
		_decode_dci_grp__rng_per_target_data_min_range_delta_mm(
			buffer_size,
			buffer,
			&VL53L5_PER_TGT_RESULTS(p_dev));
		break;
#endif

#ifdef VL53L5_MAX_RANGE_DELTA_MM_ON
	case DEV_RNG_PER_TARGET_DATA_MAX_RANGE_DELTA_MM_IDX:
		status =
		_decode_dci_grp__rng_per_target_data_max_range_delta_mm(
			buffer_size,
			buffer,
			&VL53L5_PER_TGT_RESULTS(p_dev));
		break;
#endif

#ifdef VL53L5_TARGET_REFLECTANCE_EST_PC_ON
	case DEV_RNG_PER_TARGET_DATA_TARGET_REFLECTANCE_EST_PC_IDX:
		status =
		_decode_dci_grp__rng_per_target_data_target_reflectance_est_pc(
			buffer_size,
			buffer,
			&VL53L5_PER_TGT_RESULTS(p_dev));
		break;
#endif

#ifdef VL53L5_TARGET_STATUS_ON
	case DEV_RNG_PER_TARGET_DATA_TARGET_STATUS_IDX:
		status =
		_decode_dci_grp__rng_per_target_data_target_status(
			buffer_size,
			buffer,
			&VL53L5_PER_TGT_RESULTS(p_dev));
		break;
#endif

#ifdef VL53L5_REF_TIMING_DATA_ON
	case DEV_REF_TIMING_DATA_IDX:
		status =
		_decode_dci_grp__rng_timing_data(
			buffer_size,
			buffer,
			&VL53L5_REF_TIMING_DATA(p_dev));
		break;
#endif

#ifdef VL53L5_REF_CHANNEL_DATA_ON
	case DEV_REF_CHANNEL_DATA_IDX:
		status =
		_decode_dci_grp__ref_channel_data(
			buffer_size,
			buffer,
			&VL53L5_REF_CHANNEL_DATA(p_dev));
		break;
#endif

#ifdef VL53L5_REF_TARGET_DATA_ON
	case DEV_REF_TARGET_DATA_IDX:
		status =
		_decode_dci_grp__ref_target_data(
			buffer_size,
			buffer,
			&VL53L5_REF_TARGET_DATA(p_dev));
		break;
#endif

#ifdef VL53L5_ZONE_THRESH_STATUS_BYTES_ON
	case DEV_ZONE_THRESH_STATUS_ZONE_THRESH_STATUS_BYTES_IDX:
		status =
		_decode_dci_grp__zone_thresh_status_array_zone_thresh_status_bytes(
			buffer_size,
			buffer,
			&VL53L5_ZONE_THRESH_STATUS(p_dev));
		break;
#endif

#ifdef VL53L5_DYN_XTALK_OP_PERSISTENT_DATA_ON
	case DEV_DYN_XTALK_OP_PERSISTENT_DATA_IDX:
		status =
		_decode_dci_grp__dyn_xtalk_persistent_data(
			buffer_size,
			buffer,
			&VL53L5_DYN_XTALK_OP_PERSISTENT_DATA(p_dev));
		break;
#endif

	default:
		status = VL53L5_INVALID_IDX_DECODE_CMD_ERROR;
		break;
	}

	LOG_FUNCTION_END(status);
	return status;

}
