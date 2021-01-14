/* Copyright (c) 2017, The Linux Foundation. All rights reserved.
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
#ifndef _CAM_OIS_MCU_STM32_H_
#define _CAM_OIS_MCU_STM32_H_

#include "cam_ois_dev.h"

#define INIT_X_TARGET		(800)
#define STEP_VALUE			(300)
#define STEP_COUNT			(10)
#define RUMBA_WRITE_UILD	(0x48)
#define RUMBA_READ_UILD 	(0x49)

#define CAMERA_OIS_EXT_CLK_12MHZ 0xB71B00
#define CAMERA_OIS_EXT_CLK_17MHZ 0x1036640
#define CAMERA_OIS_EXT_CLK_19P2MHZ 0x124F800
#define CAMERA_OIS_EXT_CLK_24MHZ   0x16E3600
#define CAMERA_OIS_EXT_CLK_26MHZ   0x18CBA80

struct cam_ois_sinewave_t
{
    int sin_x;
    int sin_y;
};

int cam_ois_offset_test(struct cam_ois_ctrl_t *o_ctrl,
	long *raw_data_x, long *raw_data_y, bool is_need_cal);
int cam_ois_get_offset_data(struct cam_ois_ctrl_t *o_ctrl,
	long *raw_data_x, long *raw_data_y);
int cam_ois_gyro_sensor_calibration(struct cam_ois_ctrl_t *o_ctrl,
	long *raw_data_x, long *raw_data_y);
uint32_t cam_ois_self_test(struct cam_ois_ctrl_t *o_ctrl);
bool cam_ois_sine_wavecheck(struct cam_ois_ctrl_t *o_ctrl, int threshold,
	struct cam_ois_sinewave_t *sinewave, int *result, int num_of_module);
int cam_ois_check_fw(struct cam_ois_ctrl_t *o_ctrl);
int cam_ois_wait_idle(struct cam_ois_ctrl_t *o_ctrl, int retries);
int cam_ois_init(struct cam_ois_ctrl_t *o_ctrl);
int cam_ois_i2c_write(struct cam_ois_ctrl_t *o_ctrl,
	uint32_t addr, uint32_t data,
	enum camera_sensor_i2c_type addr_type,
	enum camera_sensor_i2c_type data_type);
int cam_ois_shift_calibration(struct cam_ois_ctrl_t *o_ctrl, uint16_t af_position, uint16_t subdev_id);
int32_t cam_ois_set_debug_info(struct cam_ois_ctrl_t *o_ctrl, uint16_t mode);
int cam_ois_get_ois_mode(struct cam_ois_ctrl_t *o_ctrl, uint16_t *mode);
int cam_ois_set_ois_mode(struct cam_ois_ctrl_t *o_ctrl, uint16_t mode);
int cam_ois_set_shift(struct cam_ois_ctrl_t *o_ctrl);
int cam_ois_set_angle_for_compensation(struct cam_ois_ctrl_t *o_ctrl);
int cam_ois_set_ggfadeup(struct cam_ois_ctrl_t *o_ctrl, uint16_t value);
int cam_ois_set_ggfadedown(struct cam_ois_ctrl_t *o_ctrl, uint16_t value);
int cam_ois_fixed_aperture(struct cam_ois_ctrl_t *o_ctrl);
int cam_ois_write_xgg_ygg(struct cam_ois_ctrl_t *o_ctrl);
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
#if defined(CONFIG_SEC_C2Q_PROJECT)
bool cam_ois_write_efs_tele_center_shift(struct cam_ois_ctrl_t *o_ctrl);
#endif
int cam_ois_write_dual_cal(struct cam_ois_ctrl_t *o_ctrl);
#endif
int cam_ois_write_gyro_orientation(struct cam_ois_ctrl_t *o_ctrl);
int cam_ois_mcu_init(struct cam_ois_ctrl_t *o_ctrl);
void cam_ois_reset(void *ctrl);
int cam_ois_read_hall_position(struct cam_ois_ctrl_t *o_ctrl,
	uint32_t* targetPosition, uint32_t* hallPosition);
#if defined(CONFIG_SAMSUNG_OIS_TAMODE_CONTROL)
int ps_notifier_cb(struct notifier_block *nb, unsigned long event, void *data);
int cam_ois_add_tamode_msg(struct cam_ois_ctrl_t *o_ctrl);
int cam_ois_set_ta_mode(struct cam_ois_ctrl_t *o_ctrl);
#endif
int cam_ois_check_tele_cross_talk(struct cam_ois_ctrl_t *o_ctrl, uint16_t *result);
uint32_t cam_ois_check_ext_clk(struct cam_ois_ctrl_t *o_ctrl);
int32_t cam_ois_set_ext_clk(struct cam_ois_ctrl_t *o_ctrl, uint32_t clk);

/*
*Below code add for MCU sysboot cmd operation
*/
typedef struct
{
    uint32_t page;
    uint32_t count;
} sysboot_erase_param_type;

