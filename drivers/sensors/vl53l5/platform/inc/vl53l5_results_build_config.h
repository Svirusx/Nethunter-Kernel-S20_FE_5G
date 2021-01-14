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

/**
 * @file vl53l5_results_build_config.h
 *
 * @brief User build onfiguration options for the results struct and decoder.
 *
 */

#ifndef _VL53L5_RESULTS_BUILD_CONFIG_H_
#define _VL53L5_RESULTS_BUILD_CONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * The configuration options for the result struct are specified here.
 * Each array member of the results structs can be individually configured.
 * Also, the values of VL53L5_MAX_TARGETS and VL53L5_MAX_ZONES can be set as per
 * the maximum required configuration.
 *
 */

#define VL53L5_MAX_TARGETS 128
#define VL53L5_MAX_ZONES 64
#define VL53L5_META_DATA_ON
#define VL53L5_COMMON_DATA_ON
#define VL53L5_RANGE_SIGMA_MM_ON
#define VL53L5_MEDIAN_RANGE_MM_ON
#define VL53L5_TARGET_STATUS_ON
#define VL53L5_PEAK_RATE_KCPS_PER_SPAD_ON
#define VL53L5_NO_OF_TARGETS_ON
#define VL53L5_AMB_RATE_KCPS_PER_SPAD_ON
#define VL53L5_AMB_DMAX_MM_ON
#define VL53L5_DYN_XTALK_OP_PERSISTENT_DATA_ON
#define VL53L5_ZONE_ID_ON

#ifdef __cplusplus
}
#endif

#endif
