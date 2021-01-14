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
 * Copyright (C) 2017, Samsung Electronics. All rights reserved.
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

#include "ss_dsi_panel_common.h"

bool enable_pr_debug;

#if 1 // case 04436106
DEFINE_SPINLOCK(ss_xlock_vsync);

static struct ss_dbg_xlog_vsync {
	struct ss_tlog logs[SS_XLOG_ENTRY];
	u32 first;
	u32 last;
	u32 curr;
	struct dentry *ss_xlog;
	u32 xlog_enable;
	u32 panic_on_err;
} ss_dbg_xlog_vsync;


/************************************************************
 *
 *		Samsung XLOG & DUMP Function
 *
 **************************************************************/

void ss_xlog_vsync(const char *name, int flag, ...)
{
	unsigned long flags;
	int i, val = 0;
	va_list args;
	struct ss_tlog *log;

	spin_lock_irqsave(&ss_xlock_vsync, flags);
	log = &ss_dbg_xlog_vsync.logs[ss_dbg_xlog_vsync.curr];
	log->time = ktime_to_us(ktime_get());
	log->name = name;
	log->pid = current->pid;

	va_start(args, flag);
	for (i = 0; i < SS_XLOG_MAX_DATA; i++) {
		val = va_arg(args, int);
		if (val == DATA_LIMITER)
			break;

		log->data[i] = val;
	}
	va_end(args);
	log->data_cnt = i;
	ss_dbg_xlog_vsync.curr = (ss_dbg_xlog_vsync.curr + 1) % SS_XLOG_ENTRY;
	ss_dbg_xlog_vsync.last++;

	spin_unlock_irqrestore(&ss_xlock_vsync, flags);
}

static bool __ss_dump_xlog_calc_range_vsync(void)
{
	static u32 next;
	bool need_dump = true;
	unsigned long flags;
	struct ss_dbg_xlog_vsync *xlog = &ss_dbg_xlog_vsync;

	spin_lock_irqsave(&ss_xlock_vsync, flags);

	xlog->first = next;

	if (xlog->last == xlog->first) {
		need_dump = false;
		goto dump_exit;
	}

	if (xlog->last < xlog->first) {
		xlog->first %= SS_XLOG_ENTRY;
		if (xlog->last < xlog->first)
			xlog->last += SS_XLOG_ENTRY;
	}

	if ((xlog->last - xlog->first) > SS_XLOG_ENTRY) {
		pr_warn("xlog buffer overflow before dump: %d\n",
			xlog->last - xlog->first);
		xlog->first = xlog->last - SS_XLOG_ENTRY;
	}

	next = xlog->first + 1;

dump_exit:
	spin_unlock_irqrestore(&ss_xlock_vsync, flags);

	return need_dump;
}

static ssize_t ss_xlog_dump_entry_vsync(char *xlog_buf, ssize_t xlog_buf_size)
{
	int i;
	ssize_t off = 0;
	struct ss_tlog *log;
	unsigned long flags;

	spin_lock_irqsave(&ss_xlock_vsync, flags);

	log = &ss_dbg_xlog_vsync.logs[ss_dbg_xlog_vsync.first %
		SS_XLOG_ENTRY];

	off = snprintf((xlog_buf + off), (xlog_buf_size - off),
		"[%5llu.%6llu]:", log->time/1000000, log->time%1000000);

	if (off < SS_XLOG_BUF_ALIGN_TIME) {
		memset((xlog_buf + off), 0x20, (SS_XLOG_BUF_ALIGN_TIME - off));
		off = SS_XLOG_BUF_ALIGN_TIME;
	}

	off += snprintf((xlog_buf + off), (xlog_buf_size - off), "%s => ",
		log->name);

	for (i = 0; i < log->data_cnt; i++)
		off += snprintf((xlog_buf + off), (xlog_buf_size - off),
			"%x ", log->data[i]);

	off += snprintf((xlog_buf + off), (xlog_buf_size - off), "\n");

	spin_unlock_irqrestore(&ss_xlock_vsync, flags);

	return off;
}
#endif

DEFINE_SPINLOCK(ss_xlock);

static struct ss_dbg_xlog {
	struct ss_tlog logs[SS_XLOG_ENTRY];
	u32 first;
	u32 last;
	u32 curr;
	struct dentry *ss_xlog;
	u32 xlog_enable;
	u32 panic_on_err;
} ss_dbg_xlog;


/************************************************************
 *
 *		Samsung XLOG & DUMP Function
 *
 **************************************************************/

void ss_xlog(const char *name, int flag, ...)
{
	unsigned long flags;
	int i, val = 0;
	va_list args;
	struct ss_tlog *log;

	spin_lock_irqsave(&ss_xlock, flags);
	log = &ss_dbg_xlog.logs[ss_dbg_xlog.curr];
	log->time = ktime_to_us(ktime_get());
	log->name = name;
	log->pid = current->pid;

	va_start(args, flag);
	for (i = 0; i < SS_XLOG_MAX_DATA; i++) {
		val = va_arg(args, int);
		if (val == DATA_LIMITER)
			break;

		log->data[i] = val;
	}
	va_end(args);
	log->data_cnt = i;
	ss_dbg_xlog.curr = (ss_dbg_xlog.curr + 1) % SS_XLOG_ENTRY;
	ss_dbg_xlog.last++;

	spin_unlock_irqrestore(&ss_xlock, flags);
}

