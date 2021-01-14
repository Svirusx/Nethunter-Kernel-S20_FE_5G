
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

#ifndef __VL53L5_CORE_DEV_PATH_H__
#define __VL53L5_CORE_DEV_PATH_H__

#ifdef __cplusplus
extern "C" {
#endif

#define VL53L5_MAP_CORE_DEV(p_dev) \
	((p_dev)->host_dev.core_dev)
#define VL53L5_MAP_VERSION(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).map_version
#define VL53L5_FW_VERSION(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).fw_version
#define VL53L5_CFG_INFO(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).cfg_info
#define VL53L5_FMT_TRACEABILITY(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).fmt_traceability
#define VL53L5_EWS_TRACEABILITY(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ews_traceability
#define VL53L5_UI_RNG_DATA_ADDR(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ui_rng_data_addr
#define VL53L5_SILICON_TEMP_DATA(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).silicon_temp_data
#define VL53L5_ZONE_CFG(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).zone_cfg
#define VL53L5_RNG_RATE_CFG(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).rng_rate_cfg
#define VL53L5_INT_MAX_CFG(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).int_max_cfg
#define VL53L5_INT_MIN_CFG(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).int_min_cfg
#define VL53L5_INT_MPX_DELTA_CFG(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).int_mpx_delta_cfg
#define VL53L5_INT_DSS_CFG(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).int_dss_cfg
#define VL53L5_FACTORY_CAL_CFG(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).factory_cal_cfg
#define VL53L5_OUTPUT_DATA_CFG(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).output_data_cfg
#define VL53L5_OUTPUT_BH_CFG(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).output_bh_cfg
#define VL53L5_OUTPUT_BH_ENABLES(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).output_bh_enables
#define VL53L5_OUTPUT_BH_LIST(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).output_bh_list
#define VL53L5_INTERRUPT_CFG(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).interrupt_cfg
#define VL53L5_NVM_LASER_SAFETY_INFO(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).nvm_laser_safety_info
#define VL53L5_PATCH_HOOK_ENABLES_ARRAY(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).patch_hook_enables_array
#define VL53L5_DEVICE_ERROR_STATUS(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).device_error_status
#define VL53L5_DEVICE_WARNING_STATUS(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).device_warning_status
#define VL53L5_IC_CHECKER_0(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_0
#define VL53L5_IC_CHECKER_1(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_1
#define VL53L5_IC_CHECKER_2(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_2
#define VL53L5_IC_CHECKER_3(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_3
#define VL53L5_IC_CHECKER_4(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_4
#define VL53L5_IC_CHECKER_5(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_5
#define VL53L5_IC_CHECKER_6(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_6
#define VL53L5_IC_CHECKER_7(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_7
#define VL53L5_IC_CHECKER_8(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_8
#define VL53L5_IC_CHECKER_9(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_9
#define VL53L5_IC_CHECKER_10(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_10
#define VL53L5_IC_CHECKER_11(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_11
#define VL53L5_IC_CHECKER_12(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_12
#define VL53L5_IC_CHECKER_13(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_13
#define VL53L5_IC_CHECKER_14(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_14
#define VL53L5_IC_CHECKER_15(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_15
#define VL53L5_IC_CHECKER_16(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_16
#define VL53L5_IC_CHECKER_17(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_17
#define VL53L5_IC_CHECKER_18(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_18
#define VL53L5_IC_CHECKER_19(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_19
#define VL53L5_IC_CHECKER_20(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_20
#define VL53L5_IC_CHECKER_21(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_21
#define VL53L5_IC_CHECKER_22(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_22
#define VL53L5_IC_CHECKER_23(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_23
#define VL53L5_IC_CHECKER_24(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_24
#define VL53L5_IC_CHECKER_25(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_25
#define VL53L5_IC_CHECKER_26(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_26
#define VL53L5_IC_CHECKER_27(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_27
#define VL53L5_IC_CHECKER_28(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_28
#define VL53L5_IC_CHECKER_29(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_29
#define VL53L5_IC_CHECKER_30(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_30
#define VL53L5_IC_CHECKER_31(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_31
#define VL53L5_IC_CHECKER_32(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_32
#define VL53L5_IC_CHECKER_33(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_33
#define VL53L5_IC_CHECKER_34(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_34
#define VL53L5_IC_CHECKER_35(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_35
#define VL53L5_IC_CHECKER_36(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_36
#define VL53L5_IC_CHECKER_37(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_37
#define VL53L5_IC_CHECKER_38(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_38
#define VL53L5_IC_CHECKER_39(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_39
#define VL53L5_IC_CHECKER_40(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_40
#define VL53L5_IC_CHECKER_41(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_41
#define VL53L5_IC_CHECKER_42(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_42
#define VL53L5_IC_CHECKER_43(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_43
#define VL53L5_IC_CHECKER_44(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_44
#define VL53L5_IC_CHECKER_45(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_45
#define VL53L5_IC_CHECKER_46(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_46
#define VL53L5_IC_CHECKER_47(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_47
#define VL53L5_IC_CHECKER_48(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_48
#define VL53L5_IC_CHECKER_49(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_49
#define VL53L5_IC_CHECKER_50(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_50
#define VL53L5_IC_CHECKER_51(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_51
#define VL53L5_IC_CHECKER_52(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_52
#define VL53L5_IC_CHECKER_53(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_53
#define VL53L5_IC_CHECKER_54(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_54
#define VL53L5_IC_CHECKER_55(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_55
#define VL53L5_IC_CHECKER_56(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_56
#define VL53L5_IC_CHECKER_57(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_57
#define VL53L5_IC_CHECKER_58(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_58
#define VL53L5_IC_CHECKER_59(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_59
#define VL53L5_IC_CHECKER_60(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_60
#define VL53L5_IC_CHECKER_61(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_61
#define VL53L5_IC_CHECKER_62(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_62
#define VL53L5_IC_CHECKER_63(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_checker_63
#define VL53L5_IC_VALID_TGT_STATUS(p_dev) \
	VL53L5_MAP_CORE_DEV(p_dev).ic_valid_tgt_status

#ifdef __cplusplus
}
#endif

#endif
