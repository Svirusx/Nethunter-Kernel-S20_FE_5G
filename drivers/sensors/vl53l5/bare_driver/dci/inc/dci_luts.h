
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

#ifndef __DCI_LUTS_H__
#define __DCI_LUTS_H__

#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DCI_BH__P_TYPE__CONSEC_PARAMS_LIST \
	((uint8_t) 0U)

#define DCI_BH__P_TYPE__GRP_PARAMS_START \
	((uint8_t) 13U)

#define DCI_BH__P_TYPE__GRP_PARAMS_END \
	((uint8_t) 14U)

#define DCI_BH__P_TYPE__END_OF_DATA \
	((uint8_t) 15U)

#define DCI_FW_STATE_VAL__NOUPDATE \
	((uint8_t) 0U)

#define DCI_FW_STATE_VAL__RESET_RETENTION_STATE \
	((uint8_t) 1U)

#define DCI_FW_STATE_VAL__BOOT_STATE \
	((uint8_t) 2U)

#define DCI_FW_STATE_VAL__DFT_STATE \
	((uint8_t) 3U)

#define DCI_FW_STATE_VAL__MAIN_STATE \
	((uint8_t) 4U)

#define DCI_FW_STATE_VAL__ACTIVE_STATE \
	((uint8_t) 5U)

#define DCI_FW_STATE_VAL__IDLE_STATE \
	((uint8_t) 6U)

#define DCI_FW_MODE_STATUS__NO_MODE \
	((uint8_t) 0U)

#define DCI_FW_MODE_STATUS__STREAMING \
	((uint8_t) 1U)

#define DCI_FW_MODE_STATUS__SINGLE_SHOT \
	((uint8_t) 2U)

#define DCI_FW_MODE_STATUS__AUTONOMOUS \
	((uint8_t) 3U)

#define DCI_CMD_STATUS__NOUPDATE \
	((uint8_t) 0U)

#define DCI_CMD_STATUS__RECEIVED \
	((uint8_t) 1U)

#define DCI_CMD_STATUS__PROCESSING \
	((uint8_t) 2U)

#define DCI_CMD_STATUS__COMPLETED \
	((uint8_t) 3U)

#define DCI_CMD_STATUS__ERROR \
	((uint8_t) 4U)

#define DCI_DEVICE_STATUS__NOUPDATE \
	((uint8_t) 0U)

#define DCI_DEVICE_STATUS__OK \
	((uint8_t) 1U)

#define DCI_DEVICE_STATUS__VCSELCONTINUITYTESTFAILURE \
	((uint8_t) 2U)

#define DCI_DEVICE_STATUS__VCSELWATCHDOGTESTFAILURE \
	((uint8_t) 3U)

#define DCI_DEVICE_STATUS__VCSELREFERENCERATEFAILURE \
	((uint8_t) 4U)

#define DCI_DEVICE_STATUS__NOVHVVALUEFOUND \
	((uint8_t) 5U)

#define DCI_DEVICE_STATUS__USERROICLIP \
	((uint8_t) 6U)

#define DCI_DEVICE_STATUS__MULTCLIPFAIL \
	((uint8_t) 7U)

#define DCI_TARGET_STATUS__NOUPDATE \
	((uint8_t) 0U)

#define DCI_TARGET_STATUS__MSRCNOTARGET \
	((uint8_t) 1U)

#define DCI_TARGET_STATUS__RANGEPHASECHECK \
	((uint8_t) 2U)

#define DCI_TARGET_STATUS__SIGMATHRESHOLDCHECK \
	((uint8_t) 3U)

#define DCI_TARGET_STATUS__PHASECONSISTENCY \
	((uint8_t) 4U)

#define DCI_TARGET_STATUS__RANGECOMPLETE \
	((uint8_t) 5U)

#define DCI_TARGET_STATUS__RANGECOMPLETE_NO_WRAP_CHECK \
	((uint8_t) 6U)

#define DCI_TARGET_STATUS__EVENTCONSISTENCY \
	((uint8_t) 7U)

#define DCI_TARGET_STATUS__MINSIGNALEVENTCHECK \
	((uint8_t) 8U)

