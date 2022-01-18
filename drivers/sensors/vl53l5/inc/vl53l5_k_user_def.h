
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

#ifndef _VL53L5_K_USER_DEF_H_
#define _VL53L5_K_USER_DEF_H_

#include "vl53l5_globals.h"
#include "vl53l5_calibration_map_bh.h"
#include "vl53l5_core_map_bh.h"

#define VL53L5_K_CONFIG_DATA_MAX_SIZE VL53L5_COMMS_BUFFER_SIZE_BYTES

#define VL53L5_K_FIRMWARE_MAX_SIZE 100000

#define VL53L5_K_MAX_NUM_GET_HEADERS 32
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
#define VL53L5_K_DRIVER_NAME "vl53l5"
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
#define VL53L5_K_DRIVER_NAME "range_sensor"
#endif
#define VL53L5_K_COMMS_TYPE_I2C 0

#define VL53L5_K_COMMS_TYPE_SPI 1

#define VL53L5_K_SHAPE_FILE_SIZE 316U

#define VL53L5_K_P2P_FILE_SIZE 1224U

#define VL53L5_K_P2P_BH_SZ 24U

#define VL53L5_K_P2P_CAL_BLOCK_HEADERS { \
	VL53L5_MAP_VERSION_BH, \
	VL53L5_CAL_REF_SPAD_INFO_BH,\
	VL53L5_POFFSET_GRID_META_BH, \
	VL53L5_POFFSET_PHASE_STATS_BH, \
	VL53L5_POFFSET_TEMPERATURE_STATS_BH, \
	VL53L5_POFFSET_GRID_RATE_CAL__GRID_DATA__RATE_KCPS_PER_SPAD_BH, \
	VL53L5_POFFSET_GRID_SPADS_CAL__GRID_DATA_EFFECTIVE_SPAD_COUNT_BH, \
	VL53L5_POFFSET_GRID_OFFSET_CAL__GRID_DATA__RANGE_MM_BH, \
	VL53L5_POFFSET_ERROR_STATUS_BH, \
	VL53L5_POFFSET_WARNING_STATUS_BH, \
	VL53L5_PXTALK_GRID_META_BH, \
	VL53L5_PXTALK_PHASE_STATS_BH, \
	VL53L5_PXTALK_TEMPERATURE_STATS_BH, \
	VL53L5_PXTALK_GRID_RATE_CAL__GRID_DATA__RATE_KCPS_PER_SPAD_BH, \
	VL53L5_PXTALK_GRID_SPADS_CAL__GRID_DATA_EFFECTIVE_SPAD_COUNT_BH, \
	VL53L5_PXTALK_ERROR_STATUS_BH, \
	VL53L5_PXTALK_WARNING_STATUS_BH, \
	VL53L5_PXTALK_MON_META_BH, \
	VL53L5_PXTALK_MON_ZONES_CAL__XMON__ZONE__X_OFF_BH, \
	VL53L5_PXTALK_MON_ZONES_CAL__XMON__ZONE__Y_OFF_BH, \
	VL53L5_PXTALK_MON_ZONES_CAL__XMON__ZONE__WIDTH_BH, \
	VL53L5_PXTALK_MON_ZONES_CAL__XMON__ZONE__HEIGHT_BH, \
	VL53L5_PXTALK_MON_DATA_CAL__XMON__ZONE__RATE_KCPS_SPAD_BH, \
	VL53L5_PXTALK_MON_DATA_CAL__XMON__ZONE__AVG_COUNT_BH}

#define VL53L5_K_SHAPE_CAL_BLOCK_HEADERS_SZ  3

#define VL53L5_K_SHAPE_CAL_BLOCK_HEADERS { \
	VL53L5_MAP_VERSION_BH, \
	VL53L5_PXTALK_SHAPE_META_BH, \
	VL53L5_PXTALK_SHAPE_DATA_CAL__XTALK_SHAPE__BIN_DATA_BH}

#define VL53L5_K_DEVICE_INFO_SZ 43U
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
#define VL53L5_CAL_P2P_FILENAME \
			"/home/pi/Documents/vl53l5_cal_p2p.bin"

#define VL53L5_CAL_SHAPE_FILENAME \
			"/home/pi/Documents/vl53l5_cal_shape.bin"

#define VL53L5_GENERIC_SHAPE_FILENAME \
			"/home/pi/Documents/vl53l5_generic_xtalk_shape.bin"

#define VL53L5_TCDM_FILENAME \
			"/home/pi/Documents/vl53l5_tcdm.bin"
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
#define VL53L5_CAL_P2P_FILENAME \
			"/data/vl53l5_cal_p2p.bin"

#define VL53L5_CAL_SHAPE_FILENAME \
			"/data/vl53l5_cal_shape.bin"

#define VL53L5_GENERIC_SHAPE_FILENAME \
			"/efs/FactoryApp/range_sensor/generic_xtalk_shape.bin"

#define VL53L5_CAL_DATA_FILENAME \
			"/efs/FactoryApp/range_sensor/cal_data.bin"

#define VL53L5_TCDM_FILENAME \
			"/data/log/vl53l5_tcdm.bin"

#endif
#endif
