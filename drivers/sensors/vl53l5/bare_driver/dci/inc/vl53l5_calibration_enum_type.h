
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

#ifndef __VL53L5_CALIBRATION_ENUM_TYPE_H__
#define __VL53L5_CALIBRATION_ENUM_TYPE_H__

#include "vl53l5_dci_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VL53L5_CAL_GRP_REF_SPAD_INFO_TYPE \
	((enum block_format_type) 0x0)

#define VL53L5_CAL_GRP_GRID_META_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_CAL_GRP_PHASE_STATS_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_CAL_GRP_TEMPERATURE_STATS_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_CGGDRKPS_CAL_GRID_DATA_RATE_KCPS_PER_SPAD_TYPE \
	((enum block_format_type) 0x4)
#define VL53L5_CGGDESC_CAL_GRID_DATA_EFFECTIVE_SPAD_COUNT_TYPE \
	((enum block_format_type) 0x2)
#define VL53L5_CGGDRM_CAL_GRID_DATA_RANGE_MM_TYPE \
	((enum block_format_type) 0x2)
#define VL53L5_CAL_GRP_STATUS_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_CAL_GRP_XTALK_SHAPE_META_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_CGXSD_CAL_XTALK_SHAPE_BIN_DATA_TYPE \
	((enum block_format_type) 0x2)
#define VL53L5_CAL_GRP_XTALK_MON_META_DATA_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_CGXMZ_CAL_XMON_ZONE_X_OFF_TYPE \
	((enum block_format_type) 0x1)
#define VL53L5_CGXMZ_CAL_XMON_ZONE_Y_OFF_TYPE \
	((enum block_format_type) 0x1)
#define VL53L5_CGXMZ_CAL_XMON_ZONE_WIDTH_TYPE \
	((enum block_format_type) 0x1)
#define VL53L5_CGXMZ_CAL_XMON_ZONE_HEIGHT_TYPE \
	((enum block_format_type) 0x1)
#define VL53L5_CGXMD_CAL_XMON_ZONE_RATE_KCPS_SPAD_TYPE \
	((enum block_format_type) 0x4)
#define VL53L5_CGXMD_CAL_XMON_ZONE_AVG_COUNT_TYPE \
	((enum block_format_type) 0x2)

#ifdef __cplusplus
}
#endif

#endif
