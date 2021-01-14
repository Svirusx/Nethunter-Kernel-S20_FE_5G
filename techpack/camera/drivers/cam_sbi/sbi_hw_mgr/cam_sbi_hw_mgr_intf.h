/* Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _CAM_SBI_HW_MGR_INTF_H_
#define _CAM_SBI_HW_MGR_INTF_H_

#include <linux/of.h>
#include <linux/time.h>
#include <linux/list.h>

#include <cam_sbi.h>
#include "cam_debug_util.h"
#include "cam_hw.h"
#include "cam_hw_mgr_intf.h"
#include "cam_hw_intf.h"


#define CAM_SBI_HW_TYPE_1   1

#define CAM_SBI_HW_RES_MAX 1
#define CAM_CUSTOM_HW_RES_MAX 32

#define CAM_SBI_HW_SUB_MOD_MAX 1
#define CAM_CUSTOM_HW_SUB_MOD_MAX 1
#define CAM_CUSTOM_CSID_HW_MAX    1

#define CAM_SBI_MAX_PER_PATH_VOTES 30

enum cam_sbi_hw_event_type {
	CAM_SBI_EVENT_TYPE_ERROR,
	CAM_SBI_HW_EVENT_DONE,
    CAM_SBI_EVENT_MAX,
};

enum cam_sbi_cmd_types {
	CAM_SBI_CMD_NONE,
	CAM_SBI_SET_IRQ_CB,
	CAM_SBI_SUBMIT_REQ,
};

/**
 * struct cam_sbi_stop_args - hardware stop arguments
 *
 * @stop_only                  Send stop only to hw drivers. No Deinit to be
 *                             done.
 */
struct cam_sbi_stop_args {
	bool                          stop_only;
};

/**
 * struct cam_sbi_start_args - sbi hardware start arguments
 *
 * @config_args:               Hardware configuration commands.
 * @start_only                 Send start only to hw drivers. No init to
 *                             be done.
 *
 */
struct cam_sbi_start_args {
	struct cam_hw_config_args     hw_config;
	bool                          start_only;
};

/**
 * struct cam_sbi_bw_config_internal_v2 - Bandwidth configuration
 *
 * @num_paths:                  Number of data paths
 * @axi_path                    per path vote info
 */
struct cam_sbi_bw_config_internal_v2 {
	uint32_t                          num_paths;
	struct cam_axi_per_path_bw_vote   axi_path[CAM_SBI_MAX_PER_PATH_VOTES];
};

/**
 * struct cam_sbi_prepare_hw_update_data - hw prepare data
 *
 * @packet_opcode_type:     Packet header opcode in the packet header
 *                          this opcode defines, packet is init packet or
 *                          update packet
 *
 */
struct cam_sbi_prepare_hw_update_data {
	uint32_t                              packet_opcode_type;
	struct cam_sbi_bw_config_internal_v2  bw_config_v2;
	bool                                  bw_config_valid;
};

/**
 * struct cam_sbi_hw_cb_args : HW manager callback args
 *
 * @irq_status : irq status
 * @req_info   : Pointer to the request info associated with the cb
 */
struct cam_sbi_hw_cb_args {
	uint32_t	irq_status;
	uint32_t	cb_type;// from LRME
	struct cam_sbi_sub_mod_req_to_dev *req_info;
	struct cam_sbi_frame_request *frame_req;// from LRME
};

/**
 * cam_sbi_hw_sub_mod_init()
 *
 * @Brief:                  Initialize Custom HW device
 *
 * @sbi_hw:              sbi_hw interface to fill in and return on
 *                          successful initialization
 * @hw_idx:                 Index of Custom HW
 */
int cam_sbi_sub_mod_init(struct cam_hw_intf **sbi_hw, uint32_t hw_idx);

 /**
 * cam_custom_csid_hw_init()
 *
 * @Brief:                  Initialize Custom HW device
 *
 * @custom_hw:              cust_hw interface to fill in and return on
 *                          successful initialization
 * @hw_idx:                 Index of Custom HW
 */
int cam_custom_csid_hw_init(struct cam_hw_intf **custom_hw,
				      uint32_t hw_idx);

int cam_sbi_hw_mgr_init(struct cam_hw_mgr_intf *hw_mgr_intf,
    cam_hw_event_cb_func cam_sbi_dev_buf_done_cb, int *iommu_hdl);

int cam_sbi_hw_mgr_deinit(void);

#endif /* _CAM_SBI_HW_MGR_INTF_H_ */
