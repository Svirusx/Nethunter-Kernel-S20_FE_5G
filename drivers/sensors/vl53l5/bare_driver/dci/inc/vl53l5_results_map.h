
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

#ifndef __VL53L5_RESULTS_MAP_H__
#define __VL53L5_RESULTS_MAP_H__

#include "dci_defs.h"
#include "dci_structs.h"
#include "dci_union_structs.h"
#include "dci_ui_structs.h"
#include "common_datatype_structs.h"
#include "common_datatype_defs.h"
#include "packing_structs.h"
#include "vl53l5_types.h"
#include "dyn_xtalk_structs.h"
#include "dyn_xtalk_defs.h"
#include "ic_checkers_structs.h"
#include "ic_checkers_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VL53L5_CGHBD_HIST_BIN_EVENTS_PACKED_ARRAY_SZ \
	((uint16_t) 6912)

struct vl53l5_range_results_t {
#ifdef VL53L5_META_DATA_ON

	struct vl53l5_range_meta_data_t meta_data;
#endif

#ifdef VL53L5_COMMON_DATA_ON

	struct vl53l5_range_common_data_t common_data;
#endif

#ifdef VL53L5_RNG_TIMING_DATA_ON

	struct dci_grp__rng_timing_data_t rng_timing_data;
#endif

#ifdef VL53L5_PER_ZONE_RESULTS_ON

	struct vl53l5_range_per_zone_results_t per_zone_results;
#endif

#ifdef VL53L5_PER_TGT_RESULTS_ON

	struct vl53l5_range_per_tgt_results_t per_tgt_results;
#endif

#ifdef VL53L5_REF_TIMING_DATA_ON

	struct dci_grp__rng_timing_data_t ref_timing_data;
#endif

#ifdef VL53L5_REF_CHANNEL_DATA_ON

	struct dci_grp__ref_channel_data_t ref_channel_data;
#endif

#ifdef VL53L5_REF_TARGET_DATA_ON

	struct dci_grp__ref_target_data_t ref_target_data;
#endif

#ifdef VL53L5_ZONE_THRESH_STATUS_ON
	struct dci_grp__zone_thresh_status_array_t zone_thresh_status;
#endif

#ifdef VL53L5_DYN_XTALK_OP_PERSISTENT_DATA_ON

	struct dci_grp__dyn_xtalk_persistent_data_t
		dyn_xtalk_op_persistent_data;
#endif

};

#ifdef __cplusplus
}
#endif

#endif
