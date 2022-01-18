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

#include <linux/module.h>
#include <linux/kernel.h>
#include <media/cam_cpas.h>
#include <media/cam_req_mgr.h>

#include "cam_io_util.h"
#include "cam_soc_util.h"
#include "cam_mem_mgr_api.h"
#include "cam_req_mgr_dev.h"
#include "cam_smmu_api.h"
#include "cam_packet_util.h"
#include "cam_sbi_context.h"
#include "cam_sbi_hw_intf.h"
#include "cam_sbi_hw_core.h"
#include "cam_sbi_hw_soc.h"
#include "cam_sbi_hw_mgr_intf.h"
#include "cam_sbi_hw_mgr.h"


static struct cam_sbi_hw_mgr g_sbi_hw_mgr;

enum cam_sbi_reg_cmd {
	cam_sbi_reg_cmd_init,
	cam_sbi_reg_cmd_base_addr_upd_scenario_1,
	cam_sbi_reg_cmd_base_addr_upd_scenario_4,
	cam_sbi_reg_cmd_task2_setup,
	cam_sbi_reg_cmd_task2_action,
	cam_sbi_req_cmd_update_ssm_info,
	cam_sbi_req_cmd_ssm_record_start
};

static int
cam_sbi_hw_mgr_release_hw_for_ctx(struct cam_sbi_hw_mgr_ctx * hw_mgr_ctx);
static int cam_sbi_hw_mgr_put_ctx(struct list_head *src_list,
	struct cam_sbi_hw_mgr_ctx ** hw_mgr_ctx);
static int32_t cam_sbi_hw_mgr_dump_specific_reg(void *hw_mgr_priv, int param);
static int32_t cam_sbi_hw_mgr_invoke_writereg_cmd(void *hw_mgr_priv,
	void *val,
	int32_t write_reg_cmd);

static int32_t cam_sbi_hw_mgr_init_register_set(void *hw_mgr_priv,
	void *cmd_args, uint32_t args_size, int32_t write_reg_cmd);


static int cam_sbi_mgr_util_get_device(struct cam_sbi_hw_mgr *hw_mgr,
	uint32_t device_index,
	struct cam_sbi_device **hw_device)
{
	if (!hw_mgr) {
		CAM_ERR(CAM_SBI, "invalid params hw_mgr %pK", hw_mgr);
		return -EINVAL;
	}

	if (device_index >= CAM_SBI_HW_MAX) {
		CAM_ERR(CAM_SBI, "Wrong device index %d", device_index);
		return -EINVAL;
	}

	*hw_device = &hw_mgr->hw_device[device_index];

	return 0;
}

#if 0
static int cam_sbi_mgr_util_packet_validate(struct cam_packet *packet,
	size_t remain_len)
{
	struct cam_cmd_buf_desc *cmd_desc = NULL;
	int i, rc;

	if (!packet) {
		CAM_ERR(CAM_SBI, "Invalid args");
		return -EINVAL;
	}

	CAM_DBG(CAM_SBI, "Packet request=%d, op_code=0x%x, size=%d, flags=%d",
		packet->header.request_id, packet->header.op_code,
		packet->header.size, packet->header.flags);
	CAM_DBG(CAM_SBI,
		"Packet cmdbuf(offset=%d, num=%d) io(offset=%d, num=%d)",
		packet->cmd_buf_offset, packet->num_cmd_buf,
		packet->io_configs_offset, packet->num_io_configs);
	CAM_DBG(CAM_SBI,
		"Packet Patch(offset=%d, num=%d) kmd(offset=%d, num=%d)",
		packet->patch_offset, packet->num_patches,
		packet->kmd_cmd_buf_offset, packet->kmd_cmd_buf_index);

	if (cam_packet_util_validate_packet(packet, remain_len)) {
		CAM_ERR(CAM_SBI, "invalid packet:%d %d %d %d %d",
			packet->kmd_cmd_buf_index, packet->num_cmd_buf,
			packet->cmd_buf_offset, packet->io_configs_offset,
			packet->header.size);
		return -EINVAL;
	}

	if (!packet->num_io_configs) {
		//CAM_ERR(CAM_SBI, "no io configs");
		//return -EINVAL;
		return 0;
	}

	cmd_desc = (struct cam_cmd_buf_desc *)((uint8_t *)&packet->payload +
		packet->cmd_buf_offset);

	for (i = 0; i < packet->num_cmd_buf; i++) {
		if (!cmd_desc[i].length)
			continue;

		CAM_DBG(CAM_SBI,
			"CmdBuf[%d] hdl=%d, offset=%d, size=%d, len=%d, type=%d, meta_data=%d",
			i, cmd_desc[i].mem_handle, cmd_desc[i].offset,
			cmd_desc[i].size, cmd_desc[i].length, cmd_desc[i].type,
			cmd_desc[i].meta_data);

		rc = cam_packet_util_validate_cmd_desc(&cmd_desc[i]);
		if (rc) {
			CAM_ERR(CAM_SBI, "Invalid cmd buffer %d", i);
			return rc;
		}
	}

	return 0;
}

static int cam_sbi_mgr_util_prepare_io_buffer(
	int32_t iommu_hdl, struct cam_hw_prepare_update_args *prepare,
	struct cam_sbi_hw_io_buffer *input_buf,
	struct cam_sbi_hw_io_buffer *output_buf, uint32_t io_buf_size)
{
	int rc = -EINVAL;
	uint32_t num_in_buf, num_out_buf, i, j, plane;
	struct cam_buf_io_cfg *io_cfg;
	uint64_t io_addr[CAM_PACKET_MAX_PLANES];
	size_t size;

	num_in_buf = 0;
	num_out_buf = 0;
	io_cfg =
		(struct cam_buf_io_cfg *)((uint8_t *)&prepare->packet->payload +
			prepare->packet->io_configs_offset);

	for (i = 0; i < prepare->packet->num_io_configs; i++) {
		CAM_DBG(CAM_SBI,
			"IOConfig[%d] : handle[%d] Dir[%d] Res[%d] Fence[%d], Format[%d]",
			i, io_cfg[i].mem_handle[0], io_cfg[i].direction,
			io_cfg[i].resource_type, io_cfg[i].fence,
			io_cfg[i].format);

		if ((num_in_buf > io_buf_size) || (num_out_buf > io_buf_size)) {
			CAM_ERR(CAM_SBI, "Invalid number of buffers %d %d %d",
				num_in_buf, num_out_buf, io_buf_size);
			return -EINVAL;
		}

		memset(io_addr, 0, sizeof(io_addr));
		for (plane = 0; plane < CAM_PACKET_MAX_PLANES; plane++) {
			if (!io_cfg[i].mem_handle[plane])
				break;

			rc = cam_mem_get_io_buf(io_cfg[i].mem_handle[plane],
				iommu_hdl, &io_addr[plane],
				&size);
			if (rc) {
				CAM_ERR(CAM_SBI, "Cannot get io buf for %d %d",
					plane, rc);
				return -ENOMEM;
			}

			io_addr[plane] += io_cfg[i].offsets[plane];

			CAM_DBG(CAM_SBI, "IO Address[%d][%d] : %llu",
				io_cfg[i].direction, plane, io_addr[plane]);
		}

		switch (io_cfg[i].direction) {
		case CAM_BUF_INPUT: {
			prepare->in_map_entries[num_in_buf].resource_handle =
				io_cfg[i].resource_type;
			prepare->in_map_entries[num_in_buf].sync_id =
				io_cfg[i].fence;

			input_buf[num_in_buf].valid = true;
			for (j = 0; j < plane; j++)
				input_buf[num_in_buf].io_addr[j] = io_addr[j];
			input_buf[num_in_buf].num_plane = plane;
			input_buf[num_in_buf].io_cfg = &io_cfg[i];

			num_in_buf++;
			break;
		}
		case CAM_BUF_OUTPUT: {
			prepare->out_map_entries[num_out_buf].resource_handle =
				io_cfg[i].resource_type;
			prepare->out_map_entries[num_out_buf].sync_id =
				io_cfg[i].fence;

			output_buf[num_out_buf].valid = true;
			for (j = 0; j < plane; j++)
				output_buf[num_out_buf].io_addr[j] = io_addr[j];
			output_buf[num_out_buf].num_plane = plane;
			output_buf[num_out_buf].io_cfg = &io_cfg[i];

			num_out_buf++;
			break;
		}
		default:
			CAM_ERR(CAM_SBI, "Unsupported io direction %d",
				io_cfg[i].direction);
			return -EINVAL;
		}
	}
	prepare->num_in_map_entries = num_in_buf;
	prepare->num_out_map_entries = num_out_buf;

	return 0;
}

static int cam_sbi_mgr_util_prepare_hw_update_entries(
	struct cam_sbi_hw_mgr *hw_mgr,
	struct cam_hw_prepare_update_args *prepare,
	struct cam_sbi_hw_cmd_config_args *config_args,
	struct cam_kmd_buf_info *kmd_buf_info)
{
	int i, rc = 0;
	struct cam_sbi_device *hw_device = NULL;
	uint32_t *kmd_buf_addr;
	uint32_t num_entry;
	uint32_t kmd_buf_max_size;
	uint32_t kmd_buf_used_bytes = 0;
	struct cam_hw_update_entry *hw_entry;
	struct cam_cmd_buf_desc *cmd_desc = NULL;

	hw_device = config_args->hw_device;
	if (!hw_device) {
		CAM_ERR(CAM_SBI, "Invalid hw_device");
		return -EINVAL;
	}

	CAM_DBG(CAM_SBI, "andy, %s e", __FUNCTION__);

	kmd_buf_addr = (uint32_t *)((uint8_t *)kmd_buf_info->cpu_addr +
		kmd_buf_info->used_bytes);
	kmd_buf_max_size = kmd_buf_info->size - kmd_buf_info->used_bytes;

	config_args->cmd_buf_addr = kmd_buf_addr;
	config_args->size = kmd_buf_max_size;
	config_args->config_buf_size = 0;

	if (hw_device->hw_intf.hw_ops.process_cmd) {
		rc = hw_device->hw_intf.hw_ops.process_cmd(
			hw_device->hw_intf.hw_priv,
			CAM_SBI_HW_CMD_PREPARE_HW_UPDATE, config_args,
			sizeof(struct cam_sbi_hw_cmd_config_args));
		if (rc) {
			CAM_ERR(CAM_SBI, "Failed in CMD_PREPARE_HW_UPDATE %d",
				rc);
			return rc;
		}
	}
	else {
		CAM_ERR(CAM_SBI, "Can't find handle function");
		return -EINVAL;
		//return rc;
	}

	if (0)
	{
		kmd_buf_used_bytes += config_args->config_buf_size;

		if (!kmd_buf_used_bytes || (kmd_buf_used_bytes > kmd_buf_max_size)) {
			CAM_ERR(CAM_SBI, "Invalid kmd used bytes %d (%d)",
				kmd_buf_used_bytes, kmd_buf_max_size);
			return -ENOMEM;
		}

		hw_entry = prepare->hw_update_entries;
		num_entry = 0;

		if (config_args->config_buf_size) {
			if ((num_entry + 1) >= prepare->max_hw_update_entries) {
				CAM_ERR(CAM_SBI, "Insufficient	HW entries :%d %d",
					num_entry, prepare->max_hw_update_entries);
				return -EINVAL;
			}

			hw_entry[num_entry].handle = kmd_buf_info->handle;
			hw_entry[num_entry].len = config_args->config_buf_size;
			hw_entry[num_entry].offset = kmd_buf_info->offset;

			kmd_buf_info->used_bytes += config_args->config_buf_size;
			kmd_buf_info->offset += config_args->config_buf_size;
			num_entry++;
		}
	}

	cmd_desc = (struct cam_cmd_buf_desc *)((uint8_t *)&prepare->packet
		->payload +
		prepare->packet->cmd_buf_offset);

	//CAM_ERR(CAM_SBI, "[SBI_TEST] before num_entry : %d", num_entry);

	for (i = 0; i < prepare->packet->num_cmd_buf; i++) {
		if (!cmd_desc[i].length)
			continue;

		if ((num_entry + 1) >= prepare->max_hw_update_entries) {
			CAM_ERR(CAM_SBI, "Exceed max num of entry");
			return -EINVAL;
		}

		hw_entry[num_entry].handle = cmd_desc[i].mem_handle;
		hw_entry[num_entry].len = cmd_desc[i].length;
		hw_entry[num_entry].offset = cmd_desc[i].offset;
		num_entry++;
	}

	//CAM_ERR(CAM_SBI, "[SBI_TEST] after num_entry : %d", num_entry);

	prepare->num_hw_update_entries = num_entry;

	CAM_DBG(CAM_SBI, "FinalConfig : hw_entries=%d, Sync(in=%d, out=%d)",
		prepare->num_hw_update_entries, prepare->num_in_map_entries,
		prepare->num_out_map_entries);

	return rc;
}
#endif

static void cam_sbi_mgr_util_put_frame_req(struct list_head *src_list,
	struct list_head *list,
	spinlock_t *lock)
{
	spin_lock(lock);
	list_add_tail(list, src_list);
	spin_unlock(lock);
}

