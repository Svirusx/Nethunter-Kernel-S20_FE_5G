#ifdef SEC_TSP_FACTORY_TEST

#define BUFFER_MAX			((256 * 1024) - 16)

enum {
	TYPE_RAW_DATA = 0x30,			// only for winner, etc 0x31
	TYPE_FILTERED_RAW_DATA = 0x31,	// only for winner
	TYPE_BASELINE_DATA = 0x32,
	TYPE_STRENGTH_DATA = 0x33,
};

enum {
	BUILT_IN = 0,
	UMS,
};

enum ito_error_type {
	ITO_FORCE_SHRT_GND		= 0x60,
	ITO_SENSE_SHRT_GND		= 0x61,
	ITO_FORCE_SHRT_VDD		= 0x62,
	ITO_SENSE_SHRT_VDD		= 0x63,
	ITO_FORCE_SHRT_FORCE	= 0x64,
	ITO_SENSE_SHRT_SENSE	= 0x65,
	ITO_FORCE_OPEN			= 0x66,
	ITO_SENSE_OPEN			= 0x67,
	ITO_KEY_OPEN			= 0x68
};

#define FTS_COMP_DATA_HEADER_SIZE     16

enum fts_nvm_data_type {		/* Write Command */
	FTS_NVM_OFFSET_FAC_RESULT = 1,
	FTS_NVM_OFFSET_CAL_COUNT,
	FTS_NVM_OFFSET_DISASSEMBLE_COUNT,
	FTS_NVM_OFFSET_TUNE_VERSION,
	FTS_NVM_OFFSET_CAL_POSITION,
	FTS_NVM_OFFSET_HISTORY_QUEUE_COUNT,
	FTS_NVM_OFFSET_HISTORY_QUEUE_LASTP,
	FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO,
	FTS_NVM_OFFSET_CAL_FAIL_FLAG,
	FTS_NVM_OFFSET_CAL_FAIL_COUNT,
};

struct fts_nvm_data_map {
	int type;
	int offset;
	int length;
};

#define NVM_CMD(mtype, moffset, mlength)		.type = mtype,	.offset = moffset,	.length = mlength

/* This Flash Meory Map is FIXED by STM firmware
 * Do not change MAP.
 */
struct fts_nvm_data_map nvm_data[] = {
	{NVM_CMD(0,						0x00, 0),},
	{NVM_CMD(FTS_NVM_OFFSET_FAC_RESULT,			0x00, 1),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_CAL_COUNT,			0x01, 1),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_DISASSEMBLE_COUNT,		0x02, 1),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_TUNE_VERSION,			0x03, 2),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_CAL_POSITION,			0x05, 1),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_HISTORY_QUEUE_COUNT,		0x06, 1),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_HISTORY_QUEUE_LASTP,		0x07, 1),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO,		0x08, 20),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_CAL_FAIL_FLAG,			0x1C, 1),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_CAL_FAIL_COUNT,			0x1D, 1),},	/* SEC */
};
#define FTS_NVM_OFFSET_ALL	31

static void fw_update(void *device_data);
static void get_fw_ver_bin(void *device_data);
static void get_fw_ver_ic(void *device_data);
static void get_config_ver(void *device_data);
static void get_threshold(void *device_data);
static void module_off_master(void *device_data);
static void module_on_master(void *device_data);
static void get_chip_vendor(void *device_data);
static void get_chip_name(void *device_data);
static void run_jitter_test(void *device_data);
static void run_mutual_jitter(void *device_data);
static void run_self_jitter(void *device_data);
static void run_jitter_delta_test(void *device_data);
static void get_wet_mode(void *device_data);
static void get_x_num(void *device_data);
static void get_y_num(void *device_data);
static void get_checksum_data(void *device_data);
static void check_fw_corruption(void *device_data);
static void run_reference_read(void *device_data);
static void get_reference(void *device_data);
static void run_rawcap_read(void *device_data);
static void run_rawcap_read_all(void *device_data);
static void get_rawcap(void *device_data);
static void run_lp_single_ended_rawcap_read(void *device_data);
static void run_lp_single_ended_rawcap_read_all(void *device_data);
static void run_low_frequency_rawcap_read(void *device_data);
static void run_low_frequency_rawcap_read_all(void *device_data);
static void run_high_frequency_rawcap_read(void *device_data);
static void run_high_frequency_rawcap_read_all(void *device_data);
static void run_delta_read(void *device_data);
static void get_delta(void *device_data);
static void run_rawdata_read_all(void *device_data);
#ifdef TCLM_CONCEPT
static void get_pat_information(void *device_data);
static void set_external_factory(void *device_data);
static void tclm_test_cmd(void *device_data);
static void get_calibration(void *device_data);
#endif
static void run_ix_data_read(void *device_data);
static void run_ix_data_read_all(void *device_data);
static void run_self_raw_read(void *device_data);
static void run_factory_miscalibration(void *device_data);
static void run_miscalibration(void *device_data);
static void run_self_raw_read_all(void *device_data);
static void run_trx_short_test(void *device_data);
static void check_connection(void *device_data);
static void get_cx_data(void *device_data);
static void run_active_cx_data_read(void *device_data);
static void run_multi_mutual_cx_data_read(void *device_data);
static void run_cx_data_read(void *device_data);
static void get_cx_all_data(void *device_data);
static void run_cx_gap_data_x_all(void *device_data);
static void run_cx_gap_data_y_all(void *device_data);
static void run_rawdata_gap_data_rx_all(void *device_data);
static void run_rawdata_gap_data_tx_all(void *device_data);
static void run_prox_intensity_read_all(void *device_data);
static void get_strength_all_data(void *device_data);

static void set_tsp_test_result(void *device_data);
static void get_tsp_test_result(void *device_data);
static void increase_disassemble_count(void *device_data);
static void get_disassemble_count(void *device_data);
static void get_osc_trim_error(void *device_data);
static void get_osc_trim_info(void *device_data);
static void run_snr_non_touched(void *device_data);
static void run_snr_touched(void *device_data);
/*
static void run_elvss_test(void *device_data);
*/
#ifdef CONFIG_GLOVE_TOUCH
static void glove_mode(void *device_data);
#endif
static void clear_cover_mode(void *device_data);
static void report_rate(void *device_data);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
static void interrupt_control(void *device_data);
#endif

static void set_wirelesscharger_mode(void *device_data);
static void set_grip_data(void *device_data);
static void dead_zone_enable(void *device_data);
static void drawing_test_enable(void *device_data);
static void spay_enable(void *device_data);
static void aot_enable(void *device_data);
static void aod_enable(void *device_data);
static void singletap_enable(void *device_data);
static void set_aod_rect(void *device_data);
static void get_aod_rect(void *device_data);
static void fod_enable(void *device_data);
static void set_fod_rect(void *device_data);
static void external_noise_mode(void *device_data);
static void brush_enable(void *device_data);
static void set_touchable_area(void *device_data);
static void debug(void *device_data);
static void factory_cmd_result_all(void *device_data);
static void factory_cmd_result_all_imagetest(void *device_data);
static void run_force_calibration(void *device_data);
static void set_factory_level(void *device_data);
static void fix_active_mode(void *device_data);
static void touch_aging_mode(void *device_data);
static void run_sram_test(void *device_data);
static void set_rear_selfie_mode(void *device_data);
static void ear_detect_enable(void *device_data);
static void pocket_mode_enable(void *device_data);
static void set_sip_mode(void *device_data);
static void set_note_mode(void *device_data);
static void set_game_mode(void *device_data);
static void not_support_cmd(void *device_data);

static ssize_t fts_scrub_position(struct device *dev,
		struct device_attribute *attr, char *buf);

struct sec_cmd ft_commands[] = {
	{SEC_CMD("fw_update", fw_update),},
	{SEC_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{SEC_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{SEC_CMD("get_config_ver", get_config_ver),},
	{SEC_CMD("get_threshold", get_threshold),},
	{SEC_CMD("module_off_master", module_off_master),},
	{SEC_CMD("module_on_master", module_on_master),},
	{SEC_CMD("module_off_slave", not_support_cmd),},
	{SEC_CMD("module_on_slave", not_support_cmd),},
	{SEC_CMD("get_chip_vendor", get_chip_vendor),},
	{SEC_CMD("get_chip_name", get_chip_name),},
	{SEC_CMD("run_jitter_test", run_jitter_test),},
	{SEC_CMD("run_mutual_jitter", run_mutual_jitter),},
	{SEC_CMD("run_self_jitter", run_self_jitter),},
	{SEC_CMD("run_jitter_delta_test", run_jitter_delta_test),},
	{SEC_CMD("get_wet_mode", get_wet_mode),},
	{SEC_CMD("get_module_vendor", not_support_cmd),},
	{SEC_CMD("get_x_num", get_x_num),},
	{SEC_CMD("get_y_num", get_y_num),},
	{SEC_CMD("get_checksum_data", get_checksum_data),},
	{SEC_CMD("get_crc_check", check_fw_corruption),},
	{SEC_CMD("run_reference_read", run_reference_read),},
	{SEC_CMD("get_reference", get_reference),},
	{SEC_CMD("run_rawcap_read", run_rawcap_read),},
	{SEC_CMD("run_rawcap_read_all", run_rawcap_read_all),},
	{SEC_CMD("get_rawcap", get_rawcap),},
	{SEC_CMD("run_lp_single_ended_rawcap_read", run_lp_single_ended_rawcap_read),},
	{SEC_CMD("run_lp_single_ended_rawcap_read_all", run_lp_single_ended_rawcap_read_all),},
	{SEC_CMD("run_low_frequency_rawcap_read", run_low_frequency_rawcap_read),},
	{SEC_CMD("run_low_frequency_rawcap_read_all", run_low_frequency_rawcap_read_all),},
	{SEC_CMD("run_high_frequency_rawcap_read", run_high_frequency_rawcap_read),},
	{SEC_CMD("run_high_frequency_rawcap_read_all", run_high_frequency_rawcap_read_all),},
	{SEC_CMD("run_delta_read", run_delta_read),},
	{SEC_CMD("get_delta", get_delta),},
	{SEC_CMD("run_cs_raw_read_all", run_rawcap_read_all),},
	{SEC_CMD("run_prox_intensity_read_all", run_prox_intensity_read_all),},
	{SEC_CMD("run_cs_delta_read_all", get_strength_all_data),},
	{SEC_CMD("run_rawdata_read_all_for_ghost", run_rawdata_read_all),},
#ifdef TCLM_CONCEPT
	{SEC_CMD("get_pat_information", get_pat_information),},
	{SEC_CMD("set_external_factory", set_external_factory),},
	{SEC_CMD("tclm_test_cmd", tclm_test_cmd),},
	{SEC_CMD("get_calibration", get_calibration),},
#endif
	{SEC_CMD("run_ix_data_read", run_ix_data_read),},
	{SEC_CMD("run_ix_data_read_all", run_ix_data_read_all),},
	{SEC_CMD("run_self_raw_read", run_self_raw_read),},
	{SEC_CMD("run_self_raw_read_all", run_self_raw_read_all),},
	{SEC_CMD("run_factory_miscalibration", run_factory_miscalibration),},
	{SEC_CMD("run_miscalibration", run_miscalibration),},
	{SEC_CMD("run_trx_short_test", run_trx_short_test),},
	{SEC_CMD("check_connection", check_connection),},
	{SEC_CMD("get_cx_data", get_cx_data),},
	{SEC_CMD("run_active_cx_data_read", run_active_cx_data_read),},
	{SEC_CMD("run_multi_mutual_cx_data_read", run_multi_mutual_cx_data_read),},
	{SEC_CMD("run_cx_data_read", run_cx_data_read),},
	{SEC_CMD("run_cx_data_read_all", get_cx_all_data),},
	{SEC_CMD("get_cx_all_data", get_cx_all_data),},
	{SEC_CMD("run_cx_gap_data_rx_all", run_cx_gap_data_x_all),},
	{SEC_CMD("run_cx_gap_data_tx_all", run_cx_gap_data_y_all),},
	{SEC_CMD("run_rawdata_gap_data_rx_all", run_rawdata_gap_data_rx_all),},
	{SEC_CMD("run_rawdata_gap_data_tx_all", run_rawdata_gap_data_tx_all),},
	{SEC_CMD("get_strength_all_data", get_strength_all_data),},
	{SEC_CMD("set_tsp_test_result", set_tsp_test_result),},
	{SEC_CMD("get_tsp_test_result", get_tsp_test_result),},
	{SEC_CMD("increase_disassemble_count", increase_disassemble_count),},
	{SEC_CMD("get_disassemble_count", get_disassemble_count),},
	{SEC_CMD("get_osc_trim_error", get_osc_trim_error),},
	{SEC_CMD("get_osc_trim_info", get_osc_trim_info),},
	{SEC_CMD("run_snr_non_touched", run_snr_non_touched),},
	{SEC_CMD("run_snr_touched", run_snr_touched),},
/*
	{SEC_CMD("run_elvss_test", run_elvss_test),},
*/
#ifdef CONFIG_GLOVE_TOUCH
	{SEC_CMD_H("glove_mode", glove_mode),},
#endif
	{SEC_CMD_H("clear_cover_mode", clear_cover_mode),},
	{SEC_CMD("report_rate", report_rate),},
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	{SEC_CMD("interrupt_control", interrupt_control),},
#endif
	{SEC_CMD("set_wirelesscharger_mode", set_wirelesscharger_mode),},
	{SEC_CMD("set_grip_data", set_grip_data),},
	{SEC_CMD("dead_zone_enable", dead_zone_enable),},
	{SEC_CMD("drawing_test_enable", drawing_test_enable),},
	{SEC_CMD_H("spay_enable", spay_enable),},
	{SEC_CMD_H("aot_enable", aot_enable),},
	{SEC_CMD_H("aod_enable", aod_enable),},
	{SEC_CMD_H("singletap_enable", singletap_enable),},
	{SEC_CMD("set_aod_rect", set_aod_rect),},
	{SEC_CMD("get_aod_rect", get_aod_rect),},
	{SEC_CMD_H("fod_enable", fod_enable),},
	{SEC_CMD("set_fod_rect", set_fod_rect),},
	{SEC_CMD_H("external_noise_mode", external_noise_mode),},
	{SEC_CMD_H("brush_enable", brush_enable),},
	{SEC_CMD_H("set_touchable_area", set_touchable_area),},
	{SEC_CMD("debug", debug),},
	{SEC_CMD("factory_cmd_result_all", factory_cmd_result_all),},
	{SEC_CMD("factory_cmd_result_all_imagetest", factory_cmd_result_all_imagetest),},
	{SEC_CMD("run_force_calibration", run_force_calibration),},
	{SEC_CMD("set_factory_level", set_factory_level),},
	{SEC_CMD("fix_active_mode", fix_active_mode),},
	{SEC_CMD("touch_aging_mode", touch_aging_mode),},
	{SEC_CMD("run_sram_test", run_sram_test),},
	{SEC_CMD_H("set_rear_selfie_mode", set_rear_selfie_mode),},
	{SEC_CMD_H("ear_detect_enable", ear_detect_enable),},
	{SEC_CMD_H("pocket_mode_enable", pocket_mode_enable),},
	{SEC_CMD("set_sip_mode", set_sip_mode),},
	{SEC_CMD_H("set_note_mode", set_note_mode),},
	{SEC_CMD("set_game_mode", set_game_mode),},
	{SEC_CMD("not_support_cmd", not_support_cmd),},
};

/* read param */
static ssize_t hardware_param_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[516];
	char tbuff[128];
	char temp[128];

	memset(buff, 0x00, sizeof(buff));

	/* ito_check */
	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"TITO\":\"%02X%02X%02X%02X\",",
			info->ito_test[0], info->ito_test[1],
			info->ito_test[2], info->ito_test[3]);
	strlcat(buff, tbuff, sizeof(buff));

	/* muli_count */
	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"TMUL\":\"%d\",", info->multi_count);
	strlcat(buff, tbuff, sizeof(buff));

	/* wet_mode */
	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"TWET\":\"%d\",", info->wet_count);
	strlcat(buff, tbuff, sizeof(buff));

	/* noise_mode */
	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"TNOI\":\"%d\",", info->noise_count);
	strlcat(buff, tbuff, sizeof(buff));

	/* comm_err_count */
	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"TCOM\":\"%d\",", info->comm_err_count);
	strlcat(buff, tbuff, sizeof(buff));

	/* module_id */
	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"TMOD\":\"ST%02X%04X%02X%c%01X\",",
			info->panel_revision, info->fw_main_version_of_ic,
			info->test_result.data[0],
#ifdef TCLM_CONCEPT
			info->tdata->tclm_string[info->tdata->nvdata.cal_position].s_name,
			info->tdata->nvdata.cal_count & 0xF);
#else
			'0', 0);
#endif
	strlcat(buff, tbuff, sizeof(buff));

	/* vendor_id */
	memset(tbuff, 0x00, sizeof(tbuff));
	if (info->board->firmware_name) {
		memset(temp, 0x00, sizeof(temp));
		snprintf(temp, 9, "%s", info->board->firmware_name + 8);

		snprintf(tbuff, sizeof(tbuff), "\"TVEN\":\"STM_%s\",", temp);
	} else {
		snprintf(tbuff, sizeof(tbuff), "\"TVEN\":\"STM\",");
	}
	strlcat(buff, tbuff, sizeof(buff));

	/* checksum */
	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"TCHK\":\"%d\",", info->checksum_result);
	strlcat(buff, tbuff, sizeof(buff));

	/* all_touch_count */
	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"TTCN\":\"%d\",\"TACN\":\"%d\",\"TSCN\":\"%d\",",
			info->all_finger_count, info->all_aod_tap_count,
			info->all_spay_count);
	strlcat(buff, tbuff, sizeof(buff));

	/* xenosensor_detect_count */
	memset(tbuff, 0x00, sizeof(tbuff));
	snprintf(tbuff, sizeof(tbuff), "\"TXEN\":\"%02d/%02d_%02d:%02d:%02d_%d,%d_%d\",",
			(info->xenosensor_detect_count > 0) ? info->xenosensor_time.tm_mon + 1 : info->xenosensor_time.tm_mon,
			info->xenosensor_time.tm_mday, info->xenosensor_time.tm_hour, info->xenosensor_time.tm_min,
			info->xenosensor_time.tm_sec, info->xenosensor_detect_x, info->xenosensor_detect_y,
			info->xenosensor_detect_count);
	strlcat(buff, tbuff, sizeof(buff));

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buff);
}

/* clear param */
static ssize_t hardware_param_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	info->multi_count = 0;
	info->wet_count = 0;
	info->noise_count = 0;
	info->comm_err_count = 0;
	info->checksum_result = 0;
	info->all_finger_count = 0;
	info->all_aod_tap_count = 0;
	info->all_spay_count = 0;
	info->xenosensor_detect_count = 0;
	memset(&info->xenosensor_time, 0, sizeof(info->xenosensor_time));
	info->xenosensor_detect_x = 0;
	info->xenosensor_detect_y = 0;

	return count;
}

static ssize_t read_ambient_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	input_info(true, &info->client->dev,
			"\"TAMB_MAX\":\"%d\",\"TAMB_MAX_TX\":\"%d\",\"TAMB_MAX_RX\":\"%d\""
			"\"TAMB_MIN\":\"%d\",\"TAMB_MIN_TX\":\"%d\",\"TAMB_MIN_RX\":\"%d\"",
			info->rawcap_max, info->rawcap_max_tx, info->rawcap_max_rx,
			info->rawcap_min, info->rawcap_min_tx, info->rawcap_min_rx);

	return snprintf(buf, SEC_CMD_BUF_SIZE,
			"\"TAMB_MAX\":\"%d\",\"TAMB_MAX_TX\":\"%d\",\"TAMB_MAX_RX\":\"%d\","
			"\"TAMB_MIN\":\"%d\",\"TAMB_MIN_TX\":\"%d\",\"TAMB_MIN_RX\":\"%d\"",
			info->rawcap_max, info->rawcap_max_tx, info->rawcap_max_rx,
			info->rawcap_min, info->rawcap_min_tx, info->rawcap_min_rx);
}

static ssize_t sensitivity_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	unsigned char wbuf[3] = { 0 };
	unsigned long value = 0;
	int ret = 0;

	if (count > 2)
		return -EINVAL;

	ret = kstrtoul(buf, 10, &value);
	if (ret != 0)
		return ret;

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		return -EPERM;
	}

	wbuf[0] = FTS_CMD_SENSITIVITY_MODE;

	switch (value) {
	case 0:
		wbuf[1] = 0xFF; /* disable */
		break;
	case 1:
		wbuf[1] = 0x24; /* enable */
		wbuf[2] = 0x01;
		info->sensitivity_mode = 1;
		break;
	case 2:
		wbuf[1] = 0x24; /* flex mode enable */
		wbuf[2] = 0x02;
		info->sensitivity_mode = 2;
		break;
	default:
		break;
	}

	ret = fts_write_reg(info, wbuf, 3);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: write failed. ret: %d\n", __func__, ret);
		return ret;
	}

	fts_delay(30);

	input_info(true, &info->client->dev, "%s: %ld\n", __func__, value);
	return count;
}

static ssize_t sensitivity_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	u8 *rbuf;
	u8 reg_read = FTS_READ_SENSITIVITY_VALUE;
	int ret, i;
	s16 value[10];
	u8 count = 9;
	char *buffer;
	ssize_t len;

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		return -EPERM;
	}

	if (info->sensitivity_mode == 2)	/* flex mode */
		count = 12;

	rbuf = kzalloc(count * 2, GFP_KERNEL);
	if (!rbuf)
		return -ENOMEM;

	buffer = kzalloc(SEC_CMD_BUF_SIZE, GFP_KERNEL);
	if (!buffer) {
		kfree(rbuf);
		return -ENOMEM;
	}

	ret = info->fts_read_reg(info, &reg_read, 1, rbuf, count * 2);
	if (ret < 0) {
		kfree(rbuf);
		kfree(buffer);
		input_err(true, &info->client->dev, "%s: read failed ret = %d\n", __func__, ret);
		return ret;
	}

	for (i = 0; i < count; i++) {
		char temp[16];

		memset(temp, 0x00, 16);
		value[i] = rbuf[i * 2] + (rbuf[i * 2 + 1] << 8);
		snprintf(temp, 16, "%d,", value[i]);
		strlcat(buffer, temp, SEC_CMD_BUF_SIZE);
	}

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buffer);
	len = snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buffer);

	kfree(rbuf);
	kfree(buffer);

	return len;
}

/*
 * read_support_feature function
 * returns the bit combination of specific feature that is supported.
 */
static ssize_t read_support_feature(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u32 feature = 0;

	if (info->board->enable_settings_aot)
		feature |= INPUT_FEATURE_ENABLE_SETTINGS_AOT;

	if (info->board->sync_reportrate_120)
		feature |= INPUT_FEATURE_ENABLE_SYNC_RR120;

	if (info->board->support_open_short_test)
		feature |= INPUT_FEATURE_SUPPORT_OPEN_SHORT_TEST;

	if (info->board->support_mis_calibration_test)
		feature |= INPUT_FEATURE_SUPPORT_MIS_CALIBRATION_TEST;

	snprintf(buff, sizeof(buff), "%d", feature);
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buff);
}

#ifdef FTS_SUPPORT_SPONGELIB
#ifdef CONFIG_TOUCHSCREEN_DUAL_FOLDABLE
ssize_t get_lp_dump(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info;

	u8 string_data[10] = {0, };
	u16 current_index;
	u8 dump_format, dump_num;
	u16 dump_start, dump_end;
	int i, j, ret;
	u16 addr;
	u8 buffer[30];
	u8 position = FTS_SPONGE_LP_DUMP_DATA_FORMAT_10_LEN;

	if (!sec)
		return -ENODEV;

	info = container_of(sec, struct fts_ts_info, sec);

	if (!info->lp_dump)
		return -ENOMEM;

	if (!info->use_sponge)
		return -ENODEV;

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		goto read_buf;
	}

	if (info->reset_is_on_going) {
		input_err(true, &info->client->dev, "%s: reset is ongoing\n",
				__func__);
		goto read_buf;
	}

	if (info->lp_dump_readmore == 0)
		goto read_buf;

	fts_interrupt_set(info, INT_DISABLE);

	addr = FTS_CMD_SPONGE_LP_DUMP;

	ret = info->fts_read_from_sponge(info, addr, string_data, 4);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: Failed to read from Sponge, addr=0x%X\n", __func__, addr);
		if (buf)
			snprintf(buf, SEC_CMD_BUF_SIZE,
					"NG, Failed to read from Sponge, addr=0x%X", addr);
		fts_interrupt_set(info, INT_ENABLE);
		goto out;
	}

	dump_format = string_data[0];
	dump_num = string_data[1];
	dump_start = FTS_CMD_SPONGE_LP_DUMP + 4;
	dump_end = dump_start + (dump_format * (dump_num - 1));

	if (dump_format > 10) {
		input_err(true, &info->client->dev,
				"%s: abnormal data:0x%X, 0x%X\n", __func__, string_data[0], string_data[1]);
		if (buf)
			snprintf(buf, SEC_CMD_BUF_SIZE, "%s",
					"NG, Failed to read from dump format");
		fts_interrupt_set(info, INT_ENABLE);
		goto out;
	}

	current_index = (string_data[3] & 0xFF) << 8 | (string_data[2] & 0xFF);
	if (current_index > dump_end || current_index < dump_start) {
		input_err(true, &info->client->dev,
				"Failed to Sponge LP log %d\n", current_index);
		if (buf)
			snprintf(buf, SEC_CMD_BUF_SIZE,
					"NG, Failed to Sponge LP log, current_index=%d",
				current_index);
		fts_interrupt_set(info, INT_ENABLE);
		goto out;
	}

	input_info(true, &info->client->dev, "%s: DEBUG format=%d, num=%d, start=%d, end=%d, current_index=%d\n",
				__func__, dump_format, dump_num, dump_start, dump_end, current_index);

	for (i = dump_num - 1 ; i >= 0 ; i--) {
		u16 string_addr;
		int value, j;

		if (current_index < (dump_format * i))
			string_addr = (dump_format * dump_num) + current_index - (dump_format * i);
		else
			string_addr = current_index - (dump_format * i);

		if (string_addr < dump_start)
			string_addr += (dump_format * dump_num);

		addr = string_addr;

		ret = info->fts_read_from_sponge(info, addr, string_data, dump_format);
		if (ret < 0) {
			input_err(true, &info->client->dev,
					"%s: Failed to read from Sponge, addr=0x%X\n", __func__, addr);
			if (buf)
				snprintf(buf, SEC_CMD_BUF_SIZE,
						"NG, Failed to read from Sponge, addr=0x%X", addr);
			fts_interrupt_set(info, INT_ENABLE);
			goto out;
		}

		value = 0;

		for (j = 0; j < 10; j++)
			value += string_data[j];

		if (value == 0)
			continue;

		info->lp_dump[info->lp_dump_index] = (string_addr >> 8) & 0xFF;
		info->lp_dump[info->lp_dump_index + 1] = string_addr & 0xFF;

		for (j = 0; j < 10; j += 2) {
			info->lp_dump[info->lp_dump_index + 2 + j] = string_data[j + 1];
			info->lp_dump[info->lp_dump_index + 2 + j + 1] = string_data[j];
		}
		info->lp_dump_index += position;

		if (info->lp_dump_index >= position * FTS_SPONGE_LP_DUMP_LENGTH)
			info->lp_dump_index = 0;
	}

	fts_interrupt_set(info, INT_ENABLE);

