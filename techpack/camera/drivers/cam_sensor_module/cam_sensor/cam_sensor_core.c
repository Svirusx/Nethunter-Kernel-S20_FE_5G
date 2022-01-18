// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
 */

#include <linux/module.h>
#include <cam_sensor_cmn_header.h>
#include "cam_sensor_core.h"
#include "cam_sensor_util.h"
#include "cam_soc_util.h"
#include "cam_trace.h"
#include "cam_common_util.h"
#if defined(CONFIG_SENSOR_RETENTION)
#include "cam_sensor_retention.h"
#endif
#include "cam_packet_util.h"

#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
#include "cam_sensor_mipi.h"
#endif
#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
#include "cam_sensor_dram.h"
#endif

#if defined(CONFIG_CAMERA_FRAME_CNT_DBG)
static int frame_cnt_dbg;
module_param(frame_cnt_dbg, int, 0644);

#include "cam_sensor_thread.h"
#include <linux/slab.h>
#endif

#if defined(CONFIG_CAMERA_SSM_I2C_ENV)
struct cam_sensor_ctrl_t *g_s_ctrl_ssm;
#endif

#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
long rear_frs_test_mode;
#endif

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
//#define HWB_FILE_OPERATION 1
uint32_t sec_sensor_position;
uint32_t sec_sensor_clk_size;

static struct cam_hw_param_collector cam_hwparam_collector;
#endif

#if defined(CONFIG_SENSOR_RETENTION)
uint8_t sensor_retention_mode = RETENTION_INIT;
#endif
#if defined(CONFIG_SAMSUNG_REAR_TOF) || defined(CONFIG_SAMSUNG_FRONT_TOF) || defined(CONFIG_SEC_C2Q_PROJECT)
struct cam_sensor_ctrl_t *g_s_ctrl_tof;
int check_pd_ready;
char tof_freq[10] = "\n";
#define CRITERION_REV	(6)
static unsigned int system_rev __read_mostly;
static int __init sec_hw_rev_setup(char *p)
{
	int ret;

	ret = kstrtouint(p, 0, &system_rev);

	if (unlikely(ret < 0)) {

		pr_warn("androidboot.revision is malformed (%s)\n", p);

		return -EINVAL;
	}

	pr_info("androidboot.revision %x\n", system_rev);

	return 0;
}

early_param("androidboot.revision", sec_hw_rev_setup);

static unsigned int sec_hw_rev(void)
{
	return system_rev;
}
#endif

#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
int32_t cam_check_stream_on(
	struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t ret = 0;
	uint16_t sensor_id = 0;

#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
	if (rear_frs_test_mode >= 1) {
		CAM_ERR(CAM_SENSOR, "[FRS_DBG] No DYNAMIC_MIPI, rear_frs_test_mode : %ld", rear_frs_test_mode);
		return ret;
	}
#endif

	sensor_id = s_ctrl->sensordata->slave_info.sensor_id;
	switch (sensor_id) {
		case FRONT_SENSOR_ID_IMX374:
		case TOF_SENSOR_ID_IMX516:
		case TOF_SENSOR_ID_IMX518:
		case SENSOR_ID_IMX555:
		case SENSOR_ID_IMX586:
		case SENSOR_ID_S5K2LA:
		case SENSOR_ID_S5KGW2:
		case SENSOR_ID_S5KGH1:
		case SENSOR_ID_S5K2L3:
		case SENSOR_ID_S5KHM1:
#if !defined(CONFIG_SEC_F2Q_PROJECT) && !defined(CONFIG_SEC_VICTORY_PROJECT)
		case SENSOR_ID_S5K3M5:
#endif
			ret = 1;
#if defined(CONFIG_SEC_C2Q_PROJECT)
			if ((sensor_id == SENSOR_ID_S5K3M5) &&
				(sec_hw_rev() < 6))
				ret = 0;
#endif
			break;
		default:
			ret =0;
			break;
	}

#ifdef CONFIG_SEC_FACTORY
	ret = 0;
#endif

	return ret;
}

int cam_sensor_apply_adaptive_mipi_settings(struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	const struct cam_mipi_sensor_mode *cur_mipi_sensor_mode;
	struct i2c_settings_list mipi_i2c_list;
	uint16_t sensor_id = 0;

	sensor_id = s_ctrl->sensordata->slave_info.sensor_id;

	if (cam_check_stream_on(s_ctrl))
	{
#if defined(CONFIG_SAMSUNG_REAR_TOF) || defined(CONFIG_SAMSUNG_FRONT_TOF)
		int isToFSensor = 0;
		if ((sensor_id == TOF_SENSOR_ID_IMX516) ||
			(sensor_id == TOF_SENSOR_ID_IMX518))
			isToFSensor = 1;

		if ((isToFSensor == 0) ||
			((isToFSensor == 1) &&
			(strcmp(tof_freq, "0") == 0)))
#endif
		{
#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
			if (rear_frs_test_mode == 0) {
#endif
			cam_mipi_init_setting(s_ctrl);
			cam_mipi_update_info(s_ctrl);
			cam_mipi_get_clock_string(s_ctrl);
#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
		}
#endif
		}
	}

	if (cam_check_stream_on(s_ctrl)
		&& s_ctrl->mipi_clock_index_new != INVALID_MIPI_INDEX
		&& s_ctrl->i2c_data.streamon_settings.is_settings_valid) {
		CAM_INFO(CAM_SENSOR, "[adaptive_mipi] Write MIPI setting before Stream On setting. mipi_index : %d",
			s_ctrl->mipi_clock_index_new);

#if defined(CONFIG_SAMSUNG_FRONT_TOF) || defined(CONFIG_SAMSUNG_REAR_TOF)
		if (sensor_id == TOF_SENSOR_ID_IMX516 ||
			sensor_id == TOF_SENSOR_ID_IMX518) {
			uint32_t mode = 0;
			rc = camera_io_dev_read(&s_ctrl->io_master_info, 0x080C,
				&mode, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			CAM_INFO(CAM_SENSOR, "[adaptive_mipi] IMX516/IMX518 mode : %d", mode); // 0 : VGA, 1: QVGA, 2 : QQVGA
			if (mode == 1) {
				s_ctrl->mipi_clock_index_new += 3;
			} else if (mode == 2) {
				s_ctrl->mipi_clock_index_new += 6;
			}
		}
#endif
		cur_mipi_sensor_mode = &(s_ctrl->mipi_info[0]);
		memset(&mipi_i2c_list, 0, sizeof(mipi_i2c_list));

		memcpy(&mipi_i2c_list.i2c_settings,
			cur_mipi_sensor_mode->mipi_setting[s_ctrl->mipi_clock_index_new].clk_setting,
			sizeof(struct cam_sensor_i2c_reg_setting));

		CAM_INFO(CAM_SENSOR, "[adaptive_mipi] Picked MIPI clock : %s",
			cur_mipi_sensor_mode->mipi_setting[s_ctrl->mipi_clock_index_new].str_mipi_clk);

		if (mipi_i2c_list.i2c_settings.size > 0)
			rc = camera_io_dev_write(&s_ctrl->io_master_info,
				&(mipi_i2c_list.i2c_settings));
	}

	return rc;
}
#endif

int cam_sensor_read_frame_count(struct cam_sensor_ctrl_t *s_ctrl, uint32_t* frame_cnt)
{
	int rc = 0;
	uint32_t FRAME_COUNT_REG_ADDR = 0x0005;

#if defined(CONFIG_SEC_R8Q_PROJECT)
	if (s_ctrl->sensordata->slave_info.sensor_id == HI847_SENSOR_ID)
		FRAME_COUNT_REG_ADDR = 0x0732;
#endif

	rc = camera_io_dev_read(&s_ctrl->io_master_info, FRAME_COUNT_REG_ADDR,
		frame_cnt, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR, "[CNT_DBG] Failed to read frame_cnt");

	return rc;
}

int cam_sensor_wait_stream_on(struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	uint32_t frame_cnt = 0;
	int retry_cnt = 10;

	CAM_DBG(CAM_SENSOR, "E");

#if defined(CONFIG_SAMSUNG_FRONT_TOF) || defined(CONFIG_SAMSUNG_REAR_TOF)
	if(s_ctrl->sensordata->slave_info.sensor_id != TOF_SENSOR_ID_IMX518) {
#endif
		do {
			rc = cam_sensor_read_frame_count(s_ctrl, &frame_cnt);
			if (rc < 0)
				break;

#if defined(CONFIG_SEC_R8Q_PROJECT)
		if (((s_ctrl->sensordata->slave_info.sensor_id == HI847_SENSOR_ID) && ((frame_cnt & 0x01)  == 0x01))
			|| ((s_ctrl->sensordata->slave_info.sensor_id != HI847_SENSOR_ID) && (frame_cnt != 0xFF)
			&& (frame_cnt > 0))) {

			if (s_ctrl->sensordata->slave_info.sensor_id == HI847_SENSOR_ID)
				usleep_range(4000, 5000);
#else
			if ((frame_cnt != 0xFF) &&	(frame_cnt > 0)) {
#endif
				CAM_INFO(CAM_SENSOR, "[CNT_DBG] 0x%x : Last frame_cnt 0x%x",
					s_ctrl->sensordata->slave_info.sensor_id, frame_cnt);
				return 0;
			}
			CAM_INFO(CAM_SENSOR, "[CNT_DBG] retry cnt : %d, Stream off, frame_cnt : 0x%x", retry_cnt, frame_cnt);
			retry_cnt--;
			usleep_range(5000, 6000);
		} while ((frame_cnt < 0x01 || frame_cnt == 0xFF) && (retry_cnt > 0));

		CAM_INFO(CAM_SENSOR, "[CNT_DBG] wait fail rc %d retry cnt : %d, frame_cnt : 0x%x", rc, retry_cnt, frame_cnt);
#if defined(CONFIG_SAMSUNG_FRONT_TOF) || defined(CONFIG_SAMSUNG_REAR_TOF)
	}
#endif

	CAM_DBG(CAM_SENSOR, "X");

	return -1;
}

int cam_sensor_wait_stream_off(struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	uint32_t frame_cnt = 0;
	int retry_cnt = 30;
#if defined(CONFIG_SEC_Z3Q_PROJECT) || defined(CONFIG_SEC_C2Q_PROJECT)
	retry_cnt = 60;
#endif

	CAM_DBG(CAM_SENSOR, "E");

#if defined(CONFIG_SAMSUNG_FRONT_TOF) || defined(CONFIG_SAMSUNG_REAR_TOF)
	if(s_ctrl->sensordata->slave_info.sensor_id != TOF_SENSOR_ID_IMX518) {
#endif
		usleep_range(2000, 3000);
		do {
			rc = cam_sensor_read_frame_count(s_ctrl, &frame_cnt);
			if (rc < 0)
				break;

#if defined(CONFIG_SEC_R8Q_PROJECT)
			if (((s_ctrl->sensordata->slave_info.sensor_id == HI847_SENSOR_ID) && ((frame_cnt & 0x01)  == 0x00))
				|| ((s_ctrl->sensordata->slave_info.sensor_id != HI847_SENSOR_ID) && (frame_cnt == 0xFF))) {

				if (s_ctrl->sensordata->slave_info.sensor_id == HI847_SENSOR_ID)
					usleep_range(1000, 1010);

				 return 0;
			}
#else
  			if (frame_cnt == 0xFF)
				return 0;
#endif
			CAM_INFO(CAM_SENSOR, "[CNT_DBG] retry cnt : %d, Stream off, frame_cnt : 0x%x", retry_cnt, frame_cnt);
			retry_cnt--;
			usleep_range(5000, 6000);
		} while ((frame_cnt != 0xFF) && (retry_cnt > 0));

		CAM_INFO(CAM_SENSOR, "[CNT_DBG] wait fail rc %d retry cnt : %d, frame_cnt : 0x%x", rc, retry_cnt, frame_cnt);
#if defined(CONFIG_SAMSUNG_FRONT_TOF) || defined(CONFIG_SAMSUNG_REAR_TOF)
	}
#endif

	CAM_DBG(CAM_SENSOR, "X");
	return -1;
}

static void cam_sensor_update_req_mgr(
	struct cam_sensor_ctrl_t *s_ctrl,
	struct cam_packet *csl_packet)
{
	struct cam_req_mgr_add_request add_req;
#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
	int rc = 0;
	uint32_t frame_cnt = 0;
#endif

	add_req.link_hdl = s_ctrl->bridge_intf.link_hdl;
	add_req.req_id = csl_packet->header.request_id;
	CAM_DBG(CAM_SENSOR, " Rxed Req Id: %lld",
		csl_packet->header.request_id);

#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
	// read frame count
	if (rear_frs_test_mode >= 1) {
		rc = camera_io_dev_read(&s_ctrl->io_master_info, 0x0005,
			&frame_cnt, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "[FRS_DBG] Failed to read frame_cnt");
		}
		else {
			// CAM_INFO(CAM_SENSOR, "[FRS_DBG] frame_cnt 0x%x", frame_cnt);
			if (((rear_frs_test_mode == 1) && ((frame_cnt != 0xff) && (frame_cnt >= 0x5)))
				|| ((rear_frs_test_mode == 5) && ((frame_cnt != 0xff) && (frame_cnt >= 0x5)))) {
				CAM_INFO(CAM_SENSOR, "[FRS_DBG] frame_cnt 0x%x, STEP#%d E", frame_cnt, rear_frs_test_mode);
				rear_frs_test_mode++;
				CAM_INFO(CAM_SENSOR, "[FRS_DBG] frame_cnt 0x%x, STEP#%d X", frame_cnt, rear_frs_test_mode);
			}
		}
	}
#endif

	add_req.dev_hdl = s_ctrl->bridge_intf.device_hdl;
	add_req.skip_before_applying = 0;
	if (s_ctrl->bridge_intf.crm_cb &&
		s_ctrl->bridge_intf.crm_cb->add_req)
		s_ctrl->bridge_intf.crm_cb->add_req(&add_req);

	CAM_DBG(CAM_SENSOR, " add req to req mgr: %lld",
			add_req.req_id);
}

static void cam_sensor_release_stream_rsc(
	struct cam_sensor_ctrl_t *s_ctrl)
{
	struct i2c_settings_array *i2c_set = NULL;
	int rc;

	i2c_set = &(s_ctrl->i2c_data.streamoff_settings);
	if (i2c_set->is_settings_valid == 1) {
		i2c_set->is_settings_valid = -1;
		rc = delete_request(i2c_set);
		if (rc < 0)
			CAM_ERR(CAM_SENSOR,
				"failed while deleting Streamoff settings");
	}

	i2c_set = &(s_ctrl->i2c_data.streamon_settings);
	if (i2c_set->is_settings_valid == 1) {
		i2c_set->is_settings_valid = -1;
		rc = delete_request(i2c_set);
		if (rc < 0)
			CAM_ERR(CAM_SENSOR,
				"failed while deleting Streamon settings");
	}
}

static void cam_sensor_release_per_frame_resource(
	struct cam_sensor_ctrl_t *s_ctrl)
{
	struct i2c_settings_array *i2c_set = NULL;
	int i, rc;

	if (s_ctrl->i2c_data.per_frame != NULL) {
		for (i = 0; i < MAX_PER_FRAME_ARRAY; i++) {
			i2c_set = &(s_ctrl->i2c_data.per_frame[i]);
			if (i2c_set->is_settings_valid == 1) {
				i2c_set->is_settings_valid = -1;
				rc = delete_request(i2c_set);
				if (rc < 0)
					CAM_ERR(CAM_SENSOR,
						"delete request: %lld rc: %d",
						i2c_set->request_id, rc);
			}
		}
	}
}

static int32_t cam_sensor_i2c_pkt_parse(struct cam_sensor_ctrl_t *s_ctrl,
	void *arg)
{
	int32_t rc = 0;
	uintptr_t generic_ptr;
	struct cam_control *ioctl_ctrl = NULL;
	struct cam_packet *csl_packet = NULL;
	struct cam_cmd_buf_desc *cmd_desc = NULL;
	struct i2c_settings_array *i2c_reg_settings = NULL;
	size_t len_of_buff = 0;
	size_t remain_len = 0;
	uint32_t *offset = NULL;
	struct cam_config_dev_cmd config;
	struct i2c_data_settings *i2c_data = NULL;

	ioctl_ctrl = (struct cam_control *)arg;

	if (ioctl_ctrl->handle_type != CAM_HANDLE_USER_POINTER) {
		CAM_ERR(CAM_SENSOR, "Invalid Handle Type");
		return -EINVAL;
	}

	if (copy_from_user(&config,
		u64_to_user_ptr(ioctl_ctrl->handle),
		sizeof(config)))
		return -EFAULT;

	rc = cam_mem_get_cpu_buf(
		config.packet_handle,
		&generic_ptr,
		&len_of_buff);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Failed in getting the packet: %d", rc);
		return rc;
	}

	remain_len = len_of_buff;
	if ((sizeof(struct cam_packet) > len_of_buff) ||
		((size_t)config.offset >= len_of_buff -
		sizeof(struct cam_packet))) {
		CAM_ERR(CAM_SENSOR,
			"Inval cam_packet strut size: %zu, len_of_buff: %zu",
			 sizeof(struct cam_packet), len_of_buff);
		rc = -EINVAL;
		goto end;
	}

	remain_len -= (size_t)config.offset;
	csl_packet = (struct cam_packet *)(generic_ptr +
		(uint32_t)config.offset);

	if (cam_packet_util_validate_packet(csl_packet,
		remain_len)) {
		CAM_ERR(CAM_SENSOR, "Invalid packet params");
		rc = -EINVAL;
		goto end;

	}

	if ((csl_packet->header.op_code & 0xFFFFFF) !=
		CAM_SENSOR_PACKET_OPCODE_SENSOR_INITIAL_CONFIG &&
		csl_packet->header.request_id <= s_ctrl->last_flush_req
		&& s_ctrl->last_flush_req != 0) {
		CAM_ERR(CAM_SENSOR,
			"reject request %lld, last request to flush %u",
			csl_packet->header.request_id, s_ctrl->last_flush_req);
		rc = -EINVAL;
		goto end;
	}

	if (csl_packet->header.request_id > s_ctrl->last_flush_req)
		s_ctrl->last_flush_req = 0;

	i2c_data = &(s_ctrl->i2c_data);
	CAM_DBG(CAM_SENSOR, "Header OpCode: %d", csl_packet->header.op_code);
	switch (csl_packet->header.op_code & 0xFFFFFF) {
	case CAM_SENSOR_PACKET_OPCODE_SENSOR_INITIAL_CONFIG: {
		i2c_reg_settings = &i2c_data->init_settings;
		i2c_reg_settings->request_id = 0;
		i2c_reg_settings->is_settings_valid = 1;
		break;
	}
	case CAM_SENSOR_PACKET_OPCODE_SENSOR_CONFIG: {
#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
		if (rear_frs_test_mode >= 1) {
			CAM_ERR(CAM_SENSOR, "[FRS_DBG] SENSOR_CONFIG skip on test_mode(%ld)", rear_frs_test_mode);
			return 0;
		}
#endif
		i2c_reg_settings = &i2c_data->config_settings;
		i2c_reg_settings->request_id = 0;
		i2c_reg_settings->is_settings_valid = 1;
		break;
	}
	case CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMON: {
		if (s_ctrl->streamon_count > 0)
			goto end;

		s_ctrl->streamon_count = s_ctrl->streamon_count + 1;
		i2c_reg_settings = &i2c_data->streamon_settings;
		i2c_reg_settings->request_id = 0;
		i2c_reg_settings->is_settings_valid = 1;
		break;
	}
	case CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMOFF: {
		if (s_ctrl->streamoff_count > 0)
			goto end;

		s_ctrl->streamoff_count = s_ctrl->streamoff_count + 1;
		i2c_reg_settings = &i2c_data->streamoff_settings;
		i2c_reg_settings->request_id = 0;
		i2c_reg_settings->is_settings_valid = 1;
		break;
	}

	case CAM_SENSOR_PACKET_OPCODE_SENSOR_MODE: {
#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
		CAM_INFO(CAM_SENSOR, "[adaptive_mipi] SENSOR_MODE : %d", csl_packet->header.request_id);
		s_ctrl->sensor_mode = csl_packet->header.request_id;
#endif
		break;
	}

	case CAM_SENSOR_PACKET_OPCODE_SENSOR_UPDATE: {
		if ((s_ctrl->sensor_state == CAM_SENSOR_INIT) ||
			(s_ctrl->sensor_state == CAM_SENSOR_ACQUIRE)) {
			CAM_WARN(CAM_SENSOR,
				"Rxed Update packets without linking");
			goto end;
		}

		i2c_reg_settings =
			&i2c_data->per_frame[csl_packet->header.request_id %
				MAX_PER_FRAME_ARRAY];
		CAM_DBG(CAM_SENSOR, "Received Packet: %lld req: %lld",
			csl_packet->header.request_id % MAX_PER_FRAME_ARRAY,
			csl_packet->header.request_id);
		if (i2c_reg_settings->is_settings_valid == 1) {
			CAM_ERR(CAM_SENSOR,
				"Already some pkt in offset req : %lld",
				csl_packet->header.request_id);
			/*
			 * Update req mgr even in case of failure.
			 * This will help not to wait indefinitely
			 * and freeze. If this log is triggered then
			 * fix it.
			 */
			cam_sensor_update_req_mgr(s_ctrl, csl_packet);
			goto end;
		}
		break;
	}
	case CAM_SENSOR_PACKET_OPCODE_SENSOR_NOP: {
		if ((s_ctrl->sensor_state == CAM_SENSOR_INIT) ||
			(s_ctrl->sensor_state == CAM_SENSOR_ACQUIRE)) {
			CAM_WARN(CAM_SENSOR,
				"Rxed NOP packets without linking");
			goto end;
		}

		cam_sensor_update_req_mgr(s_ctrl, csl_packet);
		goto end;
	}
	default:
		CAM_ERR(CAM_SENSOR, "Invalid Packet Header");
		rc = -EINVAL;
		goto end;
	}

	offset = (uint32_t *)&csl_packet->payload;
	offset += csl_packet->cmd_buf_offset / 4;
	cmd_desc = (struct cam_cmd_buf_desc *)(offset);

	rc = cam_sensor_i2c_command_parser(&s_ctrl->io_master_info,
			i2c_reg_settings, cmd_desc, 1);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Fail parsing I2C Pkt: %d", rc);
		goto end;
	}

	if ((csl_packet->header.op_code & 0xFFFFFF) ==
		CAM_SENSOR_PACKET_OPCODE_SENSOR_UPDATE) {
		i2c_reg_settings->request_id =
			csl_packet->header.request_id;
		cam_sensor_update_req_mgr(s_ctrl, csl_packet);
	}