static int
cam_sbi_mgr_util_get_frame_req(struct list_head *src_list,
	struct cam_sbi_frame_request **frame_req,
	spinlock_t *lock)
{
	int rc = 0;
	struct cam_sbi_frame_request *req_ptr = NULL;

	spin_lock(lock);
	if (!list_empty(src_list)) {
		req_ptr = list_first_entry(
			src_list, struct cam_sbi_frame_request, frame_list);
		list_del_init(&req_ptr->frame_list);
	}
	else {
		rc = -ENOENT;
	}
	*frame_req = req_ptr;
	spin_unlock(lock);

	return rc;
}

static int cam_sbi_mgr_util_submit_req(void *priv, void *data)
{
	struct cam_sbi_device *hw_device;
	struct cam_sbi_hw_mgr *hw_mgr;
	struct cam_sbi_frame_request *frame_req = NULL;
	struct cam_sbi_hw_submit_args submit_args;
	struct cam_sbi_mgr_work_data *work_data;
	int rc;
	int req_prio = 0;

	if (!priv) {
		CAM_ERR(CAM_SBI, "worker doesn't have private data");
		return -EINVAL;
	}

	hw_mgr = (struct cam_sbi_hw_mgr *)priv;
	work_data = (struct cam_sbi_mgr_work_data *)data;
	hw_device = work_data->hw_device;

	rc = cam_sbi_mgr_util_get_frame_req(&hw_device->frame_pending_list_high,
		&frame_req,
		&hw_device->high_req_lock);

	if (!frame_req) {
		rc = cam_sbi_mgr_util_get_frame_req(
			&hw_device->frame_pending_list_normal, &frame_req,
			&hw_device->normal_req_lock);
		if (frame_req)
			req_prio = 1;
	}

	if (!frame_req) {
		CAM_DBG(CAM_SBI, "No pending request");
		return 0;
	}

	if (hw_device->hw_intf.hw_ops.process_cmd) {
		submit_args.hw_update_entries = frame_req->hw_update_entries;
		submit_args.num_hw_update_entries =
			frame_req->num_hw_update_entries;
		submit_args.frame_req = frame_req;

		rc = hw_device->hw_intf.hw_ops.process_cmd(
			hw_device->hw_intf.hw_priv, CAM_SBI_HW_CMD_SUBMIT,
			&submit_args, sizeof(struct cam_sbi_hw_submit_args));

		if (rc == -EBUSY)
			CAM_DBG(CAM_SBI, "device busy");
		else if (rc)
			CAM_ERR(CAM_SBI, "submit request failed rc %d", rc);
		if (rc) {
			req_prio == 0 ? spin_lock(&hw_device->high_req_lock) :
				spin_lock(&hw_device->normal_req_lock);
			list_add(
				&frame_req->frame_list,
				(req_prio == 0 ?
					&hw_device->frame_pending_list_high :
					&hw_device->frame_pending_list_normal));
			req_prio == 0 ?
				spin_unlock(&hw_device->high_req_lock) :
				spin_unlock(&hw_device->normal_req_lock);
		}
		if (rc == -EBUSY)
			rc = 0;
	}
	else {
		req_prio == 0 ? spin_lock(&hw_device->high_req_lock) :
			spin_lock(&hw_device->normal_req_lock);
		list_add(&frame_req->frame_list,
			(req_prio == 0 ?
				&hw_device->frame_pending_list_high :
				&hw_device->frame_pending_list_normal));
		req_prio == 0 ? spin_unlock(&hw_device->high_req_lock) :
			spin_unlock(&hw_device->normal_req_lock);
		rc = -EINVAL;
	}

	CAM_DBG(CAM_SBI, "End of submit, rc %d", rc);

	return rc;
}

static int cam_sbi_mgr_util_schedule_frame_req(struct cam_sbi_hw_mgr *hw_mgr,
	struct cam_sbi_device *hw_device)
{
	int rc = 0;
	struct crm_workq_task *task;
	struct cam_sbi_mgr_work_data *work_data;

	task = cam_req_mgr_workq_get_task(hw_device->work);
	if (!task) {
		CAM_ERR(CAM_SBI, "Can not get task for worker");
		return -ENOMEM;
	}

	work_data = (struct cam_sbi_mgr_work_data *)task->payload;
	work_data->hw_device = hw_device;

	task->process_cb = cam_sbi_mgr_util_submit_req;
	CAM_DBG(CAM_SBI, "enqueue submit task");
	rc = cam_req_mgr_workq_enqueue_task(task, hw_mgr, CRM_TASK_PRIORITY_0);

	return rc;
}

static int cam_sbi_mgr_cb(void *data, struct cam_sbi_hw_cb_args *cb_args)
{
	struct cam_sbi_hw_mgr *hw_mgr = &g_sbi_hw_mgr;
	int rc = 0;
	bool frame_abort = true;
	struct cam_sbi_frame_request *frame_req;
	struct cam_sbi_device *hw_device;

	if (!data || !cb_args) {
		CAM_ERR(CAM_SBI, "Invalid input args");
		return -EINVAL;
	}

	hw_device = (struct cam_sbi_device *)data;
	frame_req = cb_args->frame_req;

	if (cb_args->cb_type & CAM_SBI_CB_PUT_FRAME) {
		memset(frame_req, 0x0, sizeof(*frame_req));
		INIT_LIST_HEAD(&frame_req->frame_list);
		cam_sbi_mgr_util_put_frame_req(&hw_mgr->frame_free_list,
			&frame_req->frame_list,
			&hw_mgr->free_req_lock);
		cb_args->cb_type &= ~CAM_SBI_CB_PUT_FRAME;
		frame_req = NULL;
	}

	if (cb_args->cb_type & CAM_SBI_CB_COMP_REG_UPDATE) {
		cb_args->cb_type &= ~CAM_SBI_CB_COMP_REG_UPDATE;
		CAM_DBG(CAM_SBI, "Reg update");
	}

	if (!frame_req)
		return rc;

	if (cb_args->cb_type & CAM_SBI_CB_BUF_DONE) {
		cb_args->cb_type &= ~CAM_SBI_CB_BUF_DONE;
		frame_abort = false;
	}
	else if (cb_args->cb_type & CAM_SBI_CB_ERROR) {
		cb_args->cb_type &= ~CAM_SBI_CB_ERROR;
		frame_abort = true;
	}
	else {
		CAM_ERR(CAM_SBI, "Wrong cb type %d, req %lld", cb_args->cb_type,
			frame_req->req_id);
		return -EINVAL;
	}

	if (hw_mgr->event_cb) {
		struct cam_hw_done_event_data buf_data;

		buf_data.request_id = frame_req->req_id;
		CAM_DBG(CAM_SBI, "frame req %llu, frame_abort %d",
			frame_req->req_id, frame_abort);
		rc = hw_mgr->event_cb(frame_req->ctxt_to_hw_map, frame_abort,
			&buf_data);
	}
	else {
		CAM_ERR(CAM_SBI, "No cb function");
	}
	memset(frame_req, 0x0, sizeof(*frame_req));
	INIT_LIST_HEAD(&frame_req->frame_list);
	cam_sbi_mgr_util_put_frame_req(&hw_mgr->frame_free_list,
		&frame_req->frame_list,
		&hw_mgr->free_req_lock);

	rc = cam_sbi_mgr_util_schedule_frame_req(hw_mgr, hw_device);

	return rc;
}

static int cam_sbi_mgr_get_caps(void *hw_mgr_priv, void *hw_get_caps_args)
{
	int rc = 0;
	struct cam_sbi_hw_mgr *hw_mgr = hw_mgr_priv;
	struct cam_query_cap_cmd *query = hw_get_caps_args;
	struct cam_sbi_query_cap_cmd custom_hw_cap;

	if (copy_from_user(&custom_hw_cap, u64_to_user_ptr(query->caps_handle),
		sizeof(struct cam_sbi_query_cap_cmd))) {
		CAM_ERR(CAM_SBI, "Failed while copying query caps");
		rc = -EFAULT;
		return rc;
	}

	custom_hw_cap.device_iommu.non_secure = hw_mgr->img_iommu_hdl;
	custom_hw_cap.device_iommu.secure = -1;

	/* Initializing cdm handles to -1 */
	custom_hw_cap.cdm_iommu.non_secure = -1;
	custom_hw_cap.cdm_iommu.secure = -1;

	custom_hw_cap.num_dev = 1;
	custom_hw_cap.dev_caps[0].hw_type = 0;
	custom_hw_cap.dev_caps[0].hw_version = 0;

	if (copy_to_user(u64_to_user_ptr(query->caps_handle), &custom_hw_cap,
		sizeof(struct cam_sbi_query_cap_cmd))) {
		CAM_ERR(CAM_SBI, "copy to user failed");
		return -EFAULT;
	}

	CAM_DBG(CAM_SBI, "exit rc :%d handle %d", rc, hw_mgr->img_iommu_hdl);
	return rc;
}

static int cam_sbi_hw_mgr_acquire_get_unified_dev_str(
	struct cam_custom_acquire_hw_info *acquire_hw_info,
	uint32_t *input_size,
	struct cam_isp_in_port_generic_info **gen_port_info)
{
	int32_t rc = 0, i;
	uint32_t in_port_length = 0;
	struct cam_sbi_in_port_info *in = NULL;
	struct cam_isp_in_port_generic_info *port_info = NULL;

	in = (struct cam_sbi_in_port_info *)
		((uint8_t *)&acquire_hw_info->data +
		 acquire_hw_info->input_info_offset + *input_size);

	in_port_length = sizeof(struct cam_sbi_in_port_info) +
		(in->num_out_res - 1) *
		sizeof(struct cam_sbi_out_port_info);

	*input_size += in_port_length;

	if ((*input_size) > acquire_hw_info->input_info_size) {
		CAM_ERR(CAM_SBI, "Input is not proper");
		rc = -EINVAL;
		return rc;
	}

	port_info = kzalloc(
		sizeof(struct cam_isp_in_port_generic_info), GFP_KERNEL);

	if (!port_info)
		return -ENOMEM;


	port_info->res_type        =  in->res_type + CAM_ISP_IFE_IN_RES_BASE - CAM_CUSTOM_IN_RES_BASE;
	port_info->lane_type       =  in->lane_type;
	port_info->lane_num        =  in->lane_num;
	port_info->lane_cfg        =  in->lane_cfg;
	port_info->vc[0]           =  in->vc[0];
	port_info->dt[0]           =  in->dt[0];
	port_info->num_valid_vc_dt =  in->num_valid_vc_dt;
	port_info->format          =  in->format;
	port_info->test_pattern    =  in->test_pattern;
	port_info->usage_type      =  in->usage_type;
	port_info->left_start      =  in->left_start;
	port_info->left_stop       =  in->left_stop;
	port_info->left_width      =  in->left_width;
	port_info->right_start     =  in->right_start;
	port_info->right_stop      =  in->right_stop;
	port_info->right_width     =  in->right_width;
	port_info->line_start      =  in->line_start;
	port_info->line_stop       =  in->line_stop;
	port_info->height          =  in->height;
	port_info->pixel_clk       =  in->pixel_clk;
	port_info->cust_node       =  1;
	port_info->num_out_res     =  in->num_out_res;
	port_info->num_bytes_out   =  in->num_bytes_out;

	port_info->data = kcalloc(in->num_out_res,
		sizeof(struct cam_isp_out_port_generic_info),
		GFP_KERNEL);
	if (port_info->data == NULL) {
		rc = -ENOMEM;
		goto release_port_mem;
	}

	CAM_DBG(CAM_SBI, "-------  %s  -------", __FUNCTION__);
	CAM_DBG(CAM_SBI, "res_type : %d", port_info->res_type);
	CAM_DBG(CAM_SBI, "lane_type : %d", port_info->lane_type);
	CAM_DBG(CAM_SBI, "lane_num : %d", port_info->lane_num);
	CAM_DBG(CAM_SBI, "lane_cfg : %d", port_info->lane_cfg);
	CAM_DBG(CAM_SBI, "vc[0] : %d", port_info->vc[0]);
	CAM_DBG(CAM_SBI, "dt[0] : %d", port_info->dt[0]);
	CAM_DBG(CAM_SBI, "num_valid_vc_dt : %d", port_info->num_valid_vc_dt);
	CAM_DBG(CAM_SBI, "format : %d", port_info->format);
	CAM_DBG(CAM_SBI, "test_pattern : %d", port_info->test_pattern);
	CAM_DBG(CAM_SBI, "usage_type : %d", port_info->usage_type);
	CAM_DBG(CAM_SBI, "left_start : %d", port_info->left_start);
	CAM_DBG(CAM_SBI, "left_stop : %d", port_info->left_stop);
	CAM_DBG(CAM_SBI, "left_width : %d", port_info->left_width);
	CAM_DBG(CAM_SBI, "right_start : %d", port_info->right_start);
	CAM_DBG(CAM_SBI, "right_width : %d", port_info->right_width);
	CAM_DBG(CAM_SBI, "line_start : %d", port_info->line_start);
	CAM_DBG(CAM_SBI, "line_stop : %d", port_info->line_stop);
	CAM_DBG(CAM_SBI, "height : %d", port_info->height);


	for (i = 0; i < in->num_out_res; i++) {
		port_info->data[i].res_type     = in->data[i].res_type;
		port_info->data[i].format       = in->data[i].format;
	}

	*gen_port_info = port_info;
	return 0;

release_port_mem:
	kfree(port_info);
	return rc;
}