/* Target specific definitions
 */
#define BOOT_I2C_STARTUP_DELAY          (sysboot_i2c_startup_delay) /* msecs */
#define BOOT_I2C_TARGET_PID             (product_id)
#define BOOT_I2C_ADDR                   (sysboot_i2c_slave_address << 1) /* it used directly as parameter of I2C HAL API */

#define BOOT_I2C_HANDLE                 (hi2c1)
#define BOOT_I2C_LPHANDLE               (&(BOOT_I2C_HANDLE))

/* Protocol specific definitions
 *  NOTE: timeout interval unit: msec
 */

#define BOOT_I2C_INTER_PKT_FRONT_INTVL  (1)
#define BOOT_I2C_INTER_PKT_BACK_INTVL   (1)

#define BOOT_I2C_SYNC_RETRY_COUNT       (3)
#define BOOT_I2C_SYNC_RETRY_INTVL       (50)

#define BOOT_I2C_CMD_TMOUT              (30)
#define BOOT_I2C_WRITE_TMOUT            (flash_prog_time)
#define BOOT_I2C_FULL_ERASE_TMOUT       (flash_full_erase_time)
#define BOOT_I2C_PAGE_ERASE_TMOUT(n)    (flash_page_erase_time * n)
#define BOOT_I2C_WAIT_RESP_TMOUT        (30)
#define BOOT_I2C_WAIT_RESP_POLL_TMOUT   (500)
#define BOOT_I2C_WAIT_RESP_POLL_INTVL   (3)
#define BOOT_I2C_WAIT_RESP_POLL_RETRY   (BOOT_I2C_WAIT_RESP_POLL_TMOUT / BOOT_I2C_WAIT_RESP_POLL_INTVL)
#define BOOT_I2C_XMIT_TMOUT(count)      (5 + (1 * count))
#define BOOT_I2C_RECV_TMOUT(count)      BOOT_I2C_XMIT_TMOUT(count)

/* Payload length info. */

#define BOOT_I2C_CMD_LEN                (1)
#define BOOT_I2C_ADDRESS_LEN            (4)
#define BOOT_I2C_NUM_READ_LEN           (1)
#define BOOT_I2C_NUM_WRITE_LEN          (1)
#define BOOT_I2C_NUM_ERASE_LEN          (2)
#define BOOT_I2C_CHECKSUM_LEN           (1)

#define BOOT_I2C_MAX_WRITE_LEN          (256)  /* Protocol limitation */
#define BOOT_I2C_MAX_ERASE_PARAM_LEN    (4096) /* In case of erase parameter with 2048 pages */
#define BOOT_I2C_MAX_PAYLOAD_LEN        (BOOT_I2C_MAX_ERASE_PARAM_LEN) /* Larger one between write and erase., */

#define BOOT_I2C_REQ_CMD_LEN            (BOOT_I2C_CMD_LEN + BOOT_I2C_CHECKSUM_LEN)
#define BOOT_I2C_REQ_ADDRESS_LEN        (BOOT_I2C_ADDRESS_LEN + BOOT_I2C_CHECKSUM_LEN)
#define BOOT_I2C_READ_PARAM_LEN         (BOOT_I2C_NUM_READ_LEN + BOOT_I2C_CHECKSUM_LEN)
#define BOOT_I2C_WRITE_PARAM_LEN(len)   (BOOT_I2C_NUM_WRITE_LEN + len + BOOT_I2C_CHECKSUM_LEN)
#define BOOT_I2C_ERASE_PARAM_LEN(len)   (len + BOOT_I2C_CHECKSUM_LEN)

#define BOOT_I2C_RESP_GET_VER_LEN       (0x01) /* bootloader version(1) */
#define BOOT_I2C_RESP_GET_ID_LEN        (0x03) /* number of bytes - 1(1) + product ID(2) */

/* Commands and Response */

#define BOOT_I2C_CMD_GET                (0x00)
#define BOOT_I2C_CMD_GET_VER            (0x01)
#define BOOT_I2C_CMD_GET_ID             (0x02)
#define BOOT_I2C_CMD_READ               (0x11)
#define BOOT_I2C_CMD_GO                 (0x21)
#define BOOT_I2C_CMD_WRITE              (0x31)
#define BOOT_I2C_CMD_ERASE              (0x44)
#define BOOT_I2C_CMD_WRITE_UNPROTECT    (0x73)
#define BOOT_I2C_CMD_READ_UNPROTECT     (0x92)
#define BOOT_I2C_CMD_SYNC               (0xFF)

