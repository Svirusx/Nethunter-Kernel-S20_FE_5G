
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

#ifndef VL53L5_K_ERROR_CODES_H_
#define VL53L5_K_ERROR_CODES_H_

#define VL53L5_K_ERROR_DRIVER_NOT_INITIALISED			((int) -501)
#define VL53L5_K_ERROR_DEVICE_NOT_PRESENT			((int) -502)
#define VL53L5_K_ERROR_DEVICE_NOT_POWERED			((int) -503)
#define VL53L5_K_ERROR_DEVICE_NOT_INITIALISED			((int) -504)
#define VL53L5_K_ERROR_DEVICE_NOT_RANGING			((int) -507)
#define VL53L5_K_ERROR_DEVICE_IS_RANGING			((int) -508)
#define VL53L5_K_ERROR_DEVICE_IS_BUSY				((int) -509)
#define VL53L5_K_ERROR_DEVICE_STATE_INVALID			((int) -510)
#define VL53L5_K_ERROR_SLEEP_FOR_DATA_INTERRUPTED		((int) -511)
#define VL53L5_K_ERROR_WORKER_THREAD_TIMEOUT			((int) -512)
#define VL53L5_K_ERROR_FAILED_TO_ALLOCATE_WORKER_WAIT_LIST	((int) -513)
#define VL53L5_K_ERROR_SPI_BUSNUM_INVALID			((int) -514)
#define VL53L5_K_ERROR_SPI_NEW_DEVICE_FAILED			((int) -515)
#define VL53L5_K_ERROR_SPI_SETUP_FAILED				((int) -516)
#define VL53L5_K_ERROR_SPI_DRIVER_REGISTER_FAILED		((int) -517)
#define VL53L5_K_ERROR_I2C_ADAPTER_INVALID			((int) -518)
#define VL53L5_K_ERROR_I2C_NEW_DEVICE_FAILED			((int) -519)
#define VL53L5_K_ERROR_I2C_ADD_DRIVER_FAILED			((int) -520)
#define VL53L5_K_ERROR_I2C_CHECK_FAILED				((int) -521)
#define VL53L5_K_ERROR_GPIO_IS_DISABLED				((int) -522)
#define VL53L5_K_ERROR_GPIO_REQUEST_FAILED			((int) -523)
#define VL53L5_K_ERROR_GPIO_DIRECTION_SET_FAILED		((int) -524)
#define VL53L5_K_ERROR_ATTEMPT_TO_SET_DISABLED_GPIO		((int) -525)
#define VL53L5_K_ERROR_RANGE_POLLING_TIMEOUT			((int) -526)
#define VL53L5_K_ERROR_STATE_POLLING_TIMEOUT			((int) -527)
#define VL53L5_K_ERROR_FAILED_TO_ALLOCATE_PROFILE		((int) -528)
#define VL53L5_K_ERROR_FAILED_TO_ALLOCATE_FIRMWARE		((int) -529)
#define VL53L5_K_ERROR_FAILED_TO_ALLOCATE_VERSION		((int) -530)
#define VL53L5_K_ERROR_FAILED_TO_ALLOCATE_MODULE_INFO		((int) -531)
#define VL53L5_K_ERROR_FAILED_TO_ALLOCATE_RAW_DATA		((int) -532)
#define VL53L5_K_ERROR_FAILED_TO_COPY_FLAGS			((int) -533)
#define VL53L5_K_ERROR_FAILED_TO_COPY_FIRMWARE			((int) -534)
#define VL53L5_K_ERROR_FAILED_TO_COPY_VERSION			((int) -535)
#define VL53L5_K_ERROR_FAILED_TO_COPY_MODULE_INFO		((int) -536)
#define VL53L5_K_ERROR_FAILED_TO_COPY_RAW_DATA			((int) -537)
#define VL53L5_K_ERROR_FAILED_TO_COPY_RANGE_DATA		((int) -538)
#define VL53L5_K_ERROR_FAILED_TO_COPY_ERROR_INFO		((int) -539)
#define VL53L5_K_ERROR_RANGE_DATA_NOT_READY			((int) -540)
#define VL53L5_K_ERROR_FAILED_TO_COPY_PARAMETER_DATA		((int) -541)
#define VL53L5_K_ERROR_FAILED_TO_ALLOCATE_PARAMETER_DATA	((int) -542)
#define VL53L5_K_ERROR_FAILED_TO_COPY_POWER_MODE		((int) -543)
#define VL53L5_K_ERROR_INVALID_POWER_STATE			((int) -544)
#define VL53L5_K_ERROR_DEVICE_INTERRUPT_NOT_OWNED		((int) -545)
#define VL53L5_K_ERROR_FAILED_TO_COPY_CONFIG_PRESET		((int) -546)
#define VL53L5_K_ERROR_FAILED_TO_ALLOCATE_FILE_DATA		((int) -547)
#define VL53L5_K_ERROR_FAILED_TO_COPY_FILE_DATA 		((int) -548)
#define VL53L5_K_ERROR_FAILED_TO_ALLOCATE_CALIBRATION 		((int) -549)
#define VL53L5_K_ERROR_FAILED_TO_COPY_CALIBRATION 		((int) -550)
#define VL53L5_K_ERROR_FAILED_TO_ALLOCATE_RANGE_DATA		((int) -551)
#define VL53L5_K_ERROR_INVALID_NO_OF_ZONES			((int) -552)
#define VL53L5_K_ERROR_INVALID_ROTATION				((int) -553)
#define VL53L5_K_ERROR_INVALID_CONFIG_PRESET			((int) -554)
#define VL53L5_K_ERROR_INVALID_CALIBRATION_FLOW			((int) -555)

#define VL53L5_K_ERROR_NOT_DEFINED				((int) -888)

#ifdef STM_VL53L5_SUPPORT_SEC_CODE
#define VL53L5_PROBE_FAILED                             	((int32_t) -1000)
#define VL53L5_SHUTDOWN                            	((int32_t) -1001)
#endif
#endif
