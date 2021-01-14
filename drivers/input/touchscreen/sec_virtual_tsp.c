
#include <linux/input/sec_cmd.h>

#if defined(CONFIG_FOLDER_HALL)
#include <linux/hall.h>

static struct notifier_block hall_ic_nb;
#endif
static int flip_status;
static struct mutex switching_mutex;
static int fac_flip_status;

static struct sec_cmd_data *dual_sec;
static struct sec_cmd_data *main_sec;
static struct sec_cmd_data *sub_sec;

#define DEV_COUNT		2

#define FLIP_STATUS_DEFAULT	-1
#define FLIP_STATUS_MAIN	0
#define FLIP_STATUS_SUB		1

#define PATH_MAIN_SEC_CMD		"/sys/class/sec/tsp1/cmd"
#define PATH_MAIN_SEC_CMD_STATUS	"/sys/class/sec/tsp1/cmd_status"
#define PATH_MAIN_SEC_CMD_RESULT	"/sys/class/sec/tsp1/cmd_result"
#define PATH_MAIN_SEC_CMD_STATUS_ALL	"/sys/class/sec/tsp1/cmd_status_all"
#define PATH_MAIN_SEC_CMD_RESULT_ALL	"/sys/class/sec/tsp1/cmd_result_all"

#define PATH_SUB_SEC_CMD		"/sys/class/sec/tsp2/cmd"
#define PATH_SUB_SEC_CMD_STATUS		"/sys/class/sec/tsp2/cmd_status"
#define PATH_SUB_SEC_CMD_RESULT		"/sys/class/sec/tsp2/cmd_result"
#define PATH_SUB_SEC_CMD_STATUS_ALL	"/sys/class/sec/tsp2/cmd_status_all"
#define PATH_SUB_SEC_CMD_RESULT_ALL	"/sys/class/sec/tsp2/cmd_result_all"

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
#include <linux/sec_ts_common.h>
extern struct tsp_dump_callbacks dump_callbacks;
struct tsp_dump_callbacks *tsp_callbacks;
EXPORT_SYMBOL(tsp_callbacks);
static struct tsp_dump_callbacks callbacks[DEV_COUNT];
#endif

void sec_virtual_tsp_register(struct sec_cmd_data *sec)
{
	if (strcmp(dev_name(sec->fac_dev), SEC_CLASS_DEV_NAME_TSP1) == 0) {
		main_sec = sec;
		input_info(true, sec->fac_dev, "%s: main\n", __func__);
	} else if (strcmp(dev_name(sec->fac_dev), SEC_CLASS_DEV_NAME_TSP2) == 0) {
		sub_sec = sec;
		input_info(true, sec->fac_dev, "%s: sub\n", __func__);
	}
}
EXPORT_SYMBOL(sec_virtual_tsp_register);

#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
static void sec_virtual_tsp_dump(void)
{
	input_info(true, dual_sec->fac_dev, "%s: flip_status %s[%d]\n", __func__,
		flip_status ? "close:sub_tsp" : "open:main_tsp", flip_status);

	if (flip_status == FLIP_STATUS_MAIN) {
		if (callbacks[FLIP_STATUS_MAIN].inform_dump)
			callbacks[FLIP_STATUS_MAIN].inform_dump();
	} else if (flip_status == FLIP_STATUS_SUB) {
		if (callbacks[FLIP_STATUS_SUB].inform_dump)
			callbacks[FLIP_STATUS_SUB].inform_dump();
	}
}
#endif

static int sec_virtual_tsp_read_sysfs(struct sec_cmd_data *sec, const char *path, char *buf, int len)
{
	int ret = 0;
	mm_segment_t old_fs;
	struct file *sysfs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	sysfs = filp_open(path, O_RDONLY, 0444);
	if (IS_ERR(sysfs)) {
		ret = PTR_ERR(sysfs);
		input_err(true, sec->fac_dev, "%s: %s open fail, %d\n", __func__, path, ret);
		set_fs(old_fs);
		return ret;
	}

	ret = sysfs->f_op->read(sysfs, buf, len, &sysfs->f_pos);
	if (ret < 0) {
		input_err(true, sec->fac_dev, "%s: failed to read, len:%d, ret:%d\n", __func__, len, ret);
		ret = -EIO;
	}

	filp_close(sysfs, current->files);
	set_fs(old_fs);

	return ret;
}