#define DCI_TARGET_STATUS__RANGECOMPLETE_MERGED_PULSE \
	((uint8_t) 9U)

#define DCI_TARGET_STATUS__PREV_RANGE_NO_TARGETS \
	((uint8_t) 10U)
#define DCI_TARGET_STATUS__RANGEIGNORETHRESHOLD \
	((uint8_t) 11U)

#define DCI_TARGET_STATUS__TARGETDUETOBLUR \
	((uint8_t) 12U)

#define DCI_CMD_ID__NULL \
	((uint8_t) 0U)

#define DCI_CMD_ID__SET_PARMS \
	((uint8_t) 1U)

#define DCI_CMD_ID__GET_PARMS \
	((uint8_t) 2U)

#define DCI_CMD_ID__START_RANGE \
	((uint8_t) 3U)

#define DCI_CMD_ID__STOP_RANGE \
	((uint8_t) 4U)

#define DCI_DISTANCE_MODE__NONE \
	((uint8_t) 0U)

#define DCI_DISTANCE_MODE__AUTO \
	((uint8_t) 1U)

#define DCI_DISTANCE_MODE__SHORT \
	((uint8_t) 2U)

#define DCI_DISTANCE_MODE__LONG \
	((uint8_t) 3U)

#define DCI_RNG_MODE__NONE \
	((uint8_t) 0U)

#define DCI_RNG_MODE__BACK_TO_BACK \
	((uint8_t) 1U)

#define DCI_RNG_MODE__SINGLE_SHOT \
	((uint8_t) 2U)

#define DCI_RNG_MODE__TIMED \
	((uint8_t) 3U)

#define DCI_RNG_MODE__CAL__REF_SPAD \
	((uint8_t) 4U)

#define DCI_RNG_MODE__CAL__REF_SPAD_WITH_VHV_SEARCH \
	((uint8_t) 5U)

#define DCI_RNG_MODE__CAL__XTALK__SINGLE_TARGET \
	((uint8_t) 6U)

#define DCI_RNG_MODE__CAL__XTALK__DUAL_TARGET_0 \
	((uint8_t) 7U)

#define DCI_RNG_MODE__CAL__XTALK__DUAL_TARGET_1 \
	((uint8_t) 8U)

#define DCI_RNG_MODE__CAL__OFFSET_AND_DMAX \
	((uint8_t) 9U)

#define DCI_RNG_MODE__TEST__VHV_SEARCH \
	((uint8_t) 10U)

#define DCI_RNG_MODE__TEST__SSC__DCR \
	((uint8_t) 11U)

#define DCI_RNG_MODE__TEST__SSC__LCR \
	((uint8_t) 12U)

#define DCI_RNG_MODE__TEST__SSC__VCR \
	((uint8_t) 13U)

#define DCI_RNG_MODE__BACK_TO_BACK_BLANK \
	((uint8_t) 14U)

#define DCI_B2BB_DISABLE__RNG_CORE \
	((uint8_t) 1U)

#define DCI_B2BB_DISABLE__PLL \
	((uint8_t) 2U)

#define DCI_LIVE_XTALK_MODE__NONE \
	((uint8_t) 0U)

#define DCI_LIVE_XTALK_MODE__DISABLED \
	((uint8_t) 1U)

#define DCI_LIVE_XTALK_MODE__ENABLED \
	((uint8_t) 2U)

#define DCI_INTEGRATION_MODE__NONE \
	((uint8_t) 0U)

#define DCI_INTEGRATION_MODE__MIN \
	((uint8_t) 1U)

#define DCI_INTEGRATION_MODE__MAX \
	((uint8_t) 2U)

#define DCI_INTEGRATION_MODE__RANGING_RATE \
	((uint8_t) 3U)

#define DCI_INTEGRATION_MODE__AUTO__INVERSE_DMAX \
	((uint8_t) 4U)

#define DCI_INTEGRATION_MODE__AUTO__RNG_SIGMA \
	((uint8_t) 5U)

#define DCI_BLANKING_MODE__OFF \
	((uint8_t) 0U)

#define DCI_BLANKING_MODE__BA \
	((uint8_t) 1U)

