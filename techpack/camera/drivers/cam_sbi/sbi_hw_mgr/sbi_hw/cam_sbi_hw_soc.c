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

#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "cam_sbi_hw_core.h"
#include "cam_sbi_hw_soc.h"


int cam_sbi_soc_enable_resources(struct cam_hw_info *sbi_hw)
{
	struct cam_hw_soc_info *soc_info = &sbi_hw->soc_info;
	struct cam_sbi_soc_private *soc_private =
		(struct cam_sbi_soc_private *)soc_info->soc_private;
	struct cam_ahb_vote ahb_vote;
	struct cam_axi_vote axi_vote;
	int rc = 0;
	enum cam_vote_level clk_level;

	ahb_vote.type = CAM_VOTE_ABSOLUTE;
	ahb_vote.vote.level = CAM_SVS_VOTE;

	// axi_vote.compressed_bw = 7200000;
	// axi_vote.uncompressed_bw = 7200000;
	axi_vote.num_paths = 2;
	axi_vote.axi_path[0].path_data_type = CAM_AXI_PATH_DATA_ALL;
	axi_vote.axi_path[0].transac_type = CAM_AXI_TRANSACTION_READ;
	axi_vote.axi_path[0].camnoc_bw = 4032 * 3024 * 10 / 8 * 120;
	axi_vote.axi_path[0].mnoc_ab_bw = 4032 * 3024 * 10 / 8 * 120;
	axi_vote.axi_path[0].mnoc_ib_bw = 4032 * 3024 * 10 / 8 * 120;
	axi_vote.axi_path[1].path_data_type = CAM_AXI_PATH_DATA_ALL;
	axi_vote.axi_path[1].transac_type = CAM_AXI_TRANSACTION_WRITE;
	axi_vote.axi_path[1].camnoc_bw = 4032 * 3024 * 10 / 8 * 120;
	axi_vote.axi_path[1].mnoc_ab_bw = 4032 * 3024 * 10 / 8 * 120;
	axi_vote.axi_path[1].mnoc_ib_bw = 4032 * 3024 * 10 / 8 * 120;


	CAM_DBG(CAM_SBI, "cpas_handle is null ? %d", soc_private->cpas_handle == 0 ? 0:1);
	rc = cam_cpas_start(soc_private->cpas_handle, &ahb_vote, &axi_vote);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed to start cpas, rc %d", rc);
		return -EFAULT;
	} else {
		CAM_DBG(CAM_SBI, "succeeded to start cpas, rc %d", rc);
	}

	/* Get clock level */
	{
		int clock_level;
		struct cam_sbi_core *sbi_core = sbi_hw->core_info;
		sbi_core = sbi_hw->core_info;

		rc = cam_soc_util_get_clk_level(soc_info, sbi_core->clock_rate,
				soc_info->src_clk_idx, &clock_level);
		if (rc) {
			CAM_ERR(CAM_SBI, "Failed to get clk level for rate %d",
					sbi_core->clock_rate);
			clk_level = CAM_SVS_VOTE;
		} else {
			clk_level = (enum cam_vote_level)clock_level;
		}
	}

	rc = cam_soc_util_enable_platform_resource(soc_info, true, clk_level, true);
	if (rc) {
		CAM_ERR(CAM_SBI,
			"Failed to enable platform resource, rc %d", rc);
		goto stop_cpas;
	}

	//cam_sbi_set_irq(sbi_hw, CAM_SBI_IRQ_ENABLE);

	return rc;

stop_cpas:
	if (cam_cpas_stop(soc_private->cpas_handle))
		CAM_ERR(CAM_SBI, "Failed to stop cpas");

	return rc;
}

