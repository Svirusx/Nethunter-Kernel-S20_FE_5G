
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

#ifndef _VL53L5_ERROR_CODES_H_
#define _VL53L5_ERROR_CODES_H_

#include "vl53l5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VL53L5_Error int32_t

#define VL53L5_ERROR_NONE                               ((VL53L5_Error)  0)
#define VL53L5_FAILED_TO_CREATE_CALIB_FILE              ((VL53L5_Error) -2)

#define VL53L5_ERROR_INVALID_PARAMS                     ((VL53L5_Error) -4)

#define VL53L5_BUFFER_LARGER_THAN_EXPECTED_DATA         ((VL53L5_Error) -5)

#define VL53L5_INVALID_IDX_ENCODE_CMD_ERROR             ((VL53L5_Error) -6)

#define VL53L5_ERROR_TIME_OUT                           ((VL53L5_Error) -7)

#define VL53L5_CAL_TYPE_UNRECOGNISED                    ((VL53L5_Error) -8)

#define VL53L5_INVALID_IDX_DECODE_CMD_ERROR             ((VL53L5_Error) -9)

#define VL53L5_INVALID_PAGE_ERROR                       ((VL53L5_Error) -10)

#define VL53L5_DATA_BUFFER_MISMATCH                     ((VL53L5_Error) -11)

#define VL53L5_ERROR_NULL_DEV_PTR                       ((VL53L5_Error) -12)

#define VL53L5_ERROR_CONTROL_INTERFACE                  ((VL53L5_Error) -13)

#define VL53L5_DATA_EXCEEDS_CMD_BUFFER_SIZE             ((VL53L5_Error) -14)

#define VL53L5_ERROR_DIVISION_BY_ZERO                   ((VL53L5_Error) -15)

#define VL53L5_ERROR_INVALID_SET_COMMAND                ((VL53L5_Error) -16)

#define VL53L5_ERROR_CALIBRATION_REQUEST_INVALID        ((VL53L5_Error) -17)

#define VL53L5_ERROR_IDX_NOT_PRESENT                    ((VL53L5_Error) -18)

#define VL53L5_ERROR_INVALID_BH_ENCODE                  ((VL53L5_Error) -21)

#define VL53L5_ERROR_INVALID_BH_SIZE                    ((VL53L5_Error) -22)

#define VL53L5_ERROR_NULL_CALIBRATION_DEV_PTR           ((VL53L5_Error) -23)

#define VL53L5_GPIO_UNDEFINED                           ((VL53L5_Error) -24)

#define VL53L5_DEVICE_STATE_INVALID                     ((VL53L5_Error) -25)

#define VL53L5_ERROR_NULL_RESULTS_DEV_PTR               ((VL53L5_Error) -26)

#define VL53L5_ERROR_COMPARE_FIRMWARE_FAILURE           ((VL53L5_Error) -40)

#define VL53L5_ERROR_NOT_IMPLEMENTED                    ((VL53L5_Error) -41)

#define VL53L5_ERROR_POWER_STATE                        ((VL53L5_Error) -42)

#define VL53L5_PLL_LOCK_FAIL                            ((VL53L5_Error) -45)

#define VL53L5_LS_WATCHDOG_FAIL                         ((VL53L5_Error) -46)

#define VL53L5_FILE_NOT_EXIST                           ((VL53L5_Error) -49)

#define VL53L5_ERROR_BOOT_COMPLETE_TIMEOUT              ((VL53L5_Error) -51)

#define VL53L5_ERROR_MCU_IDLE_TIMEOUT                   ((VL53L5_Error) -52)

#define VL53L5_ERROR_RANGE_DATA_READY_TIMEOUT           ((VL53L5_Error) -53)

#define VL53L5_ERROR_CMD_STATUS_TIMEOUT                 ((VL53L5_Error) -54)

#define VL53L5_ERROR_UI_FW_STATUS_TIMEOUT               ((VL53L5_Error) -55)

#define VL53L5_ERROR_UI_RAM_WATCHDOG_TIMEOUT            ((VL53L5_Error) -56)

#define VL53L5_ERROR_FW_BUFF_NOT_FOUND                  ((VL53L5_Error) -57)

#define VL53L5_ERROR_MCU_ERROR_AT_ROM_BOOT              ((VL53L5_Error) -60)

#define VL53L5_ERROR_FALSE_MCU_ERROR_AT_ROM_BOOT        ((VL53L5_Error) -61)

#define VL53L5_ERROR_MCU_ERROR_AT_RAM_BOOT              ((VL53L5_Error) -62)

#define VL53L5_ERROR_FALSE_MCU_ERROR_AT_RAM_BOOT        ((VL53L5_Error) -63)

