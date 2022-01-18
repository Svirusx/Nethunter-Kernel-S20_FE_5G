
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

#ifndef __VL53L5_RESULTS_DEV_PATH_H__
#define __VL53L5_RESULTS_DEV_PATH_H__

#ifdef __cplusplus
extern "C" {
#endif

#define VL53L5_MAP_RESULTS_DEV(p_dev) \
	((p_dev)->host_dev.presults_dev)
#define VL53L5_META_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->meta_data
#define VL53L5_COMMON_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->common_data
#define VL53L5_RNG_TIMING_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->rng_timing_data
#define VL53L5_PER_ZONE_RESULTS(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->per_zone_results
#define VL53L5_PER_TGT_RESULTS(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->per_tgt_results
#define VL53L5_REF_TIMING_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->ref_timing_data
#define VL53L5_REF_CHANNEL_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->ref_channel_data
#define VL53L5_REF_TARGET_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->ref_target_data
#define VL53L5_HIST_ZONE_GRID_POINT_META_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->hist_zone_grid_point_meta_data
#define VL53L5_HIST_ZONE_GRID_POINT_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->hist_zone_grid_point_data
#define VL53L5_HIST_COMMON_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->hist_common_data
#define VL53L5_HIST_TIMING_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->hist_timing_data
#define VL53L5_HIST_CHANNEL_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->hist_channel_data
#define VL53L5_HIST_BIN_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->hist_bin_data
#define VL53L5_HIST_REF_COMMON_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->hist_ref_common_data
#define VL53L5_HIST_REF_TIMING_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->hist_ref_timing_data
#define VL53L5_HIST_REF_CHANNEL_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->hist_ref_channel_data
#define VL53L5_HIST_REF_BIN_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->hist_ref_bin_data
#define VL53L5_REF_MPX_META_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->ref_mpx_meta_data
#define VL53L5_REF_MPX_COMMON_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->ref_mpx_common_data
#define VL53L5_REF_MPX_TIMING_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->ref_mpx_timing_data
#define VL53L5_REF_MPX_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->ref_mpx_data
#define VL53L5_RTN_MPX_META_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->rtn_mpx_meta_data
#define VL53L5_RTN_MPX_COMMON_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->rtn_mpx_common_data
#define VL53L5_RTN_MPX_TIMING_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->rtn_mpx_timing_data
#define VL53L5_RTN_MPX_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->rtn_mpx_data
#define VL53L5_XTALK_MON_META(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->xtalk_mon_meta
#define VL53L5_XTALK_MON_ZONES_MAX(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->xtalk_mon_zones_max
#define VL53L5_XTALK_MON_ZONES_ACTUAL(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->xtalk_mon_zones_actual
#define VL53L5_XTALK_MON_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->xtalk_mon_data
#define VL53L5_VHV_RESULT_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->vhv_result_data
#define VL53L5_VHV_SEARCH_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->vhv_search_data
#define VL53L5_CAL_REF_SPAD_SEARCH_META_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->cal_ref_spad_search_meta_data
#define VL53L5_CAL_REF_SPAD_SEARCH_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->cal_ref_spad_search_data
#define VL53L5_REF_ARRAY_META_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->ref_array_meta_data
#define VL53L5_RTN_ARRAY_META_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->rtn_array_meta_data
#define VL53L5_REF_ARRAY_SPAD_EN(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->ref_array_spad_en
#define VL53L5_RTN_ARRAY_SPAD_EN(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->rtn_array_spad_en
#define VL53L5_ZONE_THRESH_STATUS(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->zone_thresh_status
#define VL53L5_DYN_XTALK_OP_PERSISTENT_DATA(p_dev) \
	VL53L5_MAP_RESULTS_DEV(p_dev)->dyn_xtalk_op_persistent_data

#ifdef __cplusplus
}
#endif

#endif