read_buf:
	if (buf) {
		int pos = info->lp_dump_index;

		for (i = 0; i < FTS_SPONGE_LP_DUMP_LENGTH; i++) {
			if (pos >= FTS_SPONGE_LP_DUMP_DATA_FORMAT_10_LEN * FTS_SPONGE_LP_DUMP_LENGTH)
				pos = 0;

			if (info->lp_dump[pos] == 0 && info->lp_dump[pos + 1] == 0) {
				pos += position;
				continue;
			}

			snprintf(buffer, sizeof(buffer), "%d: ", info->lp_dump[pos] << 8 | info->lp_dump[pos + 1]);
			strlcat(buf, buffer, PAGE_SIZE);
			memset(buffer, 0x00, sizeof(buffer));
			for (j = 0; j < 10; j++) {
				snprintf(buffer, sizeof(buffer), "%02x", info->lp_dump[pos + 2 + j]);
				strlcat(buf, buffer, PAGE_SIZE);
				memset(buffer, 0x00, sizeof(buffer));
			}
			snprintf(buffer, sizeof(buffer), "\n");
			strlcat(buf, buffer, PAGE_SIZE);
			memset(buffer, 0x00, sizeof(buffer));

			pos += position;
		}
	}
out:
	info->lp_dump_readmore = 0;

	if (buf) {
		if (strlen(buf) == 0)
			snprintf(buf, SEC_CMD_BUF_SIZE, "%s", "empty");

		return strlen(buf);
	}

	return ret;
}
#else
static ssize_t get_lp_dump(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	u8 string_data[10] = {0, };
	u16 current_index;
	u16 dump_start, dump_end, dump_cnt;
	int i, ret, dump_area, dump_gain;
	unsigned char *sec_spg_dat;
	u8 dump_clear_packet = 0x01;
	u16 addr;

	if (!info->use_sponge)
		return -ENODEV;

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", "TSP turned off");
	}

	/* preparing dump buffer */
	sec_spg_dat = vmalloc(FTS_MAX_SPONGE_DUMP_BUFFER);
	if (!sec_spg_dat) {
		input_err(true, &info->client->dev, "%s : Failed!!\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "vmalloc failed");
	}
	memset(sec_spg_dat, 0, FTS_MAX_SPONGE_DUMP_BUFFER);

	fts_interrupt_set(info, INT_DISABLE);

	addr = FTS_CMD_SPONGE_LP_DUMP_CUR_IDX;

	ret = info->fts_read_from_sponge(info, addr, string_data, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: Failed to read from Sponge, addr=0x%X\n", __func__, addr);
		snprintf(buf, SEC_CMD_BUF_SIZE,
				"NG, Failed to read from Sponge, addr=0x%X", addr);
		goto out;
	}

	if (info->sponge_inf_dump)
		dump_gain = 2;
	else
		dump_gain = 1;

	current_index = (string_data[1] & 0xFF) << 8 | (string_data[0] & 0xFF);
	dump_start = FTS_CMD_SPONGE_LP_DUMP_EVENT;
	dump_end = dump_start + (info->sponge_dump_format * ((info->sponge_dump_event * dump_gain) - 1));

	if (current_index > dump_end || current_index < dump_start) {
		input_err(true, &info->client->dev,
				"Failed to Sponge LP log %d\n", current_index);
		snprintf(buf, SEC_CMD_BUF_SIZE,
				"NG, Failed to Sponge LP log, current_index=%d",
				current_index);
		goto out;
	}

	/* legacy get_lp_dump */
	input_info(true, &info->client->dev, "%s: DEBUG format=%d, num=%d, start=%d, end=%d, current_index=%d\n",
		__func__, info->sponge_dump_format, info->sponge_dump_event, dump_start, dump_end, current_index);

	for (i = (info->sponge_dump_event * dump_gain) - 1 ; i >= 0 ; i--) {
		u16 data0, data1, data2, data3, data4;
		char buff[30] = {0, };
		u16 string_addr;

		if (current_index < (info->sponge_dump_format * i))
			string_addr = (info->sponge_dump_format * info->sponge_dump_event * dump_gain) + current_index - (info->sponge_dump_format * i);
		else
			string_addr = current_index - (info->sponge_dump_format * i);

		if (string_addr < dump_start)
			string_addr += (info->sponge_dump_format * info->sponge_dump_event * dump_gain);

		addr = string_addr;

		ret = info->fts_read_from_sponge(info, addr, string_data, info->sponge_dump_format);
		if (ret < 0) {
			input_err(true, &info->client->dev,
					"%s: Failed to read from Sponge, addr=0x%X\n", __func__, addr);
			snprintf(buf, SEC_CMD_BUF_SIZE,
					"NG, Failed to read from Sponge, addr=0x%X", addr);
			goto out;
		}

		data0 = (string_data[1] & 0xFF) << 8 | (string_data[0] & 0xFF);
		data1 = (string_data[3] & 0xFF) << 8 | (string_data[2] & 0xFF);
		data2 = (string_data[5] & 0xFF) << 8 | (string_data[4] & 0xFF);
		data3 = (string_data[7] & 0xFF) << 8 | (string_data[6] & 0xFF);
		data4 = (string_data[9] & 0xFF) << 8 | (string_data[8] & 0xFF);

		if (data0 || data1 || data2 || data3 || data4) {
			if (info->sponge_dump_format == 10) {
				snprintf(buff, sizeof(buff),
						"%d: %04x%04x%04x%04x%04x\n",
						string_addr, data0, data1, data2, data3, data4);
			} else {
				snprintf(buff, sizeof(buff),
						"%d: %04x%04x%04x%04x\n",
						string_addr, data0, data1, data2, data3);
			}
			strlcat(buf, buff, PAGE_SIZE);
		}
	}

	if (info->sponge_inf_dump) {
		if (current_index >= info->sponge_dump_border) {
			dump_cnt = ((current_index - (info->sponge_dump_border)) / info->sponge_dump_format) + 1;
			dump_area = 1;
			addr = info->sponge_dump_border;
		} else {
			dump_cnt = ((current_index - FTS_CMD_SPONGE_LP_DUMP_EVENT) / info->sponge_dump_format) + 1;
			dump_area = 0;
			addr = FTS_CMD_SPONGE_LP_DUMP_EVENT;
		}

		ret = info->fts_read_from_sponge(info, addr, sec_spg_dat, dump_cnt * info->sponge_dump_format);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: Failed to read sponge\n", __func__);
			goto out;
		}

		for (i = 0 ; i <= dump_cnt ; i++) {
			int e_offset = i * info->sponge_dump_format;
			char ibuff[30] = {0, };
			u16 edata[5];

			edata[0] = (sec_spg_dat[1 + e_offset] & 0xFF) << 8 | (sec_spg_dat[0 + e_offset] & 0xFF);
			edata[1] = (sec_spg_dat[3 + e_offset] & 0xFF) << 8 | (sec_spg_dat[2 + e_offset] & 0xFF);
			edata[2] = (sec_spg_dat[5 + e_offset] & 0xFF) << 8 | (sec_spg_dat[4 + e_offset] & 0xFF);
			edata[3] = (sec_spg_dat[7 + e_offset] & 0xFF) << 8 | (sec_spg_dat[6 + e_offset] & 0xFF);
			edata[4] = (sec_spg_dat[9 + e_offset] & 0xFF) << 8 | (sec_spg_dat[8 + e_offset] & 0xFF);

			if (edata[0] || edata[1] || edata[2] || edata[3] || edata[4]) {
				snprintf(ibuff, sizeof(ibuff), "%03d: %04x%04x%04x%04x%04x\n",
						i + (info->sponge_dump_event * dump_area),
						edata[0], edata[1], edata[2], edata[3], edata[4]);
				sec_tsp_sponge_log(ibuff);
			}
		}

		info->sponge_dump_delayed_flag = false;
		ret = fts_write_to_sponge(info, FTS_CMD_SPONGE_DUMP_FLUSH,
				&dump_clear_packet, 1);
		if (ret < 0)
			input_err(true, &info->client->dev, "%s: Failed to clear sponge dump\n", __func__);
	}

out:
	vfree(sec_spg_dat);
	fts_interrupt_set(info, INT_ENABLE);

	return strlen(buf);
}
#endif
#endif

static ssize_t prox_power_off_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	input_info(true, &info->client->dev, "%s: %d\n", __func__,
			info->prox_power_off);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", info->prox_power_off);
}

static ssize_t prox_power_off_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	int ret, data;

	ret = kstrtoint(buf, 10, &data);
	if (ret < 0)
		return ret;

	input_info(true, &info->client->dev, "%s: %d\n", __func__, data);

	info->prox_power_off = data;

	return count;
}

static ssize_t protos_event_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	input_info(true, &info->client->dev, "%s: %d\n", __func__, info->hover_event);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", info->hover_event != 3 ? 0 : 3);
}

static ssize_t protos_event_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	int ret;
	u8 data[2];

	ret = kstrtou8(buf, 8, &data[1]);
	if (ret < 0)
		return ret;

	data[0] = FTS_CMD_SET_EAR_DETECT;

	ret = fts_write_reg(info, data, 2);
	input_info(true, &info->client->dev, "%s: set %d: ret:%d\n", __func__, data[1], ret);

	return count;
}

static ssize_t fts_fod_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	if (!info->board->support_fod) {
		input_err(true, &info->client->dev, "%s: fod is not supported\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "NG");
	}

	input_info(true, &info->client->dev, "%s: x:%d/%d y:%d/%d size:%d\n",
			__func__, info->fod_x, info->ForceChannelLength,
			info->fod_y, info->SenseChannelLength, info->fod_vi_size);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d,%d,%d,%d,%d\n",
				info->fod_x, info->fod_y, info->fod_vi_size,
				info->ForceChannelLength, info->SenseChannelLength);
}

static ssize_t fts_fod_position_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[3] = { 0 };
	int i;

	if (!info->board->support_fod) {
		input_err(true, &info->client->dev, "%s: fod is not supported\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "NG");
	}

	if (!info->fod_vi_size) {
		input_err(true, &info->client->dev, "%s: fod vi size is 0\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "NG");
	}

	if (!info->fod_vi_data) {
		input_err(true, &info->client->dev, "%s: fod vi data is null\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "NG");
	}

	for (i = 0; i < info->fod_vi_size; i++) {
		snprintf(buff, 3, "%02X", info->fod_vi_data[i]);
		strlcat(buf, buff, SEC_CMD_BUF_SIZE);
	}

	return strlen(buf);
}

#ifdef CONFIG_TOUCHSCREEN_DUAL_FOLDABLE
static ssize_t dualscreen_policy_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	int ret, value;

	if (!info->board->support_flex_mode)
		return count;

	ret = kstrtoint(buf, 10, &value);
	if (ret < 0)
		return ret;

	input_info(true, &info->client->dev, "%s: power_state[%d] %sfolding\n",
					__func__, info->fts_power_state, info->flip_status_current ? "" : "un");

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN && info->flip_status_current == FTS_STATUS_UNFOLDING) {
		cancel_delayed_work(&info->switching_work);
		schedule_work(&info->switching_work.work);
	}

	return count;
}
#endif

static DEVICE_ATTR(hw_param, 0664, hardware_param_show, hardware_param_store);
static DEVICE_ATTR(read_ambient_info, 0444, read_ambient_info_show, NULL);
static DEVICE_ATTR(sensitivity_mode, 0664, sensitivity_mode_show, sensitivity_mode_store);
static DEVICE_ATTR(scrub_pos, 0444, fts_scrub_position, NULL);
static DEVICE_ATTR(support_feature, 0444, read_support_feature, NULL);
#ifdef FTS_SUPPORT_SPONGELIB
static DEVICE_ATTR(get_lp_dump, 0444, get_lp_dump, NULL);
#endif
static DEVICE_ATTR(prox_power_off, 0664, prox_power_off_show, prox_power_off_store);
static DEVICE_ATTR(virtual_prox, 0664, protos_event_show, protos_event_store);
static DEVICE_ATTR(fod_info, 0444, fts_fod_info_show, NULL);
static DEVICE_ATTR(fod_pos, 0444, fts_fod_position_show, NULL);
#ifdef CONFIG_TOUCHSCREEN_DUAL_FOLDABLE
static DEVICE_ATTR(dualscreen_policy, 0664, NULL, dualscreen_policy_store);
#endif
static struct attribute *sec_touch_facotry_attributes[] = {
	&dev_attr_scrub_pos.attr,
	&dev_attr_hw_param.attr,
	&dev_attr_read_ambient_info.attr,
	&dev_attr_sensitivity_mode.attr,
	&dev_attr_support_feature.attr,
#ifdef FTS_SUPPORT_SPONGELIB
	&dev_attr_get_lp_dump.attr,
#endif
	&dev_attr_prox_power_off.attr,
	&dev_attr_virtual_prox.attr,
	&dev_attr_fod_info.attr,
	&dev_attr_fod_pos.attr,
#ifdef CONFIG_TOUCHSCREEN_DUAL_FOLDABLE
	&dev_attr_dualscreen_policy.attr,
#endif
	NULL,
};

static struct attribute_group sec_touch_factory_attr_group = {
	.attrs = sec_touch_facotry_attributes,
};

static ssize_t fts_get_cmoffset_dump(struct fts_ts_info *info, char *buf, u8 position)
{
	u8 regaddr[4] = { 0 };
	u8 *rbuff;
	int ret, i, j, size = info->SenseChannelLength * info->ForceChannelLength;
	u32 signature;

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: Touch is stopped\n", __func__);
		return -EPERM;
	}
	if (info->fts_power_state == FTS_POWER_STATE_LOWPOWER) {
		input_err(true, &info->client->dev, "%s: Touch is LP mode\n", __func__);
		return -EPERM;
	}

	if (info->reset_is_on_going) {
		input_err(true, &info->client->dev, "%s: Reset is ongoing!\n", __func__);
		return -EPERM;
	}

	if (info->sec.cmd_is_running) {
		input_err(true, &info->client->dev, "%s: cmd is running\n", __func__);
		return -EBUSY;
	}

	rbuff = kzalloc(size, GFP_KERNEL);
	if (!rbuff) {
		input_err(true, &info->client->dev, "%s: alloc failed\n", __func__);
		return -ENOMEM;
	}

	info->fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true);

	fts_interrupt_set(info, INT_DISABLE);

	fts_release_all_finger(info);

	/* Request SEC factory debug data from flash */
	regaddr[0] = 0xA4;
	regaddr[1] = 0x06;
	regaddr[2] = 0x92;
	regaddr[3] = position;
	ret = fts_fw_wait_for_echo_event(info, regaddr, 4, 0);
	if (ret < 0) {
		snprintf(buf, info->proc_cmoffset_size, "NG, failed to request data, %d", ret);
		goto out;
	}

	/* read header info */
	regaddr[0] = 0xA6;
	regaddr[1] = 0x00;
	regaddr[2] = 0x00;
	ret = info->fts_read_reg(info, &regaddr[0], 3, rbuff, 8);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: read header failed. ret: %d\n", __func__, ret);
		snprintf(buf, info->proc_cmoffset_size, "NG, failed to read header, %d", ret);
		goto out;
	}

	signature = rbuff[3] << 24 | rbuff[2] << 16 | rbuff[1] << 8 | rbuff[0];
	input_info(true, &info->client->dev,
			"%s: position:%d, signature:%08X (%X), validation:%X, try count:%X\n",
			__func__, position, signature, SEC_OFFSET_SIGNATURE, rbuff[4], rbuff[5]);

	if (signature != SEC_OFFSET_SIGNATURE) {
		input_err(true, &info->client->dev, "%s: cmoffset[%d], signature is mismatched\n",
				__func__, position);
		snprintf(buf, info->proc_cmoffset_size, "signature mismatched %08X\n", signature);
		goto out;
	}

	/* read history data */
	regaddr[0] = 0xA6;
	regaddr[1] = 0x00;
	regaddr[2] = (u8)info->SenseChannelLength;
	ret = info->fts_read_reg(info, &regaddr[0], 3, rbuff, size);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: read data failed. ret: %d\n", __func__, ret);
		snprintf(buf, info->proc_cmoffset_size, "NG, failed to read data %d", ret);
		goto out;
	}

	memset(buf, 0x00, info->proc_cmoffset_size);
	for (i = 0; i < info->ForceChannelLength; i++) {
		char buff[4] = { 0 };

		for (j = 0; j < info->SenseChannelLength; j++) {
			snprintf(buff, sizeof(buff), " %d", rbuff[i * info->SenseChannelLength + j]);
			strlcat(buf, buff, info->proc_cmoffset_size);
		}
		snprintf(buff, sizeof(buff), "\n");
		strlcat(buf, buff, info->proc_cmoffset_size);
	}

out:
	input_err(true, &info->client->dev, "%s: pos:%d, buf size:%zu\n", __func__, position, strlen(buf));

	fts_interrupt_set(info, INT_ENABLE);
	kfree(rbuff);
	return strlen(buf);
}

static void enter_factory_mode(struct fts_ts_info *info, bool fac_mode)
{
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN)
		return;

	info->fts_systemreset(info, 50);

	fts_release_all_finger(info);

	if (fac_mode) {
		// Auto-Tune without saving
		fts_execute_autotune(info, false);

		fts_delay(50);
	}

	fts_set_scanmode(info, info->scan_mode);

	fts_delay(50);
}

static int fts_check_index(struct fts_ts_info *info)
{
	struct sec_cmd_data *sec = &info->sec;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int node;

	if (sec->cmd_param[0] < 0
			|| sec->cmd_param[0] >= info->SenseChannelLength
			|| sec->cmd_param[1] < 0
			|| sec->cmd_param[1] >= info->ForceChannelLength) {

		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		input_err(true, &info->client->dev, "%s: parameter error: %u,%u\n",
				__func__, sec->cmd_param[0], sec->cmd_param[1]);
		node = -1;
		return node;
	}
	node = sec->cmd_param[1] * info->SenseChannelLength + sec->cmd_param[0];
	input_info(true, &info->client->dev, "%s: node = %d\n", __func__, node);
	return node;
}

static ssize_t fts_scrub_position(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	input_info(true, &info->client->dev, "%s: %d %d %d\n",
			__func__, info->scrub_id, info->scrub_x, info->scrub_y);
	snprintf(buff, sizeof(buff), "%d %d %d", info->scrub_id, info->scrub_x, info->scrub_y);

	info->scrub_x = 0;
	info->scrub_y = 0;

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s\n", buff);
}

static void not_support_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "%s", "NA");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void fw_update(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[64] = { 0 };
	int retval = 0;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	retval = fts_fw_update_on_hidden_menu(info, sec->cmd_param[0]);

	if (retval < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		input_err(true, &info->client->dev, "%s: failed [%d]\n", __func__, retval);
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_OK;
		input_info(true, &info->client->dev, "%s: success [%d]\n", __func__, retval);
	}
}

static int fts_get_channel_info(struct fts_ts_info *info)
{
	int rc = -1;
	u8 regAdd[1] = { FTS_READ_PANEL_INFO };
	u8 data[FTS_EVENT_SIZE] = { 0 };

	memset(data, 0x0, FTS_EVENT_SIZE);

	rc = info->fts_read_reg(info, regAdd, 1, data, 11);
	if (rc < 0) {
		info->ForceChannelLength = 0;
		info->SenseChannelLength = 0;
		input_err(true, &info->client->dev, "%s: Get channel info Read Fail!!\n", __func__);
		return rc;
	}

	info->ForceChannelLength = data[8]; // Number of TX CH
	info->SenseChannelLength = data[9]; // Number of RX CH

	if (!(info->ForceChannelLength > 0 && info->ForceChannelLength < FTS_MAX_NUM_FORCE &&
		info->SenseChannelLength > 0 && info->SenseChannelLength < FTS_MAX_NUM_SENSE)) {
		info->ForceChannelLength = FTS_MAX_NUM_FORCE;
		info->SenseChannelLength = FTS_MAX_NUM_SENSE;
		input_err(true, &info->client->dev, "%s: set channel num based on max value, check it!\n", __func__);
	}

	info->ICXResolution = (data[0] << 8) | data[1]; // X resolution of IC
	info->ICYResolution = (data[2] << 8) | data[3]; // Y resolution of IC

	input_info(true, &info->client->dev, "%s: RX:Sense(%02d) TX:Force(%02d) resolution:(IC)x:%d y:%d, (DT)x:%d,y:%d\n",
		__func__, info->SenseChannelLength, info->ForceChannelLength,
		info->ICXResolution, info->ICYResolution, info->board->max_x, info->board->max_y);

	if (info->ICXResolution > 0 && info->ICXResolution < FTS_MAX_X_RESOLUTION &&
		info->ICYResolution > 0 && info->ICYResolution < FTS_MAX_Y_RESOLUTION &&
		info->board->max_x != info->ICXResolution && info->board->max_y != info->ICYResolution) {
		info->board->max_x = info->ICXResolution;
		info->board->max_y = info->ICYResolution;
		input_err(true, &info->client->dev, "%s: set resolution based on ic value, check it!\n", __func__);
	}

	return rc;
}

static void fts_print_channel(struct fts_ts_info *info, s16 *tx_data, s16 *rx_data)
{
	unsigned char *pStr = NULL;
	unsigned char pTmp[16] = { 0 };
	int len, max_num  = 0;
	int i = 0, k = 0;
	int tx_count = info->ForceChannelLength;
	int rx_count = info->SenseChannelLength;

	if (tx_count > 20)
		max_num = 10;
	else
		max_num = tx_count;

	if (!max_num)
		return;

	len = 7 * (max_num + 1);
	pStr = vzalloc(len);
	if (!pStr)
		return;

	/* Print TX channel */
	memset(pStr, 0x0, len);
	snprintf(pTmp, sizeof(pTmp), " TX");
	strlcat(pStr, pTmp, len);

	for (k = 0; k < max_num; k++) {
		snprintf(pTmp, sizeof(pTmp), "    %02d", k);
		strlcat(pStr, pTmp, len);
	}
	input_raw_info_d(true, &info->client->dev, "%s\n", pStr);

	memset(pStr, 0x0, len);
	snprintf(pTmp, sizeof(pTmp), " +");
	strlcat(pStr, pTmp, len);

	for (k = 0; k < max_num; k++) {
		snprintf(pTmp, sizeof(pTmp), "------");
		strlcat(pStr, pTmp, len);
	}
	input_raw_info_d(true, &info->client->dev, "%s\n", pStr);

	memset(pStr, 0x0, len);
	snprintf(pTmp, sizeof(pTmp), " | ");
	strlcat(pStr, pTmp, len);

	for (i = 0; i < tx_count; i++) {
		if (i && i % max_num == 0) {
			input_raw_info_d(true, &info->client->dev, "%s\n", pStr);
			memset(pStr, 0x0, len);
			snprintf(pTmp, sizeof(pTmp), " | ");
			strlcat(pStr, pTmp, len);
		}

		snprintf(pTmp, sizeof(pTmp), " %5d", tx_data[i]);
		strlcat(pStr, pTmp, len);
	}
	input_raw_info_d(true, &info->client->dev, "%s\n", pStr);

	/* Print RX channel */
	memset(pStr, 0x0, len);
	snprintf(pTmp, sizeof(pTmp), " RX");
	strlcat(pStr, pTmp, len);

	for (k = 0; k < max_num; k++) {
		snprintf(pTmp, sizeof(pTmp), "    %02d", k);
		strlcat(pStr, pTmp, len);
	}

	input_raw_info_d(true, &info->client->dev, "%s\n", pStr);

	memset(pStr, 0x0, len);
	snprintf(pTmp, sizeof(pTmp), " +");
	strlcat(pStr, pTmp, len);

	for (k = 0; k < max_num; k++) {
		snprintf(pTmp, sizeof(pTmp), "------");
		strlcat(pStr, pTmp, len);
	}
	input_raw_info_d(true, &info->client->dev, "%s\n", pStr);

	memset(pStr, 0x0, len);
	snprintf(pTmp, sizeof(pTmp), " | ");
	strlcat(pStr, pTmp, len);

	for (i = 0; i < rx_count; i++) {
		if (i && i % max_num == 0) {
			input_raw_info_d(true, &info->client->dev, "%s\n", pStr);
			memset(pStr, 0x0, len);
			snprintf(pTmp, sizeof(pTmp), " | ");
			strlcat(pStr, pTmp, len);
		}

		snprintf(pTmp, sizeof(pTmp), " %5d", rx_data[i]);
		strlcat(pStr, pTmp, len);
	}
	input_raw_info_d(true, &info->client->dev, "%s\n", pStr);

	vfree(pStr);
}

