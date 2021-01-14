/*
 * include/linux/sec_quest_param.h
 *
 * COPYRIGHT(C) 2006-2017 Samsung Electronics Co., Ltd. All Right Reserved.
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

#ifndef SEC_QUEST_PARAM_H
#define SEC_QUEST_PARAM_H

void quest_print_param_quest_data(void);
void quest_load_param_quest_data(void);
void quest_sync_param_quest_data(void);
void quest_clear_param_quest_data(void);
void quest_initialize_curr_step(void);
void quest_load_param_quest_ddr_result_data(void);
void quest_sync_param_quest_ddr_result_data(void);
void quest_load_param_api_gpio_test(void);
void quest_sync_param_api_gpio_test(void);
void quest_load_param_api_gpio_test_result(void);
void quest_sync_param_api_gpio_test_result(void);
#ifdef CONFIG_SEC_QUEST_BPS_CLASSIFIER
void quest_load_param_quest_bps_data(void);
void quest_sync_param_quest_bps_data(void);
#endif

#endif
