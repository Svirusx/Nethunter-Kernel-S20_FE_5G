// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
 */

#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/dma-contiguous.h>
#include <cam_sensor_cmn_header.h>
#include "cam_ois_core.h"
#include "cam_ois_soc.h"
#include "cam_sensor_util.h"
#include "cam_debug_util.h"
#include "cam_res_mgr_api.h"
#include "cam_common_util.h"
#include "cam_packet_util.h"

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32)
#include "cam_ois_mcu_stm32g.h"
#elif defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
#include "cam_ois_rumba_s4.h"
#endif
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
#include "cam_ois_thread.h"
#include <linux/slab.h>

extern int ois_reset_register(struct ois_sensor_interface *ois);

static struct cam_sensor_power_setting default_power_setting[] =
{
	//seq_type,		seq_val,	config_val,	delay,
#if defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
	{SENSOR_VIO,            CAM_VIO,        1,      	1,      	{}},
	{SENSOR_CUSTOM_REG1,	CAM_V_CUSTOM1,	1,		1,		{}},
	{SENSOR_CUSTOM_REG2,	CAM_V_CUSTOM2,	1,		1,		{}},
	{SENSOR_CUSTOM_REG3,	CAM_V_CUSTOM3,	0,		1,		{}},
	{SENSOR_MCLK,		0,		19200000,	1,		{}},
	{SENSOR_RESET,		CAM_VREG_MAX,	1,		10,		{}},
#else
	{SENSOR_VAF,		CAM_VAF,	1,		1,		{}},
	{SENSOR_VDIG,		CAM_VDIG,	1,		1,		{}},
#if defined(CONFIG_SEC_Z3Q_PROJECT)
	{SENSOR_CUSTOM_REG1,	CAM_V_CUSTOM1,	1,		1,		{}},
	{SENSOR_CUSTOM_REG2,	CAM_V_CUSTOM2,	1,		1,		{}},
	{SENSOR_CUSTOM_REG3,	CAM_V_CUSTOM3,	0,		1,		{}},
	{SENSOR_CUSTOM_GPIO2,	CAM_VREG_MAX,	1,		1,		{}},
#elif defined(CONFIG_SEC_C2Q_PROJECT)
	{SENSOR_CUSTOM_REG1,	CAM_V_CUSTOM1,	1,		1,		{}},
	{SENSOR_CUSTOM_REG2,	CAM_V_CUSTOM2,	1,		1,		{}},
	{SENSOR_CUSTOM_REG3,	CAM_V_CUSTOM3,	0,		2,		{}},
	{SENSOR_MCLK,		0,		19200000,	1,		{}},
	{SENSOR_CUSTOM_GPIO2,	CAM_VREG_MAX,	1,		1,		{}},
#elif defined(CONFIG_SEC_F2Q_PROJECT) || defined(CONFIG_SEC_VICTORY_PROJECT)
	{SENSOR_CUSTOM_REG1,	CAM_V_CUSTOM1,	1,		1,		{}},
#endif
	{SENSOR_VIO,		CAM_VIO,	1,		1,		{}},
	{SENSOR_RESET,		CAM_VREG_MAX,	1,		10,		{}},
	{SENSOR_CUSTOM_GPIO1,	CAM_VREG_MAX,	0,		1,		{}},
#endif
};

static struct cam_sensor_power_setting default_power_down_setting[] =
{
	//seq_type,		seq_val,	config_val,	delay,
#if defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
	{SENSOR_RESET,		CAM_VREG_MAX,	0,		2,		{}},
	{SENSOR_MCLK,		0,		0,		1,		{}},
	{SENSOR_CUSTOM_REG3,	CAM_V_CUSTOM3,	0,		0,		{}},
	{SENSOR_CUSTOM_REG2,	CAM_V_CUSTOM2,	0,		0,		{}},
	{SENSOR_CUSTOM_REG1,	CAM_V_CUSTOM1,	0,		0,		{}},
	{SENSOR_VIO,            CAM_VIO,        0, 		0,      	{}},
#else
	{SENSOR_CUSTOM_GPIO1,	CAM_VREG_MAX,	0,		0,		{}},
	{SENSOR_RESET,		CAM_VREG_MAX,	0,		0,		{}},
#if defined(CONFIG_SEC_C2Q_PROJECT)
	{SENSOR_VIO,		CAM_VIO,	0,		1,		{}},
#else
	{SENSOR_VIO,            CAM_VIO,        0,              0,              {}},
#endif
#if defined(CONFIG_SEC_Z3Q_PROJECT)
	{SENSOR_CUSTOM_GPIO2,	CAM_VREG_MAX,	0,		0,		{}},
	{SENSOR_CUSTOM_REG3,	CAM_V_CUSTOM3,	0,		0,		{}},
	{SENSOR_CUSTOM_REG2,	CAM_V_CUSTOM2,	0,		0,		{}},
	{SENSOR_CUSTOM_REG1,	CAM_V_CUSTOM1,	0,		0,		{}},
#elif defined(CONFIG_SEC_C2Q_PROJECT)
	{SENSOR_CUSTOM_GPIO2,	CAM_VREG_MAX,	0,		0,		{}},
	{SENSOR_MCLK,		0,		0,		0,		{}},
	{SENSOR_CUSTOM_REG3,	CAM_V_CUSTOM3,	0,		0,		{}},
	{SENSOR_CUSTOM_REG2,	CAM_V_CUSTOM2,	0,		0,		{}},
	{SENSOR_CUSTOM_REG1,	CAM_V_CUSTOM1,	0,		0,		{}},
#elif defined(CONFIG_SEC_F2Q_PROJECT) || defined(CONFIG_SEC_VICTORY_PROJECT)
	{SENSOR_CUSTOM_REG1,	CAM_V_CUSTOM1,	0,		0,		{}},
#endif
	{SENSOR_VDIG,		CAM_VDIG,	0,		0,		{}},
	{SENSOR_VAF,		CAM_VAF,	0,		0,		{}},
#endif
};
#endif