void fts_print_frame(struct fts_ts_info *info, short *min, short *max)
{
	int i = 0;
	int j = 0;
	u8 *pStr = NULL;
	u8 pTmp[16] = { 0 };

	pStr = kzalloc(BUFFER_MAX, GFP_KERNEL);
	if (pStr == NULL)
		return;

	snprintf(pTmp, 4, "    ");
	strlcat(pStr, pTmp, BUFFER_MAX);

	for (i = 0; i < info->SenseChannelLength; i++) {
		snprintf(pTmp, 6, "Rx%02d  ", i);
		strlcat(pStr, pTmp, BUFFER_MAX);
	}

	input_raw_info_d(true, &info->client->dev, "%s\n", pStr);

	memset(pStr, 0x0, 6 * (info->SenseChannelLength + 1));
	snprintf(pTmp, 2, " +");
	strlcat(pStr, pTmp, BUFFER_MAX);

	for (i = 0; i < info->SenseChannelLength; i++) {
		snprintf(pTmp, 6, "------");
		strlcat(pStr, pTmp, BUFFER_MAX);
	}

	input_raw_info_d(true, &info->client->dev, "%s\n", pStr);

	for (i = 0; i < info->ForceChannelLength; i++) {
		memset(pStr, 0x0, 6 * (info->SenseChannelLength + 1));
		snprintf(pTmp, 7, "Tx%02d | ", i);
		strlcat(pStr, pTmp, BUFFER_MAX);

		for (j = 0; j < info->SenseChannelLength; j++) {
			snprintf(pTmp, 6, "%5d ", info->pFrame[(i * info->SenseChannelLength) + j]);
			strlcat(pStr, pTmp, BUFFER_MAX);

			if (info->pFrame[(i * info->SenseChannelLength) + j] < *min) {
				*min = info->pFrame[(i * info->SenseChannelLength) + j];
				if (info->rawcap_lock) {
					info->rawcap_min = *min;
					info->rawcap_min_tx = i;
					info->rawcap_min_rx = j;
				}
			}

			if (info->pFrame[(i * info->SenseChannelLength) + j] > *max) {
				*max = info->pFrame[(i * info->SenseChannelLength) + j];
				if (info->rawcap_lock) {
					info->rawcap_max = *max;
					info->rawcap_max_tx = i;
					info->rawcap_max_rx = j;
				}
			}
		}
		input_raw_info_d(true, &info->client->dev, "%s\n", pStr);
	}

	input_raw_info_d(true, &info->client->dev, "%s, min:%d, max:%d\n", __func__, *min, *max);

	kfree(pStr);
}

int fts_read_frame(struct fts_ts_info *info, u8 type, short *min, short *max)
{
	struct FTS_SyncFrameHeader *pSyncFrameHeader;

	u8 regAdd[8] = { 0 };

	unsigned int totalbytes = 0;
	u8 *pRead;
	int rc = 0;
	int ret = 0;
	int i = 0;
	int retry = 10;

	pRead = kzalloc(info->ForceChannelLength * info->SenseChannelLength * 3 + 1, GFP_KERNEL);
	if (!pRead)
		return -ENOMEM;

	/* Request Data Type */
	regAdd[0] = 0xA4;
	regAdd[1] = 0x06;
	regAdd[2] = (u8)type;
	info->fts_write_reg(info, &regAdd[0], 3);
	fts_delay(50);

	do {
		regAdd[0] = 0xA6;
		regAdd[1] = 0x00;
		regAdd[2] = 0x00;
		ret = info->fts_read_reg(info, &regAdd[0], 3, &pRead[0], FTS_COMP_DATA_HEADER_SIZE);
		if (ret <= 0) {
			input_err(true, &info->client->dev, "%s: read failed rc = %d\n", __func__, ret);
			rc = -3;
			goto ErrorExit;
		}

		pSyncFrameHeader = (struct FTS_SyncFrameHeader *) &pRead[0];

		if ((pSyncFrameHeader->header == 0xA5) && (pSyncFrameHeader->host_data_mem_id == type))
			break;

		fts_delay(100);
	} while (retry--);

	if (retry == 0) {
		input_err(true, &info->client->dev,
				"%s: didn't match header or id. header = %02X, id = %02X\n",
				__func__, pSyncFrameHeader->header, pSyncFrameHeader->host_data_mem_id);
		rc = -4;
		goto ErrorExit;
	}

	totalbytes = (info->ForceChannelLength * info->SenseChannelLength  * 2);

	regAdd[0] = 0xA6;
	regAdd[1] = 0x00;
	regAdd[2] = FTS_COMP_DATA_HEADER_SIZE + pSyncFrameHeader->dbg_frm_len;
	ret = info->fts_read_reg(info, &regAdd[0], 3, &pRead[0], totalbytes);
	if (ret <= 0) {
		input_err(true, &info->client->dev, "%s: read failed rc = %d\n", __func__, ret);
		rc = -5;
		goto ErrorExit;
	}

	for (i = 0; i < totalbytes / 2; i++)
		info->pFrame[i] = (short)(pRead[i * 2] + (pRead[i * 2 + 1] << 8));

	switch (type) {
	case TYPE_RAW_DATA:
		input_raw_info_d(true, &info->client->dev, "%s", "[Raw Data]\n");
		break;
	case TYPE_STRENGTH_DATA:
		input_raw_info_d(true, &info->client->dev, "%s: [Strength Data]\n", __func__);
		break;
	case TYPE_BASELINE_DATA:
		input_raw_info_d(true, &info->client->dev, "%s: [Baseline Data]\n", __func__);
		break;
	}

	fts_print_frame(info, min, max);

ErrorExit:
	kfree(pRead);
	return rc;
}

int fts_read_nonsync_frame(struct fts_ts_info *info, short *min, short *max)
{
	struct FTS_SyncFrameHeader *pSyncFrameHeader;

	u8 regAdd[8] = { 0 };

	unsigned int totalbytes = 0;
	u8 *pRead;
	int rc = 0;
	int ret = 0;
	int i = 0;
	int retry = 10;

	input_info(true, &info->client->dev, "%s\n", __func__);

	pRead = kzalloc(info->ForceChannelLength * info->SenseChannelLength * 3 + 1, GFP_KERNEL);
	if (!pRead)
		return -ENOMEM;

	/* Request System Information */
	regAdd[0] = 0xA4;
	regAdd[1] = 0x06;
	regAdd[2] = 0x01;
	info->fts_write_reg(info, &regAdd[0], 3);
	fts_delay(50);

	do {
		regAdd[0] = 0xA6;
		regAdd[1] = 0x00;
		regAdd[2] = 0x00;
		ret = info->fts_read_reg(info, &regAdd[0], 3, &pRead[0], FTS_COMP_DATA_HEADER_SIZE);
		if (ret <= 0) {
			input_err(true, &info->client->dev, "%s: read failed rc = %d\n", __func__, ret);
			rc = -3;
			goto ErrorExit;
		}

		pSyncFrameHeader = (struct FTS_SyncFrameHeader *) &pRead[0];

		if ((pSyncFrameHeader->header == 0xA5) && (pSyncFrameHeader->host_data_mem_id == 0x01))
			break;

		fts_delay(100);
	} while (retry--);

	if (retry == 0) {
		input_err(true, &info->client->dev,
				"%s: didn't match header or id. header = %02X, id = %02X\n",
				__func__, pSyncFrameHeader->header, pSyncFrameHeader->host_data_mem_id);
		rc = -4;
		goto ErrorExit;
	}

	regAdd[0] = 0xA6;
	regAdd[1] = 0x00;
	regAdd[2] = 0x88;   // ms screen rawdata
	ret = info->fts_read_reg(info, &regAdd[0], 3, &pRead[0], 2);
	if (ret <= 0) {
		input_err(true, &info->client->dev, "%s: read failed rc = %d\n", __func__, ret);
		rc = -5;
		goto ErrorExit;
	}

	totalbytes = (info->ForceChannelLength * info->SenseChannelLength  * 2);

	regAdd[0] = 0xA6;
	regAdd[1] = (u8)pRead[1];
	regAdd[2] = (u8)pRead[0];
	ret = info->fts_read_reg(info, &regAdd[0], 3, &pRead[0], totalbytes);
	if (ret <= 0) {
		input_err(true, &info->client->dev, "%s: read failed rc = %d\n", __func__, ret);
		rc = -6;
		goto ErrorExit;
	}

	for (i = 0; i < totalbytes / 2; i++)
		info->pFrame[i] = (short)(pRead[i * 2] + (pRead[i * 2 + 1] << 8));

	fts_print_frame(info, min, max);

ErrorExit:
	kfree(pRead);
	return rc;
}

void fts_get_sec_ito_test_result(struct fts_ts_info *info)
{
	struct sec_cmd_data *sec = &info->sec;
	struct fts_sec_panel_test_result result[10];
	u8 regAdd[3] = { 0 };
	u8 data[sizeof(struct fts_sec_panel_test_result) * 10 + 2] = { 0 };
	int ret, i, max_count = 0;
	u8 length = sizeof(data);
	u8 buff[100] = { 0 };
	u8 pos_buf[6] = { 0 };

	regAdd[0] = 0xA4;
	regAdd[1] = 0x06;
	regAdd[2] = 0x94;

	ret = fts_fw_wait_for_echo_event(info, regAdd, 3, 0);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to get echo, %d\n", __func__, ret);
		goto done;
	}

	regAdd[0] = 0xA6;
	regAdd[1] = 0x00;
	regAdd[2] = 0x00;
	ret = info->fts_read_reg(info, regAdd, 3, data, length);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to read data, %d\n", __func__, ret);
		goto done;
	}

	memcpy(result, &data[2], length - 2);
	memset(info->ito_result, 0x00, FTS_ITO_RESULT_PRINT_SIZE);

	snprintf(buff, sizeof(buff), "%s: test count - sub:%d, main:%d\n", __func__, data[0], data[1]);
	input_info(true, &info->client->dev, "%s", buff);
	strlcat(info->ito_result, buff, FTS_ITO_RESULT_PRINT_SIZE);

	snprintf(buff, sizeof(buff), "ITO:              /   TX_GAP_MAX   /   RX_GAP_MAX\n");
	input_info(true, &info->client->dev, "%s", buff);
	strlcat(info->ito_result, buff, FTS_ITO_RESULT_PRINT_SIZE);

	for (i = 0; i < 10; i++) {
		switch (result[i].flag) {
		case OFFSET_FAC_SUB:
			snprintf(pos_buf, sizeof(pos_buf), "SUB ");
			break;
		case OFFSET_FAC_MAIN:
			snprintf(pos_buf, sizeof(pos_buf), "MAIN");
			break;
		case OFFSET_FAC_NOSAVE:
		default:
			snprintf(pos_buf, sizeof(pos_buf), "NONE");
			break;
		}

		snprintf(buff, sizeof(buff), "ITO: [%3d] %d-%s / Tx%02d,Rx%02d: %3d / Tx%02d,Rx%02d: %3d\n",
				result[i].num_of_test, result[i].flag, pos_buf,
				result[i].tx_of_txmax_gap, result[i].rx_of_txmax_gap,
				result[i].max_of_tx_gap,
				result[i].tx_of_rxmax_gap, result[i].rx_of_rxmax_gap,
				result[i].max_of_rx_gap);
		input_info(true, &info->client->dev, "%s", buff);
		strlcat(info->ito_result, buff, FTS_ITO_RESULT_PRINT_SIZE);

		/* when count is over 200, it restart from 1 */
		if (result[i].num_of_test > result[max_count].num_of_test + 100)
			continue;
		if (result[i].num_of_test > result[max_count].num_of_test)
			max_count = i;
		if (result[i].num_of_test == 1 && result[max_count].num_of_test == 200)
			max_count = i;
	}

	input_info(true, &info->client->dev, "%s: latest test is %d\n",
			__func__, result[max_count].num_of_test);

done:
	if (sec->cmd_all_factory_state != SEC_CMD_STATUS_RUNNING)
		return;

	if (ret < 0) {
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CH_OPEN/SHORT_TEST_X");
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CH_OPEN/SHORT_TEST_Y");
	} else {
		snprintf(buff, sizeof(buff), "0,%d", result[max_count].max_of_rx_gap);
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CH_OPEN/SHORT_TEST_X");
		snprintf(buff, sizeof(buff), "0,%d", result[max_count].max_of_tx_gap);
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CH_OPEN/SHORT_TEST_Y");
	}
}

int fts_set_sec_ito_test_result(struct fts_ts_info *info)
{
	struct sec_cmd_data *sec = &info->sec;
	u8 regAdd[3] = { 0 };
	int ret = -EINVAL;
	u8 buff[10] = { 0 };

	if (!info->factory_position) {
		input_err(true, &info->client->dev, "%s: not save, factory level = %d\n",
				__func__, info->factory_position);
		goto out;
	}

	regAdd[0] = 0xC7;
	regAdd[1] = 0x06;
	regAdd[2] = info->factory_position;
	ret = info->fts_write_reg(info, regAdd, 3);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to write fac position, %d\n", __func__, ret);
		goto out;
	}

	regAdd[0] = 0xA4;
	regAdd[1] = 0x05;
	regAdd[2] = 0x04;

	ret = fts_fw_wait_for_echo_event(info, regAdd, 3, 200);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to get echo, %d\n", __func__, ret);
		goto out;
	}

	input_info(true, &info->client->dev, "%s: position %d result is saved\n", __func__, info->factory_position);
	return 0;

out:
	if (sec->cmd_all_factory_state != SEC_CMD_STATUS_RUNNING)
		return ret;

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CH_OPEN/SHORT_TEST_X");
	sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CH_OPEN/SHORT_TEST_Y");
	return ret;
}

int fts_fw_wait_for_jitter_result(struct fts_ts_info *info, u8 *reg, u8 count, s16 *ret1, s16 *ret2)
{
	int rc = 0;
	u8 regAdd;
	u8 data[FTS_EVENT_SIZE];
	int retry = 0;

	mutex_lock(&info->wait_for);

	rc = info->fts_write_reg(info, reg, count);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: failed to write command\n", __func__);
		mutex_unlock(&info->wait_for);
		return rc;
	}

	memset(data, 0x0, FTS_EVENT_SIZE);

	regAdd = FTS_READ_ONE_EVENT;
	rc = -1;
	while (info->fts_read_reg(info, &regAdd, 1, (u8 *)data, FTS_EVENT_SIZE)) {
		if (data[0] != 0x00)
			input_info(true, &info->client->dev,
					"%s: event %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X\n",
					__func__, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

		if ((data[0] == FTS_EVENT_JITTER_RESULT) && (data[1] == 0x03)) {  // Check Jitter result
				if (data[2] == FTS_EVENT_JITTER_MUTUAL_MAX) {
					*ret2 = (s16)((data[3] << 8) + data[4]);
					input_info(true, &info->client->dev, "%s: Mutual max jitter Strength : %d, RX:%d, TX:%d\n",
						__func__, *ret2, data[5], data[6]);
				} else if (data[2] == FTS_EVENT_JITTER_MUTUAL_MIN) {
					*ret1 = (s16)((data[3] << 8) + data[4]);
					input_info(true, &info->client->dev, "%s: Mutual min jitter Strength : %d, RX:%d, TX:%d\n",
						__func__, *ret1, data[5], data[6]);
					rc = 0;
					break;
				} else if (data[2] == FTS_EVENT_JITTER_SELF_TX_P2P) {
					*ret1 = (s16)((data[3] << 8) + data[4]);
					input_info(true, &info->client->dev, "%s: Self TX P2P jitter Strength : %d, TX:%d\n",
						__func__, *ret1, data[6]);
				} else if (data[2] == FTS_EVENT_JITTER_SELF_RX_P2P) {
					*ret2 = (s16)((data[3] << 8) + data[4]);
					input_info(true, &info->client->dev, "%s: Self RX P2P jitter Strength : %d, RX:%d\n",
						__func__, *ret2, data[5]);
					rc = 0;
					break;
				}
		} else if (data[0] == FTS_EVENT_ERROR_REPORT) {
			input_info(true, &info->client->dev, "%s: Error detected %02X,%02X,%02X,%02X,%02X,%02X\n",
				__func__, data[0], data[1], data[2], data[3], data[4], data[5]);
			break;
		}

		if (retry++ > FTS_RETRY_COUNT * 10) {
			rc = -1;
			input_err(true, &info->client->dev, "%s: Time Over (%02X,%02X,%02X,%02X,%02X,%02X)\n",
				__func__, data[0], data[1], data[2], data[3], data[4], data[5]);
			break;
		}
		fts_delay(20);
	}

	mutex_unlock(&info->wait_for);
	return rc;
}

static void run_jitter_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[4] = { 0 };
	int ret;
	s16 mutual_min = 0, mutual_max = 0;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN ||
			info->fts_power_state == FTS_POWER_STATE_LOWPOWER) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	info->fts_systemreset(info, 0);
	fts_release_all_finger(info);

	fts_interrupt_set(info, INT_DISABLE);

	/* lock active scan mode */
	regAdd[0] = 0xA0;
	regAdd[1] = 0x03;
	regAdd[2] = 0x00;
	info->fts_write_reg(info, &regAdd[0], 3);
	fts_delay(10);

	// Mutual jitter.
	regAdd[0] = 0xC7;
	regAdd[1] = 0x08;
	regAdd[2] = 0x64;	//100 frame
	regAdd[3] = 0x00;

	ret = fts_fw_wait_for_jitter_result(info, regAdd, 4, &mutual_min, &mutual_max);
	if (ret < 0) {
		input_info(true, &info->client->dev, "%s: failed to read Mutual jitter\n", __func__);
		goto ERROR;
	}

	info->fts_systemreset(info, 0);
	fts_set_scanmode(info, info->scan_mode);

	fts_interrupt_set(info, INT_ENABLE);

	snprintf(buff, sizeof(buff), "%d", mutual_max);
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "MUTUAL_JITTER");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);

	return;

ERROR:
	info->fts_systemreset(info, 0);
	fts_set_scanmode(info, info->scan_mode);

	fts_interrupt_set(info, INT_ENABLE);

	snprintf(buff, sizeof(buff), "NG");
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "MUTUAL_JITTER");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, &info->client->dev, "%s: Fail %s\n", __func__, buff);

	return;
}

static void run_mutual_jitter(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[4] = { 0 };
	int ret;
	s16 mutual_min = 0, mutual_max = 0;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN ||
			info->fts_power_state == FTS_POWER_STATE_LOWPOWER) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	info->fts_systemreset(info, 0);
	fts_release_all_finger(info);

	fts_interrupt_set(info, INT_DISABLE);

	//lock active scan mode.
	fts_fix_active_mode(info, true);

	// Mutual jitter.
	regAdd[0] = 0xC7;
	regAdd[1] = 0x08;
	regAdd[2] = 0x64;	//100 frame
	regAdd[3] = 0x00;

	ret = fts_fw_wait_for_jitter_result(info, regAdd, 4, &mutual_min, &mutual_max);
	if (ret < 0) {
		input_info(true, &info->client->dev, "%s: failed to read Mutual jitter\n", __func__);
		goto ERROR;
	}

	info->fts_systemreset(info, 0);
	fts_set_scanmode(info, info->scan_mode);

	fts_interrupt_set(info, INT_ENABLE);

	snprintf(buff, sizeof(buff), "%d,%d", mutual_min, mutual_max);
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "MUTUAL_JITTER");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_raw_info_d(true, &info->client->dev, "%s: %s\n", __func__, buff);

	return;

ERROR:
	info->fts_systemreset(info, 0);
	fts_set_scanmode(info, info->scan_mode);

	fts_interrupt_set(info, INT_ENABLE);

	snprintf(buff, sizeof(buff), "NG");
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "MUTUAL_JITTER");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_raw_info_d(true, &info->client->dev, "%s: Fail %s\n", __func__, buff);

	return;
}

static void run_self_jitter(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[4] = { 0 };
	int ret;
	s16 tx_p2p = 0, rx_p2p = 0;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN ||
			info->fts_power_state == FTS_POWER_STATE_LOWPOWER) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	info->fts_systemreset(info, 0);
	fts_release_all_finger(info);

	fts_interrupt_set(info, INT_DISABLE);

	/* lock active scan mode */
	regAdd[0] = 0xA0;
	regAdd[1] = 0x03;
	regAdd[2] = 0x00;
	info->fts_write_reg(info, &regAdd[0], 3);
	fts_delay(10);

	/* Self jitter */
	regAdd[0] = 0xC7;
	regAdd[1] = 0x0A;
	regAdd[2] = 0x64;	/* 100 frame */
	regAdd[3] = 0x00;

	ret = fts_fw_wait_for_jitter_result(info, regAdd, 4, &tx_p2p, &rx_p2p);
	if (ret < 0) {
		input_info(true, &info->client->dev, "%s: failed to read Self jitter\n", __func__);
		goto ERROR;
	}

	info->fts_systemreset(info, 0);
	fts_set_scanmode(info, info->scan_mode);

	fts_interrupt_set(info, INT_ENABLE);

	snprintf(buff, sizeof(buff), "%d,%d", tx_p2p, rx_p2p);
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SELF_JITTER");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_raw_info_d(true, &info->client->dev, "%s: %s\n", __func__, buff);

	return;

ERROR:
	info->fts_systemreset(info, 0);
	fts_set_scanmode(info, info->scan_mode);

	fts_interrupt_set(info, INT_ENABLE);

	snprintf(buff, sizeof(buff), "NG");
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SELF_JITTER");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_raw_info_d(true, &info->client->dev, "%s: Fail %s\n", __func__, buff);

	return;

}

static void run_jitter_delta_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 reg[4] = { 0 };
	int ret;
	int result = -1;
	int retry = 0;
	u8 data[FTS_EVENT_SIZE];
	s16 min_of_min = 0;
	s16 max_of_min = 0;
	s16 min_of_max = 0;
	s16 max_of_max = 0;
	s16 min_of_avg = 0;
	s16 max_of_avg = 0;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN ||
			info->fts_power_state == FTS_POWER_STATE_LOWPOWER) {
		goto OUT_JITTER_DELTA;
	}

	info->fts_systemreset(info, 0);
	fts_release_all_finger(info);

	fts_interrupt_set(info, INT_DISABLE);

	mutex_lock(&info->wait_for);

	/* lock active scan mode */
	reg[0] = 0xA0;
	reg[1] = 0x03;
	reg[2] = 0x00;
	ret = info->fts_write_reg(info, &reg[0], 3);
	if (ret < 0) {
		input_info(true, &info->client->dev, "%s: failed to set active mode\n", __func__);
		mutex_unlock(&info->wait_for);
		goto OUT_JITTER_DELTA;
	}
	fts_delay(10);

	/* jitter delta + 1000 frame*/
	reg[0] = 0xC7;
	reg[1] = 0x08;
	reg[2] = 0xE8;
	reg[3] = 0x03;
	ret = info->fts_write_reg(info, &reg[0], 4);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to write command\n", __func__);
		mutex_unlock(&info->wait_for);
		goto OUT_JITTER_DELTA;
	}

	memset(data, 0x0, FTS_EVENT_SIZE);

	reg[0] = FTS_READ_ONE_EVENT;
	while (info->fts_read_reg(info, &reg[0], 1, data, FTS_EVENT_SIZE)) {
		if ((data[0] == FTS_EVENT_JITTER_RESULT) && (data[1] == 0x03)) {
			if (data[2] == FTS_EVENT_JITTER_MUTUAL_MAX) {
				min_of_max = data[3] << 8 | data[4];
				max_of_max = data[8] << 8 | data[9];
				input_info(true, &info->client->dev, "%s: MAX: min:%d, max:%d\n", __func__, min_of_max, max_of_max);
			} else if (data[2] == FTS_EVENT_JITTER_MUTUAL_MIN) {
				min_of_min = data[3] << 8 | data[4];
				max_of_min = data[8] << 8 | data[9];
				input_info(true, &info->client->dev, "%s: MIN: min:%d, max:%d\n", __func__, min_of_min, max_of_min);
			} else if (data[2] == FTS_EVENT_JITTER_MUTUAL_AVG) {
				min_of_avg = data[3] << 8 | data[4];
				max_of_avg = data[8] << 8 | data[9];
				input_info(true, &info->client->dev, "%s: AVG: min:%d, max:%d\n", __func__, min_of_avg, max_of_avg);
				result = 0;
				break;
			}
		} else if (data[0] == FTS_EVENT_ERROR_REPORT) {
			input_info(true, &info->client->dev, "%s: Error detected %02X,%02X,%02X,%02X,%02X,%02X\n",
				__func__, data[0], data[1], data[2], data[3], data[4], data[5]);
			break;
		}

		/* waiting 10 seconds */
		if (retry++ > FTS_RETRY_COUNT * 50) {
			input_err(true, &info->client->dev, "%s: Time Over (%02X,%02X,%02X,%02X,%02X,%02X)\n",
				__func__, data[0], data[1], data[2], data[3], data[4], data[5]);
			break;
		}
		fts_delay(20);
	}
	mutex_unlock(&info->wait_for);

