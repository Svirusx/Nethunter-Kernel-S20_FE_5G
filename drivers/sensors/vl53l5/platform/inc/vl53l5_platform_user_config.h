
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

/**
 * @file  vl53l5_platform_user_config.h
 *
 * @brief VL53L5 compile time user modifiable configuration
 */


#ifndef _VL53L5_PLATFORM_USER_CONFIG_H_
#define _VL53L5_PLATFORM_USER_CONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Used to define export tags if required
 * #define VL53L5_API  __declspec(dllexport)
 */
#define VL53L5_API

/** Default device comms wait period used in comms polling mechanisms (ms) */
#define VL53L5_DEFAULT_COMMS_POLLING_DELAY_MS 10
/** Default device stop command timeout for retention mode (ms) */
#define VL53L5_STOP_COMMAND_TIMEOUT 1000
/** Default device wait delay during MCU boot loop (ms) */
#define VL53L5_MCU_BOOT_WAIT_DELAY 50
/** Default device wait delay during transition to high power idle (ms) */
#define VL53L5_HP_IDLE_WAIT_DELAY 0
/** Default device wait delay during transition to low power idle (ms) */
#define VL53L5_LP_IDLE_WAIT_DELAY 0
/** Default device wait during SPI read to allow device interrupt handlin (us) */
#define VL53L5_RANGE_WAIT 600

#ifdef __cplusplus
}
#endif

#endif  /* _VL53L5_PLATFORM_USER_CONFIG_H_ */
