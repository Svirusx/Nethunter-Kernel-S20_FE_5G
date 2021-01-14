
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

#ifndef __DCI_STRUCTS_H__
#define __DCI_STRUCTS_H__

#include "vl53l5_results_config.h"
#include "packing_structs.h"
#include "dci_defs.h"
#include "dci_luts.h"
#include "dci_union_structs.h"
#include "packing_structs.h"
#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct dci_grp__map_version_t {

	uint16_t map__major;

	uint16_t map__minor;
};

struct dci_grp__fw_version_t {

	uint16_t fw__revision;

	uint16_t fw__build;

	uint16_t fw__minor;

	uint16_t fw__major;
};

struct dci_grp__cfg_info_t {

	union dci_union__config_presets_u cfg__presets;

	union dci_union__config_parms_u cfg__parms;

	uint32_t cfg__timing_ranging_rate_hz;

	uint32_t cfg__results__size_bytes;

	uint16_t cfg__version_major;

	uint16_t cfg__version_minor;
};

struct dci_grp__silicon_temperature_data_t {

	int8_t silicon_temp_degc__start;

	int8_t silicon_temp_degc__end;
	int8_t silicon_temp__pad_0;
	int8_t silicon_temp__pad_1;
};

struct dci_grp__fmt_traceability_t {

	union dci_union__st_fgc_0_u st_fgc_0_u;

	union dci_union__st_fgc_1_u st_fgc_1_u;

	union dci_union__st_fgc_2_u st_fgc_2_u;

	union dci_union__st_fgc_3_u st_fgc_3_u;

	union dci_union__identification_0_u identification_0_u;

	union dci_union__identification_1_u identification_1_u;
};

struct dci_grp__ews_traceability_t {

	union dci_union__test_traceability_u test_traceability_u;

	union dci_union__die_traceability_0_u die_traceability_0_u;

	union dci_union__die_traceability_1_u die_traceability_1_u;
};

struct dci_grp__zone_cfg_t {

	uint8_t zone__grid_cols;

	uint8_t zone__grid_rows;

	uint8_t zone__grid_x_ll;

	uint8_t zone__grid_y_ll;

	uint8_t zone__grid_x_pitch;

	uint8_t zone__grid_y_pitch;
	uint8_t zone__pad_0;
	uint8_t zone__pad_1;
};

struct dci_grp__ranging_rate_cfg_t {

	uint32_t ranging_rate_hz;
};

struct dci_grp__integration_time_cfg_t {

	uint32_t integration_time_us;
};

struct dci_grp__factory_cal_cfg_t {

	int16_t factory_cal__target_distance_mm;

	uint16_t factory_cal__target_reflectance_pc;

	uint8_t factory_cal__no_of_samples;
	uint8_t factory_cal__pad_0;
	uint8_t factory_cal__pad_1;
	uint8_t factory_cal__pad_2;
};

struct dci_grp__output_data_cfg_t {

	uint8_t output__max_targets_per_zone;

	uint8_t output__dummy_bytes;
	uint8_t output__spare_0;
	uint8_t output__spare_1;
};

struct dci_grp__interrupt_cfg_t {

	union dci_union__interrupt__config_u interrupt__config;

	uint8_t interrupt__cfg_mode;

	uint8_t interrupt__cfg_num_active_checkers;

	uint8_t interrupt__cfg_spare_0;

	uint8_t interrupt__cfg_spare_1;

	uint8_t interrupt__cfg_spare_2;

	uint8_t interrupt__cfg_spare_3;

	uint8_t interrupt__cfg_spare_4;
};

struct dci_grp__cal__ref_spad_info_t {

	uint8_t cal__ref_spad__offset;

	uint8_t cal__ref_spad__count;

	uint8_t cal__ref_spad__count_10x;

	uint8_t cal__ref_spad__count_100x;

	uint8_t cal__ref_spad__left_right_sel;

	uint8_t cal__ref_spad__status;
	uint8_t cal__ref_spad_info__pad_0;
	uint8_t cal__ref_spad_info__pad_1;
};

struct dci_grp__cal__temp_sensor_data_t {

	uint8_t cal__temp_sensor__degc;

	uint8_t cal__temp_sensor__value;
	uint8_t cal__temp_sensor__pad_0;
	uint8_t cal__temp_sensor__pad_1;
};

struct dci_grp__cal__optical_centre_data_t {

	uint8_t cal__optical_centre__x;

	uint8_t cal__optical_centre__y;
	uint8_t cal__optical_centre__pad_0;
	uint8_t cal__optical_centre__pad_1;
};

struct dci_grp__ui_rng_data_addr_t {

	uint16_t ui_rng_data_int_start_addr;

	uint16_t ui_rng_data_int_end_addr;

	uint16_t ui_rng_data_dummy_bytes;

	uint16_t ui_rng_data_offset_bytes;

	uint16_t ui_rng_data_size_bytes;

	uint16_t ui_device_info_start_addr;
};

struct vl53l5_range_meta_data_t {

	uint32_t time_stamp;

