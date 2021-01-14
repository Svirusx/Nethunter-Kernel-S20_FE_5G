
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

#ifndef __COMMON_DATATYPE_LUTS_H__
#define __COMMON_DATATYPE_LUTS_H__

#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FUNC_ERROR__NONE \
	((int16_t) 0)

#define FUNC_ERROR__PHOOK_OVERWRITE \
	((int16_t) 128)

#define FUNC_ERROR__WDOG_TMOUT \
	((int16_t) 129)

#define FUNC_ERROR__WDOG_FAIL \
	((int16_t) 130)

#define FUNC_ERROR__PLL_LOCK_TMOUT \
	((int16_t) 131)

#define FUNC_ERROR__CONT_TMOUT \
	((int16_t) 132)

#define FUNC_ERROR__CONT_FAIL \
	((int16_t) 133)

#define FUNC_ERROR__VCSELCP_TMOUT \
	((int16_t) 134)

#define FUNC_ERROR__VCSELCP_FAIL \
	((int16_t) 135)

#define FUNC_ERROR__POR_AVDD1V1_TMOUT \
	((int16_t) 136)

#define FUNC_ERROR__CONT_SKIP_VECTOR \
	((int16_t) 137)

#define FUNC_ERROR__INT_MGR \
	((int16_t) 138)

#define FUNC_ERROR__NVM_EN_FAILED \
	((int16_t) 139)

#define FUNC_ERROR__NVM_DONE_TMOUT \
	((int16_t) 140)

#define FUNC_ERROR__NVM_DISABLED_FAIL \
	((int16_t) 13)
#define FUNC_ERROR__TIMESTAMP_TMOUT \
	((int16_t) 141)

#define FUNC_ERROR__NVM_CRC_CHECKSUM_FAILED \
	((int16_t) 142)

#define FUNC_ERROR__ZONE_INVALID_NUM \
	((int16_t) 143)

#define FUNC_ERROR__ZONE_OUT_OF_ACTIVE_BOUND \
	((int16_t) 144)

#define FUNC_ERROR__DSS_OPTICAL_CENTRE_MISALIGNED \
	((int16_t) 17)

#define FUNC_ERROR__DSS_MIN_SIG_RATE_FAIL \
	((int16_t) 146)

#define FUNC_ERROR__DSS_MP_POSITION_INVALID \
	((int16_t) 147)

#define FUNC_ERROR__VHV_SEARCH_FAIL \
	((int16_t) 148)

#define FUNC_ERROR__RANGING_TIMEOUT \
	((int16_t) 149)

#define FUNC_ERROR__VCSEL_PERIOD_CLIPPED \
	((int16_t) 150)

#define FUNC_ERROR__VCSEL_STOP_CLIPPED \
	((int16_t) 151)

#define FUNC_ERROR__RANGING_WINDOW_CLIPPED \
	((int16_t) 152)

#define FUNC_ERROR__AMBIENT_WINDOW_DURATION_CLIPPED \
	((int16_t) 153)

#define FUNC_ERROR__SPAD_EN_XFER_FAIL \
	((int16_t) 154)

#define FUNC_ERROR__REF_SPAD_EN_TMOUT \
	((int16_t) 155)

#define FUNC_ERROR__RTN_SPAD_EN_TMOUT \
	((int16_t) 156)

#define FUNC_ERROR__SPAD_REF_STATUS_CHECK \
	((int16_t) 157)

#define FUNC_ERROR__SPAD_RTN_STATUS_CHECK \
	((int16_t) 158)

#define FUNC_ERROR__GRID_COLS_ROWS_OUT_BOUNDS \
	((int16_t) 159)

#define FUNC_ERROR__GRID_COLS_ROWS_NOT_MULT_2 \
	((int16_t) 160)

#define FUNC_ERROR__GRID_PARAM_NOT_MULT_4 \
	((int16_t) 161)

#define FUNC_ERROR__GRID_ZONES_NOT_SQUARE \
	((int16_t) 162)

#define FUNC_ERROR__GRID_ZONE_SIZE_OUT_BOUNDS \
	((int16_t) 163)

#define FUNC_ERROR__GRID_START_OUT_BOUNDS \
	((int16_t) 164)

#define FUNC_ERROR__INVALID_GRID_CONFIG \
	((int16_t) 165)

#define FUNC_ERROR__OPTICAL_CENTRE_OUT_BOUNDS \
	((int16_t) 166)

#define FUNC_ERROR__GRID_SIZE_TOO_LARGE \
	((int16_t) 167)

#define FUNC_ERROR__REPOSITION_ARRAY_FAIL \
	((int16_t) 40)

#define FUNC_WARNING__CALIBRATION \
	((int16_t) 42)

#define FUNC_WARNING__PARAM_CLIPPED \
	((int16_t) 43)

#define FUNC_WARNING__PARAM_INVALID \
	((int16_t) 44)

#define FUNC_ERROR__MODE_NOT_SUPPORTED \
	((int16_t) 45)

#define FUNC_ERROR__INVALID_CMD \
	((int16_t) 46)

#define FUNC_ERROR__NO_GPIO_PIN \
	((int16_t) 47)

#define FUNC_ERROR__GPIO_FN_NOT_SUPPORTED \
	((int16_t) 48)

#define FUNC_ERROR__CTRL_INTERFACE \
	((int16_t) 49)
#define FUNC_ERROR__VCSELCP_FAIL_RETRY \
	((int16_t) 50)

#define FUNC_ERROR__REF_SPAD_INIT \
	((int16_t) 178)

#define FUNC_ERROR__REF_SPAD_CHAR_NOT_ENOUGH \
	((int16_t) 51)

