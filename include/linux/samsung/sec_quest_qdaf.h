/*
 * include/linux/sec_quest_qdaf.h
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

#ifndef SEC_QUEST_QDAF_H
#define SEC_QUEST_QDAF_H

enum quest_qdaf_action_t {
	QUEST_QDAF_ACTION_EMPTY = 0,
		
	/*==== from AtNadCheck.java to qdaf.sh ====*/
	QUEST_QDAF_ACTION_CONTROL_START_WITHOUT_PANIC = 1,
	QUEST_QDAF_ACTION_CONTROL_START_WITH_PANIC = 2,	
	QUEST_QDAF_ACTION_CONTROL_STOP = 3,
	/*==== from quest.sh to qdaf.sh ====*/	
	QUEST_QDAF_ACTION_CONTROL_STOP_WATING = 4,	

	/*==== from AtNadCheck.java to qdaf.sh ====*/
	QUEST_QDAF_ACTION_RESULT_ERASE = 5,
	QUEST_QDAF_ACTION_RESULT_GET = 6,		

	/*==== from qdaf.sh ====*/
	QUEST_QDAF_ACTION_DEBUG_SAVE_LOGS = 7,
	QUEST_QDAF_ACTION_DEBUG_TRIGGER_PANIC = 8,
};

enum quest_qdaf_result_string_t {
	QUEST_QDAF_RESULT_OK = 0,
	QUEST_QDAF_RESULT_NG = 1,
	QUEST_QDAF_RESULT_NONE = 2,
};

#define QDAF_PROG "/system/bin/quest/qdaf.sh"
#define QDAF_QMESA_LOG "/data/log/qdaf_qmesa_log.txt"


int call_qdaf_from_quest_driver(int cmd_mode, int wait_val);
int get_qdaf_failed_cnt(void);


#endif