end:
	return rc;
}

#if defined(CONFIG_SENSOR_RETENTION)
uint32_t cam_sensor_retention_calc_checksum(struct camera_io_master *io_master_info)
{
	uint32_t read_value = 0xBEEF;
	uint8_t read_cnt = 0;

	CAM_INFO(CAM_SENSOR, "[RET_DBG] cam_sensor_retention_calc_checksum");

	for (read_cnt = 0; read_cnt < SENSOR_RETENTION_READ_RETRY_CNT; read_cnt++) {
		// 1. Wait - 6ms delay
		usleep_range(6000, 7000);

		// 2. Check result for checksum test - read addr: 0x100E
		camera_io_dev_read(io_master_info, 0x100E, &read_value,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

		if (read_value == 0x01) {
			CAM_INFO(CAM_SENSOR, "[RET_DBG] Pass checksum test");
			break;
		} else {
			CAM_ERR(CAM_SENSOR, "[RET_DBG] Fail checksum test retry : %d", read_cnt);
		}
	}

	if ((read_cnt == SENSOR_RETENTION_READ_RETRY_CNT) && (read_value != 0x01)) {
		CAM_ERR(CAM_SENSOR, "[RET_DBG] Fail checksum test!");
		return 0;
	}

	return read_value;
}

void cam_sensor_write_normal_init(struct camera_io_master *io_master_info)
{
	int32_t rc = 0;
	struct cam_sensor_i2c_reg_setting  i2c_reg_settings;


	CAM_INFO(CAM_SENSOR, "[RET_DBG] cam_sensor_write_normal_init");


	{
		CAM_INFO(CAM_SENSOR, "[RET_DBG] winner flip setting");

		//write reset setting
		i2c_reg_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
		i2c_reg_settings.data_type = CAMERA_SENSOR_I2C_TYPE_WORD;
		i2c_reg_settings.delay = 0;
		i2c_reg_settings.size = sizeof(retention_reset_reg_array)/sizeof(struct cam_sensor_i2c_reg_array);
		i2c_reg_settings.reg_setting = retention_reset_reg_array;

		rc = camera_io_dev_write(io_master_info,
			&i2c_reg_settings);

		//add 5ms delay
		usleep_range(5000, 6000);

		//write tnp setting
		i2c_reg_settings.size = sizeof(retention_tnp_reg_array)/sizeof(struct cam_sensor_i2c_reg_array);
		i2c_reg_settings.reg_setting = retention_tnp_reg_array;

		rc = camera_io_dev_write(io_master_info,
			&i2c_reg_settings);

		//write tnp burst setting
		i2c_reg_settings.size = sizeof(retention_tnp_burst_reg_array)/sizeof(struct cam_sensor_i2c_reg_array);
		i2c_reg_settings.reg_setting = retention_tnp_burst_reg_array;

		rc = camera_io_dev_write_continuous(
			io_master_info,
			&i2c_reg_settings,
			1);

		//write global setting
		i2c_reg_settings.size = sizeof(retention_global_reg_array_rev1)/sizeof(struct cam_sensor_i2c_reg_array);
		i2c_reg_settings.reg_setting = retention_global_reg_array_rev1;

		rc = camera_io_dev_write(io_master_info,
			&i2c_reg_settings);

		//write sram1 setting
		i2c_reg_settings.size = sizeof(retention_sram1_full_30_reg_array)/sizeof(struct cam_sensor_i2c_reg_array);
		i2c_reg_settings.reg_setting = retention_sram1_full_30_reg_array;

		rc = camera_io_dev_write(io_master_info,
			&i2c_reg_settings);

		//write sram1 burst setting
		i2c_reg_settings.size = sizeof(retention_sram1_full_30_burst_reg_array_rev1)/sizeof(struct cam_sensor_i2c_reg_array);
		i2c_reg_settings.reg_setting = retention_sram1_full_30_burst_reg_array_rev1;

		rc = camera_io_dev_write_continuous(
			io_master_info,
			&i2c_reg_settings,
			1);

		//write sram2 setting
		i2c_reg_settings.size = sizeof(retention_sram2_4k2k_30_reg_array)/sizeof(struct cam_sensor_i2c_reg_array);
		i2c_reg_settings.reg_setting = retention_sram2_4k2k_30_reg_array;

		rc = camera_io_dev_write(io_master_info,
			&i2c_reg_settings);

		//write sram2 burst setting
		i2c_reg_settings.size = sizeof(retention_sram2_4k2k_30_burst_reg_array_rev1)/sizeof(struct cam_sensor_i2c_reg_array);
		i2c_reg_settings.reg_setting = retention_sram2_4k2k_30_burst_reg_array_rev1;

		rc = camera_io_dev_write_continuous(
			io_master_info,
			&i2c_reg_settings,
			1);

		//write sram3 setting
		i2c_reg_settings.size = sizeof(retention_sram3_4k2k_60_reg_array)/sizeof(struct cam_sensor_i2c_reg_array);
		i2c_reg_settings.reg_setting = retention_sram3_4k2k_60_reg_array;

		rc = camera_io_dev_write(io_master_info,
			&i2c_reg_settings);

		//write sram3 burst setting
		i2c_reg_settings.size = sizeof(retention_sram3_4k2k_60_burst_reg_array_rev1)/sizeof(struct cam_sensor_i2c_reg_array);
		i2c_reg_settings.reg_setting = retention_sram3_4k2k_60_burst_reg_array_rev1;

		rc = camera_io_dev_write_continuous(
			io_master_info,
			&i2c_reg_settings,
			1);

		//write sram4 setting
		i2c_reg_settings.size = sizeof(retention_sram4_wvga_120_reg_array)/sizeof(struct cam_sensor_i2c_reg_array);
		i2c_reg_settings.reg_setting = retention_sram4_wvga_120_reg_array;

		rc = camera_io_dev_write(io_master_info,
			&i2c_reg_settings);

		//write sram4 burst setting
		i2c_reg_settings.size = sizeof(retention_sram4_wvga_120_burst_reg_array_rev1)/sizeof(struct cam_sensor_i2c_reg_array);
		i2c_reg_settings.reg_setting = retention_sram4_wvga_120_burst_reg_array_rev1;

		rc = camera_io_dev_write_continuous(
			io_master_info,
			&i2c_reg_settings,
			1);

		//write initial boot setting
		i2c_reg_settings.size = sizeof(retention_initial_boot_reg_array_rev1)/sizeof(struct cam_sensor_i2c_reg_array);
		i2c_reg_settings.reg_setting = retention_initial_boot_reg_array_rev1;

		rc = camera_io_dev_write(io_master_info,
			&i2c_reg_settings);
	}
}

#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
void cam_sensor_write_dram_init(struct camera_io_master *io_master_info)
{
	int32_t rc = 0;
	struct cam_sensor_i2c_reg_setting i2c_reg_settings;

	CAM_INFO(CAM_SENSOR, "[FRS_DBG] cam_sensor_write_dram_init E");

	i2c_reg_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	i2c_reg_settings.data_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	i2c_reg_settings.delay = 0;

	//write TnP0_0 setting
	i2c_reg_settings.size = sizeof(dram_mode_full0_0_reg_array)/sizeof(struct cam_sensor_i2c_reg_array);
	i2c_reg_settings.reg_setting = dram_mode_full0_0_reg_array;

	CAM_DBG(CAM_SENSOR, "[FRS_DBG] Write I2C(TnP0_0) E");
	rc = camera_io_dev_write(io_master_info,
		&i2c_reg_settings);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "[FRS_DBG] Fail write I2C(TnP0_0)");
	}
	CAM_DBG(CAM_SENSOR, "[FRS_DBG] Write I2C(TnP0_0) X");

	//add 5ms delay
	usleep_range(5000, 6000);

	//write TnP0_1 setting
	i2c_reg_settings.size = sizeof(dram_mode_full0_1_reg_array)/sizeof(struct cam_sensor_i2c_reg_array);
	i2c_reg_settings.reg_setting = dram_mode_full0_1_reg_array;

	CAM_DBG(CAM_SENSOR, "[FRS_DBG] Write I2C(TnP0_1) E");
	rc = camera_io_dev_write(io_master_info,
		&i2c_reg_settings);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "[FRS_DBG] Fail write I2C(TnP0_1)");
	}
	CAM_DBG(CAM_SENSOR, "[FRS_DBG] Write I2C(TnP0_1) X");

	//write TnP0_2 burst setting
	i2c_reg_settings.size = sizeof(dram_mode_full0_2_burst_reg_array)/sizeof(struct cam_sensor_i2c_reg_array);
	i2c_reg_settings.reg_setting = dram_mode_full0_2_burst_reg_array;

	CAM_DBG(CAM_SENSOR, "[FRS_DBG] Write I2C(TnP0_2) E");
	rc = camera_io_dev_write_continuous(
		io_master_info,
		&i2c_reg_settings,
		1);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "[FRS_DBG] Fail write I2C(TnP0_2)");
	}
	CAM_DBG(CAM_SENSOR, "[FRS_DBG] Write I2C(TnP0_2) X");

	//write Global setting
	i2c_reg_settings.size = sizeof(dram_mode_full1_0_reg_array)/sizeof(struct cam_sensor_i2c_reg_array);
	i2c_reg_settings.reg_setting = dram_mode_full1_0_reg_array;

	CAM_DBG(CAM_SENSOR, "[FRS_DBG] Write I2C(Global) E");
	rc = camera_io_dev_write(io_master_info,
		&i2c_reg_settings);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "[FRS_DBG] Fail write I2C(Global)");
	}
	CAM_DBG(CAM_SENSOR, "[FRS_DBG] Write I2C(Global) X");

	//write A_AEB setting
	CAM_DBG(CAM_SENSOR, "[FRS_DBG] Write I2C(A_AEB) E");
	i2c_reg_settings.size = sizeof(dram_mode_full2_0_reg_array)/sizeof(struct cam_sensor_i2c_reg_array);
	i2c_reg_settings.reg_setting = dram_mode_full2_0_reg_array;

	rc = camera_io_dev_write(io_master_info,
		&i2c_reg_settings);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "[FRS_DBG] Fail write I2C(A_AEB)");
	}
	CAM_DBG(CAM_SENSOR, "[FRS_DBG] Write I2C(A_AEB) X");

	CAM_INFO(CAM_SENSOR, "[FRS_DBG] cam_sensor_write_dram_init X");
}
#endif

