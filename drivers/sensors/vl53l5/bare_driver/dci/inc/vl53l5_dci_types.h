
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

#ifndef _VL53L5_DCI_TYPES_H_
#define _VL53L5_DCI_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dci_luts.h"

enum block_format_type {
	LIST			= DCI_BH__P_TYPE__CONSEC_PARAMS_LIST,
	ARRAY_ELEMENT_SZ_1	= 0x1,
	ARRAY_ELEMENT_SZ_2	= 0x2,
	ARRAY_ELEMENT_SZ_3	= 0x3,
	ARRAY_ELEMENT_SZ_4	= 0x4,
	ARRAY_ELEMENT_SZ_5	= 0x5,
	ARRAY_ELEMENT_SZ_6	= 0x6,
	ARRAY_ELEMENT_SZ_7	= 0x7,
	ARRAY_ELEMENT_SZ_8	= 0x8,
	ARRAY_ELEMENT_SZ_9	= 0x9,
	ARRAY_ELEMENT_SZ_10	= 0xA,
	ARRAY_ELEMENT_SZ_11	= 0xB,
	DCI_PAGE_SELECT		= 0xC,
	START_OF_GROUP		= DCI_BH__P_TYPE__GRP_PARAMS_START,
	END_OF_GROUP		= DCI_BH__P_TYPE__GRP_PARAMS_END,
	END_OF_DATA		= DCI_BH__P_TYPE__END_OF_DATA
};

#ifdef __cplusplus
}
#endif

#endif
