/* for query request (not use) */
#define COM_QUERY			0x2A

/* scan mode*/
#define COM_SURVEY_ASYNC_SCAN		0x2B
#define COM_SURVEY_EXIT			0x2D
#define COM_SURVEY_SYNC_SCAN		0x2E
#define COM_SURVEY_GARAGE_ONLY		0x3B

#define COM_SAMPLERATE_STOP		0x30
#define COM_SAMPLERATE_START		0x31

#define COM_CHECKSUM			0x63

#define COM_NORMAL_COMPENSATION		0x80
#define COM_SPECIAL_COMPENSATION	0x81
#define COM_BOOKCOVER_COMPENSATION	0x81
#define COM_KBDCOVER_COMPENSATION	0x82

/* elec test*/
#define COM_ASYNC_VSYNC			0X28
#define COM_SYNC_VSYNC			0X29
#define COM_ELEC_SCAN_START		0xC8
#define COM_ELEC_XSCAN			0xCA
#define COM_ELEC_YSCAN			0xCB
#define COM_ELEC_TRSCON			0xCE
#define COM_ELEC_TRSX0			0xCF
#define COM_ELEC_REQUESTDATA		0xD6

/* digitizer & garage open test */
#define COM_GARAGE_TEST_MODE		0xC4
#define COM_DIGITIZER_TEST_MODE		0xC5
#define COM_OPEN_CHECK_START		0xC9
#define COM_OPEN_CHECK_STATUS		0xD8

/* sensitivity set cmd */
#define COM_NORMAL_SENSE_MODE		0xDB
#define COM_LOW_SENSE_MODE		0xDC
#define COM_LOW_SENSE_MODE2		0xE7
#define COM_REQUEST_SENSE_MODE		0xDD

/* have to check below register*/
#define COM_REQUEST_GARAGEDATA		0XE5
#define COM_REQUEST_SCANMODE		0xE6

#define COM_BLE_C_DISABLE		0XE8
#define COM_BLE_C_ENABLE		0XE9
#define COM_BLE_C_RESET			0XEA
#define COM_BLE_C_START			0XEB
#define COM_BLE_C_KEEP_ON		0XEC
#define COM_BLE_C_KEEP_OFF		0XED
#define COM_BLE_C_MODE_RETURN		0XEE
#define COM_BLE_C_FORCE_RESET		0xEF
#define COM_BLE_C_FULL			0xF3
#define COM_FLASH			0xFF

/* pen ble charging */
enum epen_ble_charge_mode {
	EPEN_BLE_C_DISABLE	= 0,
	EPEN_BLE_C_ENABLE,
	EPEN_BLE_C_RESET,
	EPEN_BLE_C_START,
	EPEN_BLE_C_KEEP_ON,
	EPEN_BLE_C_KEEP_OFF,
	EPEN_BLE_C_M_RETURN,
	EPEN_BLE_C_DSPX,
	EPEN_BLE_C_FULL,
	EPEN_BLE_C_MAX,
};

enum epen_ble_charge_state {
	BLE_C_OFF	= 0,
	BLE_C_START,
	BLE_C_TRANSIT,
	BLE_C_RESET,
	BLE_C_AFTER_START,
	BLE_C_AFTER_RESET,
	BLE_C_ON_KEEP_1,
	BLE_C_OFF_KEEP_1,
	BLE_C_ON_KEEP_2,
	BLE_C_OFF_KEEP_2,
	BLE_C_FULL,
};

/* wacom query data format */
#define EPEN_REG_HEADER			0x00
#define EPEN_REG_X1			0x01
#define EPEN_REG_X2			0x02
#define EPEN_REG_Y1			0x03
#define EPEN_REG_Y2			0x04
#define EPEN_REG_PRESSURE1		0x05
#define EPEN_REG_PRESSURE2		0x06
#define EPEN_REG_FWVER1			0x07
#define EPEN_REG_FWVER2			0x08
#define EPEN_REG_MPUVER			0x09
#define EPEN_REG_BLVER			0x0A
#define EPEN_REG_TILT_X			0x0B
#define EPEN_REG_TILT_Y			0x0C
#define EPEN_REG_HEIGHT			0x0D
#define EPEN_REG_FMTREV			0x0F

/* wacom ic values */
#define MPU_W9018			0x42
#define MPU_W9019			0x43
#define MPU_W9020			0x44
#define MPU_W9021			0x45

