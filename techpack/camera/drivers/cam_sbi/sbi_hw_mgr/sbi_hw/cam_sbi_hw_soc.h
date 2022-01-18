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

#ifndef _CAM_SBI_HW_SOC_H_
#define _CAM_SBI_HW_SOC_H_

#include "cam_soc_util.h"

#define CAM_TASK3_CLK_NAME "sbi_ife_1_clk"

/*
 * struct cam_custom_hw_soc_private:
 *
 * @Brief:                   Private SOC data specific to Custom HW Driver
 *
 * @cpas_handle:             Handle returned on registering with CPAS driver.
 *                           This handle is used for all further interface
 *                           with CPAS.
 * @task3_clk:               Optional clock of the HW
 * @task3_clk_index:         clk idx of the above clk
 * @task3_clk_rate:          clk rate of the optional clk
 */
struct cam_sbi_soc_private {
	uint32_t    cpas_handle;
	struct clk *task3_clk;
	int32_t     task3_clk_index;
	int32_t     task3_clk_rate;
};

int cam_sbi_soc_enable_resources(struct cam_hw_info *sbi_hw);
int cam_sbi_soc_disable_resources(struct cam_hw_info *sbi_hw);
int cam_sbi_soc_init_resources(struct cam_hw_soc_info *soc_info,
	irq_handler_t irq_handler, void *private_data);
int cam_sbi_soc_deinit_resources(struct cam_hw_soc_info *soc_info);

#endif /* _CAM_SBI_HW_SOC_H_ */