static bool __ss_dump_xlog_calc_range(void)
{
	static u32 next;
	bool need_dump = true;
	unsigned long flags;
	struct ss_dbg_xlog *xlog = &ss_dbg_xlog;

	spin_lock_irqsave(&ss_xlock, flags);

	xlog->first = next;

	if (xlog->last == xlog->first) {
		need_dump = false;
		goto dump_exit;
	}

	if (xlog->last < xlog->first) {
		xlog->first %= SS_XLOG_ENTRY;
		if (xlog->last < xlog->first)
			xlog->last += SS_XLOG_ENTRY;
	}

	if ((xlog->last - xlog->first) > SS_XLOG_ENTRY) {
		pr_warn("xlog buffer overflow before dump: %d\n",
			xlog->last - xlog->first);
		xlog->first = xlog->last - SS_XLOG_ENTRY;
	}

	next = xlog->first + 1;

dump_exit:
	spin_unlock_irqrestore(&ss_xlock, flags);

	return need_dump;
}

static ssize_t ss_xlog_dump_entry(char *xlog_buf, ssize_t xlog_buf_size)
{
	int i;
	ssize_t off = 0;
	struct ss_tlog *log;
	unsigned long flags;

	spin_lock_irqsave(&ss_xlock, flags);

	log = &ss_dbg_xlog.logs[ss_dbg_xlog.first %
		SS_XLOG_ENTRY];

	off = snprintf((xlog_buf + off), (xlog_buf_size - off),
		"[%5llu.%6llu]:", log->time/1000000, log->time%1000000);

	if (off < SS_XLOG_BUF_ALIGN_TIME) {
		memset((xlog_buf + off), 0x20, (SS_XLOG_BUF_ALIGN_TIME - off));
		off = SS_XLOG_BUF_ALIGN_TIME;
	}

	off += snprintf((xlog_buf + off), (xlog_buf_size - off), "%s => ",
		log->name);

	for (i = 0; i < log->data_cnt; i++)
		off += snprintf((xlog_buf + off), (xlog_buf_size - off),
			"%x ", log->data[i]);

	off += snprintf((xlog_buf + off), (xlog_buf_size - off), "\n");

	spin_unlock_irqrestore(&ss_xlock, flags);

	return off;
}

void ss_dump_xlog(void)
{
	char xlog_buf[SS_XLOG_BUF_MAX] = {0,};

	pr_info("============ Start Samsung XLOG ============\n");

	while (__ss_dump_xlog_calc_range()) {
		ss_xlog_dump_entry(xlog_buf, SS_XLOG_BUF_MAX);
		pr_info("%s", xlog_buf);
	}

	pr_info("============ Finish Samsung XLOG ============\n");
}

static ssize_t ss_xlog_dump_read(struct file *file, char __user *buff,
		size_t count, loff_t *ppos)
{
	ssize_t len = 0;
	char xlog_buf[SS_XLOG_BUF_MAX];

	if (__ss_dump_xlog_calc_range()) {
		len = ss_xlog_dump_entry(xlog_buf, SS_XLOG_BUF_MAX);

		if (len < 0 || len > count) {
			pr_err("len is more than user buffer size");
			return 0;
		}

		if (copy_to_user(buff, xlog_buf, len))
			return -EFAULT;
		*ppos += len;
	}
#if 1 // case 04436106
	else if (__ss_dump_xlog_calc_range_vsync()) {
		len = ss_xlog_dump_entry_vsync(xlog_buf, SS_XLOG_BUF_MAX);

		if (len < 0 || len > count) {
			pr_err("len is more than user buffer size");
			return 0;
		}

		if (copy_to_user(buff, xlog_buf, len))
			return -EFAULT;
		*ppos += len;
	}
#endif

	/* temp: print vsync log.. case 04436106 */

	return len;
}

static int ss_xlog_dump_show(struct seq_file *s, void *unused)
{
	ssize_t len = 0;
	char xlog_buf[SS_XLOG_BUF_MAX];

	if (__ss_dump_xlog_calc_range()) {
		len = ss_xlog_dump_entry(xlog_buf, SS_XLOG_BUF_MAX);
		seq_printf(s, "%s", xlog_buf);
	}

	return len;
}

static int ss_xlog_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, ss_xlog_dump_show, inode->i_private);
}

void ss_store_xlog_panic_dbg(void)
{
	int last, i;
	ssize_t len = 0;
	char err_buf[SS_XLOG_PANIC_DBG_LENGTH] = {0,};
	struct ss_tlog *log;
	struct ss_dbg_xlog *xlog = &ss_dbg_xlog;
	struct samsung_display_driver_data *vdd_primary = ss_get_vdd(PRIMARY_DISPLAY_NDX);
	struct samsung_display_driver_data *vdd_secondary = ss_get_vdd(SECONDARY_DISPLAY_NDX);

	last = xlog->last;
	if (last)
		last--;

	while (last >= 0) {
		log = &ss_dbg_xlog.logs[last % SS_XLOG_ENTRY];
		len += snprintf((err_buf + len), (sizeof(err_buf) - len),
			"[%llu.%3llu]%s=>", log->time/1000000,
			log->time%1000000, log->name);
		if (len >= SS_XLOG_PANIC_DBG_LENGTH)
			goto end;
		for (i = 0; i < log->data_cnt; i++) {
			len += snprintf((err_buf + len),
				(sizeof(err_buf) - len), "%x ", log->data[i]);
			if (len >= SS_XLOG_PANIC_DBG_LENGTH)
				goto end;
		}
		last--;
	}
end:
	pr_info("%s:%s\n", __func__, err_buf);

	if (vdd_primary && gpio_is_valid(vdd_primary->ub_con_det.gpio))
		LCD_ERR("ub con gpio for primary = %d\n", gpio_get_value(vdd_primary->ub_con_det.gpio));
	if (vdd_secondary && gpio_is_valid(vdd_secondary->ub_con_det.gpio))
		LCD_ERR("ub con gpio for secondary = %d\n", gpio_get_value(vdd_secondary->ub_con_det.gpio));
/*
 * #ifdef CONFIG_SEC_DEBUG
 *	sec_debug_store_additional_dbg(DBG_2_DISPLAY_ERR, 0, "%s", err_buf);
 * #endif
 */

}