int32_t cam_ois_construct_default_power_setting(
	struct cam_sensor_power_ctrl_t *power_info)
{
	int rc = 0;

	power_info->power_setting_size = 1;
	power_info->power_setting =
		kzalloc(sizeof(struct cam_sensor_power_setting) * MAX_POWER_CONFIG,
			GFP_KERNEL);
	if (!power_info->power_setting)
		return -ENOMEM;

	power_info->power_setting[0].seq_type = SENSOR_VAF;
	power_info->power_setting[0].seq_val = CAM_VAF;
	power_info->power_setting[0].config_val = 1;
	power_info->power_setting[0].delay = 2;

	power_info->power_down_setting_size = 1;
	power_info->power_down_setting =
		kzalloc(sizeof(struct cam_sensor_power_setting) * MAX_POWER_CONFIG,
			GFP_KERNEL);
	if (!power_info->power_down_setting) {
		rc = -ENOMEM;
		goto free_power_settings;
	}

	power_info->power_down_setting[0].seq_type = SENSOR_VAF;
	power_info->power_down_setting[0].seq_val = CAM_VAF;
	power_info->power_down_setting[0].config_val = 0;

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
	memcpy(power_info->power_setting,
		default_power_setting,
		sizeof(default_power_setting));
	power_info->power_setting_size =
        ARRAY_SIZE(default_power_setting);

	memcpy(power_info->power_down_setting,
		default_power_down_setting,
		sizeof(default_power_down_setting));
	power_info->power_down_setting_size =
        ARRAY_SIZE(default_power_down_setting);
#endif

	return rc;

free_power_settings:
	kfree(power_info->power_setting);
	power_info->power_setting = NULL;
	power_info->power_setting_size = 0;
	return rc;
}


/**
 * cam_ois_get_dev_handle - get device handle
 * @o_ctrl:     ctrl structure
 * @arg:        Camera control command argument
 *
 * Returns success or failure
 */
static int cam_ois_get_dev_handle(struct cam_ois_ctrl_t *o_ctrl,
	void *arg)
{
	struct cam_sensor_acquire_dev    ois_acq_dev;
	struct cam_create_dev_hdl        bridge_params;
	struct cam_control              *cmd = (struct cam_control *)arg;
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
	int i = 0, idx = -1;

	if (o_ctrl->bridge_cnt >= MAX_BRIDGE_COUNT) {
		CAM_ERR(CAM_OIS, "Device is already max acquired");
		return -EFAULT;
	}

	for (i = 0; i < MAX_BRIDGE_COUNT; i++) {
		if (o_ctrl->bridge_intf[i].device_hdl == -1) {
			idx = i;
			break;
		}
	}

	if (idx == -1) {
		CAM_ERR(CAM_OIS, "All Device(%d) is already acquired", o_ctrl->bridge_cnt);
		return -EFAULT;
	}
#else
	if (o_ctrl->bridge_intf.device_hdl != -1) {
		CAM_ERR(CAM_OIS, "Device is already acquired");
		return -EFAULT;
	}
#endif

	if (copy_from_user(&ois_acq_dev, u64_to_user_ptr(cmd->handle),
		sizeof(ois_acq_dev)))
		return -EFAULT;

	bridge_params.session_hdl = ois_acq_dev.session_handle;
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
	bridge_params.ops = &o_ctrl->bridge_intf[idx].ops;
#else
	bridge_params.ops = &o_ctrl->bridge_intf.ops;
#endif
	bridge_params.v4l2_sub_dev_flag = 0;
	bridge_params.media_entity_flag = 0;
	bridge_params.priv = o_ctrl;

	ois_acq_dev.device_handle =
		cam_create_device_hdl(&bridge_params);
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
	o_ctrl->bridge_intf[idx].device_hdl = ois_acq_dev.device_handle;
	o_ctrl->bridge_intf[idx].session_hdl = ois_acq_dev.session_handle;
	o_ctrl->bridge_cnt++;
#else
	o_ctrl->bridge_intf.device_hdl = ois_acq_dev.device_handle;
	o_ctrl->bridge_intf.session_hdl = ois_acq_dev.session_handle;
#endif

	CAM_DBG(CAM_OIS, "Device Handle: %d", ois_acq_dev.device_handle);
	if (copy_to_user(u64_to_user_ptr(cmd->handle), &ois_acq_dev,
		sizeof(struct cam_sensor_acquire_dev))) {
		CAM_ERR(CAM_OIS, "ACQUIRE_DEV: copy to user failed");
		return -EFAULT;
	}
	return 0;
}

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
/**
 * cam_ois_release_dev_handle - get device handle
 * @o_ctrl:     ctrl structure
 * @arg:        Camera control command argument
 *
 * Returns success or failure
 */
static int cam_ois_release_dev_handle(struct cam_ois_ctrl_t *o_ctrl,
	void *arg)
{
	struct cam_control				*cmd = (struct cam_control *)arg;
	struct cam_sensor_release_dev	 ois_rel_dev;
	int i = 0, rc = 0;

	if (!o_ctrl || !arg) {
		CAM_INFO(CAM_OIS, "Invalid argument");
		return -EINVAL;
	}

	if (copy_from_user(&ois_rel_dev, u64_to_user_ptr(cmd->handle),
		sizeof(struct cam_sensor_release_dev)))
		return -EFAULT;

	for (i = 0; i < MAX_BRIDGE_COUNT; i++) {
		if (o_ctrl->bridge_intf[i].device_hdl == -1)
			continue;

		if ((o_ctrl->bridge_intf[i].device_hdl == ois_rel_dev.device_handle) &&
			(o_ctrl->bridge_intf[i].session_hdl == ois_rel_dev.session_handle)) {
			CAM_INFO(CAM_OIS, "Release the device hdl %d", o_ctrl->bridge_intf[i].device_hdl);
			rc = cam_destroy_device_hdl(o_ctrl->bridge_intf[i].device_hdl);
			if (rc < 0)
				CAM_ERR(CAM_OIS, "fail destroying the device hdl");
			o_ctrl->bridge_intf[i].device_hdl = -1;
			o_ctrl->bridge_intf[i].link_hdl = -1;
			o_ctrl->bridge_intf[i].session_hdl = -1;

			if (o_ctrl->bridge_cnt > 0)
				o_ctrl->bridge_cnt--;
			break;
		}
	}

	return 0;
}
#endif