#define DCI_BLANKING_MODE__AB_BA \
	((uint8_t) 2U)

#define DCI_BLANKING_TYPE__INVALID \
	((uint8_t) 0U)

#define DCI_BLANKING_TYPE__NORMAL \
	((uint8_t) 1U)

#define DCI_BLANKING_TYPE__RETENTION \
	((uint8_t) 2U)

#define DCI_BLANKING_TYPE__INTERRUPT \
	((uint8_t) 3U)

#define DCI_DSS_MODE__NONE \
	((uint8_t) 0U)

#define DCI_DSS_MODE__AUTO \
	((uint8_t) 1U)

#define DCI_DSS_MODE__PAUSE \
	((uint8_t) 2U)

#define DCI_DSS_MODE__MANUAL \
	((uint8_t) 3U)

#define DCI_DSS_SPATIAL_MODE__INVALID \
	((uint8_t) 0U)

#define DCI_DSS_SPATIAL_MODE__OFF \
	((uint8_t) 1U)

#define DCI_DSS_SPATIAL_MODE__ON \
	((uint8_t) 2U)

#define DCI_HIST_TYPE__INVALID \
	((uint8_t) 0U)

#define DCI_HIST_TYPE__RTN_ARRAY \
	((uint8_t) 1U)

#define DCI_HIST_TYPE__REF_ARRAY \
	((uint8_t) 2U)

#define DCI_HIST_TYPE__XTALK \
	((uint8_t) 3U)

#define DCI_HIST_TYPE__OFFSET \
	((uint8_t) 4U)

#define DCI_HIST_TYPE__FLEX \
	((uint8_t) 5U)

#define DCI_HIST_TYPE__XTALK__FLEX \
	((uint8_t) 6U)

#define DCI_HIST_TYPE__OFFSET__FLEX \
	((uint8_t) 7U)

#define DCI_MPX_TYPE__INVALID \
	((uint8_t) 0U)

#define DCI_MPX_TYPE__RTN_ARRAY \
	((uint8_t) 1U)

#define DCI_MPX_TYPE__REF_ARRAY \
	((uint8_t) 2U)

#define DCI_MPX_TYPE__RTN_XTALK \
	((uint8_t) 3U)

#define DCI_MPX_TYPE__RTN_OFFSET \
	((uint8_t) 4U)

#define DCI__AMB_RATE_KCPS_PER_SPAD__EN \
	((uint32_t) 1U)
#define DCI__AMB_RATE_MCPS__EN \
	((uint32_t) 2U)
#define DCI__RNG__EFFECTIVE_SPAD_COUNT__EN \
	((uint32_t) 4U)
#define DCI__AMB_DMAX_MM__EN \
	((uint32_t) 8U)
#define DCI__RNG__NO_OF_TARGETS__EN \
	((uint32_t) 16U)
#define DCI__RNG__ZONE_ID__EN \
	((uint32_t) 32U)

#define DCI__PEAK_RATE_KCPS_PER_SPAD__EN \
	((uint32_t) 1U)
#define DCI__PEAK_RATE_MCPS__EN \
	((uint32_t) 2U)
#define DCI__MEDIAN_RANGE_MM__EN \
	((uint32_t) 4U)
#define DCI__MEDIAN_PHASE__EN \
	((uint32_t) 8U)
#define DCI__TARGET_RANGING_EVENTS__EN \
	((uint32_t) 16U)
#define DCI__TARGET_AMB_EVENTS__EN \
	((uint32_t) 32U)
#define DCI__SIGNAL_SIGMA_EVENTS__EN \
	((uint32_t) 64U)
#define DCI__RANGE_SIIGMA_MM__EN \
	((uint32_t) 128U)
#define DCI__MIN_RANGE_DELTA_MM__EN \
	((uint32_t) 256U)
#define DCI__MAX_RANGE_DELTA_MM__EN \
	((uint32_t) 512U)
#define DCI__TARGET_STATUS__EN \
	((uint32_t) 1024U)

#define DCI__GBL_TGT__PEAK_RATE_KCPS_PER_SPAD__EN \
	((uint32_t) 1U)