static const struct file_operations xlog_dump_ops = {
	.open = ss_xlog_dump_open,
	.read = ss_xlog_dump_read,
	.release = single_release,
};

int ss_read_rddpm(struct samsung_display_driver_data *vdd)
{
	char rddpm = 0;
	int ret;

	ret = ss_panel_data_read(vdd, RX_LDI_DEBUG0, &rddpm, LEVEL_KEY_NONE);
	if (ret) {
		LCD_ERR("fail to read rddpm(ret=%d)\n", ret);
		return ret;
	}

	LCD_DEBUG("========== SHOW PANEL [0Ah:RDDPM] INFO ==========\n");
	LCD_DEBUG("* Reg Value : 0x%02x, Result : %s\n",
				rddpm, (rddpm == 0x9C) ? "GOOD" : "NG");
	LCD_DEBUG("* Bootster Mode : %s\n", rddpm & 0x80 ? "ON (GD)" : "OFF (NG)");
	LCD_DEBUG("* Idle Mode     : %s\n", rddpm & 0x40 ? "ON (NG)" : "OFF (GD)");
	LCD_DEBUG("* Partial Mode  : %s\n", rddpm & 0x20 ? "ON" : "OFF");
	LCD_DEBUG("* Sleep Mode    : %s\n", rddpm & 0x10 ? "OUT (GD)" : "IN (NG)");
	LCD_DEBUG("* Normal Mode   : %s\n", rddpm & 0x08 ? "OK (GD)" : "SLEEP (NG)");
	LCD_DEBUG("* DISPON          : %s\n", rddpm & 0x04 ? "ON (GD)" : "OFF (NG)");
	LCD_DEBUG("* REAL DISPON(2c) : %s\n", rddpm & 0x02 ? "ON (GD)" : "OFF (NG)");
	LCD_DEBUG("* POC_LOAD        : %s\n", rddpm & 0x01 ? "ON (GD)" : "OFF (NG)");
	LCD_DEBUG("=================================================\n");

	return rddpm;
}

int ss_read_errfg(struct samsung_display_driver_data *vdd)
{
	char err_fg = 0;
	int ret;

	ret = ss_panel_data_read(vdd, RX_LDI_DEBUG2, &err_fg, LEVEL2_KEY);
	if (ret) {
		LCD_ERR("fail to read errfg(ret=%d)\n", ret);
		return ret;
	}

	LCD_DEBUG("========== SHOW PANEL [EEh:ERR_FG] INFO ==========\n");
	LCD_DEBUG("* Reg Value : 0x%02x, Result : %s\n",
				err_fg, (err_fg & 0x4C) ? "NG" : "GOOD");

	if (err_fg & 0x04) {
		LCD_ERR("* VLOUT3 Error\n");
		inc_dpui_u32_field(DPUI_KEY_PNVLO3E, 1);
	}

	if (err_fg & 0x08) {
		LCD_ERR("* ELVDD Error\n");
		inc_dpui_u32_field(DPUI_KEY_PNELVDE, 1);
	}

	if (err_fg & 0x40) {
		LCD_ERR("* VLIN1 Error\n");
		inc_dpui_u32_field(DPUI_KEY_PNVLI1E, 1);
	}
	LCD_DEBUG("==================================================\n");
	return err_fg;
}

int ss_read_rddsm(struct samsung_display_driver_data *vdd)
{
	char rddsm = 0;
	int ret;

	ret = ss_panel_data_read(vdd, RX_LDI_DEBUG3, &rddsm, LEVEL_KEY_NONE);
	if (ret) {
		LCD_ERR("fail to read rddsm(ret=%d)\n", ret);
		return ret;
	}

	LCD_DEBUG("========== SHOW PANEL [0Eh:RDDSM] INFO ==========\n");
	LCD_DEBUG("* Reg Value : 0x%02x, Result : %s\n",
				rddsm, (rddsm == 0x80) ? "GOOD" : "NG");
	LCD_DEBUG("* TE Mode : %s\n", rddsm & 0x80 ? "ON(GD)" : "OFF(NG)");
	LCD_DEBUG("=================================================\n");

	return rddsm;
}


int ss_read_dsierr(struct samsung_display_driver_data *vdd)
{
	char dsi_err = 0;
	int ret;

	ret = ss_panel_data_read(vdd, RX_LDI_DEBUG4, &dsi_err, LEVEL1_KEY);
	if (ret) {
		LCD_ERR("fail to read dsierr(ret=%d)\n", ret);
		return ret;
	}

	LCD_DEBUG("========== SHOW PANEL [05h:DSIE_CNT] INFO ==========\n");
	LCD_DEBUG("* Reg Value : 0x%02x, Result : %s\n",
			dsi_err, (dsi_err) ? "NG" : "GOOD");
	if (dsi_err)
		LCD_ERR("* DSI Error Count : %d\n", dsi_err);
	LCD_DEBUG("====================================================\n");

	inc_dpui_u32_field(DPUI_KEY_PNDSIE, dsi_err);

	return dsi_err;
}