OUT_JITTER_DELTA:

	info->fts_systemreset(info, 0);
	fts_set_scanmode(info, info->scan_mode);

	fts_interrupt_set(info, INT_ENABLE);

	if (result < 0)
		snprintf(buff, sizeof(buff), "NG");
	else
		snprintf(buff, sizeof(buff), "%d,%d,%d,%d,%d,%d", min_of_min, max_of_min, min_of_max, max_of_max, min_of_avg, max_of_avg);

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		char buffer[SEC_CMD_STR_LEN] = { 0 };

		snprintf(buffer, sizeof(buffer), "%d,%d", min_of_min, max_of_min);
		sec_cmd_set_cmd_result_all(sec, buffer, strnlen(buffer, sizeof(buffer)), "JITTER_DELTA_MIN");

		memset(buffer, 0x00, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%d,%d", min_of_max, max_of_max);
		sec_cmd_set_cmd_result_all(sec, buffer, strnlen(buffer, sizeof(buffer)), "JITTER_DELTA_MAX");

		memset(buffer, 0x00, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%d,%d", min_of_avg, max_of_avg);
		sec_cmd_set_cmd_result_all(sec, buffer, strnlen(buffer, sizeof(buffer)), "JITTER_DELTA_AVG");
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

void fts_checking_miscal(struct fts_ts_info *info)
{
	u8 regAdd[3] = { 0 };
	u8 data[2] = { 0 };
	u16 miscal_thd = 0;
	int ret;
	short min = 0x7FFF;
	short max = 0x8000;

	info->miscal_result = MISCAL_PASS;

	info->fts_systemreset(info, 0);
	fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true);

	fts_interrupt_set(info, INT_DISABLE);

	fts_release_all_finger(info);

	/* get the raw data after C7 02 : mis cal test */
	regAdd[0] = 0xC7;
	regAdd[1] = 0x02;

	info->fts_write_reg(info, &regAdd[0], 2);
	msleep(300);
	input_raw_info_d(true, &info->client->dev, "%s: [miscal diff data]\n", __func__);
	fts_read_nonsync_frame(info, &min, &max);

	regAdd[0] = 0xC7;
	regAdd[1] = 0x0B;
	ret = info->fts_read_reg(info, regAdd, 2, data, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev,
			"%s: failed to read miscal threshold\n", __func__);
		return;
	}

	miscal_thd = data[0] << 8 | data[1];

	if (max > miscal_thd)
		info->miscal_result = MISCAL_FAIL;

	if (min < -miscal_thd)
		info->miscal_result = MISCAL_FAIL;

	fts_set_scanmode(info, info->scan_mode);

	input_raw_info_d(true, &info->client->dev, "%s: mis cal threshold:%d/%d, min/max :%d/%d, miscal:%s\n",
			__func__, miscal_thd, -miscal_thd, min, max, info->miscal_result == MISCAL_PASS ? "PASS" : "FAIL");
}

int fts_panel_ito_test(struct fts_ts_info *info, int testmode)
{
	u8 cmd = FTS_READ_ONE_EVENT;
	u8 regAdd[4] = { 0 };
	u8 data[FTS_EVENT_SIZE];
	int i;
	bool matched = false;
	int retry = 0;
	int result = 0;

	result = info->fts_systemreset(info, 0);
	if (result < 0) {
		input_info(true, &info->client->dev, "%s: fts_systemreset fail (%d)\n", __func__, result);
		return result;
	}

	fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true);

	fts_release_all_finger(info);

	fts_interrupt_set(info, INT_DISABLE);

	regAdd[0] = 0xA4;
	regAdd[1] = 0x04;
	switch (testmode) {
	case OPEN_TEST:
		regAdd[2] = 0x00;
		regAdd[3] = 0x30;
		break;

	case OPEN_SHORT_CRACK_TEST:
	case SAVE_MISCAL_REF_RAW:
	default:
		regAdd[2] = 0xFF;
		regAdd[3] = 0x01;
		break;
	}

	info->fts_write_reg(info, &regAdd[0], 4); // ITO test command
	fts_delay(100);

	memset(info->ito_test, 0x0, 4);

	memset(data, 0x0, FTS_EVENT_SIZE);
	while (info->fts_read_reg(info, &cmd, 1, (u8 *)data, FTS_EVENT_SIZE)) {
		if ((data[0] == FTS_EVENT_STATUS_REPORT) && (data[1] == 0x01)) {  // Check command ECHO - finished
			for (i = 0; i < 4; i++) {
				if (data[i + 2] != regAdd[i]) {
					matched = false;
					break;
				}
				matched = true;
			}

			if (matched == true)
				break;
		} else if (data[0] == FTS_EVENT_ERROR_REPORT) {
			info->ito_test[0] = data[0];
			info->ito_test[1] = data[1];
			info->ito_test[2] = data[2];
			info->ito_test[3] = 0x00;
			result = -ITO_FAIL;

			switch (data[1]) {
			case ITO_FORCE_SHRT_GND:
				input_info(true, &info->client->dev, "%s: Force channel [%d] short to GND\n",
						__func__, data[2]);
				result = -ITO_FAIL_SHORT;
				break;

			case ITO_SENSE_SHRT_GND:
				input_info(true, &info->client->dev, "%s: Sense channel [%d] short to GND\n",
						__func__, data[2]);
				result = ITO_FAIL_SHORT;
				break;

			case ITO_FORCE_SHRT_VDD:
				input_info(true, &info->client->dev, "%s: Force channel [%d] short to VDD\n",
						__func__, data[2]);
				result = -ITO_FAIL_SHORT;
				break;

			case ITO_SENSE_SHRT_VDD:
				input_info(true, &info->client->dev, "%s: Sense channel [%d] short to VDD\n",
						__func__, data[2]);
				result = -ITO_FAIL_SHORT;
				break;

			case ITO_FORCE_SHRT_FORCE:
				input_info(true, &info->client->dev, "%s: Force channel [%d] short to force\n",
						__func__, data[2]);
				result = -ITO_FAIL_SHORT;
				break;

			case ITO_SENSE_SHRT_SENSE:
				input_info(true, &info->client->dev, "%s: Sennse channel [%d] short to sense\n",
						__func__, data[2]);
				result = -ITO_FAIL_SHORT;
				break;

			case ITO_FORCE_OPEN:
				input_info(true, &info->client->dev, "%s: Force channel [%d] open\n",
						__func__, data[2]);
				result = -ITO_FAIL_OPEN;
				break;

			case ITO_SENSE_OPEN:
				input_info(true, &info->client->dev, "%s: Sense channel [%d] open\n",
						__func__, data[2]);
				result = -ITO_FAIL_OPEN;
				break;

			case ITO_KEY_OPEN:
				input_info(true, &info->client->dev, "%s: Key channel [%d] open\n",
						__func__, data[2]);
				result = -ITO_FAIL_OPEN;
				break;

			default:
				input_info(true, &info->client->dev, "%s: unknown event %02x %02x %02x %02x %02x %02x %02x %02x\n",
						__func__, data[0], data[1], data[2], data[3],
						data[4], data[5], data[6], data[7]);
				break;
			}
		}

		if (retry++ > 50) {
			result = -ITO_FAIL;
			input_err(true, &info->client->dev, "%s: Time over - wait for result of ITO test\n", __func__);
			break;
		}
		fts_delay(20);
	}

	if (fts_set_sec_ito_test_result(info) >= 0)
		fts_get_sec_ito_test_result(info);

	info->fts_systemreset(info, 0);

	if (info->flip_enable) {
		fts_set_cover_type(info, true);
	} else {
		info->touch_functions = (info->touch_functions & (~FTS_TOUCHTYPE_BIT_COVER)) |
					FTS_TOUCHTYPE_DEFAULT_ENABLE;
#ifdef CONFIG_GLOVE_TOUCH
		if (info->glove_enabled)
			info->touch_functions = info->touch_functions | FTS_TOUCHTYPE_BIT_GLOVE;
		else
			info->touch_functions = info->touch_functions & (~FTS_TOUCHTYPE_BIT_GLOVE);
#endif
	}

	regAdd[0] = FTS_CMD_SET_GET_TOUCHTYPE;
	regAdd[1] = (u8)(info->touch_functions & 0xFF);
	regAdd[2] = (u8)(info->touch_functions >> 8);
	fts_write_reg(info, &regAdd[0], 3);
	fts_delay(10);

#ifdef FTS_SUPPORT_TA_MODE
	if (info->TA_Pluged) {
		u8 wiredCharger;

		regAdd[0] = FTS_CMD_SET_GET_CHARGER_MODE;
		fts_read_reg(info, &regAdd[0], 1, &data[0], 1);
		wiredCharger = data[0] | FTS_BIT_CHARGER_MODE_WIRE_CHARGER;
		regAdd[1] = { wiredCharger };
		fts_write_reg(info, &regAdd[0], 2);
		fts_delay(10);
	}
#endif

	info->touch_count = 0;

	fts_set_scanmode(info, info->scan_mode);

	input_raw_info_d(true, &info->client->dev, "%s: mode:%d [%s] %02X %02X %02X %02X\n",
			__func__, testmode, result < 0 ? "FAIL" : "PASS",
			info->ito_test[0], info->ito_test[1],
			info->ito_test[2], info->ito_test[3]);

	return result;
}

static void get_fw_ver_bin(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "ST%02X%02X%02X%02X",
			info->ic_name_of_bin,
			info->project_id_of_bin,
			info->module_version_of_bin,
			info->fw_main_version_of_bin & 0xFF);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_BIN");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_fw_ver_ic(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };
	char model_ver[7] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
			sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_IC");
			sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_MODEL");
		}
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	fts_get_version_info(info);

	snprintf(buff, sizeof(buff), "ST%02X%02X%02X%02X",
			info->ic_name_of_ic,
			info->project_id_of_ic,
			info->module_version_of_ic,
			info->fw_main_version_of_ic & 0xFF);
	snprintf(model_ver, sizeof(model_ver), "ST%02X%02X",
			info->ic_name_of_ic,
			info->project_id_of_ic);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_IC");
		sec_cmd_set_cmd_result_all(sec, model_ver, strnlen(model_ver, sizeof(model_ver)), "FW_MODEL");
	}
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_config_ver(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[20] = { 0 };

	snprintf(buff, sizeof(buff), "ST_%04X",
			info->config_version_of_ic);

	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_threshold(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	u8 regAdd[2];
	u8 buff[16] = { 0 };
	u8 data[5] = { 0 };
	u16 finger_threshold = 0;
	int rc;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	sec->cmd_state = SEC_CMD_STATUS_RUNNING;

	regAdd[0] = FTS_CMD_SET_GET_TOUCH_MODE_FOR_THRESHOLD;
	regAdd[1] = 0x00;
	info->fts_write_reg(info, &regAdd[0], 2);
	fts_delay(50);

	regAdd[0] = FTS_CMD_SET_GET_TOUCH_THRESHOLD;
	rc = info->fts_read_reg(info, &regAdd[0], 1, data, 2);
	if (rc <= 0) {
		input_err(true, &info->client->dev, "%s: Get threshold Read Fail!! [Data : %2X%2X]\n",
				__func__, data[0], data[1]);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	finger_threshold = (u16)(data[0] << 8 | data[1]);

	snprintf(buff, sizeof(buff), "%d", finger_threshold);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void module_off_master(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[3] = { 0 };
	int ret = 0;

	ret = fts_stop_device(info);

	if (ret == 0)
		snprintf(buff, sizeof(buff), "OK");
	else
		snprintf(buff, sizeof(buff), "NG");

	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (strncmp(buff, "OK", 2) == 0)
		sec->cmd_state = SEC_CMD_STATUS_OK;
	else
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void module_on_master(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[3] = { 0 };
	int ret = 0;

	ret = fts_start_device(info);

	if (ret == 0)
		snprintf(buff, sizeof(buff), "OK");
	else
		snprintf(buff, sizeof(buff), "NG");

	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (strncmp(buff, "OK", 2) == 0)
		sec->cmd_state = SEC_CMD_STATUS_OK;
	else
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_chip_vendor(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };

	snprintf(buff, sizeof(buff), "STM");
	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "IC_VENDOR");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_chip_name(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };

	if (info->firmware_name)
		memcpy(buff, info->firmware_name + 8, 9);
	else
		snprintf(buff, 10, "FTS");

	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "IC_NAME");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_wet_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[3] = { 0 };
	u8 data[2] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
			sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "WET_MODE");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	regAdd[0] = 0xC7;
	regAdd[1] = 0x03;
	ret = info->fts_read_reg(info, &regAdd[0], 2, &data[0], 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: [ERROR] failed to read\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
			sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "WET_MODE");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	snprintf(buff, sizeof(buff), "%d", data[0]);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "WET_MODE");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_x_num(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "%d", info->ForceChannelLength);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_y_num(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "%d", info->SenseChannelLength);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_checksum_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };
	int rc;
	u32 checksum_data;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	info->fts_systemreset(info, 0);

	rc = info->fts_get_sysinfo_data(info, FTS_SI_CONFIG_CHECKSUM, 4, (u8 *)&checksum_data);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: Get checksum data Read Fail!! [Data : %08X]\n",
				__func__, checksum_data);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}
	fts_reinit(info, false);

	snprintf(buff, sizeof(buff), "%08X", checksum_data);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void check_fw_corruption(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };
	int rc;

	sec_cmd_set_default_result(sec);

	if (info->fw_corruption) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		rc = fts_fw_corruption_check(info);
		if (rc == -FTS_ERROR_FW_CORRUPTION) {
			snprintf(buff, sizeof(buff), "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
		} else {
			snprintf(buff, sizeof(buff), "OK");
			sec->cmd_state = SEC_CMD_STATUS_OK;
			fts_reinit(info, false);
		}
	}
	info->fw_corruption = false;

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_reference_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short min = 0x7FFF;
	short max = 0x8000;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	fts_read_frame(info, TYPE_BASELINE_DATA, &min, &max);
	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_reference(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	node = fts_check_index(info);
	if (node < 0)
		return;

	val = info->pFrame[node];
	snprintf(buff, sizeof(buff), "%d", val);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_rawcap_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short min = 0x7FFF;
	short max = 0x8000;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
			sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "RAW_DATA");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	fts_read_frame(info, TYPE_RAW_DATA, &min, &max);
	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "RAW_DATA");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_rawcap_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short min = 0x7FFF;
	short max = 0x8000;
	char *all_strbuff;
	int i, j;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	all_strbuff = kzalloc(info->ForceChannelLength * info->SenseChannelLength * 7 + 1, GFP_KERNEL);
	if (!all_strbuff) {
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_raw_info_d(true, &info->client->dev, "%s\n", __func__);

	enter_factory_mode(info, true);
	fts_read_frame(info, TYPE_RAW_DATA, &min, &max);

	for (j = 0; j < info->ForceChannelLength; j++) {
		for (i = 0; i < info->SenseChannelLength; i++) {
			snprintf(buff, sizeof(buff), "%d,", info->pFrame[j * info->SenseChannelLength + i]);
			strlcat(all_strbuff, buff, info->ForceChannelLength * info->SenseChannelLength * 7);
		}
	}
	enter_factory_mode(info, false);

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, all_strbuff, strlen(all_strbuff));
	input_info(true, &info->client->dev, "%s: %ld\n", __func__, strlen(all_strbuff));
	kfree(all_strbuff);
}

static void run_nonsync_rawcap_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short min = 0x7FFF;
	short max = 0x8000;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
			sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "RAW_DATA");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_raw_info_d(true, &info->client->dev, "%s\n", __func__);

	fts_read_nonsync_frame(info, &min, &max);

	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "RAW_DATA");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}


static void run_nonsync_rawcap_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short min = 0x7FFF;
	short max = 0x8000;
	char *all_strbuff;
	int i, j;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	all_strbuff = kzalloc(info->ForceChannelLength * info->SenseChannelLength * 7 + 1, GFP_KERNEL);
	if (!all_strbuff) {
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

//	enter_factory_mode(info, true);
//	fts_read_frame(info, TYPE_RAW_DATA, &min, &max);
	input_raw_info_d(true, &info->client->dev, "%s\n", __func__);
	fts_read_nonsync_frame(info, &min, &max);

	for (j = 0; j < info->ForceChannelLength; j++) {
		for (i = 0; i < info->SenseChannelLength; i++) {
			snprintf(buff, sizeof(buff), "%d,", info->pFrame[j * info->SenseChannelLength + i]);
			strlcat(all_strbuff, buff, info->ForceChannelLength * info->SenseChannelLength * 7);
		}
	}
	enter_factory_mode(info, false);

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, all_strbuff, strlen(all_strbuff));
	input_info(true, &info->client->dev, "%s: %ld\n", __func__, strlen(all_strbuff));
	kfree(all_strbuff);
}

static void get_rawcap(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	node = fts_check_index(info);
	if (node < 0)
		return;

	val = info->pFrame[node];
	snprintf(buff, sizeof(buff), "%d", val);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

#if 0
static void get_rawcap_gap_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int rx_max = 0, tx_max = 0, ii;

	for (ii = 0; ii < (info->SenseChannelLength * info->ForceChannelLength); ii++) {
		/* rx(x) rawcap gap max */
		if ((ii + 1) % (info->SenseChannelLength) != 0)
			rx_max = max(rx_max, (int)abs(info->pFrame[ii + 1] - info->pFrame[ii]));

		/* tx(y) rawcap gap max */
		if (ii < (info->ForceChannelLength - 1) * info->SenseChannelLength)
			tx_max = max(tx_max, (int)abs(info->pFrame[ii + info->SenseChannelLength] - info->pFrame[ii]));
	}

	input_raw_info_d(true, &info->client->dev, "%s: rx max:%d, tx max:%d\n", __func__, rx_max, tx_max);

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		snprintf(buff, sizeof(buff), "%d,%d", 0, rx_max);
		sec_cmd_set_cmd_result_all(sec, buff, SEC_CMD_STR_LEN, "RAW_DATA_GAP_RX");
		snprintf(buff, sizeof(buff), "%d,%d", 0, tx_max);
		sec_cmd_set_cmd_result_all(sec, buff, SEC_CMD_STR_LEN, "RAW_DATA_GAP_TX");
	}
}
#endif

static void run_lp_single_ended_rawcap_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regaddr[4] = { 0xA4, 0x0A, 0x01, 0x00 };
	int ret;
	short min = 0x7FFF;
	short max = 0x8000;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	info->fts_systemreset(info, 0);

	fts_interrupt_set(info, INT_DISABLE);

	/* Request to Prepare single ended raw data from flash */

	ret = fts_fw_wait_for_echo_event(info, regaddr, 4, 0);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: timeout, ret: %d\n", __func__, ret);
		goto out;
	}

//	run_nonsync_rawcap_read(sec);
	input_raw_info_d(true, &info->client->dev, "%s\n", __func__);
	fts_read_nonsync_frame(info, &min, &max);

	fts_interrupt_set(info, INT_ENABLE);
	fts_set_scanmode(info, info->scan_mode);

	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "MUTUAL_RAW");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);

	return;

out:
	fts_interrupt_set(info, INT_ENABLE);
	fts_set_scanmode(info, info->scan_mode);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "MUTUAL_RAW");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	input_info(true, &info->client->dev, "%s: fail : %s\n", __func__, buff);

	return;
}

static void run_lp_single_ended_rawcap_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regaddr[4] = { 0xA4, 0x0A, 0x01, 0x00 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_raw_info_d(true, &info->client->dev, "%s\n", __func__);

	enter_factory_mode(info, true);

	fts_set_scanmode(info, FTS_SCAN_MODE_SCAN_OFF);
	fts_delay(30);

	fts_interrupt_set(info, INT_DISABLE);

	/* Request to Prepare single ended raw data from flash */

	ret = fts_fw_wait_for_echo_event(info, regaddr, 4, 0);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: timeout, ret: %d\n", __func__, ret);
		goto out;
	}

	fts_interrupt_set(info, INT_ENABLE);

//	run_rawcap_read_all(sec);
	run_nonsync_rawcap_read_all(sec);

//	enter_factory_mode(info, false);
	fts_set_scanmode(info, info->scan_mode);

	return;

out:
	fts_interrupt_set(info, INT_ENABLE);
//	enter_factory_mode(info, false);
	fts_set_scanmode(info, info->scan_mode);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

// Micro Short Test
static void run_low_frequency_rawcap_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[4] = { 0xA4, 0x04, 0x00, 0xC0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	fts_set_scanmode(info, FTS_SCAN_MODE_SCAN_OFF);
	fts_delay(30);

	fts_interrupt_set(info, INT_DISABLE);

	/* Request to Prepare Hight Frequency(ITO) raw data from flash */

	ret = fts_fw_wait_for_echo_event(info, regAdd, 4, 30);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: timeout, ret: %d\n", __func__, ret);
		goto out;
	}

	fts_interrupt_set(info, INT_ENABLE);

	input_raw_info_d(true, &info->client->dev, "%s\n", __func__);
	run_nonsync_rawcap_read(sec);

	fts_set_scanmode(info, info->scan_mode);

	return;

out:
	fts_interrupt_set(info, INT_ENABLE);
	fts_set_scanmode(info, info->scan_mode);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void run_low_frequency_rawcap_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[4] = { 0xA4, 0x04, 0x00, 0xC0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_raw_info_d(true, &info->client->dev, "%s\n", __func__);

	enter_factory_mode(info, true);

	fts_set_scanmode(info, FTS_SCAN_MODE_SCAN_OFF);
	fts_delay(30);

	fts_interrupt_set(info, INT_DISABLE);

	/* Request to Prepare Hight Frequency(ITO) raw data from flash */

	ret = fts_fw_wait_for_echo_event(info, regAdd, 4, 30);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: timeout, ret: %d\n", __func__, ret);
		goto out;
	}

	fts_interrupt_set(info, INT_ENABLE);

//	run_rawcap_read_all(sec);
	run_nonsync_rawcap_read_all(sec);

	return;

out:
	fts_interrupt_set(info, INT_ENABLE);
	fts_set_scanmode(info, info->scan_mode);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void run_high_frequency_rawcap_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[4] = { 0xA4, 0x04, 0xFF, 0x01 };
	int ret;
	int i, j;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	fts_set_scanmode(info, FTS_SCAN_MODE_SCAN_OFF);
	fts_delay(30);

	fts_interrupt_set(info, INT_DISABLE);

	/* Request to Prepare Hight Frequency(ITO) raw data from flash */

	ret = fts_fw_wait_for_echo_event(info, regAdd, 4, 100);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: timeout, ret: %d\n", __func__, ret);
		goto out;
	}

	fts_interrupt_set(info, INT_ENABLE);

	input_raw_info_d(true, &info->client->dev, "%s\n", __func__);
	run_nonsync_rawcap_read(sec);

	fts_set_scanmode(info, info->scan_mode);

	pr_info("sec_input: %s - correlation\n", __func__);
	for (i = 0; i < info->ForceChannelLength; i++) {
		pr_info("sec_input: [%2d] ", i);
		for (j = 0; j < info->SenseChannelLength; j++)
			pr_cont("%d, ", info->pFrame[(i * info->SenseChannelLength) + j]);
		pr_cont("\n");
	}

	return;

out:
	fts_interrupt_set(info, INT_ENABLE);
	fts_set_scanmode(info, info->scan_mode);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void run_high_frequency_rawcap_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[4] = { 0xA4, 0x04, 0xFF, 0x01 };
	int ret;
	int i, j;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_raw_info_d(true, &info->client->dev, "%s\n", __func__);

	enter_factory_mode(info, true);

	fts_set_scanmode(info, FTS_SCAN_MODE_SCAN_OFF);
	fts_delay(30);

	fts_interrupt_set(info, INT_DISABLE);

	/* Request to Prepare Hight Frequency(ITO) raw data from flash */

	ret = fts_fw_wait_for_echo_event(info, regAdd, 4, 100);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: timeout, ret: %d\n", __func__, ret);
		goto out;
	}

	fts_interrupt_set(info, INT_ENABLE);

//	run_rawcap_read_all(sec);
	run_nonsync_rawcap_read_all(sec);
	fts_set_scanmode(info, info->scan_mode);

	pr_info("sec_input: %s - correlation\n", __func__);
	for (i = 0; i < info->ForceChannelLength; i++) {
		pr_info("sec_input: [%2d] ", i);
		for (j = 0; j < info->SenseChannelLength; j++)
			pr_cont("%d, ", info->pFrame[(i * info->SenseChannelLength) + j]);
		pr_cont("\n");
	}

	return;

out:
	fts_set_scanmode(info, info->scan_mode);

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void run_delta_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short min = 0x7FFF;
	short max = 0x8000;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	fts_read_frame(info, TYPE_STRENGTH_DATA, &min, &max);
	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_prox_intensity_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;
	u8 reg[2];
	u8 thd_data[4];
	u8 sum_data[4];

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	memset(thd_data, 0x00, 4);
	memset(sum_data, 0x00, 4);

	/* Threshold */
	reg[0] = 0xC7;
	reg[1] = 0x0C;
	ret = info->fts_read_reg(info, &reg[0], 2, &thd_data[0], 4);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to read thd_data\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	/* Sum */
	reg[0] = 0xC7;
	reg[1] = 0x0D;
	ret = info->fts_read_reg(info, &reg[0], 2, &sum_data[0], 4);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to read sum_data\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	snprintf(buff, sizeof(buff), "SUM_X:%d THD_X:%d SUM_Y:%d THD_Y:%d",
			(sum_data[0] << 8) + sum_data[1], (sum_data[2] << 8) + sum_data[3],
			(thd_data[0] << 8) + thd_data[1], (thd_data[2] << 8) + thd_data[3]);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);

}