#define DCI__GBL_TGT__AMB_RATE_KCPS_PER_SPAD__EN \
	((uint32_t) 2U)
#define DCI__GBL_TGT__PEAK_RATE_MCPS__EN \
	((uint32_t) 4U)
#define DCI__GBL_TGT__AMB_RATE_MCPS__EN \
	((uint32_t) 8U)
#define DCI__GBL_TGT__MEDIAN_RANGE_MM__EN \
	((uint32_t) 16U)
#define DCI__GBL_TGT__MIN_RANGE_DELTA_MM__EN \
	((uint32_t) 32U)
#define DCI__GBL_TGT__MAX_RANGE_DELTA_MM__EN \
	((uint32_t) 64U)
#define DCI__GBL_TGT__STATUS__EN \
	((uint32_t) 128U)
#define DCI__GBL_TGT__X_CENTRE__EN \
	((uint32_t) 256U)
#define DCI__GBL_TGT__Y_CENTRE__EN \
	((uint32_t) 512U)
#define DCI__GBL_TGT__WIDTH__EN \
	((uint32_t) 1024U)
#define DCI__GBL_TGT__HEIGHT__EN \
	((uint32_t) 2048U)
#define DCI__GBL_TGT__NO_OF_ROIS__EN \
	((uint32_t) 4096U)

#define DCI__RTN_MPX__PEAK_RATE_KCPS_PER_SPAD__EN \
	((uint32_t) 1U)
#define DCI__RTN_MPX__AMB_RATE_KCPS_PER_SPAD__EN \
	((uint32_t) 2U)
#define DCI__RTN_MPX__PEAK_RATE_MCPS__EN \
	((uint32_t) 4U)
#define DCI__RTN_MPX__AMB_RATE_MCPS__EN \
	((uint32_t) 8U)
#define DCI__RTN_MPX__RANGING_TOTAL_EVENTS__EN \
	((uint32_t) 16U)
#define DCI__RTN_MPX__AMBIENT_WINDOW_EVENTS__EN \
	((uint32_t) 32U)
#define DCI__RTN_MPX__EFFECTIVE_SPAD_COUNT__EN \
	((uint32_t) 64U)
#define DCI__RTN_MPX__CONFIDENCE__EN \
	((uint32_t) 128U)

#define DCI__DYN_XTALK_LEAKAGE_FULL_IDX \
	((uint8_t) 0U)
#define DCI__DYN_XTALK_LEAKAGE_LEFT_IDX \
	((uint8_t) 1U)
#define DCI__DYN_XTALK_LEAKAGE_RIGHT_IDX \
	((uint8_t) 2U)
#define DCI__DYN_XTALK_LEAKAGE_SPARE_IDX \
	((uint8_t) 3U)
#define DCI__DYN_XTALK_MONITOR_LEFT_1_IDX \
	((uint8_t) 4U)
#define DCI__DYN_XTALK_MONITOR_LEFT_2_IDX \
	((uint8_t) 5U)
#define DCI__DYN_XTALK_MONITOR_RIGHT_1_IDX \
	((uint8_t) 6U)
#define DCI__DYN_XTALK_MONITOR_RIGHT_2_IDX \
	((uint8_t) 7U)

#define DCI__INTERRUPT_CFG_MODE__NONE \
	((uint8_t) 0U)
#define DCI__INTERRUPT_CFG_MODE__NEW_DATA_READY \
	((uint8_t) 1U)
#define DCI__INTERRUPT_CFG_MODE__THRESHOLDS \
	((uint8_t) 2U)

#define DCI__PHOOK_EN_00_HOOK_000 \
	((uint32_t) 1U)
#define DCI__PHOOK_EN_00_HOOK_001 \
	((uint32_t) 2U)
#define DCI__PHOOK_EN_00_HOOK_002 \
	((uint32_t) 4U)
#define DCI__PHOOK_EN_00_HOOK_003 \
	((uint32_t) 8U)
#define DCI__PHOOK_EN_00_HOOK_004 \
	((uint32_t) 16U)
#define DCI__PHOOK_EN_00_HOOK_005 \
	((uint32_t) 32U)