static int cam_sbi_hw_mgr_release(void *hw_mgr_priv, void *hw_release_args)
{
	int rc = 0;
	struct cam_hw_release_args *release_args =
		(struct cam_hw_release_args *)hw_release_args;
	struct cam_sbi_hw_mgr_ctx *custom_ctx;

	if (!hw_mgr_priv || !hw_release_args) {
		CAM_ERR(CAM_SBI, "Invalid arguments %pK, %pK", hw_mgr_priv,
			hw_release_args);
		return -EINVAL;
	}

	custom_ctx =
		(struct cam_sbi_hw_mgr_ctx *)release_args->ctxt_to_hw_map;
	if (!custom_ctx || !custom_ctx->ctx_in_use) {
		CAM_ERR(CAM_SBI, "Invalid context is used");
		return -EPERM;
	}

	CAM_DBG(CAM_SBI, "Enter..ctx_index:%d", custom_ctx->ctx_index);

	cam_sbi_hw_mgr_release_hw_for_ctx(custom_ctx);
	list_del_init(&custom_ctx->list);
	custom_ctx->ctx_in_use = 0;
	cam_sbi_hw_mgr_put_ctx(&g_sbi_hw_mgr.free_ctx_list, &custom_ctx);
	CAM_DBG(CAM_SBI, "Release Exit..");

	return rc;
}

static int
cam_sbi_hw_mgr_init_hw_res(struct cam_sbi_hw_mgr_res *hw_mgr_res)
{
	int rc = -1;
	struct cam_isp_resource_node *custom_rsrc_node = NULL;
	struct cam_hw_intf *hw_intf = NULL;

	custom_rsrc_node =
		(struct cam_isp_resource_node *)hw_mgr_res->rsrc_node;
	if (!custom_rsrc_node) {
		CAM_ERR(CAM_SBI, "Invalid args");
		return -EINVAL;
	}

	hw_intf = custom_rsrc_node->hw_intf;
	if (hw_intf->hw_ops.init) {
		CAM_DBG(CAM_SBI, "INIT HW for res_id:%u", hw_mgr_res->res_id);
		//msleep(10);
		rc = hw_intf->hw_ops.init(hw_intf->hw_priv, custom_rsrc_node,
			sizeof(struct cam_isp_resource_node));
		if (rc)
			goto err;
	}

	return 0;

err:
	CAM_DBG(CAM_SBI, "INIT HW failed for res_id:%u", hw_mgr_res->res_id);
	return rc;
}

static int
cam_sbi_hw_mgr_start_hw_res(struct cam_sbi_hw_mgr_res *hw_mgr_res)
{
	int rc = -1;
	struct cam_isp_resource_node *isp_rsrc_node = NULL;
	struct cam_hw_intf *hw_intf = NULL;

	isp_rsrc_node = (struct cam_isp_resource_node *)hw_mgr_res->rsrc_node;
	if (!isp_rsrc_node) {
		CAM_ERR(CAM_SBI, "Invalid args");
		return -EINVAL;
	}

	hw_intf = isp_rsrc_node->hw_intf;
	if (hw_intf->hw_ops.start) {
		CAM_DBG(CAM_SBI, "Start HW for res_id:%u", hw_mgr_res->res_id);
		rc = hw_intf->hw_ops.start(
			hw_intf->hw_priv, isp_rsrc_node,
			sizeof(struct cam_isp_resource_node));
		if (rc)
			goto err;
	}

	return 0;

err:
	CAM_DBG(CAM_SBI, "START HW failed for res_id:%u", hw_mgr_res->res_id);
	return rc;
}

static int cam_sbi_hw_mgr_start(void *hw_mgr_priv, void *hw_start_args)
{
	int rc = 0, i;
	struct cam_sbi_hw_mgr *hw_mgr = hw_mgr_priv;
	struct cam_sbi_start_args *sbi_args =
		(struct cam_sbi_start_args *)hw_start_args;
	struct cam_hw_start_args *args;
	struct cam_sbi_hw_mgr_res *hw_mgr_res;
	struct cam_sbi_hw_mgr_ctx * ctx_manager;
	struct cam_sbi_device *hw_device;
	uint32_t device_index;

	uint32_t *ptr;
	int32_t scratch_hdl;
	uint64_t scratch_iova;
	size_t len, scratch_mem_len;
	struct cam_cmd_buf_desc *cmd_desc = NULL;
	enum cam_sbi_reg_cmd sbi_cmd;
	struct cam_sbi_cmd_buf_type_init * cmd;

	if (!hw_mgr || !sbi_args) {
		CAM_ERR(CAM_SBI, "Invalid input params");
		return -EINVAL;
	}

	args = (struct cam_hw_start_args *) &sbi_args->hw_config;
	ctx_manager = (struct cam_sbi_hw_mgr_ctx *)args->ctxt_to_hw_map;
	if (!ctx_manager || !ctx_manager->ctx_in_use) {
		if (ctx_manager == NULL)
			CAM_ERR(CAM_SBI, "Invalid context is used. ctx_manager is NULL");
		else
			CAM_ERR(CAM_SBI, "Invalid context is used. ctx_manager->ctx_in_use = %d", ctx_manager->ctx_in_use);
		return -EPERM;
	}

	device_index = ctx_manager->device_index;
	if (device_index >= hw_mgr->device_count) {
		CAM_ERR(CAM_SBI, "Invalid device index %d device count %d",
			device_index, hw_mgr->device_count);
		return -EPERM;
	}

	CAM_DBG(CAM_SBI, "Start device index %d", device_index);

	rc = cam_sbi_mgr_util_get_device(hw_mgr, device_index, &hw_device);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed to get hw device");
		return rc;
	}

	if (sbi_args->start_only)
		goto start_only;

	/* set current csid debug information to CUSTOM CSID HW */
	for (i = 0; i < CAM_CUSTOM_CSID_HW_MAX; i++) {
		if (g_sbi_hw_mgr.csid_devices[i])
			g_sbi_hw_mgr.csid_devices[i]->hw_ops.process_cmd(
				g_sbi_hw_mgr.csid_devices[i]->hw_priv,
				CAM_IFE_CSID_SET_CSID_DEBUG,
				&g_sbi_hw_mgr.debugfs_entry.custom_csid_debug,
				sizeof(g_sbi_hw_mgr.debugfs_entry.custom_csid_debug));
	}

	/* Init custom cid */
	list_for_each_entry(hw_mgr_res, &ctx_manager->res_list_custom_cid, list) {
		rc = cam_sbi_hw_mgr_init_hw_res(hw_mgr_res);
		if (rc) {
			CAM_ERR(CAM_SBI, "Can not INIT CID(id :%d)",
				hw_mgr_res->res_id);
			goto deinit_hw;
		}
	}

	/* Init custom csid */
	list_for_each_entry(hw_mgr_res, &ctx_manager->res_list_custom_csid, list) {
		rc = cam_sbi_hw_mgr_init_hw_res(hw_mgr_res);
		if (rc) {
			CAM_ERR(CAM_SBI, "Can not INIT CSID(id :%d)",
				hw_mgr_res->res_id);
			goto deinit_hw;
		}
	}

start_only:
	/* Check SSM record node */
	cmd_desc = (struct cam_cmd_buf_desc *)(ctx_manager->reg_dump_buf_desc);
	rc = cam_packet_util_get_cmd_mem_addr(cmd_desc->mem_handle, &ptr, &len);
	ptr += (cmd_desc->offset / 4);
	cmd = (struct cam_sbi_cmd_buf_type_init *)ptr;

	/* Set clock rate */
	{
		struct cam_hw_info *sbi_hw;
		struct cam_sbi_core *sbi_core;

		sbi_hw   = (struct cam_hw_info *)hw_device->hw_intf.hw_priv;
		sbi_core = sbi_hw->core_info;

		sbi_core->clock_rate = (uint64_t)cmd->clock_rate;
	}

	/* Start custom HW first */
	if (hw_device->hw_intf.hw_ops.start) {
		rc = hw_device->hw_intf.hw_ops.start(hw_device->hw_intf.hw_priv,
				&ctx_manager->is_ssm_recording_instance, sizeof(bool));
	}
	else {
		CAM_ERR(CAM_SBI, "Invalid start function");
		return -EINVAL;
	}

	if (!ctx_manager->is_ssm_recording_instance)
	{
		scratch_hdl = cmd->scratch_buf_hdl;
		rc = cam_mem_get_io_buf(scratch_hdl,
					hw_mgr->img_iommu_hdl,
					&scratch_iova,
					&scratch_mem_len);
		if (rc) {
			CAM_INFO(CAM_SBI, "failed to get iova for scratch_hdl 0x%x rc %d",
				scratch_hdl, rc);
			goto err;
		}

		{
			void *args = (void *)&cmd->register_set;
			uint32_t args_size = cmd->register_set_size;
			CAM_DBG(CAM_SBI, "1. sbi addr upd %llx", scratch_iova);

			/* Init custom hw here */
			sbi_cmd = cam_sbi_reg_cmd_init;

			cam_sbi_hw_mgr_init_register_set(hw_mgr, args, args_size, sbi_cmd);

			if (cmd->is_ssm) {
				struct cam_hw_info *sbi_hw;
				struct cam_hw_soc_info *soc_info;
				struct cam_sbi_soc_private *soc_private;
				cam_sbi_ssm_info ssm_info;

				sbi_hw = (struct cam_hw_info *)hw_device->hw_intf.hw_priv;
				soc_info = &sbi_hw->soc_info;
				soc_private = soc_info->soc_private;
				cam_soc_util_clk_enable(soc_private->task3_clk, CAM_TASK3_CLK_NAME, soc_private->task3_clk_rate);
				CAM_INFO(CAM_SBI, "cam_cc_sbi_ife_1_clk enable");

				sbi_cmd = cam_sbi_reg_cmd_base_addr_upd_scenario_4;
				ssm_info.addr_lr = scratch_iova;
				ssm_info.addr_hr = scratch_iova + cmd->preview_buffer_len;
				ssm_info.preview_len = cmd->preview_buffer_len;
				ssm_info.record_len = cmd->record_buffer_len;
				ssm_info.sensor_width = cmd->sensor_width;
				ssm_info.sensor_height = cmd->sensor_height;
				ssm_info.batch_num = cmd->batch_num;
				ssm_info.frame_size = cmd->frame_size;
				ssm_info.batch_size = cmd->frame_size * ssm_info.batch_num;
				ssm_info.record_len = cmd->record_buffer_len;
				ssm_info.max_frames = cmd->max_frames + (CAM_SBI_ADDR_FIFO_Q_NUM * cmd->batch_num);
				ssm_info.cue_option = cmd->cue_option;
				ssm_info.ssm_framerate = cmd->ssm_framerate;

				cam_req_mgr_set_is_crm_in_ssm_mode(true);

				cam_sbi_hw_mgr_invoke_writereg_cmd(
					hw_mgr, (void *)(&ssm_info), sbi_cmd);
				CAM_INFO(CAM_SBI, "SSM base addr update. Preview buffer:0x%llx, "
					"Recording buffer:0x%llx",
					ssm_info.addr_lr, ssm_info.addr_hr);
			}
			else {
				cam_req_mgr_set_is_crm_in_ssm_mode(false);

				/* set to dma address */
				sbi_cmd = cam_sbi_reg_cmd_base_addr_upd_scenario_1;
				cam_sbi_hw_mgr_invoke_writereg_cmd(
					hw_mgr, (void *)(&scratch_iova), sbi_cmd);
				CAM_INFO(CAM_SBI, "SBI base addr update. Preview buffer:0x%llx",
					scratch_iova);
			}

			cam_sbi_hw_mgr_dump_specific_reg(hw_mgr, 0x1fc);
			cam_sbi_hw_mgr_dump_specific_reg(hw_mgr, 0x608);	//wdma0 base address

			/* irq enable */
			{
				struct cam_hw_info *sbi_hw;
				struct cam_sbi_device *hw_device;

				rc = cam_sbi_mgr_util_get_device(hw_mgr, 0, &hw_device);
				if (rc) {
					CAM_ERR(CAM_SBI, "Failed to get hw device");
					return rc;
				}

				sbi_hw = (struct cam_hw_info *)hw_device->hw_intf.hw_priv;
				cam_sbi_set_irq(sbi_hw, CAM_SBI_IRQ_ENABLE);
			}
		}

		CAM_DBG(CAM_SBI, "frame num %u hdl 0x%x iova 0x%x len %llu",
			cmd->custom_info, scratch_hdl,
			scratch_iova, scratch_mem_len);

		/* SBI : dump register */
		rc = hw_device->hw_intf.hw_ops.process_cmd(
			hw_device->hw_intf.hw_priv, CAM_SBI_HW_CMD_DUMP_REGISTER,
			&g_sbi_hw_mgr.debugfs_entry.dump_register, sizeof(bool));
	}

	/* Start custom csid */
	list_for_each_entry(hw_mgr_res, &ctx_manager->res_list_custom_csid, list) {
		rc = cam_sbi_hw_mgr_start_hw_res(hw_mgr_res);
		if (rc) {
			CAM_ERR(CAM_ISP, "Can not START CSID(id :%d)",
				hw_mgr_res->res_id);
			goto err;
		}
	}

	/* Start custom cid */
	list_for_each_entry(hw_mgr_res, &ctx_manager->res_list_custom_cid, list) {
		rc = cam_sbi_hw_mgr_start_hw_res(hw_mgr_res);
		if (rc) {
			CAM_ERR(CAM_ISP, "Can not START CID(id :%d)",
				hw_mgr_res->res_id);
			goto err;
		}
	}

	CAM_DBG(CAM_SBI, "Start success for ctx id:%d", ctx_manager->ctx_index);
	return 0;