int ss_read_self_diag(struct samsung_display_driver_data *vdd)
{
	char self_diag = 0;
	int ret;

	ret = ss_panel_data_read(vdd, RX_LDI_DEBUG5, &self_diag, LEVEL_KEY_NONE);
	if (ret) {
		LCD_ERR("fail to read rddpm(ret=%d)\n", ret);
		return ret;
	}

	LCD_DEBUG("========== SHOW PANEL [0Fh:SELF_DIAG] INFO ==========\n");
	LCD_DEBUG("* Reg Value : 0x%02x, Result : %s\n",
			self_diag, (self_diag & 0x80) ? "GOOD" : "NG");
	if ((self_diag & 0x80) == 0)
		LCD_ERR("* OTP Reg Loading Error\n");
	LCD_DEBUG("=====================================================\n");

	inc_dpui_u32_field(DPUI_KEY_PNSDRE, (self_diag & 0x80) ? 0 : 1);

	return 0;
}

int ss_read_mipi_protocol_err(struct samsung_display_driver_data *vdd)
{
	u16 err_status = 0;
	u8 rbuf[2];
	int ret;

	ret = ss_panel_data_read(vdd, RX_LDI_DEBUG6, rbuf, LEVEL_KEY_NONE);
	if (ret) {
		LCD_ERR("fail to read protocol_err(ret=%d)\n", ret);
		return ret;
	}

	err_status = (rbuf[0] << 8) | rbuf[1];

	/* E9h mipi protocol error status register
	 * ERR[15] : DSI Protocol violation
	 * ERR[14] : DATA P lane contention detection
	 * ERR[13] : Invalid Transmission Length
	 * ERR[12] : DSI VC ID Invalid
	 * ERR[11] : DSI Data Type Not Recognized
	 * ERR[10] : Checksum Error
	 * ERR[9] : ECC Error, multi_bit (detected, not corrected)
	 * ERR[8] : ECC Error, single−bit (detected and corrected)
	 * ERR[7] : Data Lane contention detection
	 * ERR[6] : False Control Error
	 * ERR[5] : HS RX Timeout
	 * ERR[4] : Low−Power Transmit Sync Error
	 * ERR[3] : Escape Mode Entry Command Error
	 * ERR[2] : EoT Sync Error
	 * ERR[1] : SoT Sync Error
	 * ERR[0] : SoT Error
         */
	if (err_status) {
		LCD_ERR("MIPI protocol error: 0x%x\n", err_status);
	}

	LCD_DEBUG("========== SHOW PANEL [E9h: MIPI PROTOCOL ERROR] INFO ==========\n");
	LCD_DEBUG("* Reg Value : 0x%02x, Result : %s\n",
			err_status, (err_status & 0x00) ? "GOOD" : "NG");

	LCD_DEBUG("* DSI Protocol violation : %s\n", (err_status & BIT(15)) ? "ERROR" : "OK");
	LCD_DEBUG("* DATA P lane contention detection : %s\n", (err_status & BIT(14)) ? "ERROR" : "OK");
	LCD_DEBUG("* Invalid Transmission Length : %s\n", (err_status & BIT(13)) ? "ERROR" : "OK");
	LCD_DEBUG("* DSI VC ID Invalid : %s\n", (err_status & BIT(12)) ? "ERROR" : "OK");
	LCD_DEBUG("* DSI Data Type Not Recognized : %s\n", (err_status & BIT(11)) ? "ERROR" : "OK");
	LCD_DEBUG("* Checksum Error : %s\n", (err_status & BIT(10)) ? "ERROR" : "OK");
	LCD_DEBUG("* ECC Error, multi_bit : %s\n", (err_status & BIT(9)) ? "ERROR" : "OK");
	LCD_DEBUG("* ECC Error, single−bit : %s\n", (err_status & BIT(8)) ? "ERROR" : "OK");
	LCD_DEBUG("* Data Lane contention detection : %s\n", (err_status & BIT(7)) ? "ERROR" : "OK");
	LCD_DEBUG("* False Control Error : %s\n", (err_status & BIT(6)) ? "ERROR" : "OK");
	LCD_DEBUG("* HS RX Timeout : %s\n", (err_status & BIT(5)) ? "ERROR" : "OK");
	LCD_DEBUG("* Low−Power Transmit Sync Error : %s\n", (err_status & BIT(4)) ? "ERROR" : "OK");
	LCD_DEBUG("* Escape Mode Entry Command Error : %s\n", (err_status & BIT(3)) ? "ERROR" : "OK");
	LCD_DEBUG("* EoT Sync Error : %s\n", (err_status & BIT(2)) ? "ERROR" : "OK");
	LCD_DEBUG("* SoT Sync Error : %s\n", (err_status & BIT(1)) ? "ERROR" : "OK");
	LCD_DEBUG("* SoT Error : %s\n", (err_status & BIT(0)) ? "ERROR" : "OK");
	LCD_DEBUG("=====================================================\n");

	return err_status;
}


/* DDI CMD log buffer max size is 512 bytes.
 * But, qct display driver limit max read size to 255 bytes (0xFF)
 * due to panel dtsi. panel dtsi determines length with one byte.
 * samsung,ldi_debug_logbuf_rx_cmds_revA   = [06 01 00 00 00 00 01 9C FF 00];
 */

#define DDI_CMD_LOGBUF_SIZE	255

int ss_read_ddi_cmd_log(struct samsung_display_driver_data *vdd, char *read_buf)
{
	int ret;
	int i;

	ret = ss_panel_data_read(vdd, RX_LDI_DEBUG_LOGBUF, read_buf, LEVEL1_KEY);
	if (ret) {
		LCD_ERR("fail to read ddi cmd log buffer(ret=%d)\n", ret);
		return ret;
	}

	LCD_INFO("DDI command log:");
	for (i = 0; i < DDI_CMD_LOGBUF_SIZE; i++)
		pr_cont(" %02x", read_buf[i]);
	pr_cont("\n");

	return 0;
}

