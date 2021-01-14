/*
*
 * Source file for mAFPC Display Driver
 *
 * Copyright (c) 2020 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

 #ifndef _MAFPC_H_
 #define _MAFPC_H_

/* Align & Paylod Size for Embedded mode Transfer */
#define MAFPC_CMD_ALIGN 16
#define MAFPC_MAX_PAYLOAD_SIZE 200

/* Align & Paylod Size for Non-Embedded mode(Mass) Command Transfer */
#define MAFPC_MASS_CMD_ALIGN 256
#define MAFPC_MAX_PAYLOAD_SIZE_MASS 0xFFFF0 /* 983,024 */

/*
 * ioctl
 */
#define MAFPC_IOCTL_MAGIC	'M'
#define IOCTL_MAFPC_ON		_IOW(MAFPC_IOCTL_MAGIC, 1, int)
#define IOCTL_MAFPC_ON_INSTANT	_IOW(MAFPC_IOCTL_MAGIC, 2, int)
#define IOCTL_MAFPC_OFF		_IOW(MAFPC_IOCTL_MAGIC, 3, int)
#define IOCTL_MAFPC_OFF_INSTANT	_IOW(MAFPC_IOCTL_MAGIC, 4, int)
#define IOCTL_MAFPC_MAX		10

struct MAFPC {
	struct miscdevice dev;

	int is_support;
	int factory_support;
	int file_open;
	int need_to_write;
	int is_br_table_updated;
	int force_delay;

	int en;

	char *enable_cmd_buf;
	u32 enable_cmd_size;
	char *img_buf;
	u32 img_size;
	u32 brightness_scale_table_size;

	u32 wpos;
	u64 wsize;

	int img_checksum;
	u32 img_checksum_cal;

	struct mutex vdd_mafpc_lock;
	struct mutex vdd_mafpc_crc_check_lock;

	char *crc_img_buf;
	u32 crc_img_size;

	u8 *crc_pass_data;	// implemented in dtsi
	u8 *crc_read_data;
	int crc_size;

	/* mafpc Function */
	int (*init)(struct samsung_display_driver_data *vdd);
	int (*data_init)(struct samsung_display_driver_data *vdd);
	void (*make_img_cmds)(struct samsung_display_driver_data *vdd, char *data, u32 data_size, enum dsi_cmd_set_type cmd_type);
	void (*make_img_mass_cmds)(struct samsung_display_driver_data *vdd, char *data, u32 data_size, enum dsi_cmd_set_type cmd_type);
	void (*img_write)(struct samsung_display_driver_data *vdd, bool is_instant);
	void (*enable)(struct samsung_display_driver_data *vdd, int enable);
	int (*crc_check)(struct samsung_display_driver_data *vdd);
	int (*debug)(struct samsung_display_driver_data *vdd);
	struct dsi_panel_cmd_set* (*brightness_scale)(struct samsung_display_driver_data *vdd, int *level_key);
};

#endif
