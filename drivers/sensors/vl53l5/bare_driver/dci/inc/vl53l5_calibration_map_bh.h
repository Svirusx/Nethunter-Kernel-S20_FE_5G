
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

#ifndef __VL53L5_CALIBRATION_MAP_BH_H__
#define __VL53L5_CALIBRATION_MAP_BH_H__

#ifdef __cplusplus
extern "C" {
#endif

#define VL53L5_CAL_REF_SPAD_INFO_BH \
	((uint32_t) 0xa3580080U)

#define VL53L5_POFFSET_GRID_META_BH \
	((uint32_t) 0xa37400c0U)
#define VL53L5_POFFSET_PHASE_STATS_BH \
	((uint32_t) 0xa3800140U)
#define VL53L5_POFFSET_TEMPERATURE_STATS_BH \
	((uint32_t) 0xa3940040U)
#define VL53L5_POFFSET_GRID_RATE_CAL__GRID_DATA__RATE_KCPS_PER_SPAD_BH \
	((uint32_t) 0xa3980404U)
#define VL53L5_POFFSET_GRID_SPADS_CAL__GRID_DATA_EFFECTIVE_SPAD_COUNT_BH \
	((uint32_t) 0xa4980402U)
#define VL53L5_POFFSET_GRID_OFFSET_CAL__GRID_DATA__RANGE_MM_BH \
	((uint32_t) 0xa5180402U)
#define VL53L5_POFFSET_ERROR_STATUS_BH \
	((uint32_t) 0xa5980100U)
#define VL53L5_POFFSET_WARNING_STATUS_BH \
	((uint32_t) 0xa5a80100U)

#define VL53L5_PXTALK_GRID_META_BH \
	((uint32_t) 0xa7fc00c0U)
#define VL53L5_PXTALK_PHASE_STATS_BH \
	((uint32_t) 0xa8080140U)
#define VL53L5_PXTALK_TEMPERATURE_STATS_BH \
	((uint32_t) 0xa81c0040U)
#define VL53L5_PXTALK_GRID_RATE_CAL__GRID_DATA__RATE_KCPS_PER_SPAD_BH \
	((uint32_t) 0xa8200404U)
#define VL53L5_PXTALK_GRID_SPADS_CAL__GRID_DATA_EFFECTIVE_SPAD_COUNT_BH \
	((uint32_t) 0xa9200402U)
#define VL53L5_PXTALK_ERROR_STATUS_BH \
	((uint32_t) 0xa9a00100U)
#define VL53L5_PXTALK_WARNING_STATUS_BH \
	((uint32_t) 0xa9b00100U)

#define VL53L5_PXTALK_SHAPE_META_BH \
	((uint32_t) 0xad4800c0U)
#define VL53L5_PXTALK_SHAPE_DATA_CAL__XTALK_SHAPE__BIN_DATA_BH \
	((uint32_t) 0xad540902U)
#define VL53L5_PXTALK_MON_META_BH \
	((uint32_t) 0xae740040U)
#define VL53L5_PXTALK_MON_ZONES_CAL__XMON__ZONE__X_OFF_BH \
	((uint32_t) 0xae780081U)
#define VL53L5_PXTALK_MON_ZONES_CAL__XMON__ZONE__Y_OFF_BH \
	((uint32_t) 0xae800081U)
#define VL53L5_PXTALK_MON_ZONES_CAL__XMON__ZONE__WIDTH_BH \
	((uint32_t) 0xae880081U)
#define VL53L5_PXTALK_MON_ZONES_CAL__XMON__ZONE__HEIGHT_BH \
	((uint32_t) 0xae900081U)
#define VL53L5_PXTALK_MON_DATA_CAL__XMON__ZONE__RATE_KCPS_SPAD_BH \
	((uint32_t) 0xae980084U)
#define VL53L5_PXTALK_MON_DATA_CAL__XMON__ZONE__AVG_COUNT_BH \
	((uint32_t) 0xaeb80082U)

#ifdef __cplusplus
}
#endif

#endif
