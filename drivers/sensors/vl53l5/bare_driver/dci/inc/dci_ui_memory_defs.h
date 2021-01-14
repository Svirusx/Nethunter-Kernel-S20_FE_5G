
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

#ifndef __DCI_UI_MEMORY_DEFS_H__
#define __DCI_UI_MEMORY_DEFS_H__

#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DCI_UI__DEVICE_INFO__START_IDX \
	((uint32_t) 0x810000)

#define DCI_UI__DEVICE_INFO__SIZE_BYTES \
	((uint32_t) 4U)

#define DCI_UI__RANGE_DATA__START_IDX \
	((uint32_t) 0x810008)

#define DCI_UI__COMMAND_WRITE__END_IDX \
	((uint32_t) 0x812FFB - 0x04)

#define DCI_UI__COMMAND_INFO__START_IDX \
	((uint32_t) DCI_UI__COMMAND_WRITE__END_IDX + 1)

#define DCI_UI__COMMAND_INFO__SIZE_BYTES \
	((uint32_t) 4U)

#define DCI_UI__COMMAND_FOOTER__START_IDX \
	((uint32_t) 0x812FF8 - 0x04)

#define DCI_UI__COMMAND_FOOTER__SIZE_BYTES \
	((uint32_t) 4U)

#define DCI_UI__COMMAND_RETURN__START_IDX \
	((uint32_t) 0x812C00)

#define DCI_UI__FIRMWARE_CHECKSUM_IDX \
	((uint32_t) 0x812FFC)

#ifdef __cplusplus
}
#endif

#endif