err:
	// todo : fix build error
	//stop_args.ctxt_to_hw_map = hw_config->ctxt_to_hw_map;
	//cam_sbi_hw_mgr_stop(hw_mgr_priv, &stop_args); //cam_sbi_hw_mgr_stop_hw(hw_mgr_priv, &stop_args);
deinit_hw:
	/* deinit the hw previously initialized */
	CAM_DBG(CAM_SBI, "Exit...(rc=%d)", rc);

	return rc;
}

static int cam_sbi_hw_mgr_put_ctx(struct list_head *src_list,
	struct cam_sbi_hw_mgr_ctx ** hw_mgr_ctx)
{
	struct cam_sbi_hw_mgr_ctx *ctx_ptr = NULL;

	ctx_ptr = *hw_mgr_ctx;
	if (ctx_ptr)
		list_add_tail(&ctx_ptr->list, src_list);
	*hw_mgr_ctx = NULL;
	return 0;
}

static int cam_sbi_hw_mgr_get_ctx(struct list_head *src_list,
	struct cam_sbi_hw_mgr_ctx **custom_ctx)
{
	struct cam_sbi_hw_mgr_ctx *ctx_ptr = NULL;

	if (!list_empty(src_list)) {
		ctx_ptr = list_first_entry(src_list,
			struct cam_sbi_hw_mgr_ctx, list);
		list_del_init(&ctx_ptr->list);
	} else {
		CAM_ERR(CAM_SBI, "No more free custom hw mgr ctx");
		return -EINVAL;
	}
	*custom_ctx = ctx_ptr;
	memset(ctx_ptr->sub_hw_list, 0,
		sizeof(struct cam_sbi_hw_mgr_res) * CAM_CUSTOM_HW_RES_MAX);

	return 0;
}

static int cam_sbi_hw_mgr_put_res(struct list_head *src_list,
	struct cam_sbi_hw_mgr_res **res)
{
	struct cam_sbi_hw_mgr_res *res_ptr = NULL;

	res_ptr = *res;
	if (res_ptr)
		list_add_tail(&res_ptr->list, src_list);

	return 0;
}

static int cam_sbi_hw_mgr_get_res(struct list_head *src_list,
	struct cam_sbi_hw_mgr_res **res)
{
	int rc = 0;
	struct cam_sbi_hw_mgr_res *res_ptr = NULL;

	if (!list_empty(src_list)) {
		res_ptr = list_first_entry(src_list,
			struct cam_sbi_hw_mgr_res, list);
		list_del_init(&res_ptr->list);
	} else {
		CAM_ERR(CAM_SBI, "No more free custom ctx rsrc");
		rc = -1;
	}
	*res = res_ptr;

	return rc;
}

static enum cam_ife_pix_path_res_id
cam_sbi_hw_mgr_get_csid_res_type(uint32_t out_port_type)
{
	enum cam_ife_pix_path_res_id path_id;

	switch (out_port_type) {
	case CAM_CUSTOM_OUT_RES_UDI_0:
		path_id = CAM_IFE_PIX_PATH_RES_UDI_0;
		break;
	case CAM_CUSTOM_OUT_RES_UDI_1:
		path_id = CAM_IFE_PIX_PATH_RES_UDI_1;
		break;
	case CAM_CUSTOM_OUT_RES_UDI_2:
		path_id = CAM_IFE_PIX_PATH_RES_UDI_2;
		break;
	case CAM_CUSTOM_OUT_RES_DUMMY:
		path_id = CAM_IFE_PIX_PATH_RES_MAX;
		/* Dummy port purposefully added */
		CAM_DBG(CAM_SBI, "SBI O3 port %u",
			CAM_CUSTOM_OUT_RES_DUMMY);
		break;
	default:
		path_id = CAM_IFE_PIX_PATH_RES_MAX;
		CAM_DBG(CAM_SBI, "maximum rdi output type exceeded");
		break;
	}

	CAM_DBG(CAM_SBI, "out_port %x path_id %d", out_port_type, path_id);

	return path_id;
}

static int
cam_sbi_hw_mgr_acquire_cid_res(struct cam_sbi_hw_mgr_ctx *custom_ctx,
	struct cam_isp_in_port_generic_info *in_port,
	struct cam_sbi_hw_mgr_res **cid_res,
	enum cam_ife_pix_path_res_id path_res_id,
	struct cam_isp_resource_node **cid_rsrc_node)
{
	int rc = -1;
	int i;
	struct cam_sbi_hw_mgr *custom_hw_mgr;
	struct cam_hw_intf *hw_intf;
	struct cam_sbi_hw_mgr_res *cid_res_temp;
	struct cam_csid_hw_reserve_resource_args csid_acquire;
	struct cam_isp_resource_node *isp_rsrc_node;
	struct cam_isp_out_port_generic_info *out_port = NULL;

	custom_hw_mgr = custom_ctx->hw_mgr;
	*cid_res = NULL;

	rc = cam_sbi_hw_mgr_get_res(&custom_ctx->free_res_list, cid_res);
	if (rc) {
		CAM_ERR(CAM_SBI, "No more free hw mgr resource");
		goto end;
	}

	memset(&csid_acquire, 0, sizeof(csid_acquire));
	cid_res_temp = *cid_res;
	csid_acquire.res_type = CAM_ISP_RESOURCE_CID;
	csid_acquire.in_port = in_port;
	csid_acquire.res_id = path_res_id;
	csid_acquire.node_res = NULL;
	CAM_DBG(CAM_SBI, "path_res_id %d", path_res_id);

	if (in_port->num_out_res)
		out_port = &(in_port->data[0]);

	for (i = 0; i < CAM_CUSTOM_CSID_HW_MAX; i++) {
		if (!custom_hw_mgr->csid_devices[i])
			continue;

		hw_intf = custom_hw_mgr->csid_devices[i];
		rc = hw_intf->hw_ops.reserve(hw_intf->hw_priv, &csid_acquire,
			sizeof(csid_acquire));
		/* since there is a need of 1 cid at this stage */
		if (rc)
			continue;
		else
			break;
	}

	if (csid_acquire.node_res == NULL) {
		CAM_ERR(CAM_SBI, "## csid_acquire is NULL !!");
	}

	if (!csid_acquire.node_res) {
		CAM_ERR(CAM_SBI,
			"Can not acquire custom cid resource for path %d",
			path_res_id);
		rc = -EAGAIN;
		goto put_res;
	}

	*cid_rsrc_node = csid_acquire.node_res;
	isp_rsrc_node = csid_acquire.node_res;
	cid_res_temp->rsrc_node = isp_rsrc_node;
	cid_res_temp->res_type = CAM_CUSTOM_CID_HW;
	cid_res_temp->res_id = isp_rsrc_node->res_id;
	cam_sbi_hw_mgr_put_res(&custom_ctx->res_list_custom_cid,
		&cid_res_temp);

	CAM_DBG(CAM_SBI, "CID acquired successfully %u", isp_rsrc_node->res_id);

	return 0;

put_res:
	cam_sbi_hw_mgr_put_res(&custom_ctx->free_res_list, cid_res);
end:
	return rc;
}

static int cam_sbi_hw_mgr_acquire_csid_res(
	struct cam_sbi_hw_mgr_ctx *custom_ctx,
	struct cam_isp_in_port_generic_info *in_port_info)
{
	int rc = 0, i = 0;
	struct cam_sbi_hw_mgr *custom_hw_mgr;
	struct cam_isp_out_port_generic_info *out_port;
	struct cam_sbi_hw_mgr_res *custom_csid_res;
	struct cam_sbi_hw_mgr_res *custom_cid_res;
	struct cam_hw_intf *hw_intf;
	struct cam_csid_hw_reserve_resource_args custom_csid_acquire;
	enum cam_ife_pix_path_res_id path_res_id;
	struct cam_isp_resource_node *isp_rsrc_node;
	struct cam_isp_resource_node *cid_rsrc_node = NULL;

	custom_hw_mgr = custom_ctx->hw_mgr;

	for (i = 0; i < in_port_info->num_out_res; i++) {
		out_port = &in_port_info->data[i];
		if (out_port->res_type == CAM_CUSTOM_OUT_RES_DUMMY)
			custom_ctx->is_ssm_recording_instance = true;
		else
			custom_ctx->is_ssm_recording_instance = false;
		path_res_id =
			cam_sbi_hw_mgr_get_csid_res_type(out_port->res_type);

		if (path_res_id == CAM_IFE_PIX_PATH_RES_MAX) {
			//CAM_INFO(CAM_SBI, "Here");
			continue;
		}

		rc = cam_sbi_hw_mgr_acquire_cid_res(custom_ctx, in_port_info,
			&custom_cid_res,
			path_res_id,
			&cid_rsrc_node);
		if (rc) {
			CAM_ERR(CAM_SBI, "No free cid rsrc %d", rc);
			goto end;
		}

		rc = cam_sbi_hw_mgr_get_res(&custom_ctx->free_res_list,
			&custom_csid_res);
		if (rc) {
			CAM_ERR(CAM_SBI, "No more free hw mgr rsrc");
			goto end;
		}

		memset(&custom_csid_acquire, 0, sizeof(custom_csid_acquire));
		custom_csid_acquire.res_id = path_res_id;
		custom_csid_acquire.res_type = CAM_ISP_RESOURCE_PIX_PATH;
		custom_csid_acquire.cid = cid_rsrc_node->res_id;
		custom_csid_acquire.in_port = in_port_info;
		custom_csid_acquire.out_port = out_port;
		custom_csid_acquire.sync_mode = 0;
		custom_csid_acquire.node_res = NULL;

		hw_intf = cid_rsrc_node->hw_intf;
		rc = hw_intf->hw_ops.reserve(hw_intf->hw_priv,
			&custom_csid_acquire,
			sizeof(custom_csid_acquire));
		if (rc) {
			CAM_ERR(CAM_SBI,
				"Custom csid acquire failed for hw_idx %u rc %d",
				hw_intf->hw_idx, rc);
			goto put_res;
		}

		if (custom_csid_acquire.node_res == NULL) {
			CAM_ERR(CAM_SBI, "Acquire custom csid failed");
			rc = -EAGAIN;
			goto put_res;
		}

		isp_rsrc_node = custom_csid_acquire.node_res;
		custom_csid_res->rsrc_node = isp_rsrc_node;
		custom_csid_res->res_type = CAM_CUSTOM_CSID_HW;
		custom_csid_res->res_id = custom_csid_acquire.res_id;
		cam_sbi_hw_mgr_put_res(&custom_ctx->res_list_custom_csid,
			&custom_csid_res);
		CAM_DBG(CAM_SBI, "Custom CSID: %u res_id: %u acquired",
			isp_rsrc_node->hw_intf->hw_idx, isp_rsrc_node->res_id);
	}

	return 0;

put_res:
	cam_sbi_hw_mgr_put_res(&custom_ctx->free_res_list, &custom_csid_res);
end:
	return rc;
}

static int
cam_sbi_hw_mgr_free_hw_res(struct cam_sbi_hw_mgr_res *hw_mgr_res)
{
	int rc = 0;
	struct cam_isp_resource_node *isp_rsrc_node = NULL;
	struct cam_hw_intf *hw_intf = NULL;

	isp_rsrc_node = (struct cam_isp_resource_node *)hw_mgr_res->rsrc_node;
	if (!isp_rsrc_node) {
		CAM_ERR(CAM_SBI, "Invalid args");
		return -EINVAL;
	}

	hw_intf = isp_rsrc_node->hw_intf;
	if (hw_intf->hw_ops.release) {
		CAM_DBG(CAM_SBI, "RELEASE HW for res_id:%u",
			hw_mgr_res->res_id);
		rc = hw_intf->hw_ops.release(
			hw_intf->hw_priv, isp_rsrc_node,
			sizeof(struct cam_isp_resource_node));
		if (rc)
			CAM_ERR(CAM_SBI, "Release HW failed for hw_idx %d",
				hw_intf->hw_idx);
	}

	/* caller should make sure the resource is in a list */
	list_del_init(&hw_mgr_res->list);
	memset(hw_mgr_res, 0, sizeof(*hw_mgr_res));
	INIT_LIST_HEAD(&hw_mgr_res->list);

	return 0;
}

static int
cam_sbi_hw_mgr_release_hw_for_ctx(struct cam_sbi_hw_mgr_ctx *hw_mgr_ctx)
{
	int rc = -1;
	struct cam_sbi_hw_mgr_res *hw_mgr_res;
	struct cam_sbi_hw_mgr_res *hw_mgr_res_temp;

	/* Release custom cid */
	list_for_each_entry_safe(hw_mgr_res, hw_mgr_res_temp,
		&hw_mgr_ctx->res_list_custom_cid, list) {
		rc = cam_sbi_hw_mgr_free_hw_res(hw_mgr_res);
		if (rc)
			CAM_ERR(CAM_ISP, "Can not release CID(id :%d)",
				hw_mgr_res->res_id);
		cam_sbi_hw_mgr_put_res(&hw_mgr_ctx->free_res_list,
			&hw_mgr_res);
	}

	/* Release custom csid */
	list_for_each_entry_safe(hw_mgr_res, hw_mgr_res_temp,
		&hw_mgr_ctx->res_list_custom_csid, list) {
		rc = cam_sbi_hw_mgr_free_hw_res(hw_mgr_res);
		if (rc)
			CAM_ERR(CAM_ISP, "Can not release CSID(id :%d)",
				hw_mgr_res->res_id);
		cam_sbi_hw_mgr_put_res(&hw_mgr_ctx->free_res_list,
			&hw_mgr_res);
	}

	/* Release custom HW Here */

	return 0;
}

