/******************************************************************************
 *
 * Copyright(c) 2007 - 2019 Realtek Corporation.
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
#ifndef __RTW_DEBUG_H__
#define __RTW_DEBUG_H__

/* driver log level*/
enum {
	_DRV_NONE_ = 0,
	_DRV_ALWAYS_ = 1,
	_DRV_ERR_ = 2,
	_DRV_WARNING_ = 3,
	_DRV_INFO_ = 4,
	_DRV_DEBUG_ = 5,
	_DRV_MAX_ = 6
};

#define DRIVER_PREFIX "RTW: "

#ifdef PLATFORM_OS_CE
extern void rtl871x_cedbg(const char *fmt, ...);
#endif

#ifdef PLATFORM_WINDOWS
	#define RTW_PRINT do {} while (0)
	#define RTW_ERR do {} while (0)
	#define RTW_WARN do {} while (0)
	#define RTW_INFO do {} while (0)
	#define RTW_DBG do {} while (0)
	#define RTW_PRINT_SEL do {} while (0)
	#define _RTW_PRINT do {} while (0)
	#define _RTW_ERR do {} while (0)
	#define _RTW_WARN do {} while (0)
	#define _RTW_INFO do {} while (0)
	#define _RTW_DBG do {} while (0)
	#define _RTW_PRINT_SEL do {} while (0)
#else
	#define RTW_PRINT(x, ...) do {} while (0)
	#define RTW_ERR(x, ...) do {} while (0)
	#define RTW_WARN(x,...) do {} while (0)
	#define RTW_INFO(x,...) do {} while (0)
	#define RTW_DBG(x,...) do {} while (0)
	#define RTW_PRINT_SEL(x,...) do {} while (0)
	#define _RTW_PRINT(x, ...) do {} while (0)
	#define _RTW_ERR(x, ...) do {} while (0)
	#define _RTW_WARN(x,...) do {} while (0)
	#define _RTW_INFO(x,...) do {} while (0)
	#define _RTW_DBG(x,...) do {} while (0)
	#define _RTW_PRINT_SEL(x,...) do {} while (0)
#endif

#define RTW_INFO_DUMP(_TitleString, _HexData, _HexDataLen) do {} while (0)
#define RTW_DBG_DUMP(_TitleString, _HexData, _HexDataLen) do {} while (0)
#define RTW_PRINT_DUMP(_TitleString, _HexData, _HexDataLen) do {} while (0)

#define RTW_DBG_EXPR(EXPR) do {} while (0)

#define RTW_DBGDUMP 0 /* 'stream' for _dbgdump */



#undef _dbgdump
#undef _seqdump

#if defined(PLATFORM_WINDOWS) && defined(PLATFORM_OS_XP)
	#define _dbgdump DbgPrint
	#define KERN_CONT
	#define _seqdump(sel, fmt, arg...) _dbgdump(fmt, ##arg)
#elif defined(PLATFORM_WINDOWS) && defined(PLATFORM_OS_CE)
	#define _dbgdump rtl871x_cedbg
	#define KERN_CONT
	#define _seqdump(sel, fmt, arg...) _dbgdump(fmt, ##arg)
#elif defined PLATFORM_LINUX
	#define _dbgdump printk
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24))
	#define KERN_CONT
	#endif
	#define _seqdump seq_printf
#elif defined PLATFORM_FREEBSD
	#define _dbgdump printf
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24))
	#define KERN_CONT
	#endif
	#define _seqdump(sel, fmt, arg...) _dbgdump(fmt, ##arg)
#endif

void RTW_BUF_DUMP_SELbu(uint _loglevel, void *sel, u8 *_titlestring,
								bool _idx_show, const u8 *_hexdata, int _hexdatalen);

#ifdef CONFIG_RTW_DEBUG

#ifndef _OS_INTFS_C_
extern uint rtw_drv_log_levelbu;
#endif

#if defined(_dbgdump)