static int sec_virtual_tsp_write_sysfs(struct sec_cmd_data *sec, const char *path, const char *cmd)
{
	int ret = 0;
	mm_segment_t old_fs;
	struct file *sysfs;
	int len;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	sysfs = filp_open(path, O_WRONLY, 0220);
	if (IS_ERR(sysfs)) {
		ret = PTR_ERR(sysfs);
		input_err(true, sec->fac_dev, "%s: %s open fail, %d\n", __func__, path, ret);
		set_fs(old_fs);
		return ret;
	}

	len = strlen(cmd);
	ret = sysfs->f_op->write(sysfs, cmd, len, &sysfs->f_pos);
	if (ret != len) {
		input_err(true, sec->fac_dev, "%s: failed to write, len:%d, ret:%d\n", __func__, len, ret);
		ret = -EIO;
	}

	filp_close(sysfs, current->files);
	set_fs(old_fs);

	return ret;
}

static int sec_virtual_tsp_get_cmd_status(struct sec_cmd_data *sec, char *path)
{
	u8 buff[16];
	int ret;

	memset(buff, 0x00, sizeof(buff));

	ret = sec_virtual_tsp_read_sysfs(sec, path, buff, 16);
	if (ret < 0)
		return SEC_CMD_STATUS_FAIL;

	if (strncmp(buff, "WAITING", 7) == 0)
		return SEC_CMD_STATUS_WAITING;
	else if (strncmp(buff, "OK", 2) == 0)
		return SEC_CMD_STATUS_OK;
	else if (strncmp(buff, "FAIL", 4) == 0)
		return SEC_CMD_STATUS_FAIL;
	else if (strncmp(buff, "RUNNING", 7) == 0)
		return SEC_CMD_STATUS_RUNNING;
	else if (strncmp(buff, "EXPAND", 6) == 0)
		return SEC_CMD_STATUS_EXPAND;
	else
		return SEC_CMD_STATUS_NOT_APPLICABLE;
}