int cam_ois_power_up(struct cam_ois_ctrl_t *o_ctrl)
{
	int                             rc = 0;
	struct cam_hw_soc_info          *soc_info =
		&o_ctrl->soc_info;
	struct cam_ois_soc_private *soc_private;
	struct cam_sensor_power_ctrl_t  *power_info;


#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
	if (o_ctrl->is_power_up)
		return 0;
#endif

	soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	power_info = &soc_private->power_info;

	if ((power_info->power_setting == NULL) &&
		(power_info->power_down_setting == NULL)) {
		CAM_INFO(CAM_OIS,
			"Using default power settings");
		rc = cam_ois_construct_default_power_setting(power_info);
		if (rc < 0) {
			CAM_ERR(CAM_OIS,
				"Construct default ois power setting failed.");
			return rc;
		}
	}

	/* Parse and fill vreg params for power up settings */
	rc = msm_camera_fill_vreg_params(
		soc_info,
		power_info->power_setting,
		power_info->power_setting_size);
	if (rc) {
		CAM_ERR(CAM_OIS,
			"failed to fill vreg params for power up rc:%d", rc);
		return rc;
	}

	/* Parse and fill vreg params for power down settings*/
	rc = msm_camera_fill_vreg_params(
		soc_info,
		power_info->power_down_setting,
		power_info->power_down_setting_size);
	if (rc) {
		CAM_ERR(CAM_OIS,
			"failed to fill vreg params for power down rc:%d", rc);
		return rc;
	}

	power_info->dev = soc_info->dev;

	rc = cam_sensor_core_power_up(power_info, soc_info);
	if (rc) {
		CAM_ERR(CAM_OIS, "failed in ois power up rc %d", rc);
		return rc;
	}

	rc = camera_io_init(&o_ctrl->io_master_info);
	if (rc)
		CAM_ERR(CAM_OIS, "cci_init failed: rc: %d", rc);

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
	o_ctrl->is_power_up = true;
#endif

	return rc;
}

/**
 * cam_ois_power_down - power down OIS device
 * @o_ctrl:     ctrl structure
 *
 * Returns success or failure
 */
int cam_ois_power_down(struct cam_ois_ctrl_t *o_ctrl)
{
	int32_t                         rc = 0;
	struct cam_sensor_power_ctrl_t  *power_info;
	struct cam_hw_soc_info          *soc_info =
		&o_ctrl->soc_info;
	struct cam_ois_soc_private *soc_private;

	if (!o_ctrl) {
		CAM_ERR(CAM_OIS, "failed: o_ctrl %pK", o_ctrl);
		return -EINVAL;
	}

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
	if (!o_ctrl->is_power_up)
		return 0;
	o_ctrl->is_power_up = false;
	o_ctrl->is_servo_on = false;
	o_ctrl->is_config = false;
#endif

	soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	power_info = &soc_private->power_info;
	soc_info = &o_ctrl->soc_info;

	if (!power_info) {
		CAM_ERR(CAM_OIS, "failed: power_info %pK", power_info);
		return -EINVAL;
	}

	rc = cam_sensor_util_power_down(power_info, soc_info);
	if (rc) {
		CAM_ERR(CAM_OIS, "power down the core is failed:%d", rc);
		return rc;
	}

	camera_io_release(&o_ctrl->io_master_info);

	return rc;
}

int cam_ois_apply_settings(struct cam_ois_ctrl_t *o_ctrl,
	struct i2c_settings_array *i2c_set)
{
	struct i2c_settings_list *i2c_list;
	int32_t rc = 0;
	uint32_t i, size;

	if (o_ctrl == NULL || i2c_set == NULL) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	if (i2c_set->is_settings_valid != 1) {
		CAM_ERR(CAM_OIS, " Invalid settings");
		return -EINVAL;
	}

	list_for_each_entry(i2c_list,
		&(i2c_set->list_head), list) {
		if (i2c_list->op_code ==  CAM_SENSOR_I2C_WRITE_RANDOM) {
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
			if ((i2c_list->i2c_settings.size == 1) &&
				(i2c_list->i2c_settings.addr_type == CAMERA_SENSOR_I2C_TYPE_INVALID) &&
				(i2c_list->i2c_settings.data_type == CAMERA_SENSOR_I2C_TYPE_INVALID))
				continue;
#endif

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
			size = i2c_list->i2c_settings.size;
			for (i = 0; i < size; i++) {
				if (i2c_list->i2c_settings.reg_setting[i].reg_addr == 0x02) {
					rc = cam_ois_set_ois_mode(o_ctrl,
						i2c_list->i2c_settings.reg_setting[i].reg_data);
					return rc;
				}
			}
#endif
			rc = camera_io_dev_write(&(o_ctrl->io_master_info),
				&(i2c_list->i2c_settings));
			if (rc < 0) {
				CAM_ERR(CAM_OIS,
					"Failed in Applying i2c wrt settings");
				return rc;
			}
		} else if (i2c_list->op_code == CAM_SENSOR_I2C_POLL) {
			size = i2c_list->i2c_settings.size;
			for (i = 0; i < size; i++) {
				rc = camera_io_dev_poll(
				&(o_ctrl->io_master_info),
				i2c_list->i2c_settings.reg_setting[i].reg_addr,
				i2c_list->i2c_settings.reg_setting[i].reg_data,
				i2c_list->i2c_settings.reg_setting[i].data_mask,
				i2c_list->i2c_settings.addr_type,
				i2c_list->i2c_settings.data_type,
				i2c_list->i2c_settings.reg_setting[i].delay);
				if (rc < 0) {
					CAM_ERR(CAM_OIS,
						"i2c poll apply setting Fail");
					return rc;
				}
			}
		}
	}