enum cam_custom_hw_resource_state
	cam_sbi_hw_mgr_get_custom_res_state(uint32_t in_rsrc_state)
{
	enum cam_custom_hw_resource_state rsrc_state;

	switch (in_rsrc_state) {
	case CAM_ISP_RESOURCE_STATE_UNAVAILABLE:
		rsrc_state = CAM_CUSTOM_HW_RESOURCE_STATE_UNAVAILABLE;
		break;
	case CAM_ISP_RESOURCE_STATE_AVAILABLE:
		rsrc_state = CAM_CUSTOM_HW_RESOURCE_STATE_AVAILABLE;
		break;
	case CAM_ISP_RESOURCE_STATE_RESERVED:
		rsrc_state = CAM_CUSTOM_HW_RESOURCE_STATE_RESERVED;
		break;
	case CAM_ISP_RESOURCE_STATE_INIT_HW:
		rsrc_state = CAM_CUSTOM_HW_RESOURCE_STATE_INIT_HW;
		break;
	case CAM_ISP_RESOURCE_STATE_STREAMING:
		rsrc_state = CAM_CUSTOM_HW_RESOURCE_STATE_STREAMING;
		break;
	default:
		rsrc_state = CAM_CUSTOM_HW_RESOURCE_STATE_UNAVAILABLE;
		CAM_DBG(CAM_SBI, "invalid rsrc type");
		break;
	}

	return rsrc_state;
}

static int cam_sbi_hw_mgr_acquire_hw_for_ctx(
	struct cam_sbi_hw_mgr_ctx * hw_mgr_ctx,
	struct cam_isp_in_port_generic_info *in_port_info,
	uint32_t *acquired_hw_id, uint32_t *acquired_hw_path)
{
	int rc = 0, i = 0;
	struct cam_hw_intf *hw_intf;
	struct cam_sbi_hw_mgr *custom_hw_mgr;
	struct cam_custom_sub_mod_acq acq;

	custom_hw_mgr = hw_mgr_ctx->hw_mgr;

	/* Acquire custom csid */
	rc = cam_sbi_hw_mgr_acquire_csid_res(hw_mgr_ctx, in_port_info);
	if (rc) {
		CAM_ERR(CAM_SBI, "Custom csid acquire failed rc %d");
		goto err;
	}

	/* Acquire custom hw */
	for (i = 0; i < CAM_CUSTOM_HW_SUB_MOD_MAX; i++) {
		hw_intf = custom_hw_mgr->custom_hw[i];
		if (!hw_intf)
			continue;

		rc = hw_intf->hw_ops.reserve(hw_intf->hw_priv, &acq,
			sizeof(acq));
		if (rc) {
			CAM_DBG(CAM_SBI, "No custom resource from hw %d",
				hw_intf->hw_idx);
			continue;
		}
		/* need to be set in reserve based on HW being acquired */
		//custom_ctx->sub_hw_list[i].hw_res = acq.rsrc_node;
		//custom_ctx->sub_hw_list[i].res_type = <res_type>
		//custom_ctx->sub_hw_list[i].res_id = <res_id>;
		break;
	}

err:
	return rc;
}

static int cam_sbi_hw_mgr_acquire(void *hw_mgr_priv, void *hw_acquire_args)
{
	struct cam_sbi_hw_mgr *hw_mgr = hw_mgr_priv;
	struct cam_hw_acquire_args *acquire_args =
		(struct cam_hw_acquire_args *)hw_acquire_args;

	uint32_t input_size = 0;
	int rc = -1;
	int32_t i;
	//uint32_t in_port_length;
	struct cam_sbi_hw_mgr_ctx * hw_mgr_ctx;
	//struct cam_sbi_in_port_info *in_port_info;
	//struct cam_sbi_resource *custom_rsrc;
	struct cam_isp_in_port_generic_info *gen_port_info = NULL;
	struct cam_custom_acquire_hw_info *acquire_hw_info;

	if (!hw_mgr_priv || !acquire_args) {
		CAM_ERR(CAM_SBI,
			"Invalid input params hw_mgr_priv %pK, acquire_args %pK",
			hw_mgr_priv, acquire_args);
		return -EINVAL;
	}

	hw_mgr = (struct cam_sbi_hw_mgr *)hw_mgr_priv;
	mutex_lock(&g_sbi_hw_mgr.ctx_mutex);
	rc = cam_sbi_hw_mgr_get_ctx(&hw_mgr->free_ctx_list, &hw_mgr_ctx);
	if (rc || !hw_mgr_ctx) {
		CAM_ERR(CAM_SBI, "Get custom hw context failed");
		mutex_unlock(&g_sbi_hw_mgr.ctx_mutex);
		goto err;
	}
	mutex_unlock(&g_sbi_hw_mgr.ctx_mutex);

	CAM_DBG(CAM_SBI, "Enter..ctx_index:%d", hw_mgr_ctx->ctx_index);

	hw_mgr_ctx->hw_mgr = hw_mgr;
	hw_mgr_ctx->cb_priv = acquire_args->context_data;
	hw_mgr_ctx->event_cb = acquire_args->event_cb;

	acquire_hw_info =
		(struct cam_custom_acquire_hw_info *)acquire_args->acquire_info;

	for (i = 0; i < acquire_hw_info->num_inputs; i++) {
		rc = cam_sbi_hw_mgr_acquire_get_unified_dev_str(
			acquire_hw_info, &input_size, &gen_port_info);

		if (rc < 0) {
			CAM_ERR(CAM_SBI, "Failed in parsing: %d", rc);
			goto free_res;
		}

		CAM_DBG(CAM_SBI, "in_res_type %x", gen_port_info->res_type);
		rc = cam_sbi_hw_mgr_acquire_hw_for_ctx(hw_mgr_ctx,
			gen_port_info, &acquire_args->acquired_hw_id[i],
			acquire_args->acquired_hw_path[i]);
		if (rc) {
			CAM_ERR(CAM_SBI, "can not acquire resource");
			goto free_mem;
		}

		kfree(gen_port_info->data);
		kfree(gen_port_info);
		gen_port_info = NULL;
	}

	hw_mgr_ctx->ctx_in_use = 1;
	acquire_args->ctxt_to_hw_map = hw_mgr_ctx;
	cam_sbi_hw_mgr_put_ctx(&hw_mgr->used_ctx_list, &hw_mgr_ctx);
	CAM_DBG(CAM_SBI, "Exit...(success)");

	return 0;

free_mem:
	kfree(gen_port_info->data);
	kfree(gen_port_info);
free_res :
	cam_sbi_hw_mgr_release_hw_for_ctx(hw_mgr_ctx);
err:
	CAM_DBG(CAM_SBI, "Exit...(rc=%d)", rc);
	return rc;
}

enum cam_isp_resource_state
	cam_sbi_hw_mgr_get_isp_res_state(uint32_t in_rsrc_state)
{
	enum cam_isp_resource_state rsrc_state;

	switch (in_rsrc_state) {
	case CAM_CUSTOM_HW_RESOURCE_STATE_UNAVAILABLE:
		rsrc_state = CAM_ISP_RESOURCE_STATE_UNAVAILABLE;
		break;
	case CAM_CUSTOM_HW_RESOURCE_STATE_AVAILABLE:
		rsrc_state = CAM_ISP_RESOURCE_STATE_AVAILABLE;
		break;
	case CAM_CUSTOM_HW_RESOURCE_STATE_RESERVED:
		rsrc_state = CAM_ISP_RESOURCE_STATE_RESERVED;
		break;
	case CAM_CUSTOM_HW_RESOURCE_STATE_INIT_HW:
		rsrc_state = CAM_ISP_RESOURCE_STATE_INIT_HW;
		break;
	case CAM_CUSTOM_HW_RESOURCE_STATE_STREAMING:
		rsrc_state = CAM_ISP_RESOURCE_STATE_STREAMING;
		break;
	default:
		rsrc_state = CAM_ISP_RESOURCE_STATE_UNAVAILABLE;
		CAM_DBG(CAM_SBI, "invalid rsrc type");
		break;
	}

	return rsrc_state;
}

enum cam_isp_resource_type
	cam_sbi_hw_mgr_get_isp_res_type(enum cam_sbi_hw_mgr_res_type res_type)
{
	switch (res_type) {
	case CAM_CUSTOM_CID_HW:
		return CAM_ISP_RESOURCE_CID;
	case CAM_CUSTOM_CSID_HW:
		return CAM_ISP_RESOURCE_PIX_PATH;
	default:
		return CAM_ISP_RESOURCE_MAX;
	}
}

static int
cam_sbi_hw_mgr_deinit_hw_res(struct cam_sbi_hw_mgr_res *hw_mgr_res)
{
	int rc = -1;
	struct cam_isp_resource_node *isp_rsrc_node = NULL;
	struct cam_hw_intf *hw_intf = NULL;

	isp_rsrc_node = (struct cam_isp_resource_node *)hw_mgr_res->rsrc_node;
	if (!isp_rsrc_node) {
		CAM_ERR(CAM_SBI, "Invalid args");
		goto err;
	}

	hw_intf = isp_rsrc_node->hw_intf;
	if (hw_intf->hw_ops.deinit) {
		CAM_DBG(CAM_SBI, "DEINIT HW for res_id:%u", hw_mgr_res->res_id);
		rc = hw_intf->hw_ops.deinit(
			hw_intf->hw_priv, isp_rsrc_node,
			sizeof(struct cam_isp_resource_node));
		if (rc)
			goto err;
	}

	return 0;

err:
	CAM_DBG(CAM_SBI, "DEINIT HW failed for res_id:%u", hw_mgr_res->res_id);
	return rc;
}

static int
cam_sbi_hw_mgr_stop_hw_res(struct cam_sbi_hw_mgr_res *hw_mgr_res)
{
	int rc = -1;
	struct cam_csid_hw_stop_args stop_cmd;
	struct cam_isp_resource_node *isp_rsrc_node = NULL;
	struct cam_hw_intf *hw_intf = NULL;

	isp_rsrc_node = (struct cam_isp_resource_node *)hw_mgr_res->rsrc_node;
	if (!isp_rsrc_node) {
		CAM_ERR(CAM_SBI, "Invalid args");
		return -EINVAL;
	}

	hw_intf = isp_rsrc_node->hw_intf;
	if (hw_intf->hw_ops.stop) {
		CAM_DBG(CAM_SBI, "STOP HW for res_id:%u", hw_mgr_res->res_id);
		stop_cmd.num_res = 1;
		stop_cmd.node_res = &isp_rsrc_node;
		stop_cmd.stop_cmd = CAM_CSID_HALT_IMMEDIATELY;
		rc = hw_intf->hw_ops.stop(hw_intf->hw_priv, &stop_cmd,
			sizeof(struct cam_csid_hw_stop_args));
		if (rc)
			goto err;
	}

	return 0;

err:
	CAM_DBG(CAM_SBI, "STOP HW failed for res_id:%u", hw_mgr_res->res_id);
	return rc;
}

static int cam_sbi_hw_mgr_stop(void *hw_mgr_priv, void *stop_hw_args)
{
	int rc = 0;
	struct cam_sbi_hw_mgr *hw_mgr = hw_mgr_priv;
	struct cam_hw_stop_args *stop_args =
		(struct cam_hw_stop_args *)stop_hw_args;
	struct cam_sbi_stop_args  *sbi_stop_args = NULL;
	struct cam_sbi_hw_mgr_res * hw_mgr_res;
	struct cam_sbi_hw_mgr_ctx * hw_mgr_ctx;

	struct cam_sbi_device *hw_device;
	uint32_t device_index;

	if (!hw_mgr_priv || !stop_args) {
		CAM_ERR(CAM_SBI, "Invalid arguments");
		return -EINVAL;
	}
	sbi_stop_args = stop_args->args;

	hw_mgr_ctx = (struct cam_sbi_hw_mgr_ctx *)stop_args->ctxt_to_hw_map;
	if (!hw_mgr_ctx || !hw_mgr_ctx->ctx_in_use) {
		CAM_ERR(CAM_SBI, "Invalid manager context is used");
		return -EPERM;
	}

	device_index = hw_mgr_ctx->device_index;
	if (device_index >= hw_mgr->device_count) {
		CAM_ERR(CAM_SBI, "Invalid device index %d", device_index);
		return -EPERM;
	}

	CAM_DBG(CAM_SBI, "Stop device index %d", device_index);

	rc = cam_sbi_mgr_util_get_device(hw_mgr, device_index, &hw_device);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed to get hw device");
		return rc;
	}

	/* Stop custom cid here */
	list_for_each_entry(hw_mgr_res, &hw_mgr_ctx->res_list_custom_cid, list) {
		rc = cam_sbi_hw_mgr_stop_hw_res(hw_mgr_res);
		if (rc)
			CAM_ERR(CAM_SBI, "failed to stop hw %d",
				hw_mgr_res->res_id);
	}

	/* Stop custom csid here */
	list_for_each_entry(hw_mgr_res, &hw_mgr_ctx->res_list_custom_csid, list) {
		rc = cam_sbi_hw_mgr_stop_hw_res(hw_mgr_res);
		if (rc) {
			CAM_ERR(CAM_SBI, "failed to stop hw %d",
				hw_mgr_res->res_id);
		}
	}


	/* stop SBI hardware */
	if (hw_device->hw_intf.hw_ops.stop) {
		rc = hw_device->hw_intf.hw_ops.stop(hw_device->hw_intf.hw_priv,
			&hw_mgr_ctx->is_ssm_recording_instance, sizeof(bool));
		if (rc) {
			CAM_ERR(CAM_SBI, "Failed in HW stop %d", rc);
		}
	}

	if (sbi_stop_args->stop_only) {
		CAM_DBG(CAM_SBI, "sbi_stop_only");
		 goto end;
	} else {
		CAM_DBG(CAM_SBI, "sbi_stop_fully");
	}

	/* Deinit custom cid here */
	list_for_each_entry(hw_mgr_res, &hw_mgr_ctx->res_list_custom_cid, list) {
		rc = cam_sbi_hw_mgr_deinit_hw_res(hw_mgr_res);
		if (rc)
			CAM_ERR(CAM_SBI, "failed to stop hw %d",
				hw_mgr_res->res_id);
	}

	/* Deinit custom csid here */
	list_for_each_entry(hw_mgr_res, &hw_mgr_ctx->res_list_custom_csid, list) {
		rc = cam_sbi_hw_mgr_deinit_hw_res(hw_mgr_res);
		if (rc) {
			CAM_ERR(CAM_SBI, "failed to stop hw %d",
				hw_mgr_res->res_id);
		}
	}

	/* deinit custom rsrc */

