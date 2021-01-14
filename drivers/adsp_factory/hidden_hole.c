/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/dirent.h>
#include "adsp.h"

#define PREDEFINE_FILE_PATH      "/efs/FactoryApp/predefine"
#define HH_VERSION_FILE_PATH      "/efs/FactoryApp/hh_version"

struct {
	int version;
	int octa_id;
	int data[EFS_SAVE_NUMS];
} hidden_table[ID_INDEX_NUMS] = {
#if defined(CONFIG_SEC_WINNERLTE_PROJECT) || defined(CONFIG_SEC_WINNERX_PROJECT)
	{190201, ID_UTYPE,
	{944, -844, 2014, -2855, 1490, 1615, 1381, 1100, 800, 550, 6195} },
	{190201, ID_BLACK,
	{944, -844, 2014, -2855, 1490, 1615, 1381, 1100, 800, 550, 6195} },
#else
	{180911, ID_UTYPE, /* beyond project */
	{3589, -947, -425, -1777, 1754, 1116, 1500, 320, 250, 550, 5930} },
	{180911, ID_BLACK,
	{3589, -947, -425, -1777, 1754, 1116, 1500, 320, 250, 550, 5930} },
#endif
};

/*************************************************************************/
/* factory Sysfs							 */
/*************************************************************************/

static int read_window_type(void)
{
	struct file *type_filp = NULL;
	mm_segment_t old_fs;
	int ret = 0;
	char window_type[10] = {0, };

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	type_filp = filp_open("/sys/class/lcd/panel/window_type",
		O_RDONLY, 0440);
	if (IS_ERR(type_filp)) {
		ret = PTR_ERR(type_filp);
		pr_err("[FACTORY] %s: open fail window_type:%d\n",
			__func__, ret);
		goto err_open_exit;
	}

	ret = vfs_read(type_filp, (char *)window_type,
		10 * sizeof(char), &type_filp->f_pos);
	if (ret < 0) {
		pr_err("[FACTORY] %s: fd read fail:%d\n", __func__, ret);
		ret = -EIO;
		goto err_read_exit;
	}

	pr_info("[FACTORY] %s - 0x%x, 0x%x, 0x%x", __func__,
		window_type[0], window_type[1], window_type[2]);
	ret = (window_type[1] - '0') & 0x0f;

err_read_exit:
	filp_close(type_filp, current->files);
err_open_exit:
	set_fs(old_fs);

	return ret;
}

static char *tmd90x_strtok_first_dot(char *str)
{
	static char *s;
	int i, len;

	if (str == NULL || *str == '\0')
		return NULL;

	s = str;
	len = (int)strlen(str);
	for (i = 0 ; i < len; i++) {
		if (s[i] == '.') {
			s[i] = '\0';
			return s;
		}
	}

	return s;
}

