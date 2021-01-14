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

#ifndef _CAM_SBI_HW_MGR_H_
#define _CAM_SBI_HW_MGR_H_

#include <linux/module.h>
#include <linux/kernel.h>

#include <cam_sbi.h>
#include "cam_hw.h"
#include "cam_hw_intf.h"
#include "cam_cpas_api.h"
#include "cam_debug_util.h"
#include "cam_hw_mgr_intf.h"
#include "cam_req_mgr_workq.h"
#include "cam_sbi_hw_intf.h"
#include "cam_context.h"
#include "cam_ife_csid_hw_intf.h"
#include "cam_isp_hw.h"
#include "cam_sbi_hw.h"

#define SBI_ENABLE_REAL_DD // when testing on REAL SBI device, enable this macro

#define CAM_SBI_HW_MAX 1
#define CAM_SBI_WORKQ_NUM_TASK 10

#define CAM_SBI_DECODE_DEVICE_INDEX(ctxt_to_hw_map) \
	((uintptr_t)ctxt_to_hw_map & 0xF)

#define CAM_SBI_DECODE_PRIORITY(ctxt_to_hw_map) \
	(((uintptr_t)ctxt_to_hw_map & 0xF0) >> 4)

#define CAM_SBI_DECODE_CTX_INDEX(ctxt_to_hw_map) \
	((uint64_t)(uintptr_t)ctxt_to_hw_map >> CAM_SBI_CTX_INDEX_SHIFT)

/* Needs to be suitably defined */
#define CAM_CUSTOM_HW_OUT_RES_MAX 1

/**
 * enum cam_sbi_hw_mgr_ctx_priority
 *
 * CAM_SBI_PRIORITY_HIGH   : High priority client
 * CAM_SBI_PRIORITY_NORMAL : Normal priority client
 */
enum cam_sbi_hw_mgr_ctx_priority {
	CAM_SBI_PRIORITY_HIGH,
	CAM_SBI_PRIORITY_NORMAL,
};

/**
 * struct cam_sbi_mgr_work_data : HW Mgr work data
 *
 * @hw_device                    : Pointer to the hw device
 */
struct cam_sbi_mgr_work_data {
	struct cam_sbi_device *hw_device;
};

/**
 * struct cam_sbi_debugfs_entry : debugfs entry struct
 *
 * @dentry                       : entry of debugfs
 * @dump_register                : flag to dump registers
 * @custom_csid_debug            : debugfs for custom csid
 */
struct cam_sbi_debugfs_entry {
	struct dentry   *dentry;
	bool             dump_register;
	uint32_t         custom_csid_debug;
};


/* enum cam_sbi_hw_mgr_res_type - manager resource node type */
enum cam_sbi_hw_mgr_res_type {
 	CAM_CUSTOM_HW_SUB_MODULE,
	CAM_CUSTOM_CID_HW,
	CAM_CUSTOM_CSID_HW,
	CAM_CUSTOM_HW_MAX,
};


/**
 * struct cam_sbi_hw_mgr_res- HW resources for the VFE manager
 *
 * @list:                used by the resource list
 * @res_type:            SBI manager resource type
 * @res_id:              resource id based on the resource type for root or
 *                       leaf resource, it matches the KMD interface port id.
 *                       For branch resrouce, it is defined by the ISP HW
 *                       layer
 * @hw_res:              hw layer resource array. For single VFE, only one VFE
 *                       hw resrouce will be acquired. For dual VFE, two hw
 *                       resources from different VFE HW device will be
 *                       acquired
 * @parent:              point to the parent resource node.
 * @children:            point to the children resource nodes
 * @child_num:           numbe of the child resource node.
 * @is_secure            informs whether the resource is in secure mode or not
 *
 */
struct cam_sbi_hw_mgr_res {
	struct list_head list;
	enum cam_sbi_hw_mgr_res_type res_type;
	uint32_t res_id;
	void *rsrc_node;
};


/**
 * struct ctx_base_info - Base hardware information for the context
 *
 * @idx:                 Base resource index
 *
 */
struct ctx_base_info {
	uint32_t                       idx;
	// enum cam_isp_hw_split_id       split_id;
};


/**
  * struct cam_sbi_hw_mgr_ctx - SBI HW manager Context object
  *
  * @list:                   used by the ctx list.
  * @common:                 common acquired context data
  * @ctx_index:              acquired context id.
  * @hw_mgr:                 sbi hw mgr which owns this context
  * @ctx_in_use:             flag to tell whether context is active
  * @res_list_custom_csid:   custom csid modules for this context
  * @res_list_custom_cid:    custom cid modules for this context
  * @init_done               indicate whether init hw is done
  * @is_fe_enable            indicate whether fetch engine\read path is enabled
  */
struct cam_sbi_hw_mgr_ctx {
    struct list_head                list;
 	struct cam_sbi_hw_mgr           *hw_mgr;
	uint32_t                        ctx_index;
 	uint32_t                        ctx_in_use;
	int32_t                         device_index;

	struct list_head                res_list_custom_csid;
	struct list_head                res_list_custom_cid;
 	struct cam_sbi_hw_mgr_res       sub_hw_list[CAM_CUSTOM_HW_RES_MAX];