void cam_sensor_write_enable_crc(struct camera_io_master *io_master_info)
{
	int32_t rc = 0;
	struct cam_sensor_i2c_reg_setting  i2c_reg_settings;
	struct cam_sensor_i2c_reg_array    i2c_reg_array;

	CAM_INFO(CAM_SENSOR, "[RET_DBG] cam_sensor_write_enable_crc");

	i2c_reg_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	i2c_reg_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_settings.size = 1;
	i2c_reg_settings.delay = 0;
	i2c_reg_array.reg_addr = 0x010E;
	i2c_reg_array.reg_data = 0x1;
	i2c_reg_array.delay = 0;
	i2c_reg_array.data_mask = 0x0;
	i2c_reg_settings.reg_setting = &i2c_reg_array;

	rc = camera_io_dev_write(io_master_info,
		&i2c_reg_settings);
}

int cam_sensor_apply_retention_init_settings(struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	uint32_t read_value = 0xBEEF;
	struct camera_io_master *io_master_info = NULL;

	io_master_info = &s_ctrl->io_master_info;

	if (s_ctrl->sensordata->slave_info.sensor_id == RETENTION_SENSOR_ID) {
#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
		CAM_ERR(CAM_SENSOR, "[FRS_DBG] rear_frs_test_mode : %ld", rear_frs_test_mode);

		if (rear_frs_test_mode == 1) {
			CAM_INFO(CAM_SENSOR, "[FRS_DBG] Write Dram initSetting(%ld)", rear_frs_test_mode);
			cam_sensor_write_dram_init(io_master_info);
		}
		else {
#endif
			if (sensor_retention_mode == RETENTION_ON) {
				read_value = cam_sensor_retention_calc_checksum(io_master_info);
				if (read_value == 0x01) {
					CAM_INFO(CAM_SENSOR, "[RET_DBG] Retention wake! Skip to write normal initSetting");
				} else {
					CAM_INFO(CAM_SENSOR, "[RET_DBG] Fail to retention wake! Write normal initSetting");
					cam_sensor_write_normal_init(io_master_info);
				}
			} else {
				cam_sensor_write_normal_init(io_master_info);
			}
#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
		}

		if (rear_frs_test_mode >= 1) {
			CAM_ERR(CAM_SENSOR, "[FRS_DBG] RETENTION_INIT SET(%ld)", rear_frs_test_mode);
			sensor_retention_mode = RETENTION_INIT; //force setting
		}
		else {
			sensor_retention_mode = RETENTION_READY_TO_ON;
		}
#endif
	}
	return rc;
}
#endif

static int32_t cam_sensor_i2c_modes_util(
	struct cam_sensor_ctrl_t *s_ctrl,
	struct camera_io_master *io_master_info,
	struct i2c_settings_list *i2c_list)
{
	int32_t rc = 0;
	uint32_t i, size;

	if (i2c_list->op_code == CAM_SENSOR_I2C_WRITE_RANDOM) {
#if 1
		struct cam_sensor_i2c_reg_array *reg_setting;
		uint32_t i2c_size = 0, org_size = 0, offset = 0;

		if (i2c_list->i2c_settings.size >  CCI_I2C_MAX_WRITE) {
			reg_setting = i2c_list->i2c_settings.reg_setting;
			org_size = i2c_list->i2c_settings.size;

			while(offset < org_size) {
				i2c_list->i2c_settings.reg_setting = reg_setting + offset;
				i2c_size = org_size - offset;
				if (i2c_size > CCI_I2C_MAX_WRITE)
					i2c_size = CCI_I2C_MAX_WRITE - 1;
				i2c_list->i2c_settings.size = i2c_size;
				rc = camera_io_dev_write(io_master_info,
					&(i2c_list->i2c_settings));
				if (rc < 0)
					break;
				offset += i2c_size;
			}
			i2c_list->i2c_settings.reg_setting = reg_setting;
			i2c_list->i2c_settings.size = org_size;
		}
		else
#endif
		rc = camera_io_dev_write(io_master_info,
			&(i2c_list->i2c_settings));
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed to random write I2C settings: %d",
				rc);
			return rc;
		}

#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
		if (i2c_list->i2c_settings.reg_setting[0].reg_addr == STREAM_ON_ADDR
			&& i2c_list->i2c_settings.reg_setting[0].reg_data == 0x0) {
			uint32_t frame_cnt = 0;
			int retry_cnt = 20;
			CAM_INFO(CAM_SENSOR, "[RET_DBG] Stream off");

			rc = camera_io_dev_read(&s_ctrl->io_master_info, 0x0005,
				&frame_cnt, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

			while (frame_cnt != 0xFF && retry_cnt > 0) {
				usleep_range(2000, 3000);
				rc = camera_io_dev_read(&s_ctrl->io_master_info, 0x0005,
					&frame_cnt, CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
				CAM_INFO(CAM_SENSOR, "[RET_DBG] retry cnt : %d, Stream off, frame_cnt : %x", retry_cnt, frame_cnt);
				retry_cnt--;
			}
		} else if (i2c_list->i2c_settings.size >= 3) {
			if (i2c_list->i2c_settings.reg_setting[2].reg_addr == STREAM_ON_ADDR
				&& (i2c_list->i2c_settings.reg_setting[2].reg_data == 0x103
				|| i2c_list->i2c_settings.reg_setting[2].reg_data == 0x100)
				&& s_ctrl->sensordata->slave_info.sensor_id == RETENTION_SENSOR_ID) {
				CAM_INFO(CAM_SENSOR, "[RET_DBG] Stream on");
			}
		}
#endif
	} else if (i2c_list->op_code == CAM_SENSOR_I2C_WRITE_SEQ) {
		rc = camera_io_dev_write_continuous(
			io_master_info,
			&(i2c_list->i2c_settings),
			0);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed to seq write I2C settings: %d",
				rc);
			return rc;
		}
	} else if (i2c_list->op_code == CAM_SENSOR_I2C_WRITE_BURST) {
		rc = camera_io_dev_write_continuous(
			io_master_info,
			&(i2c_list->i2c_settings),
			1);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed to burst write I2C settings: %d",
				rc);
			return rc;
		}
	} else if (i2c_list->op_code == CAM_SENSOR_I2C_POLL) {
		size = i2c_list->i2c_settings.size;
		for (i = 0; i < size; i++) {
			rc = camera_io_dev_poll(
			io_master_info,
			i2c_list->i2c_settings.reg_setting[i].reg_addr,
			i2c_list->i2c_settings.reg_setting[i].reg_data,
			i2c_list->i2c_settings.reg_setting[i].data_mask,
			i2c_list->i2c_settings.addr_type,
			i2c_list->i2c_settings.data_type,
			i2c_list->i2c_settings.reg_setting[i].delay);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"i2c poll apply setting Fail: %d", rc);
				return rc;
			}
		}
	}

	return rc;
}

