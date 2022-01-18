
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

#ifndef _VL53L5_PLATFORM_H_
#define _VL53L5_PLATFORM_H_

#include "vl53l5_platform_log.h"
#include "vl53l5_platform_user_data.h"
#include "vl53l5_error_codes.h"

/**
 * @file vl53l5_platform.h
 *
 * @brief Customer implemented platform interface
 *
 */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @defgroup VL53L5_platform_group VL53L5 Platform
 * @brief    Platform comms initialisation, termination, timing, and gpio
 *           functions.
 * @{
 */

#define GPIO_LOW 0
#define GPIO_HIGH 1

/**
 * @defgroup VL53L5_platform_group VL53L5 Platform
 *  @brief    Platform comms initialisation, termination, timing, and gpio
 *            functions.
 *  @{
 */
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
/**
 * @brief  Performs any initialisation required by the platform specific I2C or
 *          GPIO functions
 *
 * @param[in,out] p_dev      pointer to instance of device handle struct
 *
 *  @return      status (error code if < 0)
 */
int32_t vl53l5_comms_initialise(struct vl53l5_dev_handle_t *p_dev);

/** @brief  Closes any platform specific handles opened with
 *          vl53l5_comms_initialise()
 *
 * @param[in,out] p_dev      pointer to instance of device handle struct
 *
 *  @return      status (error code if < 0)
 */
int32_t vl53l5_comms_close(struct vl53l5_dev_handle_t *p_dev);
#endif
/** @brief  Writes one or more bytes to the VL53L5 device using the I2C bus
 *
 *  @param[in] p_dev      pointer to instance of device handle struct
 *  @param[in] index                I2C 16-bit register index
 *  @param[in] pdata                pointer to a buffer containing the bytes to
 *                                  write on the I2C bus
 *  @param[in] count                number of bytes to write to the I2C bus
 *
 *  @return      status (error code if < 0)
 */
int32_t vl53l5_write_multi(
	struct vl53l5_dev_handle_t *p_dev, uint16_t index, uint8_t *pdata,
	uint32_t count);

/** @brief  Read one or more bytes from the VL53L5 device using the I2C bus
 *
 *  @param[in] p_dev      pointer to instance of device handle struct
 *  @param[in] index                I2C 16-bit register index
 *  @param[out] pdata               pointer to a buffer to be populated with
 *                                  bytes read from the device on the I2C bus
 *  @param[in] count                number of bytes to read from the I2C bus
 *
 *  @return      status (error code if < 0)
 */
int32_t vl53l5_read_multi(
	struct vl53l5_dev_handle_t *p_dev, uint16_t index, uint8_t *pdata,
	uint32_t count);

/** @brief  A platform specific delay function to wait for wait_us microseconds
 *
 *  @param[in] p_dev      pointer to instance of device handle struct
 *  @param[in] wait_us   time in microseconds to wait
 *
 *  @return      status (error code if < 0)
 */
int32_t vl53l5_wait_us(struct vl53l5_dev_handle_t *p_dev, uint32_t wait_us);

/** @brief  A platform specific delay function to wait for wait_ms milliseconds
 *
 *  @param[in] p_dev      pointer to instance of device handle struct
 *  @param[in] wait_ms   time in milliseconds to wait
 *
 *  @return      status (error code if < 0)
 */
int32_t vl53l5_wait_ms(struct vl53l5_dev_handle_t *p_dev, uint32_t wait_ms);
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
/**
 * @brief  Implements support for the Low Power Control GPIO pin of the VL53L5
 *          device
 *
 * @param[in] p_dev      pointer to instance of device handle struct
 *  @param[in] value     Low Power Pin GPIO state. 0 = GPIO low, 1 = GPIO high
 *
 *  @return      status (error code if < 0)
 *
 * @note This function is optional if fixed supplies are in use, or for
 *       alternative control of device power supplies.<BR>
 *       See data sheet for more information.
 */
int32_t vl53l5_gpio_low_power_control(
	struct vl53l5_dev_handle_t *p_dev, uint8_t value);

/**
 * @brief  Implements support power enable or disable to the VL53L5 device.
 *
 * @param[in] p_dev      pointer to instance of device handle struct
 *  @param[in] value     Power Enable GPIO state. 0 = GPIO low, 1 = GPIO high
 *
 *  @return      status (error code if < 0)
 *
 * @note This function is optional if fixed supplies are in use, or for
 *       alternative control of device power supplies.<BR>
 *       See data sheet for more information.
 */
int32_t vl53l5_gpio_power_enable(
	struct vl53l5_dev_handle_t *p_dev, uint8_t value);

/**
 * @brief  Implements support for comms select to the VL53L5 device.
 *
 * @param[in] p_dev      pointer to instance of device handle struct
 *  @param[in] value     Comms select GPIO state. 0 = I2C, 1 = SPI
 *
 *  @return      status (error code if < 0)
 *
 * @note This function is optional if fixed supplies are in use, or for
 *       alternative control of device power supplies.<BR>
 *       See data sheet for more information.
 */
int32_t vl53l5_gpio_comms_select(
	struct vl53l5_dev_handle_t *p_dev, uint8_t value);
#endif
/**
 * @brief Implements support for interrupt handler enabling on GPIO2
 *
 * @param[in]  p_dev      pointer to instance of device handle struct
 * @param[in]  function  a function callback supplied by the caller, for
 *                       interrupt notification
 * @param[in]  edge_type falling edge or rising edge interrupt detection
 *
 * @return      status (error code if < 0)
 */
int32_t vl53l5_gpio_interrupt_enable(
	struct vl53l5_dev_handle_t *p_dev, void (*function)(void),
	uint8_t edge_type);

/**
 * @brief Implements support for interrupt handler disabling on GPIO2
 *
 * @param[in]  p_dev      pointer to instance of device handle struct
 *
 * @return      status (error code if < 0)
 */
int32_t vl53l5_gpio_interrupt_disable(struct vl53l5_dev_handle_t *p_dev);
/**
 * @brief Get a pointer to a debug buffer. If no persistent debug buffer is
 *        implemented, this function shall allocate memory for the debug buffer.
 *        The length of the buffer must be VL53L5_COMMS_BUFFER_SIZE_BYTES bytes.
 *
 * @param[in] p_dev      pointer to instance of device handle struct
 * @param[out]   pp_comms_buffer      : pointer to comms buffer pointer
 *
 * @return  VL53L5_ERROR_NONE     Success
 * @return  "Other error code"    See ::vl53l5_Error
 */
int32_t vl53l5_get_comms_buffer(
	struct vl53l5_dev_handle_t *p_dev,
	uint8_t **pp_comms_buffer);

/**
 * @brief Free the comms buffer. This function should only release memory
 *        allocated by vl53l5_get_comms_buffer. If a persistent (static) comms
 *        buffer is in use, this function shall only return VL53L5_ERROR_NONE.
 *
 * @param[in] p_dev      pointer to instance of device handle struct
 * @param[out]   p_comms_buffer      : comms buffer pointer
 *
 * @return  VL53L5_ERROR_NONE     Success
 * @return  "Other error code"    See ::vl53l5_Error
 */
int32_t vl53l5_free_comms_buffer(
	struct vl53l5_dev_handle_t *p_dev,
	uint8_t *p_comms_buffer);

/**
 * @brief Get a pointer to a debug buffer. If no persistent debug buffer is
 *        implemented, this function shall allocate memory for the debug buffer.
 *        The length of the buffer must be VL53L5_DEBUG_BUFFER_SIZE_BYTES bytes.
 *
 * @param[in]    p_dev      pointer to instance of device handle struct
 * @param[out]   pp_debug_buffer     : pointer to debug buffer pointer
 *
 * @return  VL53L5_ERROR_NONE     Success
 * @return  "Other error code"    See ::vl53l5_Error
 */
int32_t vl53l5_get_debug_buffer(
	struct vl53l5_dev_handle_t *p_dev,
	uint8_t **pp_debug_buffer);

/**
 * @brief Free the debug buffer. This function should only release memory
 *        allocated by vl53l5_get_debug_buffer. If a persistent (static) debug
 *        buffer is in use, this function shall only return VL53L5_ERROR_NONE.
 *
 * @param[in]    p_dev      pointer to instance of device handle struct
 * @param[out]   p_debug_buffer      : debug buffer pointer
 *
 * @return  VL53L5_ERROR_NONE     Success
 * @return  "Other error code"    See ::vl53l5_Error
 */
int32_t vl53l5_free_debug_buffer(
	struct vl53l5_dev_handle_t *p_dev,
	uint8_t *p_debug_buffer);

/**
 * @brief  Returns the value (in milliseconds) of a free running system timer
 *         or clock.
 *
 * @param[in]  p_dev      pointer to instance of device handle struct
 * @param[out] ptime_ms  pointer to variable to receive the current time in
 *                       milliseconds
 *
 * @return      status (error code if < 0)
 */

int32_t vl53l5_get_tick_count(
	struct vl53l5_dev_handle_t *p_dev, uint32_t *ptime_ms);


/**
 * @brief  Implements a comparison of timeout values which is safe from timer
 *         roll over.
 *
 * @param[in] p_dev      pointer to instance of device handle struct
 * @param[in] start_time_ms          start time in milliseconds
 * @param[in] end_time_ms end time in milliseconds
 * @param[in] timeout_ms  timeout in milliseconds
 *
 * @return      status (VL53L5_ERROR_TIME_OUT if timeout occurred)
 */

int32_t vl53l5_check_for_timeout(
	struct vl53l5_dev_handle_t *p_dev, uint32_t start_time_ms,
	uint32_t end_time_ms, uint32_t timeout_ms);

/**
 * @brief Implements reading of a file in the kernel
 * @param[in]  p_dev      pointer to instance of device handle struct
 * @param[in]  data       pointer to buffer for data to be loaded into
 * @param[in]  size       size of data to be read
 * @param[in]  path       path to file to be read
 *
 * @return      status (error code if < 0)
 */
int32_t vl53l5_read_file(struct vl53l5_dev_handle_t *pdev, char *data,
			  int size, const char *path);

/**
 * @brief Implements saving of a file in the kernel
 * @param[in]  p_dev       pointer to instance of device handle struct
 * @param[in]  data        pointer to buffer for data to be saved from
 * @param[in]  size        size of calibration data to be saved
 * @param[in]  path        path to file to be saved
 * @param[in]  device_info pointer to buffer of device info data
 * @param[in]  info_size   size of device info data to be saved
 *
 * @return      status (error code if < 0)
 */
int32_t vl53l5_write_file(struct vl53l5_dev_handle_t *pdev, char *data,
			  int size, const char *path, char *device_info,
			  int info_size);

#ifdef __cplusplus
}
#endif

/** @} VL53L5_platform_group */

#endif