static void get_strength_all_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short min = 0x7FFF;
	short max = 0x8000;
	char *all_strbuff;
	int i, j;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	all_strbuff = kzalloc(info->ForceChannelLength * info->SenseChannelLength * 7 + 1, GFP_KERNEL);
	if (!all_strbuff) {
		input_err(true, &info->client->dev, "%s: alloc failed\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	fts_read_frame(info, TYPE_STRENGTH_DATA, &min, &max);

	for (i = 0; i < info->ForceChannelLength; i++) {
		for (j = 0; j < info->SenseChannelLength; j++) {
			snprintf(buff, sizeof(buff), "%d,", info->pFrame[(i * info->SenseChannelLength) + j]);
			strlcat(all_strbuff, buff, info->ForceChannelLength * info->SenseChannelLength * 7);
		}
	}

	sec->cmd_state = SEC_CMD_STATUS_OK;

	sec_cmd_set_cmd_result(sec, all_strbuff, strlen(all_strbuff));
	input_info(true, &info->client->dev, "%s: %ld\n", __func__, strlen(all_strbuff));
	kfree(all_strbuff);
}

static void get_delta(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	node = fts_check_index(info);
	if (node < 0)
		return;

	val = info->pFrame[node];
	snprintf(buff, sizeof(buff), "%d", val);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

int fts_get_hf_data(struct fts_ts_info *info)
{
	u8 regAdd[4] = { 0 };
	int ret;
	short min = 0x7FFF;
	short max = 0x8000;

	info->fts_systemreset(info, 0);

	fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true);

	fts_release_all_finger(info);

	fts_interrupt_set(info, INT_DISABLE);

	input_info(true, &info->client->dev, "%s\n", __func__);

	regAdd[0] = 0xA4;
	regAdd[1] = 0x04;
	regAdd[2] = 0xFF;
	regAdd[3] = 0x01;
	ret = fts_fw_wait_for_echo_event(info, &regAdd[0], 4, 100);
	if (ret < 0)
		goto out;

	ret = fts_read_frame(info, TYPE_RAW_DATA, &min, &max);
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: failed to get rawdata\n", __func__);

out:
	info->fts_systemreset(info, 0);

	fts_set_cover_type(info, info->flip_enable);

	fts_delay(10);

	if (info->charger_mode) {
		fts_wirelesscharger_mode(info);
		fts_delay(10);
	}

	if (!info->flip_enable)
		fts_set_scanmode(info, info->scan_mode);
	else
		fts_set_scanmode(info, FTS_SCAN_MODE_SCAN_OFF);

	return ret;
}

static void run_rawdata_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

#ifdef CONFIG_TOUCHSCREEN_DUAL_FOLDABLE
	input_raw_data_clear(MAIN_TOUCH);
#endif
	info->rawdata_read_lock = true;

	input_raw_info_d(true, &info->client->dev,
			"%s: start (noise:%d, wet:%d, tc:%d, folding:%s)##\n",
			__func__, info->touch_noise_status, info->wet_mode,
			info->touch_count, info->flip_status_current ? "close" : "open");

	run_rawcap_read(sec);
	run_self_raw_read(sec);
	fts_checking_miscal(info);
	run_cx_data_read(sec);
	run_ix_data_read(sec);
	fts_panel_ito_test(info, OPEN_SHORT_CRACK_TEST);
	fts_get_hf_data(info);
	run_mutual_jitter(sec);

	info->rawdata_read_lock = false;

	snprintf(buff, sizeof(buff), "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_raw_info_d(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

void fts_run_rawdata_read_all(struct fts_ts_info *info)
{
	struct sec_cmd_data *sec = &info->sec;

	run_rawdata_read_all(sec);
}

#ifdef TCLM_CONCEPT
static void get_pat_information(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[50] = { 0 };

	sec_cmd_set_default_result(sec);

#ifdef CONFIG_SEC_FACTORY
	if (info->factory_position == 1) {
		sec_tclm_initialize(info->tdata);
		fts_tclm_data_read(info->client, SEC_TCLM_NVM_ALL_DATA);
	}
#endif
	/* fixed tune version will be saved at excute autotune */
	snprintf(buff, sizeof(buff), "C%02XT%04X.%4s%s%c%d%c%d%c%d",
		info->tdata->nvdata.cal_count, info->tdata->nvdata.tune_fix_ver,
		info->tdata->tclm_string[info->tdata->nvdata.cal_position].f_name,
		(info->tdata->tclm_level == TCLM_LEVEL_LOCKDOWN) ? ".L " : " ",
		info->tdata->cal_pos_hist_last3[0], info->tdata->cal_pos_hist_last3[1],
		info->tdata->cal_pos_hist_last3[2], info->tdata->cal_pos_hist_last3[3],
		info->tdata->cal_pos_hist_last3[4], info->tdata->cal_pos_hist_last3[5]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void set_external_factory(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	info->tdata->external_factory = true;
	snprintf(buff, sizeof(buff), "OK");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}
#endif

static void fts_read_ix_data(struct fts_ts_info *info, bool allnode)
{
	struct sec_cmd_data *sec = &info->sec;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int rc;
	u16 max_tx_ix_sum = 0;
	u16 min_tx_ix_sum = 0xFFFF;
	u16 max_rx_ix_sum = 0;
	u16 min_rx_ix_sum = 0xFFFF;
	u8 *data;
	u8 regAdd[FTS_EVENT_SIZE];
	u8 dataID;
	u16 force_ix_data[100];
	u16 sense_ix_data[100];

	int buff_size, j;
	char *mbuff = NULL;
	int num, n, a, fzero;
	char cnum;
	int i = 0;
	u16 comp_start_tx_addr, comp_start_rx_addr;
	unsigned int rx_num, tx_num;

	data = kzalloc((info->ForceChannelLength + info->SenseChannelLength) * 2 + 1, GFP_KERNEL);
	if (!data)
		return;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	info->fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true); // Clear FIFO

	fts_release_all_finger(info);

	fts_interrupt_set(info, INT_DISABLE);

	// Request compensation data type
	dataID = 0x52;
	regAdd[0] = 0xA4;
	regAdd[1] = 0x06;
	regAdd[2] = dataID; // SS - CX total
	rc = fts_fw_wait_for_echo_event(info, &regAdd[0], 3, 0);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: failed to request data\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		fts_interrupt_set(info, INT_ENABLE);
		goto out;
	}
	fts_interrupt_set(info, INT_ENABLE);

	// Read Header
	regAdd[0] = 0xA6;
	regAdd[1] = 0x00;
	regAdd[2] = 0x00;
	rc = info->fts_read_reg(info, &regAdd[0], 3, &data[0], FTS_COMP_DATA_HEADER_SIZE);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: failed to read header\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if ((data[0] != 0xA5) && (data[1] != dataID)) {
		input_err(true, &info->client->dev, "%s: header mismatch\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	tx_num = data[4];
	rx_num = data[5];

	/* Read TX IX data */
	comp_start_tx_addr = (u16)FTS_COMP_DATA_HEADER_SIZE;
	regAdd[0] = 0xA6;
	regAdd[1] = (u8)(comp_start_tx_addr >> 8);
	regAdd[2] = (u8)(comp_start_tx_addr & 0xFF);
	rc = info->fts_read_reg(info, &regAdd[0], 3, &data[0], tx_num * 2);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: failed to read TX IX data\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	for (i = 0; i < tx_num; i++) {
		force_ix_data[i] = data[2 * i] | data[2 * i + 1] << 8;

		if (max_tx_ix_sum < force_ix_data[i])
			max_tx_ix_sum = force_ix_data[i];
		if (min_tx_ix_sum > force_ix_data[i])
			min_tx_ix_sum = force_ix_data[i];

	}

	/* Read RX IX data */
	comp_start_rx_addr = (u16)(FTS_COMP_DATA_HEADER_SIZE + (tx_num * 2));
	regAdd[0] = 0xA6;
	regAdd[1] = (u8)(comp_start_rx_addr >> 8);
	regAdd[2] = (u8)(comp_start_rx_addr & 0xFF);
	rc = info->fts_read_reg(info, &regAdd[0], 3, &data[0], rx_num * 2);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: failed to read RX IX\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	for (i = 0; i < rx_num; i++) {
		sense_ix_data[i] = data[2 * i] | data[2 * i + 1] << 8;

		if (max_rx_ix_sum < sense_ix_data[i])
			max_rx_ix_sum = sense_ix_data[i];
		if (min_rx_ix_sum > sense_ix_data[i])
			min_rx_ix_sum = sense_ix_data[i];
	}

	fts_print_channel(info, force_ix_data, sense_ix_data);

	input_raw_info_d(true, &info->client->dev, "%s: MIN_TX_IX_SUM : %d MAX_TX_IX_SUM : %d\n",
			__func__, min_tx_ix_sum, max_tx_ix_sum);
	input_raw_info_d(true, &info->client->dev, "%s: MIN_RX_IX_SUM : %d MAX_RX_IX_SUM : %d\n",
			__func__, min_rx_ix_sum, max_rx_ix_sum);

	if (allnode == true) {
		buff_size = (info->ForceChannelLength + info->SenseChannelLength + 2) * 5;
		mbuff = kzalloc(buff_size, GFP_KERNEL);
	}
	if (mbuff != NULL) {
		char *pBuf = mbuff;

		for (i = 0; i < info->ForceChannelLength; i++) {
			num =  force_ix_data[i];
			n = 100000;
			fzero = 0;
			for (j = 5; j > 0; j--) {
				n = n / 10;
				a = num / n;
				if (a)
					fzero = 1;
				cnum = a + '0';
				num  = num - a*n;
				if (fzero)
					*pBuf++ = cnum;
			}
			if (!fzero)
				*pBuf++ = '0';
			*pBuf++ = ',';
		}
		for (i = 0; i < info->SenseChannelLength; i++) {
			num =  sense_ix_data[i];
			n = 100000;
			fzero = 0;
			for (j = 5; j > 0; j--) {
				n = n / 10;
				a = num / n;
				if (a)
					fzero = 1;
				cnum = a + '0';
				num  = num - a * n;
				if (fzero)
					*pBuf++ = cnum;
			}
			if (!fzero)
				*pBuf++ = '0';
			if (i < (info->SenseChannelLength - 1))
				*pBuf++ = ',';
		}

		sec_cmd_set_cmd_result(sec, mbuff, buff_size);
		sec->cmd_state = SEC_CMD_STATUS_OK;
		kfree(mbuff);
		return;
	}

	if (allnode == true) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%d,%d,%d,%d",
				min_tx_ix_sum, max_tx_ix_sum, min_rx_ix_sum, max_rx_ix_sum);
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

out:
	kfree(data);

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		char ret_buff[SEC_CMD_STR_LEN] = { 0 };

		snprintf(ret_buff, sizeof(ret_buff), "%d,%d", min_rx_ix_sum, max_rx_ix_sum);
		sec_cmd_set_cmd_result_all(sec, ret_buff, strnlen(ret_buff, sizeof(ret_buff)), "IX_DATA_X");
		snprintf(ret_buff, sizeof(ret_buff), "%d,%d", min_tx_ix_sum, max_tx_ix_sum);
		sec_cmd_set_cmd_result_all(sec, ret_buff, strnlen(ret_buff, sizeof(ret_buff)), "IX_DATA_Y");
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_ix_data_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	sec_cmd_set_default_result(sec);

	input_raw_info_d(true, &info->client->dev, "%s\n", __func__);
	fts_read_ix_data(info, false);
}

static void run_ix_data_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	sec_cmd_set_default_result(sec);

	enter_factory_mode(info, true);
	fts_read_ix_data(info, true);
	enter_factory_mode(info, false);
}

static void fts_read_self_raw_frame(struct fts_ts_info *info, bool allnode)
{
	struct sec_cmd_data *sec = &info->sec;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	struct FTS_SyncFrameHeader *pSyncFrameHeader;
	u8 regAdd[FTS_EVENT_SIZE] = {0};
	u8 *data;
	s16 self_force_raw_data[100];
	s16 self_sense_raw_data[100];
	int Offset = 0;
	u8 count = 0;
	int i;
	int ret;
	int totalbytes;
	int retry = 10;
	s16 min_tx_self_raw_data = S16_MAX;
	s16 max_tx_self_raw_data = S16_MIN;
	s16 min_rx_self_raw_data = S16_MAX;
	s16 max_rx_self_raw_data = S16_MIN;

	data = kzalloc((info->ForceChannelLength + info->SenseChannelLength) * 2 + 1, GFP_KERNEL);
	if (!data)
		return;

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	// Request Data Type
	regAdd[0] = 0xA4;
	regAdd[1] = 0x06;
	regAdd[2] = TYPE_RAW_DATA;
	info->fts_write_reg(info, &regAdd[0], 3);

	do {
		regAdd[0] = 0xA6;
		regAdd[1] = 0x00;
		regAdd[2] = 0x00;
		ret = info->fts_read_reg(info, &regAdd[0], 3, &data[0], FTS_COMP_DATA_HEADER_SIZE);
		if (ret <= 0) {
			input_err(true, &info->client->dev, "%s: read failed rc = %d\n", __func__, ret);
			snprintf(buff, sizeof(buff), "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
			goto out;
		}

		pSyncFrameHeader = (struct FTS_SyncFrameHeader *) &data[0];

		if ((pSyncFrameHeader->header == 0xA5) && (pSyncFrameHeader->host_data_mem_id == TYPE_RAW_DATA))
			break;

		fts_delay(100);
	} while (retry--);

	if (retry == 0) {
		input_err(true, &info->client->dev, "%s: didn't match header or id. header = %02X, id = %02X\n",
				__func__, pSyncFrameHeader->header, pSyncFrameHeader->host_data_mem_id);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	Offset = FTS_COMP_DATA_HEADER_SIZE + pSyncFrameHeader->dbg_frm_len +
			(pSyncFrameHeader->ms_force_len * pSyncFrameHeader->ms_sense_len * 2);

	totalbytes = (pSyncFrameHeader->ss_force_len + pSyncFrameHeader->ss_sense_len) * 2;

	regAdd[0] = 0xA6;
	regAdd[1] = (u8)(Offset >> 8);
	regAdd[2] = (u8)(Offset & 0xFF);
	ret = info->fts_read_reg(info, &regAdd[0], 3, &data[0], totalbytes);
	if (ret <= 0) {
		input_err(true, &info->client->dev, "%s: read failed rc = %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	Offset = 0;
	for (count = 0; count < (info->ForceChannelLength); count++) {
		self_force_raw_data[count] = (s16)(data[count * 2 + Offset] + (data[count * 2 + 1 + Offset] << 8));

		if (max_tx_self_raw_data < self_force_raw_data[count])
			max_tx_self_raw_data = self_force_raw_data[count];
		if (min_tx_self_raw_data > self_force_raw_data[count])
			min_tx_self_raw_data = self_force_raw_data[count];
	}

	Offset = (info->ForceChannelLength * 2);
	for (count = 0; count < info->SenseChannelLength; count++) {
		self_sense_raw_data[count] = (s16)(data[count * 2 + Offset] + (data[count * 2 + 1 + Offset] << 8));

		if (max_rx_self_raw_data < self_sense_raw_data[count])
			max_rx_self_raw_data = self_sense_raw_data[count];
		if (min_rx_self_raw_data > self_sense_raw_data[count])
			min_rx_self_raw_data = self_sense_raw_data[count];
	}

	fts_print_channel(info, self_force_raw_data, self_sense_raw_data);

	input_raw_info_d(true, &info->client->dev, "%s: MIN_TX_SELF_RAW: %d MAX_TX_SELF_RAW : %d\n",
			__func__, (s16)min_tx_self_raw_data, (s16)max_tx_self_raw_data);
	input_raw_info_d(true, &info->client->dev, "%s: MIN_RX_SELF_RAW : %d MIN_RX_SELF_RAW : %d\n",
			__func__, (s16)min_rx_self_raw_data, (s16)max_rx_self_raw_data);

	if (allnode == true) {
		char *mbuff;
		char temp[10] = { 0 };

		mbuff = kzalloc((info->ForceChannelLength + info->SenseChannelLength + 2) * 10, GFP_KERNEL);
		if (!mbuff)
			goto out;
		for (i = 0; i < (info->ForceChannelLength); i++) {
			snprintf(temp, sizeof(temp), "%d,", (s16)self_force_raw_data[i]);
			strlcat(mbuff, temp, sizeof(mbuff));
		}
		for (i = 0; i < (info->SenseChannelLength); i++) {
			snprintf(temp, sizeof(temp), "%d,", (s16)self_sense_raw_data[i]);
			strlcat(mbuff, temp, sizeof(mbuff));
		}

		sec_cmd_set_cmd_result(sec, mbuff, sizeof(mbuff));
		sec->cmd_state = SEC_CMD_STATUS_OK;
		kfree(mbuff);
		return;
	}

	snprintf(buff, sizeof(buff), "%d,%d,%d,%d",
			(s16)min_tx_self_raw_data, (s16)max_tx_self_raw_data,
			(s16)min_rx_self_raw_data, (s16)max_rx_self_raw_data);
	sec->cmd_state = SEC_CMD_STATUS_OK;
out:
	kfree(data);
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		char ret_buff[SEC_CMD_STR_LEN] = { 0 };

		snprintf(ret_buff, sizeof(ret_buff), "%d,%d", (s16)min_rx_self_raw_data, (s16)max_rx_self_raw_data);
		sec_cmd_set_cmd_result_all(sec, ret_buff, strnlen(ret_buff, sizeof(ret_buff)), "SELF_RAW_DATA_X");
		snprintf(ret_buff, sizeof(ret_buff), "%d,%d", (s16)min_tx_self_raw_data, (s16)max_tx_self_raw_data);
		sec_cmd_set_cmd_result_all(sec, ret_buff, strnlen(ret_buff, sizeof(ret_buff)), "SELF_RAW_DATA_Y");
	}
	sec_cmd_set_cmd_result(sec, &buff[0], strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_self_raw_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	sec_cmd_set_default_result(sec);

	input_raw_info_d(true, &info->client->dev, "%s\n", __func__);
	fts_read_self_raw_frame(info, false);
}

static void run_self_raw_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	sec_cmd_set_default_result(sec);

	enter_factory_mode(info, true);
	fts_read_self_raw_frame(info, true);
	enter_factory_mode(info, false);
}

static void run_factory_miscalibration(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN];
	char data[FTS_EVENT_SIZE];
	char echo;
	int ret;
	int retry = 200;
	short min, max;

	sec_cmd_set_default_result(sec);

	fts_interrupt_set(info, INT_DISABLE);

	memset(data, 0x00, sizeof(data));
	memset(buff, 0x00, sizeof(buff));

	data[0] = 0xC7;
	data[1] = 0x02;

	ret = info->fts_write_reg(info, data, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: write failed: %d\n", __func__, ret);
		goto error;
	}

	/* maximum timeout 2sec ? */
	while (retry-- >= 0) {
		memset(data, 0x00, sizeof(data));
		echo = FTS_READ_ONE_EVENT;
		ret = info->fts_read_reg(info, &echo, 1, data, FTS_EVENT_SIZE);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: read failed: %d\n", __func__, ret);
			goto error;
		}

		input_info(true, &info->client->dev, "%s: %02X %02X %02X %02X %02X %02X %02X %02X\n",
				__func__, data[0], data[1], data[2], data[3],
				data[4], data[5], data[6], data[7]);
		fts_delay(10);

		if (data[0] == 0x03 || data[0] == 0xF3) {
			max = data[3] << 8 | data[2];
			min = data[5] << 8 | data[4];

			break;
		}
		if (retry == 0)
			goto error;
	}

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		snprintf(buff, sizeof(buff), "%d,%d", min, max);
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "MIS_CAL");
	} else {
		if (data[0] == 0x03) {
			snprintf(buff, sizeof(buff), "OK,%d,%d", min, max);
		} else {
			snprintf(buff, sizeof(buff), "NG,%d,%d", min, max);
		}
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	fts_reinit(info, false);
	fts_interrupt_set(info, INT_ENABLE);
	return;

error:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	fts_reinit(info, false);

	fts_interrupt_set(info, INT_ENABLE);
}

static void run_miscalibration(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN];
	char data[FTS_EVENT_SIZE];
	char echo;
	int ret;
	int retry = 200;
	short min, max;

	sec_cmd_set_default_result(sec);

	fts_interrupt_set(info, INT_DISABLE);

	memset(data, 0x00, sizeof(data));
	memset(buff, 0x00, sizeof(buff));

	data[0] = 0xA4;
	data[1] = 0x0B;
	data[2] = 0x00;
	data[3] = 0xC0;

	ret = info->fts_write_reg(info, data, 4);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: write failed: %d\n", __func__, ret);
		goto error;
	}

	/* maximum timeout 2sec ? */
	while (retry-- >= 0) {
		memset(data, 0x00, sizeof(data));
		echo = FTS_READ_ONE_EVENT;
		ret = info->fts_read_reg(info, &echo, 1, data, FTS_EVENT_SIZE);
		if (ret < 0) {
			input_err(true, &info->client->dev, "%s: read failed: %d\n", __func__, ret);
			goto error;
		}

		input_info(true, &info->client->dev, "%s: %02X %02X %02X %02X %02X %02X %02X %02X\n",
				__func__, data[0], data[1], data[2], data[3],
				data[4], data[5], data[6], data[7]);
		fts_delay(10);

		if (data[0] == 0x03 || data[0] == 0xF3) {
			max = data[3] << 8 | data[2];
			min = data[5] << 8 | data[4];

			break;
		}
		if (retry == 0)
			goto error;
	}

	if (data[0] == 0x03) {
		snprintf(buff, sizeof(buff), "OK,%d,%d", min, max);
	} else {
		snprintf(buff, sizeof(buff), "NG,%d,%d", min, max);
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	fts_reinit(info, false);
	fts_interrupt_set(info, INT_ENABLE);
	return;
error:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	fts_reinit(info, false);

	fts_interrupt_set(info, INT_ENABLE);
}

int fts_panel_test_result(struct fts_ts_info *info, int type)
{
	u8 data[FTS_EVENT_SIZE];
	u8 cmd = FTS_READ_ONE_EVENT;
	uint8_t regAdd[4] = { 0 };
	bool matched = false;
	int retry = 0;
	int i = 0;
	int result = 0;
	short temp_min = 32767, temp_max = -32768;

	bool cheked_short_to_gnd = false;
	bool cheked_short_to_vdd = false;
	bool cheked_short = false;
	bool cheked_open = false;
	char tempv[25] = {0};
	char temph[30] = {0};
	struct sec_cmd_data *sec = &info->sec;

	char buff[SEC_CMD_STR_LEN] = {0};
	u8 *result_buff = NULL;
	int size = 4095;

	result_buff = kzalloc(size, GFP_KERNEL);
	if (!result_buff) {
		input_err(true, &info->client->dev, "%s: result_buff kzalloc fail!\n", __func__);
		goto alloc_fail;
	}

	regAdd[0] = 0xA4;
	regAdd[1] = 0x04;
	regAdd[2] = 0xFC;
	regAdd[3] = 0x09;

	info->fts_write_reg(info, &regAdd[0], 4);
	fts_delay(100);

	memset(info->ito_test, 0x0, 4);

	memset(data, 0x0, FTS_EVENT_SIZE);
	while (info->fts_read_reg(info, &cmd, 1, (u8 *)data, FTS_EVENT_SIZE)) {
		memset(tempv, 0x00, 25);
		memset(temph, 0x00, 30);
		if ((data[0] == FTS_EVENT_STATUS_REPORT) && (data[1] == 0x01)) {
			for (i = 0; i < 4; i++) {
				if (data[i + 2] != regAdd[i]) {
					matched = false;
					break;
				}
				matched = true;
			}
			if (matched == true)
				break;

		} else if (data[0] == FTS_EVENT_ERROR_REPORT) {
			info->ito_test[0] = data[0];
			info->ito_test[1] = data[1];
			info->ito_test[2] = data[2];
			info->ito_test[3] = 0x00;

			switch (data[1]) {
			case ITO_FORCE_SHRT_GND:
			case ITO_SENSE_SHRT_GND:
				input_info(true, &info->client->dev, "%s: TX/RX_SHORT_TO_GND:%cX[%d]\n",
						__func__, data[1] == ITO_FORCE_SHRT_VDD ? 'T' : 'R', data[2]);
				if (type != SHORT_TEST)
					break;
				if (!cheked_short_to_gnd) {
					snprintf(temph, 30, "TX/RX_SHORT_TO_GND:");
					strlcat(result_buff, temph, size);
					cheked_short_to_gnd = true;
				}
				snprintf(tempv, sizeof(tempv), "%c%d,", data[1] == ITO_FORCE_SHRT_GND ? 'T' : 'R', data[2]);
				strlcat(result_buff, tempv, size);
				result = ITO_FAIL_SHORT;
				break;
			case ITO_FORCE_SHRT_VDD:
			case ITO_SENSE_SHRT_VDD:
				input_info(true, &info->client->dev, "%s: TX/RX_SHORT_TO_VDD:%cX[%d]\n",
						__func__, data[1] == ITO_FORCE_SHRT_VDD ? 'T' : 'R', data[2]);
				if (type != SHORT_TEST)
					break;
				if (!cheked_short_to_vdd) {
					snprintf(temph, 30, "TX/RX_SHORT_TO_VDD:");
					strlcat(result_buff, temph, size);
					cheked_short_to_vdd = true;
				}
				snprintf(tempv, sizeof(tempv), "%c%d,", data[1] == ITO_FORCE_SHRT_VDD ? 'T' : 'R', data[2]);
				strlcat(result_buff, tempv, size);
				result = -ITO_FAIL_SHORT;
				break;
			case ITO_FORCE_SHRT_FORCE:
			case ITO_SENSE_SHRT_SENSE:
				input_info(true, &info->client->dev, "%s: TX/RX_SHORT:%cX[%d]\n",
						__func__, data[1] == ITO_FORCE_SHRT_FORCE ? 'T' : 'R', data[2]);
				if (type != SHORT_TEST)
					break;
				if (!cheked_short) {
					snprintf(temph, 30, "TX/RX_SHORT:");
					strlcat(result_buff, temph, size);
					cheked_short = true;
				}
				snprintf(tempv, sizeof(tempv), "%c%d,", data[1] == ITO_FORCE_SHRT_FORCE ? 'T' : 'R', data[2]);
				strlcat(result_buff, tempv, size);
				result = -ITO_FAIL_SHORT;
				break;
			case ITO_FORCE_OPEN:
			case ITO_SENSE_OPEN:
				input_info(true, &info->client->dev, "%s: TX/RX_OPEN:%cX[%d]\n",
						__func__, data[1] == ITO_FORCE_OPEN ? 'T' : 'R', data[2]);
				if (type != OPEN_TEST)
					break;
				if (!cheked_open) {
					snprintf(temph, 30, "TX/RX_OPEN:");
					strlcat(result_buff, temph, size);
					cheked_open = true;
				}
				snprintf(tempv, sizeof(tempv), "%c%d,", data[1] == ITO_FORCE_OPEN ? 'T' : 'R', data[2]);
				strlcat(result_buff, tempv, size);
				result = -ITO_FAIL_OPEN;
				break;
			default:
				input_info(true, &info->client->dev, "%s: unknown event %02x %02x %02x %02x %02x %02x %02x %02x\n",
						__func__, data[0], data[1], data[2], data[3],
						data[4], data[5], data[6], data[7]);
				break;
			}
		}

		if (retry++ > 50) {
			input_err(true, &info->client->dev, "%s: Time over - wait for result of ITO test\n", __func__);
			goto test_fail;
		}
		fts_delay(20);
	}

	/* read rawdata */
//	fts_read_frame(info, TYPE_RAW_DATA, &temp_min, &temp_max);
	input_raw_info_d(true, &info->client->dev, "%s\n", __func__);
	fts_read_nonsync_frame(info, &temp_min, &temp_max);

	if (type == OPEN_TEST && result == -ITO_FAIL_OPEN) {
		snprintf(result_buff, sizeof(result_buff), "NG");
		sec_cmd_set_cmd_result(sec, result_buff, strnlen(result_buff, sizeof(result_buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;

	} else if (type == SHORT_TEST && result == -ITO_FAIL_SHORT) {
		snprintf(result_buff, sizeof(result_buff), "NG");
		sec_cmd_set_cmd_result(sec, result_buff, strnlen(result_buff, sizeof(result_buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;

	} else {
		snprintf(result_buff, sizeof(result_buff), "OK");
		sec_cmd_set_cmd_result(sec, result_buff, strnlen(result_buff, sizeof(result_buff)));
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	input_info(true, &info->client->dev, "%s: %s\n", __func__, result_buff);
	kfree(result_buff);

	return result;

test_fail:
	kfree(result_buff);
alloc_fail:
	snprintf(buff, 30, "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	return -ITO_FAIL;
}

static void run_trx_short_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int result = 0;
	int type = 0;
	char test[32];

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	if (info->fts_power_state == FTS_POWER_STATE_LOWPOWER) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is lp mode\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	if (sec->cmd_param[0] == 1 && sec->cmd_param[1] == 0) {
		input_err(true, &info->client->dev,
			"%s: %s: seperate cm1 test open / short test result\n", __func__, buff);

		snprintf(buff, sizeof(buff), "%s", "CONT");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_OK;
		return;
	}

	if (sec->cmd_param[0] == 1 && sec->cmd_param[1] == 1) {
		type = OPEN_TEST;
	} else if (sec->cmd_param[0] == 1 && sec->cmd_param[1] == 2) {
		type = SHORT_TEST;
	} else if (sec->cmd_param[0] > 1) {
		u8 result_buff[10];

		snprintf(result_buff, sizeof(result_buff), "NA");
		sec_cmd_set_cmd_result(sec, result_buff, strnlen(result_buff, sizeof(result_buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;

		input_info(true, &info->client->dev, "%s : not supported test case\n", __func__);
		return;
	}

	input_info(true, &info->client->dev, "%s : CM%d factory_position[%d]\n",
					__func__, sec->cmd_param[0], info->factory_position);
	fts_interrupt_set(info, INT_DISABLE);

	result = fts_panel_test_result(info, type);

	fts_interrupt_set(info, INT_ENABLE);

	/* reinit */
	info->fts_systemreset(info, 0);
	fts_reinit(info, false);

	info->touch_count = 0;

	if (sec->cmd_param[1])
		snprintf(test, sizeof(test), "TEST=%d,%d", sec->cmd_param[0], sec->cmd_param[1]);
	else
		snprintf(test, sizeof(test), "TEST=%d", sec->cmd_param[0]);

	if (result == 0)
		sec_cmd_send_event_to_user(sec, test, "RESULT=PASS");
	else
		sec_cmd_send_event_to_user(sec, test, "RESULT=FAIL");

	input_info(true, &info->client->dev, "%s : test done\n", __func__);
	return;
}

static void check_connection(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret = 0;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	ret = fts_panel_ito_test(info, OPEN_TEST);
	if (ret == 0)
		snprintf(buff, sizeof(buff), "OK");
	else
		snprintf(buff, sizeof(buff), "NG");

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_cx_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	node = fts_check_index(info);
	if (node < 0)
		return;

	if (info->cx_data)
		val = info->cx_data[node];
	snprintf(buff, sizeof(buff), "%d", val);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);

}

static int read_ms_cx_data(struct fts_ts_info *info, u8 *cx_min, u8 *cx_max)
{
	u8 *rdata;
	u8 regAdd[FTS_EVENT_SIZE] = { 0 };
	u8 dataID;
	u16 comp_start_addr;
	int txnum, rxnum, i, j, ret = 0;
	u8 *pStr = NULL;
	u8 pTmp[16] = { 0 };

	pStr = kzalloc(7 * (info->SenseChannelLength + 1), GFP_KERNEL);
	if (pStr == NULL)
		return -ENOMEM;

	rdata = kzalloc(info->ForceChannelLength * info->SenseChannelLength, GFP_KERNEL);
	if (!rdata)
		return -ENOMEM;

	info->fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true); // Clear FIFO
	fts_release_all_finger(info);

	fts_interrupt_set(info, INT_DISABLE);

	fts_delay(20);

	// Request compensation data type
	dataID = 0x11;  // MS - LP
	regAdd[0] = 0xA4;
	regAdd[1] = 0x06;
	regAdd[2] = dataID;
	ret = fts_fw_wait_for_echo_event(info, &regAdd[0], 3, 0);
	if (ret < 0) {
		input_info(true, &info->client->dev, "%s: failed to request data\n", __func__);
		fts_interrupt_set(info, INT_ENABLE);
		goto out;
	}

	// Read Header
	regAdd[0] = 0xA6;
	regAdd[1] = 0x00;
	regAdd[2] = 0x00;
	ret = info->fts_read_reg(info, &regAdd[0], 3, &rdata[0], FTS_COMP_DATA_HEADER_SIZE);
	if (ret < 0) {
		input_info(true, &info->client->dev, "%s: failed to read header\n", __func__);
		fts_interrupt_set(info, INT_ENABLE);
		goto out;
	}

	fts_interrupt_set(info, INT_ENABLE);

	if ((rdata[0] != 0xA5) && (rdata[1] != dataID)) {
		input_info(true, &info->client->dev, "%s: failed to read signature data of header.\n", __func__);
		ret = -EIO;
		goto out;
	}

	txnum = rdata[4];
	rxnum = rdata[5];

	comp_start_addr = (u16)FTS_COMP_DATA_HEADER_SIZE;
	regAdd[0] = 0xA6;
	regAdd[1] = (u8)(comp_start_addr >> 8);
	regAdd[2] = (u8)(comp_start_addr & 0xFF);
	ret = info->fts_read_reg(info, &regAdd[0], 3, &rdata[0], txnum * rxnum);
	if (ret < 0) {
		input_info(true, &info->client->dev, "%s: failed to read data\n", __func__);
		goto out;
	}

	*cx_min = *cx_max = rdata[0];
	for (j = 0; j < info->ForceChannelLength; j++) {
		memset(pStr, 0x0, 7 * (info->SenseChannelLength + 1));
		snprintf(pTmp, sizeof(pTmp), "Tx%02d | ", j);
		strlcat(pStr, pTmp, 7 * (info->SenseChannelLength + 1));

		for (i = 0; i < info->SenseChannelLength; i++) {
			snprintf(pTmp, sizeof(pTmp), "%3d", rdata[j * info->SenseChannelLength + i]);
			strlcat(pStr, pTmp, 7 * (info->SenseChannelLength + 1));
			*cx_min = min(*cx_min, rdata[j * info->SenseChannelLength + i]);
			*cx_max = max(*cx_max, rdata[j * info->SenseChannelLength + i]);
		}
		input_raw_info(true, &info->client->dev, "%s\n", pStr);
	}
	input_raw_info(true, &info->client->dev, "cx min:%d, cx max:%d\n", *cx_min, *cx_max);

	if (info->cx_data)
		memcpy(&info->cx_data[0], &rdata[0], info->ForceChannelLength * info->SenseChannelLength);

out:
	kfree(rdata);
	kfree(pStr);
	return ret;
}

#define NORMAL_CX2	0	/* single driving */
#define ACTIVE_CX2	1	/* multi driving */

static int read_ms_cx2_data(struct fts_ts_info *info, u8 active, s8 *cx_min, s8 *cx_max)
{
	s8 *rdata = NULL;
	u8 cdata[FTS_COMP_DATA_HEADER_SIZE];
	u8 regAdd[FTS_EVENT_SIZE] = { 0 };
	u8 dataID = 0;
	u16 comp_start_addr;
	int txnum, rxnum, i, j, ret = 0;
	u8 *pStr = NULL;
	u8 pTmp[16] = { 0 };

	info->fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true); // Clear FIFO
	fts_release_all_finger(info);

	fts_interrupt_set(info, INT_DISABLE);

	fts_delay(20);

	// Request compensation data type
	if (active == NORMAL_CX2)
		dataID = 0x11;  // MS - LP
	else if (active == ACTIVE_CX2)
		dataID = 0x10;	// MS - ACTIVE

	regAdd[0] = 0xA4;
	regAdd[1] = 0x06;
	regAdd[2] = dataID;

	ret = fts_fw_wait_for_echo_event(info, &regAdd[0], 3, 0);
	if (ret < 0) {
		fts_interrupt_set(info, INT_ENABLE);
		return ret;
	}

	// Read Header
	regAdd[0] = 0xA6;
	regAdd[1] = 0x00;
	regAdd[2] = 0x00;
	ret = info->fts_read_reg(info, &regAdd[0], 3, &cdata[0], FTS_COMP_DATA_HEADER_SIZE);
	if (ret < 0) {
		fts_interrupt_set(info, INT_ENABLE);
		return ret;
	}
	fts_interrupt_set(info, INT_ENABLE);

	if ((cdata[0] != 0xA5) && (cdata[1] != dataID)) {
		input_info(true, &info->client->dev, "%s: failed to read signature data of header.\n", __func__);
		ret = -EIO;
		return ret;
	}

	txnum = cdata[4];
	rxnum = cdata[5];

	rdata = kzalloc(txnum * rxnum, GFP_KERNEL);
	if (!rdata)
		return -ENOMEM;

	comp_start_addr = (u16)FTS_COMP_DATA_HEADER_SIZE;
	regAdd[0] = 0xA6;
	regAdd[1] = (u8)(comp_start_addr >> 8);
	regAdd[2] = (u8)(comp_start_addr & 0xFF);
	ret = info->fts_read_reg(info, &regAdd[0], 3, &rdata[0], txnum * rxnum);
	if (ret < 0)
		goto out;

	pStr = kzalloc(7 * (rxnum + 1), GFP_KERNEL);
	if (!pStr) {
		ret = -ENOMEM;
		goto out;
	}

	*cx_min = *cx_max = rdata[0];
	for (i = 0; i < txnum; i++) {
		memset(pStr, 0x0, 7 * (rxnum + 1));
		snprintf(pTmp, sizeof(pTmp), "Tx%02d | ", i);
		strlcat(pStr, pTmp, 7 * (rxnum + 1));

		for (j = 0; j < rxnum; j++) {
			snprintf(pTmp, sizeof(pTmp), "%4d", rdata[i * rxnum + j]);
			strlcat(pStr, pTmp, 7 * (rxnum + 1));

			*cx_min = min(*cx_min, rdata[i * rxnum + j]);
			*cx_max = max(*cx_max, rdata[i * rxnum + j]);
		}
		input_raw_info_d(true, &info->client->dev, "%s\n", pStr);
	}
	input_raw_info_d(true, &info->client->dev, "cx min:%d, cx max:%d\n", *cx_min, *cx_max);

	/* info->cx_data length: Force * Sense */
	if (info->cx_data && active == NORMAL_CX2)
		memcpy(&info->cx_data[0], &rdata[0], info->ForceChannelLength * info->SenseChannelLength);

	kfree(pStr);
out:
	kfree(rdata);
	return ret;
}

static void run_active_cx_data_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char buff_minmax[SEC_CMD_STR_LEN] = { 0 };
	int rc;
	s8 cx_min, cx_max;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
			sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "ACTIVE_CX2_DATA");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_raw_info_d(true, &info->client->dev, "%s: start\n", __func__);

	rc = read_ms_cx2_data(info, ACTIVE_CX2, &cx_min, &cx_max);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "NG");
		snprintf(buff_minmax, sizeof(buff_minmax), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff_minmax, sizeof(buff_minmax), "%d,%d", cx_min, cx_max);
		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff_minmax, strnlen(buff_minmax, sizeof(buff_minmax)), "ACTIVE_CX2_DATA");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_multi_mutual_cx_data_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[FTS_EVENT_SIZE] = { 0 };
	u8 *data;
	u8 dataID;
	u16 sense_cx_data[100];
	u16 comp_start_rx_addr;
	unsigned int rx_num, tx_num;
	int rc, i, ret;

	u16 max_sense_cx = 0;
	u16 min_sense_cx = 0xFFFF;

	input_info(true, &info->client->dev, "%s\n", __func__);

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		goto out;
	}

	info->fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true);

	fts_release_all_finger(info);

	fts_interrupt_set(info, INT_DISABLE);

	// Request compensation data type
	dataID = 0x56;
	regAdd[0] = 0xA4;
	regAdd[1] = 0x06;
	regAdd[2] = dataID; // Multi Mutual CX data

	rc = fts_fw_wait_for_echo_event(info, &regAdd[0], 3, 0);

	fts_interrupt_set(info, INT_ENABLE);

	if (rc < 0)
		goto out;

	data = kzalloc((info->ForceChannelLength + info->SenseChannelLength) * 2 + 1, GFP_KERNEL);
	if (!data) {
		input_err(true, &info->client->dev, "%s: data kzalloc fail!\n", __func__);
		goto out;
	}

	// Read Header
	regAdd[0] = 0xA6;
	regAdd[1] = 0x00;
	regAdd[2] = 0x00;
	info->fts_read_reg(info, &regAdd[0], 3, &data[0], FTS_COMP_DATA_HEADER_SIZE);

	if ((data[0] != 0xA5) && (data[1] != dataID)) {
		kfree(data);
		goto out;
	}

	tx_num = data[4];
	rx_num = data[5];

	/* Read Multi Mutual CX - RX data */
	comp_start_rx_addr = FTS_COMP_DATA_HEADER_SIZE + (((tx_num * 2) + rx_num) * 2);
	regAdd[0] = 0xA6;
	regAdd[1] = (u8)(comp_start_rx_addr >> 8);
	regAdd[2] = (u8)(comp_start_rx_addr & 0xFF);
	ret = info->fts_read_reg(info, &regAdd[0], 3, &data[0], rx_num * 2);
	if (ret < 0) {
		kfree(data);
		goto out;
	}

	for (i = 0; i < rx_num; i++) {
		sense_cx_data[i] = data[2 * i] | data[2 * i + 1] << 8;

		if (max_sense_cx < sense_cx_data[i])
			max_sense_cx = sense_cx_data[i];
		if (min_sense_cx > sense_cx_data[i])
			min_sense_cx = sense_cx_data[i];
	}

	kfree(data);

	snprintf(buff, sizeof(buff), "%d,%d", min_sense_cx, max_sense_cx);
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "MULTI_MUTUAL_CX");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);

	return;

out:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "MULTI_MUTUAL_CX");

	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_cx_data_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char buff_minmax[SEC_CMD_STR_LEN] = { 0 };
	int rc;
	u8 cx_min, cx_max;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
			sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CX_DATA");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_raw_info_d(true, &info->client->dev, "%s: start\n", __func__);

	rc = read_ms_cx_data(info, &cx_min, &cx_max);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "NG");
		snprintf(buff_minmax, sizeof(buff_minmax), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff_minmax, sizeof(buff_minmax), "%d,%d", cx_min, cx_max);
		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff_minmax, strnlen(buff_minmax, sizeof(buff_minmax)), "CX_DATA");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_cx_all_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int rc, i, j;
	u8 cx_min, cx_max;
	char *all_strbuff;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_raw_info_d(true, &info->client->dev, "%s\n", __func__);

	input_info(true, &info->client->dev, "%s: start\n", __func__);

	rc = read_ms_cx_data(info, &cx_min, &cx_max);

	/* do not systemreset in COB type */
	if (info->board->chip_on_board)
		fts_set_scanmode(info, info->scan_mode);
	else
		enter_factory_mode(info, false);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	all_strbuff = kzalloc(info->ForceChannelLength * info->SenseChannelLength * 4 + 1, GFP_KERNEL);
	if (!all_strbuff) {
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	/* Read compensation data */
	if (info->cx_data) {
		for (j = 0; j < info->ForceChannelLength; j++) {
			for (i = 0; i < info->SenseChannelLength; i++) {
				snprintf(buff, sizeof(buff), "%d,", info->cx_data[j * info->SenseChannelLength + i]);
				strlcat(all_strbuff, buff, info->ForceChannelLength * info->SenseChannelLength * 4 + 1);
			}
		}
	}

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, all_strbuff, strlen(all_strbuff));
	input_info(true, &info->client->dev, "%s: %ld\n", __func__, strlen(all_strbuff));
	kfree(all_strbuff);
}