end:
	return rc;
}

#if 0
static int32_t cam_sbi_hw_mgr_dump_all_reg(void *hw_mgr_priv, int param)
{
	struct cam_sbi_hw_mgr *hw_mgr = hw_mgr_priv;
	int32_t device_index = 0;
	int32_t rc;
	struct cam_sbi_device *hw_device;

	// TODO: device_index should not be a fixed value
	rc = cam_sbi_mgr_util_get_device(hw_mgr, device_index, &hw_device);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed to get hw device");
		return rc;
	}

	rc = hw_device->hw_intf.hw_ops.process_cmd(
		hw_device->hw_intf.hw_priv, CAM_SBI_HW_CMD_DUMP_ALL_REG,
		&param, sizeof(int));

	return rc;
}
#endif

static int32_t cam_sbi_hw_mgr_dump_specific_reg(void *hw_mgr_priv, int param)
{
	struct cam_sbi_hw_mgr *hw_mgr = hw_mgr_priv;
	int32_t rc;
	struct cam_sbi_device *hw_device;

	// TODO: device_index should not be a fixed value
	rc = cam_sbi_mgr_util_get_device(hw_mgr, 0, &hw_device);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed to get hw device");
		return rc;
	}

	rc = hw_device->hw_intf.hw_ops.process_cmd(
		hw_device->hw_intf.hw_priv, CAM_SBI_HW_CMD_DUMP_SPECIFIC_REG,
		&param, sizeof(int));

	return rc;
}

int32_t cam_sbi_hw_mgr_invoke_writereg_cmd(void *hw_mgr_priv,
	void *val,
	int32_t write_reg_cmd)
{
	struct cam_sbi_hw_mgr *hw_mgr = hw_mgr_priv;
	int32_t rc;
	struct cam_sbi_device *hw_device;
	enum cam_sbi_hw_cmd_type sbi_ctl_cmd;

	// TODO: device_index should not be a fixed value
	rc = cam_sbi_mgr_util_get_device(hw_mgr, 0, &hw_device);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed to get hw device");
		return rc;
	}

	switch (write_reg_cmd) {
	case cam_sbi_reg_cmd_base_addr_upd_scenario_1:
		sbi_ctl_cmd = CAM_SBI_HW_CMD_INIT_PREVIEW;
		rc = hw_device->hw_intf.hw_ops.process_cmd(hw_device->hw_intf.hw_priv,
			sbi_ctl_cmd, val,
			sizeof(uint64_t));
		break;
	case cam_sbi_reg_cmd_base_addr_upd_scenario_4:
		sbi_ctl_cmd = CAM_SBI_HW_CMD_INIT_SSM;
		rc = hw_device->hw_intf.hw_ops.process_cmd(hw_device->hw_intf.hw_priv,
			sbi_ctl_cmd, val,
			sizeof(uint64_t));
		break;
	case cam_sbi_reg_cmd_task2_setup:
		sbi_ctl_cmd = CAM_SBI_HW_CMD_WRITEREG_TASK2_SETUP;
		rc = hw_device->hw_intf.hw_ops.process_cmd(hw_device->hw_intf.hw_priv,
			sbi_ctl_cmd, val,
			sizeof(uint32_t));
		break;
	case cam_sbi_reg_cmd_task2_action:
		sbi_ctl_cmd = CAM_SBI_HW_CMD_WRITEREG_TASK2_ACTION;
		rc = hw_device->hw_intf.hw_ops.process_cmd(hw_device->hw_intf.hw_priv,
			sbi_ctl_cmd, val,
			sizeof(uint32_t));
		break;
	case cam_sbi_req_cmd_update_ssm_info:
		rc = hw_device->hw_intf.hw_ops.process_cmd(hw_device->hw_intf.hw_priv,
			CAM_SBI_HW_CMD_UPDATE_SSM_INFO, val,
			sizeof(struct cam_sbi_cmd_buf_type_1 *));
		break;

	default:
		break;
	}

	return rc;
}
//#endif

int32_t cam_sbi_hw_mgr_init_register_set(void *hw_mgr_priv,
	void *cmd_args, uint32_t args_size, int32_t write_reg_cmd)
{
	struct cam_sbi_hw_mgr *hw_mgr = hw_mgr_priv;
	int32_t rc;
	struct cam_sbi_device *hw_device;
	enum cam_sbi_hw_cmd_type sbi_ctl_cmd = 0;

	// TODO: device_index should not be a fixed value
	rc = cam_sbi_mgr_util_get_device(hw_mgr, 0, &hw_device);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed to get hw device");
		return rc;
	}

	switch (write_reg_cmd) {
	case cam_sbi_reg_cmd_init:
		sbi_ctl_cmd = CAM_SBI_HW_CMD_SET_INIT;
		break;
	default:
		break;
	}

	rc = hw_device->hw_intf.hw_ops.process_cmd(hw_device->hw_intf.hw_priv,
						   sbi_ctl_cmd, cmd_args, args_size);

	return rc;
}

static int cam_sbi_hw_mgr_add_io_buffers(int iommu_hdl,
	struct cam_hw_prepare_update_args *prepare)
{
	int rc = 0, i = 0;
	int32_t hdl;
	uint32_t plane_id;
	struct cam_buf_io_cfg *io_cfg;

	io_cfg =
		(struct cam_buf_io_cfg *)((uint8_t *)&prepare->packet->payload +
			prepare->packet->io_configs_offset);

	/* Validate hw update entries */

	for (i = 0; i < prepare->packet->num_io_configs; i++) {
		CAM_DBG(CAM_SBI, "======= io config idx %d ============", i);
		CAM_DBG(CAM_SBI,
			"i %d req_id %llu resource_type:%d fence:%d direction %d",
			i, prepare->packet->header.request_id,
			io_cfg[i].resource_type, io_cfg[i].fence,
			io_cfg[i].direction);

		CAM_DBG(CAM_SBI, "format: %d", io_cfg[i].format);

		if (io_cfg[i].direction == CAM_BUF_OUTPUT) {
			CAM_DBG(CAM_SBI, "output fence 0x%x", io_cfg[i].fence);
		}
		else if (io_cfg[i].direction == CAM_BUF_INPUT) {
			CAM_DBG(CAM_SBI, "input fence 0x%x", io_cfg[i].fence);
		}
		else {
			CAM_ERR(CAM_SBI, "Invalid io config direction :%d",
				io_cfg[i].direction);
			return -EINVAL;
		}

		for (plane_id = 0; plane_id < CAM_PACKET_MAX_PLANES;
			plane_id++) {
			if (!io_cfg[i].mem_handle[plane_id])
				continue;

			hdl = io_cfg[i].mem_handle[plane_id];
			CAM_DBG(CAM_SBI, "handle 0x%x for plane %d", hdl,
				plane_id);
			/* Use cam_mem_get_io_buf() to retrieve iova */
		}

		/* Do other I/O config operations */
	}

	return rc;
}

static int cam_sbi_hw_mgr_prepare_update(void *hw_mgr_priv,
	void *prepare_hw_update_args)
{
	struct cam_sbi_hw_mgr *hw_mgr = hw_mgr_priv;
	struct cam_hw_prepare_update_args *prepare =
		(struct cam_hw_prepare_update_args *)prepare_hw_update_args;
	struct cam_sbi_prepare_hw_update_data *prepare_hw_data;
	struct cam_sbi_hw_mgr_ctx * hw_mgr_ctx = NULL;
	struct cam_cmd_buf_desc *cmd_desc = NULL;

	if (!hw_mgr_priv || !prepare_hw_update_args) {
		CAM_ERR(CAM_SBI, "Invalid args %pK %pK", hw_mgr_priv,
			prepare_hw_update_args);
		return -EINVAL;
	}

	//cam_sbi_hw_mgr_dump_all_reg(hw_mgr, 4);
	//prepare = (struct cam_hw_prepare_update_args *)prepare_hw_update_args;

	CAM_DBG(CAM_SBI, "Enter for req_id %lld",
		prepare->packet->header.request_id);
	//CAM_DBG(CAM_SBI, "prepare->packet->io_configs_offset %d",
	//   prepare->packet->io_configs_offset);

	/* Prepare packet */
	prepare_hw_data =
		(struct cam_sbi_prepare_hw_update_data *)prepare->priv;
	prepare_hw_data->packet_opcode_type =
		(prepare->packet->header.op_code & 0xFFF);
	hw_mgr_ctx = (struct cam_sbi_hw_mgr_ctx *)prepare->ctxt_to_hw_map;

	/* Test purposes-check the data in cmd buffer */
	cmd_desc = (struct cam_cmd_buf_desc *)((uint8_t *)&prepare->packet
		->payload +
		prepare->packet->cmd_buf_offset);

	cam_sbi_hw_mgr_add_io_buffers(hw_mgr->img_iommu_hdl, prepare);

	/* copy reg_dump_buf_desc */
	if (prepare->num_reg_dump_buf > 0) {
		memcpy(hw_mgr_ctx->reg_dump_buf_desc, cmd_desc, sizeof(struct cam_cmd_buf_desc) * prepare->num_reg_dump_buf);
	} else {
		memcpy(hw_mgr_ctx->reg_dump_buf_desc, cmd_desc, sizeof(struct cam_cmd_buf_desc));
	}

	return 0;
}

static int cam_sbi_set_csid_debug(void *data, u64 val)
{
	g_sbi_hw_mgr.debugfs_entry.custom_csid_debug = val;
	CAM_DBG(CAM_SBI, "Set CSID Debug value :%lld", val);
	return 0;
}

static int cam_sbi_get_csid_debug(void *data, u64 *val)
{
	*val = g_sbi_hw_mgr.debugfs_entry.custom_csid_debug;
	CAM_DBG(CAM_SBI, "Get CSID Debug value :%lld",
		g_sbi_hw_mgr.debugfs_entry.custom_csid_debug);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(cam_custom_csid_debug,
	cam_sbi_get_csid_debug,
	cam_sbi_set_csid_debug, "%16llu");


static int cam_sbi_blob_bw_update_v2(
	struct cam_sbi_hw_mgr *hw_mgr,
	struct cam_sbi_bw_config_v2 *bw_config_v2)
{
	bool apply_bw_update = false;
	int rc = -EINVAL;

	struct cam_hw_info *sbi_hw;
	struct cam_hw_soc_info *soc_info;
	struct cam_sbi_soc_private *soc_private;
	struct cam_axi_vote to_be_applied_axi_vote;

	struct cam_sbi_device *hw_device;

	// TODO: device_index should not be a fixed value
	rc = cam_sbi_mgr_util_get_device(hw_mgr, 0, &hw_device);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed to get hw device");
		return rc;
	}

	sbi_hw = (struct cam_hw_info *)hw_device->hw_intf.hw_priv;
	soc_info = &sbi_hw->soc_info;
	soc_private = (struct cam_sbi_soc_private *)soc_info->soc_private;

	//if ((to_be_applied_axi_vote->num_paths != top_common->applied_axi_vote.num_paths) || (total_bw_new_vote != top_common->total_bw_applied))
		apply_bw_update = true;

	to_be_applied_axi_vote.num_paths = bw_config_v2->num_paths;
	memcpy(&to_be_applied_axi_vote.axi_path, bw_config_v2->axi_path,
			bw_config_v2->num_paths * sizeof(struct cam_axi_per_path_bw_vote));
	//CAM_INFO(CAM_SBI, "n_paths:%d, val:%lld", to_be_applied_axi_vote.num_paths,
	//   to_be_applied_axi_vote.axi_path[1].camnoc_bw);

	if (apply_bw_update) {
		rc = cam_cpas_update_axi_vote(soc_private->cpas_handle, &to_be_applied_axi_vote);
		if (!rc) {
			//memcpy(&top_common->applied_axi_vote, to_be_applied_axi_vote,
			//       sizeof(struct cam_axi_vote));
			//top_common->total_bw_applied = total_bw_new_vote;
			static uint64_t prev_camnoc_bw = 0;
			if (prev_camnoc_bw != to_be_applied_axi_vote.axi_path[1].camnoc_bw) {
				CAM_INFO(CAM_SBI, "BW request success %lld mnoc_ib %lld mnoc_ab %lld",
					to_be_applied_axi_vote.axi_path[1].camnoc_bw,
					to_be_applied_axi_vote.axi_path[1].mnoc_ib_bw,
					to_be_applied_axi_vote.axi_path[1].mnoc_ab_bw);
				prev_camnoc_bw = to_be_applied_axi_vote.axi_path[1].camnoc_bw;
			}
		} else {
			CAM_ERR(CAM_SBI, "BW request failed, rc=%d", rc);
		}
	}

	return rc;
}

