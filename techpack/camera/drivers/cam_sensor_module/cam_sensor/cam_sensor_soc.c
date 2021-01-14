// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
 */

#include <linux/of.h>
#include <linux/of_gpio.h>
#include <cam_sensor_cmn_header.h>
#include <cam_sensor_util.h>
#include <cam_sensor_io.h>
#include <cam_req_mgr_util.h>
#include "cam_sensor_soc.h"
#include "cam_soc_util.h"

#if defined(CONFIG_CAMERA_SYSFS_V2)
extern char rear_cam_info[150];
extern char front_cam_info[150];
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
extern char front2_cam_info[150];
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
extern char front3_cam_info[150];
#else
extern char front2_cam_info[150];
#endif
#endif

#if defined(CONFIG_SAMSUNG_SECURE_CAMERA)
extern char iris_cam_info[150];
#endif
#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
extern char rear2_cam_info[150];
#endif
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
extern char rear3_cam_info[150];
#endif
#if defined(CONFIG_SAMSUNG_REAR_TOF)
extern char rear_tof_cam_info[150];
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOF)
extern char front_tof_cam_info[150];
#endif

int cam_sensor_get_dt_camera_info(struct device_node *of_node, char *buf)
{
	int rc = 0, val = 0;
	int size = 0;
	char camera_info[150] = {0, };
	char *isp_type[3] = { "INT;", "EXT;", "SOC;" };
	char *cal_mem_type[4] = { "N;", "Y;", "Y;", "Y;" };
	char *read_ver[2] = { "SYSFS;", "CAMON;" };
	char *core_volt[2] = { "N;", "Y;" };
	char *fw_upgrade[3] = { "N;", "SYSFS;", "CAMON;" };
	char *fw_write[4] = { "N;", "OIS;", "SD;", "ALL;" };
	char *fw_dump[2] = { "N;", "Y;" };
	char *companion[2] = { "N;", "Y;" };
	char *ois[2] = { "N;", "Y;" };
	char *valid[2] = { "N;", "Y;" };
	char *dual_open[2] = { "N;", "Y;" };

	rc = of_property_read_u32(of_node, "cam,isp",
			&val);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "failed");
		goto ERROR1;
	}
	strcpy(camera_info, "ISP=");
	size = sizeof(isp_type) / sizeof(char *);
	if ((val >= 0) && (val < size))
		strcat(camera_info, isp_type[val]);
	else
		strcat(camera_info, "NULL;");

	rc = of_property_read_u32(of_node, "cam,cal_memory",
			&val);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "failed");
		goto ERROR1;
	}
	strcat(camera_info, "CALMEM=");
	size = sizeof(cal_mem_type) / sizeof(char *);
	if ((val >= 0) && (val < size))
		strcat(camera_info, cal_mem_type[val]);
	else
		strcat(camera_info, "NULL;");

	rc = of_property_read_u32(of_node, "cam,read_version",
			&val);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "failed");
		goto ERROR1;
	}
	strcat(camera_info, "READVER=");
	size = sizeof(read_ver) / sizeof(char *);
	if ((val >= 0) && (val < size))
		strcat(camera_info, read_ver[val]);
	else
		strcat(camera_info, "NULL;");

	rc = of_property_read_u32(of_node, "cam,core_voltage",
			&val);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "failed");
		goto ERROR1;
	}
	strcat(camera_info, "COREVOLT=");
	size = sizeof(core_volt) / sizeof(char *);
	if ((val >= 0) && (val < size))
		strcat(camera_info, core_volt[val]);
	else
		strcat(camera_info, "NULL;");

	rc = of_property_read_u32(of_node, "cam,upgrade",
			&val);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "failed");
		goto ERROR1;
	}
	strcat(camera_info, "UPGRADE=");
	size = sizeof(fw_upgrade) / sizeof(char *);
	if ((val >= 0) && (val < size))
		strcat(camera_info, fw_upgrade[val]);
	else
		strcat(camera_info, "NULL;");

	rc = of_property_read_u32(of_node, "cam,fw_write",
			&val);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "failed");
		goto ERROR1;
	}
	strcat(camera_info, "FWWRITE=");
	size = sizeof(fw_write) / sizeof(char *);
	if ((val >= 0) && (val < size))
		strcat(camera_info, fw_write[val]);
	else
		strcat(camera_info, "NULL;");

	rc = of_property_read_u32(of_node, "cam,fw_dump",
			&val);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "failed");
		goto ERROR1;
	}
	strcat(camera_info, "FWDUMP=");
	size = sizeof(fw_dump) / sizeof(char *);
	if ((val >= 0) && (val < size))
		strcat(camera_info, fw_dump[val]);
	else
		strcat(camera_info, "NULL;");

	rc = of_property_read_u32(of_node, "cam,companion_chip",
			&val);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "failed");
		goto ERROR1;
	}
	strcat(camera_info, "CC=");
	size = sizeof(companion) / sizeof(char *);
	if ((val >= 0) && (val < size))
		strcat(camera_info, companion[val]);
	else
		strcat(camera_info, "NULL;");

	rc = of_property_read_u32(of_node, "cam,ois",
			&val);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "failed");
		goto ERROR1;
	}
	strcat(camera_info, "OIS=");
	size = sizeof(ois) / sizeof(char *);
	if ((val >= 0) && (val < size))
		strcat(camera_info, ois[val]);
	else
		strcat(camera_info, "NULL;");

	rc = of_property_read_u32(of_node, "cam,valid",
			&val);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "failed");
		goto ERROR1;
	}
	strcat(camera_info, "VALID=");
	size = sizeof(valid) / sizeof(char *);
	if ((val >= 0) && (val < size))
		strcat(camera_info, valid[val]);
	else
		strcat(camera_info, "NULL;");

	rc = of_property_read_u32(of_node, "cam,dual_open",
			&val);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "failed");
		goto ERROR1;
	}
	strcat(camera_info, "DUALOPEN=");
	size = sizeof(dual_open) / sizeof(char *);
	if ((val >= 0) && (val < size))
		strcat(camera_info, dual_open[val]);
	else
		strcat(camera_info, "NULL;");

	snprintf(buf, sizeof(camera_info), "%s", camera_info);
	return 0;