#define DCI__PHOOK_EN_00_HOOK_006 \
	((uint32_t) 64U)
#define DCI__PHOOK_EN_00_HOOK_007 \
	((uint32_t) 128U)
#define DCI__PHOOK_EN_00_HOOK_008 \
	((uint32_t) 256U)
#define DCI__PHOOK_EN_00_HOOK_009 \
	((uint32_t) 512U)
#define DCI__PHOOK_EN_00_HOOK_010 \
	((uint32_t) 1024U)
#define DCI__PHOOK_EN_00_HOOK_011 \
	((uint32_t) 2048U)
#define DCI__PHOOK_EN_00_HOOK_012 \
	((uint32_t) 4096U)
#define DCI__PHOOK_EN_00_HOOK_013 \
	((uint32_t) 8192U)
#define DCI__PHOOK_EN_00_HOOK_014 \
	((uint32_t) 16384U)
#define DCI__PHOOK_EN_00_HOOK_015 \
	((uint32_t) 32768U)
#define DCI__PHOOK_EN_00_HOOK_016 \
	((uint32_t) 65536U)
#define DCI__PHOOK_EN_00_HOOK_017 \
	((uint32_t) 131072U)
#define DCI__PHOOK_EN_00_HOOK_018 \
	((uint32_t) 262144U)
#define DCI__PHOOK_EN_00_HOOK_019 \
	((uint32_t) 524288U)
#define DCI__PHOOK_EN_00_HOOK_020 \
	((uint32_t) 1048576U)
#define DCI__PHOOK_EN_00_HOOK_021 \
	((uint32_t) 2097152U)
#define DCI__PHOOK_EN_00_HOOK_022 \
	((uint32_t) 4194304U)
#define DCI__PHOOK_EN_00_HOOK_023 \
	((uint32_t) 8388608U)
#define DCI__PHOOK_EN_00_HOOK_024 \
	((uint32_t) 16777216U)
#define DCI__PHOOK_EN_00_HOOK_025 \
	((uint32_t) 33554432U)
#define DCI__PHOOK_EN_00_HOOK_026 \
	((uint32_t) 67108864U)
#define DCI__PHOOK_EN_00_HOOK_027 \
	((uint32_t) 134217728U)
#define DCI__PHOOK_EN_00_HOOK_028 \
	((uint32_t) 268435456U)
#define DCI__PHOOK_EN_00_HOOK_029 \
	((uint32_t) 536870912U)
#define DCI__PHOOK_EN_00_HOOK_030 \
	((uint32_t) 1073741824U)
#define DCI__PHOOK_EN_00_HOOK_031 \
	((uint32_t) 2147483648U)
#define DCI__PHOOK_EN_01_HOOK_032 \
	((uint32_t) 1U)
#define DCI__PHOOK_EN_01_HOOK_033 \
	((uint32_t) 2U)
#define DCI__PHOOK_EN_01_HOOK_034 \
	((uint32_t) 4U)
#define DCI__PHOOK_EN_01_HOOK_035 \
	((uint32_t) 8U)
#define DCI__PHOOK_EN_01_HOOK_036 \
	((uint32_t) 16U)
#define DCI__PHOOK_EN_01_HOOK_037 \
	((uint32_t) 32U)
#define DCI__PHOOK_EN_01_HOOK_038 \
	((uint32_t) 64U)
#define DCI__PHOOK_EN_01_HOOK_039 \
	((uint32_t) 128U)
#define DCI__PHOOK_EN_01_HOOK_040 \
	((uint32_t) 256U)
#define DCI__PHOOK_EN_01_HOOK_041 \
	((uint32_t) 512U)
#define DCI__PHOOK_EN_01_HOOK_042 \
	((uint32_t) 1024U)
#define DCI__PHOOK_EN_01_HOOK_043 \
	((uint32_t) 2048U)
#define DCI__PHOOK_EN_01_HOOK_044 \
	((uint32_t) 4096U)
#define DCI__PHOOK_EN_01_HOOK_045 \
	((uint32_t) 8192U)
#define DCI__PHOOK_EN_01_HOOK_046 \
	((uint32_t) 16384U)
