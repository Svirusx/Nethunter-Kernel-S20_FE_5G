
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

#ifndef __DCI_SIZE_H__
#define __DCI_SIZE_H__

#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VL53L5_FW_WORKSPACE_SZ \
	((uint16_t) 10240)

#define VL53L5_FW_FW_WORKSPACE_ARRAY_SZ \
	((uint16_t) 10240)

#define VL53L5_UI_BUFFER_SZ \
	((uint16_t) 11264)

#define VL53L5_UB_DCI_OUTPUT_BUFFER_SZ \
	((uint16_t) 11264)

#define VL53L5_MAP_VERSION_SZ \
	((uint16_t) 4)

#define VL53L5_FW_VERSION_SZ \
	((uint16_t) 8)

#define VL53L5_CFG_INFO_SZ \
	((uint16_t) 20)

#define VL53L5_CP_COLLAPSE_CTRL_SZ \
	((uint16_t) 4)

#define VL53L5_SILICON_TEMPERATURE_DATA_SZ \
	((uint16_t) 4)

#define VL53L5_FMT_TRACEABILITY_SZ \
	((uint16_t) 24)

#define VL53L5_EWS_TRACEABILITY_SZ \
	((uint16_t) 12)

#define VL53L5_ZONE_CFG_SZ \
	((uint16_t) 8)

#define VL53L5_DEVICE_MODE_CFG_SZ \
	((uint16_t) 8)

#define VL53L5_DSS_CFG_SZ \
	((uint16_t) 16)

#define VL53L5_RANGING_RATE_CFG_SZ \
	((uint16_t) 4)

#define VL53L5_INTEGRATION_TIME_CFG_SZ \
	((uint16_t) 4)

#define VL53L5_FACTORY_CAL_CFG_SZ \
	((uint16_t) 8)

#define VL53L5_OUTPUT_DATA_CFG_SZ \
	((uint16_t) 4)

#define VL53L5_INTERRUPT_CFG_SZ \
	((uint16_t) 8)

#define VL53L5_TIMING_CFG_SZ \
	((uint16_t) 16)

#define VL53L5_STATIC_TIMING_CFG_SZ \
	((uint16_t) 8)

#define VL53L5_VHV_CFG_SZ \
	((uint16_t) 12)

#define VL53L5_REF_MSR_CFG_SZ \
	((uint16_t) 8)

#define VL53L5_VHV_REF_SCHEDULING_CTRL_SZ \
	((uint16_t) 8)

#define VL53L5_CAL_REF_SPAD_CFG_SZ \
	((uint16_t) 8)

#define VL53L5_SSC_CFG_SZ \
	((uint16_t) 4)

#define VL53L5_CAL_REF_SPAD_INFO_SZ \
	((uint16_t) 8)

#define VL53L5_CAL_TEMP_SENSOR_DATA_SZ \
	((uint16_t) 4)

#define VL53L5_CAL_OPTICAL_CENTRE_DATA_SZ \
	((uint16_t) 4)

#define VL53L5_REF_ARRAY_META_DATA_SZ \
	((uint16_t) 8)

#define VL53L5_RTN_ARRAY_META_DATA_SZ \
	((uint16_t) 8)

#define VL53L5_REF_ARRAY_SPAD_ENABLES_SZ \
	((uint16_t) 16)

#define VL53L5_RASE_REF_ARRAY_SPADS_ENABLES_SZ \
	((uint16_t) 16)

#define VL53L5_RTN_ARRAY_SPAD_ENABLES_SZ \
	((uint16_t) 280)

#define VL53L5_RASE_RTN_ARRAY_SPAD_ENABLES_SZ \
	((uint16_t) 280)

#define VL53L5_UI_RNG_DATA_ADDR_SZ \
	((uint16_t) 12)

#define VL53L5_BUF_META_DATA_SZ \
	((uint16_t) 12)

#define VL53L5_HIST_ZONE_GRID_POINT_META_DATA_SZ \
	((uint16_t) 8)

#define VL53L5_HIST_ZONE_GRID_POINT_DATA_SZ \
	((uint16_t) 128)

#define VL53L5_HZGPD_HIST_ZONE_CENTRE_X_COORD_SZ \
	((uint16_t) 64)

#define VL53L5_HZGPD_HIST_ZONE_CENTRE_Y_COORD_SZ \
	((uint16_t) 64)