static void get_cx_gap_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int rx_max = 0, tx_max = 0, ii;

	for (ii = 0; ii < (info->SenseChannelLength * info->ForceChannelLength); ii++) {
		/* rx(x) gap max */
		if ((ii + 1) % (info->SenseChannelLength) != 0)
			rx_max = max(rx_max, (int)abs(info->cx_data[ii + 1] - info->cx_data[ii]));

		/* tx(y) gap max */
		if (ii < (info->ForceChannelLength - 1) * info->SenseChannelLength)
			tx_max = max(tx_max, (int)abs(info->cx_data[ii + info->SenseChannelLength] - info->cx_data[ii]));
	}

	input_raw_info_d(true, &info->client->dev, "%s: max: rx:%d, tx:%d\n", __func__, rx_max, tx_max);

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		snprintf(buff, sizeof(buff), "0,%d", rx_max);
		sec_cmd_set_cmd_result_all(sec, buff, SEC_CMD_STR_LEN, "CX_DATA_GAP_X");
		snprintf(buff, sizeof(buff), "0,%d", tx_max);
		sec_cmd_set_cmd_result_all(sec, buff, SEC_CMD_STR_LEN, "CX_DATA_GAP_Y");
	}
}

static void run_cx_gap_data_x_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char *buff = NULL;
	int ii, jj;
	char temp[5] = { 0 };

	sec_cmd_set_default_result(sec);

	buff = kzalloc(info->ForceChannelLength * info->SenseChannelLength * 5, GFP_KERNEL);
	if (!buff) {
		snprintf(temp, sizeof(temp), "NG");
		sec_cmd_set_cmd_result(sec, temp, strnlen(temp, sizeof(temp)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	for (ii = 0; ii < info->ForceChannelLength; ii++) {
		for (jj = 0; jj < info->SenseChannelLength - 1; jj++) {
			snprintf(temp, sizeof(temp), "%d,",
					abs(info->cx_data[(ii * info->SenseChannelLength) + jj + 1] -
						info->cx_data[(ii * info->SenseChannelLength) + jj]));
			strlcat(buff, temp, info->ForceChannelLength * info->SenseChannelLength * 5);
			memset(temp, 0x00, 5);
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, info->ForceChannelLength * info->SenseChannelLength * 5));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(buff);
}

static void run_cx_gap_data_y_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char *buff = NULL;
	int ii, jj;
	char temp[5] = { 0 };

	sec_cmd_set_default_result(sec);

	buff = kzalloc(info->ForceChannelLength * info->SenseChannelLength * 5, GFP_KERNEL);
	if (!buff) {
		snprintf(temp, sizeof(temp), "NG");
		sec_cmd_set_cmd_result(sec, temp, strnlen(temp, sizeof(temp)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	for (ii = 0; ii < info->ForceChannelLength - 1; ii++) {
		for (jj = 0; jj < info->SenseChannelLength; jj++) {
			snprintf(temp, sizeof(temp), "%d,",
					abs(info->cx_data[((ii + 1) * info->SenseChannelLength) + jj] -
						info->cx_data[(ii * info->SenseChannelLength) + jj]));
			strlcat(buff, temp, info->ForceChannelLength * info->SenseChannelLength * 5);
			memset(temp, 0x00, 5);
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, info->ForceChannelLength * info->SenseChannelLength * 5));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(buff);
}

static void run_rawdata_gap_data_rx_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char *buff = NULL;
	int ii;
	char temp[5] = { 0 };

	sec_cmd_set_default_result(sec);

	buff = kzalloc(info->ForceChannelLength * info->SenseChannelLength * 5, GFP_KERNEL);
	if (!buff) {
		snprintf(temp, sizeof(temp), "NG");
		sec_cmd_set_cmd_result(sec, temp, strnlen(temp, sizeof(temp)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	for (ii = 0; ii < (info->SenseChannelLength * info->ForceChannelLength); ii++) {
		if ((ii + 1) % (info->SenseChannelLength) != 0) {
			snprintf(temp, sizeof(temp), "%d,", (int)abs(info->pFrame[ii + 1] - info->pFrame[ii]));
			strlcat(buff, temp, info->ForceChannelLength * info->SenseChannelLength * 5);
			memset(temp, 0x00, 5);
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, info->ForceChannelLength * info->SenseChannelLength * 5));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(buff);
}

static void run_rawdata_gap_data_tx_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char *buff = NULL;
	int ii;
	char temp[5] = { 0 };

	sec_cmd_set_default_result(sec);

	buff = kzalloc(info->ForceChannelLength * info->SenseChannelLength * 5, GFP_KERNEL);
	if (!buff) {
		snprintf(temp, sizeof(temp), "NG");
		sec_cmd_set_cmd_result(sec, temp, strnlen(temp, sizeof(temp)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	for (ii = 0; ii < (info->SenseChannelLength * info->ForceChannelLength); ii++) {
		if (ii < (info->ForceChannelLength - 1) * info->SenseChannelLength) {
			snprintf(temp, sizeof(temp), "%d,",
					(int)abs(info->pFrame[ii + info->SenseChannelLength] - info->pFrame[ii]));
			strlcat(buff, temp, info->ForceChannelLength * info->SenseChannelLength * 5);
			memset(temp, 0x00, 5);
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, info->ForceChannelLength * info->SenseChannelLength * 5));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(buff);
}

static void factory_cmd_result_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	sec->item_count = 0;
	memset(sec->cmd_result_all, 0x00, SEC_CMD_RESULT_STR_LEN);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		sec->cmd_all_factory_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	sec->cmd_all_factory_state = SEC_CMD_STATUS_RUNNING;

	get_chip_vendor(sec);
	get_chip_name(sec);
	get_fw_ver_bin(sec);
	get_fw_ver_ic(sec);

	enter_factory_mode(info, true);

//	run_lp_single_ended_rawcap_read(sec);	/* Mutual raw */
	run_rawcap_read(sec);
	run_self_raw_read(sec);

	fts_set_scanmode(info, FTS_SCAN_MODE_SCAN_OFF);
	fts_release_all_finger(info);

	// have to check items
//	get_strength_all_data(sec); /* jitter data */
//	run_high_frequency_rawcap_read(sec);	/* micro_open */
//	get_rawcap_gap_data(sec);		/* micro_open rawcap gap */
//	run_low_frequency_rawcap_read(sec);	/* micro_short gap diff */

//	run_active_cx_data_read(sec);
	run_cx_data_read(sec);
	get_cx_gap_data(sec);
	run_ix_data_read(sec);

	get_wet_mode(sec);

	/* do not systemreset in COB type */
	if (info->board->chip_on_board)
		fts_set_scanmode(info, info->scan_mode);
	else
		enter_factory_mode(info, false);

	run_mutual_jitter(sec);
	run_self_jitter(sec);
	run_factory_miscalibration(sec);

	if (info->board->support_sram_test)
		run_sram_test(sec);

	sec->cmd_all_factory_state = SEC_CMD_STATUS_OK;

out:
	input_info(true, &info->client->dev, "%s: %d%s\n", __func__, sec->item_count, sec->cmd_result_all);
}

static void factory_cmd_result_all_imagetest(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	sec->item_count = 0;
	memset(sec->cmd_result_all, 0x00, SEC_CMD_RESULT_STR_LEN);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		sec->cmd_all_factory_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	sec->cmd_all_factory_state = SEC_CMD_STATUS_RUNNING;

	run_jitter_delta_test(sec);

	sec->cmd_all_factory_state = SEC_CMD_STATUS_OK;

out:
	input_info(true, &info->client->dev, "%s: %d%s\n", __func__, sec->item_count, sec->cmd_result_all);
}

static void set_factory_level(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: Touch is stopped!\n", __func__);
		goto NG;
	}

	if (sec->cmd_param[0] < OFFSET_FAC_SUB || sec->cmd_param[0] > OFFSET_FAC_MAIN) {
		input_err(true, &info->client->dev,
				"%s: cmd data is abnormal, %d\n", __func__, sec->cmd_param[0]);
		goto NG;
	}

	info->factory_position = sec->cmd_param[0];

	input_info(true, &info->client->dev, "%s: %d\n", __func__, info->factory_position);
	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	return;

NG:
	snprintf(buff, sizeof(buff), "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}

int fts_get_tsp_test_result(struct fts_ts_info *info)
{
	u8 data;
	int ret;

	ret = get_nvm_data(info, FTS_NVM_OFFSET_FAC_RESULT, &data);
	if (ret < 0)
		goto err_read;

	if (data == 0xFF)
		data = 0;

	info->test_result.data[0] = data;

err_read:
	return ret;
}
EXPORT_SYMBOL(fts_get_tsp_test_result);

static void get_tsp_test_result(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	ret = fts_get_tsp_test_result(info);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: get failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "M:%s, M:%d, A:%s, A:%d",
				info->test_result.module_result == 0 ? "NONE" :
				info->test_result.module_result == 1 ? "FAIL" :
				info->test_result.module_result == 2 ? "PASS" : "A",
				info->test_result.module_count,
				info->test_result.assy_result == 0 ? "NONE" :
				info->test_result.assy_result == 1 ? "FAIL" :
				info->test_result.assy_result == 2 ? "PASS" : "A",
				info->test_result.assy_count);

		sec_cmd_set_cmd_result(sec, buff, strlen(buff));
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
}

/* FACTORY TEST RESULT SAVING FUNCTION
 * bit 3 ~ 0 : OCTA Assy
 * bit 7 ~ 4 : OCTA module
 * param[0] : OCTA module(1) / OCTA Assy(2)
 * param[1] : TEST NONE(0) / TEST FAIL(1) / TEST PASS(2) : 2 bit
 */
static void set_tsp_test_result(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	ret = fts_get_tsp_test_result(info);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: get failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}
	sec->cmd_state = SEC_CMD_STATUS_RUNNING;

	if (sec->cmd_param[0] == TEST_OCTA_ASSAY) {
		info->test_result.assy_result = sec->cmd_param[1];
		if (info->test_result.assy_count < 3)
			info->test_result.assy_count++;

	} else if (sec->cmd_param[0] == TEST_OCTA_MODULE) {
		info->test_result.module_result = sec->cmd_param[1];
		if (info->test_result.module_count < 3)
			info->test_result.module_count++;
	}

	input_info(true, &info->client->dev, "%s: [0x%X] M:%s, M:%d, A:%s, A:%d\n",
			__func__, info->test_result.data[0],
			info->test_result.module_result == 0 ? "NONE" :
			info->test_result.module_result == 1 ? "FAIL" :
			info->test_result.module_result == 2 ? "PASS" : "A",
			info->test_result.module_count,
			info->test_result.assy_result == 0 ? "NONE" :
			info->test_result.assy_result == 1 ? "FAIL" :
			info->test_result.assy_result == 2 ? "PASS" : "A",
			info->test_result.assy_count);

	ret = set_nvm_data(info, FTS_NVM_OFFSET_FAC_RESULT, info->test_result.data);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: set failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

int fts_get_disassemble_count(struct fts_ts_info *info)
{
	u8 data;
	int ret;

	ret = get_nvm_data(info, FTS_NVM_OFFSET_DISASSEMBLE_COUNT, &data);
	if (ret < 0)
		goto err_read;

	if (data == 0xFF)
		data = 0;

	info->disassemble_count = data;

err_read:
	return ret;
}

static void increase_disassemble_count(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	ret = fts_get_disassemble_count(info);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: get failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}
	sec->cmd_state = SEC_CMD_STATUS_RUNNING;

	if (info->disassemble_count < 0xFE)
		info->disassemble_count++;

	ret = set_nvm_data(info, FTS_NVM_OFFSET_DISASSEMBLE_COUNT, &info->disassemble_count);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: set failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_disassemble_count(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	ret = fts_get_disassemble_count(info);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: get failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%d", info->disassemble_count);

		sec_cmd_set_cmd_result(sec, buff, strlen(buff));
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
}

static void get_osc_trim_error(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	ret = info->fts_systemreset(info, 0);
	if (ret == -FTS_ERROR_BROKEN_OSC_TRIM) {
		snprintf(buff, sizeof(buff), "1");
	} else {
		snprintf(buff, sizeof(buff), "0");
	}
	fts_set_scanmode(info, info->scan_mode);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "OSC_TRIM_ERR");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_osc_trim_info(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;
	unsigned char data[4];
	unsigned char regAdd[3];

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	ret = info->fts_get_sysinfo_data(info, FTS_SI_OSC_TRIM_INFO, 2, data);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: system info read failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	regAdd[0] = 0xA6;
	regAdd[1] = data[1];
	regAdd[2] = data[0];

	memset(data, 0x00, 4);
	ret = info->fts_read_reg(info, regAdd, 3, data, 4);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: osc trim info read failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	snprintf(buff, sizeof(buff), "%02X%02X%02X%02X", data[0], data[1], data[2], data[3]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "OSC_TRIM_INFO");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_snr_non_touched(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 address[5] = { 0 };
	u16 status;
	int ret = 0;
	int wait_time = 0;
	int retry_cnt = 0;

	input_info(true, &info->client->dev, "%s\n", __func__);

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		goto out_init;
	}

	if (sec->cmd_param[0] < 1 || sec->cmd_param[0] > 1000) {
		input_err(true, &info->client->dev, "%s: strange value frame:%d\n",
				__func__, sec->cmd_param[0]);
		goto out_init;
	}

	fts_fix_active_mode(info, true);

	/* enter SNR mode */
	address[0] = 0x70;
	address[1] = 0x25;
	ret = info->fts_write_reg(info, &address[0], 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: i2c_write failed\n", __func__);
		goto out;
	}

	/* start Non-touched Peak Noise */
	address[0] = 0xC7;
	address[1] = 0x09;
	address[2] = 0x01;
	address[3] = (u8)(sec->cmd_param[0] & 0xff);
	address[4] = (u8)(sec->cmd_param[0] >> 8);
	ret = info->fts_write_reg(info, &address[0], 5);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: i2c_write failed\n", __func__);
		goto out;
	}


	wait_time = (sec->cmd_param[0] * 1000) / 120 + (sec->cmd_param[0] * 1000) % 120;

	fts_delay(wait_time);

	retry_cnt = 50;
	address[0] = 0x72;
	while ((info->fts_read_reg(info, &address[0], 1, (u8 *)&status, 2) > 0) && (retry_cnt-- > 0)) {
		if (status == 1)
			break;
		fts_delay(20);
	}

	if (status == 0) {
		input_err(true, &info->client->dev, "%s: failed non-touched status:%d\n", __func__, status);
		goto out;
	}

	snprintf(buff, sizeof(buff), "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	/* EXIT SNR mode */
	address[0] = 0x70;
	address[1] = 0x00;
	info->fts_write_reg(info, &address[0], 2);
	fts_fix_active_mode(info, false);
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);

	return;

out:
	address[0] = 0x70;
	address[1] = 0x00;
	info->fts_write_reg(info, &address[0], 2);
	fts_fix_active_mode(info, false);

out_init:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_snr_touched(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	struct stm_ts_snr_result_cmd snr_cmd_result;
	struct stm_ts_snr_result snr_result;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char tbuff[SEC_CMD_STR_LEN] = { 0 };
	u8 address[5] = { 0 };
	int ret = 0;
	int wait_time = 0;
	int retry_cnt = 0;
	int i = 0;

	input_info(true, &info->client->dev, "%s\n", __func__);

	sec_cmd_set_default_result(sec);
	memset(&snr_result, 0, sizeof(struct stm_ts_snr_result));
	memset(&snr_cmd_result, 0, sizeof(struct stm_ts_snr_result_cmd));

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		goto out_init;
	}

	if (sec->cmd_param[0] < 1 || sec->cmd_param[0] > 1000) {
		input_err(true, &info->client->dev, "%s: strange value frame:%d\n",
				__func__, sec->cmd_param[0]);
		goto out_init;
	}

	fts_fix_active_mode(info, true);

	/* enter SNR mode */
	address[0] = 0x70;
	address[1] = 0x25;
	ret = info->fts_write_reg(info, &address[0], 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: i2c_write failed\n", __func__);
		goto out;
	}

	/* start touched Peak Noise */
	address[0] = 0xC7;
	address[1] = 0x09;
	address[2] = 0x02;
	address[3] = (u8)(sec->cmd_param[0] & 0xff);
	address[4] = (u8)(sec->cmd_param[0] >> 8);
	ret = info->fts_write_reg(info, &address[0], 5);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: i2c_write failed\n", __func__);
		goto out;
	}

	wait_time = (sec->cmd_param[0] * 1000) / 120 + (sec->cmd_param[0] * 1000) % 120;

	fts_delay(wait_time);

	retry_cnt = 50;
	address[0] = 0x72;
	while ((info->fts_read_reg(info, &address[0], 1, (u8 *)&snr_cmd_result, 6) > 0) && (retry_cnt-- > 0)) {
		if (snr_cmd_result.status == 1)
			break;
		fts_delay(20);
	}

	if (snr_cmd_result.status == 0) {
		input_err(true, &info->client->dev, "%s: failed non-touched status:%d\n", __func__, snr_cmd_result.status);
		goto out;
	} else {
		input_info(true, &info->client->dev, "%s: status:%d, point:%d, average:%d\n", __func__,
			snr_cmd_result.status, snr_cmd_result.point, snr_cmd_result.average);
	}

	address[0] = 0x72;
	ret = info->fts_read_reg(info, &address[0], 1, (u8 *)&snr_result, sizeof(struct stm_ts_snr_result));
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: i2c_write failed size:%ld\n", __func__, sizeof(struct stm_ts_snr_result));
		goto out;
	}

	for (i = 0; i < 9; i++) {
		input_info(true, &info->client->dev, "%s: average:%d, snr1:%d, snr2:%d\n", __func__,
			snr_result.result[i].average, snr_result.result[i].snr1, snr_result.result[i].snr2);
		snprintf(tbuff, sizeof(tbuff), "%d,%d,%d,",
			snr_result.result[i].average,
			snr_result.result[i].snr1,
			snr_result.result[i].snr2);
		strlcat(buff, tbuff, sizeof(buff));
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	/* EXIT SNR mode */
	address[0] = 0x70;
	address[1] = 0x00;
	info->fts_write_reg(info, &address[0], 2);
	fts_fix_active_mode(info, false);
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);

	return;
out:
	address[0] = 0x70;
	address[1] = 0x00;
	info->fts_write_reg(info, &address[0], 2);
	fts_fix_active_mode(info, false);
out_init:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

/*
static void run_elvss_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;
	u8 data[FTS_EVENT_SIZE + 1];
	int retry = 10;

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "NG");

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	info->fts_systemreset(info, 0);
	fts_interrupt_set(info, INT_DISABLE);

	memset(data, 0x00, 8);

	data[0] = 0xA4;
	data[1] = 0x04;
	data[2] = 0x00;
	data[3] = 0x04;

	ret = info->fts_write_reg(info, data, 4);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: write failed. ret: %d\n", __func__, ret);
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		fts_reinit(info, false);
		fts_interrupt_set(info, INT_ENABLE);
		return;
	}

	memset(data, 0x00, FTS_EVENT_SIZE);
	data[0] = FTS_READ_ONE_EVENT;
	while (info->fts_read_reg(info, &data[0], 1, &data[1], FTS_EVENT_SIZE) > 0) {
		input_info(true, &info->client->dev,
			"%s: %02X: %02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X\n",
			__func__, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
			data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

		if (data[1] == FTS_EVENT_ERROR_REPORT) {
			break;
		} else if (data[1] == 0x03) {
			snprintf(buff, sizeof(buff), "OK");
			break;
		} else if (retry < 0) {
			break;
		}
		retry--;
		fts_delay(50);

	}

	fts_reinit(info, false);
	fts_interrupt_set(info, INT_ENABLE);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}
*/
int set_nvm_data_by_size(struct fts_ts_info *info, u8 offset, int length, u8 *buf)
{
	u8 regAdd[256] = { 0 };
	u8 remaining, index, sendinglength;
	int ret;

	info->fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true); // Clear FIFO

	fts_release_all_finger(info);

	remaining = length;
	index = 0;
	sendinglength = 0;

	while (remaining) {
		regAdd[0] = 0xC7;
		regAdd[1] = 0x01;
		regAdd[2] = offset + index;

		// write data up to 12 bytes available
		if (remaining < 13) {
			memcpy(&regAdd[3], &buf[index], remaining);
			sendinglength = remaining;
		} else {
			memcpy(&regAdd[3], &buf[index], 12);
			index += 12;
			sendinglength = 12;
		}

		ret = fts_write_reg(info, &regAdd[0], sendinglength + 3);
		if (ret < 0) {
			input_err(true, &info->client->dev,
					"%s: write failed. ret: %d\n", __func__, ret);
			return ret;
		}
		remaining -= sendinglength;
	}

	fts_interrupt_set(info, INT_DISABLE);

	// Save to flash
	regAdd[0] = 0xA4;
	regAdd[1] = 0x05;
	regAdd[2] = 0x04; // panel configuration area

	ret = fts_fw_wait_for_echo_event(info, &regAdd[0], 3, 200);
	if (ret < 0)
		input_err(true, &info->client->dev,
				"%s: failed to get echo. ret: %d\n", __func__, ret);

	fts_interrupt_set(info, INT_ENABLE);

	return ret;
}

int set_nvm_data(struct fts_ts_info *info, u8 type, u8 *buf)
{
	return set_nvm_data_by_size(info, nvm_data[type].offset, nvm_data[type].length, buf);
}

int get_nvm_data_by_size(struct fts_ts_info *info, u8 offset, int length, u8 *nvdata)
{
	u8 regAdd[3] = {0};
	u8 data[128] = { 0 };
	int ret;

	info->fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true); // Clear FIFO

	fts_release_all_finger(info);

	fts_interrupt_set(info, INT_DISABLE);

	// Request SEC factory debug data from flash
	regAdd[0] = 0xA4;
	regAdd[1] = 0x06;
	regAdd[2] = 0x90;

	ret = fts_fw_wait_for_echo_event(info, &regAdd[0], 3, 0);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: timeout. ret: %d\n", __func__, ret);
		fts_interrupt_set(info, INT_ENABLE);
		return ret;
	}

	fts_interrupt_set(info, INT_ENABLE);

	regAdd[0] = 0xA6;
	regAdd[1] = 0x00;
	regAdd[2] = offset;

	ret = fts_read_reg(info, &regAdd[0], 3, data, length + 1);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: read failed. ret: %d\n", __func__, ret);
		return ret;
	}

	memcpy(nvdata, &data[0], length);

	input_raw_info_d(true, &info->client->dev, "%s: offset [%d], length [%d]\n",
			__func__, offset, length);

	return ret;
}

int get_nvm_data(struct fts_ts_info *info, int type, u8 *nvdata)
{
	int size = sizeof(nvm_data) / sizeof(struct fts_nvm_data_map);

	if (type >= size)
		return -EINVAL;

	return get_nvm_data_by_size(info, nvm_data[type].offset, nvm_data[type].length, nvdata);
}

#ifdef TCLM_CONCEPT
int fts_tclm_data_read(struct i2c_client *client, int address)
{
	struct fts_ts_info *info = i2c_get_clientdata(client);
	int ret = 0;
	int i = 0;
	u8 nbuff[FTS_NVM_OFFSET_ALL];
	u16 ic_version;

	switch (address) {
	case SEC_TCLM_NVM_OFFSET_IC_FIRMWARE_VER:
		ret = info->fts_get_version_info(info);
		ic_version = (info->module_version_of_ic << 8) | (info->fw_main_version_of_ic & 0xFF);
		return ic_version;
	case SEC_TCLM_NVM_ALL_DATA:
		ret = get_nvm_data_by_size(info, nvm_data[FTS_NVM_OFFSET_FAC_RESULT].offset,
				FTS_NVM_OFFSET_ALL, nbuff);
		if (ret < 0)
			return ret;
		info->tdata->nvdata.cal_count = nbuff[nvm_data[FTS_NVM_OFFSET_CAL_COUNT].offset];
		info->tdata->nvdata.tune_fix_ver = (nbuff[nvm_data[FTS_NVM_OFFSET_TUNE_VERSION].offset] << 8) |
							nbuff[nvm_data[FTS_NVM_OFFSET_TUNE_VERSION].offset + 1];
		info->tdata->nvdata.cal_position = nbuff[nvm_data[FTS_NVM_OFFSET_CAL_POSITION].offset];
		info->tdata->nvdata.cal_pos_hist_cnt = nbuff[nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_COUNT].offset];
		info->tdata->nvdata.cal_pos_hist_lastp = nbuff[nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_LASTP].offset];
		for (i = nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO].offset;
				i < nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO].offset +
				nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO].length; i++)
			info->tdata->nvdata.cal_pos_hist_queue[i - nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO].offset] = nbuff[i];

		info->tdata->nvdata.cal_fail_falg = nbuff[nvm_data[FTS_NVM_OFFSET_CAL_FAIL_FLAG].offset];
		info->tdata->nvdata.cal_fail_cnt = nbuff[nvm_data[FTS_NVM_OFFSET_CAL_FAIL_COUNT].offset];
		info->fac_nv = nbuff[nvm_data[FTS_NVM_OFFSET_FAC_RESULT].offset];
		info->disassemble_count = nbuff[nvm_data[FTS_NVM_OFFSET_DISASSEMBLE_COUNT].offset];
		return ret;
	case SEC_TCLM_NVM_TEST:
		input_info(true, &info->client->dev, "%s: dt: tclm_level [%d] afe_base [%04X]\n",
			__func__, info->tdata->tclm_level, info->tdata->afe_base);
		ret = get_nvm_data_by_size(info, FTS_NVM_OFFSET_ALL + SEC_TCLM_NVM_OFFSET,
			SEC_TCLM_NVM_OFFSET_LENGTH, info->tdata->tclm);
		if (info->tdata->tclm[0] != 0xFF) {
			info->tdata->tclm_level = info->tdata->tclm[0];
			info->tdata->afe_base = (info->tdata->tclm[1] << 8) | info->tdata->tclm[2];
			input_info(true, &info->client->dev, "%s: nv: tclm_level [%d] afe_base [%04X]\n",
				__func__, info->tdata->tclm_level, info->tdata->afe_base);
		}
		return ret;
	default:
		return ret;
	}
}