#define DCI__PHOOK_EN_01_HOOK_047 \
	((uint32_t) 32768U)
#define DCI__PHOOK_EN_01_HOOK_048 \
	((uint32_t) 65536U)
#define DCI__PHOOK_EN_01_HOOK_049 \
	((uint32_t) 131072U)
#define DCI__PHOOK_EN_01_HOOK_050 \
	((uint32_t) 262144U)
#define DCI__PHOOK_EN_01_HOOK_051 \
	((uint32_t) 524288U)
#define DCI__PHOOK_EN_01_HOOK_052 \
	((uint32_t) 1048576U)
#define DCI__PHOOK_EN_01_HOOK_053 \
	((uint32_t) 2097152U)
#define DCI__PHOOK_EN_01_HOOK_054 \
	((uint32_t) 4194304U)
#define DCI__PHOOK_EN_01_HOOK_055 \
	((uint32_t) 8388608U)
#define DCI__PHOOK_EN_01_HOOK_056 \
	((uint32_t) 16777216U)
#define DCI__PHOOK_EN_01_HOOK_057 \
	((uint32_t) 33554432U)
#define DCI__PHOOK_EN_01_HOOK_058 \
	((uint32_t) 67108864U)
#define DCI__PHOOK_EN_01_HOOK_059 \
	((uint32_t) 134217728U)
#define DCI__PHOOK_EN_01_HOOK_060 \
	((uint32_t) 268435456U)
#define DCI__PHOOK_EN_01_HOOK_061 \
	((uint32_t) 536870912U)
#define DCI__PHOOK_EN_01_HOOK_062 \
	((uint32_t) 1073741824U)
#define DCI__PHOOK_EN_01_HOOK_063 \
	((uint32_t) 2147483648U)
#define DCI__PHOOK_EN_02_HOOK_064 \
	((uint32_t) 1U)
#define DCI__PHOOK_EN_02_HOOK_065 \
	((uint32_t) 2U)
#define DCI__PHOOK_EN_02_HOOK_066 \
	((uint32_t) 4U)
#define DCI__PHOOK_EN_02_HOOK_067 \
	((uint32_t) 8U)
#define DCI__PHOOK_EN_02_HOOK_068 \
	((uint32_t) 16U)
#define DCI__PHOOK_EN_02_HOOK_069 \
	((uint32_t) 32U)
#define DCI__PHOOK_EN_02_HOOK_070 \
	((uint32_t) 64U)
#define DCI__PHOOK_EN_02_HOOK_071 \
	((uint32_t) 128U)
#define DCI__PHOOK_EN_02_HOOK_072 \
	((uint32_t) 256U)
#define DCI__PHOOK_EN_02_HOOK_073 \
	((uint32_t) 512U)
#define DCI__PHOOK_EN_02_HOOK_074 \
	((uint32_t) 1024U)
#define DCI__PHOOK_EN_02_HOOK_075 \
	((uint32_t) 2048U)
#define DCI__PHOOK_EN_02_HOOK_076 \
	((uint32_t) 4096U)
#define DCI__PHOOK_EN_02_HOOK_077 \
	((uint32_t) 8192U)
#define DCI__PHOOK_EN_02_HOOK_078 \
	((uint32_t) 16384U)
#define DCI__PHOOK_EN_02_HOOK_079 \
	((uint32_t) 32768U)
#define DCI__PHOOK_EN_02_HOOK_080 \
	((uint32_t) 65536U)
#define DCI__PHOOK_EN_02_HOOK_081 \
	((uint32_t) 131072U)
#define DCI__PHOOK_EN_02_HOOK_082 \
	((uint32_t) 262144U)
#define DCI__PHOOK_EN_02_HOOK_083 \
	((uint32_t) 524288U)
#define DCI__PHOOK_EN_02_HOOK_084 \
	((uint32_t) 1048576U)
#define DCI__PHOOK_EN_02_HOOK_085 \
	((uint32_t) 2097152U)
#define DCI__PHOOK_EN_02_HOOK_086 \
	((uint32_t) 4194304U)
#define DCI__PHOOK_EN_02_HOOK_087 \
	((uint32_t) 8388608U)