#define BOOT_I2C_RESP_ACK               (0x79)
#define BOOT_I2C_RESP_NACK              (0x1F)
#define BOOT_I2C_RESP_BUSY              (0x76)

/* Exported functions ------------------------------------------------------- */
int sysboot_i2c_sync(struct cam_ois_ctrl_t *o_ctrl, uint8_t *cmd);
int sysboot_i2c_info(struct cam_ois_ctrl_t *o_ctrl);
int sysboot_i2c_read(struct cam_ois_ctrl_t *o_ctrl, uint32_t address, uint8_t *dst, size_t len);
int sysboot_i2c_write(struct cam_ois_ctrl_t *o_ctrl, uint32_t address, uint8_t *src, size_t len);
int sysboot_i2c_erase(struct cam_ois_ctrl_t *o_ctrl, uint32_t address, size_t len);
int sysboot_i2c_go(struct cam_ois_ctrl_t *o_ctrl, uint32_t address);
int sysboot_i2c_write_unprotect(struct cam_ois_ctrl_t *o_ctrl);
int sysboot_i2c_read_unprotect(struct cam_ois_ctrl_t *o_ctrl);

/* Private definitaions ----------------------------------------------------- */
#define BOOT_NRST_PULSE_INTVL           (2) /* msec */

/* Utility MACROs */

#ifndef NTOHL
#define NTOHL(x)                        ((((x) & 0xFF000000U) >> 24) | \
                                         (((x) & 0x00FF0000U) >>  8) | \
                                         (((x) & 0x0000FF00U) <<  8) | \
                                         (((x) & 0x000000FFU) << 24))
#endif
#ifndef HTONL
#define HTONL(x)                        NTOHL(x)
#endif

#ifndef NTOHS
#define NTOHS(x)                        (((x >> 8) & 0x00FF) | ((x << 8) & 0xFF00))
#endif
#ifndef HTONS
#define HTONS(x)                        NTOHS(x)
#endif

/* ERROR definitions -------------------------------------------------------- */

enum
{
	/* BASE ERROR ------------------------------------------------------------- */
	BOOT_ERR_BASE                         = -999, /* -9xx */
	BOOT_ERR_INVALID_PROTOCOL_GET_INFO,
	BOOT_ERR_INVALID_PROTOCOL_SYNC,
	BOOT_ERR_INVALID_PROTOCOL_READ,
	BOOT_ERR_INVALID_PROTOCOL_WRITE,
	BOOT_ERR_INVALID_PROTOCOL_ERASE,
	BOOT_ERR_INVALID_PROTOCOL_GO,
	BOOT_ERR_INVALID_PROTOCOL_WRITE_UNPROTECT,
	BOOT_ERR_INVALID_PROTOCOL_READ_UNPROTECT,
	BOOT_ERR_INVALID_MAX_WRITE_BYTES,

	/* I2C ERROR -------------------------------------------------------------- */
	BOOT_ERR_I2C_BASE                     = -899, /* -8xx */
	BOOT_ERR_I2C_RESP_NACK,
	BOOT_ERR_I2C_RESP_UNKNOWN,
	BOOT_ERR_I2C_RESP_API_FAIL,
	BOOT_ERR_I2C_XMIT_API_FAIL,
	BOOT_ERR_I2C_RECV_API_FAIL,

	/* SPI ERROR -------------------------------------------------------------- */
	BOOT_ERR_SPI_BASE                     = -799, /* -7xx */

	/* UART ERROR ------------------------------------------------------------- */
	BOOT_ERR_UART_BASE                    = -699, /* -6xx */

	/* DEVICE ERROR ----------------------------------------------------------- */
	BOOT_ERR_DEVICE_MEMORY_MAP            = -599, /* -5xx */
	BOOT_ERR_DEVICE_PAGE_SIZE_NOT_FOUND,

	/* API ERROR (OFFSET) ----------------------------------------------------- */
	BOOT_ERR_API_GET                      = -1000,
	BOOT_ERR_API_GET_ID                   = -2000,
	BOOT_ERR_API_GET_VER                  = -3000,
	BOOT_ERR_API_SYNC                     = -4000,
	BOOT_ERR_API_READ                     = -5000,
	BOOT_ERR_API_WRITE                    = -6000,
	BOOT_ERR_API_ERASE                    = -7000,
	BOOT_ERR_API_GO                       = -8000,
	BOOT_ERR_API_WRITE_UNPROTECT          = -9000,
	BOOT_ERR_API_READ_UNPROTECT           = -10000,
	BOOT_ERR_API_SAVE_CONTENTS            = -11000,
	BOOT_ERR_API_RESTORE_CONTENTS         = -12000,
};
#endif/* _CAM_OIS_MCU_STM32_H_ */
