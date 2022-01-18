
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

#ifndef __VL53L5_CALIBRATION_MAP_H__
#define __VL53L5_CALIBRATION_MAP_H__

#include "dci_defs.h"
#include "dci_structs.h"
#include "dci_union_structs.h"
#include "dci_ui_structs.h"
#include "cal_defs.h"
#include "cal_structs.h"
#include "padding_structs.h"
#include "vl53l5_types.h"
#include "ic_checkers_structs.h"
#include "ic_checkers_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct vl53l5_calibration_dev_t {
	struct cal_grp__ref_spad_info_t cal_ref_spad_info;

	struct cal_grp__grid_meta_t poffset_grid_meta;

	struct cal_grp__phase_stats_t poffset_phase_stats;

	struct cal_grp__temperature_stats_t poffset_temperature_stats;

	struct cal_grp__grid_data__rate_kcps_per_spad_t poffset_grid_rate;

	struct cal_grp__grid_data__effective_spad_count_t poffset_grid_spads;

	struct cal_grp__grid_data__range_mm_t poffset_grid_offset;

	struct cal_grp__status_t poffset_error_status;

	struct cal_grp__status_t poffset_warning_status;

	struct cal_grp__grid_meta_t pxtalk_grid_meta;

	struct cal_grp__phase_stats_t pxtalk_phase_stats;

	struct cal_grp__temperature_stats_t pxtalk_temperature_stats;

	struct cal_grp__grid_data__rate_kcps_per_spad_t pxtalk_grid_rate;

	struct cal_grp__grid_data__effective_spad_count_t pxtalk_grid_spads;

	struct cal_grp__status_t pxtalk_error_status;

	struct cal_grp__status_t pxtalk_warning_status;

	struct cal_grp__xtalk_shape_meta_t pxtalk_shape_meta;

	struct cal_grp__xtalk_shape_data_t pxtalk_shape_data;

	struct cal_grp__xtalk_mon__meta_data_t pxtalk_mon_meta;

	struct cal_grp__xtalk_mon__zones_t pxtalk_mon_zones;

	struct cal_grp__xtalk_mon__data_t pxtalk_mon_data;

};

#ifdef __cplusplus
}
#endif

#endif
