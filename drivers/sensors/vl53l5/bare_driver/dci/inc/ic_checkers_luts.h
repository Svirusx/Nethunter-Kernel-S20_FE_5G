
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

#ifndef __IC_CHECKERS_LUTS_H__
#define __IC_CHECKERS_LUTS_H__

#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CHECKER_TYPE__IN_WINDOW \
	((uint8_t) 0U)
#define CHECKER_TYPE__OUT_OF_WINDOW \
	((uint8_t) 1U)
#define CHECKER_TYPE__LESS_THAN_EQUAL_MIN_CHECKER \
	((uint8_t) 2U)
#define CHECKER_TYPE__GREATER_THAN_MAX_CHECKER \
	((uint8_t) 3U)
#define CHECKER_TYPE__EQUAL_MIN_CHECKER \
	((uint8_t) 4U)
#define CHECKER_TYPE__NOT_EQUAL_MIN_CHECKER  \
	((uint8_t) 5U)

#define CHECKER_PARAM_TYPE__INVALID_CHECKER \
	((uint8_t) 0U)

#define CHECKER_PARAM_TYPE__MEDIAN_RANGE_MM \
	((uint8_t) 1U)

#define CHECKER_PARAM_TYPE__PEAK_RATE_KCPS_PER_SPAD \
	((uint8_t) 2U)

#define CHECKER_PARAM_TYPE__RATE_SIGMA_KCPS_PER_SPAD \
	((uint8_t) 3U)

#define CHECKER_PARAM_TYPE__RANGE_SIGMA_MM \
	((uint8_t) 4U)

#define CHECKER_PARAM_TYPE__TARGET_REFLECTANCE_EST \
	((uint8_t) 5U)

#define CHECKER_PARAM_TYPE__MIN_RANGE \
	((uint8_t) 6U)

#define CHECKER_PARAM_TYPE__MAX_RANGE \
	((uint8_t) 7U)

#define CHECKER_PARAM_TYPE__AMB_RATE_KCPS_PER_SPAD \
	((uint8_t) 8U)

#define CHECKER_PARAM_TYPE__NUM_OF_TARGETS \
	((uint8_t) 9U)

#define CHECKER_PARAM_TYPE__DMAX_MM \
	((uint8_t) 10U)

#define CHECKER_PARAM_TYPE__EWOKMZ_TEMP \
	((uint8_t) 11U)

#define CHECKER_PARAM_TYPE__EWOKMZ_TARGET_STATUS \
	((uint8_t) 12U)

#define CHECKER_PARAM_TYPE__EFFECTIVE_SPAD_COUNT \
	((uint8_t) 13U)

#define CHECKER_PARAM_TYPE__DELTA_START_END_TEMP \
	((uint8_t) 14U)

#define CHECKER_PARAM_TYPE__DELTA_START_END_PHASE \
	((uint8_t) 15U)

#ifdef __cplusplus
}
#endif

#endif
