
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
#include "cal_size.h"
#include "padding_size.h"
#include "ic_checkers_size.h"
#include "vl53l5_calibration_decode.h"
#include "vl53l5_calibration_enum_type.h"
#include "vl53l5_calibration_map_idx.h"
#include "vl53l5_calibration_dev_path.h"
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
static int32_t _decode_cal_grp__ref_spad_info(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct cal_grp__ref_spad_info_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_CAL_GRP_REF_SPAD_INFO_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->cal__ref_spad__offset =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__ref_spad__count =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__ref_spad__count_10x =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__ref_spad__count_100x =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__ref_spad__left_right_sel =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__ref_spad__status =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__ref_spad_info__pad_0 =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__ref_spad_info__pad_1 =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _decode_cal_grp__grid_meta(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct cal_grp__grid_meta_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_CAL_GRP_GRID_META_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->cal__grid_meta__distance_mm =
		vl53l5_decode_int16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	buff_count += BYTE_2;

	pstruct->cal__grid_meta__reflectance_pc =
		vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	buff_count += BYTE_2;

	pstruct->cal__grid_meta__silicon_temp_degc =
		vl53l5_decode_int8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__grid_meta__cols =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__grid_meta__rows =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__grid_meta__x_offset_spads =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__grid_meta__y_offset_spads =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__grid_meta__x_pitch_spads =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__grid_meta__y_pitch_spads =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__grid_meta__avg_count =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _decode_cal_grp__phase_stats(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct cal_grp__phase_stats_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_CAL_GRP_PHASE_STATS_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->cal__stats__avg_phase__ref =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->cal__stats__avg_phase__rtn =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->cal__stats__avg_phase__flex =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->cal__stats__avg_count__ref =
		vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	buff_count += BYTE_2;

	pstruct->cal__stats__avg_count__rtn =
		vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	buff_count += BYTE_2;

	pstruct->cal__stats__avg_count__flex =
		vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	buff_count += BYTE_2;

	pstruct->cal__stats__spare_0 =
		vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	buff_count += BYTE_2;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _decode_cal_grp__temperature_stats(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct cal_grp__temperature_stats_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_CAL_GRP_TEMPERATURE_STATS_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->cal__stats__avg_temp_degc__start__ref =
		vl53l5_decode_int8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__stats__avg_temp_degc__end__ref =
		vl53l5_decode_int8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__stats__avg_temp_degc__start__rtn =
		vl53l5_decode_int8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__stats__avg_temp_degc__end__rtn =
		vl53l5_decode_int8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _decode_cal_grp__grid_data__rate_kcps_per_spad_cal__grid_data__rate_kcps_per_spad(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct cal_grp__grid_data__rate_kcps_per_spad_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_CGGDRKPS_CAL_GRID_DATA_RATE_KCPS_PER_SPAD_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->cal__grid_data__rate_kcps_per_spad[i] =
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

static int32_t _decode_cal_grp__grid_data__effective_spad_count_cal__grid_data_effective_spad_count(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct cal_grp__grid_data__effective_spad_count_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_CGGDESC_CAL_GRID_DATA_EFFECTIVE_SPAD_COUNT_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->cal__grid_data_effective_spad_count[i] =
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

static int32_t _decode_cal_grp__grid_data__range_mm_cal__grid_data__range_mm(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct cal_grp__grid_data__range_mm_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_CGGDRM_CAL_GRID_DATA_RANGE_MM_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->cal__grid_data__range_mm[i] =
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

static int32_t _decode_cal_grp__status(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct cal_grp__status_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_CAL_GRP_STATUS_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->cal__status__type =
		vl53l5_decode_int32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->cal__status__stage_id =
		vl53l5_decode_int32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->cal__target__idx =
		vl53l5_decode_int32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->cal__zone_id =
		vl53l5_decode_int32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _decode_cal_grp__xtalk_shape_meta(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct cal_grp__xtalk_shape_meta_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_CAL_GRP_XTALK_SHAPE_META_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->cal__xtalk_shape__median_phase =
		vl53l5_decode_uint32_t(BYTE_4, p_buff);
	p_buff += BYTE_4;
	buff_count += BYTE_4;

	pstruct->cal__xtalk_shape__avg_count =
		vl53l5_decode_uint16_t(BYTE_2, p_buff);
	p_buff += BYTE_2;
	buff_count += BYTE_2;

	pstruct->cal__xtalk_shape__no_of_bins =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__xtalk_shape__normalisation_power =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__xtalk_shape__silicon_temp_degc =
		vl53l5_decode_int8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__xtalk_shape__spare_0 =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__xtalk_shape__spare_1 =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__xtalk_shape__spare_2 =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _decode_cal_grp__xtalk_shape_data_cal__xtalk_shape__bin_data(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct cal_grp__xtalk_shape_data_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_CGXSD_CAL_XTALK_SHAPE_BIN_DATA_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->cal__xtalk_shape__bin_data[i] =
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

static int32_t _decode_cal_grp__xtalk_mon__meta_data(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct cal_grp__xtalk_mon__meta_data_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;

	LOG_FUNCTION_START("");

	if (buffer_size >
	    (uint32_t)VL53L5_CAL_GRP_XTALK_MON_META_DATA_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	pstruct->cal__xmon__max_zones =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__xmon__no_of_zones =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__xmon__zone_idx =
		vl53l5_decode_uint8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	pstruct->cal__xmon__silicon_temp_degc =
		vl53l5_decode_int8_t(BYTE_1, p_buff);
	p_buff += BYTE_1;
	buff_count += BYTE_1;

	if (buffer_size != buff_count)
		status = VL53L5_DATA_SIZE_MISMATCH;

exit:
	LOG_FUNCTION_END(status);
	return status;
}

static int32_t _decode_cal_grp__xtalk_mon__zones_cal__xmon__zone__x_off(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct cal_grp__xtalk_mon__zones_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_CGXMZ_CAL_XMON_ZONE_X_OFF_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->cal__xmon__zone__x_off[i] =
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

static int32_t _decode_cal_grp__xtalk_mon__zones_cal__xmon__zone__y_off(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct cal_grp__xtalk_mon__zones_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_CGXMZ_CAL_XMON_ZONE_Y_OFF_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->cal__xmon__zone__y_off[i] =
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

static int32_t _decode_cal_grp__xtalk_mon__zones_cal__xmon__zone__width(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct cal_grp__xtalk_mon__zones_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_CGXMZ_CAL_XMON_ZONE_WIDTH_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->cal__xmon__zone__width[i] =
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

static int32_t _decode_cal_grp__xtalk_mon__zones_cal__xmon__zone__height(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct cal_grp__xtalk_mon__zones_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_CGXMZ_CAL_XMON_ZONE_HEIGHT_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->cal__xmon__zone__height[i] =
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

static int32_t _decode_cal_grp__xtalk_mon__data_cal__xmon__zone__rate_kcps_spad(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct cal_grp__xtalk_mon__data_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_CGXMD_CAL_XMON_ZONE_RATE_KCPS_SPAD_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->cal__xmon__zone__rate_kcps_spad[i] =
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

static int32_t _decode_cal_grp__xtalk_mon__data_cal__xmon__zone__avg_count(
	uint32_t buffer_size,
	uint8_t *buffer,
	struct cal_grp__xtalk_mon__data_t *pstruct)
{
	int32_t status = STATUS_OK;
	uint32_t buff_count = 0;
	uint8_t *p_buff = buffer;
	uint32_t i = 0;

	LOG_FUNCTION_START("");

	if (buffer_size >
		(uint32_t)VL53L5_CGXMD_CAL_XMON_ZONE_AVG_COUNT_SZ) {
		status = VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA;
		goto exit;
	}

	while (buff_count < buffer_size) {
		pstruct->cal__xmon__zone__avg_count[i] =
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

int32_t vl53l5_calibration_decode_cmd(
	uint16_t idx,
	uint32_t buffer_size,
	uint8_t *buffer,
	struct vl53l5_dev_handle_t *p_dev)
{
	int32_t status = STATUS_OK;

	LOG_FUNCTION_START("");

	switch (idx) {
	case DEV_CAL_REF_SPAD_INFO_IDX:
		status =
		_decode_cal_grp__ref_spad_info(
			buffer_size,
			buffer,
			&VL53L5_CAL_REF_SPAD_INFO(p_dev));
		break;

	case DEV_POFFSET_GRID_META_IDX:
		status =
		_decode_cal_grp__grid_meta(
			buffer_size,
			buffer,
			&VL53L5_POFFSET_GRID_META(p_dev));
		break;
	case DEV_POFFSET_PHASE_STATS_IDX:
		status =
		_decode_cal_grp__phase_stats(
			buffer_size,
			buffer,
			&VL53L5_POFFSET_PHASE_STATS(p_dev));
		break;
	case DEV_POFFSET_TEMPERATURE_STATS_IDX:
		status =
		_decode_cal_grp__temperature_stats(
			buffer_size,
			buffer,
			&VL53L5_POFFSET_TEMPERATURE_STATS(p_dev));
		break;
	case DEV_POFFSET_GRID_RATE_CAL__GRID_DATA__RATE_KCPS_PER_SPAD_IDX:
		status =
		_decode_cal_grp__grid_data__rate_kcps_per_spad_cal__grid_data__rate_kcps_per_spad(
			buffer_size,
			buffer,
			&VL53L5_POFFSET_GRID_RATE(p_dev));
		break;
	case DEV_POFFSET_GRID_SPADS_CAL__GRID_DATA_EFFECTIVE_SPAD_COUNT_IDX:
		status =
		_decode_cal_grp__grid_data__effective_spad_count_cal__grid_data_effective_spad_count(
			buffer_size,
			buffer,
			&VL53L5_POFFSET_GRID_SPADS(p_dev));
		break;
	case DEV_POFFSET_GRID_OFFSET_CAL__GRID_DATA__RANGE_MM_IDX:
		status =
		_decode_cal_grp__grid_data__range_mm_cal__grid_data__range_mm(
			buffer_size,
			buffer,
			&VL53L5_POFFSET_GRID_OFFSET(p_dev));
		break;
	case DEV_POFFSET_ERROR_STATUS_IDX:
		status =
		_decode_cal_grp__status(
			buffer_size,
			buffer,
			&VL53L5_POFFSET_ERROR_STATUS(p_dev));
		break;
	case DEV_POFFSET_WARNING_STATUS_IDX:
		status =
		_decode_cal_grp__status(
			buffer_size,
			buffer,
			&VL53L5_POFFSET_WARNING_STATUS(p_dev));
		break;

	case DEV_PXTALK_GRID_META_IDX:
		status =
		_decode_cal_grp__grid_meta(
			buffer_size,
			buffer,
			&VL53L5_PXTALK_GRID_META(p_dev));
		break;
	case DEV_PXTALK_PHASE_STATS_IDX:
		status =
		_decode_cal_grp__phase_stats(
			buffer_size,
			buffer,
			&VL53L5_PXTALK_PHASE_STATS(p_dev));
		break;
	case DEV_PXTALK_TEMPERATURE_STATS_IDX:
		status =
		_decode_cal_grp__temperature_stats(
			buffer_size,
			buffer,
			&VL53L5_PXTALK_TEMPERATURE_STATS(p_dev));
		break;
	case DEV_PXTALK_GRID_RATE_CAL__GRID_DATA__RATE_KCPS_PER_SPAD_IDX:
		status =
		_decode_cal_grp__grid_data__rate_kcps_per_spad_cal__grid_data__rate_kcps_per_spad(
			buffer_size,
			buffer,
			&VL53L5_PXTALK_GRID_RATE(p_dev));
		break;
	case DEV_PXTALK_GRID_SPADS_CAL__GRID_DATA_EFFECTIVE_SPAD_COUNT_IDX:
		status =
		_decode_cal_grp__grid_data__effective_spad_count_cal__grid_data_effective_spad_count(
			buffer_size,
			buffer,
			&VL53L5_PXTALK_GRID_SPADS(p_dev));
		break;
	case DEV_PXTALK_ERROR_STATUS_IDX:
		status =
		_decode_cal_grp__status(
			buffer_size,
			buffer,
			&VL53L5_PXTALK_ERROR_STATUS(p_dev));
		break;
	case DEV_PXTALK_WARNING_STATUS_IDX:
		status =
		_decode_cal_grp__status(
			buffer_size,
			buffer,
			&VL53L5_PXTALK_WARNING_STATUS(p_dev));
		break;

	case DEV_PXTALK_SHAPE_META_IDX:
		status =
		_decode_cal_grp__xtalk_shape_meta(
			buffer_size,
			buffer,
			&VL53L5_PXTALK_SHAPE_META(p_dev));
		break;
	case DEV_PXTALK_SHAPE_DATA_CAL__XTALK_SHAPE__BIN_DATA_IDX:
		status =
		_decode_cal_grp__xtalk_shape_data_cal__xtalk_shape__bin_data(
			buffer_size,
			buffer,
			&VL53L5_PXTALK_SHAPE_DATA(p_dev));
		break;
	case DEV_PXTALK_MON_META_IDX:
		status =
		_decode_cal_grp__xtalk_mon__meta_data(
			buffer_size,
			buffer,
			&VL53L5_PXTALK_MON_META(p_dev));
		break;
	case DEV_PXTALK_MON_ZONES_CAL__XMON__ZONE__X_OFF_IDX:
		status =
		_decode_cal_grp__xtalk_mon__zones_cal__xmon__zone__x_off(
			buffer_size,
			buffer,
			&VL53L5_PXTALK_MON_ZONES(p_dev));
		break;
	case DEV_PXTALK_MON_ZONES_CAL__XMON__ZONE__Y_OFF_IDX:
		status =
		_decode_cal_grp__xtalk_mon__zones_cal__xmon__zone__y_off(
			buffer_size,
			buffer,
			&VL53L5_PXTALK_MON_ZONES(p_dev));
		break;
	case DEV_PXTALK_MON_ZONES_CAL__XMON__ZONE__WIDTH_IDX:
		status =
		_decode_cal_grp__xtalk_mon__zones_cal__xmon__zone__width(
			buffer_size,
			buffer,
			&VL53L5_PXTALK_MON_ZONES(p_dev));
		break;
	case DEV_PXTALK_MON_ZONES_CAL__XMON__ZONE__HEIGHT_IDX:
		status =
		_decode_cal_grp__xtalk_mon__zones_cal__xmon__zone__height(
			buffer_size,
			buffer,
			&VL53L5_PXTALK_MON_ZONES(p_dev));
		break;
	case DEV_PXTALK_MON_DATA_CAL__XMON__ZONE__RATE_KCPS_SPAD_IDX:
		status =
		_decode_cal_grp__xtalk_mon__data_cal__xmon__zone__rate_kcps_spad(
			buffer_size,
			buffer,
			&VL53L5_PXTALK_MON_DATA(p_dev));
		break;
	case DEV_PXTALK_MON_DATA_CAL__XMON__ZONE__AVG_COUNT_IDX:
		status =
		_decode_cal_grp__xtalk_mon__data_cal__xmon__zone__avg_count(
			buffer_size,
			buffer,
			&VL53L5_PXTALK_MON_DATA(p_dev));
		break;

	default:
		status = VL53L5_INVALID_IDX_DECODE_CMD_ERROR;
		break;
	}

	LOG_FUNCTION_END(status);
	return status;

}
