
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

#ifndef _VL53L5_DEVICE_H_
#define _VL53L5_DEVICE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "vl53l5_globals.h"
#include "vl53l5_types.h"
#include "dci_ui_structs.h"
#include "vl53l5_power_states.h"
#include "vl53l5_core_map.h"
#include "vl53l5_results_map.h"
#include "vl53l5_calibration_map.h"

struct vl53l5_dev_handle_internal_t {

	uint8_t                               *p_fw_buff;

	uint32_t                              fw_buff_count;

	uint8_t                               *p_comms_buff;

	uint32_t                              comms_buff_max_count;

	uint32_t                              comms_buff_count;

	uint32_t                               latest_trans_id;

	bool                                  fw_readback_on;

	struct dci_ui__cmd_info_t             cmd_status;

	struct vl53l5_core_dev_t              core_dev;

	struct vl53l5_range_results_t         *presults_dev;

	struct vl53l5_calibration_dev_t       *pcalibration_dev;

	struct dci_ui__dev_info_t             ui_dev_info;

	bool                                  mcu_warnings_on;

	enum vl53l5_power_states              power_state;

	struct {

		bool map_version_match;
	} version_match;

	struct {

		uint16_t dev_info_start_addr;

		uint16_t data_start_offset_bytes;

		uint16_t data_size_bytes;
	} range_data_addr;

	bool                                  device_booted;
};

#ifdef __cplusplus
}
#endif

#endif