#ifdef PLATFORM_LINUX
#ifdef DBG_THREAD_PID
#define T_PID_FMT	"(%5u) "
#define T_PID_ARG	current->pid
#else /* !DBG_THREAD_PID */
#define T_PID_FMT	"%s"
#define T_PID_ARG	""
#endif /* !DBG_THREAD_PID */

#ifdef DBG_CPU_INFO
#define CPU_INFO_FMT	"[%u] "
#define CPU_INFO_ARG	get_cpu()
#else /* !DBG_CPU_INFO */
#define CPU_INFO_FMT	"%s"
#define CPU_INFO_ARG	""
#endif /* !DBG_CPU_INFO */

/* Extra information in prefix */
#define EX_INFO_FMT	T_PID_FMT CPU_INFO_FMT
#define EX_INFO_ARG	T_PID_ARG, CPU_INFO_ARG
#else /* !PLATFORM_LINUX */
#define EX_INFO_FMT	"%s"
#define EX_INFO_ARG	""
#endif /* !PLATFORM_LINUX */

#define DBG_PREFIX	EX_INFO_FMT DRIVER_PREFIX
#define DBG_PREFIX_ARG	EX_INFO_ARG

/* with driver-defined prefix */
#undef RTW_PRINT
#define RTW_PRINT(fmt, arg...)     \
	do {\
		if (_DRV_ALWAYS_ <= rtw_drv_log_levelbu) {\
			_dbgdump(DBG_PREFIX fmt, DBG_PREFIX_ARG, ##arg);\
		} \
	} while (0)

#undef RTW_ERR
#define RTW_ERR(fmt, arg...)     \
	do {\
		if (_DRV_ERR_ <= rtw_drv_log_levelbu) {\
			_dbgdump(DBG_PREFIX "ERROR " fmt, \
				 DBG_PREFIX_ARG, ##arg);\
		} \
	} while (0)


#undef RTW_WARN
#define RTW_WARN(fmt, arg...)     \
	do {\
		if (_DRV_WARNING_ <= rtw_drv_log_levelbu) {\
			_dbgdump(DBG_PREFIX "WARN " fmt, \
				 DBG_PREFIX_ARG, ##arg);\
		} \
	} while (0)

#undef RTW_INFO
#define RTW_INFO(fmt, arg...)     \
	do {\
		if (_DRV_INFO_ <= rtw_drv_log_levelbu) {\
			_dbgdump(DBG_PREFIX fmt, DBG_PREFIX_ARG, ##arg);\
		} \
	} while (0)


#undef RTW_DBG
#define RTW_DBG(fmt, arg...)     \
	do {\
		if (_DRV_DEBUG_ <= rtw_drv_log_levelbu) {\
			_dbgdump(DBG_PREFIX fmt, DBG_PREFIX_ARG, ##arg);\
		} \
	} while (0)

#undef RTW_INFO_DUMP
#define RTW_INFO_DUMP(_TitleString, _HexData, _HexDataLen)	\
	RTW_BUF_DUMP_SELbu(_DRV_INFO_, RTW_DBGDUMP, _TitleString, _FALSE, _HexData, _HexDataLen)

#undef RTW_DBG_DUMP
#define RTW_DBG_DUMP(_TitleString, _HexData, _HexDataLen)	\
	RTW_BUF_DUMP_SELbu(_DRV_DEBUG_, RTW_DBGDUMP, _TitleString, _FALSE, _HexData, _HexDataLen)


#undef RTW_PRINT_DUMP
#define RTW_PRINT_DUMP(_TitleString, _HexData, _HexDataLen)	\
	RTW_BUF_DUMP_SELbu(_DRV_ALWAYS_, RTW_DBGDUMP, _TitleString, _FALSE, _HexData, _HexDataLen)

/* without driver-defined prefix */
#undef _RTW_PRINT
#define _RTW_PRINT(fmt, arg...)     \
	do {\
		if (_DRV_ALWAYS_ <= rtw_drv_log_levelbu) {\
			_dbgdump(KERN_CONT fmt, ##arg);\
		} \
	} while (0)

#undef _RTW_ERR
#define _RTW_ERR(fmt, arg...)     \
	do {\
		if (_DRV_ERR_ <= rtw_drv_log_levelbu) {\
			_dbgdump(KERN_CONT fmt, ##arg);\
		} \
	} while (0)


#undef _RTW_WARN
#define _RTW_WARN(fmt, arg...)     \
	do {\
		if (_DRV_WARNING_ <= rtw_drv_log_levelbu) {\
			_dbgdump(KERN_CONT fmt, ##arg);\
		} \
	} while (0)

#undef _RTW_INFO
#define _RTW_INFO(fmt, arg...)     \
	do {\
		if (_DRV_INFO_ <= rtw_drv_log_levelbu) {\
			_dbgdump(KERN_CONT fmt, ##arg);\
		} \
	} while (0)

#undef _RTW_DBG
#define _RTW_DBG(fmt, arg...)     \
	do {\
		if (_DRV_DEBUG_ <= rtw_drv_log_levelbu) {\
			_dbgdump(KERN_CONT fmt, ##arg);\
		} \
	} while (0)


/* other debug APIs */
#undef RTW_DBG_EXPR
#define RTW_DBG_EXPR(EXPR) do { if (_DRV_DEBUG_ <= rtw_drv_log_levelbu) EXPR; } while (0)

#endif /* defined(_dbgdump) */
#endif /* CONFIG_RTW_DEBUG */


#if defined(_seqdump)
/* dump message to selected 'stream' with driver-defined prefix */
#undef RTW_PRINT_SEL
#define RTW_PRINT_SEL(sel, fmt, arg...) \
	do {\
		if (sel == RTW_DBGDUMP)\
			RTW_PRINT(fmt, ##arg); \
		else {\
			_seqdump(sel, fmt, ##arg) /*rtw_warn_on(1)*/; \
		} \
	} while (0)

/* dump message to selected 'stream' */
#undef _RTW_PRINT_SEL
#define _RTW_PRINT_SEL(sel, fmt, arg...) \
	do {\
		if (sel == RTW_DBGDUMP)\
			_RTW_PRINT(fmt, ##arg); \
		else {\
			_seqdump(sel, fmt, ##arg) /*rtw_warn_on(1)*/; \
		} \
	} while (0)

/* dump message to selected 'stream' */
#undef RTW_DUMP_SEL
#define RTW_DUMP_SEL(sel, _HexData, _HexDataLen) \
	RTW_BUF_DUMP_SELbu(_DRV_ALWAYS_, sel, NULL, _FALSE, _HexData, _HexDataLen)

#define RTW_MAP_DUMP_SEL(sel, _TitleString, _HexData, _HexDataLen) \
	RTW_BUF_DUMP_SELbu(_DRV_ALWAYS_, sel, _TitleString, _TRUE, _HexData, _HexDataLen)
#endif /* defined(_seqdump) */


#ifdef CONFIG_DBG_COUNTER
	#define DBG_COUNTER(counter) counter++
#else
	#define DBG_COUNTER(counter)
#endif

void dump_drv_versionbu(void *sel);
void dump_log_levelbu(void *sel);

#ifdef CONFIG_SDIO_HCI
void sd_f0_reg_dump(void *sel, _adapter *adapter);
void sdio_local_reg_dump(void *sel, _adapter *adapter);
#endif /* CONFIG_SDIO_HCI */

void mac_reg_dumpbu(void *sel, _adapter *adapter);
void bb_reg_dumpbu(void *sel, _adapter *adapter);
void bb_reg_dumpbu_ex(void *sel, _adapter *adapter);
void rf_reg_dumpbu(void *sel, _adapter *adapter);

void rtw_sink_rtp_seq_dbgbu(_adapter *adapter, u8 *ehdr_pos);

struct sta_info;
void sta_rx_reorder_ctl_dumpbu(void *sel, struct sta_info *sta);

struct dvobj_priv;
void dump_tx_rate_bmpbu(void *sel, struct dvobj_priv *dvobj);
void dump_adapters_statusbu(void *sel, struct dvobj_priv *dvobj);

struct sec_cam_ent;
#if defined(CONFIG_RTW_DEBUG) || defined(CONFIG_PROC_DEBUG)
void dump_sec_cambu_entbu(void *sel, struct sec_cam_ent *ent, int id);
void dump_sec_cambu_entbu_title(void *sel, u8 has_id);
#endif
void dump_sec_cambu(void *sel, _adapter *adapter);
void dump_sec_cambu_cache(void *sel, _adapter *adapter);

bool rtw_fwdl_test_trigger_chksum_failbu(void);
bool rtw_fwdl_test_trigger_wintint_rdy_failbu(void);
bool rtw_del_rx_ampdu_test_trigger_no_tx_failbu(void);
u32 rtw_get_wait_hiq_empty_msbu(void);
void rtw_sta_linking_test_set_startbu(void);
bool rtw_sta_linking_test_wait_donebu(void);
bool rtw_sta_linking_test_force_failbu(void);
#ifdef CONFIG_AP_MODE
u16 rtw_ap_linking_test_force_auth_failbu(void);
u16 rtw_ap_linking_test_force_asoc_failbu(void);
#endif

#ifdef CONFIG_PROC_DEBUG
ssize_t proc_set_write_regbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_read_regbu(struct seq_file *m, void *v);
ssize_t proc_set_read_regbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
void dump_drv_cfgbu(void *sel);
int proc_get_fwstatebu(struct seq_file *m, void *v);
int proc_get_sec_infobu(struct seq_file *m, void *v);
int proc_get_mlmext_statebu(struct seq_file *m, void *v);
#ifdef CONFIG_LAYER2_ROAMING
int proc_get_roam_flagsbu(struct seq_file *m, void *v);
ssize_t proc_set_roam_flagsbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_roam_parambu(struct seq_file *m, void *v);
ssize_t proc_set_roam_parambu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
ssize_t proc_set_roam_tgt_addrbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif /* CONFIG_LAYER2_ROAMING */
int proc_get_qos_optionbu(struct seq_file *m, void *v);
int proc_get_ht_optionbu(struct seq_file *m, void *v);
int proc_get_rf_infobu(struct seq_file *m, void *v);
int proc_get_scan_parambu(struct seq_file *m, void *v);
ssize_t proc_set_scan_parambu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_scan_abortbu(struct seq_file *m, void *v);
#ifdef CONFIG_RTW_REPEATER_SON
int proc_get_rson_data(struct seq_file *m, void *v);
ssize_t proc_set_rson_data(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif
int proc_get_survey_infobu(struct seq_file *m, void *v);
ssize_t proc_set_survey_infobu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_ap_infobu(struct seq_file *m, void *v);
#ifdef ROKU_PRIVATE
int proc_get_infra_ap(struct seq_file *m, void *v);
#endif /* ROKU_PRIVATE */
ssize_t proc_reset_trx_infobu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_trx_infobu(struct seq_file *m, void *v);
ssize_t proc_set_tx_power_offsetbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_tx_power_offsetbu(struct seq_file *m, void *v);
int proc_get_rate_ctlbu(struct seq_file *m, void *v);
int proc_get_wifi_specbu(struct seq_file *m, void *v);
ssize_t proc_set_rate_ctlbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_bw_ctlbu(struct seq_file *m, void *v);
ssize_t proc_set_bw_ctlbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#ifdef DBG_RX_COUNTER_DUMP
int proc_get_rx_cnt_dump(struct seq_file *m, void *v);
ssize_t proc_set_rx_cnt_dump(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif

#ifdef CONFIG_AP_MODE
int proc_get_bmc_tx_ratebu(struct seq_file *m, void *v);
ssize_t proc_set_bmc_tx_ratebu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif /*CONFIG_AP_MODE*/

int proc_get_ps_dbg_infobu(struct seq_file *m, void *v);
ssize_t proc_set_ps_dbg_infobu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

ssize_t proc_set_fwdl_test_casebu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
ssize_t proc_set_del_rx_ampdu_test_casebu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
ssize_t proc_set_wait_hiq_emptybu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
ssize_t proc_set_sta_linking_testbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#ifdef CONFIG_AP_MODE
ssize_t proc_set_ap_linking_testbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif

int proc_get_rx_statbu(struct seq_file *m, void *v);
int proc_get_tx_statbu(struct seq_file *m, void *v);
#ifdef CONFIG_AP_MODE
int proc_get_all_sta_infobu(struct seq_file *m, void *v);
#endif /* CONFIG_AP_MODE */

#ifdef DBG_MEMORY_LEAK
int proc_get_malloc_cnt(struct seq_file *m, void *v);
#endif /* DBG_MEMORY_LEAK */

#ifdef CONFIG_FIND_BEST_CHANNEL
int proc_get_best_channelbu(struct seq_file *m, void *v);
ssize_t proc_set_best_channelbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif /* CONFIG_FIND_BEST_CHANNEL */

int proc_get_trx_infobu_debug(struct seq_file *m, void *v);

#ifdef CONFIG_HUAWEI_PROC
int proc_get_huawei_trx_info(struct seq_file *m, void *v);
#endif

int proc_get_rx_signalbu(struct seq_file *m, void *v);
ssize_t proc_set_rx_signalbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_hw_statusbu(struct seq_file *m, void *v);
ssize_t proc_set_hw_statusbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_mac_rptbufbu(struct seq_file *m, void *v);

#ifdef CONFIG_80211N_HT
int proc_get_ht_enablebu(struct seq_file *m, void *v);
ssize_t proc_set_ht_enablebu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

int proc_get_bw_modebu(struct seq_file *m, void *v);
ssize_t proc_set_bw_modebu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

int proc_get_ampdu_enablebu(struct seq_file *m, void *v);
ssize_t proc_set_ampdu_enablebu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

void dump_regsty_rx_ampdu_size_limitbu(void *sel, _adapter *adapter);
int proc_get_rx_ampdubu(struct seq_file *m, void *v);
ssize_t proc_set_rx_ampdubu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

void rtw_dump_dft_phy_capbu(void *sel, _adapter *adapter);
void rtw_get_dft_phy_capbu(void *sel, _adapter *adapter);
void rtw_dump_drv_phy_capbu(void *sel, _adapter *adapter);

int proc_get_rx_stbcbu(struct seq_file *m, void *v);
ssize_t proc_set_rx_stbcbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_stbc_capbu(struct seq_file *m, void *v);
ssize_t proc_set_stbc_capbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_ldpc_capbu(struct seq_file *m, void *v);
ssize_t proc_set_ldpc_capbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#ifdef CONFIG_BEAMFORMING
int proc_get_txbf_capbu(struct seq_file *m, void *v);
ssize_t proc_set_txbf_capbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif
#ifdef CONFIG_SDIO_TX_ENABLE_AVAL_INT
int proc_get_tx_aval_th(struct seq_file *m, void *v);
ssize_t proc_set_tx_aval_th(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif /*CONFIG_SDIO_TX_ENABLE_AVAL_INT*/
int proc_get_rx_ampdubu_factor(struct seq_file *m, void *v);
ssize_t proc_set_rx_ampdubu_factor(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

int proc_get_tx_max_agg_numbu(struct seq_file *m, void *v);
ssize_t proc_set_tx_max_agg_numbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

int proc_get_rx_ampdubu_density(struct seq_file *m, void *v);
ssize_t proc_set_rx_ampdubu_density(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

int proc_get_tx_ampdu_densitybu(struct seq_file *m, void *v);
ssize_t proc_set_tx_ampdu_densitybu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

int proc_get_tx_quick_addba_req(struct seq_file *m, void *v);
ssize_t proc_set_tx_quick_addba_req(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#ifdef CONFIG_TX_AMSDU
int proc_get_tx_amsdu(struct seq_file *m, void *v);
ssize_t proc_set_tx_amsdu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_tx_amsdu_rate(struct seq_file *m, void *v);
ssize_t proc_set_tx_amsdu_rate(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif
#endif /* CONFIG_80211N_HT */

#ifdef CONFIG_80211AC_VHT
int proc_get_vht_24g_enable(struct seq_file *m, void *v);
ssize_t proc_set_vht_24g_enable(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif

ssize_t proc_set_dyn_rrsr(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_dyn_rrsr(struct seq_file *m, void *v);

int proc_get_en_fwpsbu(struct seq_file *m, void *v);
ssize_t proc_set_en_fwpsbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

#if 0
int proc_get_two_path_rssi(struct seq_file *m, void *v);
int proc_get_rssi_disp(struct seq_file *m, void *v);
ssize_t proc_set_rssi_disp(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif

#ifdef CONFIG_BT_COEXIST
int proc_get_btcoex_dbg(struct seq_file *m, void *v);
ssize_t proc_set_btcoex_dbg(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_btcoex_info(struct seq_file *m, void *v);
#ifdef CONFIG_RF4CE_COEXIST
int proc_get_rf4ce_state(struct seq_file *m, void *v);
ssize_t proc_set_rf4ce_state(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif
#endif /* CONFIG_BT_COEXIST */

#if defined(DBG_CONFIG_ERROR_DETECT)
int proc_get_sresetbu(struct seq_file *m, void *v);
ssize_t proc_set_sresetbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif /* DBG_CONFIG_ERROR_DETECT */

int proc_get_odm_adaptivitybu(struct seq_file *m, void *v);
ssize_t proc_set_odm_adaptivitybu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

#ifdef CONFIG_DBG_COUNTER
int proc_get_rx_logs(struct seq_file *m, void *v);
int proc_get_tx_logs(struct seq_file *m, void *v);
int proc_get_int_logs(struct seq_file *m, void *v);
#endif

#ifdef CONFIG_PCI_HCI
int proc_get_rx_ring(struct seq_file *m, void *v);
int proc_get_tx_ring(struct seq_file *m, void *v);
int proc_get_pci_aspm(struct seq_file *m, void *v);
int proc_get_pci_conf_space(struct seq_file *m, void *v);
ssize_t proc_set_pci_conf_space(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

int proc_get_pci_bridge_conf_space(struct seq_file *m, void *v);
ssize_t proc_set_pci_bridge_conf_space(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);


#ifdef DBG_TXBD_DESC_DUMP
int proc_get_tx_ring_ext(struct seq_file *m, void *v);
ssize_t proc_set_tx_ring_ext(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif
#endif

#ifdef CONFIG_WOWLAN
int proc_get_wow_enable(struct seq_file *m, void *v);
ssize_t proc_set_wow_enable(struct file *file, const char __user *buffer,
		size_t count, loff_t *pos, void *data);
int proc_get_pattern_info(struct seq_file *m, void *v);
ssize_t proc_set_pattern_info(struct file *file, const char __user *buffer,
		size_t count, loff_t *pos, void *data);
int proc_get_wakeup_event(struct seq_file *m, void *v);
ssize_t proc_set_wakeup_event(struct file *file, const char __user *buffer,
		size_t count, loff_t *pos, void *data);
int proc_get_wakeup_reason(struct seq_file *m, void *v);
#ifdef CONFIG_WOW_KEEP_ALIVE_PATTERN
int proc_dump_wow_keep_alive_info(struct seq_file *m, void *v);
#endif /*CONFIG_WOW_KEEP_ALIVE_PATTERN*/
#endif

#ifdef CONFIG_WAR_OFFLOAD
int proc_get_war_offload_enable(struct seq_file *m, void *v);
ssize_t proc_set_war_offload_enable(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_war_offload_ipv4_addr(struct seq_file *m, void *v);
ssize_t proc_set_war_offload_ipv4_addr(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_war_offload_ipv6_addr(struct seq_file *m, void *v);
ssize_t proc_set_war_offload_ipv6_addr(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_war_offload_mdns_domain_name(struct seq_file *m, void *v);
ssize_t proc_set_war_offload_mdns_domain_name(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_war_offload_mdns_machine_name(struct seq_file *m, void *v);
ssize_t proc_set_war_offload_mdns_machine_name(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_war_offload_mdns_txt_rsp(struct seq_file *m, void *v);
ssize_t proc_set_war_offload_mdns_txt_rsp(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_war_offload_mdns_service_info(struct seq_file *m, void *v);
ssize_t proc_set_war_offload_mdns_service_info(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif /* CONFIG_WAR_OFFLOAD */


#ifdef CONFIG_GPIO_WAKEUP
int proc_get_wowlan_gpio_info(struct seq_file *m, void *v);
ssize_t proc_set_wowlan_gpio_info(struct file *file, const char __user *buffer,
		size_t count, loff_t *pos, void *data);
#endif /*CONFIG_GPIO_WAKEUP*/

#ifdef CONFIG_P2P_WOWLAN
int proc_get_p2p_wowlan_info(struct seq_file *m, void *v);
#endif /* CONFIG_P2P_WOWLAN */

int proc_get_new_bcn_max(struct seq_file *m, void *v);
ssize_t proc_set_new_bcn_max(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

#ifdef CONFIG_POWER_SAVING
int proc_get_ps_info(struct seq_file *m, void *v);
ssize_t proc_set_ps_info(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#ifdef CONFIG_WMMPS_STA	
int proc_get_wmmps_info(struct seq_file *m, void *v);
ssize_t proc_set_wmmps_info(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif /* CONFIG_WMMPS_STA */
#endif /* CONFIG_POWER_SAVING */

#ifdef CONFIG_TDLS
int proc_get_tdls_enable(struct seq_file *m, void *v);
ssize_t proc_set_tdls_enable(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_tdls_info(struct seq_file *m, void *v);
#endif

int proc_get_monitorbu(struct seq_file *m, void *v);
ssize_t proc_set_monitorbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

#ifdef RTW_SIMPLE_CONFIG
int proc_get_simple_config(struct seq_file *m, void *v);
ssize_t proc_set_simple_config(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif

#ifdef DBG_XMIT_BLOCK
int proc_get_xmit_block(struct seq_file *m, void *v);
ssize_t proc_set_xmit_block(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif

#ifdef CONFIG_PREALLOC_RX_SKB_BUFFER
int proc_get_rtkm_info(struct seq_file *m, void *v);
#endif /* CONFIG_PREALLOC_RX_SKB_BUFFER */

#ifdef CONFIG_IEEE80211W
ssize_t proc_set_tx_sa_querybu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_tx_sa_querybu(struct seq_file *m, void *v);
ssize_t proc_set_tx_deauthbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_tx_deauthbu(struct seq_file *m, void *v);
ssize_t proc_set_tx_authbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_tx_authbu(struct seq_file *m, void *v);
#endif /* CONFIG_IEEE80211W */

#endif /* CONFIG_PROC_DEBUG */

int proc_get_efuse_mapbu(struct seq_file *m, void *v);
ssize_t proc_set_efuse_mapbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

#ifdef CONFIG_CUSTOMER01_SMART_ANTENNA
int proc_get_pathb_phase(struct seq_file *m, void *v);
ssize_t proc_set_pathb_phase(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif

#ifdef CONFIG_MCC_MODE
int proc_get_mcc_info(struct seq_file *m, void *v);
ssize_t proc_set_mcc_enable(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
ssize_t proc_set_mcc_duration(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#ifdef CONFIG_MCC_PHYDM_OFFLOAD
ssize_t proc_set_mcc_phydm_offload_enable(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif
ssize_t proc_set_mcc_single_tx_criteria(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
ssize_t proc_set_mcc_ap_bw20_target_tp(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
ssize_t proc_set_mcc_ap_bw40_target_tp(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
ssize_t proc_set_mcc_ap_bw80_target_tp(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
ssize_t proc_set_mcc_sta_bw20_target_tp(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
ssize_t proc_set_mcc_sta_bw40_target_tp(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
ssize_t proc_set_mcc_sta_bw80_target_tp(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_mcc_policy_table(struct seq_file *m, void *v);
#endif /* CONFIG_MCC_MODE */

int proc_get_ack_timeoutbu(struct seq_file *m, void *v);
ssize_t proc_set_ack_timeoutbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

int proc_get_fw_offloadbu(struct seq_file *m, void *v);
ssize_t proc_set_fw_offloadbu(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

#ifdef CONFIG_FW_HANDLE_TXBCN
ssize_t proc_set_fw_tbtt_rpt(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_fw_tbtt_rpt(struct seq_file *m, void *v);
#endif

#ifdef CONFIG_DBG_RF_CAL
int proc_get_iqk_info(struct seq_file *m, void *v);
ssize_t proc_set_iqk(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_lck_info(struct seq_file *m, void *v);
ssize_t proc_set_lck(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
#endif /*CONFIG_DBG_RF_CAL*/

#ifdef CONFIG_CTRL_TXSS_BY_TP
ssize_t proc_set_txss_tp(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_txss_tp(struct seq_file *m, void *v);
#ifdef DBG_CTRL_TXSS
ssize_t proc_set_txss_ctrl(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_txss_ctrl(struct seq_file *m, void *v);
#endif
#endif

#ifdef CONFIG_LPS_CHK_BY_TP
ssize_t proc_set_lps_chk_tp(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_lps_chk_tp(struct seq_file *m, void *v);
#endif

#ifdef CONFIG_SUPPORT_STATIC_SMPS
ssize_t proc_set_smps(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);
int proc_get_smps(struct seq_file *m, void *v);
#endif

int proc_get_defs_param(struct seq_file *m, void *v);
ssize_t proc_set_defs_param(struct file *file, const char __user *buffer, size_t count, loff_t *pos, void *data);

#define _drv_always_		1
#define _drv_emerg_			2
#define _drv_alert_			3
#define _drv_crit_			4
#define _drv_err_			5
#define _drv_warning_		6
#define _drv_notice_		7
#define _drv_info_			8
#define _drv_dump_			9
#define _drv_debug_			10

#define _module_rtl871x_xmit_c_		BIT(0)
#define _module_xmit_osdep_c_		BIT(1)
#define _module_rtl871x_recv_c_		BIT(2)
#define _module_recv_osdep_c_		BIT(3)
#define _module_rtl871x_mlme_c_		BIT(4)
#define _module_mlme_osdep_c_		BIT(5)
#define _module_rtl871x_sta_mgt_c_		BIT(6)
#define _module_rtl871x_cmd_c_			BIT(7)
#define _module_cmd_osdep_c_		BIT(8)
#define _module_rtl871x_io_c_				BIT(9)
#define _module_io_osdep_c_		BIT(10)
#define _module_os_intfs_c_			BIT(11)
#define _module_rtl871x_security_c_		BIT(12)
#define _module_rtl871x_eeprom_c_			BIT(13)
#define _module_hal_init_c_		BIT(14)
#define _module_hci_hal_init_c_		BIT(15)
#define _module_rtl871x_ioctl_c_		BIT(16)
#define _module_rtl871x_ioctl_set_c_		BIT(17)
#define _module_rtl871x_ioctl_query_c_	BIT(18)
#define _module_rtl871x_pwrctrl_c_			BIT(19)
#define _module_hci_intfs_c_			BIT(20)
#define _module_hci_ops_c_			BIT(21)
#define _module_osdep_service_c_			BIT(22)
#define _module_mp_			BIT(23)
#define _module_hci_ops_os_c_			BIT(24)
#define _module_rtl871x_ioctl_os_c		BIT(25)
#define _module_rtl8712_cmd_c_		BIT(26)
/* #define _module_efuse_			BIT(27) */
#define	_module_rtl8192c_xmit_c_ BIT(28)
#define _module_hal_xmit_c_	BIT(28)
#define _module_efuse_			BIT(29)
#define _module_rtl8712_recv_c_		BIT(30)
#define _module_rtl8712_led_c_		BIT(31)

#endif /* __RTW_DEBUG_H__ */
