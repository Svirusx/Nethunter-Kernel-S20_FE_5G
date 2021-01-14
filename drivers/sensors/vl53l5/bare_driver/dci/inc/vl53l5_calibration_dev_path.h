
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

#ifndef __VL53L5_CALIBRATION_DEV_PATH_H__
#define __VL53L5_CALIBRATION_DEV_PATH_H__

#ifdef __cplusplus
extern "C" {
#endif

#define VL53L5_MAP_CALIBRATION_DEV(p_dev) \
	((p_dev)->host_dev.pcalibration_dev)
#define VL53L5_CAL_REF_SPAD_INFO(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->cal_ref_spad_info
#define VL53L5_CAL_TEMP_SENSOR(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->cal_temp_sensor
#define VL53L5_CAL_OPTICAL_CENTRE(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->cal_optical_centre
#define VL53L5_CAL_OSCILLATORS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->cal_oscillators
#define VL53L5_CAL_VHV(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->cal_vhv
#define VL53L5_POFFSET_GRID_META(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->poffset_grid_meta
#define VL53L5_POFFSET_PHASE_STATS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->poffset_phase_stats
#define VL53L5_POFFSET_TEMPERATURE_STATS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->poffset_temperature_stats
#define VL53L5_POFFSET_GRID_RATE(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->poffset_grid_rate
#define VL53L5_POFFSET_GRID_SPADS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->poffset_grid_spads
#define VL53L5_POFFSET_GRID_OFFSET(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->poffset_grid_offset
#define VL53L5_POFFSET_ERROR_STATUS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->poffset_error_status
#define VL53L5_POFFSET_WARNING_STATUS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->poffset_warning_status
#define VL53L5_POFFSET_4X4_GRID_META(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->poffset_4x4_grid_meta
#define VL53L5_POFFSET_4X4_PHASE_STATS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->poffset_4x4_phase_stats
#define VL53L5_POFFSET_4X4_TEMPERATURE_STATS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->poffset_4x4_temperature_stats
#define VL53L5_POFFSET_4X4_GRID_RATE(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->poffset_4x4_grid_rate
#define VL53L5_POFFSET_4X4_GRID_SPADS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->poffset_4x4_grid_spads
#define VL53L5_POFFSET_4X4_GRID_OFFSET(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->poffset_4x4_grid_offset
#define VL53L5_POFFSET_4X4_ERROR_STATUS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->poffset_4x4_error_status
#define VL53L5_POFFSET_4X4_WARNING_STATUS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->poffset_4x4_warning_status
#define VL53L5_PXTALK_GRID_META(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pxtalk_grid_meta
#define VL53L5_PXTALK_PHASE_STATS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pxtalk_phase_stats
#define VL53L5_PXTALK_TEMPERATURE_STATS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pxtalk_temperature_stats
#define VL53L5_PXTALK_GRID_RATE(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pxtalk_grid_rate
#define VL53L5_PXTALK_GRID_SPADS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pxtalk_grid_spads
#define VL53L5_PXTALK_ERROR_STATUS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pxtalk_error_status
#define VL53L5_PXTALK_WARNING_STATUS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pxtalk_warning_status
#define VL53L5_PCURRENT_XTALK_4X4_GRID_META(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcurrent_xtalk_4x4_grid_meta
#define VL53L5_PCURRENT_XTALK_4X4_PHASE_STATS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcurrent_xtalk_4x4_phase_stats
#define VL53L5_PCURRENT_XTALK_4X4_TEMPERATURE_STATS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcurrent_xtalk_4x4_temperature_stats
#define VL53L5_PCURRENT_XTALK_4X4_GRID_RATE(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcurrent_xtalk_4x4_grid_rate
#define VL53L5_PCURRENT_XTALK_4X4_GRID_SPADS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcurrent_xtalk_4x4_grid_spads
#define VL53L5_PCURRENT_XTALK_4X4_ERROR_STATUS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcurrent_xtalk_4x4_error_status
#define VL53L5_PCURRENT_XTALK_4X4_WARNING_STATUS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcurrent_xtalk_4x4_warning_status
#define VL53L5_PCURRENT_XTALK_8X8_GRID_META(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcurrent_xtalk_8x8_grid_meta
#define VL53L5_PCURRENT_XTALK_8X8_PHASE_STATS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcurrent_xtalk_8x8_phase_stats
#define VL53L5_PCURRENT_XTALK_8X8_TEMPERATURE_STATS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcurrent_xtalk_8x8_temperature_stats
#define VL53L5_PCURRENT_XTALK_8X8_GRID_RATE(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcurrent_xtalk_8x8_grid_rate
#define VL53L5_PCURRENT_XTALK_8X8_GRID_SPADS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcurrent_xtalk_8x8_grid_spads
#define VL53L5_PCURRENT_XTALK_8X8_ERROR_STATUS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcurrent_xtalk_8x8_error_status
#define VL53L5_PCURRENT_XTALK_8X8_WARNING_STATUS(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcurrent_xtalk_8x8_warning_status
#define VL53L5_PXTALK_SHAPE_META(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pxtalk_shape_meta
#define VL53L5_PXTALK_SHAPE_DATA(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pxtalk_shape_data
#define VL53L5_PXTALK_MON_META(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pxtalk_mon_meta
#define VL53L5_PXTALK_MON_ZONES(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pxtalk_mon_zones
#define VL53L5_PXTALK_MON_DATA(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pxtalk_mon_data
#define VL53L5_PRAD2PERP_GRID_META(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->prad2perp_grid_meta
#define VL53L5_PRAD2PERP_GRID_DATA(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->prad2perp_grid_data
#define VL53L5_PRAD2PERP_GRID_4X4_META(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->prad2perp_grid_4x4_meta
#define VL53L5_PRAD2PERP_GRID_4X4_DATA(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->prad2perp_grid_4x4_data
#define VL53L5_PRAD2PERP_GRID_8X8_META(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->prad2perp_grid_8x8_meta
#define VL53L5_PRAD2PERP_GRID_8X8_DATA(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->prad2perp_grid_8x8_data
#define VL53L5_PNORM_GRID_META(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pnorm_grid_meta
#define VL53L5_PNORM_GRID_DATA(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pnorm_grid_data
#define VL53L5_PDYN_XTALK__SUB_FRAME_XTALK_GRID_RATE(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pdyn_xtalk__sub_frame_xtalk_grid_rate
#define VL53L5_PDYN_XTALK__OTF_XTALK_GRID_RATE(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pdyn_xtalk__otf_xtalk_grid_rate
#define VL53L5_PCAL_COMMON_SUM(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcal_common_sum
#define VL53L5_PCAL_XMON_SUM(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcal_xmon_sum
#define VL53L5_PCAL_ZONE_SUM(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcal_zone_sum
#define VL53L5_PCAL_HIST_META(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcal_hist_meta
#define VL53L5_PCAL_HIST_SUM(p_dev) \
	VL53L5_MAP_CALIBRATION_DEV(p_dev)->pcal_hist_sum

#ifdef __cplusplus
}
#endif

#endif