int32_t cam_sensor_update_i2c_info(struct cam_cmd_i2c_info *i2c_info,
	struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct cam_sensor_cci_client   *cci_client = NULL;
#if defined(CONFIG_SAMSUNG_FRONT_TOF) || defined(CONFIG_SAMSUNG_REAR_TOF)
	struct i2c_client *client;
#endif

	if (s_ctrl->io_master_info.master_type == CCI_MASTER) {
		cci_client = s_ctrl->io_master_info.cci_client;
		if (!cci_client) {
			CAM_ERR(CAM_SENSOR, "failed: cci_client %pK",
				cci_client);
			return -EINVAL;
		}
		cci_client->cci_i2c_master = s_ctrl->cci_i2c_master;
		cci_client->sid = i2c_info->slave_addr >> 1;
		cci_client->retries = 3;
		cci_client->id_map = 0;
		cci_client->i2c_freq_mode = i2c_info->i2c_freq_mode;
		CAM_DBG(CAM_SENSOR, " Master: %d sid: %d freq_mode: %d",
			cci_client->cci_i2c_master, i2c_info->slave_addr,
			i2c_info->i2c_freq_mode);
	}
#if defined(CONFIG_SAMSUNG_FRONT_TOF) || defined(CONFIG_SAMSUNG_REAR_TOF)
	else if (s_ctrl->io_master_info.master_type == I2C_MASTER) {
		client = s_ctrl->io_master_info.client;
		if (!client) {
			CAM_ERR(CAM_SENSOR, "failed: i2c client %pK",
				client);
			return -EINVAL;
		}
		client->addr = i2c_info->slave_addr;
		CAM_DBG(CAM_SENSOR, "slave addr 0x%x",
			client->addr);
	}
#endif

	s_ctrl->sensordata->slave_info.sensor_slave_addr =
		i2c_info->slave_addr;
	return rc;
}

int32_t cam_sensor_update_slave_info(struct cam_cmd_probe *probe_info,
	struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;

	s_ctrl->sensordata->slave_info.sensor_id_reg_addr =
		probe_info->reg_addr;
	s_ctrl->sensordata->slave_info.sensor_id =
		probe_info->expected_data;
	s_ctrl->sensordata->slave_info.sensor_id_mask =
		probe_info->data_mask;
	/* Userspace passes the pipeline delay in reserved field */
	s_ctrl->pipeline_delay =
		probe_info->reserved;

	s_ctrl->sensor_probe_addr_type =  probe_info->addr_type;
	s_ctrl->sensor_probe_data_type =  probe_info->data_type;
	CAM_DBG(CAM_SENSOR,
		"Sensor Addr: 0x%x sensor_id: 0x%x sensor_mask: 0x%x sensor_pipeline_delay:0x%x",
		s_ctrl->sensordata->slave_info.sensor_id_reg_addr,
		s_ctrl->sensordata->slave_info.sensor_id,
		s_ctrl->sensordata->slave_info.sensor_id_mask,
		s_ctrl->pipeline_delay);
	return rc;
}

int32_t cam_handle_cmd_buffers_for_probe(void *cmd_buf,
	struct cam_sensor_ctrl_t *s_ctrl,
	int32_t cmd_buf_num, uint32_t cmd_buf_length, size_t remain_len)
{
	int32_t rc = 0;

	switch (cmd_buf_num) {
	case 0: {
		struct cam_cmd_i2c_info *i2c_info = NULL;
		struct cam_cmd_probe *probe_info;

		if (remain_len <
			(sizeof(struct cam_cmd_i2c_info) +
			sizeof(struct cam_cmd_probe))) {
			CAM_ERR(CAM_SENSOR,
				"not enough buffer for cam_cmd_i2c_info");
			return -EINVAL;
		}
		i2c_info = (struct cam_cmd_i2c_info *)cmd_buf;
		rc = cam_sensor_update_i2c_info(i2c_info, s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "Failed in Updating the i2c Info");
			return rc;
		}
		probe_info = (struct cam_cmd_probe *)
			(cmd_buf + sizeof(struct cam_cmd_i2c_info));
		rc = cam_sensor_update_slave_info(probe_info, s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "Updating the slave Info");
			return rc;
		}
		cmd_buf = probe_info;
	}
		break;
	case 1: {
		rc = cam_sensor_update_power_settings(cmd_buf,
			cmd_buf_length, &s_ctrl->sensordata->power_info,
			remain_len);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed in updating power settings");
			return rc;
		}
	}
		break;
	default:
		CAM_ERR(CAM_SENSOR, "Invalid command buffer");
		break;
	}
	return rc;
}

int32_t cam_handle_mem_ptr(uint64_t handle, struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0, i;
	uint32_t *cmd_buf;
	void *ptr;
	size_t len;
	struct cam_packet *pkt = NULL;
	struct cam_cmd_buf_desc *cmd_desc = NULL;
	uintptr_t cmd_buf1 = 0;
	uintptr_t packet = 0;
	size_t    remain_len = 0;

	rc = cam_mem_get_cpu_buf(handle,
		&packet, &len);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Failed to get the command Buffer");
		return -EINVAL;
	}

	pkt = (struct cam_packet *)packet;
	if (pkt == NULL) {
		CAM_ERR(CAM_SENSOR, "packet pos is invalid");
		rc = -EINVAL;
		goto end;
	}

	if ((len < sizeof(struct cam_packet)) ||
		(pkt->cmd_buf_offset >= (len - sizeof(struct cam_packet)))) {
		CAM_ERR(CAM_SENSOR, "Not enough buf provided");
		rc = -EINVAL;
		goto end;
	}

	cmd_desc = (struct cam_cmd_buf_desc *)
		((uint32_t *)&pkt->payload + pkt->cmd_buf_offset/4);
	if (cmd_desc == NULL) {
		CAM_ERR(CAM_SENSOR, "command descriptor pos is invalid");
		rc = -EINVAL;
		goto end;
	}
	if (pkt->num_cmd_buf != 2) {
		CAM_ERR(CAM_SENSOR, "Expected More Command Buffers : %d",
			 pkt->num_cmd_buf);
		rc = -EINVAL;
		goto end;
	}

	for (i = 0; i < pkt->num_cmd_buf; i++) {
		if (!(cmd_desc[i].length))
			continue;
		rc = cam_mem_get_cpu_buf(cmd_desc[i].mem_handle,
			&cmd_buf1, &len);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed to parse the command Buffer Header");
			goto end;
		}
		if (cmd_desc[i].offset >= len) {
			CAM_ERR(CAM_SENSOR,
				"offset past length of buffer");
			rc = -EINVAL;
			goto end;
		}
		remain_len = len - cmd_desc[i].offset;
		if (cmd_desc[i].length > remain_len) {
			CAM_ERR(CAM_SENSOR,
				"Not enough buffer provided for cmd");
			rc = -EINVAL;
			goto end;
		}
		cmd_buf = (uint32_t *)cmd_buf1;
		cmd_buf += cmd_desc[i].offset/4;
		ptr = (void *) cmd_buf;

		rc = cam_handle_cmd_buffers_for_probe(ptr, s_ctrl,
			i, cmd_desc[i].length, remain_len);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed to parse the command Buffer Header");
			goto end;
		}
	}

end:
	return rc;
}

void cam_sensor_query_cap(struct cam_sensor_ctrl_t *s_ctrl,
	struct  cam_sensor_query_cap *query_cap)
{
	query_cap->pos_roll = s_ctrl->sensordata->pos_roll;
	query_cap->pos_pitch = s_ctrl->sensordata->pos_pitch;
	query_cap->pos_yaw = s_ctrl->sensordata->pos_yaw;
	query_cap->secure_camera = 0;
	query_cap->actuator_slot_id =
		s_ctrl->sensordata->subdev_id[SUB_MODULE_ACTUATOR];
	query_cap->csiphy_slot_id =
		s_ctrl->sensordata->subdev_id[SUB_MODULE_CSIPHY];
	query_cap->eeprom_slot_id =
		s_ctrl->sensordata->subdev_id[SUB_MODULE_EEPROM];
	query_cap->flash_slot_id =
		s_ctrl->sensordata->subdev_id[SUB_MODULE_LED_FLASH];
	query_cap->ois_slot_id =
		s_ctrl->sensordata->subdev_id[SUB_MODULE_OIS];
	query_cap->slot_info =
		s_ctrl->soc_info.index;
}

static uint16_t cam_sensor_id_by_mask(struct cam_sensor_ctrl_t *s_ctrl,
	uint32_t chipid)
{
	uint16_t sensor_id = (uint16_t)(chipid & 0xFFFF);
	int16_t sensor_id_mask = s_ctrl->sensordata->slave_info.sensor_id_mask;

	if (!sensor_id_mask)
		sensor_id_mask = ~sensor_id_mask;

	sensor_id &= sensor_id_mask;
	sensor_id_mask &= -sensor_id_mask;
	sensor_id_mask -= 1;
	while (sensor_id_mask) {
		sensor_id_mask >>= 1;
		sensor_id >>= 1;
	}
	return sensor_id;
}

void cam_sensor_shutdown(struct cam_sensor_ctrl_t *s_ctrl)
{
	struct cam_sensor_power_ctrl_t *power_info =
		&s_ctrl->sensordata->power_info;
	int rc = 0;

#if defined(CONFIG_CAMERA_FRAME_CNT_DBG)
	cam_sensor_thread_destroy(s_ctrl);
#endif

	if ((s_ctrl->sensor_state == CAM_SENSOR_INIT) &&
		(s_ctrl->is_probe_succeed == 0))
		return;

	cam_sensor_release_stream_rsc(s_ctrl);
	cam_sensor_release_per_frame_resource(s_ctrl);

	if (s_ctrl->sensor_state != CAM_SENSOR_INIT)
		cam_sensor_power_down(s_ctrl);

	if (s_ctrl->bridge_intf.device_hdl != -1) {
		rc = cam_destroy_device_hdl(s_ctrl->bridge_intf.device_hdl);
		if (rc < 0)
			CAM_ERR(CAM_SENSOR,
				"dhdl already destroyed: rc = %d", rc);
	}

	s_ctrl->bridge_intf.device_hdl = -1;
	s_ctrl->bridge_intf.link_hdl = -1;
	s_ctrl->bridge_intf.session_hdl = -1;
	kfree(power_info->power_setting);
	kfree(power_info->power_down_setting);
	power_info->power_setting = NULL;
	power_info->power_down_setting = NULL;
	power_info->power_setting_size = 0;
	power_info->power_down_setting_size = 0;
	s_ctrl->streamon_count = 0;
	s_ctrl->streamoff_count = 0;
	s_ctrl->is_probe_succeed = 0;
	s_ctrl->last_flush_req = 0;
	s_ctrl->sensor_state = CAM_SENSOR_INIT;
}

int cam_sensor_match_id(struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	uint32_t chipid = 0;
	struct cam_camera_slave_info *slave_info;

	slave_info = &(s_ctrl->sensordata->slave_info);

	if (!slave_info) {
		CAM_ERR(CAM_SENSOR, " failed: %pK",
			 slave_info);
		return -EINVAL;
	}

	rc = camera_io_dev_read(
		&(s_ctrl->io_master_info),
		slave_info->sensor_id_reg_addr,
		&chipid, CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD);

	chipid = cam_sensor_id_by_mask(s_ctrl, chipid);
	CAM_INFO(CAM_SENSOR, "read id: 0x%x expected id 0x%x:",
		chipid, slave_info->sensor_id);

	if (chipid != slave_info->sensor_id) {
		CAM_WARN(CAM_SENSOR, "read id: 0x%x expected id 0x%x:",
				chipid, slave_info->sensor_id);
		return -ENODEV;
	}
	return rc;
}

#if defined(CONFIG_SAMSUNG_FRONT_TOF) || defined(CONFIG_SAMSUNG_REAR_TOF)
int cam_sensor_tofled_enable(struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	uint32_t value = 0xFF;
	struct cam_camera_slave_info *slave_info;
	struct cam_sensor_i2c_reg_setting  i2c_reg_settings;
	struct cam_sensor_i2c_reg_array    i2c_reg_array;
	uint16_t sid = 0x0, sensor_id_reg_addr = 0x0;

	slave_info = &(s_ctrl->sensordata->slave_info);

	if (!slave_info) {
		CAM_ERR(CAM_SENSOR, " failed: %pK",
			 slave_info);
		return -EINVAL;
	}

	sid = s_ctrl->io_master_info.cci_client->sid;
	sensor_id_reg_addr = slave_info->sensor_id_reg_addr;

	s_ctrl->io_master_info.cci_client->sid = 0x1C;
	slave_info->sensor_id_reg_addr = 0x01;
	rc = camera_io_dev_read(
		&(s_ctrl->io_master_info),
		slave_info->sensor_id_reg_addr,
		&value, CAMERA_SENSOR_I2C_TYPE_BYTE,
		CAMERA_SENSOR_I2C_TYPE_BYTE);

	CAM_ERR(CAM_SENSOR, "[TOF_DBG] rc: %d, reg_addr: 0x%x  0x%x:",
		rc, slave_info->sensor_id_reg_addr, value);

	i2c_reg_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_settings.size = 1;
	i2c_reg_settings.delay = 0;
	i2c_reg_array.reg_addr = 0x01;
	i2c_reg_array.reg_data = 0x30;
	i2c_reg_array.delay = 0;
	i2c_reg_array.data_mask = 0x0;
	i2c_reg_settings.reg_setting = &i2c_reg_array;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
		&i2c_reg_settings);

	CAM_ERR(CAM_SENSOR, "[TOF_DBG]  re: %d, tof_i2c_write, addr : 0x%x, data : 0x%x", rc, i2c_reg_array.reg_addr, i2c_reg_array.reg_data);

	rc = camera_io_dev_read(
		&(s_ctrl->io_master_info),
		slave_info->sensor_id_reg_addr,
		&value, CAMERA_SENSOR_I2C_TYPE_BYTE,
		CAMERA_SENSOR_I2C_TYPE_BYTE);

	CAM_ERR(CAM_SENSOR, "[TOF_DBG]  rc: %d, reg_addr: 0x%x  0x%x:",
		rc,slave_info->sensor_id_reg_addr, value);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR, "[TOF_DBG] Failed to i2c write");

	s_ctrl->io_master_info.cci_client->sid = sid;
	slave_info->sensor_id_reg_addr = sensor_id_reg_addr;

	return rc;
}
#endif