	uint8_t device_status;

	uint8_t transaction_id;

	uint8_t buffer_id;

	uint8_t stream_count;

	int8_t silicon_temp_degc;
	uint8_t buf_meta_data__pad_0;
	uint8_t buf_meta_data__pad_1;
	uint8_t buf_meta_data__pad_2;
};

struct dci_grp__rng_timing_data_t {

	uint32_t rng__integration_time_us;

	uint32_t rng__total_periods_elapsed;

	uint32_t rng__blanking_time_us;
};

struct vl53l5_range_common_data_t {

	uint16_t wrap_dmax_mm;

	uint8_t rng__no_of_zones;

	uint8_t rng__max_targets_per_zone;
};

struct vl53l5_range_per_zone_results_t {
#ifdef VL53L5_AMB_RATE_KCPS_PER_SPAD_ON

	uint32_t amb_rate_kcps_per_spad[VL53L5_MAX_ZONES];
#endif

#ifdef VL53L5_EFFECTIVE_SPAD_COUNT_ON

	uint32_t rng__effective_spad_count[VL53L5_MAX_ZONES];
#endif

#ifdef VL53L5_AMB_DMAX_MM_ON

	uint16_t amb_dmax_mm[VL53L5_MAX_ZONES];
#endif

#ifdef VL53L5_SILICON_TEMP_DEGC_START_ON

	int8_t silicon_temp_degc__start[VL53L5_MAX_ZONES];
#endif

#ifdef VL53L5_SILICON_TEMP_DEGC_END_ON

	int8_t silicon_temp_degc__end[VL53L5_MAX_ZONES];
#endif

#ifdef VL53L5_NO_OF_TARGETS_ON

	uint8_t rng__no_of_targets[VL53L5_MAX_ZONES];
#endif

#ifdef VL53L5_ZONE_ID_ON

	uint8_t rng__zone_id[VL53L5_MAX_ZONES];
#endif

#ifdef VL53L5_SEQUENCE_IDX_ON

	uint8_t rng__sequence_idx[VL53L5_MAX_ZONES];
#endif

};

struct vl53l5_range_per_tgt_results_t {
#ifdef VL53L5_PEAK_RATE_KCPS_PER_SPAD_ON

	uint32_t peak_rate_kcps_per_spad[VL53L5_MAX_TARGETS];
#endif

#ifdef VL53L5_MEDIAN_PHASE_ON

	uint32_t median_phase[VL53L5_MAX_TARGETS];
#endif

#ifdef VL53L5_RATE_SIGMA_KCPS_PER_SPAD_ON

	uint32_t rate_sigma_kcps_per_spad[VL53L5_MAX_TARGETS];
#endif

#ifdef VL53L5_RANGE_SIGMA_MM_ON

	uint16_t range_sigma_mm[VL53L5_MAX_TARGETS];
#endif

#ifdef VL53L5_MEDIAN_RANGE_MM_ON

	int16_t median_range_mm[VL53L5_MAX_TARGETS];
#endif

#ifdef VL53L5_MIN_RANGE_DELTA_MM_ON

	uint8_t min_range_delta_mm[VL53L5_MAX_TARGETS];
#endif

#ifdef VL53L5_MAX_RANGE_DELTA_MM_ON

	uint8_t max_range_delta_mm[VL53L5_MAX_TARGETS];
#endif

#ifdef VL53L5_TARGET_REFLECTANCE_EST_PC_ON

	uint8_t target_reflectance_est_pc[VL53L5_MAX_TARGETS];
#endif

#ifdef VL53L5_TARGET_STATUS_ON

	uint8_t target_status[VL53L5_MAX_TARGETS];
#endif

};

struct dci_grp__ref_channel_data_t {

	uint32_t amb_rate_kcps_per_spad;

	uint32_t rng__effective_spad_count;

	uint16_t amb_dmax_mm;

	int8_t silicon_temp_degc__start;

	int8_t silicon_temp_degc__end;

	uint8_t rng__no_of_targets;

	uint8_t rng__zone_id;

	uint8_t rng__sequence_idx;
	uint8_t ref_channel_data__pad_0;
};

struct dci_grp__ref_target_data_t {

	uint32_t peak_rate_kcps_per_spad;

	uint32_t median_phase;

	uint32_t rate_sigma_kcps_per_spad;

	uint16_t range_sigma_mm;

	int16_t median_range_mm;

	uint8_t min_range_delta_mm;

	uint8_t max_range_delta_mm;

	uint8_t target_reflectance_est_pc;

	uint8_t target_status;
};

struct dci_grp__gbl_tgt_meta_data_t {

	uint16_t gbl_tgt__wrap_dmax_mm;

	uint16_t gbl_tgt__amb_dmax_mm;

	uint8_t gbl_tgt__no_of_targets;

	uint8_t gbl_tgt__max_targets;
	uint8_t gbl_tgt__pad_0;
	uint8_t gbl_tgt__pad_1;
};