ERROR1:
	strcpy(camera_info, "ISP=NULL;CALMEM=NULL;READVER=NULL;COREVOLT=NULL;UPGRADE=NULL;FWWRITE=NULL;FWDUMP=NULL;FW_CC=NULL;OIS=NULL;DUALOPEN=NULL");
	snprintf(buf, sizeof(camera_info), "%s", camera_info);
	return 0;
}
#endif

int32_t cam_sensor_get_sub_module_index(struct device_node *of_node,
	struct cam_sensor_board_info *s_info)
{
	int rc = 0, i = 0;
	uint32_t val = 0;
	struct device_node *src_node = NULL;
	struct cam_sensor_board_info *sensor_info;

	sensor_info = s_info;

	for (i = 0; i < SUB_MODULE_MAX; i++)
		sensor_info->subdev_id[i] = -1;

	src_node = of_parse_phandle(of_node, "actuator-src", 0);
	if (!src_node) {
		CAM_DBG(CAM_SENSOR, "src_node NULL");
	} else {
		rc = of_property_read_u32(src_node, "cell-index", &val);
		CAM_DBG(CAM_SENSOR, "actuator cell index %d, rc %d", val, rc);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "failed %d", rc);
			of_node_put(src_node);
			return rc;
		}
		sensor_info->subdev_id[SUB_MODULE_ACTUATOR] = val;
		of_node_put(src_node);
	}

	src_node = of_parse_phandle(of_node, "ois-src", 0);
	if (!src_node) {
		CAM_DBG(CAM_SENSOR, "src_node NULL");
	} else {
		rc = of_property_read_u32(src_node, "cell-index", &val);
		CAM_DBG(CAM_SENSOR, " ois cell index %d, rc %d", val, rc);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "failed %d",  rc);
			of_node_put(src_node);
			return rc;
		}
		sensor_info->subdev_id[SUB_MODULE_OIS] = val;
		of_node_put(src_node);
	}

	src_node = of_parse_phandle(of_node, "eeprom-src", 0);
	if (!src_node) {
		CAM_DBG(CAM_SENSOR, "eeprom src_node NULL");
	} else {
		rc = of_property_read_u32(src_node, "cell-index", &val);
		CAM_DBG(CAM_SENSOR, "eeprom cell index %d, rc %d", val, rc);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "failed %d", rc);
			of_node_put(src_node);
			return rc;
		}
		sensor_info->subdev_id[SUB_MODULE_EEPROM] = val;
		of_node_put(src_node);
	}

	src_node = of_parse_phandle(of_node, "led-flash-src", 0);
	if (!src_node) {
		CAM_DBG(CAM_SENSOR, " src_node NULL");
	} else {
		rc = of_property_read_u32(src_node, "cell-index", &val);
		CAM_DBG(CAM_SENSOR, "led flash cell index %d, rc %d", val, rc);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "failed %d", rc);
			of_node_put(src_node);
			return rc;
		}
		sensor_info->subdev_id[SUB_MODULE_LED_FLASH] = val;
		of_node_put(src_node);
	}

	rc = of_property_read_u32(of_node, "csiphy-sd-index", &val);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR, "paring the dt node for csiphy rc %d", rc);
	else
		sensor_info->subdev_id[SUB_MODULE_CSIPHY] = val;

	return rc;
}