int32_t cam_sensor_driver_cmd(struct cam_sensor_ctrl_t *s_ctrl,
	void *arg)
{
	int rc = 0, pkt_opcode = 0;
	struct cam_control *cmd = (struct cam_control *)arg;
	struct cam_sensor_power_ctrl_t *power_info =
		&s_ctrl->sensordata->power_info;
#if 1
	int RETRY_CNT = 3, i = 0;
#endif
#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
	struct cam_hw_param *hw_param = NULL;
#endif

	if (!s_ctrl || !arg) {
		CAM_ERR(CAM_SENSOR, "s_ctrl is NULL");
		return -EINVAL;
	}

	if (cmd->op_code != CAM_SENSOR_PROBE_CMD) {
		if (cmd->handle_type != CAM_HANDLE_USER_POINTER) {
			CAM_ERR(CAM_SENSOR, "Invalid handle type: %d",
				cmd->handle_type);
			return -EINVAL;
		}
	}

	mutex_lock(&(s_ctrl->cam_sensor_mutex));
	switch (cmd->op_code) {
	case CAM_SENSOR_PROBE_CMD: {
		if (s_ctrl->is_probe_succeed == 1) {
			CAM_ERR(CAM_SENSOR,
				"Already Sensor Probed in the slot");
			break;
		}
#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
		sec_sensor_position = s_ctrl->id;
#endif

		if (cmd->handle_type ==
			CAM_HANDLE_MEM_HANDLE) {
			rc = cam_handle_mem_ptr(cmd->handle, s_ctrl);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR, "Get Buffer Handle Failed");
				goto release_mutex;
			}
		} else {
			CAM_ERR(CAM_SENSOR, "Invalid Command Type: %d",
				 cmd->handle_type);
			rc = -EINVAL;
			goto release_mutex;
		}

		/* Parse and fill vreg params for powerup settings */
		rc = msm_camera_fill_vreg_params(
			&s_ctrl->soc_info,
			s_ctrl->sensordata->power_info.power_setting,
			s_ctrl->sensordata->power_info.power_setting_size);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Fail in filling vreg params for PUP rc %d",
				 rc);
			goto free_power_settings;
		}

		/* Parse and fill vreg params for powerdown settings*/
		rc = msm_camera_fill_vreg_params(
			&s_ctrl->soc_info,
			s_ctrl->sensordata->power_info.power_down_setting,
			s_ctrl->sensordata->power_info.power_down_setting_size);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Fail in filling vreg params for PDOWN rc %d",
				 rc);
			goto free_power_settings;
		}

		/* Power up and probe sensor */
		rc = cam_sensor_power_up(s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "power up failed");
			goto free_power_settings;
		}

		/* Match sensor ID */
		rc = cam_sensor_match_id(s_ctrl);
#if defined(CONFIG_SENSOR_RETENTION)
		if (rc == 0
			&& s_ctrl->sensordata->slave_info.sensor_id == RETENTION_SENSOR_ID
			&& sensor_retention_mode == RETENTION_INIT) {
			cam_sensor_write_normal_init(&s_ctrl->io_master_info);
			sensor_retention_mode = RETENTION_READY_TO_ON;
			cam_sensor_write_enable_crc(&s_ctrl->io_master_info);
			usleep_range(5000, 6000);
		}
#else
		usleep_range(1000, 2000);
#endif

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
		if (rc < 0) {
			CAM_ERR(CAM_HWB, "failed rc %d\n", rc);
			if (s_ctrl != NULL) {
				switch (s_ctrl->id) {
				case CAMERA_0:
					if (!msm_is_sec_get_rear_hw_param(&hw_param)) {
						if (hw_param != NULL) {
							CAM_ERR(CAM_HWB, "[R][I2C] Err\n");
							hw_param->i2c_sensor_err_cnt++;
							hw_param->need_update_to_file = TRUE;
						}
					}
					break;

				case CAMERA_1:
				case CAMERA_12:
					if (!msm_is_sec_get_front_hw_param(&hw_param)) {
						if (hw_param != NULL) {
							CAM_ERR(CAM_HWB, "[F][I2C] Err\n");
							hw_param->i2c_sensor_err_cnt++;
							hw_param->need_update_to_file = TRUE;
						}
					}
					break;

#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
				case CAMERA_11:
					if (!msm_is_sec_get_front3_hw_param(&hw_param)) {
						if (hw_param != NULL) {
							CAM_ERR(CAM_HWB, "[F3][I2C] Err\n");
							hw_param->i2c_sensor_err_cnt++;
							hw_param->need_update_to_file = TRUE;
						}
					}
					break;
#else
				case CAMERA_11:
					if (!msm_is_sec_get_front2_hw_param(&hw_param)) {
						if (hw_param != NULL) {
							CAM_ERR(CAM_HWB, "[F2][I2C] Err\n");
							hw_param->i2c_sensor_err_cnt++;
							hw_param->need_update_to_file = TRUE;
						}
					}
					break;
#endif
#endif

#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
				case CAMERA_2:
					if (!msm_is_sec_get_rear2_hw_param(&hw_param)) {
						if (hw_param != NULL) {
							CAM_ERR(CAM_HWB, "[R2][I2C] Err\n");
							hw_param->i2c_sensor_err_cnt++;
							hw_param->need_update_to_file = TRUE;
						}
					}
					break;
#endif

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
				case CAMERA_3:
					if (!msm_is_sec_get_rear3_hw_param(&hw_param)) {
						if (hw_param != NULL) {
							CAM_ERR(CAM_HWB, "[R3][I2C] Err\n");
							hw_param->i2c_sensor_err_cnt++;
							hw_param->need_update_to_file = TRUE;
						}
					}
					break;
#endif

				default:
					CAM_DBG(CAM_HWB, "[NON][I2C][%d] Unsupport\n", s_ctrl->id);
					break;
				}
			}
		}
#endif

#if defined(CONFIG_SAMSUNG_REAR_TOF) || defined(CONFIG_SAMSUNG_FRONT_TOF)
		if ((rc < 0) &&
			(s_ctrl->sensordata->slave_info.sensor_id == TOF_SENSOR_ID_IMX518))
		{
			cam_sensor_power_down(s_ctrl);
			msleep(20);
			goto free_power_settings;
		}
#endif
#if defined(CONFIG_SEC_X1Q_PROJECT) || defined(CONFIG_SEC_Y2Q_PROJECT) || defined(CONFIG_SEC_C1Q_PROJECT)
		if ((rc < 0) &&
			(((s_ctrl->soc_info.index == 0) &&
			(s_ctrl->sensordata->slave_info.sensor_id == SENSOR_ID_IMX555)) ||
			(((s_ctrl->soc_info.index == 1) || (s_ctrl->soc_info.index == 12))&&
			(s_ctrl->sensordata->slave_info.sensor_id == FRONT_SENSOR_ID_IMX374))))
		{
			cam_sensor_power_down(s_ctrl);
			msleep(20);
			goto free_power_settings;
		}
#endif
#if 0
		if (rc < 0) {
			cam_sensor_power_down(s_ctrl);
			msleep(20);
			goto free_power_settings;
		}
#endif

		CAM_INFO(CAM_SENSOR,
			"Probe success,slot:%d,slave_addr:0x%x,sensor_id:0x%x",
			s_ctrl->soc_info.index,
			s_ctrl->sensordata->slave_info.sensor_slave_addr,
			s_ctrl->sensordata->slave_info.sensor_id);

#if defined(CONFIG_SAMSUNG_FRONT_TOF) || defined(CONFIG_SAMSUNG_REAR_TOF)
		if (s_ctrl->sensordata->slave_info.sensor_id == 0x374)
		{
			unsigned int rev = sec_hw_rev();
			CAM_INFO(CAM_SENSOR, "[RET_DBG] board rev : %d", rev);

			if(rev < CRITERION_REV)
				rc = cam_sensor_tofled_enable(s_ctrl);
		}
#endif

		rc = cam_sensor_power_down(s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "fail in Sensor Power Down");
			goto free_power_settings;
		}
		/*
		 * Set probe succeeded flag to 1 so that no other camera shall
		 * probed on this slot
		 */
		s_ctrl->is_probe_succeed = 1;
		s_ctrl->sensor_state = CAM_SENSOR_INIT;
	}
		break;
	case CAM_ACQUIRE_DEV: {
		struct cam_sensor_acquire_dev sensor_acq_dev;
		struct cam_create_dev_hdl bridge_params;

		if ((s_ctrl->is_probe_succeed == 0) ||
			(s_ctrl->sensor_state != CAM_SENSOR_INIT)) {
			CAM_WARN(CAM_SENSOR,
				"Not in right state to aquire %d",
				s_ctrl->sensor_state);
			rc = -EINVAL;
			goto release_mutex;
		}

		if (s_ctrl->bridge_intf.device_hdl != -1) {
			CAM_ERR(CAM_SENSOR, "Device is already acquired");
			rc = -EINVAL;
			goto release_mutex;
		}
		rc = copy_from_user(&sensor_acq_dev,
			u64_to_user_ptr(cmd->handle),
			sizeof(sensor_acq_dev));
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "Failed Copying from user");
			goto release_mutex;
		}

		bridge_params.session_hdl = sensor_acq_dev.session_handle;
		bridge_params.ops = &s_ctrl->bridge_intf.ops;
		bridge_params.v4l2_sub_dev_flag = 0;
		bridge_params.media_entity_flag = 0;
		bridge_params.priv = s_ctrl;

		sensor_acq_dev.device_handle =
			cam_create_device_hdl(&bridge_params);
		s_ctrl->bridge_intf.device_hdl = sensor_acq_dev.device_handle;
		s_ctrl->bridge_intf.session_hdl = sensor_acq_dev.session_handle;

		CAM_DBG(CAM_SENSOR, "Device Handle: %d",
			sensor_acq_dev.device_handle);

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
		if (sec_sensor_position < s_ctrl->id) {
			sec_sensor_position = s_ctrl->id;
			CAM_ERR(CAM_HWB, "sensor_position: %d", sec_sensor_position);
		}
#endif

#if defined(CONFIG_SAMSUNG_REAR_TOF) || defined(CONFIG_SAMSUNG_FRONT_TOF)
		if ((s_ctrl->sensordata->slave_info.sensor_id == TOF_SENSOR_ID_IMX516) ||
			(s_ctrl->sensordata->slave_info.sensor_id == TOF_SENSOR_ID_IMX518))
		{
			g_s_ctrl_tof = s_ctrl;
			check_pd_ready = 0;
			cam_mipi_init_setting(s_ctrl);
			cam_mipi_update_info(s_ctrl);
			cam_mipi_get_clock_string(s_ctrl);
		}
#endif

#if defined(CONFIG_CAMERA_SSM_I2C_ENV)
		if (s_ctrl->id == CAMERA_0)
		{
			g_s_ctrl_ssm = s_ctrl;
		}
#endif
		if (copy_to_user(u64_to_user_ptr(cmd->handle),
			&sensor_acq_dev,
			sizeof(struct cam_sensor_acquire_dev))) {
			CAM_ERR(CAM_SENSOR, "Failed Copy to User");
			rc = -EFAULT;
			goto release_mutex;
		}

#if 1
		for (i = 0; i < RETRY_CNT; i++) {
			rc = cam_sensor_power_up(s_ctrl);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR, "Sensor Power up failed");
				goto release_mutex;
			}

			/* Match sensor ID */
			rc = cam_sensor_match_id(s_ctrl);
			if (rc < 0) {
				cam_sensor_power_down(s_ctrl);
				msleep(20);
				CAM_ERR(CAM_SENSOR, "Match sensor ID fail retry %d", i);
			}
			else
				break;
		}
		if (rc < 0) {
			if (s_ctrl->bridge_intf.device_hdl != -1)
				cam_destroy_device_hdl(s_ctrl->bridge_intf.device_hdl);
			s_ctrl->bridge_intf.device_hdl = -1;
			s_ctrl->bridge_intf.link_hdl = -1;
			s_ctrl->bridge_intf.session_hdl = -1;
			goto release_mutex;
		}
#else
		rc = cam_sensor_power_up(s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "Sensor Power up failed");
			goto release_mutex;
		}

#if 1 //For factory module test
		/* Match sensor ID */
		rc = cam_sensor_match_id(s_ctrl);
		if (rc < 0) {
			cam_sensor_power_down(s_ctrl);
			msleep(20);
			if (s_ctrl->bridge_intf.device_hdl != -1)
				cam_destroy_device_hdl(s_ctrl->bridge_intf.device_hdl);
			s_ctrl->bridge_intf.device_hdl = -1;
			s_ctrl->bridge_intf.link_hdl = -1;
			s_ctrl->bridge_intf.session_hdl = -1;
			goto release_mutex;
		}