static int sec_virtual_tsp_write_cmd(struct sec_cmd_data *sec, bool main, bool sub)
{
	u8 buff[16];
	int ret_sub = 0;
	int ret_main = 0;
	bool exit = false;

	sec_cmd_set_default_result(sec);

	if (!main && !sub) {
		snprintf(buff, sizeof(buff), "%s", "NA");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto err;
	}

	if (sub && sub_sec) {
		input_dbg(true, sec->fac_dev, "%s: send to sub\n", sec->cmd);
		ret_sub = sec_virtual_tsp_write_sysfs(sec, PATH_SUB_SEC_CMD, sec->cmd);
		if (ret_sub < 0) {
			snprintf(buff, sizeof(buff), "%s", "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
			goto main;
		}
		sec->cmd_state = sec_virtual_tsp_get_cmd_status(sec, PATH_SUB_SEC_CMD_STATUS);
		input_dbg(true, sec->fac_dev, "%s: sub_sec OK\n", sec->cmd);
		if (!sub_sec->cmd_is_running)
			exit = true;
		sec_virtual_tsp_read_sysfs(sec, PATH_SUB_SEC_CMD_RESULT, sec->cmd_result, SEC_CMD_RESULT_STR_LEN);
		memset(sec->cmd_result, 0x00, SEC_CMD_RESULT_STR_LEN_EXPAND);
		sec_cmd_set_cmd_result(sec, sub_sec->cmd_result, strlen(sub_sec->cmd_result));
	}
main:
	if (main && main_sec) {
		input_dbg(true, sec->fac_dev, "%s: send to main\n", sec->cmd);
		ret_main = sec_virtual_tsp_write_sysfs(sec, PATH_MAIN_SEC_CMD, sec->cmd);
		if (ret_main < 0) {
			snprintf(buff, sizeof(buff), "%s", "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
			goto err;
		}
		sec->cmd_state = sec_virtual_tsp_get_cmd_status(sec, PATH_MAIN_SEC_CMD_STATUS);
		input_dbg(true, sec->fac_dev, "%s: main_sec OK\n", sec->cmd);
		if (!main_sec->cmd_is_running)
			exit = true;
		sec_virtual_tsp_read_sysfs(sec, PATH_MAIN_SEC_CMD_RESULT, sec->cmd_result, SEC_CMD_RESULT_STR_LEN);
		memset(sec->cmd_result, 0x00, SEC_CMD_RESULT_STR_LEN_EXPAND);
		sec_cmd_set_cmd_result(sec, main_sec->cmd_result, strlen(main_sec->cmd_result));
	}
	if (exit) {
		input_dbg(true, sec->fac_dev, "%s: set_cmd_exit\n", sec->cmd);
		sec_cmd_set_cmd_exit(sec);
	}

	return (ret_sub < 0 || ret_main < 0) ? -1 : 0;

err:
	sec_cmd_set_cmd_result(sec, buff, SEC_CMD_RESULT_STR_LEN);
	sec_cmd_set_cmd_exit(sec);

	return -1;
}

static void sec_virtual_tsp_write_cmd_factory_all(struct sec_cmd_data *sec, bool main, bool sub)
{
	u8 buff[16];
	int ret = 0;

	sec_cmd_set_default_result(sec);

	if (!main && !sub) {
		snprintf(buff, sizeof(buff), "%s", "NA");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto err;
	}

	if (sub && sub_sec) {
		input_dbg(true, sec->fac_dev, "%s: sub\n", sec->cmd);
		ret = sec_virtual_tsp_write_sysfs(sec, PATH_SUB_SEC_CMD, sec->cmd);
		if (ret < 0) {
			snprintf(buff, sizeof(buff), "%s", "NG");
			sec->cmd_all_factory_state = SEC_CMD_STATUS_FAIL;
			goto main;
		}
		sec->cmd_all_factory_state = sec_virtual_tsp_get_cmd_status(sec, PATH_SUB_SEC_CMD_STATUS_ALL);
		sec_virtual_tsp_read_sysfs(sec, PATH_SUB_SEC_CMD_RESULT_ALL, sec->cmd_result_all, SEC_CMD_RESULT_STR_LEN);
	}
main:
	if (main && main_sec) {
		input_dbg(true, sec->fac_dev, "%s: main\n", sec->cmd);
		ret = sec_virtual_tsp_write_sysfs(sec, PATH_MAIN_SEC_CMD, sec->cmd);
		if (ret < 0) {
			snprintf(buff, sizeof(buff), "%s", "NG");
			sec->cmd_all_factory_state = SEC_CMD_STATUS_FAIL;
			goto err;
		}
		sec->cmd_all_factory_state = sec_virtual_tsp_get_cmd_status(sec, PATH_MAIN_SEC_CMD_STATUS_ALL);
		sec_virtual_tsp_read_sysfs(sec, PATH_MAIN_SEC_CMD_RESULT_ALL, sec->cmd_result_all, SEC_CMD_RESULT_STR_LEN);
	}

	return;

err:
	sec_cmd_set_cmd_result_all(sec, buff, SEC_CMD_RESULT_STR_LEN, "NONE");
}

static void sec_virtual_tsp_dual_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;

	sec_virtual_tsp_write_cmd(sec, true, true);
}

static void sec_virtual_tsp_main_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	bool main = true;
	bool sub = false;

	mutex_lock(&switching_mutex);
	if (!((fac_flip_status == FLIP_STATUS_DEFAULT && flip_status == FLIP_STATUS_MAIN)
			|| fac_flip_status == FLIP_STATUS_MAIN)) {
		input_err(true, sec->fac_dev, "%s: flip is sub, skip[%d,%d]\n",
			__func__, fac_flip_status, flip_status);
		main = false;
	}
	mutex_unlock(&switching_mutex);

	sec_virtual_tsp_write_cmd(sec, main, sub);
}

static void sec_virtual_tsp_sub_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	bool main = false;
	bool sub = true;

	mutex_lock(&switching_mutex);
	if (!((fac_flip_status == FLIP_STATUS_DEFAULT && flip_status == FLIP_STATUS_SUB)
			|| fac_flip_status == FLIP_STATUS_SUB)) {
		input_err(true, sec->fac_dev, "%s: flip is main, skip[%d,%d]\n",
			__func__, fac_flip_status, flip_status);
		sub = false;
	}
	mutex_unlock(&switching_mutex);

	sec_virtual_tsp_write_cmd(sec, main, sub);
}

static void sec_virtual_tsp_switch_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	bool main = false;
	bool sub = false;

	mutex_lock(&switching_mutex);
	if (fac_flip_status == FLIP_STATUS_DEFAULT) {
		/* Using hall_ic */
		if (flip_status == FLIP_STATUS_MAIN) {
			main = true;
		} else if (flip_status == FLIP_STATUS_SUB) {
			sub = true;
		}
	} else if (fac_flip_status == FLIP_STATUS_MAIN) {
		main = true;
	} else if (fac_flip_status == FLIP_STATUS_SUB) {
		sub = true;
	}
	input_dbg(true, sec->fac_dev, "%s: %d,%d\n", __func__, fac_flip_status, flip_status);
	mutex_unlock(&switching_mutex);

	sec_virtual_tsp_write_cmd(sec, main, sub);
}

