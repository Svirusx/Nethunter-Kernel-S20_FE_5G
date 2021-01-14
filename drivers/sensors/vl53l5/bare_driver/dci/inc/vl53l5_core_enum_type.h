
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

#ifndef __VL53L5_CORE_ENUM_TYPE_H__
#define __VL53L5_CORE_ENUM_TYPE_H__

#include "vl53l5_dci_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VL53L5_MAP_VERSION_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_FW_VERSION_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_CFG_INFO_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_FMT_TRACEABILITY_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_EWS_TRACEABILITY_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_UI_RNG_DATA_ADDR_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_SILICON_TEMPERATURE_DATA_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_ZONE_CFG_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_RANGING_RATE_CFG_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_INTEGRATION_TIME_CFG_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_FACTORY_CAL_CFG_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_OUTPUT_DATA_CFG_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_OUTPUT_BH_CFG_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_OBE_OP_BH_ENABLES_TYPE \
	((enum block_format_type) 0x4)
#define VL53L5_OBL_OP_BH_LIST_TYPE \
	((enum block_format_type) 0x4)
#define VL53L5_INTERRUPT_CFG_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_NVM_LASER_SAFETY_NVM2_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_PHE_PATCH_HOOK_ENABLES_TYPE \
	((enum block_format_type) 0x4)
#define VL53L5_COMMON_GRP_STATUS_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_IC_GRP_CHECKER_CFG_TYPE \
	((enum block_format_type) 0x0)
#define VL53L5_IGVTS_VAILD_TARGET_STATUS_TYPE \
	((enum block_format_type) 0x1)

#ifdef __cplusplus
}
#endif

#endif