#endif
#endif
		s_ctrl->sensor_state = CAM_SENSOR_ACQUIRE;
		s_ctrl->last_flush_req = 0;
		CAM_INFO(CAM_SENSOR,
			"CAM_ACQUIRE_DEV Success, sensor_id:0x%x,sensor_slave_addr:0x%x",
			s_ctrl->sensordata->slave_info.sensor_id,
			s_ctrl->sensordata->slave_info.sensor_slave_addr);
	}
		break;
	case CAM_RELEASE_DEV: {
		if ((s_ctrl->sensor_state == CAM_SENSOR_INIT) ||
			(s_ctrl->sensor_state == CAM_SENSOR_START)) {
			rc = -EINVAL;
			CAM_WARN(CAM_SENSOR,
			"Not in right state to release : %d",
			s_ctrl->sensor_state);
			goto release_mutex;
		}

		if (s_ctrl->bridge_intf.link_hdl != -1) {
			CAM_ERR(CAM_SENSOR,
				"Device [%d] still active on link 0x%x",
				s_ctrl->sensor_state,
				s_ctrl->bridge_intf.link_hdl);
			rc = -EAGAIN;
			goto release_mutex;
		}

#if defined(CONFIG_SAMSUNG_REAR_TOF) || defined(CONFIG_SAMSUNG_FRONT_TOF)
		if (s_ctrl->sensordata->slave_info.sensor_id == TOF_SENSOR_ID_IMX516
			|| s_ctrl->sensordata->slave_info.sensor_id == TOF_SENSOR_ID_IMX518) {
			scnprintf(tof_freq, sizeof(tof_freq), "0");
			CAM_INFO(CAM_SENSOR, "[TOF_FREQ_DBG] tof_freq : %s", tof_freq);
		}
#endif

		rc = cam_sensor_power_down(s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "Sensor Power Down failed");
			goto release_mutex;
		}

		cam_sensor_release_per_frame_resource(s_ctrl);
		cam_sensor_release_stream_rsc(s_ctrl);
		if (s_ctrl->bridge_intf.device_hdl == -1) {
			CAM_ERR(CAM_SENSOR,
				"Invalid Handles: link hdl: %d device hdl: %d",
				s_ctrl->bridge_intf.device_hdl,
				s_ctrl->bridge_intf.link_hdl);
			rc = -EINVAL;
			goto release_mutex;
		}
		rc = cam_destroy_device_hdl(s_ctrl->bridge_intf.device_hdl);
		if (rc < 0)
			CAM_ERR(CAM_SENSOR,
				"failed in destroying the device hdl");
		s_ctrl->bridge_intf.device_hdl = -1;
		s_ctrl->bridge_intf.link_hdl = -1;
		s_ctrl->bridge_intf.session_hdl = -1;

		s_ctrl->sensor_state = CAM_SENSOR_INIT;
		CAM_INFO(CAM_SENSOR,
			"CAM_RELEASE_DEV Success, sensor_id:0x%x,sensor_slave_addr:0x%x",
			s_ctrl->sensordata->slave_info.sensor_id,
			s_ctrl->sensordata->slave_info.sensor_slave_addr);
		s_ctrl->streamon_count = 0;
		s_ctrl->streamoff_count = 0;
		s_ctrl->last_flush_req = 0;
#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
		// CAM_ERR(CAM_SENSOR, "[FRS_DBG] FRS init");
		rear_frs_test_mode = 0;
#endif
#if defined(CONFIG_SAMSUNG_REAR_TOF) || defined(CONFIG_SAMSUNG_FRONT_TOF)
		g_s_ctrl_tof = NULL;
		check_pd_ready = 0;
#endif
	}
		break;
	case CAM_QUERY_CAP: {
		struct  cam_sensor_query_cap sensor_cap;

		cam_sensor_query_cap(s_ctrl, &sensor_cap);
		if (copy_to_user(u64_to_user_ptr(cmd->handle),
			&sensor_cap, sizeof(struct  cam_sensor_query_cap))) {
			CAM_ERR(CAM_SENSOR, "Failed Copy to User");
			rc = -EFAULT;
			goto release_mutex;
		}
		break;
	}
	case CAM_START_DEV: {
		if ((s_ctrl->sensor_state == CAM_SENSOR_INIT) ||
			(s_ctrl->sensor_state == CAM_SENSOR_START)) {
			rc = -EINVAL;
			CAM_WARN(CAM_SENSOR,
			"Not in right state to start : %d",
			s_ctrl->sensor_state);
			goto release_mutex;
		}

#if defined(CONFIG_CAMERA_FRAME_CNT_DBG)
		// To print frame count,
		// echo 1 > /sys/module/cam_sensor_core/parameters/frame_cnt_dbg
		if (frame_cnt_dbg > 0)
		{
			rc = cam_sensor_thread_create(s_ctrl);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR, "Failed create sensor thread");
				goto release_mutex;
			}
		}
#endif

		if (s_ctrl->i2c_data.streamon_settings.is_settings_valid &&
			(s_ctrl->i2c_data.streamon_settings.request_id == 0)) {
			rc = cam_sensor_apply_settings(s_ctrl, 0,
				CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMON);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"cannot apply streamon settings");
				goto release_mutex;
			}
		}
		s_ctrl->sensor_state = CAM_SENSOR_START;
		CAM_INFO(CAM_SENSOR,
			"CAM_START_DEV Success, sensor_id:0x%x,sensor_slave_addr:0x%x",
			s_ctrl->sensordata->slave_info.sensor_id,
			s_ctrl->sensordata->slave_info.sensor_slave_addr);
	}
		break;
	case CAM_STOP_DEV: {
		if (s_ctrl->sensor_state != CAM_SENSOR_START) {
			rc = -EINVAL;
			CAM_WARN(CAM_SENSOR,
			"Not in right state to stop : %d",
			s_ctrl->sensor_state);
			goto release_mutex;
		}

#if defined(CONFIG_CAMERA_FRAME_CNT_DBG)
		cam_sensor_thread_destroy(s_ctrl);
#endif
		if (s_ctrl->i2c_data.streamoff_settings.is_settings_valid &&
			(s_ctrl->i2c_data.streamoff_settings.request_id == 0)) {
			rc = cam_sensor_apply_settings(s_ctrl, 0,
				CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMOFF);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
				"cannot apply streamoff settings");
			}
		}

		cam_sensor_release_per_frame_resource(s_ctrl);
		s_ctrl->last_flush_req = 0;
		s_ctrl->sensor_state = CAM_SENSOR_ACQUIRE;
		CAM_INFO(CAM_SENSOR,
			"CAM_STOP_DEV Success, sensor_id:0x%x,sensor_slave_addr:0x%x",
			s_ctrl->sensordata->slave_info.sensor_id,
			s_ctrl->sensordata->slave_info.sensor_slave_addr);
#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
		// CAM_ERR(CAM_SENSOR, "[FRS_DBG] FRS init");
		rear_frs_test_mode = 0;
#endif
	}
		break;
	case CAM_CONFIG_DEV: {
		rc = cam_sensor_i2c_pkt_parse(s_ctrl, arg);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "Failed i2c pkt parse: %d", rc);
			goto release_mutex;
		}
		if (s_ctrl->i2c_data.init_settings.is_settings_valid &&
			(s_ctrl->i2c_data.init_settings.request_id == 0)) {

			pkt_opcode =
				CAM_SENSOR_PACKET_OPCODE_SENSOR_INITIAL_CONFIG;
			rc = cam_sensor_apply_settings(s_ctrl, 0,
				pkt_opcode);

			if ((rc == -EAGAIN) &&
			(s_ctrl->io_master_info.master_type == CCI_MASTER)) {
				/* If CCI hardware is resetting we need to wait
				 * for sometime before reapply
				 */
				CAM_WARN(CAM_SENSOR,
					"Reapplying the Init settings due to cci hw reset");
				usleep_range(1000, 1010);
				rc = cam_sensor_apply_settings(s_ctrl, 0,
					pkt_opcode);
			}
			s_ctrl->i2c_data.init_settings.request_id = -1;

			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"cannot apply init settings rc= %d",
					rc);
				delete_request(&s_ctrl->i2c_data.init_settings);
				goto release_mutex;
			}
			rc = delete_request(&s_ctrl->i2c_data.init_settings);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"Fail in deleting the Init settings");
				goto release_mutex;
			}
		}

		if (s_ctrl->i2c_data.config_settings.is_settings_valid &&
			(s_ctrl->i2c_data.config_settings.request_id == 0)) {
			rc = cam_sensor_apply_settings(s_ctrl, 0,
				CAM_SENSOR_PACKET_OPCODE_SENSOR_CONFIG);

			s_ctrl->i2c_data.config_settings.request_id = -1;

			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"cannot apply config settings");
				delete_request(
					&s_ctrl->i2c_data.config_settings);
				goto release_mutex;
			}
			rc = delete_request(&s_ctrl->i2c_data.config_settings);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"Fail in deleting the config settings");
				goto release_mutex;
			}
			s_ctrl->sensor_state = CAM_SENSOR_CONFIG;
		}
	}
		break;
	default:
		CAM_ERR(CAM_SENSOR, "Invalid Opcode: %d", cmd->op_code);
		rc = -EINVAL;
		goto release_mutex;
	}

release_mutex:
	mutex_unlock(&(s_ctrl->cam_sensor_mutex));
	return rc;

free_power_settings:
#if defined(CONFIG_CAMERA_FRS_DRAM_TEST)
	// CAM_ERR(CAM_SENSOR, "[FRS_DBG] FRS init");
	rear_frs_test_mode = 0;
#endif
	kfree(power_info->power_setting);
	kfree(power_info->power_down_setting);
	power_info->power_setting = NULL;
	power_info->power_down_setting = NULL;
	power_info->power_down_setting_size = 0;
	power_info->power_setting_size = 0;
	mutex_unlock(&(s_ctrl->cam_sensor_mutex));
	return rc;
}

int cam_sensor_publish_dev_info(struct cam_req_mgr_device_info *info)
{
	int rc = 0;
	struct cam_sensor_ctrl_t *s_ctrl = NULL;

	if (!info)
		return -EINVAL;

	s_ctrl = (struct cam_sensor_ctrl_t *)
		cam_get_device_priv(info->dev_hdl);

	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "Device data is NULL");
		return -EINVAL;
	}

	info->dev_id = CAM_REQ_MGR_DEVICE_SENSOR;
	strlcpy(info->name, CAM_SENSOR_NAME, sizeof(info->name));
	if (s_ctrl->pipeline_delay >= 1 && s_ctrl->pipeline_delay <= 3)
		info->p_delay = s_ctrl->pipeline_delay;
	else
		info->p_delay = 2;
	info->trigger = CAM_TRIGGER_POINT_SOF;

	return rc;
}

int cam_sensor_establish_link(struct cam_req_mgr_core_dev_link_setup *link)
{
	struct cam_sensor_ctrl_t *s_ctrl = NULL;

	if (!link)
		return -EINVAL;

	s_ctrl = (struct cam_sensor_ctrl_t *)
		cam_get_device_priv(link->dev_hdl);
	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "Device data is NULL");
		return -EINVAL;
	}

	mutex_lock(&s_ctrl->cam_sensor_mutex);
	if (link->link_enable) {
		s_ctrl->bridge_intf.link_hdl = link->link_hdl;
		s_ctrl->bridge_intf.crm_cb = link->crm_cb;
	} else {
		s_ctrl->bridge_intf.link_hdl = -1;
		s_ctrl->bridge_intf.crm_cb = NULL;
	}
	mutex_unlock(&s_ctrl->cam_sensor_mutex);

	return 0;
}

int cam_sensor_power(struct v4l2_subdev *sd, int on)
{
	struct cam_sensor_ctrl_t *s_ctrl = v4l2_get_subdevdata(sd);

	mutex_lock(&(s_ctrl->cam_sensor_mutex));
	if (!on && s_ctrl->sensor_state == CAM_SENSOR_START) {
		cam_sensor_power_down(s_ctrl);
		s_ctrl->sensor_state = CAM_SENSOR_ACQUIRE;
	}
	mutex_unlock(&(s_ctrl->cam_sensor_mutex));

	return 0;
}