 	struct list_head                free_res_list;
	struct cam_sbi_hw_mgr_res       res_pool[CAM_CUSTOM_HW_RES_MAX];
 	struct ctx_base_info            base[CAM_CUSTOM_HW_SUB_MOD_MAX];
 	uint32_t                        num_base;
 	bool                            init_done;
    bool                            is_fe_enable;
    cam_hw_event_cb_func            event_cb;
    void                           *cb_priv;

    struct cam_cmd_buf_desc         reg_dump_buf_desc[CAM_REG_DUMP_MAX_BUF_ENTRIES];
    uint32_t                        num_reg_dump_buf;
    bool                            is_ssm_recording_instance;
};



/**
 * struct cam_sbi_device     : SBI HW device
 *
 * @hw_caps                   : HW device's capabilities
 * @hw_intf                   : HW device's interface information
 * @num_context               : Number of contexts using this device
 * @valid                     : Whether this device is valid
 * @work                      : HW device's work queue
 * @work_data                 : HW device's work data
 * @frame_pending_list_high   : High priority request queue
 * @frame_pending_list_normal : Normal priority request queue
 * @high_req_lock             : Spinlock of high priority queue
 * @normal_req_lock           : Spinlock of normal priority queue
 */
struct cam_sbi_device {
	struct cam_sbi_dev_cap_info    hw_caps;
	struct cam_hw_intf             hw_intf;
	uint32_t                       num_context;
	bool                           valid;
	struct cam_req_mgr_core_workq *work;
	struct cam_sbi_mgr_work_data   work_data[CAM_SBI_WORKQ_NUM_TASK];
	struct list_head               frame_pending_list_high;
	struct list_head               frame_pending_list_normal;
	spinlock_t                     high_req_lock;
	spinlock_t                     normal_req_lock;
};

/**
 * struct cam_sbi_hw_mgr : SBI HW manager
 *
 * @device_count    : Number of HW devices
 * @frame_free_list : List of free frame request
 * @free_ctx_list   : free hw context list
 * @used_ctx_list   : used hw context list
 * @hw_mgr_mutex    : Mutex to protect HW manager data
 * @free_req_lock   :Spinlock to protect frame_free_list
 * @hw_device       : List of HW devices
 * @custom_hw		: custom sub hardwares
 * @device_iommu    : Device iommu
 * @cdm_iommu       : cdm iommu
 * @frame_req       : List of frame request to use
 * @sbi_caps       : SBI capabilities
 * @event_cb        : IRQ callback function
 * @debugfs_entry   : debugfs entry to set debug prop
 */
struct cam_sbi_hw_mgr {
	uint32_t                      device_count;
    int                           img_iommu_hdl;// SBI-CSID
    struct cam_sbi_hw_mgr_ctx     ctx_pool[CAM_CTX_MAX];// SBI-CSID
	struct list_head              frame_free_list;
    struct list_head              free_ctx_list;
	struct list_head              used_ctx_list;
	struct mutex                  ctx_mutex;
	spinlock_t                    free_req_lock;
	struct cam_sbi_device         hw_device[CAM_SBI_HW_MAX];
	struct cam_iommu_handle       device_iommu;
	struct cam_iommu_handle       cdm_iommu;

	// CSID
	struct cam_hw_intf *          custom_hw[CAM_CUSTOM_HW_SUB_MOD_MAX]; // not in use ; just sample
	struct cam_hw_intf *          csid_devices[CAM_CUSTOM_CSID_HW_MAX];

	struct cam_sbi_frame_request  frame_req[CAM_CTX_REQ_MAX * CAM_CTX_MAX];
	struct cam_sbi_query_cap_cmd  sbi_caps;
	cam_hw_event_cb_func          event_cb;
	struct cam_sbi_debugfs_entry  debugfs_entry;
};


/* Utility APIs */

/**
 * cam_custom_hw_mgr_get_custom_res_state()
 *
 * @brief:              Obtain equivalent custom rsrc state
 *                      from isp rsrc state
 *
 * @in_rsrc_state:      isp rsrc state
 *
 */
enum cam_custom_hw_resource_state
	cam_sbi_hw_mgr_get_custom_res_state(
	uint32_t in_rsrc_state);

/**
 * cam_custom_hw_mgr_get_isp_res_state()
 *
 * @brief:              Obtain equivalent isp rsrc state
 *                      from custom rsrc state
 *
 * @in_rsrc_state:      custom rsrc state
 *
 */
enum cam_isp_resource_state
	cam_sbi_hw_mgr_get_isp_res_state(
	uint32_t in_rsrc_state);

/**
 * cam_custom_hw_mgr_get_isp_res_type()
 *
 * @brief:         Obtain equivalent isp rsrc type
 *                 from custom rsrc type
 *
 * @res_type:      custom rsrc type
 *
 */
enum cam_isp_resource_type
	cam_sbi_hw_mgr_get_isp_res_type(
	enum cam_sbi_hw_mgr_res_type res_type);

int cam_sbi_mgr_register_device(struct cam_hw_intf *sbi_hw_intf,
	struct cam_iommu_handle *device_iommu
	// ,struct cam_iommu_handle *cdm_iommu
	);
int cam_sbi_mgr_deregister_device(int device_index);

#endif /* _CAM_SBI_HW_MGR_H_ */