#if defined(CONFIG_SEC_FACTORY)
char bootloader_pps1_data[SZ_64]; /* 0xA2 : PPS data (0x00 ~ 0x2C) */
char bootloader_pps2_data[SZ_64]; /* 0xA2 : PPS data (0x2d ~ 0x58)*/
int ss_read_pps_data(struct samsung_display_driver_data *vdd)
{
	int ret;
#if defined(CONFIG_SEC_F2Q_PROJECT)
	LCD_ERR("temp block ss_read_pps_data\n");
	//Temporally blocked because of null pointer error.
	return 0;
#endif
	ret = ss_panel_data_read(vdd, RX_LDI_DEBUG_PPS1, bootloader_pps1_data, LEVEL1_KEY);
	if (ret) {
		LCD_ERR("fail to read pps_data(ret=%d)\n", ret);
		return ret;
	}

	ret = ss_panel_data_read(vdd, RX_LDI_DEBUG_PPS2, bootloader_pps2_data, LEVEL1_KEY);
	if (ret) {
		LCD_ERR("fail to read pps_data(ret=%d)\n", ret);
		return ret;
	}

	return 0;
}
#else
int ss_read_pps_data(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("nothing to do\n");
	return 0;
}
#endif

struct clock_index_table {
	int rat;
	int band;
	int link_MHz;
	int index;
};

/* TODO: print info. for both of primary and secondary vdd */
static ssize_t ss_read_dyn_mipi_clk_index_table(struct file *file, char __user *buff,
		size_t count, loff_t *ppos)
{
	static char clk_table_buff[1024];
	static int num_of_initialized;
	int buff_offset = 0;
	int buff_size = 1024;
	struct samsung_display_driver_data *vdd;
	struct clk_timing_table *clk_timing_table = NULL;
	struct clk_sel_table *clk_sel_table = NULL;
	struct clock_index_table *clk_index_table = NULL;
	int i = 0;
	int index_table_size = 0;
	int clk_timing_table_size = 0;

	vdd = (struct samsung_display_driver_data *)file->private_data;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid vdd\n");
		return -EINVAL;
	}

	/* Initialize index table size and clock index table */
	clk_timing_table = &vdd->dyn_mipi_clk.clk_timing_table;
	if (IS_ERR_OR_NULL(clk_timing_table)) {
		LCD_ERR("Invalid timing table\n");
		return -EINVAL;
	}

	index_table_size = clk_timing_table->tab_size;
	clk_index_table = kzalloc(sizeof(struct clock_index_table)
				* index_table_size, GFP_KERNEL);

	if (num_of_initialized >= index_table_size)
		goto end;

	for (i = 0; i < index_table_size; i++) {
		/* Initialize the index in  increasing order */
		clk_index_table[i].index = i;
	}

	/*
	 * Initialize index 0 to abnormal value
	 * because the index 0 is used when if abnormal values
	 * are written into "/sys/class/lcd/panel/rf_info"
	 */
	clk_index_table[0].rat = -1;
	clk_index_table[0].band = -1;
	clk_index_table[0].link_MHz = -1;
	clk_index_table[0].index = 0;
	num_of_initialized++;

	if (IS_ERR_OR_NULL(clk_index_table)) {
		LCD_ERR("Fail to allocate clk_index_table\n");
		return -ENOMEM;
	}

	/* Initialize clock select table size */
	clk_sel_table = &vdd->dyn_mipi_clk.clk_sel_table;
	if (IS_ERR_OR_NULL(clk_timing_table)) {
		LCD_ERR("Invalid timing table\n");
		return -EINVAL;
	}

	clk_timing_table_size = clk_sel_table->tab_size;

	/* Check clcok select table and initialize index table */
	for (i = 0; i < clk_timing_table_size; i++) {
		int index = 0;

		if (num_of_initialized >= index_table_size)
			break;

		if (unlikely(IS_ERR_OR_NULL(clk_sel_table->rat) &&
				IS_ERR_OR_NULL(clk_sel_table->band) &&
				IS_ERR_OR_NULL(clk_sel_table->from) &&
				IS_ERR_OR_NULL(clk_sel_table->end) &&
				IS_ERR_OR_NULL(clk_sel_table->target_clk_idx))
		   )
			break;

		index = clk_sel_table->target_clk_idx[i];
		if (unlikely(index > clk_timing_table_size)) {
			LCD_ERR("Invalid index: %d\n", index);
			continue;
		}

		/* */
		if (!clk_index_table[index].rat &&
				(clk_index_table[index].index == index)) {
			clk_index_table[index].rat = clk_sel_table->rat[i];
			clk_index_table[index].band = clk_sel_table->band[i];
			clk_index_table[index].link_MHz =
				(clk_sel_table->from[i] + clk_sel_table->end[i]) / 2;
			clk_index_table[index].index = clk_sel_table->target_clk_idx[i];
			num_of_initialized++;
		}
	}