int cam_sensor_power_up(struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc;
	struct cam_sensor_power_ctrl_t *power_info;
	struct cam_camera_slave_info *slave_info;
	struct cam_hw_soc_info *soc_info =
		&s_ctrl->soc_info;
#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
	struct cam_hw_param *hw_param = NULL;
#endif

	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "failed: %pK", s_ctrl);
		return -EINVAL;
	}

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
	if (s_ctrl != NULL) {
		switch (s_ctrl->id) {
			case CAMERA_0:
				if (!msm_is_sec_get_rear_hw_param(&hw_param)) {
					if (hw_param != NULL) {
						CAM_DBG(CAM_HWB, "[R][INIT] Init\n");
						hw_param->i2c_chk = FALSE;
						hw_param->mipi_chk = FALSE;
						hw_param->need_update_to_file = FALSE;
						hw_param->comp_chk = FALSE;
					}
				}
				break;

			case CAMERA_1:
			case CAMERA_12:
				if (!msm_is_sec_get_front_hw_param(&hw_param)) {
					if (hw_param != NULL) {
						CAM_DBG(CAM_HWB, "[F][INIT] Init\n");
						hw_param->i2c_chk = FALSE;
						hw_param->mipi_chk = FALSE;
						hw_param->need_update_to_file = FALSE;
						hw_param->comp_chk = FALSE;
					}
				}
				break;

#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
			case CAMERA_11:
				if (!msm_is_sec_get_front2_hw_param(&hw_param)) {
					if (hw_param != NULL) {
						CAM_DBG(CAM_HWB, "[F2][INIT] Init\n");
						hw_param->i2c_chk = FALSE;
						hw_param->mipi_chk = FALSE;
						hw_param->need_update_to_file = FALSE;
						hw_param->comp_chk = FALSE;
					}
				}
				break;
#else
			case CAMERA_11:
				if (!msm_is_sec_get_front3_hw_param(&hw_param)) {
					if (hw_param != NULL) {
						CAM_DBG(CAM_HWB, "[F3][INIT] Init\n");
						hw_param->i2c_chk = FALSE;
						hw_param->mipi_chk = FALSE;
						hw_param->need_update_to_file = FALSE;
						hw_param->comp_chk = FALSE;
					}
				}
				break;
#endif
#endif

#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
			case CAMERA_2:
				if (!msm_is_sec_get_rear2_hw_param(&hw_param)) {
					if (hw_param != NULL) {
						CAM_DBG(CAM_HWB, "[R2][INIT] Init\n");
						hw_param->i2c_chk = FALSE;
						hw_param->mipi_chk = FALSE;
						hw_param->need_update_to_file = FALSE;
						hw_param->comp_chk = FALSE;
					}
				}
				break;
#endif

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
			case CAMERA_3:
				if (!msm_is_sec_get_rear3_hw_param(&hw_param)) {
					if (hw_param != NULL) {
						CAM_DBG(CAM_HWB, "[R3][INIT] Init\n");
						hw_param->i2c_chk = FALSE;
						hw_param->mipi_chk = FALSE;
						hw_param->need_update_to_file = FALSE;
						hw_param->comp_chk = FALSE;
					}
				}
				break;
#endif

			default:
				CAM_DBG(CAM_HWB, "[NON][INIT][%d] Unsupport\n", s_ctrl->id);
				break;
		}
	}
#endif

	power_info = &s_ctrl->sensordata->power_info;
	slave_info = &(s_ctrl->sensordata->slave_info);

	if (!power_info || !slave_info) {
		CAM_ERR(CAM_SENSOR, "failed: %pK %pK", power_info, slave_info);
		return -EINVAL;
	}

	if (s_ctrl->bob_pwm_switch) {
		rc = cam_sensor_bob_pwm_mode_switch(soc_info,
			s_ctrl->bob_reg_index, true);
		if (rc) {
			CAM_WARN(CAM_SENSOR,
			"BoB PWM setup failed rc: %d", rc);
			rc = 0;
		}
	}

	rc = cam_sensor_core_power_up(power_info, soc_info);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "power up the core is failed:%d", rc);
		return rc;
	}

	rc = camera_io_init(&(s_ctrl->io_master_info));
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "cci_init failed: rc: %d", rc);
	}

	return rc;
}

int cam_sensor_power_down(struct cam_sensor_ctrl_t *s_ctrl)
{
	struct cam_sensor_power_ctrl_t *power_info;
	struct cam_hw_soc_info *soc_info;
	int rc = 0;
#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
	struct cam_hw_param *hw_param = NULL;
#endif

	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "failed: s_ctrl %pK", s_ctrl);
		return -EINVAL;
	}

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
	if (s_ctrl != NULL) {
		switch (s_ctrl->id) {
			case CAMERA_0:
				if (!msm_is_sec_get_rear_hw_param(&hw_param)) {
					if (hw_param != NULL) {
						hw_param->i2c_chk = FALSE;
						hw_param->mipi_chk = FALSE;
						hw_param->comp_chk = FALSE;

						if (hw_param->need_update_to_file) {
						CAM_DBG(CAM_HWB, "[R][DEINIT] Update\n");
						msm_is_sec_copy_err_cnt_to_file();
						}
						hw_param->need_update_to_file = FALSE;
					}
				}
				break;

			case CAMERA_1:
			case CAMERA_12:
				if (!msm_is_sec_get_front_hw_param(&hw_param)) {
					if (hw_param != NULL) {
						hw_param->i2c_chk = FALSE;
						hw_param->mipi_chk = FALSE;
						hw_param->comp_chk = FALSE;

						if (hw_param->need_update_to_file) {
							CAM_DBG(CAM_HWB, "[F][DEINIT] Update\n");
							msm_is_sec_copy_err_cnt_to_file();
						}
							hw_param->need_update_to_file = FALSE;
					}
				}
				break;

#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
			case CAMERA_11:
				if (!msm_is_sec_get_front3_hw_param(&hw_param)) {
					if (hw_param != NULL) {
						hw_param->i2c_chk = FALSE;
						hw_param->mipi_chk = FALSE;
						hw_param->comp_chk = FALSE;

						if (hw_param->need_update_to_file) {
							CAM_DBG(CAM_HWB, "[F3][DEINIT] Update\n");
							msm_is_sec_copy_err_cnt_to_file();
						}
						hw_param->need_update_to_file = FALSE;
					}
				}
				break;
#else
			case CAMERA_11:
				if (!msm_is_sec_get_front2_hw_param(&hw_param)) {
					if (hw_param != NULL) {
						hw_param->i2c_chk = FALSE;
						hw_param->mipi_chk = FALSE;
						hw_param->comp_chk = FALSE;

						if (hw_param->need_update_to_file) {
							CAM_DBG(CAM_HWB, "[F2][DEINIT] Update\n");
							msm_is_sec_copy_err_cnt_to_file();
						}
						hw_param->need_update_to_file = FALSE;
					}
				}
				break;
#endif
#endif

#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
			case CAMERA_2:
				if (!msm_is_sec_get_rear2_hw_param(&hw_param)) {
					if (hw_param != NULL) {
						hw_param->i2c_chk = FALSE;
						hw_param->mipi_chk = FALSE;
						hw_param->comp_chk = FALSE;

						if (hw_param->need_update_to_file) {
							CAM_DBG(CAM_HWB, "[R2][DEINIT] Update\n");
							msm_is_sec_copy_err_cnt_to_file();
						}
						hw_param->need_update_to_file = FALSE;
					}
				}
				break;
#endif

#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
			case CAMERA_3:
				if (!msm_is_sec_get_rear3_hw_param(&hw_param)) {
					if (hw_param != NULL) {
						hw_param->i2c_chk = FALSE;
						hw_param->mipi_chk = FALSE;
						hw_param->comp_chk = FALSE;

						if (hw_param->need_update_to_file) {
							CAM_DBG(CAM_HWB, "[R3][DEINIT] Update\n");
							msm_is_sec_copy_err_cnt_to_file();
						}
						hw_param->need_update_to_file = FALSE;
					}
				}
				break;
#endif

			default:
				CAM_DBG(CAM_HWB, "[NON][DEINIT][%d] Unsupport\n", s_ctrl->id);
				break;
		}
	}
#endif

	power_info = &s_ctrl->sensordata->power_info;
	soc_info = &s_ctrl->soc_info;

	if (!power_info) {
		CAM_ERR(CAM_SENSOR, "failed: power_info %pK", power_info);
		return -EINVAL;
	}
	rc = cam_sensor_util_power_down(power_info, soc_info);
#if defined(CONFIG_SENSOR_RETENTION)
	if (s_ctrl->sensordata->slave_info.sensor_id == RETENTION_SENSOR_ID) {
		if (sensor_retention_mode == RETENTION_READY_TO_ON) {
			sensor_retention_mode = RETENTION_ON;
		}
	}
#endif
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "power down the core is failed:%d", rc);
		return rc;
	}

	if (s_ctrl->bob_pwm_switch) {
		rc = cam_sensor_bob_pwm_mode_switch(soc_info,
			s_ctrl->bob_reg_index, false);
		if (rc) {
			CAM_WARN(CAM_SENSOR,
				"BoB PWM setup failed rc: %d", rc);
			rc = 0;
		}
	}

	camera_io_release(&(s_ctrl->io_master_info));

	return rc;
}

int cam_sensor_apply_settings(struct cam_sensor_ctrl_t *s_ctrl,
	int64_t req_id, enum cam_sensor_packet_opcodes opcode)
{
	int rc = 0, offset, i;
	uint64_t top = 0, del_req_id = 0;
	struct i2c_settings_array *i2c_set = NULL;
	struct i2c_settings_list *i2c_list;

	if (req_id == 0) {
		switch (opcode) {
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMON: {
#if defined(CONFIG_CAMERA_FRAME_CNT_DBG)
			CAM_ERR(CAM_SENSOR, "[CNT_DBG][APPLY] CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMON");
#endif
			i2c_set = &s_ctrl->i2c_data.streamon_settings;
			break;
		}
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_INITIAL_CONFIG: {
			i2c_set = &s_ctrl->i2c_data.init_settings;
			break;
		}
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_CONFIG: {
			i2c_set = &s_ctrl->i2c_data.config_settings;
			break;
		}
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMOFF: {
#if defined(CONFIG_CAMERA_FRAME_CNT_DBG)
			CAM_ERR(CAM_SENSOR, "[CNT_DBG][APPLY] CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMOFF");
#endif
			i2c_set = &s_ctrl->i2c_data.streamoff_settings;
			break;
		}
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_UPDATE:
		case CAM_SENSOR_PACKET_OPCODE_SENSOR_PROBE:
		default:
			return 0;
		}
		if (i2c_set->is_settings_valid == 1) {

#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI)
			if (opcode == CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMON)
			{
				cam_sensor_apply_adaptive_mipi_settings(s_ctrl);
			}
#endif
			if (opcode == CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMOFF)
			{
#if defined(CONFIG_SENSOR_RETENTION)
				cam_sensor_write_enable_crc(&s_ctrl->io_master_info);
#endif
				cam_sensor_wait_stream_on(s_ctrl);
			}

#if defined(CONFIG_SENSOR_RETENTION)
			if (opcode == CAM_SENSOR_PACKET_OPCODE_SENSOR_INITIAL_CONFIG)
			{
				cam_sensor_apply_retention_init_settings(s_ctrl);
			}
#endif

			list_for_each_entry(i2c_list,
				&(i2c_set->list_head), list) {
				rc = cam_sensor_i2c_modes_util(
					s_ctrl,
					&(s_ctrl->io_master_info),
					i2c_list);
				if (rc < 0) {
					if (opcode == CAM_SENSOR_PACKET_OPCODE_SENSOR_INITIAL_CONFIG) {
						CAM_ERR(CAM_SENSOR,
							"Failed to apply settings: %d, retry initial setting",
							rc);
						msleep(10);
						rc = cam_sensor_power_down(s_ctrl);
						msleep(10);
						rc = cam_sensor_power_up(s_ctrl);
						msleep(10);
						rc = cam_sensor_i2c_modes_util(
							s_ctrl,
							&(s_ctrl->io_master_info),
							i2c_list);
						if (rc < 0) {
							CAM_ERR(CAM_SENSOR,
								"Failed to apply settings: %d",
								rc);
							return rc;
						}
					} else {
						CAM_ERR(CAM_SENSOR,
							"Failed to apply settings: %d",
							rc);
					}
					return rc;
				}
			}

			if (opcode == CAM_SENSOR_PACKET_OPCODE_SENSOR_STREAMOFF)
			{
				cam_sensor_wait_stream_off(s_ctrl);
			}
		}
	} else {
		offset = req_id % MAX_PER_FRAME_ARRAY;
		i2c_set = &(s_ctrl->i2c_data.per_frame[offset]);
		if (i2c_set->is_settings_valid == 1 &&
			i2c_set->request_id == req_id) {
			list_for_each_entry(i2c_list,
				&(i2c_set->list_head), list) {
				rc = cam_sensor_i2c_modes_util(
					s_ctrl,
					&(s_ctrl->io_master_info),
					i2c_list);
				if (rc < 0) {
					CAM_ERR(CAM_SENSOR,
						"Failed to apply settings: %d",
						rc);
					return rc;
				}
			}
		} else {
			CAM_DBG(CAM_SENSOR,
				"Invalid/NOP request to apply: %lld", req_id);
		}

		/* Change the logic dynamically */
		for (i = 0; i < MAX_PER_FRAME_ARRAY; i++) {
			if ((req_id >=
				s_ctrl->i2c_data.per_frame[i].request_id) &&
				(top <
				s_ctrl->i2c_data.per_frame[i].request_id) &&
				(s_ctrl->i2c_data.per_frame[i].is_settings_valid
					== 1)) {
				del_req_id = top;
				top = s_ctrl->i2c_data.per_frame[i].request_id;
			}
		}

		if (top < req_id) {
			if ((((top % MAX_PER_FRAME_ARRAY) - (req_id %
				MAX_PER_FRAME_ARRAY)) >= BATCH_SIZE_MAX) ||
				(((top % MAX_PER_FRAME_ARRAY) - (req_id %
				MAX_PER_FRAME_ARRAY)) <= -BATCH_SIZE_MAX))
				del_req_id = req_id;
		}

		if (!del_req_id)
			return rc;

		CAM_DBG(CAM_SENSOR, "top: %llu, del_req_id:%llu",
			top, del_req_id);

		for (i = 0; i < MAX_PER_FRAME_ARRAY; i++) {
			if ((del_req_id >
				 s_ctrl->i2c_data.per_frame[i].request_id) && (
				 s_ctrl->i2c_data.per_frame[i].is_settings_valid
					== 1)) {
				s_ctrl->i2c_data.per_frame[i].request_id = 0;
				rc = delete_request(
					&(s_ctrl->i2c_data.per_frame[i]));
				if (rc < 0)
					CAM_ERR(CAM_SENSOR,
						"Delete request Fail:%lld rc:%d",
						del_req_id, rc);
			}
		}
	}

	return rc;
}

