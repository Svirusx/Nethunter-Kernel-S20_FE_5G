#ifndef __SEC_PARAM_H__
#define __SEC_PARAM_H__

struct fiemap_extent_p {
	unsigned long fe_logical;  /* logical offset in bytes for the start of the extent from the beginning of the file */
	unsigned long fe_physical; /* physical offset in bytes for the start of the extent from the beginning of the disk */
	unsigned long fe_length;   /* length in bytes for this extent */
	unsigned long fe_reserved64[2];
	unsigned int fe_flags;    /* FIEMAP_EXTENT_* flags for this extent */
	unsigned int fe_reserved[3];
};

struct fiemap_p {
	unsigned long fm_start;         /* logical offset (inclusive) at which to start mapping (in) */
	unsigned long fm_length;        /* logical length of mapping which userspace wants (in) */
	unsigned int fm_flags;         /* FIEMAP_FLAG_* flags for request (in/out) */
	unsigned int fm_mapped_extents;/* number of extents that were mapped (out) */
	unsigned int fm_extent_count;  /* size of fm_extents array (in) */
	unsigned int fm_reserved;
	struct fiemap_extent_p fm_extents[128]; /* array of mapped extents (out) */
};

struct sec_param_data {
	unsigned int debuglevel;
	unsigned int uartsel;
	unsigned int rory_control;
	unsigned int product_device;   /* product/dev device */
	unsigned int reserved1;
	unsigned int cp_debuglevel;
	unsigned int reserved2;
#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
	unsigned int sapa[3];
#else
	unsigned int reserved3[3];
#endif
#ifdef CONFIG_SEC_MONITOR_BATTERY_REMOVAL
	unsigned int normal_poweroff;
#else
	unsigned int reserved4;
#endif
#ifdef CONFIG_WIRELESS_IC_PARAM
	unsigned int wireless_ic;
#else
	unsigned int reserved5;
#endif
	char used0[80];
	char used1[80];
	char used2[80];
	char used3[80];
	char used4[80];
#ifdef CONFIG_WIRELESS_CHARGER_HIGH_VOLTAGE
	unsigned int wireless_charging_mode;
#else
	unsigned int reserved6;
#endif
#if defined(CONFIG_MUIC_HV) || defined(CONFIG_SUPPORT_QC30) || defined(CONFIG_AFC)
	unsigned int afc_disable;
#else
	unsigned int reserved7;
#endif
	unsigned int cp_reserved_mem;
	char used5[4];
	char used6[4];
	char reserved8[8];
	char used7[16];
	unsigned int api_gpio_test;
	char api_gpio_test_result[256];
	char reboot_recovery_cause[256];
	unsigned int user_partition_flashed;
	unsigned int force_upload_flag;
	unsigned int cp_reserved_mem_backup;
	unsigned int FMM_lock;
	unsigned int dump_sink;
	unsigned int fiemap_update;
	struct fiemap_p fiemap_result;
	char used8[80];
	char window_color[2];
	char VrrStatus[16];
#if defined(CONFIG_PD_CHARGER_HV_DISABLE)
	unsigned int pd_disable;
#else
	unsigned int reserved9;
#endif
#if defined(CONFIG_VIB_STORE_LE_PARAM)
	unsigned int vib_le_est;
#else
	unsigned int Reserved_VibLeEst;
#endif
};

struct sec_param_data_s {
	struct work_struct sec_param_work;
	struct completion work;
	void *value;
	unsigned int offset;
	unsigned int size;
	unsigned int direction;
	bool success;
};

enum sec_param_index {
	param_index_debuglevel,
	param_index_uartsel,
	param_rory_control,
	param_cp_debuglevel,
#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
	param_index_sapa,
#endif
#ifdef CONFIG_SEC_MONITOR_BATTERY_REMOVAL
	param_index_normal_poweroff,
#endif
#ifdef CONFIG_WIRELESS_IC_PARAM
	param_index_wireless_ic,
#endif
#ifdef CONFIG_WIRELESS_CHARGER_HIGH_VOLTAGE
	param_index_wireless_charging_mode,
#endif
#if defined(CONFIG_MUIC_HV) || defined(CONFIG_SUPPORT_QC30) || defined(CONFIG_AFC)
	param_index_afc_disable,
#endif
	param_index_cp_reserved_mem,
	param_index_api_gpio_test,
	param_index_api_gpio_test_result,
	param_index_reboot_recovery_cause,
	param_index_user_partition_flashed,
	param_index_force_upload_flag,
	param_index_cp_reserved_mem_backup,
	param_index_FMM_lock,
	param_index_dump_sink,
	param_index_fiemap_update,
	param_index_fiemap_result,
	param_index_window_color,
#ifdef CONFIG_SEC_QUEST
	param_index_quest,
	param_index_quest_ddr_result,
#endif
#ifdef CONFIG_SEC_QUEST_BPS_CLASSIFIER
	param_index_quest_bps_data,
#endif
	param_index_VrrStatus,
#if defined(CONFIG_VIB_STORE_LE_PARAM)
	param_vib_le_est,
#endif
#ifdef CONFIG_PD_CHARGER_HV_DISABLE
	param_index_pd_hv_disable,
#endif
	param_index_max_sec_param_data,
};

#define SEC_PARAM_FILE_SIZE	CONFIG_SEC_PARAM_SIZE  /* 10MB is default, but for some chipset it is 12MB */
#define SEC_PARAM_FILE_OFFSET	(SEC_PARAM_FILE_SIZE - 0x100000)
#define SECTOR_UNIT_SIZE	(4096)		/* UFS */

#define FMMLOCK_MAGIC_NUM	0x464D4F4E
#define SAPA_KPARAM_MAGIC       0x41504153
#define EDTBO_FIEMAP_MAGIC      0x7763

#define CP_MEM_RESERVE_OFF      0
#define CP_MEM_RESERVE_ON_1     0x4350
#define CP_MEM_RESERVE_ON_2     0x4D42

#ifdef CONFIG_SEC_QUEST
/* SEC QUEST region in PARAM partition */
#define SEC_PARAM_QUEST_OFFSET			(SEC_PARAM_FILE_OFFSET - 0x100000)
#define SEC_PARAM_QUEST_SIZE				(0x2000) /* 8KB */
#define SEC_PARAM_QUEST_DDR_RESULT_OFFSET	(SEC_PARAM_QUEST_OFFSET + SEC_PARAM_QUEST_SIZE) /* 8MB + 8KB */
#endif

#ifdef CONFIG_SEC_QUEST_BPS_CLASSIFIER
/* SEC QUEST BPS region in PARAM partition */
#define SEC_PARAM_QUEST_DDR_RESULT_SIZE				(0x2000) /* 8KB */
#define SEC_PARAM_QUEST_BPS_DATA_OFFSET	(SEC_PARAM_QUEST_DDR_RESULT_OFFSET + SEC_PARAM_QUEST_DDR_RESULT_SIZE) /* 8MB + 8KB + 8KB*/
#endif

#ifdef CONFIG_SEC_PARAM
extern bool sec_get_param(enum sec_param_index index, void *value);
extern bool sec_set_param(enum sec_param_index index, void *value);
#else
static inline bool sec_get_param(enum sec_param_index index, void *value) { return false; }
static inline bool sec_set_param(enum sec_param_index index, void *value) { return false; }
#endif

#endif	/* __SEC_PARAM_H__ */
