/******************************************************************************
 *
 * Copyright(c) 2007 - 2017 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#ifndef __HAL_COM_PHYCFG_H__
#define __HAL_COM_PHYCFG_H__

#define		PathA                     			0x0	/* Useless */
#define		PathB			0x1
#define		PathC			0x2
#define		PathD			0x3

typedef enum _RF_TX_NUM {
	RF_1TX = 0,
	RF_2TX,
	RF_3TX,
	RF_4TX,
	RF_MAX_TX_NUM,
	RF_TX_NUM_NONIMPLEMENT,
} RF_TX_NUM;

/*------------------------------Define structure----------------------------*/
typedef struct _BB_REGISTER_DEFINITION {
	u32 rfintfs;			/* set software control: */
	/*		0x870~0x877[8 bytes] */

	u32 rfintfo; 			/* output data: */
	/*		0x860~0x86f [16 bytes] */

	u32 rfintfe; 			/* output enable: */
	/*		0x860~0x86f [16 bytes] */

	u32 rf3wireOffset;	/* LSSI data: */
	/*		0x840~0x84f [16 bytes] */

	u32 rfHSSIPara2;	/* wire parameter control2 :  */
	/*		0x824~0x827,0x82c~0x82f, 0x834~0x837, 0x83c~0x83f [16 bytes] */

	u32 rfLSSIReadBack;	/* LSSI RF readback data SI mode */
	/*		0x8a0~0x8af [16 bytes] */

	u32 rfLSSIReadBackPi;	/* LSSI RF readback data PI mode 0x8b8-8bc for Path A and B */

} BB_REGISTER_DEFINITION_T, *PBB_REGISTER_DEFINITION_T;


/* ---------------------------------------------------------------------- */
u8
PHY_GetTxPowerByRatexBase(
		PADAPTER		Adapter,
		u8				Band,
		u8				RfPath,
		RATE_SECTION	RateSection
);

void
PHY_GetRateValuesOfTxPowerByRatex(
		PADAPTER pAdapter,
		u32 RegAddr,
		u32 BitMask,
		u32 Value,
		u8 *Rate,
		s8 *PwrByRateVal,
		u8 *RateNum
);

u8
PHY_GetRateIndexOfTxPowerByRatex(
		u8	Rate
);

void
phy_set_tx_power_index_by_rate_sectionx(
		PADAPTER		pAdapter,
		enum rf_path		RFPath,
		u8				Channel,
		u8				RateSection
);

s8
_PHY_GetTxPowerByRatex(
		PADAPTER	pAdapter,
		u8			Band,
		enum rf_path	RFPath,
		u8			RateIndex
);

s8
PHY_GetTxPowerByRatex(
		PADAPTER	pAdapter,
		u8			Band,
		enum rf_path	RFPath,
		u8			RateIndex
);

void
PHY_SetTxPowerxByRatex(
		PADAPTER	pAdapter,
		u8			Band,
		enum rf_path	RFPath,
		u8			Rate,
		s8			Value
);

void
phy_set_tx_power_level_by_pathx(
		PADAPTER	Adapter,
		u8			channel,
		u8			path
);

void
PHY_SetTxPowerxIndexxByRateArray(
		PADAPTER		pAdapter,
		enum rf_path		RFPath,
		enum channel_width BandWidth,
		u8				Channel,
		u8				*Rates,
		u8				RateArraySize
);

void
PHY_InitTxPowerByRatex(
		PADAPTER	pAdapter
);

void
phy_store_tx_power_by_ratex(
		PADAPTER	pAdapter,
		u32			Band,
		u32			RfPath,
		u32			TxNum,
		u32			RegAddr,
		u32			BitMask,
		u32			Data
);

void
PHY_TxPowerByRateConfigurationx(
	  PADAPTER			pAdapter
);

