
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

#ifndef __DCI_UI_UNION_STRUCTS_H__
#define __DCI_UI_UNION_STRUCTS_H__

#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

union dci_union__go2_status_0_go1_u {
	uint8_t bytes;
	struct {

		uint8_t mcu__boot_complete_go1 : 1;

		uint8_t mcu__analog_checks_ok_go1 : 1;

		uint8_t mcu__threshold_triggered_g01 : 1;
		uint8_t mcu__error_flag_go1 : 1;
		uint8_t mcu__ui_range_data_present_go1 : 1;
		uint8_t mcu__ui_new_range_data_avail_go1 : 1;
		uint8_t mcu__ui_update_blocked_go1 : 1;

		uint8_t mcu__hw_trap_flag_go1 : 1;
	};
};

union dci_union__go2_status_1_go1_u {
	uint8_t bytes;
	struct {

		uint8_t mcu__avdd_reg_ok_go1 : 1;

		uint8_t mcu__pll_lock_ok_go1 : 1;

		uint8_t mcu__ls_watchdog_pass_go1 : 1;

		uint8_t mcu__warning_flag_go1 : 1;

		uint8_t mcu__cp_collapse_flag_go1 : 1;
		uint8_t mcu__spare0 : 1;
		uint8_t mcu__initial_ram_boot_complete : 1;
		uint8_t mcu__spare1 : 1;
	};
};

union dci_union__block_header_u {
	uint32_t bytes;
	struct {

		uint32_t p_type : 4;

		uint32_t b_size__p_rep : 12;

		uint32_t b_idx__p_idx : 16;
	};
};

union dci_union__cmd_footer_u {
	uint32_t bytes;
	struct {

		uint32_t cmd__ip_data_size : 16;

		uint32_t cmd__ip_command_id : 8;

		uint32_t cmd__ip_transaction_id : 8;
	};
};

union dci_union__cmd_info_u {
	uint32_t bytes;
	struct {

		uint32_t cmd__rtn_data_size : 16;

		uint32_t cmd__rtn_command_status : 8;

		uint32_t cmd__rtn_transaction_id : 8;
	};
};

union dci_union__rng_data__header_u {
	uint32_t bytes;
	struct {
		uint32_t rng_data__header__id : 16;
		uint32_t rng_data__header__reserved_0 : 8;
		uint32_t rng_data__header__reserved_1 : 8;
	};
};

union dci_union__rng_data__footer_u {
	uint32_t bytes;
	struct {
		uint32_t rng_data__footer__id : 16;
		uint32_t rng_data__footer__reserved_0 : 8;
		uint32_t rng_data__footer__reserved_1 : 8;
	};
};

#ifdef __cplusplus
}
#endif

#endif