end:
	/* Add index table Length */
	if (!strnlen(clk_table_buff, buff_size)) {
		snprintf(clk_table_buff + buff_offset, buff_size - buff_offset,
				"Index Table Length: %d\n", index_table_size);
		buff_offset = strnlen(clk_table_buff, buff_size);

		for (i = 0; i < index_table_size; i++) {
			if (clk_index_table[i].rat) {
				snprintf(clk_table_buff + buff_offset, buff_size - buff_offset,
						"RAT: %d, BAND: %d, link_MHz: %d, INDEX: %d\n",
						clk_index_table[i].rat, clk_index_table[i].band,
						clk_index_table[i].link_MHz, clk_index_table[i].index
						);
				buff_offset = strnlen(clk_table_buff, buff_size);
			}
		}
	}

	kfree(clk_index_table);

	return simple_read_from_buffer(buff, count, ppos, clk_table_buff, strlen(clk_table_buff));
}


static const struct file_operations dyn_mipi_clk_ops = {
	.open = simple_open,
	.read = ss_read_dyn_mipi_clk_index_table,
};

static void ss_panel_debug_create(struct samsung_display_driver_data *vdd)
{
	struct samsung_display_debug_data *debug_data;
	struct samsung_display_driver_data *vdd_secondary =
					ss_get_vdd(SECONDARY_DISPLAY_NDX);

	debug_data = vdd->debug_data;

	/* Create file on debugfs of display_driver */


	/* Create file on debugfs on dump */
	debugfs_create_file("xlog_dump", 0644, debug_data->dump,
		vdd, &xlog_dump_ops);
	debugfs_create_bool("print_cmds", 0600, debug_data->dump,
		&debug_data->print_cmds);
	debugfs_create_bool("panic_on_pptimeout", 0600, debug_data->dump,
		&debug_data->panic_on_pptimeout);

	/* Create file on debugfs on display_status */
	debugfs_create_u32("panel_attach_status", 0600,
		debug_data->display_status, (u32 *)&vdd->panel_attach_status);

	debugfs_create_u32("panel1_attach_status", 0600,
		debug_data->display_status, (u32 *)&vdd_secondary->panel_attach_status);

	if (!IS_ERR_OR_NULL(debug_data->is_factory_mode))
		debugfs_create_bool("is_factory_mode", 0600, debug_data->root,
		debug_data->is_factory_mode);

	debugfs_create_bool("enable_pr_debug", 0600, debug_data->root,
			&enable_pr_debug);

	/* Create debugfs file on display LTP */
	debugfs_create_file("dyn_mipi_clk_index_table", 0644, debug_data->display_ltp,
		vdd, &dyn_mipi_clk_ops);

	/* Create file on debugfs on hw_info */
	/* TBD */
}

static bool ss_read_debug_partition(struct lcd_debug_t *value)
{
	return read_debug_partition(debug_index_lcd_debug_info, (void *)value);
}

static bool ss_write_debug_partition(struct lcd_debug_t *value)
{
	return write_debug_partition(debug_index_lcd_debug_info, (void *)value);
}

void ss_inc_ftout_debug(const char *name)
{
	struct lcd_debug_t lcd_debug;

	memset(&lcd_debug, 0, sizeof(struct lcd_debug_t));
	ss_read_debug_partition(&lcd_debug);
	lcd_debug.ftout.count += 1;
	strncpy(lcd_debug.ftout.name, name, MAX_FTOUT_NAME);
	ss_write_debug_partition(&lcd_debug);
}

static int dpci_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	ssize_t len = 0;
	char tbuf[SS_XLOG_DPCI_LENGTH] = {0,};
	struct lcd_debug_t lcd_debug;

	/* 1. Read */
	ss_read_debug_partition(&lcd_debug);
	LCD_INFO("Read Result FTOUT_CNT=%d, FTOUT_NAME=%s\n", lcd_debug.ftout.count, lcd_debug.ftout.name);

	/* 2. Make String */
	if (lcd_debug.ftout.count) {
		len += snprintf((tbuf + len), (SS_XLOG_DPCI_LENGTH - len),
			"FTOUT CNT=%d ", lcd_debug.ftout.count);
		lcd_debug.ftout.name[sizeof(lcd_debug.ftout.name) - 1] = '\0';
		len += snprintf((tbuf + len), (SS_XLOG_DPCI_LENGTH - len),
			"NAME=%s ", lcd_debug.ftout.name);
	}

	/* 3. Info Clear */
	memset((void *)&lcd_debug, 0, sizeof(struct lcd_debug_t));
	ss_write_debug_partition(&lcd_debug);
	set_dpui_field(DPUI_KEY_QCT_SSLOG, tbuf, len);

	return 0;
}


static int ss_register_dpci(struct samsung_display_driver_data *vdd)
{
	int ret;
	memset(&vdd->dpci_notif, 0,
			sizeof(vdd->dpci_notif));
	vdd->dpci_notif.notifier_call = dpci_notifier_callback;

	ret = dpui_logging_register(&vdd->dpci_notif, DPUI_TYPE_CTRL);
	return ret;
}