static void sec_virtual_tsp_factory_cmd_result_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	bool main = false;
	bool sub = false;

	mutex_lock(&switching_mutex);
	if (fac_flip_status == FLIP_STATUS_DEFAULT) {
		/* Using hall_ic */
		if (flip_status == FLIP_STATUS_MAIN) {
			main = true;
		} else if (flip_status == FLIP_STATUS_SUB) {
			sub = true;
		}
	} else if (fac_flip_status == FLIP_STATUS_MAIN) {
		main = true;
	} else if (fac_flip_status == FLIP_STATUS_SUB) {
		sub = true;
	}
	input_dbg(true, sec->fac_dev, "%s: %d,%d\n", __func__, fac_flip_status, flip_status);
	mutex_unlock(&switching_mutex);

	sec_virtual_tsp_write_cmd_factory_all(sec, main, sub);
}

static void set_rear_selfie_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	char buff[16] = { 0 };
	int ret;
	static bool selfie_mode;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	mutex_lock(&switching_mutex);
	if (sec->cmd_param[0] == 1) {
		fac_flip_status = FLIP_STATUS_SUB;
	} else {
		fac_flip_status = FLIP_STATUS_DEFAULT;
	}
	input_err(true, sec->fac_dev, "%s: %d\n", __func__, fac_flip_status);
	mutex_unlock(&switching_mutex);

	if (selfie_mode == false && sec->cmd_param[0] == 0) {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_WAITING;
		goto out;
	} else {
		ret = sec_virtual_tsp_write_cmd(sec, true, true);
		if (ret >= 0) {
			if (sec->cmd_param[0] == 1)
				selfie_mode = true;
			else
				selfie_mode = false;
		}
	}

	return;
out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, sec->fac_dev, "%s: %s\n", __func__, buff);
}

static void set_factory_panel(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < FLIP_STATUS_DEFAULT || sec->cmd_param[0] > FLIP_STATUS_SUB) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto err;
	}

	mutex_lock(&switching_mutex);
	fac_flip_status = sec->cmd_param[0];
	input_err(true, sec->fac_dev, "%s: %d\n", __func__, fac_flip_status);
	mutex_unlock(&switching_mutex);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;

err:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}

static void dev_count(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "%s,%d", "OK", DEV_COUNT);
	sec->cmd_state = SEC_CMD_STATUS_OK;

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}

#if defined(CONFIG_FOLDER_HALL)
static int sec_virtual_tsp_hall_ic_notify(struct notifier_block *nb,
			unsigned long flip_cover, void *v)
{
	input_info(true, dual_sec->fac_dev, "%s: %s\n", __func__,
			 flip_cover ? "close" : "open");

	mutex_lock(&switching_mutex);
	flip_status = flip_cover;
	mutex_unlock(&switching_mutex);

	return 0;
}
#endif

static struct sec_cmd tsp_commands[] = {
	{SEC_CMD_H("glove_mode", sec_virtual_tsp_dual_cmd),},
	{SEC_CMD_H("set_wirelesscharger_mode", sec_virtual_tsp_dual_cmd),},
	{SEC_CMD_H("spay_enable", sec_virtual_tsp_dual_cmd),},
	{SEC_CMD_H("aot_enable", sec_virtual_tsp_dual_cmd),},
	{SEC_CMD_H("aod_enable", sec_virtual_tsp_dual_cmd),},
	{SEC_CMD_H("external_noise_mode", sec_virtual_tsp_dual_cmd),},
	{SEC_CMD_H("brush_enable", sec_virtual_tsp_dual_cmd),},
	{SEC_CMD_H("singletap_enable", sec_virtual_tsp_dual_cmd),},
	{SEC_CMD_H("set_touchable_area", sec_virtual_tsp_dual_cmd),},
	{SEC_CMD("set_rear_selfie_mode", set_rear_selfie_mode),},
	{SEC_CMD("clear_cover_mode", sec_virtual_tsp_dual_cmd),},
#ifdef CONFIG_TOUCHSCREEN_FTS9CU80F_B
	{SEC_CMD_H("ear_detect_enable", sec_virtual_tsp_dual_cmd),},
	{SEC_CMD_H("touch_aging_mode", sec_virtual_tsp_dual_cmd),},
#endif

