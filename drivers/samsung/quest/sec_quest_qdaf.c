/*
 * drivers/debug/sec_quest_qdaf.c
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

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/sec_class.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/fcntl.h>
#include <linux/syscalls.h>

#include <linux/uaccess.h>
#include <linux/umh.h>
#include <linux/sec_quest.h>
#include <linux/sec_quest_qdaf.h>


extern struct device *sec_nad;
static int qdaf_cur_cmd_mode;


//////////////////////////////////////////////////
/////// helper functions /////////////////////////////
//////////////////////////////////////////////////
static int call_user_prg( char **argv, int wait )
{
	int ret_userapp;
	char *envp[5] = { 
		"HOME=/", 
		"PATH=/system/bin/quest:/system/bin:/system/xbin", 
		"ANDROID_DATA=/data", 
		"ANDROID_ROOT=/system", 
		NULL };

	ret_userapp = call_usermodehelper(argv[0], argv, envp, wait);
	if (!ret_userapp) {
		QUEST_PRINT("%s is executed. ret_userapp = %d\n", argv[0], ret_userapp);
		return 0;
	} else {
		QUEST_PRINT("%s is NOT executed. ret_userapp = %d\n", argv[0], ret_userapp);
		return ret_userapp;
	}		
}

int get_qdaf_failed_cnt()
{
	int ret_userapp, wait_val=0;
	char *argv[4] = { NULL, NULL, NULL, NULL };
	char qdaf_cmd[BUFF_SZ];

	argv[0] = QDAF_PROG;
	snprintf(qdaf_cmd, BUFF_SZ, "%d", QUEST_QDAF_ACTION_RESULT_GET);
	argv[1] = qdaf_cmd;
	wait_val = UMH_WAIT_PROC;
	
	ret_userapp = call_user_prg(argv, wait_val);	

	// evaulate return value after ret_userapp>>8	
	// qdaf.sh exit with failed_cnt if persist.qdaf.failed_cnt>0
	// so let's use return value from call_usermodehelper	
	return (ret_userapp>=0)? (ret_userapp>>8) : 0;
}

int call_qdaf_from_quest_driver(int cmd_mode, int wait_val)
{
	int ret_userapp;
	char *argv[4] = { NULL, NULL, NULL, NULL };
	char qdaf_cmd[BUFF_SZ];

	QUEST_PRINT("%s : cmd_mode=%d, wait_val=%d\n", __func__, cmd_mode, wait_val);

	// let's just return if receiving the same command as before one
	if ( qdaf_cur_cmd_mode==cmd_mode ) {
		if( cmd_mode==QUEST_QDAF_ACTION_CONTROL_START_WITHOUT_PANIC ||
			cmd_mode==QUEST_QDAF_ACTION_CONTROL_START_WITH_PANIC) {
			QUEST_PRINT("%s : return because qdaf.sh already has been running.\n", __func__);
		}else if( cmd_mode==QUEST_QDAF_ACTION_CONTROL_STOP || 
				cmd_mode==QUEST_QDAF_ACTION_CONTROL_STOP_WATING) {
			QUEST_PRINT("%s : return because qdaf.sh has not been running yet.\n", __func__);
		}
		return cmd_mode;
	}	

	argv[0] = QDAF_PROG;
	snprintf(qdaf_cmd, BUFF_SZ, "%d", cmd_mode);
	argv[1] = qdaf_cmd;

	ret_userapp = call_user_prg(argv, wait_val);
	if (!ret_userapp) qdaf_cur_cmd_mode = cmd_mode;
	else qdaf_cur_cmd_mode = QUEST_QDAF_ACTION_EMPTY;
	
	return ret_userapp;
}

static void qdaf_save_logs(void)
{
	int fd, idx=0;
	char temp[1]={'\0'}, buf[BUFF_SZ]={'\0',};

	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,17,0)	
	fd = sys_open(QDAF_QMESA_LOG, O_RDONLY, 0);
#else	
	fd = ksys_open(QDAF_QMESA_LOG, O_RDONLY, 0);
#endif	
	if (fd >= 0) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,17,0)		
		while (sys_read(fd, temp, 1) == 1) {
#else		
		while (ksys_read(fd, temp, 1) == 1) {
#endif		
			buf[idx++] = temp[0];
			if( temp[0]=='\n' ) {
				buf[idx] = '\0';
				QUEST_PRINT("%s", buf);
				idx = 0;
			}
		}
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,17,0)
		sys_close(fd);
#else					
		ksys_close(fd);
#endif		
	}
	set_fs(old_fs);
}
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// NAD_QDAF_CONTROL ////////////////////////
//////////////////////////////////////////////////
static ssize_t show_quest_qdaf_control(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	if( qdaf_cur_cmd_mode==QUEST_QDAF_ACTION_CONTROL_START_WITHOUT_PANIC ||
		qdaf_cur_cmd_mode==QUEST_QDAF_ACTION_CONTROL_START_WITH_PANIC )
		return snprintf(buf, BUFF_SZ, "RUNNING\n");

	return snprintf(buf, BUFF_SZ, "READY\n");
}
static ssize_t store_quest_qdaf_control(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int idx = 0, cmd_mode=0, wait_val=0;
	char temp[QUEST_BUFF_SIZE * 3];
	char quest_cmd[QUEST_CMD_LIST][QUEST_BUFF_SIZE];
	char *quest_ptr, *string;
	int ret_userapp;
	char *argv[4] = { NULL, NULL, NULL, NULL };

	QUEST_PRINT("%s : is called\n",__func__);

	if ((int)count < QUEST_BUFF_SIZE) {
		QUEST_PRINT("%s : return error (count=%d)\n", __func__, (int)count);
		return -EINVAL;;
	}

	if (strncmp(buf, "nad_qdaf_control", 16)) {
		QUEST_PRINT("%s : return error (buf=%s)\n", __func__, buf);
		return -EINVAL;;
	}

	strlcpy(temp, buf, QUEST_BUFF_SIZE * 3);
	string = temp;
	while (idx < QUEST_CMD_LIST) {
		quest_ptr = strsep(&string, ",");
		strlcpy(quest_cmd[idx++], quest_ptr, QUEST_BUFF_SIZE);
	}
	sscanf(quest_cmd[1], "%d", &cmd_mode);

	// let's just return if receiving the same command as before one
	if ( qdaf_cur_cmd_mode==cmd_mode ) {
		if( cmd_mode==QUEST_QDAF_ACTION_CONTROL_START_WITHOUT_PANIC ||
			cmd_mode==QUEST_QDAF_ACTION_CONTROL_START_WITH_PANIC) {
			QUEST_PRINT("%s : return because qdaf.sh already has been running.\n", __func__);
		}else if( cmd_mode==QUEST_QDAF_ACTION_CONTROL_STOP || 
				cmd_mode==QUEST_QDAF_ACTION_CONTROL_STOP_WATING) {
			QUEST_PRINT("%s : return because qdaf.sh has not been running yet.\n", __func__);
		}
		return count;
	}

	argv[0] = QDAF_PROG;
	argv[1] = quest_cmd[1];
	switch (cmd_mode) {
	case QUEST_QDAF_ACTION_CONTROL_START_WITHOUT_PANIC :
		QUEST_PRINT("%s : qdaf will be started (without panic)\n",__func__);
		wait_val = UMH_WAIT_EXEC;
		break;		
	case QUEST_QDAF_ACTION_CONTROL_START_WITH_PANIC:
		QUEST_PRINT("%s : qdaf will be started (with panic)\n",__func__);
		wait_val = UMH_WAIT_EXEC;		
		break;
	case QUEST_QDAF_ACTION_CONTROL_STOP :
		QUEST_PRINT("%s : qdaf will be stopped\n",__func__);
		wait_val = UMH_WAIT_EXEC;		
		break;
	case QUEST_QDAF_ACTION_CONTROL_STOP_WATING :
		QUEST_PRINT("%s : qdaf will be stopped (waiting)\n",__func__);
		// should wait for completion to gurantee not working of qdaf
		wait_val = UMH_WAIT_PROC;
		break;
	default :
		QUEST_PRINT("%s : return because invalid cmd mode(%d)\n", __func__, cmd_mode);
		return count;
	}

	ret_userapp = call_user_prg(argv, wait_val);
	if (!ret_userapp) qdaf_cur_cmd_mode = cmd_mode;
	else qdaf_cur_cmd_mode = QUEST_QDAF_ACTION_EMPTY;

	return count;
}
static DEVICE_ATTR(nad_qdaf_control, S_IRUGO | S_IWUSR, show_quest_qdaf_control, store_quest_qdaf_control);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// NAD_QDAF_RESULT //////////////////////////
//////////////////////////////////////////////////
static ssize_t show_quest_qdaf_result(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	int failed_cnt;
	
	QUEST_PRINT("%s : is called\n",__func__);

	failed_cnt = get_qdaf_failed_cnt();
	QUEST_PRINT("%s : failed_cnt = %d\n", __func__, failed_cnt);
	
	if( failed_cnt == 0 )
		return snprintf(buf, BUFF_SZ, "OK\n");
	else if ( failed_cnt > 0 ) 
		return snprintf(buf, BUFF_SZ, "NG,%d\n", failed_cnt);		

	return snprintf(buf, BUFF_SZ, "NONE\n");
}

static ssize_t store_quest_qdaf_result(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int idx = 0, cmd_mode=0, wait_val=0;
	char temp[QUEST_BUFF_SIZE * 3];
	char quest_cmd[QUEST_CMD_LIST][QUEST_BUFF_SIZE];
	char *quest_ptr, *string;
	char *argv[4] = { NULL, NULL, NULL, NULL };

	QUEST_PRINT("%s : is called\n",__func__);

	if ((int)count < QUEST_BUFF_SIZE) {
		QUEST_PRINT("%s : return error (count=%d)\n", __func__, (int)count);
		return -EINVAL;;
	}

	if (strncmp(buf, "nad_qdaf_result", 15)) {
		QUEST_PRINT("%s : return error (buf=%s)\n", __func__, buf);
		return -EINVAL;;
	}

	strlcpy(temp, buf, QUEST_BUFF_SIZE * 3);
	string = temp;
	while (idx < QUEST_CMD_LIST) {
		quest_ptr = strsep(&string, ",");
		strlcpy(quest_cmd[idx++], quest_ptr, QUEST_BUFF_SIZE);
	}
	sscanf(quest_cmd[1], "%d", &cmd_mode);	
	
	argv[0] = QDAF_PROG;
	argv[1] = quest_cmd[1];
	switch (cmd_mode) {
	case QUEST_QDAF_ACTION_RESULT_ERASE :
		QUEST_PRINT("%s : qdaf will erase failed count\n",__func__);
		wait_val = UMH_WAIT_PROC;
		break;
	default :
		QUEST_PRINT("%s : return because invalid cmd mode(%d)\n", __func__, cmd_mode);
		return count;
	}

	call_user_prg(argv, wait_val);
	
	return count;
}
static DEVICE_ATTR(nad_qdaf_result, S_IRUGO | S_IWUSR, show_quest_qdaf_result, store_quest_qdaf_result);
//////////////////////////////////////////////////



//////////////////////////////////////////////////
/////// NAD_QDAF_DEBUG //////////////////////////
//////////////////////////////////////////////////
static ssize_t store_quest_qdaf_debug(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int idx = 0, cmd_mode=0;
	char temp[QUEST_BUFF_SIZE * 3];
	char quest_cmd[QUEST_CMD_LIST][QUEST_BUFF_SIZE];
	char *quest_ptr, *string;

	QUEST_PRINT("%s : is called\n",__func__);

	if ((int)count < QUEST_BUFF_SIZE) {
		QUEST_PRINT("%s : return error (count=%d)\n", __func__, (int)count);
		return -EINVAL;;
	}

	if (strncmp(buf, "nad_qdaf_debug", 14)) {
		QUEST_PRINT("%s : return error (buf=%s)\n", __func__, buf);
		return -EINVAL;;
	}

	strlcpy(temp, buf, QUEST_BUFF_SIZE * 3);
	string = temp;
	while (idx < QUEST_CMD_LIST) {
		quest_ptr = strsep(&string, ",");
		strlcpy(quest_cmd[idx++], quest_ptr, QUEST_BUFF_SIZE);
	}
	sscanf(quest_cmd[1], "%d", &cmd_mode);

	switch (cmd_mode) {
	case QUEST_QDAF_ACTION_DEBUG_SAVE_LOGS :
		QUEST_PRINT("%s : qdaf will save log into kmsg\n",__func__);
		qdaf_save_logs();
		return count;
	case QUEST_QDAF_ACTION_DEBUG_TRIGGER_PANIC :
		QUEST_PRINT("%s : will trigger panic\n",__func__);
		panic("qdaf_fail");
		return count;
	}

	return count;
}
static DEVICE_ATTR(nad_qdaf_debug, S_IWUSR, NULL, store_quest_qdaf_debug);

//////////////////////////////////////////////////



static int __init sec_quest_qdaf_init(void)
{
	int ret = 0;

	QUEST_PRINT("%s\n", __func__);

	ret = device_create_file(sec_nad, &dev_attr_nad_qdaf_control);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_qdaf_debug);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_qdaf_result);
	if (ret) {
		QUEST_PRINT("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	return 0;
err_create_nad_sysfs:
	return ret;		
}	

module_init(sec_quest_qdaf_init);