	return rc;
}

static int cam_ois_slaveInfo_pkt_parser(struct cam_ois_ctrl_t *o_ctrl,
	uint32_t *cmd_buf, size_t len)
{
	int32_t rc = 0;
	struct cam_cmd_ois_info *ois_info;

	if (!o_ctrl || !cmd_buf || len < sizeof(struct cam_cmd_ois_info)) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	ois_info = (struct cam_cmd_ois_info *)cmd_buf;
	if (o_ctrl->io_master_info.master_type == CCI_MASTER) {
		o_ctrl->io_master_info.cci_client->i2c_freq_mode =
			ois_info->i2c_freq_mode;
		o_ctrl->io_master_info.cci_client->sid =
			ois_info->slave_addr >> 1;
		o_ctrl->ois_fw_flag = ois_info->ois_fw_flag;
		o_ctrl->is_ois_calib = ois_info->is_ois_calib;
		memcpy(o_ctrl->ois_name, ois_info->ois_name, OIS_NAME_LEN);
		o_ctrl->ois_name[OIS_NAME_LEN - 1] = '\0';
		o_ctrl->io_master_info.cci_client->retries = 3;
		o_ctrl->io_master_info.cci_client->id_map = 0;
		memcpy(&(o_ctrl->opcode), &(ois_info->opcode),
			sizeof(struct cam_ois_opcode));
		CAM_DBG(CAM_OIS, "Slave addr: 0x%x Freq Mode: %d",
			ois_info->slave_addr, ois_info->i2c_freq_mode);
	} else if (o_ctrl->io_master_info.master_type == I2C_MASTER) {
		o_ctrl->io_master_info.client->addr = ois_info->slave_addr;
		CAM_DBG(CAM_OIS, "Slave addr: 0x%x", ois_info->slave_addr);
	} else {
		CAM_ERR(CAM_OIS, "Invalid Master type : %d",
			o_ctrl->io_master_info.master_type);
		rc = -EINVAL;
	}

	return rc;
}

#if !defined(CONFIG_SAMSUNG_OIS_MCU_STM32) && !defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
static int cam_ois_fw_download(struct cam_ois_ctrl_t *o_ctrl)
{
	uint16_t                           total_bytes = 0;
	uint8_t                           *ptr = NULL;
	int32_t                            rc = 0, cnt;
	uint32_t                           fw_size;
	const struct firmware             *fw = NULL;
	const char                        *fw_name_prog = NULL;
	const char                        *fw_name_coeff = NULL;
	char                               name_prog[32] = {0};
	char                               name_coeff[32] = {0};
	struct device                     *dev = NULL;
	struct cam_sensor_i2c_reg_setting  i2c_reg_setting;
	struct page                       *page = NULL;

	if (!o_ctrl) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

        dev = &(o_ctrl->pdev->dev);
	snprintf(name_coeff, 32, "%s.coeff", o_ctrl->ois_name);

	snprintf(name_prog, 32, "%s.prog", o_ctrl->ois_name);

	/* cast pointer as const pointer*/
	fw_name_prog = name_prog;
	fw_name_coeff = name_coeff;

	/* Load FW */
	rc = request_firmware(&fw, fw_name_prog, dev);
	if (rc) {
		CAM_ERR(CAM_OIS, "Failed to locate %s", fw_name_prog);
		return rc;
	}

	total_bytes = fw->size;
	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.size = total_bytes;
	i2c_reg_setting.delay = 0;
	fw_size = PAGE_ALIGN(sizeof(struct cam_sensor_i2c_reg_array) *
		total_bytes) >> PAGE_SHIFT;
	page = cma_alloc(dev_get_cma_area((o_ctrl->soc_info.dev)),
		fw_size, 0, GFP_KERNEL);
	if (!page) {
		CAM_ERR(CAM_OIS, "Failed in allocating i2c_array");
		release_firmware(fw);
		return -ENOMEM;
	}

	i2c_reg_setting.reg_setting = (struct cam_sensor_i2c_reg_array *) (
		page_address(page));

	for (cnt = 0, ptr = (uint8_t *)fw->data; cnt < total_bytes;
		cnt++, ptr++) {
		i2c_reg_setting.reg_setting[cnt].reg_addr =
			o_ctrl->opcode.prog;
		i2c_reg_setting.reg_setting[cnt].reg_data = *ptr;
		i2c_reg_setting.reg_setting[cnt].delay = 0;
		i2c_reg_setting.reg_setting[cnt].data_mask = 0;
	}

	rc = camera_io_dev_write_continuous(&(o_ctrl->io_master_info),
		&i2c_reg_setting, 1);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "OIS FW download failed %d", rc);
		goto release_firmware;
	}
	cma_release(dev_get_cma_area((o_ctrl->soc_info.dev)),
		page, fw_size);
	page = NULL;
	fw_size = 0;
	release_firmware(fw);

	rc = request_firmware(&fw, fw_name_coeff, dev);
	if (rc) {
		CAM_ERR(CAM_OIS, "Failed to locate %s", fw_name_coeff);
		return rc;
	}

	total_bytes = fw->size;
	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.size = total_bytes;
	i2c_reg_setting.delay = 0;
	fw_size = PAGE_ALIGN(sizeof(struct cam_sensor_i2c_reg_array) *
		total_bytes) >> PAGE_SHIFT;
	page = cma_alloc(dev_get_cma_area((o_ctrl->soc_info.dev)),
		fw_size, 0, GFP_KERNEL);
	if (!page) {
		CAM_ERR(CAM_OIS, "Failed in allocating i2c_array");
		release_firmware(fw);
		return -ENOMEM;
	}

	i2c_reg_setting.reg_setting = (struct cam_sensor_i2c_reg_array *) (
		page_address(page));

	for (cnt = 0, ptr = (uint8_t *)fw->data; cnt < total_bytes;
		cnt++, ptr++) {
		i2c_reg_setting.reg_setting[cnt].reg_addr =
			o_ctrl->opcode.coeff;
		i2c_reg_setting.reg_setting[cnt].reg_data = *ptr;
		i2c_reg_setting.reg_setting[cnt].delay = 0;
		i2c_reg_setting.reg_setting[cnt].data_mask = 0;
	}

	rc = camera_io_dev_write_continuous(&(o_ctrl->io_master_info),
		&i2c_reg_setting, 1);
	if (rc < 0)
		CAM_ERR(CAM_OIS, "OIS FW download failed %d", rc);