static int cam_sbi_hw_mgr_notify_error(
	struct cam_sbi_hw_mgr_ctx * hw_mgr_ctx, uint64_t request_id)
{
	struct cam_sbi_device * hw_device = &hw_mgr_ctx->hw_mgr->hw_device[0];
	struct cam_hw_info *    sbi_hw = hw_device->hw_intf.hw_priv;
	struct cam_sbi_core *   core = sbi_hw->core_info;
	cam_sbi_ssm_info *      ssm = &core->ssm_info;
	struct cam_context *    context = hw_mgr_ctx->cb_priv;
	struct cam_req_mgr_message req_msg;
	int rc = 0;

	if (ssm->error_status == CAM_SBI_NO_ERROR)
		return 0;

	if (ssm->error_notified == false) {
		req_msg.session_hdl = context->session_hdl;
		req_msg.u.err_msg.device_hdl = context->dev_hdl;
		req_msg.u.err_msg.error_type =
			CAM_REQ_MGR_ERROR_TYPE_RESERVED;
		req_msg.u.err_msg.link_hdl = context->link_hdl;
		req_msg.u.err_msg.request_id = request_id;
		req_msg.u.err_msg.resource_size = ((uint64_t)ssm->error_status << 16) |
				ssm->frame_drop_count;

		CAM_DBG(CAM_SBI, "type %d, requestID %llu, device hdl 0x%x, "
				"session hdl 0x%x, link hdl 0x%x, resource_size 0x%llx",
				req_msg.u.err_msg.error_type,
				req_msg.u.err_msg.request_id,
				req_msg.u.err_msg.device_hdl,
				req_msg.session_hdl,
				req_msg.u.err_msg.link_hdl,
				req_msg.u.err_msg.resource_size);

#ifdef __CAM_SBI_FRAME_DROP_NOTIFY_ENABLE__
		if (cam_req_mgr_notify_message(&req_msg,
				V4L_EVENT_CAM_REQ_MGR_ERROR,
				V4L_EVENT_CAM_REQ_MGR_EVENT))
			CAM_ERR(CAM_SBI,
				"Error in notifying the error time for req id:%lld ctx %u",
				request_id, context->ctx_id);
#endif

		ssm->error_notified = true;
	}

	return rc;
}


static int cam_sbi_hw_mgr_reset_csid_res(struct cam_sbi_hw_mgr_res *hw_mgr_res)
{
	int rc = -1;
	struct cam_csid_reset_cfg_args  csid_reset_args;
	struct cam_isp_resource_node *custom_rsrc_node = NULL;
	struct cam_hw_intf *hw_intf = NULL;

	custom_rsrc_node =
		(struct cam_isp_resource_node *)hw_mgr_res->rsrc_node;
	if (!custom_rsrc_node) {
		CAM_ERR(CAM_SBI, "Invalid args");
		return -EINVAL;
	}

	csid_reset_args.reset_type = CAM_IFE_CSID_RESET_PATH;
	csid_reset_args.node_res = custom_rsrc_node;
	hw_intf = custom_rsrc_node->hw_intf;
	if (hw_intf->hw_ops.reset) {
		CAM_DBG(CAM_SBI, "RESET HW for res_id:%u", hw_mgr_res->res_id);
		rc = hw_intf->hw_ops.reset(hw_intf->hw_priv,
			&csid_reset_args,
			sizeof(struct cam_csid_reset_cfg_args));
		if (rc)
			goto err;
	}

	return 0;

err:
	CAM_ERR(CAM_SBI, "RESET HW failed for res_id:%u", hw_mgr_res->res_id);
	return rc;
}


static int cam_sbi_hw_mgr_reset(void *hw_mgr_priv, void *hw_reset_args)
{
	struct cam_hw_reset_args         *reset_args = hw_reset_args;
	struct cam_sbi_hw_mgr *hw_mgr = hw_mgr_priv;
	struct cam_sbi_device *hw_device = NULL;
	struct cam_sbi_hw_reset_args reset_cmd = { 0, };
	uint32_t device_index;
	struct cam_sbi_hw_mgr_ctx        *ctx;
	struct cam_sbi_hw_mgr_res        *hw_mgr_res;
	int                               rc = 0;

	if (!hw_mgr_priv || !hw_reset_args) {
		CAM_ERR(CAM_SBI, "Invalid arguments");
		return -EINVAL;
	}

	ctx = (struct cam_sbi_hw_mgr_ctx *)reset_args->ctxt_to_hw_map;
	if (!ctx || !ctx->ctx_in_use) {
		CAM_ERR(CAM_SBI, "Invalid context is used");
		return -EPERM;
	}

	device_index = ctx->device_index;
	if (device_index >= hw_mgr->device_count) {
		CAM_ERR(CAM_SBI, "Invalid device index %d", device_index);
		return -EPERM;
	}

	rc = cam_sbi_mgr_util_get_device(hw_mgr, device_index, &hw_device);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed to get hw device");
		return rc;
	}

	CAM_DBG(CAM_SBI, "Reset SBI CSID and SBI core");
	list_for_each_entry(hw_mgr_res, &ctx->res_list_custom_csid, list) {
		rc = cam_sbi_hw_mgr_reset_csid_res(hw_mgr_res);
		if (rc) {
			CAM_ERR(CAM_SBI, "Failed to reset CSID:%d rc: %d",
				hw_mgr_res->res_id, rc);
			goto end;
		}
	}

	/* Reset SBI HW */
	if (hw_device->hw_intf.hw_ops.reset) {
		reset_cmd.reset_type = CAM_SBI_HW_RESET_TYPE_ALL_RESET;// no meaning
		rc = hw_device->hw_intf.hw_ops.reset(hw_device->hw_intf.hw_priv,
			&reset_cmd, sizeof(struct cam_sbi_hw_reset_args));
		if (rc) {
			CAM_ERR(CAM_SBI, "Failed in HW stop %d", rc);
			goto end;
		}
	}

end:
	return rc;
}


static int cam_sbi_hw_mgr_config(void *hw_mgr_priv, void *config_hw_args)
{
	int rc = -1;
	struct cam_sbi_hw_mgr * hw_mgr = hw_mgr_priv;
	struct cam_hw_config_args * cfg;
	struct cam_sbi_hw_mgr_ctx * hw_mgr_ctx;

	uint32_t *ptr;
	int32_t scratch_hdl;
	uint64_t scratch_iova;
	size_t len, scratch_mem_len;
	struct cam_cmd_buf_desc *cmd_desc = NULL;

	if (!hw_mgr_priv || !config_hw_args) {
		CAM_ERR(CAM_SBI, "Invalid arguments");
		return -EINVAL;
	}

	cfg = config_hw_args;
	hw_mgr_ctx = (struct cam_sbi_hw_mgr_ctx *)cfg->ctxt_to_hw_map;
	if (!hw_mgr_ctx) {
		CAM_ERR(CAM_SBI, "Invalid manager_context parameters");
		return -EPERM;
	}

	if (!hw_mgr_ctx->ctx_in_use) {
		CAM_ERR(CAM_SBI, "This manager_context is already in use");
		return -EPERM;
	}

	//CAM_DBG(CAM_SBI, "Ctx[%pK][%d] : Applying Req %lld",
	//	ctx, ctx->ctx_index, cfg->request_id);

	CAM_DBG(CAM_SBI,
		"Enter ctx id:%d num_hw_upd_entries %d request id: %llu",
		hw_mgr_ctx->ctx_index, cfg->num_hw_update_entries, cfg->request_id);

	cmd_desc = (struct cam_cmd_buf_desc *)(hw_mgr_ctx->reg_dump_buf_desc);
	rc = cam_packet_util_get_cmd_mem_addr(cmd_desc->mem_handle, &ptr, &len);

	if (cfg->num_hw_update_entries > 0 && !rc) {
		struct cam_sbi_cmd_buf_type_1 * cmd;

		ptr += (cmd_desc->offset / 4);
		cmd = (struct cam_sbi_cmd_buf_type_1 *)ptr;

		if (cmd->umd_node_type == 10) {
			//CAM_DBG(CAM_SBI, "Ignore this command request becasue "
			//		"it's from SSM record node");
			return 0;
		}

		if (cmd->is_ssm) {
			cam_sbi_hw_mgr_invoke_writereg_cmd(hw_mgr, cmd,
					cam_sbi_req_cmd_update_ssm_info);

			rc = cam_sbi_blob_bw_update_v2(hw_mgr, &cmd->bw_config_v2);
			if (rc)
				CAM_ERR(CAM_SBI, "Bandwidth Update Failed rc: %d", rc);

			cam_sbi_hw_mgr_notify_error(hw_mgr_ctx, cfg->request_id);
			return rc;
		}

		/*
		 * setting up preview DMA address
		 */
		scratch_hdl = cmd->scratch_buf_hdl;
		rc = cam_mem_get_io_buf(scratch_hdl,
			hw_mgr->img_iommu_hdl,
			&scratch_iova,
			&scratch_mem_len);
		if (rc)
			CAM_ERR(CAM_SBI,
				"Failed to get iova for hdl 0x%x", scratch_hdl);
		else {
			CAM_DBG(CAM_SBI, "2. sbi addr upd %llx", scratch_iova);
			cam_sbi_hw_mgr_invoke_writereg_cmd(hw_mgr,
				(void *)(&scratch_iova),
				cam_sbi_reg_cmd_base_addr_upd_scenario_1);
		}

		/*
		 * Setup registers if we have the list
		 */
		if (cmd->register_set_size > 0) {
			cam_sbi_hw_mgr_init_register_set(hw_mgr,
				(void *)&cmd->register_set,
				cmd->register_set_size,
				cam_sbi_reg_cmd_init);
		}

		// update : TASK2(preview image) setup
		cam_sbi_hw_mgr_invoke_writereg_cmd(hw_mgr,
			(void *)(&cmd->task2_setup),
			cam_sbi_reg_cmd_task2_setup);

		// update : TASK2_ACTION
		cam_sbi_hw_mgr_invoke_writereg_cmd(hw_mgr,
			(void *)(&cmd->task2_action),
			cam_sbi_reg_cmd_task2_action);

		CAM_DBG(CAM_SBI, "frame num %u hdl 0x%x iova 0x%x len %llu",
			cmd->custom_info, scratch_hdl,
			scratch_iova, scratch_mem_len);

		rc = cam_sbi_blob_bw_update_v2(hw_mgr, &cmd->bw_config_v2);
		if (rc)
			CAM_ERR(CAM_SBI, "Bandwidth Update Failed rc: %d", rc);

	} else {
		CAM_ERR(CAM_SBI, "num_hw_update_entries %d rc %d", cfg->num_hw_update_entries, rc);
	}
	CAM_DBG(CAM_SBI, "Exit: Config Done: %llu",  cfg->request_id);

	return rc;
}

static int cam_sbi_mgr_create_debugfs_entry(void)
{
	int rc = 0;

	g_sbi_hw_mgr.debugfs_entry.dentry =
		debugfs_create_dir("camera_sbi", NULL);
	if (!g_sbi_hw_mgr.debugfs_entry.dentry) {
		CAM_ERR(CAM_SBI, "failed to create dentry");
		return -ENOMEM;
	}

	if (!debugfs_create_bool("dump_register", 0644,
		g_sbi_hw_mgr.debugfs_entry.dentry,
		&g_sbi_hw_mgr.debugfs_entry.dump_register)) {
		CAM_ERR(CAM_SBI, "failed to create dump register entry");
		rc = -ENOMEM;
		goto err;
	}

	if (!debugfs_create_file("custom_csid_debug",
		0644,
		g_sbi_hw_mgr.debugfs_entry.dentry, NULL,
		&cam_custom_csid_debug)) {
		CAM_ERR(CAM_ISP, "failed to create cam_custom_csid_debug");
		goto err;
	}


	return rc;

err:
	debugfs_remove_recursive(g_sbi_hw_mgr.debugfs_entry.dentry);
	g_sbi_hw_mgr.debugfs_entry.dentry = NULL;
	return rc;
}