int ss_panel_debug_init(struct samsung_display_driver_data *vdd)
{
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd))
		return -ENODEV;

	if (vdd->ndx != COMMON_DISPLAY_NDX) {
		struct samsung_display_driver_data *vdd_common = ss_get_vdd(COMMON_DISPLAY_NDX);
		vdd->debug_data = vdd_common->debug_data;
		LCD_INFO("vdd->ndx = %d Skip.. creat debugfs for only primary vdd & copy those from common\n",
			vdd->ndx);
		return 0;
	}

	/*
	 * The debugfs must be init one time
	 * in case of dual dsi, this function will be called twice
	 */
	if (vdd->debug_data) {
		LCD_ERR("try to initialize debug_data again...\n");
		return 0;
	}

	vdd->debug_data = kzalloc(sizeof(struct samsung_display_debug_data),
		GFP_KERNEL);
	if (IS_ERR_OR_NULL(vdd->debug_data)) {
		LCD_ERR("no memory to create display debug data\n");
		return -ENOMEM;
	}

	/* INIT debug data */
	vdd->debug_data->is_factory_mode = &vdd->is_factory_mode;

	/*
	 * panic_on_pptimeout default value is false
	 * if you want to enable panic for specific project
	 * please change the value on your panel file.
	 * if you want to enable panic for all project
	 * please change the value here.
	 */
	vdd->debug_data->panic_on_pptimeout = false;

	/* Root directory for display driver */
	vdd->debug_data->root = debugfs_create_dir("display_driver", NULL);
	if (IS_ERR_OR_NULL(vdd->debug_data->root)) {
		LCD_ERR("debugfs_create_dir failed, error %ld(line:%d)\n",
		       PTR_ERR(vdd->debug_data->root), __LINE__);
		ret = -ENODEV;
		goto fail_alloc;
	}

	/* Directory for dump */
	vdd->debug_data->dump = debugfs_create_dir("dump", vdd->debug_data->root);
	if (IS_ERR_OR_NULL(vdd->debug_data->dump)) {
		LCD_ERR("debugfs_create_dir failed, error %ld(line:%d)\n",
		       PTR_ERR(vdd->debug_data->dump), __LINE__);
		ret = -ENODEV;
		goto fail;
	}

	/* Directory for hw_info */
	vdd->debug_data->hw_info = debugfs_create_dir("hw_info", vdd->debug_data->root);
	if (IS_ERR_OR_NULL(vdd->debug_data->root)) {
		LCD_ERR("debugfs_create_dir failed, error %ld(line:%d)\n",
		       PTR_ERR(vdd->debug_data->root), __LINE__);
		ret = -ENODEV;
		goto fail;
	}

	/* Directory for display_status */
	vdd->debug_data->display_status = debugfs_create_dir("display_status",
					vdd->debug_data->root);
	if (IS_ERR_OR_NULL(vdd->debug_data->display_status)) {
		LCD_ERR("debugfs_create_dir failed, error %ld(line:%d)\n",
		       PTR_ERR(vdd->debug_data->root), __LINE__);
		ret = -ENODEV;
		goto fail;
	}

	/* Directory for display LTP */
	vdd->debug_data->display_ltp = debugfs_create_dir("display_ltp",
					vdd->debug_data->root);
	if (IS_ERR_OR_NULL(vdd->debug_data->display_status)) {
		LCD_ERR("debugfs_create_dir failed, error %ld(line:%d)\n",
		       PTR_ERR(vdd->debug_data->root), __LINE__);
		ret = -ENODEV;
		goto fail;
	}

	ss_panel_debug_create(vdd);

	ss_register_dpci(vdd);

	return 0;

fail:
	debugfs_remove_recursive(vdd->debug_data->root);

fail_alloc:
	kfree(vdd->debug_data);
	LCD_ERR("Fail to create files for debugfs(ret=%d)\n", ret);

	return ret;
}


/**
 *	sde & sde-rotator smmu debug.
 *	Because of preemption & performance issue,
 *	sde & sde-rotator use unique spin_lock.
 */
int ss_smmu_debug_init(struct samsung_display_driver_data *vdd)
{
	int cnt;
	int ret = 0;

	/* This debug is available by sde_debug enabled condition */
	if (!sec_debug_is_enabled()) {
		LCD_ERR("sec_debug_is_enabled : %d\n", sec_debug_is_enabled());
		goto init_fail;
	}

	/* Create KMEM_CACHE slab */
	if (IS_ERR_OR_NULL(vdd->ss_debug_smmu_cache)) {
		vdd->ss_debug_smmu_cache = KMEM_CACHE(ss_smmu_logging, 0);

		if (IS_ERR_OR_NULL(vdd->ss_debug_smmu_cache)) {
			LCD_ERR("ss_debug_smmu_cache is not created\n");
			goto init_fail;
		}
	}

	for (cnt = 0; cnt < SMMU_MAX_DEBUG; cnt++) {
		if (!vdd->ss_debug_smmu[cnt].init_done) {
			spin_lock_init(&vdd->ss_debug_smmu[cnt].lock);
			INIT_LIST_HEAD(&vdd->ss_debug_smmu[cnt].list);
			vdd->ss_debug_smmu[cnt].init_done = true;
		}
	}

	return ret;

init_fail:
	for (cnt = 0; cnt < SMMU_MAX_DEBUG; cnt++)
		vdd->ss_debug_smmu[cnt].init_done = false;

	return -EPERM;
}

void ss_smmu_debug_map(enum ss_smmu_type type, struct sg_table *table)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(COMMON_DISPLAY_NDX);

	spinlock_t *smmu_lock = NULL;
	struct list_head *smmu_list = NULL;
	struct ss_smmu_logging *smmu_debug = NULL;

	if (IS_ERR_OR_NULL(vdd))
		return;

	if (type >= SMMU_MAX_DEBUG || !vdd->ss_debug_smmu[type].init_done) {
		LCD_ERR("type : %d init_done : %d\n", type,
				(type < SMMU_MAX_DEBUG) ?
				vdd->ss_debug_smmu[type].init_done : -1);
		return;
	}

	if (IS_ERR_OR_NULL(vdd->ss_debug_smmu_cache)) {
		LCD_ERR("ss_debug_smmu_cache is not created\n");
		return;
	}

	smmu_lock = &vdd->ss_debug_smmu[type].lock;
	smmu_list = &vdd->ss_debug_smmu[type].list;

	smmu_debug = kmem_cache_alloc(vdd->ss_debug_smmu_cache, GFP_KERNEL | __GFP_ZERO);

	spin_lock(smmu_lock);

	if (!IS_ERR_OR_NULL(smmu_debug)) {
		smmu_debug->time = ktime_get();
		smmu_debug->table = table;
		INIT_LIST_HEAD(&smmu_debug->list);
		list_add(&smmu_debug->list, smmu_list);

		LCD_DEBUG("addr : 0x%llx size : 0x%x \n", table->sgl->dma_address, table->sgl->dma_length);
	}

	spin_unlock(smmu_lock);
}

