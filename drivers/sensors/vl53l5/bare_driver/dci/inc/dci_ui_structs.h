
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

#ifndef __DCI_UI_STRUCTS_H__
#define __DCI_UI_STRUCTS_H__

#include "dci_defs.h"
#include "dci_luts.h"
#include "dci_ui_union_structs.h"
#include "packing_structs.h"
#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct dci_ui__cmd_footer_t {
	union dci_union__cmd_footer_u cmd_footer;
};

struct dci_ui__cmd_info_t {
	union dci_union__cmd_info_u cmd_info;
};

struct dci_ui__dev_info_t {

	union dci_union__go2_status_0_go1_u dev_info__go2_status_0;

	union dci_union__go2_status_1_go1_u dev_info__go2_status_1;

	uint8_t dev_info__device_status;

	uint8_t dev_info__ui_stream_count;
};

struct dci_ui__rng_data__header_t {
	union dci_union__rng_data__header_u rng_data_header;
};

struct dci_ui__rng_data__footer_t {
	union dci_union__rng_data__footer_u rng_data_footer;
};

struct dci_ui__packed_data__bh_t {

	union dci_union__block_header_u packed_data__bh;
};

struct dci_ui__host_packed_data__bh_t {

	union dci_union__block_header_u packed_data__bh;

	uint32_t block_sz_bytes;
};

struct dci_ui__fw_breakpoints_t {

	uint32_t fw_breakpoints_status;
};

struct dci_ui__fw_checksum_t {

	uint32_t fw_checksum;
};

#ifdef __cplusplus
}
#endif

#endif