static int32_t cam_sensor_driver_get_dt_data(struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	int i = 0;
	struct cam_sensor_board_info *sensordata = NULL;
	struct device_node *of_node = s_ctrl->of_node;
	struct device_node *of_parent = NULL;
	struct cam_hw_soc_info *soc_info = &s_ctrl->soc_info;
   	uint32_t temp = 0;

	s_ctrl->sensordata = kzalloc(sizeof(*sensordata), GFP_KERNEL);
	if (!s_ctrl->sensordata)
		return -ENOMEM;

	sensordata = s_ctrl->sensordata;

	rc = cam_soc_util_get_dt_properties(soc_info);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Failed to read DT properties rc %d", rc);
		goto FREE_SENSOR_DATA;
	}

	rc =  cam_sensor_util_init_gpio_pin_tbl(soc_info,
			&sensordata->power_info.gpio_num_info);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Failed to read gpios %d", rc);
		goto FREE_SENSOR_DATA;
	}

	s_ctrl->id = soc_info->index;

	rc = of_property_read_u32(of_node, "slave-addr", &temp);
	sensordata->slave_info.sensor_slave_addr = (uint16_t)temp;
	if (rc < 0) {
		CAM_INFO(CAM_SENSOR, "No slave-addr found");
		rc = 0;
	}

	/* Validate cell_id */
	if (s_ctrl->id >= MAX_CAMERAS) {
		CAM_ERR(CAM_SENSOR, "Failed invalid cell_id %d", s_ctrl->id);
		rc = -EINVAL;
		goto FREE_SENSOR_DATA;
	}

	/* Store the index of BoB regulator if it is available */
	for (i = 0; i < soc_info->num_rgltr; i++) {
		if (!strcmp(soc_info->rgltr_name[i],
			"cam_bob")) {
			CAM_DBG(CAM_SENSOR,
				"i: %d cam_bob", i);
			s_ctrl->bob_reg_index = i;
			soc_info->rgltr[i] = devm_regulator_get(soc_info->dev,
				soc_info->rgltr_name[i]);
			if (IS_ERR_OR_NULL(soc_info->rgltr[i])) {
				CAM_WARN(CAM_SENSOR,
					"Regulator: %s get failed",
					soc_info->rgltr_name[i]);
				soc_info->rgltr[i] = NULL;
			} else {
				if (!of_property_read_bool(of_node,
					"pwm-switch")) {
					CAM_DBG(CAM_SENSOR,
					"No BoB PWM switch param defined");
					s_ctrl->bob_pwm_switch = false;
				} else {
					s_ctrl->bob_pwm_switch = true;
				}
			}
		}
	}

	/* Read subdev info */
	rc = cam_sensor_get_sub_module_index(of_node, sensordata);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "failed to get sub module index, rc=%d",
			 rc);
		goto FREE_SENSOR_DATA;
	}

	if (s_ctrl->io_master_info.master_type == CCI_MASTER) {
		/* Get CCI master */
		rc = of_property_read_u32(of_node, "cci-master",
			&s_ctrl->cci_i2c_master);
		CAM_DBG(CAM_SENSOR, "cci-master %d, rc %d",
			s_ctrl->cci_i2c_master, rc);
		if (rc < 0) {
			/* Set default master 0 */
			s_ctrl->cci_i2c_master = MASTER_0;
			rc = 0;
		}

		of_parent = of_get_parent(of_node);
		if (of_property_read_u32(of_parent, "cell-index",
				&s_ctrl->cci_num) < 0)
			/* Set default master 0 */
			s_ctrl->cci_num = CCI_DEVICE_0;

		CAM_DBG(CAM_SENSOR, "cci-index %d", s_ctrl->cci_num);
	}

	if (of_property_read_u32(of_node, "sensor-position-pitch",
		&sensordata->pos_pitch) < 0) {
		CAM_DBG(CAM_SENSOR, "Invalid sensor position");
		sensordata->pos_pitch = 360;
	}
	if (of_property_read_u32(of_node, "sensor-position-roll",
		&sensordata->pos_roll) < 0) {
		CAM_DBG(CAM_SENSOR, "Invalid sensor position");
		sensordata->pos_roll = 360;
	}
	if (of_property_read_u32(of_node, "sensor-position-yaw",
		&sensordata->pos_yaw) < 0) {
		CAM_DBG(CAM_SENSOR, "Invalid sensor position");
		sensordata->pos_yaw = 360;
	}