	/* run_xxx_read_all common */
	{SEC_CMD("run_cs_raw_read_all", sec_virtual_tsp_switch_cmd),},
	{SEC_CMD("run_cs_delta_read_all", sec_virtual_tsp_switch_cmd),},
	{SEC_CMD("run_rawcap_read_all", sec_virtual_tsp_switch_cmd),},

	/* run_xxx_read_all main(stm) */
	{SEC_CMD("run_ix_data_read_all", sec_virtual_tsp_main_cmd),},
	{SEC_CMD("run_self_raw_read_all", sec_virtual_tsp_main_cmd),},
	{SEC_CMD("run_cx_data_read_all", sec_virtual_tsp_main_cmd),},
	{SEC_CMD("run_cx_gap_data_rx_all", sec_virtual_tsp_main_cmd),},
	{SEC_CMD("run_cx_gap_data_tx_all", sec_virtual_tsp_main_cmd),},

	/* only main */
	/* only sub */

#ifdef CONFIG_TOUCHSCREEN_SEC_TS_Y771_SUB
	/* run_xxx_read_all sub(s.lsi) */
	{SEC_CMD("run_gap_data_x_all", sec_virtual_tsp_sub_cmd),},
	{SEC_CMD("run_gap_data_y_all", sec_virtual_tsp_sub_cmd),},
	{SEC_CMD("run_reference_read_all", sec_virtual_tsp_sub_cmd),},
	{SEC_CMD("run_delta_read_all", sec_virtual_tsp_sub_cmd),},
	{SEC_CMD("run_raw_p2p_min_read_all", sec_virtual_tsp_sub_cmd),},
	{SEC_CMD("run_raw_p2p_max_read_all", sec_virtual_tsp_sub_cmd),},
	{SEC_CMD("run_self_reference_read_all", sec_virtual_tsp_sub_cmd),},
	{SEC_CMD("run_self_rawcap_read_all", sec_virtual_tsp_sub_cmd),},
	{SEC_CMD("run_self_delta_read_all", sec_virtual_tsp_sub_cmd),},
	{SEC_CMD("run_self_raw_p2p_min_read_all", sec_virtual_tsp_sub_cmd),},
	{SEC_CMD("run_self_raw_p2p_max_read_all", sec_virtual_tsp_sub_cmd),},
#endif

#ifdef CONFIG_TOUCHSCREEN_ZINITIX_ZTW522
	/* run_xxx_read_all sub(zinitix) */
	{SEC_CMD("run_dnd_read_all", sec_virtual_tsp_sub_cmd),},
	{SEC_CMD("run_dnd_v_gap_read_all", sec_virtual_tsp_sub_cmd),},
	{SEC_CMD("run_dnd_h_gap_read_all", sec_virtual_tsp_sub_cmd),},
	{SEC_CMD("run_selfdnd_read_all", sec_virtual_tsp_sub_cmd),},
	{SEC_CMD("run_selfdnd_h_gap_read_all", sec_virtual_tsp_sub_cmd),},
	{SEC_CMD("run_jitter_read_all", sec_virtual_tsp_sub_cmd),},
	{SEC_CMD("run_self_saturation_read_all", sec_virtual_tsp_sub_cmd),},
#endif

	{SEC_CMD("factory_cmd_result_all", sec_virtual_tsp_factory_cmd_result_all),},

	{SEC_CMD_H("set_factory_panel", set_factory_panel),},
	{SEC_CMD("dev_count", dev_count),},

	{SEC_CMD("not_support_cmd", sec_virtual_tsp_switch_cmd),},
};

static ssize_t sec_virtual_tsp_support_feature_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char buffer[10];
	int ret;

	memset(buffer, 0x00, sizeof(buffer));

	ret = sec_virtual_tsp_read_sysfs(dual_sec, "/sys/class/sec/tsp1/support_feature", buffer, sizeof(buffer));
	if (ret < 0)
		return snprintf(buf, SEC_CMD_BUF_SIZE, "NG\n");

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buffer);
}

static ssize_t sec_virtual_tsp_prox_power_off_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char buffer[10];
	int ret;

	memset(buffer, 0x00, sizeof(buffer));

	if (flip_status)
		ret = sec_virtual_tsp_read_sysfs(dual_sec, "/sys/class/sec/tsp2/prox_power_off", buffer, sizeof(buffer));
	else
		ret = sec_virtual_tsp_read_sysfs(dual_sec, "/sys/class/sec/tsp1/prox_power_off", buffer, sizeof(buffer));

	input_info(false, dual_sec->fac_dev, "%s: %s, ret:%d\n", __func__,
			 flip_status ? "close" : "open", ret);

	if (ret < 0)
		return snprintf(buf, SEC_CMD_BUF_SIZE, "NG\n");

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s\n", buffer);
}