release_firmware:
	cma_release(dev_get_cma_area((o_ctrl->soc_info.dev)),
		page, fw_size);
	release_firmware(fw);

	return rc;
}
#endif

/**
 * cam_ois_pkt_parse - Parse csl packet
 * @o_ctrl:     ctrl structure
 * @arg:        Camera control command argument
 *
 * Returns success or failure
 */
static int cam_ois_pkt_parse(struct cam_ois_ctrl_t *o_ctrl, void *arg)
{
	int32_t                         rc = 0;
	int32_t                         i = 0;
	uint32_t                        total_cmd_buf_in_bytes = 0;
	struct common_header           *cmm_hdr = NULL;
	uintptr_t                       generic_ptr;
	struct cam_control             *ioctl_ctrl = NULL;
	struct cam_config_dev_cmd       dev_config;
	struct i2c_settings_array      *i2c_reg_settings = NULL;
	struct cam_cmd_buf_desc        *cmd_desc = NULL;
	uintptr_t                       generic_pkt_addr;
	size_t                          pkt_len;
	size_t                          remain_len = 0;
	struct cam_packet              *csl_packet = NULL;
	size_t                          len_of_buff = 0;
	uint32_t                       *offset = NULL, *cmd_buf;
	struct cam_ois_soc_private     *soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	struct cam_sensor_power_ctrl_t  *power_info = &soc_private->power_info;
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
	struct cam_ois_thread_msg_t    *msg = NULL;
#endif

	ioctl_ctrl = (struct cam_control *)arg;
	if (copy_from_user(&dev_config,
		u64_to_user_ptr(ioctl_ctrl->handle),
		sizeof(dev_config)))
		return -EFAULT;
	rc = cam_mem_get_cpu_buf(dev_config.packet_handle,
		&generic_pkt_addr, &pkt_len);
	if (rc) {
		CAM_ERR(CAM_OIS,
			"error in converting command Handle Error: %d", rc);
		return rc;
	}

	remain_len = pkt_len;
	if ((sizeof(struct cam_packet) > pkt_len) ||
		((size_t)dev_config.offset >= pkt_len -
		sizeof(struct cam_packet))) {
		CAM_ERR(CAM_OIS,
			"Inval cam_packet strut size: %zu, len_of_buff: %zu",
			 sizeof(struct cam_packet), pkt_len);
		return -EINVAL;
	}

	remain_len -= (size_t)dev_config.offset;
	csl_packet = (struct cam_packet *)
		(generic_pkt_addr + (uint32_t)dev_config.offset);

	if (cam_packet_util_validate_packet(csl_packet,
		remain_len)) {
		CAM_ERR(CAM_OIS, "Invalid packet params");
		return -EINVAL;
	}


	switch (csl_packet->header.op_code & 0xFFFFFF) {
	case CAM_OIS_PACKET_OPCODE_INIT:
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
		if (o_ctrl->is_config)
			return rc;
#endif
		offset = (uint32_t *)&csl_packet->payload;
		offset += (csl_packet->cmd_buf_offset / sizeof(uint32_t));
		cmd_desc = (struct cam_cmd_buf_desc *)(offset);

		/* Loop through multiple command buffers */
		for (i = 0; i < csl_packet->num_cmd_buf; i++) {
			total_cmd_buf_in_bytes = cmd_desc[i].length;
			if (!total_cmd_buf_in_bytes)
				continue;

			rc = cam_mem_get_cpu_buf(cmd_desc[i].mem_handle,
				&generic_ptr, &len_of_buff);
			if (rc < 0) {
				CAM_ERR(CAM_OIS, "Failed to get cpu buf : 0x%x",
					cmd_desc[i].mem_handle);
				return rc;
			}
			cmd_buf = (uint32_t *)generic_ptr;
			if (!cmd_buf) {
				CAM_ERR(CAM_OIS, "invalid cmd buf");
				return -EINVAL;
			}

			if ((len_of_buff < sizeof(struct common_header)) ||
				(cmd_desc[i].offset > (len_of_buff -
				sizeof(struct common_header)))) {
				CAM_ERR(CAM_OIS,
					"Invalid length for sensor cmd");
				return -EINVAL;
			}
			remain_len = len_of_buff - cmd_desc[i].offset;
			cmd_buf += cmd_desc[i].offset / sizeof(uint32_t);
			cmm_hdr = (struct common_header *)cmd_buf;

			switch (cmm_hdr->cmd_type) {
			case CAMERA_SENSOR_CMD_TYPE_I2C_INFO:
				rc = cam_ois_slaveInfo_pkt_parser(
					o_ctrl, cmd_buf, remain_len);
				if (rc < 0) {
					CAM_ERR(CAM_OIS,
					"Failed in parsing slave info");
					return rc;
				}
				break;
			case CAMERA_SENSOR_CMD_TYPE_PWR_UP:
			case CAMERA_SENSOR_CMD_TYPE_PWR_DOWN:
				CAM_DBG(CAM_OIS,
					"Received power settings buffer");
				rc = cam_sensor_update_power_settings(
					cmd_buf,
					total_cmd_buf_in_bytes,
					power_info, remain_len);
				if (rc) {
					CAM_ERR(CAM_OIS,
					"Failed: parse power settings");
					return rc;
				}
				break;
			default:
			if (o_ctrl->i2c_init_data.is_settings_valid == 0) {
				CAM_DBG(CAM_OIS,
				"Received init settings");
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
				mutex_lock(&(o_ctrl->i2c_init_data_mutex));
#endif
				i2c_reg_settings =
					&(o_ctrl->i2c_init_data);
				i2c_reg_settings->is_settings_valid = 1;
				i2c_reg_settings->request_id = 0;
				rc = cam_sensor_i2c_command_parser(
					&o_ctrl->io_master_info,
					i2c_reg_settings,
					&cmd_desc[i], 1);
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
				mutex_unlock(&(o_ctrl->i2c_init_data_mutex));
#endif
				if (rc < 0) {
					CAM_ERR(CAM_OIS,
					"init parsing failed: %d", rc);
					return rc;
				}
			} else if ((o_ctrl->is_ois_calib != 0) &&
				(o_ctrl->i2c_calib_data.is_settings_valid ==
				0)) {
				CAM_DBG(CAM_OIS,
					"Received calib settings");
				i2c_reg_settings = &(o_ctrl->i2c_calib_data);
				i2c_reg_settings->is_settings_valid = 1;
				i2c_reg_settings->request_id = 0;
				rc = cam_sensor_i2c_command_parser(
					&o_ctrl->io_master_info,
					i2c_reg_settings,
					&cmd_desc[i], 1);
				if (rc < 0) {
					CAM_ERR(CAM_OIS,
						"Calib parsing failed: %d", rc);
					return rc;
				}
			}
			break;
			}
		}

		if (o_ctrl->cam_ois_state != CAM_OIS_CONFIG) {
			rc = cam_ois_power_up(o_ctrl);
			if (rc) {
				CAM_ERR(CAM_OIS, " OIS Power up failed");
				return rc;
			}
			o_ctrl->cam_ois_state = CAM_OIS_CONFIG;
		}

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
		o_ctrl->ois_mode = 0;

		rc = cam_ois_thread_create(o_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "Failed create OIS thread");
			goto pwr_dwn;
		}

		msg = kmalloc(sizeof(struct cam_ois_thread_msg_t), GFP_KERNEL);
		if (msg == NULL) {
			CAM_ERR(CAM_OIS, "Failed alloc memory for msg, Out of memory");
			goto pwr_dwn;
		}

		memset(msg, 0, sizeof(struct cam_ois_thread_msg_t));
		msg->msg_type = CAM_OIS_THREAD_MSG_START;
		rc = cam_ois_thread_add_msg(o_ctrl, msg);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "Failed add msg to OIS thread");
			goto pwr_dwn;
		}
		o_ctrl->is_config = true;
