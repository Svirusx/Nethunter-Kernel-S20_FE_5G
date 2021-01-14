
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

#ifndef __CAL_LUTS_H__
#define __CAL_LUTS_H__

#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CAL_ERROR__NONE \
	((int32_t) 0)

#define CAL_ERROR__NO_SAMPLE_FAIL \
	((int32_t) 1)

#define CAL_ERROR__RATE_SIGMA_LIMIT_FAIL \
	((int32_t) 2)

#define CAL_ERROR__RANGE_SIGMA_LIMIT_FAIL \
	((int32_t) 3)

#define CAL_ERROR__MISSING_SAMPLES \
	((int32_t) 4)

#define CAL_ERROR__INVALID_REPLACEMENT_MODE \
	((int32_t) 5)

#define CAL_ERROR__MAX_FAILING_ZONES_LIMIT_FAIL \
	((int32_t) 6)

#define CAL_ERROR__NO_VALID_ZONES \
	((int32_t) 7)

#define CAL_ERROR__8x8_TO_4x4__CONVERSION_FAIL \
	((int32_t) 8)

#define CAL_ERROR__TARGET_TOO_CLOSE \
	((int32_t) 9)

#define CAL_WARNING__NONE \
	((int32_t) 128)

#define CAL_WARNING__NO_SAMPLE_FAIL \
	((int32_t) 129)

#define CAL_WARNING__RATE_SIGMA_LIMIT_FAIL \
	((int32_t) 130)

#define CAL_WARNING__RANGE_SIGMA_LIMIT_FAIL \
	((int32_t) 131)

#define CAL_WARNING__MISSING_SAMPLES \
	((int32_t) 132)

#define CAL_WARNING__ZONE_REPLACED_BY_MIN_XTALK \
	((int32_t) 133)

#define CAL_WARNING__ZONE_REPLACED_BY_FIXED_VAL_XTALK \
	((int32_t) 134)

#define CAL_WARNING__8x8_TO_4x4_CONVERSION__INPUT_RATE_CLIPPED \
	((int32_t) 135)

#define CAL_WARNING__8x8_TO_4x4_CONVERSION__ZERO_RATES_DETECTED \
	((int32_t) 136)

#define CAL_WARNING__8x8_TO_4x4_CONVERSION__SPAD_COUNTS_SUM_CLIPPED \
	((int32_t) 137)

#define CAL_WARNING__TARGET_TOO_CLOSE \
	((int32_t) 138)

#define CAL_STAGE__INVALID \
	((int32_t) 0)

#define CAL_STAGE__RANGE_OFFSET \
	((int32_t) 1)

#define CAL_STAGE__PEAK_RATE \
	((int32_t) 2)

#define CAL_STAGE__XTALK_RATE \
	((int32_t) 3)

#define CAL_STAGE__XTALK_SHAPE \
	((int32_t) 4)

#define CAL_STAGE__RANGE_OFFSET_8x8_TO_4x4_CONVERSION \
	((int32_t) 5)

#define CAL_STAGE__PEAK_RATE_8x8_TO_4x4_CONVERSION \
	((int32_t) 6)

#define CAL_STAGE__XTALK_RATE_8x8_TO_4x4_CONVERSION \
	((int32_t) 7)

#ifdef __cplusplus
}
#endif

#endif