static ssize_t sec_virtual_tsp_prox_power_off_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;

	if (flip_status)
		ret = sec_virtual_tsp_write_sysfs(dual_sec, "/sys/class/sec/tsp2/prox_power_off", buf);
	else
		ret = sec_virtual_tsp_write_sysfs(dual_sec, "/sys/class/sec/tsp1/prox_power_off", buf);

	input_info(false, dual_sec->fac_dev, "%s: %s, ret:%d\n", __func__,
			 flip_status ? "close" : "open", ret);

	return count;
}

#if defined(CONFIG_TOUCHSCREEN_FTS9CU80F_B) && defined(CONFIG_TOUCHSCREEN_SEC_TS_Y771_SUB)
static ssize_t dualscreen_policy_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret, value;

	ret = kstrtoint(buf, 10, &value);
	if (ret != 0)
		return ret;

	if (value <= FLIP_STATUS_DEFAULT || value > FLIP_STATUS_SUB) {
		input_info(false, dual_sec->fac_dev, "%s: value=%d %s\n", __func__, value,
				 flip_status ? "close" : "open");
		return count;
	}

	mutex_lock(&switching_mutex);
	flip_status = value;
	mutex_unlock(&switching_mutex);

	if (value == FLIP_STATUS_MAIN)
		ret = sec_virtual_tsp_write_sysfs(dual_sec, "/sys/class/sec/tsp1/dualscreen_policy", buf);

	input_info(false, dual_sec->fac_dev, "%s: value=%d %s, ret:%d\n", __func__, value,
			 flip_status ? "close" : "open", ret);

	return count;
}

static DEVICE_ATTR(dualscreen_policy, 0644, NULL, dualscreen_policy_store);
#endif

static DEVICE_ATTR(support_feature, 0444, sec_virtual_tsp_support_feature_show, NULL);
static DEVICE_ATTR(prox_power_off, 0644, sec_virtual_tsp_prox_power_off_show, sec_virtual_tsp_prox_power_off_store);

static struct attribute *sec_virtual_tsp_attrs[] = {
	&dev_attr_support_feature.attr,
	&dev_attr_prox_power_off.attr,
#if defined(CONFIG_TOUCHSCREEN_FTS9CU80F_B) && defined(CONFIG_TOUCHSCREEN_SEC_TS_Y771_SUB)
	&dev_attr_dualscreen_policy.attr,
#endif
	NULL,
};

static struct attribute_group sec_virtual_tsp_attrs_group = {
	.attrs = sec_virtual_tsp_attrs,
};

static int __init __init_sec_virtual_tsp(void)
{
	int ret;

	dual_sec = kzalloc(sizeof(struct sec_cmd_data), GFP_KERNEL);
	if (!dual_sec) {
		input_err(true, NULL, "%s: failed to alloc sec_cmd_data for dual tsp\n", __func__);
		return -ENOMEM;
	}

	sec_cmd_init(dual_sec, tsp_commands,
		ARRAY_SIZE(tsp_commands), SEC_CLASS_DEVT_TSP);

	input_info(true, dual_sec->fac_dev, "%s\n", __func__);

	fac_flip_status = FLIP_STATUS_DEFAULT;
	flip_status = FLIP_STATUS_MAIN;
	mutex_init(&switching_mutex);

#if defined(CONFIG_FOLDER_HALL)
	hall_ic_nb.priority = 1;
	hall_ic_nb.notifier_call = sec_virtual_tsp_hall_ic_notify;
	hall_ic_register_notify(&hall_ic_nb);
	input_info(true, dual_sec->fac_dev, "%s: hall ic register\n", __func__);
#endif
#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
	tsp_callbacks = callbacks;
	dump_callbacks.inform_dump = sec_virtual_tsp_dump;
#endif

	ret = sysfs_create_group(&dual_sec->fac_dev->kobj, &sec_virtual_tsp_attrs_group);
	if (ret < 0) {
		pr_err("%s %s: failed to create sysfs group\n", SECLOG, __func__);
		return -ENODEV;
	}

	return 0;
}

module_init(__init_sec_virtual_tsp);