#define VL53L5_HIST_COMMON_DATA_SZ \
	((uint16_t) 20)

#define VL53L5_HIST_TIMING_DATA_SZ \
	((uint16_t) 4)

#define VL53L5_HIST_CHANNEL_DATA_SZ \
	((uint16_t) 224)

#define VL53L5_HIST_CHANNEL_DATA_PACKED_SZ \
	((uint16_t) 176)

#define VL53L5_HCD_HIST_EFFECTIVE_SPAD_COUNT_SZ \
	((uint16_t) 64)

#define VL53L5_HCD_HIST_EFFECTIVE_SPAD_COUNT_PACKED_SZ \
	((uint16_t) 48)

#define VL53L5_HCD_HIST_AMB_WINDOW_EVENTS_SZ \
	((uint16_t) 64)

#define VL53L5_HCD_HIST_AMB_WINDOW_EVENTS_PACKED_SZ \
	((uint16_t) 48)

#define VL53L5_HCD_HIST_AMB_EVENTS_PER_BIN_SZ \
	((uint16_t) 64)

#define VL53L5_HCD_HIST_AMB_EVENTS_PER_BIN_PACKED_SZ \
	((uint16_t) 48)

#define VL53L5_HCD_HIST_ZONE_ID_SZ \
	((uint16_t) 16)

#define VL53L5_HCD_HIST_TYPE_SZ \
	((uint16_t) 16)

#define VL53L5_HIST_REF_CHANNEL_DATA_SZ \
	((uint16_t) 16)

#define VL53L5_HIST_REF_CHANNEL_DATA_PACKED_SZ \
	((uint16_t) 13)

#define VL53L5_MPX_META_DATA_SZ \
	((uint16_t) 4)

#define VL53L5_MPX_COMMON_DATA_SZ \
	((uint16_t) 12)

#define VL53L5_MPX_TIMING_DATA_SZ \
	((uint16_t) 4)

#define VL53L5_REF_MPX_DATA_SZ \
	((uint16_t) 192)

#define VL53L5_RMD_REF_MPX_PEAK_RATE_KCPS_PER_SPAD_SZ \
	((uint16_t) 32)

#define VL53L5_RMD_REF_MPX_AMB_RATE_KCPS_PER_SPAD_SZ \
	((uint16_t) 32)

#define VL53L5_RMD_REF_MPX_RANGING_TOTAL_EVENTS_SZ \
	((uint16_t) 32)

#define VL53L5_RMD_REF_MPX_AMBIENT_WINDOW_EVENTS_SZ \
	((uint16_t) 32)

#define VL53L5_RMD_REF_MPX_EFFECTIVE_SPAD_COUNT_SZ \
	((uint16_t) 32)

#define VL53L5_RMD_REF_MPX_SPATIAL_METRIC_SZ \
	((uint16_t) 32)

#define VL53L5_RTN_MPX_DATA_SZ \
	((uint16_t) 3360)

#define VL53L5_RMD_RTN_MPX_PEAK_RATE_KCPS_PER_SPAD_SZ \
	((uint16_t) 560)

#define VL53L5_RMD_RTN_MPX_AMB_RATE_KCPS_PER_SPAD_SZ \
	((uint16_t) 560)

#define VL53L5_RMD_RTN_MPX_RANGING_TOTAL_EVENTS_SZ \
	((uint16_t) 560)

#define VL53L5_RMD_RTN_MPX_AMBIENT_WINDOW_EVENTS_SZ \
	((uint16_t) 560)

#define VL53L5_RMD_RTN_MPX_EFFECTIVE_SPAD_COUNT_SZ \
	((uint16_t) 560)

#define VL53L5_RMD_RTN_MPX_SPATIAL_METRIC_SZ \
	((uint16_t) 560)

#define VL53L5_RNG_TIMING_DATA_SZ \
	((uint16_t) 12)

#define VL53L5_RNG_COMMON_DATA_SZ \
	((uint16_t) 4)

#define VL53L5_RNG_PER_ZONE_DATA_SZ \
	((uint16_t) 960)

#define VL53L5_RPZD_AMB_RATE_KCPS_PER_SPAD_SZ \
	((uint16_t) 4 * VL53L5_MAX_ZONES)

#define VL53L5_RPZD_RNG_EFFECTIVE_SPAD_COUNT_SZ \
	((uint16_t) 4 * VL53L5_MAX_ZONES)

