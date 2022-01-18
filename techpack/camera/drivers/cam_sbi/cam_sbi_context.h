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

#ifndef _CAM_SBI_CONTEXT_H_
#define _CAM_SBI_CONTEXT_H_

#include "cam_context.h"
#include "cam_sbi_hw_mgr_intf.h"
#include "cam_context_utils.h"
#include "cam_hw_mgr_intf.h"
#include "cam_req_mgr_interface.h"
#include "cam_sync_api.h"

#define CAM_SBI_CTX_INDEX_SHIFT 32

/*
 * Maximum hw resource - This number is based on the maximum
 * output port resource. The current maximum resource number
 * is 5.
 */
#define CAM_SBI_DEV_CTX_RES_MAX                     2

#define CAM_SBI_CTX_CFG_MAX                         8


struct cam_sbi_dev_context;

/* cam sbi context irq handling function type */
typedef int (*cam_sbi_hw_event_cb_func)(struct cam_sbi_dev_context *ctx_sbi,
					void *evt_data);

/**
 * struct cam_isp_ctx_irq_ops - Function table for handling IRQ callbacks
 *
 * @irq_ops:               Array of handle function pointers.
 *
 */
struct cam_sbi_ctx_irq_ops {
	cam_sbi_hw_event_cb_func irq_ops[CAM_SBI_EVENT_MAX];
};


/**
 * struct cam_sbi_dev_ctx_req - SBI context request object
 *
 * @base:                  Common request object pointer
 * @cfg:                   SBI hardware configuration array
 * @num_cfg:               Number of SBI hardware configuration entries
 * @fence_map_out:         Output fence mapping array
 * @num_fence_map_out:     Number of the output fence map
 * @fence_map_in:          Input fence mapping array
 * @num_fence_map_in:      Number of input fence map
 * @num_acked:             Count to track acked entried for output.
 *                         If count equals the number of fence out, it means
 *                         the request has been completed.
 * @hw_update_data:        HW update data for this request
 * @bubble_detected:
 *
 */
struct cam_sbi_dev_ctx_req {
	struct cam_ctx_request                  *base;
	struct cam_hw_update_entry              cfg[CAM_SBI_CTX_CFG_MAX];
	uint32_t                                num_cfg;
	struct cam_hw_fence_map_entry           fence_map_out[CAM_SBI_DEV_CTX_RES_MAX];
	uint32_t                                num_fence_map_out;
	struct cam_hw_fence_map_entry           fence_map_in[CAM_SBI_DEV_CTX_RES_MAX];
	uint32_t                                num_fence_map_in;
	uint32_t                                num_acked;
	struct cam_sbi_prepare_hw_update_data hw_update_data;
    bool                                    bubble_detected;
};


/**
 * enum cam_sbi_ctx_activated_substate - sub states for activated
 *
 */
enum cam_sbi_ctx_activated_substate {
	CAM_SBI_CTX_ACTIVATED_SOF,
	CAM_SBI_CTX_ACTIVATED_APPLIED,
	CAM_SBI_CTX_ACTIVATED_EPOCH,
	CAM_SBI_CTX_ACTIVATED_BUBBLE,
	CAM_SBI_CTX_ACTIVATED_BUBBLE_APPLIED,
	CAM_SBI_CTX_ACTIVATED_HW_ERROR,
	CAM_SBI_CTX_ACTIVATED_HALT,
	CAM_SBI_CTX_ACTIVATED_MAX,
};


/**
 * struct cam_sbi_context - SBI device context
 * @base: SBI device context object
 * @state_machine: state machine for Custom device context
 * @state: Common context state
 * @hw_ctx: HW object returned by the acquire device command
 * @init_received: Indicate whether init config packet is received
 * @subscribe_event: The irq event mask that CRM subscribes to, IFE
 *                   will invoke CRM cb at those event.
 * @active_req_cnt: Counter for the active request
 * @frame_id: Frame id tracking for the SBI context
 * @req_base: common request structure
 * @req_sbi: sbi request structure
 *
 */
struct cam_sbi_dev_context {
	uint64_t                      index;// todo : delete
	struct cam_context           *base;
	struct cam_ctx_ops           *state_machine;
	uint32_t                      state;
	void                         *hw_ctx;
    bool                          hw_acquired;
	bool                          init_received;
	uint32_t                      subscribe_event;
	uint32_t                      active_req_cnt;
	int64_t                       frame_id;
	struct cam_ctx_request        req_base[CAM_CTX_REQ_MAX];
	struct cam_sbi_dev_ctx_req    req_sbi[CAM_CTX_REQ_MAX];
};

int cam_sbi_context_init(struct cam_sbi_dev_context *sbi_ctx,
	struct cam_context *base_ctx, struct cam_req_mgr_kmd_ops *bridge_ops, struct cam_hw_mgr_intf *hw_intf,
	uint32_t index);
int cam_sbi_context_deinit(struct cam_sbi_dev_context *sbi_ctx);

#endif /* _CAM_SBI_CONTEXT_H_ */
