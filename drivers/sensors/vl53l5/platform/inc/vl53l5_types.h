
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

#ifndef _VL53L5_TYPES_H_
#define _VL53L5_TYPES_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/string.h>
#endif

#ifndef __KERNEL__
#include <stdint.h>
#include <stddef.h>
#endif

#ifndef NULL
#error "Error NULL definition should be done. Please add required include "
#endif

#ifndef STM_VL53L5_SUPPORT_LEGACY_CODE
#define STM_VL53L5_SUPPORT_SEC_CODE
#endif

#if defined(__BORLANDC__)
#include <vector>
#endif

#if !defined(_MSC_VER) && !defined(__BORLANDC__)
#include <stdbool.h>
#else
#pragma once

#define false   0
#define true    1

#define bool int
#endif

#if !defined(STDINT_H) && !defined(_STDINT_H) && !defined(__STDINT_H) && \
	!defined(_GCC_STDINT_H) && !defined(__STDINT_DECLS) && \
	!defined(_GCC_WRAP_STDINT_H)  && !defined(_STDINT) && \
	!defined(_LINUX_TYPES_H)

typedef u32 uint32_t;

typedef i32 int32_t;

typedef u16 uint16_t;

typedef i16 int16_t;

typedef u8 uint8_t;

typedef i8 int8_t;

#endif

#ifdef __cplusplus
}
#endif

#endif /* _VL53L5_TYPES_H_ */
