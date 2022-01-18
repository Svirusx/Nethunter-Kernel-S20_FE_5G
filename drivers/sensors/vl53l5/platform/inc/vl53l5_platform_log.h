
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
 * @file  vl53l5_platform_log.h
 *
 * @brief Platform logging functions
 */


#ifndef _VL53L5_PLATFORM_LOG_H_
#define _VL53L5_PLATFORM_LOG_H_

#include "vl53l5_platform_user_config.h"
#include <linux/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef VL53L5_KERNEL_LOG
#define VL53L5_MAX_STRING_LENGTH                       0x8000

/**
 * @brief Set the level, output and specific functions for module logging.
 *
 *
 * @param filename  - full path of output log file, NULL for print to stdout
 *
 * @param modules   - Module or None or All to trace
 *                      VL53L5_TRACE_MODULE_NONE
 *                      VL53L5_TRACE_MODULE_API
 *                      VL53L5_TRACE_MODULE_CORE
 *                      VL53L5_TRACE_MODULE_TUNING
 *                      VL53L5_TRACE_MODULE_CHARACTERISATION
 *                      VL53L5_TRACE_MODULE_PLATFORM
 *                      VL53L5_TRACE_MODULE_ALL
 *
 * @param level     - trace level
 *                      VL53L5_TRACE_LEVEL_NONE
 *                      VL53L5_TRACE_LEVEL_ERRORS
 *                      VL53L5_TRACE_LEVEL_WARNING
 *                      VL53L5_TRACE_LEVEL_INFO
 *                      VL53L5_TRACE_LEVEL_DEBUG
 *                      VL53L5_TRACE_LEVEL_ALL
 *                      VL53L5_TRACE_LEVEL_IGNORE
 *
 *  @param functions - function level to trace;
 *                      VL53L5_TRACE_FUNCTION_NONE
 *                      VL53L5_TRACE_FUNCTION_I2C
 *                      VL53L5_TRACE_FUNCTION_ALL
 *
 * @return status - always VL53L5_ERROR_NONE
 *
 */

#define		VL53L5_TRACE_LEVEL_NONE			0x00000000
#define		VL53L5_TRACE_LEVEL_ERRORS		0x00000001
#define		VL53L5_TRACE_LEVEL_WARNING		0x00000002
#define		VL53L5_TRACE_LEVEL_INFO			0x00000004
#define		VL53L5_TRACE_LEVEL_DEBUG		0x00000008
#define		VL53L5_TRACE_LEVEL_ALL			0x00000010
#define		VL53L5_TRACE_LEVEL_IGNORE		0x00000020

#define		VL53L5_TRACE_FUNCTION_NONE		0x00000000
#define		VL53L5_TRACE_FUNCTION_I2C		0x00000001
#define		VL53L5_TRACE_FUNCTION_ALL		0x7fffffff

#define		VL53L5_TRACE_MODULE_NONE		0x00000000
#define		VL53L5_TRACE_MODULE_API			0x00000001
#define		VL53L5_TRACE_MODULE_STTOF		0x00000002
#define		VL53L5_TRACE_MODULE_STTOF_HAL		0x00000004
#define		VL53L5_TRACE_MODULE_STTOF_CONTROLS	0x00000008
#define		VL53L5_TRACE_MODULE_HAL_VL53L5		0x00000010
#define		VL53L5_TRACE_MODULE_REGISTER_FUNCS	0x00000020
#define		VL53L5_TRACE_MODULE_DCI			0x00000040
#define		VL53L5_TRACE_MODULE_DCI_DECODE		0x00000080
#define		VL53L5_TRACE_MODULE_DCI_ENCODE		0x00000100
#define		VL53L5_TRACE_MODULE_DCI_INIT_DATA	0x00000200
#define		VL53L5_TRACE_MODULE_LOAD_FIRMWARE	0x00000400
#define		VL53L5_TRACE_MODULE_LOAD_CONFIG		0x00000800
#define		VL53L5_TRACE_MODULE_POWER_API		0x00001000

#define		VL53L5_TRACE_MODULE_CUSTOMER_API	0x40000000
#define		VL53L5_TRACE_MODULE_PLATFORM		0x40000010
#define		VL53L5_TRACE_MODULE_ALL			0x7fffffff

extern uint32_t _trace_level;

/*
 * NOTE: dynamically exported if we enable logging.
 *       this way, Python interfaces can access this function, but we don't
 *       need to include it in the .def files.
 */
VL53L5_API int8_t vl53l5_trace_config(
	char *filename,
	uint32_t modules,
	uint32_t level,
	uint32_t functions);

/*
 * NOTE: dynamically exported if we enable logging.
 *       this way, Python interfaces can access this function, but we don't
 *       need to include it in the .def files.
 */
VL53L5_API void vl53l5_trace_close(void);

/**
 * @brief Get global _trace_functions parameter
 *
 * @return _trace_functions
 */

uint32_t vl53l5_get_trace_functions(void);

/**
 * @brief Set global _trace_functions parameter
 *
 * @param[in] function : new function code
 */

void vl53l5_set_trace_functions(uint32_t function);


/*
* @brief Returns the current system tick count in [ms]
*
* @return  time_ms : current time in [ms]
*
*/
uint32_t vl53l5_clock(void);

#define LOG_GET_TIME() \
	((int)vl53l5_clock())

#define vl53l5_k_log_error(fmt, args...) \
	pr_err("LAF %s: " fmt, __func__, ##args)

#define vl53l5_k_log_debug(fmt, args...) \
	pr_debug("LAF %s: " fmt, __func__, ##args)

#define vl53l5_k_log_info(fmt, args...) \
	pr_info("LAF %s: " fmt, __func__, ##args)

#define vl53l5_k_log_warning(fmt, args...) \
	pr_warn("LAF %s: " fmt, __func__, ##args)

#define _LOG_TRACE_PRINT(module, level, function, fmt, args...)

#define _LOG_FUNCTION_START(module, fmt, args...) \
	vl53l5_k_log_debug("START " fmt "\n", ##args)

#define	_LOG_FUNCTION_END(module, status, args...)\
	vl53l5_k_log_debug("END %d\n", (int)status, ##args)

#define _LOG_GET_TRACE_FUNCTIONS()\
	vl53l5_get_trace_functions()

#define _LOG_SET_TRACE_FUNCTIONS(functions)\
	vl53l5_set_trace_functions(functions)

#define _FLUSH_TRACE_TO_OUTPUT()

#define _LOG_STRING_BUFFER(x) char x[VL53L5_MAX_STRING_LENGTH]

#else /* VL53L5_KERNEL_LOG - no logging */

#define _LOG_TRACE_PRINT(module, level, function, ...) ((void)0)
#define _LOG_FUNCTION_START(module, fmt, ...) ((void)0)
#define _LOG_FUNCTION_END(module, status, ...) ((void)0)
#define _FLUSH_TRACE_TO_OUTPUT() ((void)0)

#define vl53l5_k_log_debug(fmt, args...) ((void)0)
#define vl53l5_k_log_info(fmt, args...) ((void)0)
#define vl53l5_k_log_warning(fmt, args...) ((void)0)
#define vl53l5_k_log_error(fmt, args...) ((void)0)

#endif
#ifdef __cplusplus
}
#endif

#endif  /* _VL53L5_PLATFORM_LOG_H_ */