#define FUNC_ERROR__REF_SPAD_CHAR_RATE_HIGH \
	((int16_t) 52)

#define FUNC_ERROR__REF_SPAD_CHAR_RATE_LOW \
	((int16_t) 53)

#define FUNC_ERROR__REF_SPAD_APPLY_NUM_MAXED \
	((int16_t) 182)

#define FUNC_ERROR__REF_SPAD_APPLY_MIN_CLIP \
	((int16_t) 55)

#define FUNC_ERROR__XTALK_EXTRN_NO_SAMPLE_FAIL \
	((int16_t) 56)

#define FUNC_ERROR__XTALK_EXTRN_SIGMA_LIMIT_FAIL \
	((int16_t) 57)

#define FUNC_ERROR__XTALK_EXTRN_MISSING_SAMPLES \
	((int16_t) 58)

#define FUNC_ERROR__OFFSET_CAL_NO_SAMPLE_FAIL \
	((int16_t) 59)

#define FUNC_ERROR__OFFSET_CAL_NO_SPADS_EN_FAIL \
	((int16_t) 60)

#define FUNC_ERROR__OFFSET_CAL_MISSING_SAMPLES \
	((int16_t) 61)

#define FUNC_ERROR__OFFSET_CAL_SIGMA_HIGH \
	((int16_t) 62)

#define FUNC_ERROR__OFFSET_CAL_RATE_HIGH \
	((int16_t) 63)

#define FUNC_ERROR__OFFSET_CAL_SPAD_COUNT_LOW \
	((int16_t) 64)

#define FUNC_ERROR__LEGACY_MODE_CHECK_FAILED \
	((int16_t) 193)

#define FUNC_ERROR__UNIT_TEST_FAIL \
	((int16_t) 194)

#define FUNC_ERROR__DEBUG_DATA_REQ_UI_OVERFLOW \
	((int16_t) 195)

#define FUNC_ERROR__TEMPERATURE_LIMIT_REACHED \
	((int16_t) 196)

#define FUNC_ERROR__PIPE_ERROR \
	((int16_t) 197)

#define FUNC_WARNING__OUTPUT_RATE_CLIPPED \
	((int16_t) 70)

#define FUNC_ERROR__DYNXTALKMON_ZONES_SELECT_FAIL \
	((int16_t) 199)

#define FUNC_ERROR__CAL_ERROR \
	((int16_t) 200)

#define FUNC_ERROR__PIPE_WARNING  \
	((int16_t) 73)

#define FUNC_ERROR__CAL_WARNING  \
	((int16_t) 74)

#define FUNC_WARNING__INCOMPATABLE_BLANKING_INTEGRATION_MODES \
	((int16_t) 75)

#define FUNC_WARNING__INTEGRATION_TIME_MIN_CLIPPED \
	((int16_t) 76)

#define FUNC_WARNING__INTEGRATION_TIME_MAX_CLIPPED \
	((int16_t) 77)

#define FUNC_WARNING__CP_COLLAPSE  \
	((int16_t) 78)

#define FUNC_ERROR__HW_NVM_COPY_LASER_SAFETY_CLIP  \
	((int16_t) 230)

#define FUNC_ERROR__OVER_VOLT_PROT_AVDD \
	((int16_t) 233)

#define FUNC_ERROR__OVER_VOLT_PROT_ANODE \
	((int16_t) 234)

#define FUNC_ERROR__OVER_VOLT_PROT_DVDDHP \
	((int16_t) 235)

#define FUNC_ERROR__VCSEL_MONITOR_FAIL \
	((int16_t) 236)

#define FUNC_ERROR__VCSEL_CP_OVER_CURRENT \
	((int16_t) 237)

#define FUNC_ERROR__PLL_LOCK_LOST \
	((int16_t) 238)

#define FUNC_ERROR__BAND_GAP_FAIL \
	((int16_t) 239)

#define FUNC_ERROR__CP_COLLAPSE \
	((int16_t) 240)

#define FUNC_ERROR__DVIM_DIV_BY_ZERO_TRAP \
	((int16_t) 241)

#define FUNC_ERROR__OVERFLOW_TRAP \
	((int16_t) 242)

#define FUNC_ERROR__TRAP_PROTECT \
	((int16_t) 243)

#define FUNC_ERROR__TRAP_OPCODE \
	((int16_t) 244)

#define FUNC_ERROR__TRAP_GPRSIZE \
	((int16_t) 245)

#define FUNC_ERROR__TRAP_PMISALIGN \
	((int16_t) 246)

#define FUNC_ERROR__TRAP_POUTOFMEM \
	((int16_t) 247)

#define FUNC_ERROR__TRAP_PEXECUTE \
	((int16_t) 248)

#define FUNC_ERROR__TRAP_DMISALIGN \
	((int16_t) 249)

#define FUNC_ERROR__TRAP_DOUTOFMEM \
	((int16_t) 250)

#define FUNC_ERROR__TRAP_DREAD \
	((int16_t) 251)

#define FUNC_ERROR__TRAP_DWRITE \
	((int16_t) 252)

#define FUNC_ERROR__TRAP_DVOLATILE \
	((int16_t) 253)

#define FUNC_ERROR__TRAP_PSYSERR \
	((int16_t) 254)

#define FUNC_EROR__PARAM_INVALID \
	((int16_t) 65533)

#define FUNC_ERROR__UNDEFINED \
	((int16_t) 65534)

#define FUNC_ERROR__DIVIDE_BY_ZERO \
	((int16_t) 65535)

#ifdef __cplusplus
}
#endif

#endif