#define VL53L5_RPZD_AMB_DMAX_MM_SZ \
	((uint16_t) 2 * VL53L5_MAX_ZONES)

#define VL53L5_RPZD_SILICON_TEMP_DEGC_START_SZ \
	((uint16_t) 1 * VL53L5_MAX_ZONES)

#define VL53L5_RPZD_SILICON_TEMP_DEGC_END_SZ \
	((uint16_t) 1 * VL53L5_MAX_ZONES)

#define VL53L5_RPZD_RNG_NO_OF_TARGETS_SZ \
	((uint16_t) 1 * VL53L5_MAX_ZONES)

#define VL53L5_RPZD_RNG_ZONE_ID_SZ \
	((uint16_t) 1 * VL53L5_MAX_ZONES)

#define VL53L5_RPZD_RNG_SEQUENCE_IDX_SZ \
	((uint16_t) 1 * VL53L5_MAX_ZONES)

#define VL53L5_RNG_PER_TARGET_DATA_SZ \
	((uint16_t) 5120)

#define VL53L5_RPTD_PEAK_RATE_KCPS_PER_SPAD_SZ \
	((uint16_t) 4 * VL53L5_MAX_TARGETS)

#define VL53L5_RPTD_MEDIAN_PHASE_SZ \
	((uint16_t) 4 * VL53L5_MAX_TARGETS)

#define VL53L5_RPTD_RATE_SIGMA_KCPS_PER_SPAD_SZ \
	((uint16_t) 4 * VL53L5_MAX_TARGETS)

#define VL53L5_RPTD_RANGE_SIGMA_MM_SZ \
	((uint16_t) 2 * VL53L5_MAX_TARGETS)

#define VL53L5_RPTD_MEDIAN_RANGE_MM_SZ \
	((uint16_t) 2 * VL53L5_MAX_TARGETS)

#define VL53L5_RPTD_MIN_RANGE_DELTA_MM_SZ \
	((uint16_t) 1 * VL53L5_MAX_TARGETS)

#define VL53L5_RPTD_MAX_RANGE_DELTA_MM_SZ \
	((uint16_t) 1 * VL53L5_MAX_TARGETS)

#define VL53L5_RPTD_TARGET_REFLECTANCE_EST_PC_SZ \
	((uint16_t) 1 * VL53L5_MAX_TARGETS)

#define VL53L5_RPTD_TARGET_STATUS_SZ \
	((uint16_t) 1 * VL53L5_MAX_TARGETS)

#define VL53L5_REF_CHANNEL_DATA_SZ \
	((uint16_t) 16)

#define VL53L5_REF_TARGET_DATA_SZ \
	((uint16_t) 20)

#define VL53L5_GBL_TGT_META_DATA_SZ \
	((uint16_t) 8)

#define VL53L5_GBL_TGT_DATA_SZ \
	((uint16_t) 216)

#define VL53L5_GBL_TGT_DATA_PACKED_SZ \
	((uint16_t) 200)

#define VL53L5_GTD_GBL_TGT_PEAK_RATE_KCPS_PER_SPAD_SZ \
	((uint16_t) 32)

#define VL53L5_GTD_GBL_TGT_AMB_RATE_KCPS_PER_SPAD_SZ \
	((uint16_t) 32)

#define VL53L5_GTD_GBL_TGT_PEAK_RATE_MCPS_SZ \
	((uint16_t) 32)

#define VL53L5_GTD_GBL_TGT_PEAK_RATE_MCPS_PACKED_SZ \
	((uint16_t) 24)

#define VL53L5_GTD_GBL_TGT_AMB_RATE_MCPS_SZ \
	((uint16_t) 32)

#define VL53L5_GTD_GBL_TGT_AMB_RATE_MCPS_PACKED_SZ \
	((uint16_t) 24)

#define VL53L5_GTD_GBL_TGT_MEDIAN_RANGE_MM_SZ \
	((uint16_t) 16)

#define VL53L5_GTD_GBL_TGT_MIN_RANGE_DELTA_MM_SZ \
	((uint16_t) 8)

#define VL53L5_GTD_GBL_TGT_MAX_RANGE_DELTA_MM_SZ \
	((uint16_t) 8)

#define VL53L5_GTD_GBL_TGT_REFLECT_EST_PC_SZ \
	((uint16_t) 8)

#define VL53L5_GTD_GBL_TGT_STATUS_SZ \
	((uint16_t) 8)