int32_t cam_sensor_apply_request(struct cam_req_mgr_apply_request *apply)
{
	int32_t rc = 0;
	struct cam_sensor_ctrl_t *s_ctrl = NULL;

	if (!apply)
		return -EINVAL;

	s_ctrl = (struct cam_sensor_ctrl_t *)
		cam_get_device_priv(apply->dev_hdl);
	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "Device data is NULL");
		return -EINVAL;
	}
	CAM_DBG(CAM_REQ, " Sensor update req id: %lld", apply->request_id);
	trace_cam_apply_req("Sensor", apply->request_id);
	mutex_lock(&(s_ctrl->cam_sensor_mutex));
	rc = cam_sensor_apply_settings(s_ctrl, apply->request_id,
		CAM_SENSOR_PACKET_OPCODE_SENSOR_UPDATE);
	mutex_unlock(&(s_ctrl->cam_sensor_mutex));
	return rc;
}

int32_t cam_sensor_flush_request(struct cam_req_mgr_flush_request *flush_req)
{
	int32_t rc = 0, i;
	uint32_t cancel_req_id_found = 0;
	struct cam_sensor_ctrl_t *s_ctrl = NULL;
	struct i2c_settings_array *i2c_set = NULL;

	if (!flush_req)
		return -EINVAL;

	s_ctrl = (struct cam_sensor_ctrl_t *)
		cam_get_device_priv(flush_req->dev_hdl);
	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "Device data is NULL");
		return -EINVAL;
	}

	mutex_lock(&(s_ctrl->cam_sensor_mutex));
	if (s_ctrl->sensor_state != CAM_SENSOR_START ||
		s_ctrl->sensor_state != CAM_SENSOR_CONFIG) {
		mutex_unlock(&(s_ctrl->cam_sensor_mutex));
		return rc;
	}

	if (s_ctrl->i2c_data.per_frame == NULL) {
		CAM_ERR(CAM_SENSOR, "i2c frame data is NULL");
		mutex_unlock(&(s_ctrl->cam_sensor_mutex));
		return -EINVAL;
	}

	if (flush_req->type == CAM_REQ_MGR_FLUSH_TYPE_ALL) {
		s_ctrl->last_flush_req = flush_req->req_id;
		CAM_DBG(CAM_SENSOR, "last reqest to flush is %lld",
			flush_req->req_id);
	}

	for (i = 0; i < MAX_PER_FRAME_ARRAY; i++) {
		i2c_set = &(s_ctrl->i2c_data.per_frame[i]);

		if ((flush_req->type == CAM_REQ_MGR_FLUSH_TYPE_CANCEL_REQ)
				&& (i2c_set->request_id != flush_req->req_id))
			continue;

		if (i2c_set->is_settings_valid == 1) {
			rc = delete_request(i2c_set);
			if (rc < 0)
				CAM_ERR(CAM_SENSOR,
					"delete request: %lld rc: %d",
					i2c_set->request_id, rc);

			if (flush_req->type ==
				CAM_REQ_MGR_FLUSH_TYPE_CANCEL_REQ) {
				cancel_req_id_found = 1;
				break;
			}
		}
	}

	if (flush_req->type == CAM_REQ_MGR_FLUSH_TYPE_CANCEL_REQ &&
		!cancel_req_id_found)
		CAM_DBG(CAM_SENSOR,
			"Flush request id:%lld not found in the pending list",
			flush_req->req_id);
	mutex_unlock(&(s_ctrl->cam_sensor_mutex));
	return rc;
}

#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
void msm_is_sec_init_all_cnt(void)
{
	CAM_INFO(CAM_HWB, "All_Init_Cnt\n");
	memset(&cam_hwparam_collector, 0, sizeof(struct cam_hw_param_collector));
}

void msm_is_sec_init_err_cnt_file(struct cam_hw_param *hw_param)
{
	if (hw_param != NULL) {
		CAM_INFO(CAM_HWB, "Init_Cnt\n");

		memset(hw_param, 0, sizeof(struct cam_hw_param));
		msm_is_sec_copy_err_cnt_to_file();
	} else {
		CAM_INFO(CAM_HWB, "NULL\n");
	}
}

void msm_is_sec_dbg_check(void)
{
	CAM_INFO(CAM_HWB, "Dbg E\n");
	CAM_INFO(CAM_HWB, "Dbg X\n");
}

void msm_is_sec_copy_err_cnt_to_file(void)
{
#if defined(HWB_FILE_OPERATION)
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long nwrite = 0;
	int old_mask = 0;

	CAM_INFO(CAM_HWB, "To_F\n");

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	old_mask = sys_umask(0);

	fp = filp_open(CAM_HW_ERR_CNT_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0660);
	if (IS_ERR_OR_NULL(fp)) {
		CAM_ERR(CAM_HWB, "[To_F] Err\n");
		sys_umask(old_mask);
		set_fs(old_fs);
		return;
	}

	nwrite = vfs_write(fp, (char *)&cam_hwparam_collector, sizeof(struct cam_hw_param_collector), &fp->f_pos);

	filp_close(fp, NULL);
	fp = NULL;
	sys_umask(old_mask);
	set_fs(old_fs);
#endif
}

void msm_is_sec_copy_err_cnt_from_file(void)
{
#if defined(HWB_FILE_OPERATION)
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long nread = 0;
	int ret = 0;

	ret = msm_is_sec_file_exist(CAM_HW_ERR_CNT_FILE_PATH, HW_PARAMS_NOT_CREATED);
	if (ret == 1) {
		CAM_INFO(CAM_HWB, "From_F\n");
		old_fs = get_fs();
		set_fs(KERNEL_DS);

		fp = filp_open(CAM_HW_ERR_CNT_FILE_PATH, O_RDONLY, 0660);
		if (IS_ERR_OR_NULL(fp)) {
			CAM_ERR(CAM_HWB, "[From_F] Err\n");
			set_fs(old_fs);
			return;
		}

		nread = vfs_read(fp, (char *)&cam_hwparam_collector, sizeof(struct cam_hw_param_collector), &fp->f_pos);

		filp_close(fp, NULL);
		fp = NULL;
		set_fs(old_fs);
	} else {
		CAM_INFO(CAM_HWB, "NoEx_F\n");
	}
#endif
}

int msm_is_sec_file_exist(char *filename, hw_params_check_type chktype)
{
	int ret = 0;
#if defined(HWB_FILE_OPERATION)
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long nwrite = 0;
	int old_mask = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if (sys_access(filename, 0) == 0) {
		CAM_INFO(CAM_HWB, "Ex_F\n");
		ret = 1;
	} else {
		switch (chktype) {
		case HW_PARAMS_CREATED:
			CAM_INFO(CAM_HWB, "Ex_Cr\n");
			msm_is_sec_init_all_cnt();

			old_mask = sys_umask(0);

			fp = filp_open(CAM_HW_ERR_CNT_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0660);
			if (IS_ERR_OR_NULL(fp)) {
				CAM_ERR(CAM_HWB, "[Ex_F] ERROR\n");
				ret = 0;
			} else {
				nwrite = vfs_write(fp, (char *)&cam_hwparam_collector, sizeof(struct cam_hw_param_collector), &fp->f_pos);

				filp_close(fp, current->files);
				fp = NULL;
				ret = 2;
			}
			sys_umask(old_mask);
			break;

		case HW_PARAMS_NOT_CREATED:
			CAM_INFO(CAM_HWB, "Ex_NoCr\n");
			ret = 0;
			break;

		default:
			CAM_INFO(CAM_HWB, "Ex_Err\n");
			ret = 0;
			break;
		}
	}

	set_fs(old_fs);
#endif

	return ret;
}

int msm_is_sec_get_sensor_position(uint32_t **cam_position)
{
	*cam_position = &sec_sensor_position;
	return 0;
}

int msm_is_sec_get_sensor_comp_mode(uint32_t **sensor_clk_size)
{
	*sensor_clk_size = &sec_sensor_clk_size;
	return 0;
}

int msm_is_sec_get_rear_hw_param(struct cam_hw_param **hw_param)
{
	*hw_param = &cam_hwparam_collector.rear_hwparam;
	return 0;
}

int msm_is_sec_get_front_hw_param(struct cam_hw_param **hw_param)
{
	*hw_param = &cam_hwparam_collector.front_hwparam;
	return 0;
}

int msm_is_sec_get_iris_hw_param(struct cam_hw_param **hw_param)
{
	*hw_param = &cam_hwparam_collector.iris_hwparam;
	return 0;
}

int msm_is_sec_get_rear2_hw_param(struct cam_hw_param **hw_param)
{
	*hw_param = &cam_hwparam_collector.rear2_hwparam;
	return 0;
}

int msm_is_sec_get_front2_hw_param(struct cam_hw_param **hw_param)
{
	*hw_param = &cam_hwparam_collector.front2_hwparam;
	return 0;
}

int msm_is_sec_get_front3_hw_param(struct cam_hw_param **hw_param)
{
	*hw_param = &cam_hwparam_collector.front3_hwparam;
	return 0;
}

int msm_is_sec_get_rear3_hw_param(struct cam_hw_param **hw_param)
{
	*hw_param = &cam_hwparam_collector.rear3_hwparam;
	return 0;
}
#endif


#if defined(CONFIG_SAMSUNG_REAR_TOF) || defined(CONFIG_SAMSUNG_FRONT_TOF)
void cam_sensor_tof_i2c_read(uint32_t addr, uint32_t *data,
	enum camera_sensor_i2c_type addr_type,
	enum camera_sensor_i2c_type data_type)
{
	int rc = 0;

	if (g_s_ctrl_tof)
	{
		rc = camera_io_dev_read(&g_s_ctrl_tof->io_master_info, addr,
			data, addr_type, data_type);
		if (rc < 0)
			CAM_ERR(CAM_SENSOR, "Failed to read 0x%x", addr);

		CAM_INFO(CAM_SENSOR, "[TOF_I2C] tof_i2c_read, addr : 0x%x, data : 0x%x", addr, *data);
	}
	else
	{
		CAM_ERR(CAM_SENSOR, "tof i2c is not ready!");
	}
}

void cam_sensor_tof_i2c_write(uint32_t addr, uint32_t data,
	enum camera_sensor_i2c_type addr_type,
	enum camera_sensor_i2c_type data_type)
{
	int rc = 0;

	struct cam_sensor_i2c_reg_setting  i2c_reg_settings;
	struct cam_sensor_i2c_reg_array    i2c_reg_array;

    CAM_INFO(CAM_SENSOR, "[TOF_I2C] tof_i2c_write, addr : 0x%x, data : 0x%x", addr, data);

	if (g_s_ctrl_tof)
	{
		i2c_reg_settings.addr_type = addr_type;
		i2c_reg_settings.data_type = data_type;
		i2c_reg_settings.size = 1;
		i2c_reg_settings.delay = 0;
		i2c_reg_array.reg_addr = addr;
		i2c_reg_array.reg_data = data;
		i2c_reg_array.delay = 0;
		i2c_reg_array.data_mask = 0x0;
		i2c_reg_settings.reg_setting = &i2c_reg_array;

		rc = camera_io_dev_write(&g_s_ctrl_tof->io_master_info,
			&i2c_reg_settings);

		if (rc < 0)
			CAM_ERR(CAM_SENSOR, "Failed to i2c write");
	}
	else
	{
		CAM_ERR(CAM_SENSOR, "tof i2c is not ready!");
	}
}
#endif

#if defined(CONFIG_CAMERA_SSM_I2C_ENV)
void cam_sensor_ssm_i2c_read(uint32_t addr, uint32_t *data,
	enum camera_sensor_i2c_type addr_type,
	enum camera_sensor_i2c_type data_type)
{
	int rc = 0;

	if (g_s_ctrl_ssm)
	{
		rc = camera_io_dev_read(&g_s_ctrl_ssm->io_master_info, addr,
			data, addr_type, data_type);

		if (rc < 0)
			CAM_ERR(CAM_SENSOR, "Failed to read 0x%x", addr);

		CAM_ERR(CAM_SENSOR, "[SSM_I2C] ssm_i2c_read, addr : 0x%x, data : 0x%x", addr, *data);
	}
	else
	{
		CAM_ERR(CAM_SENSOR, "ssm i2c is not ready!");
	}
}

void cam_sensor_ssm_i2c_write(uint32_t addr, uint32_t data,
	enum camera_sensor_i2c_type addr_type,
	enum camera_sensor_i2c_type data_type)
{
	int rc = 0;

	struct cam_sensor_i2c_reg_setting  i2c_reg_settings;
	struct cam_sensor_i2c_reg_array    i2c_reg_array;

	CAM_INFO(CAM_SENSOR, "[SSM_I2C] ssm_i2c_write, addr : 0x%x, data : 0x%x", addr, data);

	if (g_s_ctrl_ssm)
	{
		i2c_reg_settings.addr_type = addr_type;
		i2c_reg_settings.data_type = data_type;
		i2c_reg_settings.size = 1;
		i2c_reg_settings.delay = 0;
		i2c_reg_array.reg_addr = addr;
		i2c_reg_array.reg_data = data;
		i2c_reg_array.delay = 0;
		i2c_reg_array.data_mask = 0x0;
		i2c_reg_settings.reg_setting = &i2c_reg_array;

		rc = camera_io_dev_write(&g_s_ctrl_ssm->io_master_info,
			&i2c_reg_settings);

		if (rc < 0)
			CAM_ERR(CAM_SENSOR, "Failed to i2c write");

	}
	else
	{
		CAM_ERR(CAM_SENSOR, "ssm i2c is not ready!");
	}
}
#endif