int fts_tclm_data_write(struct i2c_client *client, int address)
{
	struct fts_ts_info *info = i2c_get_clientdata(client);
	int ret = 1;
	int i = 0;
	u8 nbuff[FTS_NVM_OFFSET_ALL];

	switch (address) {
	case SEC_TCLM_NVM_ALL_DATA:
		memset(nbuff, 0x00, FTS_NVM_OFFSET_ALL);
		nbuff[nvm_data[FTS_NVM_OFFSET_FAC_RESULT].offset] = info->fac_nv;
		nbuff[nvm_data[FTS_NVM_OFFSET_DISASSEMBLE_COUNT].offset] = info->disassemble_count;
		nbuff[nvm_data[FTS_NVM_OFFSET_CAL_COUNT].offset] = info->tdata->nvdata.cal_count;
		nbuff[nvm_data[FTS_NVM_OFFSET_TUNE_VERSION].offset] = (u8)(info->tdata->nvdata.tune_fix_ver >> 8);
		nbuff[nvm_data[FTS_NVM_OFFSET_TUNE_VERSION].offset + 1] = (u8)(0xff & info->tdata->nvdata.tune_fix_ver);
		nbuff[nvm_data[FTS_NVM_OFFSET_CAL_POSITION].offset] = info->tdata->nvdata.cal_position;
		nbuff[nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_COUNT].offset] = info->tdata->nvdata.cal_pos_hist_cnt;
		nbuff[nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_LASTP].offset] = info->tdata->nvdata.cal_pos_hist_lastp;
		for (i = nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO].offset;
				i < nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO].offset +
				nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO].length; i++)
			nbuff[i] = info->tdata->nvdata.cal_pos_hist_queue[i - nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO].offset];
		nbuff[nvm_data[FTS_NVM_OFFSET_CAL_FAIL_FLAG].offset] = info->tdata->nvdata.cal_fail_falg;
		nbuff[nvm_data[FTS_NVM_OFFSET_CAL_FAIL_COUNT].offset] = info->tdata->nvdata.cal_fail_cnt;
		ret = set_nvm_data_by_size(info, nvm_data[FTS_NVM_OFFSET_FAC_RESULT].offset, FTS_NVM_OFFSET_ALL, nbuff);
		return ret;
	case SEC_TCLM_NVM_TEST:
		ret = set_nvm_data_by_size(info, FTS_NVM_OFFSET_ALL + SEC_TCLM_NVM_OFFSET,
			SEC_TCLM_NVM_OFFSET_LENGTH, info->tdata->tclm);
		return ret;
	default:
		return ret;
	}
}

static void tclm_test_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	struct sec_tclm_data *data = info->tdata;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret = 0;

	sec_cmd_set_default_result(sec);

	if (!info->tdata->support_tclm_test)
		goto not_support;

	ret = tclm_test_command(data, sec->cmd_param[0], sec->cmd_param[1], sec->cmd_param[2], buff);
	if (ret < 0)
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	else
		sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	return;

not_support:
	snprintf(buff, sizeof(buff), "%s", "NA");
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}

static void get_calibration(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (!info->tdata->support_tclm_test)
		goto not_support;

	snprintf(buff, sizeof(buff), "%d", info->is_cal_done);

	info->is_cal_done = false;
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	return;

not_support:
	snprintf(buff, sizeof(buff), "%s", "NA");
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}
#endif

#ifdef CONFIG_GLOVE_TOUCH
static void glove_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[3] = {0};

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		info->glove_enabled = sec->cmd_param[0];

		if (info->fts_power_state != FTS_POWER_STATE_POWERDOWN && info->reinit_done) {
			if (info->glove_enabled)
				info->touch_functions = info->touch_functions | FTS_TOUCHTYPE_BIT_GLOVE |
							FTS_TOUCHTYPE_DEFAULT_ENABLE;
			else
				info->touch_functions = (info->touch_functions & (~FTS_TOUCHTYPE_BIT_GLOVE)) |
							FTS_TOUCHTYPE_DEFAULT_ENABLE;
		}

		regAdd[0] = FTS_CMD_SET_GET_TOUCHTYPE;
		regAdd[1] = (u8)(info->touch_functions & 0xFF);
		regAdd[2] = (u8)(info->touch_functions >> 8);
		fts_write_reg(info, &regAdd[0], 3);

		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}
#endif

static void clear_cover_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 3) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		if (sec->cmd_param[0] > 1) {
			info->flip_enable = true;
			info->cover_type = sec->cmd_param[1];
		} else {
			info->flip_enable = false;
		}

		if (info->fts_power_state != FTS_POWER_STATE_POWERDOWN && info->reinit_done) {
			if (info->flip_enable)
				fts_set_cover_type(info, true);
			else
				fts_set_cover_type(info, false);
		}

		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
};

static void report_rate(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 scan_rate;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 255) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {

		scan_rate = sec->cmd_param[0];
		fts_change_scan_rate(info, scan_rate);

		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;

out:
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
static void interrupt_control(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		int enables = sec->cmd_param[0];

		if (enables)
			fts_irq_enable(info, true);
		else
			fts_irq_enable(info, false);

		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;

out:
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}
#endif

static void set_wirelesscharger_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		info->charger_mode = sec->cmd_param[0];

		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 3) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		info->charger_mode = sec->cmd_param[0];

		fts_wirelesscharger_mode(info);

		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;

out:
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
};

/*********************************************************************
 * flag	1  :  set edge handler
 *	2  :  set (portrait, normal) edge zone data
 *	4  :  set (portrait, normal) dead zone data
 *	8  :  set landscape mode data
 *	16 :  mode clear
 * data
 *	0x00, FFF (y start), FFF (y end),  FF(direction)
 *	0x01, FFFF (edge zone)
 *	0x02, FF (up x), FF (down x), FFFF (y)
 *	0x03, FF (mode), FFF (edge), FFF (dead zone)
 * case
 *	edge handler set :  0x00....
 *	booting time :  0x00...  + 0x01...
 *	normal mode : 0x02...  (+0x01...)
 *	landscape mode : 0x03...
 *	landscape -> normal (if same with old data) : 0x03, 0
 *	landscape -> normal (etc) : 0x02....  + 0x03, 0
 *********************************************************************/

void fts_set_grip_data_to_ic(struct fts_ts_info *info, u8 flag)
{
	u8 data[4] = { 0 };
	u8 regAdd[11] = {FTS_CMD_SET_FUNCTION_ONOFF, };

	input_info(true, &info->client->dev, "%s: flag: %02X (clr,lan,nor,edg,han)\n", __func__, flag);

	memset(&regAdd[1], 0x00, 10);

	if (flag & G_SET_EDGE_HANDLER) {
		if (info->grip_edgehandler_direction == 0) {
			data[0] = 0x0;
			data[1] = 0x0;
			data[2] = 0x0;
			data[3] = 0x0;
		} else {
			data[0] = (info->grip_edgehandler_start_y >> 4) & 0xFF;
			data[1] = (info->grip_edgehandler_start_y << 4 & 0xF0) |
					((info->grip_edgehandler_end_y >> 8) & 0xF);
			data[2] = info->grip_edgehandler_end_y & 0xFF;
			data[3] = info->grip_edgehandler_direction & 0x3;
		}

		regAdd[1] = FTS_FUNCTION_EDGE_HANDLER;
		regAdd[2] = data[0];
		regAdd[3] = data[1];
		regAdd[4] = data[2];
		regAdd[5] = data[3];

		fts_write_reg(info, regAdd, 6);
	}

	if (flag & G_SET_EDGE_ZONE) {
		/* ex) C1 07 00 3E 00 3E
		*	- 0x003E(60) px : Grip Right zone
		*	- 0x003E(60) px : Grip Left zone
		*/
		regAdd[1] = FTS_FUNCTION_EDGE_AREA;
		regAdd[2] = (info->grip_edge_range >> 8) & 0xFF;
		regAdd[3] = info->grip_edge_range & 0xFF;
		regAdd[4] = (info->grip_edge_range >> 8) & 0xFF;
		regAdd[5] = info->grip_edge_range & 0xFF;

		fts_write_reg(info, regAdd, 6);
	}

	if (flag & G_SET_NORMAL_MODE) {
		/* ex) C1 08 1E 1E 00 00
		*	- 0x1E (30) px : upper X range
		*	- 0x1E (30) px : lower X range
		*	- 0x0000 (0) px : division Y
		*/
		regAdd[1] = FTS_FUNCTION_DEAD_ZONE;
		regAdd[2] = info->grip_deadzone_up_x & 0xFF;
		regAdd[3] = info->grip_deadzone_dn_x & 0xFF;
		regAdd[4] = (info->grip_deadzone_y >> 8) & 0xFF;
		regAdd[5] = info->grip_deadzone_y & 0xFF;

		fts_write_reg(info, regAdd, 6);
	}

	if (flag & G_SET_LANDSCAPE_MODE) {
		/* ex) C1 09 01 00 3C 00 3C 00 1E
		*	- 0x01 : horizontal mode
		*	- 0x03C (60) px : Grip zone range (Right)
		*	- 0x03C (60) px : Grip zone range (Left)
		*	- 0x01E (30) px : Reject zone range (Left/Right)
		*/
		regAdd[1] = FTS_FUNCTION_LANDSCAPE_MODE;
		regAdd[2] = info->grip_landscape_mode;
		regAdd[3] = (info->grip_landscape_edge >> 8) & 0xFF;
		regAdd[4] = info->grip_landscape_edge & 0xFF;
		regAdd[5] = (info->grip_landscape_edge >> 8) & 0xFF;
		regAdd[6] = info->grip_landscape_edge & 0xFF;
		regAdd[7] = (info->grip_landscape_deadzone >> 8) & 0xFF;
		regAdd[8] = info->grip_landscape_deadzone & 0xFF;

		fts_write_reg(info, regAdd, 9);

		/*ex) C1 0A 01 00 3C 00 3C 00 1E 00 1E
		*	- 0x01(1) : Enable function
		*	- 0x003C (60) px : Grip Top zone range
		*	- 0x001E (60) px : Grip Bottom zone range
		*	- 0x001E (30) px : Reject Top zone range
		*	- 0x001E (30) px : Reject Bottom zone range
		*/
		regAdd[1] = FTS_FUNCTION_LANDSCAPE_TOP_BOTTOM;
		regAdd[2] = info->grip_landscape_mode;
		regAdd[3] = (info->grip_landscape_top_gripzone >> 8) & 0xFF;
		regAdd[4] = info->grip_landscape_top_gripzone & 0xFF;
		regAdd[5] = (info->grip_landscape_bottom_gripzone >> 8) & 0xFF;
		regAdd[6] = info->grip_landscape_bottom_gripzone & 0xFF;
		regAdd[7] = (info->grip_landscape_top_deadzone >> 8) & 0xFF;
		regAdd[8] = info->grip_landscape_top_deadzone & 0xFF;
		regAdd[9] = (info->grip_landscape_bottom_deadzone >> 8) & 0xFF;
		regAdd[10] = info->grip_landscape_bottom_deadzone & 0xFF;

		fts_write_reg(info, regAdd, 11);
	}

	if (flag & G_CLR_LANDSCAPE_MODE) {
		memset(&regAdd[1], 0x00, 10);

		/* ex) C1 09  00 00 00 00 00 00 00
		*	- 0x00 : Apply previous vertical mode value for grip zone and reject zone range
		*/
		regAdd[1] = FTS_FUNCTION_LANDSCAPE_MODE;
		regAdd[2] = info->grip_landscape_mode;

		fts_write_reg(info, regAdd, 9);

		/*ex) C1 0A 00 00 00 00 00 00 00 00 00
		*	- Disable function
		*/
		regAdd[1] = FTS_FUNCTION_LANDSCAPE_TOP_BOTTOM;

		fts_write_reg(info, regAdd, 11);
	}
}