#define VL53L5_GTD_GBL_TGT_X_CENTRE_SZ \
	((uint16_t) 8)

#define VL53L5_GTD_GBL_TGT_Y_CENTRE_SZ \
	((uint16_t) 8)

#define VL53L5_GTD_GBL_TGT_WIDTH_SZ \
	((uint16_t) 8)

#define VL53L5_GTD_GBL_TGT_HEIGHT_SZ \
	((uint16_t) 8)

#define VL53L5_GTD_GBL_TGT_NO_OF_ROIS_SZ \
	((uint16_t) 8)

#define VL53L5_VHV_RESULT_DATA_SZ \
	((uint16_t) 8)

#define VL53L5_VHV_SEARCH_DATA_SZ \
	((uint16_t) 384)

#define VL53L5_VSD_VHV_SEARCH_EVENTS_SZ \
	((uint16_t) 256)

#define VL53L5_VSD_VHV_SEARCH_VHV_VALUE_SZ \
	((uint16_t) 64)

#define VL53L5_VSD_VHV_SEARCH_SILICON_TEMP_DEGC_SZ \
	((uint16_t) 64)

#define VL53L5_CAL_REF_SPAD_SEARCH_META_DATA_SZ \
	((uint16_t) 4)

#define VL53L5_CAL_REF_SPAD_SEARCH_DATA_SZ \
	((uint16_t) 384)

#define VL53L5_CRSSD_CAL_REF_SPAD_SEARCH_TOTAL_RATE_MCPS_SZ \
	((uint16_t) 256)

#define VL53L5_CRSSD_CAL_REF_SPAD_SEARCH_COUNT_SZ \
	((uint16_t) 64)

#define VL53L5_CRSSD_CAL_REF_SPAD_SEARCH_APERTURE_SZ \
	((uint16_t) 64)

#define VL53L5_XTALK_MON_META_DATA_SZ \
	((uint16_t) 4)

#define VL53L5_XTALK_MON_ZONES_SZ \
	((uint16_t) 32)

#define VL53L5_XMZ_XTALK_MON_ZONE_X_OFF_SZ \
	((uint16_t) 8)

#define VL53L5_XMZ_XTALK_MON_ZONE_Y_OFF_SZ \
	((uint16_t) 8)

#define VL53L5_XMZ_XTALK_MON_ZONE_WIDTH_SZ \
	((uint16_t) 8)

#define VL53L5_XMZ_XTALK_MON_ZONE_HEIGHT_SZ \
	((uint16_t) 8)

#define VL53L5_XTALK_MON_DATA_SZ \
	((uint16_t) 112)

#define VL53L5_XMD_XTALK_MON_PEAK_RATE_KCPS_SPAD_SZ \
	((uint16_t) 32)

#define VL53L5_XMD_XTALK_MON_AMB_RATE_KCPS_SPAD_SZ \
	((uint16_t) 32)

#define VL53L5_XMD_XTALK_MON_PEAK_RATE_SIGMA_KCPS_PER_SPAD_SZ \
	((uint16_t) 32)

#define VL53L5_XMD_XTALK_MON_MPX_COUNT_SZ \
	((uint16_t) 16)

#define VL53L5_DYN_XTALK_PERSISTENT_DATA_SZ \
	((uint16_t) 28)

#define VL53L5_ZONE_THRESH_STATUS_ARRAY_SZ \
	((uint16_t) 8)

#define VL53L5_ZTSA_ZONE_THRESH_STATUS_BYTES_SZ \
	((uint16_t) 8)

#define VL53L5_NVM_LASER_SAFETY_NVM2_SZ \
	((uint16_t) 4)

#define VL53L5_OUTPUT_BH_CFG_SZ \
	((uint16_t) 8)

#define VL53L5_OUTPUT_BH_ENABLES_SZ \
	((uint16_t) 16)

#define VL53L5_OBE_OP_BH_ENABLES_SZ \
	((uint16_t) 16)

#define VL53L5_OUTPUT_BH_LIST_SZ \
	((uint16_t) 512)

#define VL53L5_OBL_OP_BH_LIST_SZ \
	((uint16_t) 512)

#define VL53L5_PATCH_HOOK_ENABLES_SZ \
	((uint16_t) 16)

#define VL53L5_PHE_PATCH_HOOK_ENABLES_SZ \
	((uint16_t) 16)

#ifdef __cplusplus
}
#endif

#endif