struct dci_grp__gbl_tgt_data_t {
	uint32_t gbl_tgt__peak_rate_kcps_per_spad[DCI_MAX_GLOBAL_TARGETS];
	uint32_t gbl_tgt__amb_rate_kcps_per_spad[DCI_MAX_GLOBAL_TARGETS];
	uint32_t gbl_tgt__peak_rate_mcps[DCI_MAX_GLOBAL_TARGETS];
	uint32_t gbl_tgt__amb_rate_mcps[DCI_MAX_GLOBAL_TARGETS];
	uint16_t gbl_tgt__median_range_mm[DCI_MAX_GLOBAL_TARGETS];
	uint8_t gbl_tgt__min_range_delta_mm[DCI_MAX_GLOBAL_TARGETS];
	uint8_t gbl_tgt__max_range_delta_mm[DCI_MAX_GLOBAL_TARGETS];

	uint8_t gbl_tgt__reflect_est_pc[DCI_MAX_GLOBAL_TARGETS];

	uint8_t gbl_tgt__status[DCI_MAX_GLOBAL_TARGETS];

	uint8_t gbl_tgt__x_centre[DCI_MAX_GLOBAL_TARGETS];

	uint8_t gbl_tgt__y_centre[DCI_MAX_GLOBAL_TARGETS];

	uint8_t gbl_tgt__width[DCI_MAX_GLOBAL_TARGETS];

	uint8_t gbl_tgt__height[DCI_MAX_GLOBAL_TARGETS];
	uint8_t gbl_tgt__no_of_rois[DCI_MAX_GLOBAL_TARGETS];
};

struct dci_grp__dyn_xtalk_persistent_data_t {

	uint32_t dyn_xt__dyn_xtalk_grid_maximum_kcps_per_spad;

	uint32_t dyn_xt__dyn_xtalk_grid_max_sigma_kcps_per_spad;

	uint32_t dyn_xt__new_max_xtalk_sigma_kcps_per_spad;

	int32_t dyn_xt__calibration_gain;

	int32_t temp_comp__temp_gain;

	uint16_t dyn_xt__nb_samples;

	uint16_t dyn_xt__desired_samples;

	uint8_t dyn_xt__accumulating;

	uint8_t dyn_xt__grid_ready;

	uint8_t dyn_xt__persist_spare_1;

	uint8_t dyn_xt__persist_spare_2;
};

struct dci_grp__zone_thresh_status_array_t {
#ifdef VL53L5_ZONE_THRESH_STATUS_BYTES_ON

	uint8_t zone_thresh_status_bytes[DCI__ZONE_THRESH_STATUS_ARRAY_SIZE];
#endif

};

struct dci_grp__nvm_laser_safety_nvm2_t {

	uint32_t nvm_laser_safety__regdvdd1v1_sel_lp;
};

struct dci_grp__output_bh_cfg_t {

	uint32_t op_bh__op_packet_size_bytes;

	uint32_t op_bh__debug_data_start;
};

struct dci_grp__output_bh_enables_t {

	uint32_t op_bh__enables[DCI_DEF__MAX_OP_BH_ENABLE_SIZE];
};

struct dci_grp__output_bh_list_t {

	uint32_t op_bh__list[DCI_DEF__MAX_OP_BH_LIST_SIZE];
};

struct dci_grp__patch_hook_enables_t {

	uint32_t patch_hook_enables[DCI_DEF__PATCH_HOOK_ENABLE_WORD32];
};

struct dci_grp__gbl_tgt_data_packed_t {
	uint32_t gbl_tgt__peak_rate_kcps_per_spad[DCI_MAX_GLOBAL_TARGETS];
	uint32_t gbl_tgt__amb_rate_kcps_per_spad[DCI_MAX_GLOBAL_TARGETS];
	struct bit_packed_24 gbl_tgt__peak_rate_mcps[
		DCI_MAX_GLOBAL_TARGETS / 4];
	struct bit_packed_24 gbl_tgt__amb_rate_mcps[DCI_MAX_GLOBAL_TARGETS / 4];
	uint16_t gbl_tgt__median_range_mm[DCI_MAX_GLOBAL_TARGETS];
	uint8_t gbl_tgt__min_range_delta_mm[DCI_MAX_GLOBAL_TARGETS];
	uint8_t gbl_tgt__max_range_delta_mm[DCI_MAX_GLOBAL_TARGETS];

	uint8_t gbl_tgt__reflect_est_pc[DCI_MAX_GLOBAL_TARGETS];

	uint8_t gbl_tgt__status[DCI_MAX_GLOBAL_TARGETS];

	uint8_t gbl_tgt__x_centre[DCI_MAX_GLOBAL_TARGETS];

	uint8_t gbl_tgt__y_centre[DCI_MAX_GLOBAL_TARGETS];

	uint8_t gbl_tgt__width[DCI_MAX_GLOBAL_TARGETS];

	uint8_t gbl_tgt__height[DCI_MAX_GLOBAL_TARGETS];
	uint8_t gbl_tgt__no_of_rois[DCI_MAX_GLOBAL_TARGETS];
};

#ifdef __cplusplus
}
#endif

#endif
