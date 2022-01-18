
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

#ifndef __DCI_DEFS_H__
#define __DCI_DEFS_H__

#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DCI_DEF__FW_WORKSPACE_SIZE \
	((uint32_t) 2560U)

#define DCI_DEF__UI_OUTPUT_BUFFER_SIZE \
	((uint32_t) 2816U)

#define DCI_DEF__UI_COMMAND_BUFFER_SIZE \
	((uint32_t) 256U)

#define DCI_MAX_CHANNELS \
	((uint32_t) 16U)

#define DCI_MAX_ZONES \
	((uint32_t) 64U)

#define DCI_MAX_GLOBAL_TARGETS \
	((uint32_t) 8U)
#define DCI_MAX_TARGET_PER_ZONE \
	((uint32_t) 4U)
#define DCI_MAX_TARGETS \
	((uint32_t) 256U)
#define DCI_MAX_HIST_BINS \
	((uint32_t) 144U)
#define DCI_MAX_XTALK_SHAPE_BINS \
	((uint32_t) 144U)
#define DCI_REF_MPX_MAX_COLS \
	((uint32_t) 4U)
#define DCI_REF_MPX_MAX_ROWS \
	((uint32_t) 2U)
#define DCI_RTN_MPX_MAX_COLS \
	((uint32_t) 14U)
#define DCI_RTN_MPX_MAX_ROWS \
	((uint32_t) 10U)
#define DCI_OFFSET_CAL_MAX_COLS \
	((uint32_t) 8U)
#define DCI_OFFSET_CAL_MAX_ROWS \
	((uint32_t) 8U)
#define DCI_XTALK_CAL_MAX_COLS \
	((uint32_t) 8U)
#define DCI_XTALK_CAL_MAX_ROWS \
	((uint32_t) 8U)
#define DCI_XTALK_MON_MAX_ZONES \
	((uint32_t) 8U)
#define DCI_INITIAL_IDX \
	((uint32_t) 0U)
#define DCI_REF_MPX_MAX_COLS_X_MAX_ROWS \
	((uint32_t) DCI_REF_MPX_MAX_COLS * DCI_REF_MPX_MAX_ROWS)
#define DCI_RTN_MPX_MAX_COLS_X_MAX_ROWS \
	((uint32_t) DCI_RTN_MPX_MAX_COLS * DCI_RTN_MPX_MAX_ROWS)
#define DCI_OFFSET_CAL_MAX_COLS_X_MAX_ROWS \
	((uint32_t) 64U)
#define DCI_XTALK_CAL_MAX_COLS_X_MAX_ROWS \
	((uint32_t) 64U)
#define DCI_TRIG_CORR_MAX_COLS_X_MAX_ROWS \
	((uint32_t) 64U)
#define DCI_REF_ARRAY_WIDTH_SPADS \
	((uint32_t) 16U)
#define DCI_REF_ARRAY_HEIGHT_SPADS \
	((uint32_t) 8U)
#define DCI_REF_ARRAY_WIDTH_X_HEIGHT_SPADS \
	((uint32_t) DCI_REF_ARRAY_WIDTH_SPADS * DCI_REF_ARRAY_HEIGHT_SPADS)
#define DCI_REF_ARRAY_ENABLE_SIZE_BYTES \
	((uint32_t) 16U)
#define DCI_RTN_ARRAY_WIDTH_SPADS \
	((uint32_t) 56U)
#define DCI_RTN_ARRAY_HEIGHT_SPADS \
	((uint32_t) 40U)
#define DCI_RTN_ARRAY_WIDTH_X_HEIGHT_SPADS \
	((uint32_t) DCI_RTN_ARRAY_WIDTH_SPADS * DCI_RTN_ARRAY_HEIGHT_SPADS)
#define DCI_RTN_ARRAY_ENABLE_SIZE_BYTES \
	((uint32_t) 280U)
#define DCI_RTN_MPX_BLOCK_INDEX_COUNT \
	((uint32_t) 140U)
#define DCI_RTN_MPX_BLOCK_DEFINITIONS_COUNT \
	((uint32_t) 16U)
#define DCI_RTN_MPX_ENABLES_SIZE_BYTES \
	((uint32_t) 18U)

#define DCI_RTN_MPX_ENABLES_BUFFER_BYTES \
	((uint32_t) 20U)
#define DCI_VHV_MAX_SEARCH_STEPS \
	((uint32_t) 64U)
#define DCI_CAL_REF_SPAD_MAX_SEARCH_STEPS \
	((uint32_t) 64U)
#define DCI_DEF__MIN_BIN_IDX \
	((uint32_t) 0U)
#define DCI_DEF__MAX_BIN_IDX \
	((uint32_t) 255U)
#define DCI_DEF__MIN_RATE_KCPS_PER_SPAD \
	((uint32_t) 0U)

#define DCI_DEF__MAX_RATE_KCPS_PER_SPAD \
	((uint32_t) 1073741823U)
#define DCI_DEF__MIN_RATE_MCPS \
	((uint32_t) 0U)

#define DCI_DEF__MAX_RATE_MCPS \
	((uint32_t) 524287U)
#define DCI_DEF__MIN_PHASE \
	((uint32_t) 0U)

#define DCI_DEF__MAX_PHASE \
	((uint32_t) 524287U)

#define DCI_DEF__MIN_RANGE_MM \
	((int32_t) -32768)

#define DCI_DEF__MAX_RANGE_MM \
	((int32_t) 32767)
#define DCI_DEF__MIN_RNG_SIGMA_EST_MM \
	((uint32_t) 0U)

#define DCI_DEF__MAX_RNG_SIGMA_EST_MM \
	((uint32_t) 65535U)
#define DCI_DEF__MIN_REFLECT_EST_PC \
	((uint32_t) 0U)

#define DCI_DEF__MAX_REFLECT_EST_PC \
	((uint32_t) 255U)

#define DCI__ZONE_THRESH_STATUS_ARRAY_SIZE \
	((uint32_t) 8U)

#define DCI_INTERRUPT_CFG_MAX_IC_CHECKERS \
	((uint32_t) 64U)
#define DCI_DEF__MAX_OP_BH_ENABLE_SIZE \
	((uint32_t) 4U)
#define DCI_DEF__MAX_OP_BH_LIST_SIZE \
	((uint32_t) 128U)

#define DCI_DEF__PATCH_HOOK_ENABLE_WORD32 \
	((uint32_t) 4U)

#ifdef __cplusplus
}
#endif

#endif