void ss_smmu_debug_unmap(enum ss_smmu_type type, struct sg_table *table)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(COMMON_DISPLAY_NDX);

	spinlock_t *smmu_lock = NULL;
	struct list_head *smmu_list = NULL;
	struct ss_smmu_logging *smmu_debug = NULL;

	if (IS_ERR_OR_NULL(vdd))
		return;

	if (type >= SMMU_MAX_DEBUG || !vdd->ss_debug_smmu[type].init_done) {
		LCD_ERR("type : %d init_done : %d\n", type,
				(type < SMMU_MAX_DEBUG) ?
				vdd->ss_debug_smmu[type].init_done : -1);

		return;
	}

	smmu_lock = &vdd->ss_debug_smmu[type].lock;
	smmu_list = &vdd->ss_debug_smmu[type].list;

	spin_lock(smmu_lock);

	list_for_each_entry(smmu_debug, smmu_list, list) {
		if (smmu_debug->table == table) {
			LCD_DEBUG("addr : 0x%llx size : 0x%x \n", table->sgl->dma_address, table->sgl->dma_length);
			list_del(&smmu_debug->list);
			kmem_cache_free(vdd->ss_debug_smmu_cache, smmu_debug);
			break;
		}
	}

	spin_unlock(smmu_lock);
}

#if defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
void ss_smmu_debug_log(void)
{
	LCD_DEBUG("nothing to do\n");
}
#else
void ss_smmu_debug_log(void)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(COMMON_DISPLAY_NDX);

	enum ss_smmu_type type = SMMU_MAX_DEBUG;
	spinlock_t *smmu_lock = NULL;
	struct list_head *smmu_list = NULL;
	struct ss_smmu_logging *smmu_debug = NULL;

	if (IS_ERR_OR_NULL(vdd))
		return;

	for (type = SMMU_RT_DISPLAY_DEBUG; type < SMMU_MAX_DEBUG; type++) {

		if (vdd->ss_debug_smmu[type].init_done != true)
			continue;

		smmu_lock = &vdd->ss_debug_smmu[type].lock;
		smmu_list = &vdd->ss_debug_smmu[type].list;

		spin_lock(smmu_lock);

		list_for_each_entry(smmu_debug, smmu_list, list) {
#if defined(CONFIG_NEED_SG_DMA_LENGTH)
			LCD_INFO("type : %s time : %lld.%6lld dma_address : 0x%llx dma_length : %d\n",
				type == SMMU_RT_DISPLAY_DEBUG ? "SMMU_RT_DISPLAY_DEBUG" : "SMMU_NRT_ROTATOR_DEBUG",
				smmu_debug->time / NSEC_PER_SEC, smmu_debug->time - ((smmu_debug->time / NSEC_PER_SEC) * NSEC_PER_SEC),
				smmu_debug->table->sgl->dma_address,
				smmu_debug->table->sgl->dma_length);
#else
			LCD_INFO("type : %s time : %d.%6d dma_address : 0x%llx\n",
				type == SMMU_RT_DISPLAY_DEBUG ? "SMMU_RT_DISPLAY_DEBUG" : "SMMU_NRT_ROTATOR_DEBUG",
				smmu_debug->time / NSEC_PER_SEC, smmu_debug->time - ((smmu_debug->time / NSEC_PER_SEC) * NSEC_PER_SEC),
				smmu_debug->table->sgl->dma_address);
#endif
		}

		spin_unlock(smmu_lock);
	}
}
#endif

static DEFINE_SPINLOCK(image_logging_lock);
#define MAX_IMAGE_LOGGING 256
struct ss_image_logging image_logging[MAX_IMAGE_LOGGING];
static int image_logging_index = 0;

void ss_image_logging_update(uint32_t plane_addr, int width, int height, int src_format)
{
	if (!sec_debug_is_enabled()) {
		return;
	}

	spin_lock(&image_logging_lock);

	image_logging[image_logging_index].dma_address = plane_addr;
	image_logging[image_logging_index].src_width = width;
	image_logging[image_logging_index].src_height = height;
	image_logging[image_logging_index].src_format = src_format;

	image_logging_index++;
	image_logging_index %= MAX_IMAGE_LOGGING;

	spin_unlock(&image_logging_lock);
}

void ss_xlog_vrr_change_in_drm_ioctl(int vrefresh, int sot_hs_mode)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(PRIMARY_DISPLAY_NDX);

	if (vdd->vrr.adjusted_refresh_rate != 0 &&
			(vdd->vrr.adjusted_refresh_rate != vrefresh ||
			 vdd->vrr.adjusted_sot_hs_mode != sot_hs_mode)) {
		LCD_INFO("switch mode: drm_ioctl: %d%s -> %d%s\n",
				vdd->vrr.adjusted_refresh_rate,
				vdd->vrr.adjusted_sot_hs_mode ? "HS" : "NM",
				vrefresh,
				sot_hs_mode ? "HS" : "NM");
		SS_XLOG(vdd->vrr.adjusted_refresh_rate,
				vdd->vrr.adjusted_sot_hs_mode,
				vrefresh, sot_hs_mode);
	}
}
