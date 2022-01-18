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

#ifndef _CAM_SBI_HW_CORE_H_
#define _CAM_SBI_HW_CORE_H_

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <cam_defs.h>
#include "cam_sbi.h"

#include "cam_common_util.h"
#include "cam_debug_util.h"
#include "cam_io_util.h"
#include "cam_cpas_api.h"
#include "cam_cdm_intf_api.h"
#include "cam_sbi_hw_intf.h"
#include "cam_sbi_hw_soc.h"
#include "cam_req_mgr_workq.h"
#include "cam_sbi_hw.h"

#define __CAM_SBI_FRAME_DROP_NOTIFY_ENABLE__

#define CAM_SBI_HW_RESET_TIMEOUT 10
#define CAM_SBI_HW_FRAME_DONE_TIMEOUT 50

#define CAM_SBI_BUS_RD_MAX_CLIENTS 2
#define CAM_SBI_BUS_WR_MAX_CLIENTS 2

#define CAM_SBI_HW_WORKQ_NUM_TASK 30

#define CAM_SBI_TOP_IRQ_MASK 0x1000 //0x19
#define CAM_SBI_WE_IRQ_MASK_0 0x2
#define CAM_SBI_WE_IRQ_MASK_1 0x0
#define CAM_SBI_FE_IRQ_MASK 0x0

#define CAM_SBI_MAX_REG_PAIR_NUM 256
#define CAM_SBI_REG_OFFSET 0X200

#define CAM_SBI_ADDR_FIFO_Q_NUM 3
/**
 * enum cam_sbi_irq_set
 *
 * @CAM_SBI_IRQ_ENABLE  : Enable irqs
 * @CAM_SBI_IRQ_DISABLE : Disable irqs
 */
enum cam_sbi_irq_set {
	CAM_SBI_IRQ_ENABLE,
	CAM_SBI_IRQ_DISABLE,
};

/**
 * struct cam_sbi_cdm_info : information used to submit cdm command
 *
 * @cdm_handle      : CDM handle for this device
 * @cdm_ops         : CDM ops
 * @cdm_cmd         : CDM command pointer
 */
struct cam_sbi_cdm_info {
	uint32_t cdm_handle;
	struct cam_cdm_utils_ops *cdm_ops;
	struct cam_cdm_bl_request *cdm_cmd;
};

/**
 * struct cam_sbi_hw_work_data : Work data for HW work queue
 *
 * @top_irq_status : Top registers irq status
 * @fe_irq_status  : custom1 engine irq status
 * @we_irq_status  : custom2 engine irq status
 */
struct cam_sbi_hw_work_data {
	uint32_t top_irq_status;
};


enum cam_custom_hw_resource_type {
	CAM_CUSTOM_HW_RESOURCE_UNINT,
	CAM_CUSTOM_HW_RESOURCE_SRC,
	CAM_CUSTOM_HW_RESOURCE_MAX,
};


struct cam_custom_sub_mod_acq {
	enum cam_custom_hw_resource_type rsrc_type;
	int32_t acq;
	struct cam_custom_resource_node *rsrc_node;
};


/**
 *  enum cam_sbi_core_state : SBI core states
 *
 * @CAM_SBI_CORE_STATE_UNINIT        : SBI is in uninit state
 * @CAM_SBI_CORE_STATE_INIT          : SBI is in init state after probe
 * @ CAM_SBI_CORE_STATE_IDLE         : SBI is in idle state. Hardware is in
 *                                      this state when no frame is processing
 *                                      or waiting for this core.
 * @CAM_SBI_CORE_STATE_REQ_PENDING   : SBI is in pending state. One frame is
 *                                      waiting for processing
 * @CAM_SBI_CORE_STATE_PROCESSING    : SBI is in processing state. HW manager
 *                                      can submit one more frame to HW
 * @CAM_SBI_CORE_STATE_REQ_PROC_PEND : Indicate two frames are inside HW.
 * @CAM_SBI_CORE_STATE_RECOVERY      : Indicate core is in the process of reset
 * @CAM_SBI_CORE_STATE_MAX           : upper limit of states
 */
enum cam_sbi_core_state {
	CAM_SBI_CORE_STATE_UNINIT,
	CAM_SBI_CORE_STATE_INIT,
	CAM_SBI_CORE_STATE_IDLE,
	CAM_SBI_CORE_STATE_REQ_PENDING,
	CAM_SBI_CORE_STATE_PROCESSING,
	CAM_SBI_CORE_STATE_REQ_PROC_PEND,
	CAM_SBI_CORE_STATE_RECOVERY,
	CAM_SBI_CORE_STATE_MAX,
};

enum cam_sbi_cue_option {
	CAM_SBI_CUE_OPTION_MANUAL = 0,
	CAM_SBI_CUE_OPTION_AUTO
};

enum cam_sbi_error {
	CAM_SBI_NO_ERROR = 0,
	CAM_SBI_SSM_WRITE_FRAME_DROP,
	CAM_SBI_SSM_READ_FRAME_DROP,
	CAM_SBI_SSM_READ_FRAME_ERROR,
};