u8 phy_get_pg_txpwr_idx(
		PADAPTER		pAdapter,
		enum rf_path		RFPath,
		u8				Rate,
	u8 ntx_idx,
		enum channel_width	BandWidth,
		u8				Channel,
		PBOOLEAN		bIn24G
);

#if CONFIG_TXPWR_LIMIT
s8 phy_get_txpwr_lmtx_abs(_adapter *adapter
	, const char *regd_name
	, BAND_TYPE band, enum channel_width bw
	, u8 tlrs, u8 ntx_idx, u8 cch, u8 lock
);

s8 phy_get_txpwr_lmtx(_adapter *adapter
	, const char *regd_name
	, BAND_TYPE band, enum channel_width bw
	, u8 rfpath, u8 rs, u8 ntx_idx, u8 cch, u8 lock
);

s8 PHY_GetTxPowerLimitx(_adapter *adapter
	, const char *regd_name
	, BAND_TYPE band, enum channel_width bw
	, u8 rfpath, u8 rate, u8 ntx_idx, u8 cch
);
#else
#define phy_get_txpwr_lmtx_abs(adapter, regd_name, band, bw, tlrs, ntx_idx, cch, lock) (GET_HAL_SPEC(adapter)->txgi_max)
#define phy_get_txpwr_lmtx(adapter, regd_name, band, bw, rfpath, rs, ntx_idx, cch, lock) (GET_HAL_SPEC(adapter)->txgi_max)
#define PHY_GetTxPowerLimitx(adapter, regd_name, band, bw, rfpath, rate, ntx_idx, cch) (GET_HAL_SPEC(adapter)->txgi_max)
#endif /* CONFIG_TXPWR_LIMIT */

s8
PHY_GetTxPowerTrackingOffsetx(
	PADAPTER	pAdapter,
	enum rf_path	RFPath,
	u8			Rate
);

struct txpwr_idx_comp {
	u8 ntx_idx;
	u8 pg;
	s8 by_rate;
	s8 limit;
	s8 tpt;
	s8 ebias;
	s8 btc;
	s8 dpd;
};

#define txpwr_idx_comp_set(_tic, _ntx_idx, _pg, _by_rate, _limit, _tpt, _ebias, _btc, _dpd) \
	do { \
		(_tic)->ntx_idx = _ntx_idx; \
		(_tic)->pg = _pg; \
		(_tic)->by_rate = _by_rate; \
		(_tic)->limit = _limit; \
		(_tic)->tpt = _tpt; \
		(_tic)->ebias = _ebias; \
		(_tic)->btc = _btc; \
		(_tic)->dpd = _dpd; \
	} while (0)

u8
phy_get_tx_power_indexx(
		PADAPTER			pAdapter,
		enum rf_path			RFPath,
		u8					Rate,
		enum channel_width	BandWidth,
		u8					Channel
);

void
PHY_SetTxPowerxIndexx(
		PADAPTER		pAdapter,
		u32				PowerIndex,
		enum rf_path		RFPath,
		u8				Rate
);

void dump_tx_power_idxx_title(void *sel, _adapter *adapter);
void dump_tx_power_idxx_by_path_rs(void *sel, _adapter *adapter, u8 rfpath, u8 rs);
void dump_tx_power_idxx(void *sel, _adapter *adapter);

bool phy_is_tx_power_limit_neededx(_adapter *adapter);
bool phy_is_tx_power_by_rate_neededx(_adapter *adapter);
int phy_load_tx_power_by_ratex(_adapter *adapter, u8 chk_file);
#if CONFIG_TXPWR_LIMIT
int phy_load_tx_power_limitx(_adapter *adapter, u8 chk_file);
#endif
void phy_load_tx_power_ext_infox(_adapter *adapter, u8 chk_file);
void phy_reload_tx_power_ext_infox(_adapter *adapter);
void phy_reload_default_tx_power_ext_infox(_adapter *adapter);

const struct map_t *hal_pg_txpwr_def_infoxx(_adapter *adapter);

