/*
 * Copyright (C) 2010 Trusted Logic S.A.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/******************************************************************************
 *
 *  The original Work has been changed by NXP Semiconductors.
 *
 *  Copyright (C) 2013-2020 NXP Semiconductors
 *   *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ******************************************************************************/
#ifndef _PN553_H_
#define _PN553_H_

#include <linux/miscdevice.h>

#define FEATURE_CORE_RESET_NTF_CHECK

#ifdef FEATURE_CORE_RESET_NTF_CHECK
enum nfc_err_state {
	STATE_NORMAL		= 0,
	STATE_CORE_RESET	= 1,
	STATE_ABNORMAL_POWER	= 2,
};

#define CORE_RESET_NTF_NO_CLOCK		0x00205FD100A0ULL
#define CORE_RESET_NTF_CLOCK_LOST	0x0000000000A4ULL
#endif

#if !defined(CONFIG_NFC_FEATURE_SN100U)
#define FEATURE_PN80T
#else
#define FEATURE_SN100X
/*
 * VEN is kept ON all the time if you define the macro VEN_ALWAYS_ON.
 * Used for SN100 usecases
 */
#define VEN_ALWAYS_ON
#endif

#define SEC_NFC_WAKELOCK

#define PN547_MAGIC	0xE9

/*
 * PN544 power control via ioctl
 * PN544_SET_PWR(0): power off
 * PN544_SET_PWR(1): power on
 * PN544_SET_PWR(2): reset and power on with firmware download enabled
 */
#define PN547_SET_PWR             _IOW(PN547_MAGIC, 0x01, long)

/*
 * SPI Request NFCC to enable p61 power, only in param
 * Only for SPI
 * level 1 = Enable power
 * level 0 = Disable power
 * This also be used to perform eSE cold reset when
 * argument value is 0x03
 */
#define P61_SET_SPI_PWR          _IOW(PN547_MAGIC, 0x02, long)

/* SPI or DWP can call this ioctl to get the current power state of P61 */
#define P61_GET_PWR_STATUS       _IOR(PN547_MAGIC, 0x03, long)

/*
 * DWP side this ioctl will be called
 * level 1 = Wired access is enabled/ongoing
 * level 0 = Wired access is disalbed/stopped
 */
#define P61_SET_WIRED_ACCESS     _IOW(PN547_MAGIC, 0x04, long)

/* NFC Init will call the ioctl to register the PID with the i2c driver */
#define P547_SET_NFC_SERVICE_PID _IOW(PN547_MAGIC, 0x05, long)

/* NFC and SPI will call the ioctl to get the i2c/spi bus access */
#define P547_GET_ESE_ACCESS      _IOW(PN547_MAGIC, 0x06, long)

/* NFC and SPI will call the ioctl to update the power scheme */
#define P547_SET_POWER_SCHEME    _IOW(PN547_MAGIC, 0x07, long)

/* NFC will call the ioctl to release the svdd protection */
#define P547_REL_SVDD_WAIT       _IOW(PN547_MAGIC, 0x08, long)

/* SPI or DWP can call this ioctl to get the current power state of P61 */
#define PN547_SET_DWNLD_STATUS   _IOW(PN547_MAGIC, 0x09, long)

/* NFC will call the ioctl to release the dwp on/off protection */
#define P547_REL_DWPONOFF_WAIT   _IOW(PN547_MAGIC, 0x0A, long)

/* NFC HAL can call this ioctl to get the current IRQ state */
#define PN547_GET_IRQ_STATE    _IOW(PN547_MAGIC, 0x0C, long)

#define NFC_I2C_LDO_ON	1
#define NFC_I2C_LDO_OFF	0

/* vendor/samsung/interfaces/nfc/nxp/SN100x/halimpl/tml/phTmlNfc_i2c.h#4 */
enum {
	MODE_POWER_OFF = 0x00,
	MODE_POWER_ON,
	MODE_FW_DWNLD_WITH_VEN,
	MODE_ISO_RST,
	MODE_FW_DWND_HIGH,
	MODE_POWER_RESET,
	MODE_FW_GPIO_LOW,
};

#ifdef CONFIG_NFC_FEATURE_SN100U
/* NFC_ON: Driver is being used by the NFC service  (bit b0)*/
#define PN547_STATE_NFC_ON              0x01

/* FW_DNLD: NFC_ON and FW download is going on (bit b1)*/
#define PN547_STATE_FW_DNLD             0x02

/* VEN_RESET: NFC_ON and FW download with VEN reset (bit b2)*/
#define PN547_STATE_NFC_VEN_RESET       0x04

/* ESE_RESET: Starting of flag positions for eSE cold reset origin */
#define ESE_COLD_RESET_ORIGIN_FLAGS_POS     (4) //(bit b4)
#define ESE_COLD_RESET_ORIGIN_NFC_FLAG_POS  (4) //(bit b4)

/* ESE_RESET: Mask for the flags used for Driver to driver cold reset
 * b6, b5, b4 :
 * 0   0   0 -> no request for ese_cold_reset
 * 0   0   1 -> ese_cold_reset requested from NFC driver
 * 0   1   0 -> ese_cold_reset requested from eSE driver
 * 1   0   0 -> ese_cold_reset requested from UWB driver
 */
#define MASK_ESE_COLD_RESET              (0x70)

/* ESE_RESET: Bit mask to check if ese_reset_guard timer is started (bit b7) */
#define MASK_ESE_COLD_RESET_GUARD_TIMER  (0x80)

/* ESE_RESET: Guard time to allow eSE cold reset from the driver */
#define ESE_COLD_RESET_GUARD_TIME        (3000) //3s