#define DCI__PHOOK_EN_02_HOOK_088 \
	((uint32_t) 16777216U)
#define DCI__PHOOK_EN_02_HOOK_089 \
	((uint32_t) 33554432U)
#define DCI__PHOOK_EN_02_HOOK_090 \
	((uint32_t) 67108864U)
#define DCI__PHOOK_EN_02_HOOK_091 \
	((uint32_t) 134217728U)
#define DCI__PHOOK_EN_02_HOOK_092 \
	((uint32_t) 268435456U)
#define DCI__PHOOK_EN_02_HOOK_093 \
	((uint32_t) 536870912U)
#define DCI__PHOOK_EN_02_HOOK_094 \
	((uint32_t) 1073741824U)
#define DCI__PHOOK_EN_02_HOOK_095 \
	((uint32_t) 2147483648U)
#define DCI__PHOOK_EN_03_HOOK_096 \
	((uint32_t) 1U)
#define DCI__PHOOK_EN_03_HOOK_097 \
	((uint32_t) 2U)
#define DCI__PHOOK_EN_03_HOOK_098 \
	((uint32_t) 4U)
#define DCI__PHOOK_EN_03_HOOK_099 \
	((uint32_t) 8U)
#define DCI__PHOOK_EN_03_HOOK_100 \
	((uint32_t) 16U)
#define DCI__PHOOK_EN_03_HOOK_101 \
	((uint32_t) 32U)
#define DCI__PHOOK_EN_03_HOOK_102 \
	((uint32_t) 64U)
#define DCI__PHOOK_EN_03_HOOK_103 \
	((uint32_t) 128U)
#define DCI__PHOOK_EN_03_HOOK_104 \
	((uint32_t) 256U)
#define DCI__PHOOK_EN_03_HOOK_105 \
	((uint32_t) 512U)
#define DCI__PHOOK_EN_03_HOOK_106 \
	((uint32_t) 1024U)
#define DCI__PHOOK_EN_03_HOOK_107 \
	((uint32_t) 2048U)
#define DCI__PHOOK_EN_03_HOOK_108 \
	((uint32_t) 4096U)
#define DCI__PHOOK_EN_03_HOOK_109 \
	((uint32_t) 8192U)
#define DCI__PHOOK_EN_03_HOOK_110 \
	((uint32_t) 16384U)
#define DCI__PHOOK_EN_03_HOOK_111 \
	((uint32_t) 32768U)
#define DCI__PHOOK_EN_03_HOOK_112 \
	((uint32_t) 65536U)
#define DCI__PHOOK_EN_03_HOOK_113 \
	((uint32_t) 131072U)
#define DCI__PHOOK_EN_03_HOOK_114 \
	((uint32_t) 262144U)
#define DCI__PHOOK_EN_03_HOOK_115 \
	((uint32_t) 524288U)
#define DCI__PHOOK_EN_03_HOOK_116 \
	((uint32_t) 1048576U)
#define DCI__PHOOK_EN_03_HOOK_117 \
	((uint32_t) 2097152U)
#define DCI__PHOOK_EN_03_HOOK_118 \
	((uint32_t) 4194304U)
#define DCI__PHOOK_EN_03_HOOK_119 \
	((uint32_t) 8388608U)
#define DCI__PHOOK_EN_03_HOOK_120 \
	((uint32_t) 16777216U)
#define DCI__PHOOK_EN_03_HOOK_121 \
	((uint32_t) 33554432U)
#define DCI__PHOOK_EN_03_HOOK_122 \
	((uint32_t) 67108864U)
#define DCI__PHOOK_EN_03_HOOK_123 \
	((uint32_t) 134217728U)
#define DCI__PHOOK_EN_03_HOOK_124 \
	((uint32_t) 268435456U)
#define DCI__PHOOK_EN_03_HOOK_125 \
	((uint32_t) 536870912U)
#define DCI__PHOOK_EN_03_HOOK_126 \
	((uint32_t) 1073741824U)
#define DCI__PHOOK_EN_03_HOOK_127 \
	((uint32_t) 2147483648U)

#ifdef __cplusplus
}
#endif

#endif