typedef struct t_cam_sbi_dma_base_addr {
	uint64_t addr_lr;
	uint64_t addr_hr; // only for SSM
	uint64_t preview_len;
	uint64_t record_len;
	bool is_ssm_mode;
	bool rec_write_enable;
	bool rec_read_enable;
	int total_written_frames;
	int total_read_frames;
	int t1_num;
	int t3_num;
	int t3_num_before_cue;
	int before_cue_num;
	int max_num;
	uint64_t preview;
	uint64_t preview_timestamp;
	uint64_t record_start;
	uint64_t record_timestamp_start;
	uint32_t sensor_width;
	uint32_t sensor_height;
	uint32_t frame_size;
	uint32_t batch_num;
	uint32_t batch_size;
	uint32_t max_frames;
	bool is_activated;
	uint32_t frame_id;
	uint32_t prev_frame_id;
	bool enter_960_60_transition;
	enum cam_sbi_cue_option cue_option;
	uint32_t ssm_framerate; // 0 : 960, 1 : 480
	bool cue_option_changed;
	enum cam_sbi_error error_status;
	bool error_notified;
	int frame_drop_count;

	// print info flags
	bool rdma_meet_last_frame;
	bool meet_last_frame_info;
	bool write_done_info;
	bool make_960_info;
	bool make_60_info;
	bool task3_enable_info;
} cam_sbi_ssm_info;

/**
 *  struct cam_sbi_core : SBI HW core information
 *
 * @hw_info        : Pointer to base HW information structure
 * @device_iommu   : Device iommu handle
 * @cdm_iommu      : CDM iommu handle
 * @hw_caps        : Hardware capabilities
 * @state          : Hardware state
 * @reset_complete : Reset completion
 * @work           : Hardware workqueue to handle irq events
 * @work_data      : Work data used by hardware workqueue
 * @hw_mgr_cb      : Hw manager callback
 * @req_proc       : Pointer to the processing frame request
 * @req_submit     : Pointer to the frame request waiting for processing
 * @hw_cdm_info    : CDM information used by this device
 * @hw_idx         : Hardware index
 */
struct cam_sbi_core {
	struct cam_sbi_hw_info *hw_info;
	struct cam_iommu_handle device_iommu;
	struct cam_iommu_handle cdm_iommu;
	struct cam_sbi_dev_cap_info hw_caps;
	enum cam_sbi_core_state state;
	struct completion all_reset_complete;
	struct completion t1_reset_complete;
	struct completion t1_frame_done_complete;
	struct completion t2_reset_complete;
	struct completion t2_frame_done_complete;
	struct completion t3_reset_complete;
	struct cam_req_mgr_core_workq *work;
	struct cam_sbi_hw_work_data work_data[CAM_SBI_HW_WORKQ_NUM_TASK];
	struct cam_sbi_hw_cmd_set_cb hw_mgr_cb;
	struct cam_sbi_frame_request *req_proc;
	struct cam_sbi_frame_request *req_submit;
	struct cam_sbi_cdm_info *hw_cdm_info;
	cam_sbi_ssm_info ssm_info;
	uint32_t hw_idx;
	bool dump_flag;

	//task2_action
	bool t2_stat_check;
	uint32_t t2_act_val;
	uint32_t prev_t2_act_val;

	// reset irq
	void *sbi_irq_controller;
	int reset_irq_handle;

	uint64_t clock_rate;
};

struct cam_sbi_top_reg_addr {
	uint32_t top_sbi_idle_update;
	uint32_t top_task1_setup;
	uint32_t top_task2_setup;
	uint32_t top_task3_setup;
	uint32_t top_task1_start_option;
	uint32_t top_task2_start_option;
	uint32_t top_task3_start_option;
	uint32_t top_task1_err_reset;
	uint32_t top_task2_err_reset;
	uint32_t top_task3_err_reset;
	uint32_t top_task1_done;
	uint32_t top_task2_done;
	uint32_t top_task3_done;
};

