
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

#ifndef __CAL_STRUCTS_H__
#define __CAL_STRUCTS_H__

#include "cal_defs.h"
#include "cal_luts.h"
#include "packing_structs.h"
#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct cal_grp__status_t {

	int32_t cal__status__type;

	int32_t cal__status__stage_id;

	int32_t cal__target__idx;

	int32_t cal__zone_id;
};

struct cal_grp__ref_spad_info_t {

	uint8_t cal__ref_spad__offset;

	uint8_t cal__ref_spad__count;

	uint8_t cal__ref_spad__count_10x;

	uint8_t cal__ref_spad__count_100x;

	uint8_t cal__ref_spad__left_right_sel;

	uint8_t cal__ref_spad__status;
	uint8_t cal__ref_spad_info__pad_0;
	uint8_t cal__ref_spad_info__pad_1;
};

struct cal_grp__grid_meta_t {

	int16_t cal__grid_meta__distance_mm;

	uint16_t cal__grid_meta__reflectance_pc;

	int8_t cal__grid_meta__silicon_temp_degc;

	uint8_t cal__grid_meta__cols;

	uint8_t cal__grid_meta__rows;

	uint8_t cal__grid_meta__x_offset_spads;

	uint8_t cal__grid_meta__y_offset_spads;

	uint8_t cal__grid_meta__x_pitch_spads;

	uint8_t cal__grid_meta__y_pitch_spads;

	uint8_t cal__grid_meta__avg_count;
};

struct cal_grp__grid_data__rate_kcps_per_spad_t {

	uint32_t cal__grid_data__rate_kcps_per_spad[
		CAL_DEF__MAX_COLS_X_MAX_ROWS];
};

struct cal_grp__grid_data__effective_spad_count_t {

	uint16_t cal__grid_data_effective_spad_count[
		CAL_DEF__MAX_COLS_X_MAX_ROWS];
};

struct cal_grp__grid_data__range_mm_t {

	int16_t cal__grid_data__range_mm[CAL_DEF__MAX_COLS_X_MAX_ROWS];
};

struct cal_grp__xtalk_shape_meta_t {

	uint32_t cal__xtalk_shape__median_phase;

	uint16_t cal__xtalk_shape__avg_count;

	uint8_t cal__xtalk_shape__no_of_bins;

	uint8_t cal__xtalk_shape__normalisation_power;

	int8_t cal__xtalk_shape__silicon_temp_degc;
	uint8_t cal__xtalk_shape__spare_0;
	uint8_t cal__xtalk_shape__spare_1;
	uint8_t cal__xtalk_shape__spare_2;
};

struct cal_grp__xtalk_shape_data_t {

	uint16_t cal__xtalk_shape__bin_data[CAL_DEF__MAX_XTALK_SHAPE_BINS];
};

struct cal_grp__xtalk_mon__meta_data_t {

	uint8_t cal__xmon__max_zones;

	uint8_t cal__xmon__no_of_zones;

	uint8_t cal__xmon__zone_idx;

	int8_t cal__xmon__silicon_temp_degc;
};

struct cal_grp__xtalk_mon__zones_t {

	uint8_t cal__xmon__zone__x_off[CAL_DEF__XTALK_MON_MAX_ZONES];

	uint8_t cal__xmon__zone__y_off[CAL_DEF__XTALK_MON_MAX_ZONES];

	uint8_t cal__xmon__zone__width[CAL_DEF__XTALK_MON_MAX_ZONES];

	uint8_t cal__xmon__zone__height[CAL_DEF__XTALK_MON_MAX_ZONES];
};

struct cal_grp__xtalk_mon__data_t {

	uint32_t cal__xmon__zone__rate_kcps_spad[CAL_DEF__XTALK_MON_MAX_ZONES];

	uint16_t cal__xmon__zone__avg_count[CAL_DEF__XTALK_MON_MAX_ZONES];
};

struct cal_grp__phase_stats_t {

	uint32_t cal__stats__avg_phase__ref;

	uint32_t cal__stats__avg_phase__rtn;

	uint32_t cal__stats__avg_phase__flex;

	uint16_t cal__stats__avg_count__ref;

	uint16_t cal__stats__avg_count__rtn;

	uint16_t cal__stats__avg_count__flex;
	uint16_t cal__stats__spare_0;
};

struct cal_grp__temperature_stats_t {

	int8_t cal__stats__avg_temp_degc__start__ref;

	int8_t cal__stats__avg_temp_degc__end__ref;

	int8_t cal__stats__avg_temp_degc__start__rtn;

	int8_t cal__stats__avg_temp_degc__end__rtn;
};

#ifdef __cplusplus
}
#endif

#endif