#ifdef CONFIG_EFUSE_CONFIG_FILE
int check_phy_efuse_tx_power_info_validx(_adapter *adapter);
#endif

void dump_hal_txpwr_info_2gx(void *sel, _adapter *adapter, u8 rfpath_num, u8 max_tx_cnt);
void dump_hal_txpwr_info_5gx(void *sel, _adapter *adapter, u8 rfpath_num, u8 max_tx_cnt);

void hal_load_txpwr_infox(
	_adapter *adapter,
	TxPowerInfo24G *pwr_info_2g,
	TxPowerInfo5G *pwr_info_5g,
	u8 *pg_data
);

void dump_tx_power_ext_infox(void *sel, _adapter *adapter);
void dump_target_tx_powerx(void *sel, _adapter *adapter);
void dump_tx_power_by_ratex(void *sel, _adapter *adapter);

int rtw_get_phy_file_pathx(_adapter *adapter, const char *file_name);

#ifdef CONFIG_LOAD_PHY_PARA_FROM_FILE
#define MAC_FILE_FW_NIC			"FW_NIC.bin"
#define MAC_FILE_FW_WW_IMG		"FW_WoWLAN.bin"
#define PHY_FILE_MAC_REG		"MAC_REG.txt"

#define PHY_FILE_AGC_TAB		"AGC_TAB.txt"
#define PHY_FILE_PHY_REG		"PHY_REG.txt"
#define PHY_FILE_PHY_REG_MP		"PHY_REG_MP.txt"
#define PHY_FILE_PHY_REG_PG		"PHY_REG_PG.txt"

#define PHY_FILE_RADIO_A		"RadioA.txt"
#define PHY_FILE_RADIO_B		"RadioB.txt"
#define PHY_FILE_RADIO_C		"RadioC.txt"
#define PHY_FILE_RADIO_D		"RadioD.txt"
#define PHY_FILE_TXPWR_TRACK	"TxPowerTrack.txt"
#define PHY_FILE_TXPWR_LMT		"TXPWR_LMT.txt"

#define PHY_FILE_WIFI_ANT_ISOLATION	"wifi_ant_isolation.txt"

#define MAX_PARA_FILE_BUF_LEN	32768 /* 32k */

#define LOAD_MAC_PARA_FILE				BIT0
#define LOAD_BB_PARA_FILE					BIT1
#define LOAD_BB_PG_PARA_FILE				BIT2
#define LOAD_BB_MP_PARA_FILE				BIT3
#define LOAD_RF_PARA_FILE					BIT4
#define LOAD_RF_TXPWR_TRACK_PARA_FILE	BIT5
#define LOAD_RF_TXPWR_LMT_PARA_FILE		BIT6

int phy_ConfigMACWithParaFilex(PADAPTER	Adapter, char	*pFileName);
int phy_ConfigBBWithParaFilex(PADAPTER	Adapter, char	*pFileName, u32	ConfigType);
int phy_ConfigBBWithPgParaFilex(PADAPTER	Adapter, const char *pFileName);
int phy_ConfigBBWithMpParaFilex(PADAPTER	Adapter, char	*pFileName);
int PHY_ConfigRFWithParaFilex(PADAPTER	Adapter, char	*pFileName, enum rf_path	eRFPath);
int PHY_ConfigRFWithTxPwrTrackParaFilex(PADAPTER	Adapter, char	*pFileName);
#if CONFIG_TXPWR_LIMIT
int PHY_ConfigRFWithPowerLimitTableParaFilex(PADAPTER	Adapter, const char *pFileName);
#endif
void phy_free_filebufx_mask(_adapter *padapter, u8 mask);
void phy_free_filebufx(_adapter *padapter);
#endif /* CONFIG_LOAD_PHY_PARA_FROM_FILE */
u8 phy_check_under_survey_chx(_adapter *adapter);
#endif /* __HAL_COMMON_H__ */