struct cam_sbi_dma_reg_addr {
	uint32_t dma_wdma0_frame_size;
	uint32_t dma_wdma0_baseaddr;
	uint32_t dma_wdma0_stride;
	uint32_t dma_wdma0_format;
	uint32_t dma_wdma0_control;
	uint32_t dma_wdma0_itr_mode;
	uint32_t dma_wdma0_error_enable;
	uint32_t dma_wdma0_offset_addr;
	uint32_t dma_wdma0_baseaddr_hr;
	uint32_t dma_wdma1_frame_size;
	uint32_t dma_wdma1_baseaddr;
	uint32_t dma_wdma1_stride;
	uint32_t dma_wdma1_format;
	uint32_t dma_wdma1_control;
	uint32_t dma_wdma1_itr_mode;
	uint32_t dma_wdma1_error_enable;
	uint32_t dma_wdma1_offset_addr;
	uint32_t dma_wdma2_frame_size;
	uint32_t dma_wdma2_baseaddr;
	uint32_t dma_wdma2_stride;
	uint32_t dma_wdma2_format;
	uint32_t dma_wdma2_control;
	uint32_t dma_wdma2_itr_mode;
	uint32_t dma_wdma2_error_enable;
	uint32_t dma_wdma2_baseaddr_hr;
	uint32_t dma_wdma2_offset_addr;
	uint32_t dma_wdma_max_moa;
	uint32_t dma_rdma0_frame_size;
	uint32_t dma_rdma0_baseaddr;
	uint32_t dma_rdma0_stride;
	uint32_t dma_rdma0_format;
	uint32_t dma_rdma0_control;
	uint32_t dma_rdma0_itr_mode;
	uint32_t dma_rdma0_error_enable;
	uint32_t dma_rdma0_offset_addr;
	uint32_t dma_rdma0_baseaddr_hr;
	uint32_t dma_rdma1_frame_size;
	uint32_t dma_rdma1_baseaddr;
	uint32_t dma_rdma1_stride;
	uint32_t dma_rdma1_format;
	uint32_t dma_rdma1_control;
	uint32_t dma_rdma1_itr_mode;
	uint32_t dma_rdma1_error_enable;
	uint32_t dma_rdma1_offset_addr;
	uint32_t dma_rdma1_baseaddr_hr;
	uint32_t dma_rdma2_frame_size;
	uint32_t dma_rdma2_baseaddr;
	uint32_t dma_rdma2_stride;
	uint32_t dma_rdma2_format;
	uint32_t dma_rdma2_control;
	uint32_t dma_rdma2_itr_mode;
	uint32_t dma_rdma2_error_enable;
	uint32_t dma_rdma2_baseaddr_hr;
	uint32_t dma_rdma2_offset_addr;
	uint32_t dma_rdma_max_moa;
};


/*
 * struct cam_sbi_irq_controller_reg_info
 * @Brief:                  Structure describing the SBI IRQ registers
 *
 * @num_registers:          Number of sets(mask/clear/status) of IRQ registers
 * @irq_reg_set:            Array of Register Set Offsets.
 *                          Length of array = num_registers
 * @global_clear_offset:    Offset of Global IRQ Clear register. This register
 *                          contains the BIT that needs to be set for the CLEAR
 *                          to take effect
 * @global_clear_bitmask:   Bitmask needed to be used in Global Clear register
 *                          for Clear IRQ cmd to take effect
 * @reset_cmd_offset:   	Bitmask needed to be used in Global reset register
 *                          for reset SBI cmd to take effect
 */
struct cam_sbi_irq_controller_reg_info {
	uint32_t                      num_registers;
	struct cam_irq_register_set  *irq_reg_set;
	uint32_t                      global_clear_offset;
	uint32_t                      global_clear_bitmask;
	uint32_t                      reset_cmd_offset;
};

/**
 * struct cam_sbi_hw_info : SBI registers information
 *
 */
struct cam_sbi_hw_info {
	struct cam_sbi_irq_controller_reg_info *irq_reg_info;
	struct cam_sbi_top_reg_addr top_reg;
	struct cam_sbi_dma_reg_addr dma_reg;
};

void cam_sbi_hw_configure_init(struct cam_hw_info *sbi_hw, void *args, uint32_t args_size);
void cam_sbi_hw_configure_mem_base_addr(struct cam_hw_info *sbi_hw, uint64_t base_addr);
void cam_sbi_hw_dump_all_reg(struct cam_hw_info *sbi_hw, int idx);// tempp code for checking power, clk enabled
int cam_sbi_hw_core_writereg(struct cam_hw_info *sbi_hw, int addr, int val);
int cam_sbi_hw_core_readreg(struct cam_hw_info *sbi_hw, int addr);
int cam_sbi_hw_process_irq(void *priv, void *data);
int cam_sbi_hw_submit_req(void *hw_priv, void *hw_submit_args,
			  uint32_t arg_size);
int cam_sbi_hw_reset(void *hw_priv, void *reset_core_args, uint32_t arg_size);
int cam_sbi_hw_stop(void *hw_priv, void *stop_hw_args, uint32_t arg_size);
int cam_sbi_hw_get_caps(void *hw_priv, void *get_hw_cap_args,
			uint32_t arg_size);
irqreturn_t cam_sbi_hw_irq(int irq_num, void *data);
int cam_sbi_hw_process_cmd(void *hw_priv, uint32_t cmd_type, void *cmd_args,
			   uint32_t arg_size);
int cam_sbi_hw_util_get_caps(struct cam_hw_info *sbi_hw,
				 struct cam_sbi_dev_cap_info *hw_caps);
int cam_sbi_hw_start(void *hw_priv, void *hw_init_args, uint32_t arg_size);
void cam_sbi_set_irq(struct cam_hw_info *sbi_hw, enum cam_sbi_irq_set set);

#endif /* _CAM_SBI_HW_CORE_H_ */
