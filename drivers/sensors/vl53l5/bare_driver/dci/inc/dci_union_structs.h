
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

#ifndef __DCI_UNION_STRUCTS_H__
#define __DCI_UNION_STRUCTS_H__

#include "dci_defs.h"
#include "dci_version_defs.h"
#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

union dci_union__config_presets_u {
	uint32_t bytes;
	struct {

		uint32_t config__preset__0 : 4;

		uint32_t config__preset__1 : 4;

		uint32_t config__preset__2 : 4;

		uint32_t config__preset__3 : 4;

		uint32_t config__preset__4 : 4;

		uint32_t config__preset__5 : 4;

		uint32_t config__preset__6 : 4;

		uint32_t config__preset__7 : 4;
	};
};

union dci_union__config_parms_u {
	uint32_t bytes;
	struct {

		uint32_t config__parms__0 : 8;

		uint32_t config__parms__1 : 4;

		uint32_t config__parms__2 : 4;

		uint32_t config__parms__3 : 4;

		uint32_t config__parms__4 : 4;

		uint32_t config__parms__5 : 2;

		uint32_t config__parms__6 : 2;

		uint32_t config__parms__7 : 2;

		uint32_t config__parms__8 : 1;

		uint32_t config__parms__9 : 1;
	};
};

union dci_union__interrupt__config_u {
	uint8_t bytes;
	struct {

		uint8_t interrupt__config__pos_edge : 1;
		uint8_t interrupt__config__pad_0 : 1;
		uint8_t interrupt__config__pad_1 : 1;
		uint8_t interrupt__config__pad_2 : 1;
		uint8_t interrupt__config__pad_3 : 1;
		uint8_t interrupt__config__pad_4 : 1;
		uint8_t interrupt__config__pad_5 : 1;
		uint8_t interrupt__config__pad_6 : 1;
	};
};

union dci_union__vhv__config_u {
	uint8_t bytes;
	struct {

		uint8_t stage_en : 1;

		uint8_t manual_override : 1;

		uint8_t init_mode : 1;

		uint8_t vcsel_enable : 1;

		uint8_t debug_en : 1;

		uint8_t search_source : 1;

		uint8_t search_reset : 1;

		uint8_t search_control : 1;
	};
};

union dci_union__ref_msr__config_u {
	uint8_t bytes;
	struct {

		uint8_t stage_en : 1;

		uint8_t laser_safety_check_en : 1;
		uint8_t ref_msr__config__pad_0 : 1;
		uint8_t ref_msr__config__pad_1 : 1;
		uint8_t ref_msr__config__pad_2 : 1;
		uint8_t ref_msr__config__pad_3 : 1;
		uint8_t ref_msr__config__pad_4 : 1;
		uint8_t ref_msr__config__pad_5 : 1;
	};
};

union dci_union__vhv_ref_scheduling_ctrl_u {
	uint8_t bytes;
	struct {

		uint8_t vhv_repeat_en : 1;

		uint8_t ref_msr_repeat_en : 1;
		uint8_t vhv_ref_scheduling_pad_0 : 1;
		uint8_t vhv_ref_scheduling_pad_1 : 1;
		uint8_t vhv_ref_scheduling_pad_2 : 1;
		uint8_t vhv_ref_scheduling_pad_3 : 1;
		uint8_t vhv_ref_scheduling_pad_4 : 1;
		uint8_t vhv_ref_scheduling_pad_5 : 1;
	};
};

union dci_union__cal__ref_spad__debug_cfg_u {
	uint8_t bytes;
	struct {

		uint8_t debug_en : 1;
		uint8_t cal__ref_spad__debug_cfg__pad_0 : 1;
		uint8_t cal__ref_spad__debug_cfg__pad_1 : 1;
		uint8_t cal__ref_spad__debug_cfg__pad_2 : 1;
		uint8_t cal__ref_spad__debug_cfg__pad_3 : 1;
		uint8_t cal__ref_spad__debug_cfg__pad_4 : 1;
		uint8_t cal__ref_spad__debug_cfg__pad_5 : 1;
		uint8_t cal__ref_spad__debug_cfg__pad_6 : 1;
	};
};