#else
		if (o_ctrl->ois_fw_flag) {
			rc = cam_ois_fw_download(o_ctrl);
			if (rc) {
				CAM_ERR(CAM_OIS, "Failed OIS FW Download");
				goto pwr_dwn;
			}
		}

		rc = cam_ois_apply_settings(o_ctrl, &o_ctrl->i2c_init_data);
		if ((rc == -EAGAIN) &&
			(o_ctrl->io_master_info.master_type == CCI_MASTER)) {
			CAM_WARN(CAM_OIS,
				"CCI HW is restting: Reapplying INIT settings");
			usleep_range(1000, 1010);
			rc = cam_ois_apply_settings(o_ctrl,
				&o_ctrl->i2c_init_data);
		}
		if (rc < 0) {
			CAM_ERR(CAM_OIS,
				"Cannot apply Init settings: rc = %d",
				rc);
			goto pwr_dwn;
		}

		if (o_ctrl->is_ois_calib) {
			rc = cam_ois_apply_settings(o_ctrl,
				&o_ctrl->i2c_calib_data);
			if (rc) {
				CAM_ERR(CAM_OIS, "Cannot apply calib data");
				goto pwr_dwn;
			}
		}

		rc = delete_request(&o_ctrl->i2c_init_data);
		if (rc < 0) {
			CAM_WARN(CAM_OIS,
				"Fail deleting Init data: rc: %d", rc);
			rc = 0;
		}
		rc = delete_request(&o_ctrl->i2c_calib_data);
		if (rc < 0) {
			CAM_WARN(CAM_OIS,
				"Fail deleting Calibration data: rc: %d", rc);
			rc = 0;
		}
#endif
		break;
	case CAM_OIS_PACKET_OPCODE_OIS_CONTROL:
		if (o_ctrl->cam_ois_state < CAM_OIS_CONFIG) {
			rc = -EINVAL;
			CAM_WARN(CAM_OIS,
				"Not in right state to control OIS: %d",
				o_ctrl->cam_ois_state);
			return rc;
		}
		offset = (uint32_t *)&csl_packet->payload;
		offset += (csl_packet->cmd_buf_offset / sizeof(uint32_t));
		cmd_desc = (struct cam_cmd_buf_desc *)(offset);
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
		mutex_lock(&(o_ctrl->i2c_mode_data_mutex));
