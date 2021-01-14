
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

#ifndef VL53L5_K_IOCTL_CONTROLS_H_
#define VL53L5_K_IOCTL_CONTROLS_H_

#include "vl53l5_k_module_data.h"
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
#include <linux/regulator/consumer.h>
#endif

int vl53l5_ioctl_init(struct vl53l5_k_module_t *p_module);

int vl53l5_ioctl_term(struct vl53l5_k_module_t *p_module);

int vl53l5_ioctl_get_version(struct vl53l5_k_module_t *p_module,
			     void __user *p);
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
int vl53l5_ioctl_get_module_info(struct vl53l5_k_module_t *p_module,
				 void __user *p);

int vl53l5_ioctl_start(struct vl53l5_k_module_t *p_module, void __user *p);

int vl53l5_ioctl_stop(struct vl53l5_k_module_t *p_module, void __user *p);
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
int vl53l5_ioctl_get_module_info(struct vl53l5_k_module_t *p_module,
				 void __user *p, void *internal_p);

int vl53l5_ioctl_start(struct vl53l5_k_module_t *p_module, void __user *p, int mode_control);

int vl53l5_ioctl_stop(struct vl53l5_k_module_t *p_module, void __user *p, int mode_control);

#endif //STM_VL53L5_SUPPORT_SEC_CODE
int vl53l5_ioctl_get_range(struct vl53l5_k_module_t *p_module, void __user *p);

#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
int vl53l5_ioctl_blocking_get_range(struct vl53l5_k_module_t *p_module,
				    void __user *p);

int vl53l5_ioctl_set_device_parameters(struct vl53l5_k_module_t *p_module,
				       void __user *p);
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
int vl53l5_ioctl_set_device_parameters(struct vl53l5_k_module_t *p_module,
				       void __user *p, int config);
#endif //STM_VL53L5_SUPPORT_SEC_CODE
int vl53l5_ioctl_get_device_parameters(struct vl53l5_k_module_t *p_module,
				       void __user *p);
#ifdef STM_VL53L5_SUPPORT_LEGACY_CODE
int vl53l5_ioctl_get_error_info(struct vl53l5_k_module_t *p_module,
				void __user *p);

int vl53l5_ioctl_set_power_mode(struct vl53l5_k_module_t *p_module,
				void __user *p);
#endif
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
int vl53l5_ioctl_set_power_mode(struct vl53l5_k_module_t *p_module,
				void __user *p, enum vl53l5_power_states state);
#endif //STM_VL53L5_SUPPORT_SEC_CODE
int vl53l5_ioctl_poll_data_ready(struct vl53l5_k_module_t *p_module);

int vl53l5_ioctl_read_p2p_calibration(struct vl53l5_k_module_t *p_module);

int vl53l5_ioctl_read_shape_calibration(struct vl53l5_k_module_t *p_module);

int vl53l5_perform_calibration(struct vl53l5_k_module_t *p_module,
				int flow);

int vl53l5_ioctl_read_generic_shape(struct vl53l5_k_module_t *p_module);
#ifdef STM_VL53L5_SUPPORT_SEC_CODE
int vl53l5_k_power_onoff(struct vl53l5_k_module_t *data,
    struct regulator *vdd_vreg, const char *vdd_reg_name, bool on);
#endif //STM_VL53L5_SUPPORT_SEC_CODE
#endif