union dci_union_cp_collapse_ctrl_u {
	uint8_t bytes;
	struct {
		uint8_t cp_collapse_ctrl : 1;
		uint8_t cp_collapse_ctrl_pad_0 : 1;
		uint8_t cp_collapse_ctrl_pad_1 : 1;
		uint8_t cp_collapse_ctrl_pad_2 : 1;
		uint8_t cp_collapse_ctrl_pad_3 : 1;
		uint8_t cp_collapse_ctrl_pad_4 : 1;
		uint8_t cp_collapse_ctrl_pad_5 : 1;
		uint8_t cp_collapse_ctrl_pad_6 : 1;
	};
};

union dci_union__st_fgc_0_u {
	uint32_t bytes;
	struct {
		uint32_t fgc_4__6_3 : 4;
		uint32_t fgc_3 : 7;
		uint32_t fgc_2 : 7;
		uint32_t fgc_1 : 7;
		uint32_t fgc_0 : 7;
	};
};

union dci_union__st_fgc_1_u {
	uint32_t bytes;
	struct {
		uint32_t fgc_9__6 : 1;
		uint32_t fgc_8 : 7;
		uint32_t fgc_7 : 7;
		uint32_t fgc_6 : 7;
		uint32_t fgc_5 : 7;
		uint32_t fgc_4__2_0 : 3;
	};
};

union dci_union__st_fgc_2_u {
	uint32_t bytes;
	struct {
		uint32_t fgc_13__6_2 : 5;
		uint32_t fgc_12 : 7;
		uint32_t fgc_11 : 7;
		uint32_t fgc_10 : 7;
		uint32_t fgc_9__5_0 : 6;
	};
};

union dci_union__st_fgc_3_u {
	uint32_t bytes;
	struct {
		uint32_t word32_250__pad_0 : 2;
		uint32_t fgc_17 : 7;
		uint32_t fgc_16 : 7;
		uint32_t fgc_15 : 7;
		uint32_t fgc_14 : 7;
		uint32_t fgc_13__1_0 : 2;
	};
};

union dci_union__identification_0_u {
	uint32_t bytes;
	struct {
		uint32_t module_date_phase : 3;
		uint32_t day : 5;
		uint32_t month : 4;
		uint32_t year : 4;
		uint32_t map_minor : 5;
		uint32_t map_major : 3;
		uint32_t test_prog_fmt_minor : 5;
		uint32_t test_mrog_fmt_major : 3;
	};
};

union dci_union__identification_1_u {
	uint32_t bytes;
	struct {
		uint32_t code_site_id_fmt : 8;
		uint32_t code_tester_id_fmt : 8;
		uint32_t time : 16;
	};
};

union dci_union__test_traceability_u {
	uint32_t bytes;
	struct {
		uint32_t word32_253__pad_0 : 8;
		uint32_t tester_id_ews : 8;
		uint32_t probe_card_minor : 4;
		uint32_t probe_card_major : 4;
		uint32_t test_prog_ews_minor : 5;
		uint32_t test_prog_ews_major : 3;
	};
};

union dci_union__die_traceability_0_u {
	uint32_t bytes;
	struct {
		uint32_t lot1_5_4 : 2;
		uint32_t lot2 : 6;
		uint32_t lot3 : 6;
		uint32_t lot4 : 6;
		uint32_t lot5 : 6;
		uint32_t lot6 : 6;
	};
};

union dci_union__die_traceability_1_u {
	uint32_t bytes;
	struct {
		uint32_t ycoord : 8;
		uint32_t xcoord : 8;
		uint32_t wafer : 5;
		uint32_t word32_255__pad_0 : 1;
		uint32_t lot0 : 6;
		uint32_t lot1_3_0 : 4;
	};
};

#ifdef __cplusplus
}
#endif

#endif