#define VL53L5_ERROR_MCU_ERROR_POWER_STATE              ((VL53L5_Error) -64)

#define VL53L5_ERROR_FALSE_MCU_ERROR_POWER_STATE        ((VL53L5_Error) -65)

#define VL53L5_DEV_INFO_MCU_ERROR                       ((VL53L5_Error) -66)

#define VL53L5_FALSE_DEV_INFO_MCU_ERROR                 ((VL53L5_Error) -67)

#define VL53L5_ERROR_FALSE_MCU_ERROR_IN_BANK_CHECK      ((VL53L5_Error) -68)

#define VL53L5_ERROR_PADDING_NOT_REQUIRED               ((VL53L5_Error) -70)

#define VL53L5_BYTE_SWAP_FAIL                           ((VL53L5_Error) -71)

#define VL53L5_IDX_MISSING_FROM_RETURN_PACKET           ((VL53L5_Error) -72)

#define VL53L5_VERSION_IDX_NOT_PRESENT                  ((VL53L5_Error) -73)

#define VL53L5_RANGE_DATA_ADDR_NOT_PRESENT              ((VL53L5_Error) -74)

#define VL53L5_ENCODE_CMD_ERROR                         ((VL53L5_Error) -75)

#define VL53L5_DECODE_CMD_ERROR                         ((VL53L5_Error) -76)

#define VL53L5_CONFIG_ERROR_INVALID_VERSION             ((VL53L5_Error) -77)

#define VL53L5_DCI_VERSION_ERROR                        ((VL53L5_Error) -78)

#define VL53L5_DCI_CMD_STATUS_ERROR                     ((VL53L5_Error) -79)

#define VL53L5_INVALID_CMD_ID                           ((VL53L5_Error) -80)

#define VL53L5_DCI_END_BLOCK_ERROR                      ((VL53L5_Error) -81)

#define VL53L5_INVALID_GROUP_INDEX                      ((VL53L5_Error) -82)

#define VL53L5_BOOT_TIMEOUT_BEFORE_FW_DOWNLOAD          ((VL53L5_Error) -83)

#define VL53L5_BOOT_TIMEOUT_AFTER_FW_DOWNLOAD           ((VL53L5_Error) -84)

#define VL53L5_BOOT_TIMEOUT_MCU_IDLE                    ((VL53L5_Error) -85)

#define VL53L5_DCI_RANGE_INTEGRITY_ERROR                ((VL53L5_Error) -86)

#define VL53L5_ERROR_NVM_COPY                           ((VL53L5_Error) -87)

#define VL53L5_NO_NEW_RANGE_DATA_ERROR                  ((VL53L5_Error) -89)

#define VL53L5_DATA_BUFFER_TOO_LARGE                    ((VL53L5_Error) -90)

#define VL53L5_GET_PARMS_ERROR_INVALID_IDX              ((VL53L5_Error) -91)

#define VL53L5_MAX_BUFFER_SIZE_REACHED                  ((VL53L5_Error) -92)

#define VL53L5_COMMS_ERROR                              ((VL53L5_Error) -93)

#define VL53L5_SPI_COMMS_ERROR                          ((VL53L5_Error) -94)

#define VL53L5_I2C_COMMS_ERROR                          ((VL53L5_Error) -95)

#define VL53L5_DATA_SIZE_MISMATCH                       ((VL53L5_Error) -96)

#define VL53L5_ERROR_GPIO_SET_FAIL                      ((VL53L5_Error) -97)

#define VL53L5_READ_RANGE_ERROR_CALIB_FILE_UNSPECIFIED  ((VL53L5_Error) -98)

#define VL53L5_VER_CHECK_NOT_PERFORMED                  ((VL53L5_Error) -99)

#define VL53L5_UNKNOWN_SILICON_REVISION                 ((VL53L5_Error) -100)

#define VL53L5_TOO_HIGH_AMBIENT_WARNING                 ((VL53L5_Error) -58)
#define VL53L5_ERROR_MCU_ERROR_WAIT_STATE               ((VL53L5_Error) -69)
#define VL53L5_ERROR_MCU_ERROR_HW_STATE                 ((VL53L5_Error) -59)
#define VL53L5_ERROR_MCU_NVM_NOT_PROGRAMMED             ((VL53L5_Error) -50)
#define VL53L5_ERROR_INIT_FW_CHECKSUM                   ((VL53L5_Error) -48)

#define VL53L5_NEW_RANGE_DATA_PRESENT                   ((VL53L5_Error)   1)

#define VL53L5_ERROR_UNDEFINED                          ((VL53L5_Error) -999)

#ifdef __cplusplus
}
#endif

#endif