static int need_update_coef_efs(void)
{
	struct file *type_filp = NULL;
	mm_segment_t old_fs;
	int ret = 0, current_coef_version = 0;
	char coef_version[VERSION_FILE_NAME_LEN] = {0, };
	char *temp_version;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	type_filp = filp_open(HH_VERSION_FILE_PATH, O_RDONLY, 0440);
	if (PTR_ERR(type_filp) == -ENOENT || PTR_ERR(type_filp) == -ENXIO) {
		pr_err("[FACTORY] %s : no version file\n", __func__);
		set_fs(old_fs);
		return true;
	} else if (IS_ERR(type_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(type_filp);
		pr_err("[FACTORY] %s: open fail version:%d\n", __func__, ret);
		return ret;
	}

	ret = vfs_read(type_filp, (char *)coef_version,
		VERSION_FILE_NAME_LEN * sizeof(char), &type_filp->f_pos);
	if (ret < 0) {
		pr_err("[FACTORY] %s: fd read fail:%d\n", __func__, ret);
		ret = -EIO;
	}

	filp_close(type_filp, current->files);
	set_fs(old_fs);

	temp_version = tmd90x_strtok_first_dot(coef_version);
	if (temp_version == NULL) {
		pr_err("[FACTORY] %s : Dot NULL.\n", __func__);
		return false;
	}

	ret = kstrtoint(temp_version, 10, &current_coef_version);
	pr_info("[FACTORY] %s: %s,%s,%d\n",
		__func__, coef_version, temp_version, current_coef_version);

	if (ret < 0) {
		pr_err("[FACTORY] %s : kstrtoint failed:%d\n", __func__, ret);
		return ret;
	}

	if (current_coef_version < hidden_table[ID_INDEX_NUMS - 1].version) {
		pr_err("[FACTORY] %s : small:%d:%d", __func__,
			current_coef_version,
			hidden_table[ID_INDEX_NUMS - 1].version);
		return true;
	}

	return false;
}

int check_crc_table(int index)
{
	int i, sum = 0;

	for (i = 0; i < SUM_CRC; i++)
		sum += hidden_table[index].data[i];

	return (sum == hidden_table[index].data[SUM_CRC]) ? true : false;
}

int make_coef_efs(int index)
{
	struct file *type_filp = NULL;
	mm_segment_t old_fs;
	int ret = 0;
	char *predefine_value_path = kzalloc(PATH_LEN + 1, GFP_KERNEL);
	char *write_buf = kzalloc(FILE_BUF_LEN, GFP_KERNEL);

	snprintf(predefine_value_path, PATH_LEN,
		"%s%d", PREDEFINE_FILE_PATH, hidden_table[index].octa_id);

	pr_info("[FACTORY] %s: path:%s\n", __func__,
		predefine_value_path);

	sprintf(write_buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		hidden_table[index].data[0], hidden_table[index].data[1],
		hidden_table[index].data[2], hidden_table[index].data[3],
		hidden_table[index].data[4], hidden_table[index].data[5],
		hidden_table[index].data[6], hidden_table[index].data[7],
		hidden_table[index].data[8], hidden_table[index].data[9],
		hidden_table[index].data[10]);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	type_filp = filp_open(predefine_value_path,
			O_TRUNC | O_RDWR | O_CREAT, 0660);

	if (IS_ERR(type_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(type_filp);
		pr_err("[FACTORY] %s: open fail predefine_value_path:%d\n",
			__func__, ret);
		kfree(write_buf);
		kfree(predefine_value_path);
		return ret;
	}

	ret = vfs_write(type_filp, (char *)write_buf,
		FILE_BUF_LEN * sizeof(char), &type_filp->f_pos);
	if (ret < 0) {
		pr_err("[FACTORY] %s: fd write %d\n", __func__, ret);
		ret = -EIO;
	}

	filp_close(type_filp, current->files);
	set_fs(old_fs);
	kfree(write_buf);
	kfree(predefine_value_path);

	return ret;
}

static void tmd490x_itoa(char *buf, int v)
{
	int mod[10];
	int i;

	for (i = 0; i < 3; i++) {
		mod[i] = (v % 10);
		v = v / 10;
		if (v == 0)
			break;
	}

	if (i == 3)
		i--;

	if (i >= 1)
		*buf = (char) ('a' + mod[0]);
	else
		*buf = (char) ('0' + mod[0]);

	buf++;
	*buf = '\0';
}

int update_coef_version(void)
{
	struct file *type_filp = NULL;
	mm_segment_t old_fs;
	char version[VERSION_FILE_NAME_LEN] = {0,};
	char tmp[5] = {0,};
	int i = 0, ret = 0;

	sprintf(version, "%d.",	hidden_table[ID_INDEX_NUMS - 1].version);

	for (i = 0 ; i < ID_INDEX_NUMS ; i++) {
		if (check_crc_table(i)) {
			tmd490x_itoa(tmp, hidden_table[i].octa_id);
			strncat(version, tmp, 1);
			pr_err("[FACTORY] %s: version:%s\n", __func__, version);
		}
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	type_filp = filp_open(HH_VERSION_FILE_PATH,
			O_TRUNC | O_RDWR | O_CREAT, 0660);
	if (IS_ERR(type_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(type_filp);
		pr_err("[FACTORY] %s: open fail version:%d\n", __func__, ret);
		return ret;
	}

	ret = vfs_write(type_filp, (char *)version,
		VERSION_FILE_NAME_LEN * sizeof(char), &type_filp->f_pos);
	if (ret < 0) {
		pr_err("[FACTORY] %s: fd write fail:%d\n", __func__, ret);
		ret = -EIO;
	}

	filp_close(type_filp, current->files);
	set_fs(old_fs);

	return ret;
}

void hidden_hole_init_work(void)
{
	struct file *hole_filp = NULL;
	int msg_buf[EFS_SAVE_NUMS];
	mm_segment_t old_fs;
	int ret = 0, win_type = 0, i;
	char *predefine_value_path = kzalloc(PATH_LEN + 1, GFP_KERNEL);
	char *read_buf = kzalloc(FILE_BUF_LEN * sizeof(char), GFP_KERNEL);

	pr_info("[FACTORY] %s: start!\n", __func__);
	if (need_update_coef_efs()) {
		for (i = 0 ; i < ID_INDEX_NUMS ; i++) {
			if (!check_crc_table(i)) {
				pr_err("[FACTORY] %s: CRC check fail (%d)\n",
					__func__, i);
				ret = -1;
			}
		}

		if (ret == 0) {
			for (i = 0 ; i < ID_INDEX_NUMS ; i++) {
				ret = make_coef_efs(i);
				if (ret < 0)
					pr_err("[FACTORY] %s: NUCE fail:%d\n",
					__func__, i);
			}
			update_coef_version();
		} else {
			pr_err("[FACTORY] %s: can't not update/make coef_efs\n",
				__func__);
		}
	}

	win_type = read_window_type();
	if (win_type >= 0)
		snprintf(predefine_value_path, PATH_LEN,
			"%s%d", PREDEFINE_FILE_PATH, win_type);
	else {
		pr_err("[FACTORY] %s: win_type fail\n", __func__);
		goto exit;
	}

	pr_info("[FACTORY] %s: win_type:%d, %s\n",
		__func__, win_type, predefine_value_path);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	hole_filp = filp_open(predefine_value_path, O_RDONLY, 0440);
	if (IS_ERR(hole_filp)) {
		ret = PTR_ERR(hole_filp);
		pr_err("[FACTORY] %s - Can't open hidden hole file:%d\n",
			__func__, ret);
		set_fs(old_fs);
		goto exit;
	}

	ret = vfs_read(hole_filp, (char *)read_buf,
		FILE_BUF_LEN * sizeof(char), &hole_filp->f_pos);
	if (ret < 0) {
		pr_err("[FACTORY] %s: fd read fail:%d\n", __func__, ret);
		filp_close(hole_filp, current->files);
		set_fs(old_fs);
		goto exit;
	}

	filp_close(hole_filp, current->files);
	set_fs(old_fs);

	ret = sscanf(read_buf, "%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d",
		&msg_buf[0], &msg_buf[1], &msg_buf[2], &msg_buf[3],
		&msg_buf[4], &msg_buf[5], &msg_buf[6], &msg_buf[7],
		&msg_buf[8], &msg_buf[9], &msg_buf[10]);
	if (ret != EFS_SAVE_NUMS) {
		pr_err("[FACTORY] %s: sscanf fail:%d\n", __func__, ret);
		goto exit;
	}

	pr_info("[FACTORY] %s: data: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		__func__, msg_buf[0], msg_buf[1], msg_buf[2], msg_buf[3],
		msg_buf[4], msg_buf[5], msg_buf[6], msg_buf[7],
		msg_buf[8], msg_buf[9], msg_buf[10]);

	adsp_unicast(msg_buf, sizeof(msg_buf),
		MSG_LIGHT, 0, MSG_TYPE_SET_HIDDEN_HOLE_DATA);
exit:
	kfree(read_buf);
	kfree(predefine_value_path);
}

static int tmd490x_hh_check_crc(void)
{
	struct file *hole_filp = NULL;
	int msg_buf[EFS_SAVE_NUMS];
	mm_segment_t old_fs;
	int i, j, sum = 0, ret = 0;
	char efs_version[VERSION_FILE_NAME_LEN] = {0, };
	char *predefine_value_path = kzalloc(PATH_LEN + 1, GFP_KERNEL);
	char *read_buf = kzalloc(FILE_BUF_LEN, GFP_KERNEL);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	hole_filp = filp_open(HH_VERSION_FILE_PATH, O_RDONLY, 0440);
	if (IS_ERR(hole_filp)) {
		pr_info("[FACTORY] %s: open fail\n", __func__);
		ret = PTR_ERR(hole_filp);
		goto crc_err_open_ver;
	}

	ret = vfs_read(hole_filp, (char *)&efs_version,
		VERSION_FILE_NAME_LEN * sizeof(char), &hole_filp->f_pos);
	if (ret < 0) {
		pr_err("[FACTORY] %s: fd read fail:%d\n", __func__, ret);
		goto crc_err_read_ver;
	}
	efs_version[VERSION_FILE_NAME_LEN - 1] = '\0';

	pr_info("[FACTORY] %s: efs_version:%s\n", __func__, efs_version);

	filp_close(hole_filp, current->files);
	set_fs(old_fs);

	i = ID_MAX;
	while (efs_version[i] >= '0' && efs_version[i] <= 'f') {
		if (efs_version[i] >= 'a')
			snprintf(predefine_value_path, PATH_LEN,
				"%s%d", PREDEFINE_FILE_PATH,
				efs_version[i] - 'a' + 10);
		else
			snprintf(predefine_value_path, PATH_LEN,
				"%s%d", PREDEFINE_FILE_PATH,
				efs_version[i] - '0');
		pr_info("[FACTORY] %s: path:%s\n",
			__func__, predefine_value_path);

		old_fs = get_fs();
		set_fs(KERNEL_DS);

		hole_filp = filp_open(predefine_value_path, O_RDONLY, 0440);
		if (IS_ERR(hole_filp)) {
			set_fs(old_fs);
			ret = PTR_ERR(hole_filp);
			pr_err("%s - Can't open hidden hole file:%d\n",
				__func__, ret);
			goto crc_err_open;
		}

		ret = vfs_read(hole_filp, (char *)read_buf,
			FILE_BUF_LEN * sizeof(char), &hole_filp->f_pos);
		if (ret < 0) {
			pr_err("[FACTORY] %s: fd read fail:%d\n",
				__func__, ret);
			goto crc_err_read;
		}

		filp_close(hole_filp, current->files);
		set_fs(old_fs);

		ret = sscanf(read_buf,
			"%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d",
			&msg_buf[0], &msg_buf[1], &msg_buf[2], &msg_buf[3],
			&msg_buf[4], &msg_buf[5], &msg_buf[6], &msg_buf[7],
			&msg_buf[8], &msg_buf[9], &msg_buf[10]);
		if (ret != EFS_SAVE_NUMS) {
			pr_err("[FACTORY] %s: sscanf fail:%d\n",
				__func__, ret);
			goto crc_err_sum;
		}

		for (j = 0, sum = 0; j < SUM_CRC; j++)
			sum += msg_buf[j];
		if (sum != msg_buf[SUM_CRC]) {
			pr_err("[FACTORY] %s: CRC error %d:%d\n",
				__func__, sum, msg_buf[SUM_CRC]);
			ret = -1;
			goto crc_err_sum;
		}
		i++;
	}

	goto exit;

crc_err_read_ver:
crc_err_read:
	filp_close(hole_filp, current->files);
crc_err_open_ver:
	set_fs(old_fs);
crc_err_open:
crc_err_sum:
exit:
	kfree(read_buf);
	kfree(predefine_value_path);
	return ret;
}

static ssize_t tmd490x_hh_ver_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{

	struct file *type_filp = NULL;
	mm_segment_t old_fs;
	int ret = 0;
	char version[VERSION_FILE_NAME_LEN] = {0, };

	if ((size < 2) || (size > VERSION_FILE_NAME_LEN)) {
		pr_err("[FACTORY] %s: size %d error\n", __func__, (int)size);
		return -EINVAL;
	}

	strlcpy(version, &buf[1], size);
	pr_info("[FACTORY] %s: buf: %s\n", __func__, version);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	type_filp = filp_open(HH_VERSION_FILE_PATH,
			O_TRUNC | O_RDWR | O_CREAT, 0660);
	if (IS_ERR(type_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(type_filp);
		pr_err("[FACTORY] %s: open fail version:%d\n", __func__, ret);
		return size;
	}

	ret = vfs_write(type_filp, version,
		VERSION_FILE_NAME_LEN * sizeof(char), &type_filp->f_pos);
	if (ret < 0) {
		pr_err("[FACTORY] %s: fd write fail:%d\n", __func__, ret);
		ret = -EIO;
	}

	filp_close(type_filp, current->files);
	set_fs(old_fs);

	return size;
}

static ssize_t tmd490x_hh_ver_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct file *ver_filp = NULL;
	mm_segment_t old_fs;
	char efs_version[VERSION_FILE_NAME_LEN] = {0, };
	char table_version[VERSION_FILE_NAME_LEN] = {0, };
	char tmp[5] = {0,};
	int i = 0, ret = 0;

	ret = tmd490x_hh_check_crc();
	if (ret < 0) {
		pr_err("[FACTORY] %s: CRC check error:%d\n", __func__, ret);
		goto err_check_crc;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	ver_filp = filp_open(HH_VERSION_FILE_PATH, O_RDONLY, 0440);
	if (IS_ERR(ver_filp)) {
		pr_err("[FACTORY] %s: open fail\n", __func__);
		goto err_open_fail;
	}

	ret = vfs_read(ver_filp, (char *)&efs_version,
		VERSION_FILE_NAME_LEN * sizeof(char), &ver_filp->f_pos);
	if (ret < 0) {
		pr_err("[FACTORY] %s: fd read fail:%d\n", __func__, ret);
		goto err_fail;
	}
	efs_version[VERSION_FILE_NAME_LEN - 1] = '\0';

	filp_close(ver_filp, current->files);
	set_fs(old_fs);

	sprintf(table_version, "%d.", hidden_table[ID_INDEX_NUMS - 1].version);

	for (i = 0 ; i < ID_INDEX_NUMS ; i++) {
		tmd490x_itoa(tmp, hidden_table[i].octa_id);
		strlcat(table_version, tmp, sizeof(table_version));
		pr_err("[FACTORY] %s: version:%s\n", __func__, table_version);
	}

	pr_info("[FACTORY] %s: ver:%s:%s\n",
		__func__, efs_version, table_version);

	return snprintf(buf, PAGE_SIZE, "P%s,P%s\n",
		efs_version, table_version);
err_fail:
	filp_close(ver_filp, current->files);
err_open_fail:
	set_fs(old_fs);
err_check_crc:
	pr_info("[FACTORY] %s: fail\n", __func__);
	return snprintf(buf, PAGE_SIZE, "0,0\n");
}

static ssize_t tmd490x_hh_write_all_data_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct file *type_filp = NULL;
	int msg_buf[EFS_SAVE_NUMS];
	mm_segment_t old_fs;
	int ret = 0, octa_id = 0;
	char *predefine_value_path = kzalloc(PATH_LEN + 1, GFP_KERNEL);
	char *write_buf = kzalloc(FILE_BUF_LEN, GFP_KERNEL);

	ret = sscanf(buf, "%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d",
		&octa_id, &msg_buf[0], &msg_buf[1], &msg_buf[2], &msg_buf[3],
		&msg_buf[4], &msg_buf[5], &msg_buf[6], &msg_buf[7], &msg_buf[8],
		&msg_buf[9], &msg_buf[10]);
	if (ret != EFS_SAVE_NUMS + 1) {
		pr_err("[FACTORY] %s: sscanf fail:%d\n", __func__, ret);
		kfree(write_buf);
		kfree(predefine_value_path);
		return size;
	}

	pr_info("[FACTORY] %s: ID:%d, DATA: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		__func__, octa_id, msg_buf[0], msg_buf[1], msg_buf[2],
		msg_buf[3], msg_buf[4], msg_buf[5], msg_buf[6], msg_buf[7],
		msg_buf[8], msg_buf[9], msg_buf[10]);

	snprintf(write_buf, FILE_BUF_LEN, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		msg_buf[0], msg_buf[1], msg_buf[2], msg_buf[3], msg_buf[4],
		msg_buf[5], msg_buf[6], msg_buf[7], msg_buf[8], msg_buf[9],
		msg_buf[10]);

	snprintf(predefine_value_path, PATH_LEN,
		"%s%d", PREDEFINE_FILE_PATH, octa_id);

	pr_info("[FACTORY] %s: path:%s\n", __func__, predefine_value_path);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	type_filp = filp_open(predefine_value_path,
			O_TRUNC | O_RDWR | O_CREAT, 0660);
	if (IS_ERR(type_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(type_filp);
		pr_err("[FACTORY] %s: open fail predefine_value_path:%d\n",
			__func__, ret);
		kfree(write_buf);
		kfree(predefine_value_path);
		return size;
	}

	ret = vfs_write(type_filp, (char *)write_buf,
		FILE_BUF_LEN * sizeof(char), &type_filp->f_pos);
	if (ret < 0) {
		pr_err("[FACTORY] %s: fd write:%d\n", __func__, ret);
		ret = -EIO;
	}

	filp_close(type_filp, current->files);
	set_fs(old_fs);
	kfree(write_buf);
	kfree(predefine_value_path);
	return size;
}

static ssize_t tmd490x_hh_write_all_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;
	int i;

	for (i = 0; i < ID_INDEX_NUMS ; i++) {
		if (!check_crc_table(i)) {
			pr_err("[FACTORY] %s: CRC check fail (%d)\n",
				__func__, i);

			return snprintf(buf, PAGE_SIZE, "%s\n", "FALSE");
		}
	}

	for (i = 0; i < ID_INDEX_NUMS ; i++) {
		ret = make_coef_efs(i);
		if (ret < 0)
			pr_err("[FACTORY] %s: make_coef_efs fail:%d\n",
			__func__, i);
	}

	ret = tmd490x_hh_check_crc();
	pr_info("[FACTORY] %s: success to write all data:%d\n", __func__, ret);

	if (ret < 0)
		return snprintf(buf, PAGE_SIZE, "%s\n", "FALSE");
	else
		return snprintf(buf, PAGE_SIZE, "%s\n", "TRUE");
}

static ssize_t tmd490x_hh_is_exist_efs_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct file *hole_filp = NULL;
	mm_segment_t old_fs;
	int retry_cnt = 0, win_type;
	bool is_exist = false;
	char *predefine_value_path = kzalloc(PATH_LEN + 1, GFP_KERNEL);

	win_type = read_window_type();
	if (win_type >= 0)
		snprintf(predefine_value_path, PATH_LEN,
			"%s%d", PREDEFINE_FILE_PATH, win_type);
	else {
		pr_err("[FACTORY] %s: win_type fail\n", __func__);
		kfree(predefine_value_path);
		return snprintf(buf, PAGE_SIZE, "%s\n", "FALSE");
	}

	pr_info("[FACTORY] %s: win:%d:%s\n",
		__func__, win_type, predefine_value_path);
	old_fs = get_fs();
	set_fs(KERNEL_DS);

retry_open_efs:
	hole_filp = filp_open(predefine_value_path, O_RDONLY, 0440);

	if (IS_ERR(hole_filp)) {
		pr_err("[FACTORY] %s: open fail fail:%d\n",
			__func__, IS_ERR(hole_filp));
		if (retry_cnt < RETRY_MAX) {
			retry_cnt++;
			goto retry_open_efs;
		} else
			is_exist = false;
	} else {
		filp_close(hole_filp, current->files);
		is_exist = true;
	}

	set_fs(old_fs);
	kfree(predefine_value_path);

	pr_info("[FACTORY] %s: is_exist:%d, retry :%d\n",
		__func__, is_exist, retry_cnt);

	if (is_exist)
		return snprintf(buf, PAGE_SIZE, "%s\n", "TRUE");
	else
		return snprintf(buf, PAGE_SIZE, "%s\n", "FALSE");
}

static ssize_t tmd490x_hh_ext_prox_th_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int threshold = 0;
	int win_type;

	if (kstrtoint(buf, 10, &threshold)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return size;
	}

	win_type = read_window_type();
	if (win_type < 0) {
		pr_err("[FACTORY] %s: win_type read fail\n", __func__);
		return size;
	}

	pr_info("[FACTORY] %s: win_type:%d, thd:%d\n",
		__func__, win_type, threshold);
	hidden_table[win_type].data[IRIS_PROX_THD] = threshold;

	return size;
}

static ssize_t tmd490x_hh_ext_prox_th_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int win_type;

	win_type = read_window_type();
	if (win_type < 0) {
		pr_err("[FACTORY] %s: win_type read fail\n", __func__);
		return -EINVAL;
	}

	pr_info("[FACTORY] %s: win_type:%d, thd:%d\n", __func__, win_type,
		hidden_table[win_type].data[IRIS_PROX_THD]);

	return snprintf(buf, PAGE_SIZE, "%d\n",
		hidden_table[win_type].data[IRIS_PROX_THD]);
}

static DEVICE_ATTR(hh_ver, 0664, tmd490x_hh_ver_show, tmd490x_hh_ver_store);
static DEVICE_ATTR(hh_write_all_data, 0664,
	tmd490x_hh_write_all_data_show, tmd490x_hh_write_all_data_store);
static DEVICE_ATTR(hh_is_exist_efs, 0444, tmd490x_hh_is_exist_efs_show, NULL);
static DEVICE_ATTR(hh_ext_prox_th, 0664,
	tmd490x_hh_ext_prox_th_show, tmd490x_hh_ext_prox_th_store);

static struct device_attribute *hh_hole_attrs[] = {
	&dev_attr_hh_ver,
	&dev_attr_hh_write_all_data,
	&dev_attr_hh_is_exist_efs,
	&dev_attr_hh_ext_prox_th,
	NULL,
};

static int __init hh_hole_factory_init(void)
{
	adsp_factory_register(MSG_HH_HOLE, hh_hole_attrs);

	pr_info("[FACTORY] %s\n", __func__);

	return 0;
}

static void __exit hh_hole_factory_exit(void)
{
	adsp_factory_unregister(MSG_HH_HOLE);

	pr_info("[FACTORY] %s\n", __func__);
}
module_init(hh_hole_factory_init);
module_exit(hh_hole_factory_exit);