#endif
		i2c_reg_settings = &(o_ctrl->i2c_mode_data);
		i2c_reg_settings->is_settings_valid = 1;
		i2c_reg_settings->request_id = 0;
		rc = cam_sensor_i2c_command_parser(&o_ctrl->io_master_info,
			i2c_reg_settings,
			cmd_desc, 1);
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
		mutex_unlock(&(o_ctrl->i2c_mode_data_mutex));
#endif
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "OIS pkt parsing failed: %d", rc);
			return rc;
		}

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
		msg = kmalloc(sizeof(struct cam_ois_thread_msg_t), GFP_KERNEL);
		if (msg == NULL) {
			CAM_ERR(CAM_OIS, "Failed alloc memory for msg, Out of memory");
			return -ENOMEM;
		}

		memset(msg, 0, sizeof(struct cam_ois_thread_msg_t));
		msg->i2c_reg_settings = i2c_reg_settings;
		msg->msg_type = CAM_OIS_THREAD_MSG_APPLY_SETTING;
		rc = cam_ois_thread_add_msg(o_ctrl, msg);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "Failed add msg to OIS thread");
#else
		rc = cam_ois_apply_settings(o_ctrl, i2c_reg_settings);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "Cannot apply mode settings");
			return rc;
		}

		rc = delete_request(i2c_reg_settings);
		if (rc < 0) {
			CAM_ERR(CAM_OIS,
				"Fail deleting Mode data: rc: %d", rc);
			return rc;
		}
#endif
		break;
	default:
		CAM_ERR(CAM_OIS, "Invalid Opcode: %d",
			(csl_packet->header.op_code & 0xFFFFFF));
		return -EINVAL;
	}

	if (!rc)
		return rc;
pwr_dwn:
	cam_ois_power_down(o_ctrl);
	return rc;
}

void cam_ois_shutdown(struct cam_ois_ctrl_t *o_ctrl)
{
	int rc = 0;
	struct cam_ois_soc_private *soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	struct cam_sensor_power_ctrl_t *power_info = &soc_private->power_info;
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
	int i = 0;
#endif

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
	CAM_INFO(CAM_OIS, "cam_ois_shutdown");

	cam_ois_thread_destroy(o_ctrl);
	for (i = MAX_BRIDGE_COUNT - 1; i >= 0; i--) {
		if (o_ctrl->bridge_intf[i].device_hdl == -1)
			continue;

		CAM_INFO(CAM_OIS, "Release the device hdl %d", o_ctrl->bridge_intf[i].device_hdl);
		rc = cam_destroy_device_hdl(o_ctrl->bridge_intf[i].device_hdl);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "fail destroying the device hdl");
		o_ctrl->bridge_intf[i].device_hdl = -1;
		o_ctrl->bridge_intf[i].link_hdl = -1;
		o_ctrl->bridge_intf[i].session_hdl = -1;
	}
	o_ctrl->start_cnt = 0;
	o_ctrl->bridge_cnt = 0;
#endif

	if (o_ctrl->cam_ois_state == CAM_OIS_INIT)
		return;

	if (o_ctrl->cam_ois_state >= CAM_OIS_CONFIG) {
		rc = cam_ois_power_down(o_ctrl);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "OIS Power down failed");
		o_ctrl->cam_ois_state = CAM_OIS_ACQUIRE;
	}

#if !defined(CONFIG_SAMSUNG_OIS_MCU_STM32) && !defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
	if (o_ctrl->cam_ois_state >= CAM_OIS_ACQUIRE) {
		rc = cam_destroy_device_hdl(o_ctrl->bridge_intf.device_hdl);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "destroying the device hdl");
		o_ctrl->bridge_intf.device_hdl = -1;
		o_ctrl->bridge_intf.link_hdl = -1;
		o_ctrl->bridge_intf.session_hdl = -1;
	}
#endif

	if (o_ctrl->i2c_mode_data.is_settings_valid == 1)
		delete_request(&o_ctrl->i2c_mode_data);

	if (o_ctrl->i2c_calib_data.is_settings_valid == 1)
		delete_request(&o_ctrl->i2c_calib_data);

	if (o_ctrl->i2c_init_data.is_settings_valid == 1)
		delete_request(&o_ctrl->i2c_init_data);

	kfree(power_info->power_setting);
	kfree(power_info->power_down_setting);
	power_info->power_setting = NULL;
	power_info->power_down_setting = NULL;
	power_info->power_down_setting_size = 0;
	power_info->power_setting_size = 0;

	o_ctrl->cam_ois_state = CAM_OIS_INIT;
}

/**
 * cam_ois_driver_cmd - Handle ois cmds
 * @e_ctrl:     ctrl structure
 * @arg:        Camera control command argument
 *
 * Returns success or failure
 */