#if defined(CONFIG_CAMERA_SYSFS_V2)
	/* camera information */
	if (s_ctrl->id == CAMERA_0)
		rc = cam_sensor_get_dt_camera_info(of_node, rear_cam_info);
	else if (s_ctrl->id == CAMERA_1)
		rc = cam_sensor_get_dt_camera_info(of_node, front_cam_info);
#if defined(CONFIG_SAMSUNG_REAR_DUAL) || defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	else if (s_ctrl->id == CAMERA_2)
		rc = cam_sensor_get_dt_camera_info(of_node, rear2_cam_info);
#endif
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	else if (s_ctrl->id == CAMERA_3)
		rc = cam_sensor_get_dt_camera_info(of_node, rear3_cam_info);
#endif
#if defined(CONFIG_SAMSUNG_REAR_TOF)
	else if (s_ctrl->id == CAMERA_4)
		rc = cam_sensor_get_dt_camera_info(of_node, rear_tof_cam_info);
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOF)
	else if (s_ctrl->id == CAMERA_5)
		rc = cam_sensor_get_dt_camera_info(of_node, front_tof_cam_info);
#endif
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
	else if (s_ctrl->id == CAMERA_8)
		rc = cam_sensor_get_dt_camera_info(of_node, front2_cam_info);
#endif
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
	else if (s_ctrl->id == CAMERA_11) {
#if defined(CONFIG_SAMSUNG_FRONT_DUAL)
		rc = cam_sensor_get_dt_camera_info(of_node, front3_cam_info);
#else
		rc = cam_sensor_get_dt_camera_info(of_node, front2_cam_info);
#endif
    }
#endif
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "failed: cam_sensor_get_dt_camera_info for cell-index %d rc %d", s_ctrl->id, rc);
		goto FREE_SENSOR_DATA;
	}
#endif

	return rc;

FREE_SENSOR_DATA:
	kfree(sensordata);
	return rc;
}

int32_t msm_sensor_init_default_params(struct cam_sensor_ctrl_t *s_ctrl)
{
	/* Validate input parameters */
	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "failed: invalid params s_ctrl %pK",
			s_ctrl);
		return -EINVAL;
	}

	CAM_DBG(CAM_SENSOR,
		"master_type: %d", s_ctrl->io_master_info.master_type);
	/* Initialize cci_client */
	if (s_ctrl->io_master_info.master_type == CCI_MASTER) {
		s_ctrl->io_master_info.cci_client = kzalloc(sizeof(
			struct cam_sensor_cci_client), GFP_KERNEL);
		if (!(s_ctrl->io_master_info.cci_client))
			return -ENOMEM;

		s_ctrl->io_master_info.cci_client->cci_device
			= s_ctrl->cci_num;
	} else if (s_ctrl->io_master_info.master_type == I2C_MASTER) {
		if (!(s_ctrl->io_master_info.client))
			return -EINVAL;
	} else {
		CAM_ERR(CAM_SENSOR,
			"Invalid master / Master type Not supported");
		return -EINVAL;
	}

	return 0;
}

int32_t cam_sensor_parse_dt(struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t i, rc = 0;
	struct cam_hw_soc_info *soc_info = &s_ctrl->soc_info;

	/* Parse dt information and store in sensor control structure */
	rc = cam_sensor_driver_get_dt_data(s_ctrl);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Failed to get dt data rc %d", rc);
		return rc;
	}

	/* Initialize mutex */
	mutex_init(&(s_ctrl->cam_sensor_mutex));

	/* Initialize default parameters */
	for (i = 0; i < soc_info->num_clk; i++) {
		soc_info->clk[i] = devm_clk_get(soc_info->dev,
					soc_info->clk_name[i]);
		if (!soc_info->clk[i]) {
			CAM_ERR(CAM_SENSOR, "get failed for %s",
				 soc_info->clk_name[i]);
			rc = -ENOENT;
			return rc;
		}
	}
	rc = msm_sensor_init_default_params(s_ctrl);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR,
			"failed: msm_sensor_init_default_params rc %d", rc);
		goto FREE_DT_DATA;
	}

	return rc;

FREE_DT_DATA:
	kfree(s_ctrl->sensordata);
	s_ctrl->sensordata = NULL;

	return rc;
}