int cam_sbi_mgr_register_device(struct cam_hw_intf *sbi_hw_intf,
	struct cam_iommu_handle *device_iommu
	// ,struct cam_iommu_handle *cdm_iommu
)
{
#if 0 // for workq removal from SBI DD
	struct cam_sbi_device *hw_device;
	char buf[128];
	int i, rc;

	hw_device = &g_sbi_hw_mgr.hw_device[sbi_hw_intf->hw_idx];

	g_sbi_hw_mgr.device_iommu = *device_iommu;
	// g_sbi_hw_mgr.cdm_iommu = *cdm_iommu;

	memcpy(&hw_device->hw_intf, sbi_hw_intf, sizeof(struct cam_hw_intf));

	spin_lock_init(&hw_device->high_req_lock);
	spin_lock_init(&hw_device->normal_req_lock);
	INIT_LIST_HEAD(&hw_device->frame_pending_list_high);
	INIT_LIST_HEAD(&hw_device->frame_pending_list_normal);

	rc = snprintf(buf, sizeof(buf), "cam_sbi_device_submit_worker%d",
		sbi_hw_intf->hw_idx);
	CAM_DBG(CAM_SBI, "Create submit workq for %s", buf);
	rc = cam_req_mgr_workq_create(buf,
		CAM_SBI_WORKQ_NUM_TASK,
		&hw_device->work, CRM_WORKQ_USAGE_NON_IRQ,
		0);
	if (rc) {
		CAM_ERR(CAM_SBI,
			"Unable to create a worker, rc=%d", rc);
		return rc;
	}

	for (i = 0; i < CAM_SBI_WORKQ_NUM_TASK; i++)
		hw_device->work->task.pool[i].payload =
		&hw_device->work_data[i];

	if (hw_device->hw_intf.hw_ops.process_cmd) {
		struct cam_sbi_hw_cmd_set_cb cb_args;

		cb_args.cam_sbi_hw_mgr_cb = cam_sbi_mgr_cb;
		cb_args.data = hw_device;

		rc = hw_device->hw_intf.hw_ops.process_cmd(
			hw_device->hw_intf.hw_priv,
			CAM_SBI_HW_CMD_REGISTER_CB,
			&cb_args, sizeof(cb_args));
		if (rc) {
			CAM_ERR(CAM_SBI, "Register cb failed");
			goto destroy_workqueue;
		}
		CAM_DBG(CAM_SBI, "cb registered");
	}

	if (hw_device->hw_intf.hw_ops.get_hw_caps) {
		rc = hw_device->hw_intf.hw_ops.get_hw_caps(
			hw_device->hw_intf.hw_priv, &hw_device->hw_caps,
			sizeof(hw_device->hw_caps));
		if (rc)
			CAM_ERR(CAM_SBI, "Get caps failed");
	}
	else {
		CAM_ERR(CAM_SBI, "No get_hw_caps function");
		goto destroy_workqueue;
	}
	g_sbi_hw_mgr.sbi_caps.dev_caps[sbi_hw_intf->hw_idx] =
		hw_device->hw_caps;
	g_sbi_hw_mgr.device_count++;
	g_sbi_hw_mgr.sbi_caps.device_iommu = g_sbi_hw_mgr.device_iommu;
	// g_sbi_hw_mgr.sbi_caps.cdm_iommu = g_sbi_hw_mgr.cdm_iommu;
	g_sbi_hw_mgr.sbi_caps.num_devices = g_sbi_hw_mgr.device_count;

	hw_device->valid = true;

	CAM_DBG(CAM_SBI, "device registration done");
	return 0;

destroy_workqueue:
	cam_req_mgr_workq_destroy(&hw_device->work);

	return rc;
#else
	int rc;
	struct cam_sbi_device *hw_device;

	hw_device = &g_sbi_hw_mgr.hw_device[sbi_hw_intf->hw_idx];

	g_sbi_hw_mgr.device_iommu = *device_iommu;
	// g_sbi_hw_mgr.cdm_iommu = *cdm_iommu;

	memcpy(&hw_device->hw_intf, sbi_hw_intf, sizeof(struct cam_hw_intf));

	spin_lock_init(&hw_device->high_req_lock);
	spin_lock_init(&hw_device->normal_req_lock);
	INIT_LIST_HEAD(&hw_device->frame_pending_list_high);
	INIT_LIST_HEAD(&hw_device->frame_pending_list_normal);

	if (hw_device->hw_intf.hw_ops.process_cmd) {
		struct cam_sbi_hw_cmd_set_cb cb_args;

		cb_args.cam_sbi_hw_mgr_cb = cam_sbi_mgr_cb;
		cb_args.data = hw_device;

		rc = hw_device->hw_intf.hw_ops.process_cmd(
			hw_device->hw_intf.hw_priv, CAM_SBI_HW_CMD_REGISTER_CB,
			&cb_args, sizeof(cb_args));
		if (rc) {
			CAM_ERR(CAM_SBI, "Register cb failed");
			// goto destroy_workqueue;
		}
		CAM_DBG(CAM_SBI, "cb registered");
	}

	if (hw_device->hw_intf.hw_ops.get_hw_caps) {
		rc = hw_device->hw_intf.hw_ops.get_hw_caps(
			hw_device->hw_intf.hw_priv, &hw_device->hw_caps,
			sizeof(hw_device->hw_caps));
		if (rc)
			CAM_ERR(CAM_SBI, "Get caps failed");
	} else {
		CAM_ERR(CAM_SBI, "No get_hw_caps function");
		// goto destroy_workqueue;
	}
	g_sbi_hw_mgr.sbi_caps.dev_caps[sbi_hw_intf->hw_idx] =
		hw_device->hw_caps;
	g_sbi_hw_mgr.device_count++;
	g_sbi_hw_mgr.sbi_caps.device_iommu = g_sbi_hw_mgr.device_iommu;
	// g_sbi_hw_mgr.sbi_caps.cdm_iommu = g_sbi_hw_mgr.cdm_iommu;
	g_sbi_hw_mgr.sbi_caps.num_devices = g_sbi_hw_mgr.device_count;

	hw_device->valid = true;

	CAM_DBG(CAM_SBI, "device registration done");

	return 0;
#endif
}

int cam_sbi_mgr_deregister_device(int device_index)
{
#if 0 // for workq removal from SBI DD
	struct cam_sbi_device *hw_device;

	hw_device = &g_sbi_hw_mgr.hw_device[device_index];
	cam_req_mgr_workq_destroy(&hw_device->work);
	memset(hw_device, 0x0, sizeof(struct cam_sbi_device));
	g_sbi_hw_mgr.device_count--;
#else
	struct cam_sbi_device *hw_device;

	hw_device = &g_sbi_hw_mgr.hw_device[device_index];
	memset(hw_device, 0x0, sizeof(struct cam_sbi_device));
	g_sbi_hw_mgr.device_count--;
#endif

	return 0;
}

int cam_sbi_hw_mgr_deinit(void)
{
	mutex_destroy(&g_sbi_hw_mgr.ctx_mutex);
	memset(&g_sbi_hw_mgr, 0x0, sizeof(g_sbi_hw_mgr));

	return 0;
}

int cam_sbi_hw_sub_mod_init(struct cam_hw_intf **custom_hw, uint32_t hw_idx);

int cam_sbi_hw_mgr_init(struct cam_hw_mgr_intf *hw_mgr_intf,
	cam_hw_event_cb_func cam_sbi_dev_buf_done_cb,
	int *iommu_hdl)
{
	int i, j, rc = 0;

	if (!hw_mgr_intf)
		return -EINVAL;

	memset(hw_mgr_intf, 0, sizeof(*hw_mgr_intf));

	mutex_init(&g_sbi_hw_mgr.ctx_mutex);

	CAM_DBG(CAM_SBI, "cam_sbi_hw_mgr_init 0");

	/* fill custom hw intf information */
	for (i = 0; i < CAM_SBI_HW_SUB_MOD_MAX; i++) {
		/* Initialize sub modules */
		rc = cam_sbi_hw_sub_mod_init(&g_sbi_hw_mgr.custom_hw[i], i);
		if (rc) {
			CAM_ERR(CAM_SBI,
				"%s , cam_custom_hw_sub_mod_init failed",
				__FUNCTION__);
		}

		/* handle in case init fails */
		if (g_sbi_hw_mgr.custom_hw[i]) {
			//hw_intf = g_sbi_hw_mgr.custom_hw[i];
			//if (hw_intf->hw_ops.process_cmd) {
			// irq_cb_args.custom_hw_mgr_cb =
			//	cam_custom_hw_mgr_irq_cb;
			// irq_cb_args.data =
			//	g_sbi_hw_mgr.custom_hw[i]->hw_priv;
			//hw_intf->hw_ops.process_cmd(hw_intf->hw_priv,
			//CAM_CUSTOM_SET_IRQ_CB,
			//&irq_cb_args,
			//sizeof(irq_cb_args));
			//}
		}
	}

	CAM_DBG(CAM_SBI, "cam_sbi_hw_mgr_init 1");

	for (i = 0; i < CAM_CUSTOM_CSID_HW_MAX; i++) {
		/* Initialize csid custom modules */
		rc = cam_custom_csid_hw_init(&g_sbi_hw_mgr.csid_devices[i], i);
		if (rc) {
			CAM_ERR(CAM_SBI, "%s , cam_custom_csid_hw_init failed",
				__FUNCTION__);
		}
	}

	CAM_DBG(CAM_SBI, "cam_sbi_hw_mgr_init 2");

	CAM_DBG(CAM_SBI, "device count %d", g_sbi_hw_mgr.device_count);
	if (g_sbi_hw_mgr.device_count > CAM_SBI_HW_MAX) {
		CAM_ERR(CAM_SBI, "Invalid count of devices");
		return -EINVAL;
	}

	CAM_DBG(CAM_SBI, "cam_sbi_hw_mgr_init 3");

	spin_lock_init(&g_sbi_hw_mgr.free_req_lock);
	INIT_LIST_HEAD(&g_sbi_hw_mgr.frame_free_list);
	INIT_LIST_HEAD(&g_sbi_hw_mgr.free_ctx_list);
	INIT_LIST_HEAD(&g_sbi_hw_mgr.used_ctx_list);


	CAM_DBG(CAM_SBI, "cam_sbi_hw_mgr_init 4");

	// SBI-CSID code added
	{
		/*
		 *	for now, we only support one iommu handle. later
		 *	we will need to setup more iommu handle for other
		 *	use cases.
		 *	Also, we have to release them once we have the
		 *	deinit support
		 */
		if (cam_smmu_get_handle("custom",
			&g_sbi_hw_mgr.img_iommu_hdl)) {
			CAM_ERR(CAM_SBI, "Can not get iommu handle");
			return -EINVAL;
		}
		CAM_INFO(CAM_SBI, "handle 0x%x", g_sbi_hw_mgr.img_iommu_hdl);

		for (i = 0; i < CAM_CTX_MAX; i++) {
			memset(&g_sbi_hw_mgr.ctx_pool[i], 0,
				sizeof(g_sbi_hw_mgr.ctx_pool[i]));
			INIT_LIST_HEAD(&g_sbi_hw_mgr.ctx_pool[i].list);

			//ctx_pool = &g_sbi_hw_mgr.ctx_pool[i];

			/* init context pool */
			INIT_LIST_HEAD(&g_sbi_hw_mgr.ctx_pool[i].free_res_list);
			INIT_LIST_HEAD(
				&g_sbi_hw_mgr.ctx_pool[i].res_list_custom_csid);
			INIT_LIST_HEAD(
				&g_sbi_hw_mgr.ctx_pool[i].res_list_custom_cid);
			for (j = 0; j < CAM_CUSTOM_HW_RES_MAX; j++) {
				INIT_LIST_HEAD(&g_sbi_hw_mgr.ctx_pool[i]
					.res_pool[j]
					.list);
				list_add_tail(
					&g_sbi_hw_mgr.ctx_pool[i]
					.res_pool[j]
					.list,
					&g_sbi_hw_mgr.ctx_pool[i].free_res_list);
			}

			g_sbi_hw_mgr.ctx_pool[i].ctx_index = i;
			g_sbi_hw_mgr.ctx_pool[i].hw_mgr = &g_sbi_hw_mgr;

			list_add_tail(&g_sbi_hw_mgr.ctx_pool[i].list,
				&g_sbi_hw_mgr.free_ctx_list);
		}
	}

	hw_mgr_intf->hw_mgr_priv = &g_sbi_hw_mgr;
	hw_mgr_intf->hw_get_caps = cam_sbi_mgr_get_caps;
	hw_mgr_intf->hw_acquire = cam_sbi_hw_mgr_acquire;
	hw_mgr_intf->hw_release = cam_sbi_hw_mgr_release;
	hw_mgr_intf->hw_start = cam_sbi_hw_mgr_start;
	hw_mgr_intf->hw_stop = cam_sbi_hw_mgr_stop;
	hw_mgr_intf->hw_prepare_update = cam_sbi_hw_mgr_prepare_update;
	hw_mgr_intf->hw_config = cam_sbi_hw_mgr_config;
	hw_mgr_intf->hw_reset = cam_sbi_hw_mgr_reset;
	hw_mgr_intf->hw_read = NULL;
	hw_mgr_intf->hw_write = NULL;
	hw_mgr_intf->hw_close = NULL;

	g_sbi_hw_mgr.event_cb = cam_sbi_dev_buf_done_cb;

	if (iommu_hdl)
		*iommu_hdl = g_sbi_hw_mgr.img_iommu_hdl;

	cam_sbi_mgr_create_debugfs_entry();

	CAM_DBG(CAM_SBI, "Hw mgr init done");
	return rc;
}