int cam_ois_driver_cmd(struct cam_ois_ctrl_t *o_ctrl, void *arg)
{
	int                              rc = 0;
	struct cam_ois_query_cap_t       ois_cap = {0};
	struct cam_control              *cmd = (struct cam_control *)arg;
	struct cam_ois_soc_private      *soc_private = NULL;
	struct cam_sensor_power_ctrl_t  *power_info = NULL;

	if (!o_ctrl || !cmd) {
		CAM_ERR(CAM_OIS, "Invalid arguments");
		return -EINVAL;
	}

	if (cmd->handle_type != CAM_HANDLE_USER_POINTER) {
		CAM_ERR(CAM_OIS, "Invalid handle type: %d",
			cmd->handle_type);
		return -EINVAL;
	}

	soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	power_info = &soc_private->power_info;

	mutex_lock(&(o_ctrl->ois_mutex));
	switch (cmd->op_code) {
	case CAM_QUERY_CAP:
		ois_cap.slot_info = o_ctrl->soc_info.index;

		if (copy_to_user(u64_to_user_ptr(cmd->handle),
			&ois_cap,
			sizeof(struct cam_ois_query_cap_t))) {
			CAM_ERR(CAM_OIS, "Failed Copy to User");
			rc = -EFAULT;
			goto release_mutex;
		}

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
		rc = cam_ois_check_fw(o_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "Failed check fw");
			rc = 0; // return success even if check fw is failed
		}
#endif
		CAM_DBG(CAM_OIS, "ois_cap: ID: %d", ois_cap.slot_info);
		break;
	case CAM_ACQUIRE_DEV:
		rc = cam_ois_get_dev_handle(o_ctrl, arg);
		if (rc) {
			CAM_ERR(CAM_OIS, "Failed to acquire dev");
			goto release_mutex;
		}

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
		if (o_ctrl->bridge_cnt > 1)
			goto release_mutex;
#endif

		o_ctrl->cam_ois_state = CAM_OIS_ACQUIRE;
		break;
	case CAM_START_DEV:
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
		o_ctrl->start_cnt++;
#endif
		if (o_ctrl->cam_ois_state != CAM_OIS_CONFIG) {
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
			rc = 0;
#else
			rc = -EINVAL;
#endif
			CAM_WARN(CAM_OIS,
			"Not in right state for start : %d",
			o_ctrl->cam_ois_state);
			goto release_mutex;
		}
		o_ctrl->cam_ois_state = CAM_OIS_START;
		break;
	case CAM_CONFIG_DEV:
		rc = cam_ois_pkt_parse(o_ctrl, arg);
		if (rc) {
			CAM_ERR(CAM_OIS, "Failed in ois pkt Parsing");
			goto release_mutex;
		}
		break;
	case CAM_RELEASE_DEV:

#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
		rc = cam_ois_release_dev_handle(o_ctrl, arg);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "destroying the device hdl");
			goto release_mutex;
		}

		if (o_ctrl->bridge_cnt > 0)
			goto release_mutex;

		cam_ois_thread_destroy(o_ctrl);
#if defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
		rc = cam_ois_i2c_write(o_ctrl, 0x0000, 0x00,CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE); //servo off
		CAM_DBG(CAM_OIS, "SERVO OFF: Command");
		if (rc < 0)
			CAM_ERR(CAM_OIS, "SERVO OFF: I2C write fail");
#endif
		o_ctrl->ois_mode = 0;

		if (o_ctrl->cam_ois_state == CAM_OIS_CONFIG) {
			rc = cam_ois_power_down(o_ctrl);
			if (rc < 0) {
				CAM_ERR(CAM_OIS, "OIS Power Down Failed");
				goto release_mutex;
			}
		}
#else
		if (o_ctrl->cam_ois_state == CAM_OIS_START) {
			rc = -EINVAL;
			CAM_WARN(CAM_OIS,
				"Cant release ois: in start state");
			goto release_mutex;
		}

		if (o_ctrl->cam_ois_state == CAM_OIS_CONFIG) {
			rc = cam_ois_power_down(o_ctrl);
			if (rc < 0) {
				CAM_ERR(CAM_OIS, "OIS Power down failed");
				goto release_mutex;
			}
		}

		if (o_ctrl->bridge_intf.device_hdl == -1) {
			CAM_ERR(CAM_OIS, "link hdl: %d device hdl: %d",
				o_ctrl->bridge_intf.device_hdl,
				o_ctrl->bridge_intf.link_hdl);
			rc = -EINVAL;
			goto release_mutex;
		}
		rc = cam_destroy_device_hdl(o_ctrl->bridge_intf.device_hdl);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "destroying the device hdl");
		o_ctrl->bridge_intf.device_hdl = -1;
		o_ctrl->bridge_intf.link_hdl = -1;
		o_ctrl->bridge_intf.session_hdl = -1;
#endif

		o_ctrl->cam_ois_state = CAM_OIS_INIT;

		kfree(power_info->power_setting);
		kfree(power_info->power_down_setting);
		power_info->power_setting = NULL;
		power_info->power_down_setting = NULL;
		power_info->power_down_setting_size = 0;
		power_info->power_setting_size = 0;

		if (o_ctrl->i2c_mode_data.is_settings_valid == 1)
			delete_request(&o_ctrl->i2c_mode_data);

		if (o_ctrl->i2c_calib_data.is_settings_valid == 1)
			delete_request(&o_ctrl->i2c_calib_data);

		if (o_ctrl->i2c_init_data.is_settings_valid == 1)
			delete_request(&o_ctrl->i2c_init_data);

		break;
	case CAM_STOP_DEV:
		CAM_DBG(CAM_OIS, "CAM_STOP_DEV");
#if defined(CONFIG_SAMSUNG_OIS_MCU_STM32) || defined(CONFIG_SAMSUNG_OIS_RUMBA_S4)
		if (o_ctrl->start_cnt > 0)
			o_ctrl->start_cnt--;

		if (o_ctrl->start_cnt != 0) {
			CAM_WARN(CAM_OIS,
				"Still device running : %d",
				o_ctrl->start_cnt);
			goto release_mutex;
		}

		if (o_ctrl->cam_ois_state != CAM_OIS_START) {
			rc = 0;
			CAM_WARN(CAM_OIS,
				"Not in right state for stop : %d",
				o_ctrl->cam_ois_state);
			goto release_mutex;
		}
#else
		if (o_ctrl->cam_ois_state != CAM_OIS_START) {
			rc = -EINVAL;
			CAM_WARN(CAM_OIS,
			"Not in right state for stop : %d",
			o_ctrl->cam_ois_state);
		}
#endif
		o_ctrl->cam_ois_state = CAM_OIS_CONFIG;
		break;
	default:
		CAM_ERR(CAM_OIS, "invalid opcode");
		goto release_mutex;
	}
release_mutex:
	mutex_unlock(&(o_ctrl->ois_mutex));
	return rc;
}