int cam_sbi_soc_disable_resources(struct cam_hw_info *sbi_hw)
{
	struct cam_sbi_core *sbi_core;
	struct cam_hw_soc_info *soc_info = &sbi_hw->soc_info;
	struct cam_sbi_soc_private *soc_private;
	int rc = 0;

	soc_private = soc_info->soc_private;

	sbi_core = (struct cam_sbi_core *)sbi_hw->core_info;
	if (sbi_core->ssm_info.is_ssm_mode == true) {
		CAM_INFO(CAM_SBI, "cam_cc_sbi_ife_1_clk disable");
		cam_soc_util_clk_disable(soc_private->task3_clk, CAM_TASK3_CLK_NAME);
	}

	cam_sbi_set_irq(sbi_hw, CAM_SBI_IRQ_DISABLE);

	rc = cam_soc_util_disable_platform_resource(soc_info, true, true);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed to disable platform resource");
		return rc;
	}
	rc = cam_cpas_stop(soc_private->cpas_handle);
	if (rc)
		CAM_ERR(CAM_SBI, "Failed to stop cpas");

	return rc;
}

int cam_sbi_soc_init_resources(struct cam_hw_soc_info *soc_info,
	irq_handler_t irq_handler, void *private_data)
{
	struct cam_sbi_soc_private *soc_private;
	struct cam_cpas_register_params cpas_register_param;
	int rc;

	CAM_INFO(CAM_SBI, "enter");

	rc = cam_soc_util_get_dt_properties(soc_info);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed in get_dt_properties, rc=%d", rc);
		return rc;
	}

	rc = cam_soc_util_request_platform_resource(soc_info, irq_handler,
		private_data);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed in request_platform_resource rc=%d",
			rc);
		return rc;
	} else {
		CAM_DBG(CAM_SBI, "request_platform_resource success");
	}

	soc_private = kzalloc(sizeof(struct cam_sbi_soc_private), GFP_KERNEL);
	if (!soc_private) {
		rc = -ENOMEM;
		goto release_res;
	}
	soc_info->soc_private = soc_private;

	rc = cam_soc_util_get_option_clk_by_name(soc_info,
		CAM_TASK3_CLK_NAME, &soc_private->task3_clk,
		&soc_private->task3_clk_index,
		&soc_private->task3_clk_rate);

	memset(&cpas_register_param, 0, sizeof(cpas_register_param));
	strlcpy(cpas_register_param.identifier,
		"custom", CAM_HW_IDENTIFIER_LENGTH);
	cpas_register_param.cell_index = soc_info->index;
	cpas_register_param.dev = &soc_info->pdev->dev;
	cpas_register_param.userdata = private_data;
	cpas_register_param.cam_cpas_client_cb = NULL;
	CAM_DBG(CAM_SBI, "-- cpas_register_param --");
	CAM_DBG(CAM_SBI, "cpas_register_param.cell_index = %d",cpas_register_param.cell_index);

#if 1 //defined(SBI_ENABLE_REAL_DD)
	rc = cam_cpas_register_client(&cpas_register_param);
	if (rc) {
		CAM_ERR(CAM_SBI, "CPAS registration failed");
		goto free_soc_private;
	} else {
		CAM_DBG(CAM_SBI, "CPAS registration success");
	}
#endif
	soc_private->cpas_handle = cpas_register_param.client_handle;
	CAM_DBG(CAM_SBI, "CPAS handle=%d", soc_private->cpas_handle);

	return rc;

#if 1 //defined(SBI_ENABLE_REAL_DD)
free_soc_private:
	kfree(soc_info->soc_private);
	soc_info->soc_private = NULL;
#endif
release_res:
	cam_soc_util_release_platform_resource(soc_info);

	return rc;
}

int cam_sbi_soc_deinit_resources(struct cam_hw_soc_info *soc_info)
{
	struct cam_sbi_soc_private *soc_private =
		(struct cam_sbi_soc_private *)soc_info->soc_private;
	int rc;

	rc = cam_cpas_unregister_client(soc_private->cpas_handle);
	if (rc)
		CAM_ERR(CAM_SBI, "Unregister cpas failed, handle=%d, rc=%d",
			soc_private->cpas_handle, rc);

	rc = cam_soc_util_release_platform_resource(soc_info);
	if (rc)
		CAM_ERR(CAM_SBI, "release platform failed, rc=%d", rc);

	kfree(soc_info->soc_private);
	soc_info->soc_private = NULL;

	return rc;
}
