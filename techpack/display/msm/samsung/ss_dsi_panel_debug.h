/*
 * =================================================================
 *
 *	Description:  samsung display debug common file
 *	Company:  Samsung Electronics
 *
 * ================================================================
 *
 *
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015, Samsung Electronics. All rights reserved.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef _SAMSUNG_DSI_PANEL_DEBUG_H_
#define _SAMSUNG_DSI_PANEL_DEBUG_H_


//#include "ss_dsi_panel_common.h"
#ifdef CONFIG_SEC_DEBUG
#include <linux/sec_debug.h>
#endif


struct samsung_display_driver_data;

/**** SAMSUNG XLOG *****/
#define SS_XLOG_ENTRY 256
#define SS_XLOG_BUF_MAX 128
#define SS_XLOG_MAX_DATA 7
#define SS_XLOG_BUF_ALIGN_TIME 14
#define SS_XLOG_BUF_ALIGN_NAME 32
#define SS_XLOG_START 0x1111
#define SS_XLOG_FINISH 0xFFFF
#define SS_XLOG_PANIC_DBG_LENGTH 256
#define SS_XLOG_DPCI_LENGTH (700 - 1)
#define DATA_LIMITER (-1)

enum FW_UP_OP {
	FW_UP_TRY,
	FW_FAIL_LINE,
	FW_UP_FAIL,
	FW_UP_PASS,
	FW_UP_RESET, /* For debug */
	FW_UP_MAX,
};

enum mdss_samsung_xlog_flag {
	SS_XLOG_DEFAULT,
	SS_XLOG_BIGDATA,
	SS_XLOG_MAX
};

struct ss_tlog {
	int pid;
	s64 time;
	u32 data[SS_XLOG_MAX_DATA];
	u32 data_cnt;
	const char *name;
};

/* PANEL DEBUG FUNCTION */
void ss_xlog(const char *name, int flag, ...);
void ss_xlog_vsync(const char *name, int flag, ...);
void ss_dump_xlog(void);
void ss_store_xlog_panic_dbg(void);
int ss_panel_debug_init(struct samsung_display_driver_data *vdd);

#define SS_XLOG(...) ss_xlog(__func__, SS_XLOG_DEFAULT, \
		##__VA_ARGS__, DATA_LIMITER)
#define SS_XLOG_BG(...) ss_xlog(__func__, SS_XLOG_BIGDATA, \
		##__VA_ARGS__, DATA_LIMITER)

#define SS_XLOG_VSYNC(...) ss_xlog_vsync(__func__, SS_XLOG_DEFAULT, \
		##__VA_ARGS__, DATA_LIMITER)

/**
 *	sde & sde-rotator smmu debug
 */
enum ss_smmu_type {
	SMMU_RT_DISPLAY_DEBUG,
	SMMU_NRT_ROTATOR_DEBUG,
	SMMU_MAX_DEBUG,
};

struct ss_smmu_logging {
	ktime_t time;

	struct sg_table *table; /*To compare with whole ion buffer(page_link) */

	struct list_head list;
};

struct ss_smmu_debug {
	int init_done;

	struct list_head list;
	spinlock_t lock;
};

struct ss_image_logging {
	uint32_t	dma_address;
	int src_width, src_height;
	int src_format;
};

int ss_read_rddpm(struct samsung_display_driver_data *vdd);
int ss_read_rddsm(struct samsung_display_driver_data *vdd);
int ss_read_errfg(struct samsung_display_driver_data *vdd);
int ss_read_dsierr(struct samsung_display_driver_data *vdd);
int ss_read_mipi_protocol_err(struct samsung_display_driver_data *vdd);
int ss_read_self_diag(struct samsung_display_driver_data *vdd);
int ss_read_ddi_cmd_log(struct samsung_display_driver_data *vdd, char *read_buf);
int ss_read_pps_data(struct samsung_display_driver_data *vdd);

int ss_smmu_debug_init(struct samsung_display_driver_data *vdd);
void ss_smmu_debug_map(enum ss_smmu_type type, struct sg_table *table);
void ss_smmu_debug_unmap(enum ss_smmu_type type, struct sg_table *table);
void ss_smmu_debug_log(void);
void ss_image_logging_update(uint32_t plane_addr, int width, int height, int src_format);

bool ss_read_debug_partition(struct lcd_debug_t *value);
bool ss_write_debug_partition(struct lcd_debug_t *value);

void ss_inc_ftout_debug(const char *name);
int ss_write_fw_up_debug_partition(enum FW_UP_OP op, uint32_t addr);
void ss_read_fw_up_debug_partition(void);

#if 0 // tmp, comment in until SS bsp team bringup sec debug feature...
extern bool read_debug_partition(enum debug_partition_index index, void *value);
extern bool write_debug_partition(enum debug_partition_index index, void *value);
#endif

#endif