/* ESE_RESET: NCI command response timeout */
#define NCI_CMD_RSP_TIMEOUT              (2000) //2s

/* ESE_RESET: Guard time to reboot the JCOP */
#define ESE_COLD_RESET_REBOOT_GUARD_TIME   (50) //50ms
#endif

#define P61_STATE_JCP_DL 0x8000

enum p61_access_state {
	P61_STATE_INVALID   = 0x0000,
	P61_STATE_IDLE      = 0x0100, /* p61 is free to use */
	P61_STATE_WIRED     = 0x0200, /* p61 is being accessed by DWP (NFCC)*/
	P61_STATE_SPI       = 0x0400, /* P61 is being accessed by SPI */
	P61_STATE_DWNLD     = 0x0800, /* NFCC fw download is in progress */
	P61_STATE_SPI_PRIO  = 0x1000, /*Start of p61 access by SPI on priority*/
	P61_STATE_SPI_PRIO_END = 0x2000,/*End of p61 access by SPI on priority*/
	P61_STATE_SPI_END   = 0x4000,
	P61_STATE_JCP_DWNLD = P61_STATE_JCP_DL, /* JCOP download in progress */
	P61_STATE_SPI_SVDD_SYNC_START = 0x0001, /*ESE_VDD Low req by SPI*/
	P61_STATE_SPI_SVDD_SYNC_END   = 0x0002, /*ESE_VDD is Low by SPI*/
	P61_STATE_DWP_SVDD_SYNC_START = 0x0004, /*ESE_VDD  Low req by Nfc*/
	P61_STATE_DWP_SVDD_SYNC_END   = 0x0008 /*ESE_VDD is Low by Nfc*/
};

enum chip_type_pwr_scheme {
	PN67T_PWR_SCHEME = 0x01,
	PN80T_LEGACY_PWR_SCHEME,
	PN80T_EXT_PMU_SCHEME,
};

enum jcop_dwnld_state {
	JCP_DWNLD_IDLE         = P61_STATE_JCP_DL, /* jcop dwnld is ongoing*/
	JCP_DWNLD_INIT         = 0x8010, /* jcop dwonload init state*/
	JCP_DWNLD_START        = 0x8020, /* download started */
	JCP_SPI_DWNLD_COMPLETE = 0x8040, /* jcop download complete in spi i/f*/
	JCP_DWP_DWNLD_COMPLETE = 0x8080, /* jcop download complete */

};

#if defined(CONFIG_NFC_FEATURE_SN100U)
void pn547_register_ese_shutdown(void (*func)(void));
#endif

extern unsigned int lpcharge;

struct pn547_dev {
	wait_queue_head_t read_wq;
	struct mutex read_mutex;
	struct i2c_client *client;
	struct miscdevice pn547_device;
	struct mutex dev_ref_mutex;
	int ven_gpio;
	int firm_gpio;
	int irq_gpio;
	int pvdd;
	struct regulator *nfc_pvdd;

#ifdef CONFIG_NFC_PN547_ESE_SUPPORT
#ifdef FEATURE_PN80T
	int ese_pwr_req;
#endif
#ifdef ISO_RST
	int iso_rst_gpio; /* ISO-RST pin gpio*/
#endif
	enum p61_access_state  p61_current_state;
	struct completion ese_comp;
	struct completion svdd_sync_comp;
	struct completion dwp_onoff_comp;
	struct mutex p61_state_mutex; /* used to make p61_current_state flag secure */
#endif
	bool spi_ven_enabled;
	bool nfc_ven_enabled;
	bool release_read;

	atomic_t irq_enabled;
	spinlock_t irq_enabled_lock;
	bool cancel_read;
#ifdef SEC_NFC_WAKELOCK
	struct wakeup_source	*ws;
#endif
	struct clk *nfc_clock;
	struct clk *clk;
	int  i2c_probe;
	char *r_buf;
	char *w_buf;

	int  clk_req_gpio;
	int  clk_req_irq;
	bool clk_req_wake;
#ifdef CONFIG_NFC_PN547_CLOCK_REQUEST
	struct msm_xo_voter *nfc_clock;
	struct work_struct work_nfc_clock;
	struct workqueue_struct *wq_clock;
	bool clock_state;
#endif

	long nfc_service_pid; /*used to signal the nfc the nfc service */

#ifdef FEATURE_SN100X
/* This byte represents different flags used during eSE cold reset request from
 * Driver to driver
 * Bit value  Status	Remark
 * b0 : 1  -> NFC_ON	Driver Open should set the flag
 *	0  -> NFC_OFF	Driver release should reset this flag
 * b1 : 1  -> FWDNLD	If FWDNLD is going on.
 *	0		Normal operation
 * b2 : 1  -> Ven reset has been requested
 * b3 : reserved bit
 * b6, b5, b4 :
 * 0   0   0 -> no request for ese_cold_reset
 * 0   0   1 -> ese_cold_reset requested from NFC driver
 * 0   1   0 -> ese_cold_reset requested from eSE driver
 * 0   1   1 -> ese_cold_reset requested from UWB driver
 *		Remaining combinations: Reserved for future use.
 *		These bits will be cleared once cold reset rsp is received.
 * b7 : 1 -->	The ese_cold reset guard time has is running
 *		It will be cleared by the Guard Timer Callback
 *
 */
	uint8_t state_flags;
	void (*ese_shutdown)(void);
#endif
};

ssize_t pn547_dev_read(struct file *filp, char __user *buf,
		size_t count, loff_t *offset);
struct pn547_dev *get_nfcc_dev_data(void);
#endif
