
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

#ifndef __DYN_XTALK_LUTS_H__
#define __DYN_XTALK_LUTS_H__

#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DYN_XTALK_NO_GRID_UPDATE \
	((uint8_t) 0U)
#define DYN_XTALK_GRID_UPDATE \
	((uint8_t) 1U)

#define DYN_XTALK_KEEP_GRID \
	((uint8_t) 0U)
#define DYN_XTALK_DISCARD_GRID \
	((uint8_t) 1U)
#define DYN_XTALK_FORCE_MAX_SAMPLES_KEEP_GRID \
	((uint8_t) 2U)

#define DYN_XTALK_DISABLE_XTALK_GRID_UPDATE \
	((uint8_t) 0U)
#define DYN_XTALK_ENABLE_XTALK_GRID_UPDATE \
	((uint8_t) 1U)

#define DYN_XTALK_CONTINUE \
	((uint8_t) 0U)
#define DYN_XTALK_RESET \
	((uint8_t) 1U)

#define DYN_XTALK_FALSE \
	((uint8_t) 0U)
#define DYN_XTALK_TRUE \
	((uint8_t) 1U)

#define TEMP_COMP_MODE_NONE \
	((uint8_t) 0U)
#define TEMP_COMP_MODE_DISABLED \
	((uint8_t) 1U)
#define TEMP_COMP_MODE_ENABLED \
	((uint8_t) 2U)

#define DYN_XTALK_ERROR_WARN_OVERFLOW \
	((uint8_t) 1U)
#define DYN_XTALK_ERROR_WARN_UNSUPPORTED_MODE \
	((uint8_t) 2U)
#define DYN_XTALK_ERROR_WARN_DEGRADED_PERFORMANCE \
	((uint8_t) 3U)

#define DYN_XTALK_STAGE_RESET \
	((uint8_t) 0U)
#define DYN_XTALK_STAGE_ACCUMULATOR_RESET \
	((uint8_t) 1U)
#define DYN_XTALK_STAGE_TEMPERATURE_COMPENSATION_GAIN \
	((uint8_t) 2U)
#define DYN_XTALK_STAGE_LINEAR_INTERPOLATION \
	((uint8_t) 3U)
#define DYN_XTALK_STAGE_LEAKAGE_GATING \
	((uint8_t) 4U)
#define DYN_XTALK_STAGE_LEAKAGE_COMPENSATION \
	((uint8_t) 5U)
#define DYN_XTALK_STAGE_GRID_INDEX \
	((uint8_t) 6U)
#define DYN_XTALK_STAGE_ACCUMULATE_MONITORS \
	((uint8_t) 7U)
#define DYN_XTALK_STAGE_INVERSE_TEMPERATURE_COMPENSATION \
	((uint8_t) 8U)
#define DYN_XTALK_STAGE_FORWARD_TEMPERATURE_COMPENSATION \
	((uint8_t) 9U)
#define DYN_XTALK_STAGE_NEW_VARIANCE_GRID \
	((uint8_t) 10U)
#define DYN_XTALK_STAGE_VARIANCE_GRID_POINT \
	((uint8_t) 11U)
#define DYN_XTALK_STAGE_VARIANCE_TO_SIGMA \
	((uint8_t) 12U)
#define DYN_XTALK_STAGE_DESIRED_SAMPLES_CHECK \
	((uint8_t) 13U)
#define DYN_XTALK_STAGE_REACHED_MAX_SAMPLES_OR_FRAMES \
	((uint8_t) 14U)
#define DYN_XTALK_STAGE_FINISHED_ACCUMULATING \
	((uint8_t) 15U)
#define DYN_XTALK_STAGE_TX_SUB_FRAME_GRID_TO_OTF \
	((uint8_t) 16U)
#define DYN_XTALK_STAGE_TX_OTF_GRID_TO_OUTPUT_GRID \
	((uint8_t) 17U)

#define DYN_XTALK_RESET_MODE__START_WITH_PREVIOUS_GRID \
	((uint8_t) 0U)

#define DYN_XTALK_RESET_MODE__START_WITH_CAL_GRID \
	((uint8_t) 1U)

#define DYN_XTALK_RESET_MODE__START_WITH_XTALK_MON \
	((uint8_t) 2U)

#define DYN_XTALK_MODE__DISABLED \
	((uint8_t) 1U)

#define DYN_XTALK_MODE__ENABLED \
	((uint8_t) 2U)

#ifdef __cplusplus
}
#endif

#endif