/*
 * index
 *	0 :  set edge handler
 *	1 :  portrait (normal) mode
 *	2 :  landscape mode
 * data
 *	0, X (direction), X (y start), X (y end)
 *	direction : 0 (off), 1 (left), 2 (right)
 *	ex) echo set_grip_data,0,2,600,900 > cmd
 *
 *
 *	1, X (edge zone), X (dead zone up x), X (dead zone down x), X (dead zone y)
 *	ex) echo set_grip_data,1,200,10,50,1500 > cmd
 *
 *	2, 1 (landscape mode), X (edge zone), X (dead zone), X (dead zone top y), X (dead zone bottom y)
 *	ex) echo set_grip_data,2,1,200,100,120,0 > cmd
 *
 *	2, 0 (portrait mode)
 *	ex) echo set_grip_data,2,0  > cmd
 */
static void set_grip_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 mode = G_NONE;

	sec_cmd_set_default_result(sec);

	memset(buff, 0, sizeof(buff));

	mutex_lock(&info->device_mutex);

	if (sec->cmd_param[0] == 0) {	// edge handler
		if (sec->cmd_param[1] == 0) {	// clear
			info->grip_edgehandler_direction = 0;
		} else if (sec->cmd_param[1] < 3) {
			info->grip_edgehandler_direction = sec->cmd_param[1];
			info->grip_edgehandler_start_y = sec->cmd_param[2];
			info->grip_edgehandler_end_y = sec->cmd_param[3];
		} else {
			input_err(true, &info->client->dev, "%s: cmd1 is abnormal, %d (%d)\n",
					__func__, sec->cmd_param[1], __LINE__);
			goto err_grip_data;
		}

		mode = mode | G_SET_EDGE_HANDLER;
		fts_set_grip_data_to_ic(info, mode);
	} else if (sec->cmd_param[0] == 1) {	// normal mode
		if (info->grip_edge_range != sec->cmd_param[1])
			mode = mode | G_SET_EDGE_ZONE;

		info->grip_edge_range = sec->cmd_param[1];
		info->grip_deadzone_up_x = sec->cmd_param[2];
		info->grip_deadzone_dn_x = sec->cmd_param[3];
		info->grip_deadzone_y = sec->cmd_param[4];
		mode = mode | G_SET_NORMAL_MODE;

		if (info->grip_landscape_mode == 1) {
			info->grip_landscape_mode = 0;
			mode = mode | G_CLR_LANDSCAPE_MODE;
		}

		fts_set_grip_data_to_ic(info, mode);
	} else if (sec->cmd_param[0] == 2) {	// landscape mode
		if (sec->cmd_param[1] == 0) {	// normal mode
			info->grip_landscape_mode = 0;
			mode = mode | G_CLR_LANDSCAPE_MODE;
		} else if (sec->cmd_param[1] == 1) {
			info->grip_landscape_mode = 1;
			info->grip_landscape_edge = sec->cmd_param[2];
			info->grip_landscape_deadzone = sec->cmd_param[3];
			info->grip_landscape_top_deadzone = sec->cmd_param[4];
			info->grip_landscape_bottom_deadzone = sec->cmd_param[5];
			info->grip_landscape_top_gripzone = sec->cmd_param[6];
			info->grip_landscape_bottom_gripzone = sec->cmd_param[7];
			mode = mode | G_SET_LANDSCAPE_MODE;
		} else {
			input_err(true, &info->client->dev, "%s: cmd1 is abnormal, %d (%d)\n",
					__func__, sec->cmd_param[1], __LINE__);
			goto err_grip_data;
		}

		fts_set_grip_data_to_ic(info, mode);
	} else {
		input_err(true, &info->client->dev, "%s: cmd0 is abnormal, %d", __func__, sec->cmd_param[0]);
		goto err_grip_data;
	}

	mutex_unlock(&info->device_mutex);

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;

err_grip_data:
	mutex_unlock(&info->device_mutex);

	snprintf(buff, sizeof(buff), "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void dead_zone_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[3] = {FTS_CMD_SET_FUNCTION_ONOFF, FTS_FUNCTION_ENABLE_DEAD_ZONE, 0x00};
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		if (sec->cmd_param[0] == 0)
			regAdd[2] = 0x01;	/* dead zone disable */
		else
			regAdd[2] = 0x00;	/* dead zone enable */

		ret = info->fts_write_reg(info, regAdd, 3);
		if (ret < 0)
			input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		else
			input_info(true, &info->client->dev, "%s: reg:%d, ret: %d\n", __func__, sec->cmd_param[0], ret);

		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void drawing_test_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "NA");
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void spay_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0])
		info->lowpower_flag |= FTS_MODE_SPAY;
	else
		info->lowpower_flag &= ~FTS_MODE_SPAY;

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void aot_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0])
		info->lowpower_flag |= FTS_MODE_DOUBLETAP_WAKEUP;
	else
		info->lowpower_flag &= ~FTS_MODE_DOUBLETAP_WAKEUP;

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	input_info(true, &info->client->dev, "%s: %d\n", __func__, sec->cmd_param[0]);
}

static void aod_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0])
		info->lowpower_flag |= FTS_MODE_AOD;
	else
		info->lowpower_flag &= ~FTS_MODE_AOD;

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	input_info(true, &info->client->dev, "%s: %d\n", __func__, sec->cmd_param[0]);
}

static void singletap_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0])
		info->lowpower_flag |= FTS_MODE_SINGLETAP;
	else
		info->lowpower_flag &= ~FTS_MODE_SINGLETAP;

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	input_info(true, &info->client->dev, "%s: %d\n", __func__, sec->cmd_param[0]);
}

static void set_aod_rect(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 data[8] = {0, };
	int i, ret = -1;

	sec_cmd_set_default_result(sec);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	input_info(true, &info->client->dev, "%s: w:%d, h:%d, x:%d, y:%d\n",
			__func__, sec->cmd_param[0], sec->cmd_param[1],
			sec->cmd_param[2], sec->cmd_param[3]);
#endif

	for (i = 0; i < 4; i++) {
		data[i * 2] = sec->cmd_param[i] & 0xFF;
		data[i * 2 + 1] = (sec->cmd_param[i] >> 8) & 0xFF;
		info->rect_data[i] = sec->cmd_param[i];
	}

#ifdef FTS_SUPPORT_SPONGELIB
	if (!info->use_sponge)
		goto NG;

	ret = info->fts_write_to_sponge(info, FTS_CMD_SPONGE_OFFSET_AOD_RECT, data, sizeof(data));
#endif
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		goto NG;
	}

	if (info->fts_power_state == FTS_POWER_STATE_LOWPOWER) {
		if (sec->cmd_param[0] == 0 && sec->cmd_param[1] == 0 &&
			sec->cmd_param[2] == 0 && sec->cmd_param[3] == 0) {

			input_info(true, &info->client->dev, "%s: sync -> async base scan\n", __func__);
			ret = fts_set_hsync_scanmode(info, FTS_CMD_LPM_ASYNC_SCAN);
		} else {

			input_info(true, &info->client->dev, "%s: sync base scan\n", __func__);
			ret = fts_set_hsync_scanmode(info, FTS_CMD_LPM_SYNC_SCAN);
		}

		if (ret <= 0) {
			input_err(true, &info->client->dev, "%s: Failed to send command!", __func__);
			goto NG;
		}
	}

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;
NG:

	snprintf(buff, sizeof(buff), "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void get_aod_rect(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 data[8] = {0, };
	u16 rect_data[4] = {0, };
	int i, ret = -1;

	sec_cmd_set_default_result(sec);

#ifdef FTS_SUPPORT_SPONGELIB
	if (!info->use_sponge)
		goto NG;

	ret = info->fts_read_from_sponge(info, FTS_CMD_SPONGE_OFFSET_AOD_RECT, data, sizeof(data));
#endif
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		goto NG;
	}

	for (i = 0; i < 4; i++)
		rect_data[i] = (data[i * 2 + 1] & 0xFF) << 8 | (data[i * 2] & 0xFF);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	input_info(true, &info->client->dev, "%s: w:%d, h:%d, x:%d, y:%d\n",
			__func__, rect_data[0], rect_data[1], rect_data[2], rect_data[3]);
#endif

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;
NG:
	snprintf(buff, sizeof(buff), "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void fod_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	int ret = 0;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0])
		info->lowpower_flag |= FTS_MODE_PRESS;
	else
		info->lowpower_flag &= ~FTS_MODE_PRESS;

	info->press_prop = !!sec->cmd_param[1];

	input_info(true, &info->client->dev, "%s: %s, fast:%d, 0x%02X\n",
			__func__, sec->cmd_param[0] ? "on" : "off",
			info->press_prop, info->lowpower_flag);

	ret = info->fts_write_to_sponge(info, FTS_CMD_SPONGE_OFFSET_MODE,
			&info->lowpower_flag, sizeof(info->lowpower_flag));
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	fts_set_press_property(info);
	fts_set_fod_finger_merge(info);

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

int fts_set_fod_rect(struct fts_ts_info *info)
{
	int i, ret;
	u8 data[8];
	u32 sum = 0;

	for (i = 0; i < 4; i++) {
		data[i * 2] = info->fod_rect_data[i] & 0xFF;
		data[i * 2 + 1] = (info->fod_rect_data[i] >> 8) & 0xFF;
		sum += info->fod_rect_data[i];
	}

	if (!sum) /* no data */
		return 0;

	input_info(true, &info->client->dev, "%s: l:%d, t:%d, r:%d, b:%d\n",
			__func__, info->fod_rect_data[0], info->fod_rect_data[1],
			info->fod_rect_data[2], info->fod_rect_data[3]);

	ret = info->fts_write_to_sponge(info, FTS_CMD_SPONGE_FOD_RECT, data, sizeof(data));
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);

	return ret;
}

static void set_fod_rect(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int i, ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] > info->board->max_x
			|| sec->cmd_param[1] > info->board->max_y
			|| sec->cmd_param[2] > info->board->max_x
			|| sec->cmd_param[3] > info->board->max_y) {
		input_err(true, &info->client->dev, "%s: Abnormal fod_rect data\n", __func__);
		goto NG;
	}

	input_info(true, &info->client->dev, "%s: l:%d, t:%d, r:%d, b:%d\n",
			__func__, sec->cmd_param[0], sec->cmd_param[1],
			sec->cmd_param[2], sec->cmd_param[3]);

	for (i = 0; i < 4; i++)
		info->fod_rect_data[i] = sec->cmd_param[i];

	ret = fts_set_fod_rect(info);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		goto NG;
	}

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;
NG:

	snprintf(buff, sizeof(buff), "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

/*
 * Enable or disable external_noise_mode
 *
 * If mode has EXT_NOISE_MODE_MAX,
 * then write enable cmd for all enabled mode. (set as ts->external_noise_mode bit value)
 * This routine need after IC power reset. TSP IC need to be re-wrote all enabled modes.
 *
 * Else if mode has specific value like EXT_NOISE_MODE_MONITOR,
 * then write enable/disable cmd about for that mode's latest setting value.
 *
 * If you want to add new mode,
 * please define new enum value like EXT_NOISE_MODE_MONITOR,
 * then set cmd for that mode like below. (it is in this function)
 * noise_mode_cmd[EXT_NOISE_MODE_MONITOR] = SEC_TS_CMD_SET_MONITOR_NOISE_MODE;
 */
int fts_set_external_noise_mode(struct fts_ts_info *info, u8 mode)
{
	int i, ret, fail_count = 0;
	u8 mode_bit_to_set, check_bit, mode_enable;
	u8 noise_mode_cmd[EXT_NOISE_MODE_MAX] = { 0 };
	u8 regAdd[3] = {FTS_CMD_SET_FUNCTION_ONOFF, 0x00, 0x00};

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: Touch is stopped!\n", __func__);
		return -ENODEV;
	}

	if (mode == EXT_NOISE_MODE_MAX) {
		/* write all enabled mode */
		mode_bit_to_set = info->external_noise_mode;
	} else {
		/* make enable or disable the specific mode */
		mode_bit_to_set = 1 << mode;
	}

	input_info(true, &info->client->dev, "%s: %sable %d\n", __func__,
			info->external_noise_mode & mode_bit_to_set ? "en" : "dis", mode_bit_to_set);

	/* set cmd for each mode */
	noise_mode_cmd[EXT_NOISE_MODE_MONITOR] = FTS_FUNCTION_SET_MONITOR_NOISE_MODE;

	/* write mode */
	for (i = EXT_NOISE_MODE_NONE + 1; i < EXT_NOISE_MODE_MAX; i++) {
		check_bit = 1 << i;
		if (mode_bit_to_set & check_bit) {
			mode_enable = !!(info->external_noise_mode & check_bit);
			regAdd[1] = noise_mode_cmd[i];
			regAdd[2] = mode_enable;

			ret = info->fts_write_reg(info, regAdd, 3);
			if (ret < 0) {
				input_err(true, &info->client->dev, "%s: failed to set %02X %02X %02X\n",
						__func__, regAdd[0], regAdd[1], regAdd[2]);
				fail_count++;
			}
		}
	}

	if (fail_count != 0)
		return -EIO;
	else
		return 0;
}

/*
 * Enable or disable specific external_noise_mode (sec_cmd)
 *
 * This cmd has 2 params.
 * param 0 : the mode that you want to change.
 * param 1 : enable or disable the mode.
 *
 * For example,
 * enable EXT_NOISE_MODE_MONITOR mode,
 * write external_noise_mode,1,1
 * disable EXT_NOISE_MODE_MONITOR mode,
 * write external_noise_mode,1,0
 */
static void external_noise_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] <= EXT_NOISE_MODE_NONE || sec->cmd_param[0] >= EXT_NOISE_MODE_MAX ||
			sec->cmd_param[1] < 0 || sec->cmd_param[1] > 1) {
		input_err(true, &info->client->dev, "%s: not support param\n", __func__);
		goto NG;
	}

	if (sec->cmd_param[1] == 1)
		info->external_noise_mode |= 1 << sec->cmd_param[0];
	else
		info->external_noise_mode &= ~(1 << sec->cmd_param[0]);

	ret = fts_set_external_noise_mode(info, sec->cmd_param[0]);
	if (ret < 0)
		goto NG;

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;

NG:
	snprintf(buff, sizeof(buff), "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void brush_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[3] = {FTS_CMD_SET_FUNCTION_ONOFF, FTS_FUNCTION_ENABLE_BRUSH_MODE, 0x00};
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	info->brush_mode = sec->cmd_param[0];

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	input_info(true, &info->client->dev,
			"%s: set brush mode %s\n", __func__, info->brush_mode ? "enable" : "disable");

	if (info->brush_mode == 0)
		regAdd[2] = 0x00;	/* 0: Disable Artcanvas min phi mode */
	else
		regAdd[2] = 0x01;	/* 1: Enable Artcanvas min phi mode  */

	ret = info->fts_write_reg(info, &regAdd[0], 3);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: failed to set brush mode\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;

out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void set_touchable_area(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[3] = {FTS_CMD_SET_FUNCTION_ONOFF, FTS_FUNCTION_SET_TOUCHABLE_AREA, 0x00};
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	info->touchable_area = sec->cmd_param[0];

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	input_info(true, &info->client->dev,
			"%s: set 16:9 mode %s\n", __func__, info->touchable_area ? "enable" : "disable");

	if (info->touchable_area == 0)
		regAdd[2] = 0x00;	/* 0: Disable 16:9 mode */
	else
		regAdd[2] = 0x01;	/* 1: Enable 16:9 mode  */

	ret = info->fts_write_reg(info, &regAdd[0], 3);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: failed to set 16:9 mode\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;

out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void debug(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	info->debug_string = sec->cmd_param[0];

	input_info(true, &info->client->dev, "%s: command is %d\n", __func__, info->debug_string);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_force_calibration(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	bool touch_on = false;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
#ifdef TCLM_CONCEPT
		info->tdata->external_factory = false;
#endif
		return;
	}

	if (info->rawdata_read_lock == 1) {
		input_err(true, &info->client->dev, "%s: ramdump mode is running, %d\n",
				__func__, info->rawdata_read_lock);
		goto autotune_fail;
	}

	if (info->touch_count > 0) {
		touch_on = true;
		input_err(true, &info->client->dev, "%s: finger on touch(%d)\n", __func__, info->touch_count);
	}

	info->fts_systemreset(info, 0);

	fts_release_all_finger(info);

	if (touch_on) {
		input_err(true, &info->client->dev, "%s: finger! do not run autotune\n", __func__);
	} else {
		input_info(true, &info->client->dev, "%s: run autotune\n", __func__);

		input_err(true, &info->client->dev, "%s: RUN OFFSET CALIBRATION\n", __func__);
		if (fts_execute_autotune(info, true) < 0) {
			fts_set_scanmode(info, info->scan_mode);
			goto autotune_fail;
		}
#ifdef TCLM_CONCEPT
		/* devide tclm case */
		sec_tclm_case(info->tdata, sec->cmd_param[0]);

		input_info(true, &info->client->dev, "%s: param, %d, %c, %d\n", __func__,
			sec->cmd_param[0], sec->cmd_param[0], info->tdata->root_of_calibration);

		if (sec_execute_tclm_package(info->tdata, 1) < 0)
			input_err(true, &info->client->dev,
						"%s: sec_execute_tclm_package\n", __func__);

		sec_tclm_root_of_cal(info->tdata, CALPOSITION_NONE);
#endif
	}

	fts_set_scanmode(info, info->scan_mode);

	if (touch_on) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
#ifdef TCLM_CONCEPT
	info->tdata->external_factory = false;
#endif

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
	return;

autotune_fail:
#ifdef TCLM_CONCEPT
	info->tdata->external_factory = false;
#endif
	fts_interrupt_set(info, INT_ENABLE);
	snprintf(buff, sizeof(buff), "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void fix_active_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		if (info->fts_power_state == FTS_POWER_STATE_ACTIVE)
			fts_fix_active_mode(info, !!sec->cmd_param[0]);
		info->fix_active_mode = !!sec->cmd_param[0];
	}

	snprintf(buff, sizeof(buff), "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void touch_aging_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char regAdd[3];

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		info->touch_aging_mode = sec->cmd_param[0];

		if (info->touch_aging_mode) {
			regAdd[0] = 0xA0;
			regAdd[1] = 0x03;
			regAdd[2] = 0x20;

			info->fts_write_reg(info, &regAdd[0], 3);
		} else {
			fts_reinit(info, false);
		}
	}
	snprintf(buff, sizeof(buff), "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_sram_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN];
	u8 regAdd;
	u8 data[FTS_EVENT_SIZE];
	int rc, retry = 0;

	sec_cmd_set_default_result(sec);

	info->fts_systemreset(info, 0);

	fts_interrupt_set(info, INT_DISABLE);
	mutex_lock(&info->wait_for);

	regAdd = FTS_CMD_RUN_SRAM_TEST;
	rc = info->fts_write_reg(info, &regAdd, 1);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: failed to write cmd\n", __func__);
		goto error;
	}

	fts_delay(300);

	memset(data, 0x0, FTS_EVENT_SIZE);
	rc = -EIO;
	regAdd = FTS_READ_ONE_EVENT;
	while (info->fts_read_reg(info, &regAdd, 1, data, FTS_EVENT_SIZE) > 0) {
		if (data[0] != 0x00)
			input_info(true, &info->client->dev,
					"%s: event %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X\n",
					__func__, data[0], data[1], data[2], data[3],
					data[4], data[5], data[6], data[7]);

		if (data[0] == FTS_EVENT_PASS_REPORT && data[1] == FTS_EVENT_SRAM_TEST_RESULT) {
			rc = 0; /* PASS */
			break;
		} else if (data[0] == FTS_EVENT_ERROR_REPORT && data[1] == FTS_EVENT_SRAM_TEST_RESULT) {
			rc = 1; /* FAIL */
			break;
		}

		if (retry++ > FTS_RETRY_COUNT * 25) {
			input_err(true, &info->client->dev,
					"%s: Time Over (%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X)\n",
					__func__, data[0], data[1], data[2], data[3],
					data[4], data[5], data[6], data[7]);
			break;
		}
		fts_delay(20);
	}

error:
	mutex_unlock(&info->wait_for);

	if (rc < 0) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%d", rc);
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SRAM");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	fts_reinit(info, false);
	fts_interrupt_set(info, INT_ENABLE);
}

static void set_rear_selfie_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		info->rear_selfie_mode = sec->cmd_param[0];
		snprintf(buff, sizeof(buff), "OK");
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void ear_detect_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 data[2];
	int ret;

	sec_cmd_set_default_result(sec);

	if (!info->board->support_ear_detect || sec->cmd_param[0] < 0 || sec->cmd_param[0] > 3) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		info->ed_enable = sec->cmd_param[0];
		snprintf(buff, sizeof(buff), "OK");

		data[0] = FTS_CMD_SET_EAR_DETECT;
		data[1] = info->ed_enable;

		ret = fts_write_reg(info, data, 2);
		input_info(true, &info->client->dev, "%s: %s, ret = %d\n",
				__func__, info->ed_enable ? "enable" : "disable", ret);
	}

	if (info->board->hw_i2c_reset) {
		cancel_delayed_work_sync(&info->fw_reset_work);
		if (info->ed_enable == 3) {
			info->fw_reset_cmd = 0x01;
		} else {
			info->fw_reset_cmd = 0x00;
		}
		schedule_work(&info->fw_reset_work.work);
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void pocket_mode_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 data[2];
	int ret;

	sec_cmd_set_default_result(sec);

	if (!info->board->support_ear_detect || sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		info->pocket_mode = sec->cmd_param[0];
		snprintf(buff, sizeof(buff), "OK");

		data[0] = FTS_CMD_SET_POCKET_MODE;
		data[1] = info->pocket_mode;

		ret = fts_write_reg(info, data, 2);
		input_info(true, &info->client->dev, "%s: %s, ret = %d\n",
				__func__, info->pocket_mode ? "enable" : "disable", ret);
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void set_sip_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		ret = fts_set_sip_mode(info, (u8)sec->cmd_param[0]);
		if (ret < 0) {
			snprintf(buff, sizeof(buff), "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
		} else {
			snprintf(buff, sizeof(buff), "OK");
			sec->cmd_state = SEC_CMD_STATUS_OK;
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void set_note_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[3];
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		input_err(true, &info->client->dev,
				"%s: wrong param %d\n", __func__, sec->cmd_param[0]);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	regAdd[0] = FTS_CMD_SET_FUNCTION_ONOFF;
	regAdd[1] = FTS_FUNCTION_SET_NOTE_MODE;
	regAdd[2] = sec->cmd_param[0] & 0xFF;

	input_info(true, &info->client->dev, "%s: %s\n",
			__func__, sec->cmd_param[0] ? "enable" : "disable");

	ret = fts_write_reg(info, regAdd, 3);
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);
}

static void set_game_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;
	u8 reg[3] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		reg[0] = FTS_CMD_SET_FUNCTION_ONOFF;
		reg[1] = FTS_FUNCTION_SET_GAME_MODE;
		reg[2] = sec->cmd_param[0];
		ret = fts_write_reg(info, reg, 3);
		if (ret < 0) {
			snprintf(buff, sizeof(buff), "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
		} else {
			snprintf(buff, sizeof(buff), "OK");
			sec->cmd_state = SEC_CMD_STATUS_OK;
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}
#endif

