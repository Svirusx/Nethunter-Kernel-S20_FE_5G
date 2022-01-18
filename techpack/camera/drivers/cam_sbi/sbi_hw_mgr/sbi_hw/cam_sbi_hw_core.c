/* Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
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

#include "cam_sbi_hw_core.h"
#include "cam_sbi_hw_soc.h"
#include "cam_smmu_api.h"
#include "cam_irq_controller.h"

#define CONV_STR(name) #name

#define REG_READ(reg)            cam_io_r_mb(sbi_hw->soc_info.reg_map[0].mem_base + (reg))
#define REG_WRITE(reg, val)      cam_io_w_mb(val, sbi_hw->soc_info.reg_map[0].mem_base + (reg))
#define SBI_REG_READ(reg)        REG_READ((reg) + CAM_SBI_REG_OFFSET)
#define SBI_REG_WRITE(reg, val)  REG_WRITE((reg) + CAM_SBI_REG_OFFSET, val)

#define SENSOR60(id)             ( ((id) == 0) || ((id) == 1) )
#define SENSOR960(id)            (!SENSOR60(id))
#define RECORD60(id)             ( ((id) == 0) || ((id) == 1) || ((id) == 2) || ((id) == 3) )
#define RECORD960(id)            (!RECORD60(id))


#define INC_TNUM(num, step, max) { num = ((num) + (step)) % (max); }
#define DEC_TNUM(num, step, max) { num = ((num) + (max) - (step)) % (max); }

// Timestamp buffer size for one batch(16 frsmes)
#define SBI_TS_BUF_SIZE 0x100000

typedef enum _enum_sbi_reset_cmd {
	TASK1_RST = 0,
	TASK2_RST = 1,
	TASK3_RST = 2,
	CORE_RST = 3,
} enum_sbi_reset_cmd;

typedef enum _enum_sbi_irq_status {
	TASK1_RUP_DONE = 0,
	TASK2_RUP_DONE = 1,
	TASK3_RUP_DONE = 2,
	TASK1_FRAME_DONE = 3,
	TASK2_FRAME_DONE = 4,
	TASK3_FRAME_DONE = 5,
	TASK1_FRAME_DROP = 6,
	TASK2_FRAME_DROP = 7,
	TASK3_FRAME_DROP = 8,
	TASK1_RST_DONE = 9,
	TASK2_RST_DONE = 10,
	TASK3_RST_DONE = 11,
	CORE_RST_DONE = 12,
	TASK1_FRAME_PAD = 13,
	TASK2_FRAME_PAD = 14,
	TASK3_FRAME_PAD = 15,
	TASK1_CCIF_VIOLATION = 16,
	TASK2_CCIF_VIOLATION = 17,
	TASK3_CCIF_VIOLATION = 18,
} enum_sbi_irq_status;

typedef enum _rup_ready {
	TASK1_RUP_RDY = 4,
	TASK2_RUP_RDY = 8,
	TASK3_RUP_RDY = 12,
} rup_ready;

/*
 * Generate debug string with regitser array
 */
static char * make_regs_string(struct cam_hw_info *sbi_hw, int regs[], int n_regs)
{
	static char buf[1024];
	char unit[80];
	int i;

	memset(buf, 0, sizeof(buf));
	for (i = 0; i < n_regs; i++) {
		int val = SBI_REG_READ(regs[i]);
		snprintf(unit, sizeof(unit), " [%03x:0x%x]", regs[i], val);
		strcat(buf, unit);

		// FRAME_PAD : frame error status
		if (regs[i] == 0x174) {
			int i;
			const char * err_status[] = {
					"ID1_DRAM_BACKPRESSURE",
					"ID2_AF_BUF_OVERFLOW",
					"ID3_T1_IFE0_AF_RUPRDY_NOT_ARRIVED",
					"ID3_T2_IFE0_BAYER_RUPRDY_NOT_ARRIVED",
					"ID3_T3_IFE1_BAYER_RUPRDY_NOT_ARRIVED",
					"ID7_Q0_IFE0_AF_PORT_TIMEOUT",
					"ID7_Q1_IFE0_BAYER_PORT_TIMEOUT",
					"ID7_Q2_IFE1_BAYER_PORT_TIMEOUT",
					"RD2_ERR_DATA_CORRUPTED",
					"SDC2_TIMEOUT",
					"SDC0_ERROR",
					"SDC1_ERROR",
					"SDC2_ERROR" };
			for (i = 0; i < 13; i++ ) {
				int bit_mask = 1 << i;
				if (val & bit_mask) {
					snprintf(unit, sizeof(unit), ", %s", err_status[i]);
					strcat(buf, unit);
				}
			}
		}
	}

	return buf;
}

void cam_sbi_hw_configure_init(
	struct cam_hw_info *sbi_hw, void *args, uint32_t args_size)
{
	struct cam_sbi_core *core = (struct cam_sbi_core *)sbi_hw->core_info;
	int i = 0;
	void *arg_ptr = args;

	core->t2_stat_check = true;
	core->t2_act_val = 0;
	core->prev_t2_act_val = 0;

	CAM_DBG(CAM_SBI, "%s e, register set size %d",
		__FUNCTION__, args_size);

	for (i = 0; i < args_size; i++) {
		uint32_t reg_addr = *((uint32_t *)(arg_ptr + 0));
		uint32_t reg_val  = *((uint32_t *)(arg_ptr + sizeof(uint32_t)));
		arg_ptr += (2 * sizeof(uint32_t));

		SBI_REG_WRITE(reg_addr, reg_val);
	}

	arg_ptr = args;

	for (i = 0; i < args_size; i++) {
		uint32_t reg_addr = *((uint32_t *)(arg_ptr + 0));
		uint32_t reg_val = 0;
		arg_ptr += (2 * sizeof(uint32_t));

		reg_val = cam_io_r_mb(sbi_hw->soc_info.reg_map[0].mem_base +
				reg_addr + CAM_SBI_REG_OFFSET);
		CAM_DBG(CAM_SBI, "[%d/%d] %08x, %08x", args_size, i, reg_addr, reg_val);
	}
	CAM_INFO(CAM_SBI, "Registers initialized. # of registers = %d",
		args_size);

	CAM_DBG(CAM_SBI, "%s x", __FUNCTION__);
	return;
}


// configure SBI mem base addr
void cam_sbi_hw_configure_mem_base_addr(struct cam_hw_info *sbi_hw,
						uint64_t base_addr)
{
	struct cam_sbi_core *         core = (struct cam_sbi_core *)sbi_hw->core_info;
	struct cam_sbi_dma_reg_addr * dma  = &core->hw_info->dma_reg;
	cam_sbi_ssm_info *            ssm  = &core->ssm_info;

	ssm->is_ssm_mode          = false;
	ssm->total_written_frames = 0;
	ssm->total_read_frames    = 0;
	ssm->frame_drop_count     = 0;
	ssm->is_activated         = false;
	ssm->enter_960_60_transition = false;
	ssm->cue_option = CAM_SBI_CUE_OPTION_MANUAL;
	ssm->write_done_info = false;
	ssm->make_960_info = false;
	ssm->make_60_info = false;
	ssm->meet_last_frame_info = false;
	ssm->task3_enable_info = false;

	CAM_DBG(CAM_SBI, "baseaddr = %llx", base_addr);

	SBI_REG_WRITE(dma->dma_wdma0_baseaddr, base_addr + SBI_TS_BUF_SIZE);
	SBI_REG_WRITE(dma->dma_wdma2_baseaddr, base_addr);

	// Update RUP_READY
	SBI_REG_WRITE(0, 0x1110);
}

static void cam_sbi_hw_reg_manual_update(struct cam_hw_info *sbi_hw)
{
	SBI_REG_WRITE(0x4, 1);
	SBI_REG_WRITE(0x4, 0);
}

/*
 * TASK1 writing target buffer control
 *
 * stby  true  : write to record buffer  : real 960 recording time
 *       false : write to preview buffer
 *               Sensor is 960 but doesn't write to recording buffer
 *               It write to preview buffer and handle like 60 preview
 */
static void cam_sbi_hw_task1_switch(struct cam_hw_info *sbi_hw, bool stby)
{
	SBI_REG_WRITE(0xec, (stby) ? 0 : 1);
}

static int cam_sbi_hw_get_wdma0_q_num(struct cam_hw_info *sbi_hw)
{
	return (SBI_REG_READ(0x62c) & 0xF0) >> 4;
}

static int cam_sbi_hw_get_rdma1_q_num(struct cam_hw_info *sbi_hw)
{
	return (SBI_REG_READ(0x76c) & 0xF0) >> 4;
}

/*
 * TASK3's interrupts enable / disable
 */
static void cam_sbi_hw_task3_irq_enable(struct cam_hw_info *sbi_hw, bool flag)
{
	struct cam_sbi_core * sbi_core = sbi_hw->core_info;
	struct cam_sbi_hw_info * hw_info = sbi_core->hw_info;

	int mask = BIT(TASK3_RUP_DONE) | BIT(TASK3_FRAME_DONE) |
			   BIT(TASK3_FRAME_DROP) | BIT(TASK3_FRAME_PAD);
	int reg = hw_info->irq_reg_info->irq_reg_set->mask_reg_offset;
	int val = (REG_READ(reg) & ~mask) | (flag ? mask : 0);
	REG_WRITE(reg, val);
}

static void cam_sbi_hw_task3_enable(struct cam_hw_info *sbi_hw, bool flag)
{
	struct cam_sbi_core * core = (struct cam_sbi_core *)sbi_hw->core_info;
	cam_sbi_ssm_info *    ssm  = &core->ssm_info;

	/* TASK3 enable / disable */
	int reg = 0x14;
	int val = (SBI_REG_READ(reg) & 0xFFFFFFFE) | (flag ? 1 : 0);
	SBI_REG_WRITE(reg, val);
	cam_sbi_hw_task3_irq_enable(sbi_hw, flag);
	ssm->task3_enable_info = true;
}

static void cam_sbi_hw_t1_frame_done_make_960(struct cam_hw_info *sbi_hw, bool is_960)
{
	struct cam_sbi_core * core = (struct cam_sbi_core *)sbi_hw->core_info;
	cam_sbi_ssm_info *    ssm  = &core->ssm_info;

	if (is_960)
	{
		SBI_REG_WRITE(0x110, 0x03327632); // B setting
		ssm->make_960_info = true;
	}
	else
	{
		SBI_REG_WRITE(0x110, 0x01327632); // A setting
		ssm->make_60_info = true;
	}
	cam_sbi_hw_reg_manual_update(sbi_hw);
}



static void cam_sbi_hw_update_wdma_address(struct cam_hw_info *sbi_hw)
{
	struct cam_sbi_core *         core = (struct cam_sbi_core *)sbi_hw->core_info;
	struct cam_sbi_dma_reg_addr * dma  = &core->hw_info->dma_reg;
	cam_sbi_ssm_info *            ssm  = &core->ssm_info;
	uint64_t record = 0, record_timestamp = 0;

	while (cam_sbi_hw_get_wdma0_q_num(sbi_hw) < CAM_SBI_ADDR_FIFO_Q_NUM) {
		int regs[] = { 0x62c };

		/* update recording address */
		record = ssm->record_start + ssm->batch_size * ssm->t1_num;
		record_timestamp = ssm->record_timestamp_start + (256 * ssm->batch_num) * ssm->t1_num;

		CAM_DBG(CAM_SBI, "@@t1 wdma0 addr_lr   (0x%llx) addr_hr   (0x%llx) %s t1_num=%d, t3_num=%d",
				ssm->preview, record,
				make_regs_string(sbi_hw, regs, sizeof(regs)/sizeof(int)),
				ssm->t1_num, ssm->t3_num);
		//CAM_DBG(CAM_SBI, "@@t1 wdma2 addr_ts_lr(0x%llx) addr_ts_hr(0x%llx)",
		//		ssm->preview_timestamp, record_timestamp);

		/* update registers */
		SBI_REG_WRITE(dma->dma_wdma0_baseaddr,    ssm->preview);
		SBI_REG_WRITE(dma->dma_wdma0_baseaddr_hr, record);
		SBI_REG_WRITE(dma->dma_wdma2_baseaddr,    ssm->preview_timestamp);
		SBI_REG_WRITE(dma->dma_wdma2_baseaddr_hr, record_timestamp);

		if (ssm->total_written_frames < ssm->max_frames) {
			INC_TNUM(ssm->t1_num, 1, ssm->max_num);
		}
	}
}

static void cam_sbi_hw_update_rdma_address(struct cam_hw_info *sbi_hw)
{
	struct cam_sbi_core *         core = (struct cam_sbi_core *)sbi_hw->core_info;
	struct cam_sbi_dma_reg_addr * dma  = &core->hw_info->dma_reg;
	cam_sbi_ssm_info *            ssm  = &core->ssm_info;
	uint64_t record = 0, record_timestamp = 0;
	int max_frames;

	while (cam_sbi_hw_get_rdma1_q_num(sbi_hw) < CAM_SBI_ADDR_FIFO_Q_NUM) {
		int regs[] = { 0x76c };

		record = ssm->record_start + ssm->batch_size * ssm->t3_num;
		record_timestamp = ssm->record_timestamp_start + (256 * ssm->batch_num) * ssm->t3_num;

		CAM_DBG(CAM_SBI, "@@t3 rdma1 addr_lr   (0x%llx) addr_hr   (0x%llx) %s ",
				ssm->preview, record,
				make_regs_string(sbi_hw, regs, sizeof(regs)/sizeof(int)));
		//CAM_DBG(CAM_SBI, "@@t3 rdma2 addr_ts_lr(0x%llx) addr_ts_hr(0x%llx)",
		//		ssm->preview_timestamp, record_timestamp);

		SBI_REG_WRITE(dma->dma_rdma1_baseaddr,    ssm->preview);
		SBI_REG_WRITE(dma->dma_rdma1_baseaddr_hr, record);
		SBI_REG_WRITE(dma->dma_rdma2_baseaddr,    ssm->preview_timestamp);
		SBI_REG_WRITE(dma->dma_rdma2_baseaddr_hr, record_timestamp);

		if (ssm->error_status)
			max_frames = ssm->total_written_frames - (CAM_SBI_ADDR_FIFO_Q_NUM * ssm->batch_num);
		else
			max_frames = ssm->max_frames - (CAM_SBI_ADDR_FIFO_Q_NUM * ssm->batch_num);

		if (ssm->total_read_frames < max_frames) {
			if (ssm->cue_option == CAM_SBI_CUE_OPTION_AUTO &&
				ssm->t3_num_before_cue < ssm->before_cue_num)
				ssm->t3_num_before_cue++;
			else
				INC_TNUM(ssm->t3_num, 1, ssm->max_num);
		}
		else {
			if (!ssm->rdma_meet_last_frame) {
				ssm->rdma_meet_last_frame = true;
				ssm->meet_last_frame_info = true;
			}
		}
	}
}

static void cam_sbi_hw_reset_wdma_address_queue(struct cam_hw_info *sbi_hw)
{
	SBI_REG_WRITE(0x62c,0x88); SBI_REG_WRITE(0x62c,0); // WDMA0
	SBI_REG_WRITE(0x6ac,0x88); SBI_REG_WRITE(0x6ac,0); // WDMA2
}

static void cam_sbi_hw_reset_rdma_address_queue(struct cam_hw_info *sbi_hw)
{
	SBI_REG_WRITE(0x76c,0x88); SBI_REG_WRITE(0x76c,0); // RDMA1
	SBI_REG_WRITE(0x7ac,0x88); SBI_REG_WRITE(0x7ac,0); // RDMA2
}

static void cam_sbi_hw_init_dma_address(struct cam_hw_info *sbi_hw)
{
	struct cam_sbi_core * core = (struct cam_sbi_core *)sbi_hw->core_info;
	cam_sbi_ssm_info *    ssm  = &core->ssm_info;

	//CAM_INFO(CAM_SBI, "SSM recording init dma addresse");

	/* cleare task index variables */
	ssm->t1_num = 0;
	ssm->t3_num = 0;
	ssm->t3_num_before_cue = 0;
	/* init variables */
	ssm->rec_write_enable = true;
	ssm->rec_read_enable = true;
	ssm->rdma_meet_last_frame = false;
	ssm->total_written_frames = 0;
	ssm->total_read_frames = 0;
	ssm->frame_drop_count = 0;
	ssm->before_cue_num = (ssm->ssm_framerate == 960) ? 7 : 5;

	/* reset address queue */
	CAM_DBG(CAM_SBI, "Reset WDMA0/WDMA2 and RDMA1/RDMA2 address queues");
	cam_sbi_hw_reset_wdma_address_queue(sbi_hw);
	cam_sbi_hw_reset_rdma_address_queue(sbi_hw);

	/* prepare DMA addresses */
	cam_sbi_hw_update_wdma_address(sbi_hw);
	cam_sbi_hw_update_rdma_address(sbi_hw);
}


static void cam_sbi_hw_task1_switch_check(struct cam_hw_info *sbi_hw,
		uint32_t irq_status)
{
	struct cam_sbi_core * core = (struct cam_sbi_core *)sbi_hw->core_info;
	cam_sbi_ssm_info *    ssm  = &core->ssm_info;

	if (ssm->rec_write_enable) {
		/* buffer full check */
		if (ssm->total_written_frames >= ssm->max_frames) {
			ssm->rec_write_enable = false;
			ssm->write_done_info = true;
		}
	}
}

static void cam_sbi_hw_rup_ready(struct cam_hw_info *sbi_hw, uint32_t mask)
{
	int val = SBI_REG_READ(0) | mask;
	SBI_REG_WRITE(0, val);
}

// configure SBI mem base addr
void cam_sbi_hw_configure_mem_base_addr_for_ssm(struct cam_hw_info *sbi_hw)
{
	struct cam_sbi_core *         core = (struct cam_sbi_core *)sbi_hw->core_info;
	struct cam_sbi_dma_reg_addr * dma  = &core->hw_info->dma_reg;
	cam_sbi_ssm_info *            ssm  = &core->ssm_info;

	core->t2_stat_check = false;
	core->t2_act_val = 0;
	core->prev_t2_act_val = 0;

	ssm->is_ssm_mode            = true;
	ssm->preview                = ssm->addr_lr + SBI_TS_BUF_SIZE;
	ssm->preview_timestamp      = ssm->addr_lr;
	ssm->max_num                = ssm->record_len / ssm->batch_size;
	ssm->record_start           = ssm->addr_hr + SBI_TS_BUF_SIZE;
	ssm->record_timestamp_start = ssm->addr_hr;
	ssm->is_activated           = false;
	ssm->enter_960_60_transition = false;
	ssm->prev_frame_id          = 1;
	ssm->error_status           = CAM_SBI_NO_ERROR;
	ssm->error_notified = false;

	ssm->write_done_info = false;
	ssm->make_960_info = false;
	ssm->make_60_info = false;
	ssm->meet_last_frame_info = false;
	ssm->task3_enable_info = false;


	CAM_DBG(CAM_SBI, "ssm->preview_len            = %d(%dMB)",
			ssm->preview_len, ssm->preview_len / (1024*1024));
	CAM_DBG(CAM_SBI, "ssm->record_len             = %d(%dMB)",
			ssm->record_len, ssm->record_len / (1024*1024));
	CAM_DBG(CAM_SBI, "ssm->preview                = 0x%x", ssm->preview);
	CAM_DBG(CAM_SBI, "ssm->preview_timestamp      = 0x%x", ssm->preview_timestamp);
	CAM_DBG(CAM_SBI, "ssm->record_start           = 0x%x", ssm->record_start);
	CAM_DBG(CAM_SBI, "ssm->record_timestamp_start = 0x%x", ssm->record_timestamp_start);
	CAM_DBG(CAM_SBI, "ssm->max_num                = %d", ssm->max_num);
	CAM_DBG(CAM_SBI, "ssm->sensor_width           = %d", ssm->sensor_width);
	CAM_DBG(CAM_SBI, "ssm->sensor_height          = %d", ssm->sensor_height);
	CAM_DBG(CAM_SBI, "ssm->frame_size             = %d", ssm->frame_size);
	CAM_DBG(CAM_SBI, "ssm->batch_num              = %d", ssm->batch_num);
	CAM_DBG(CAM_SBI, "ssm->batch_size             = %d", ssm->batch_size);
	CAM_DBG(CAM_SBI, "ssm->max_frames             = %d", ssm->max_frames);
	CAM_DBG(CAM_SBI, "ssm->cue_option             = %d", ssm->cue_option);
	CAM_DBG(CAM_SBI, "ssm->ssm_framerate          = %d", ssm->ssm_framerate);

	cam_sbi_hw_init_dma_address(sbi_hw);

	/* set up DMA offset value */
	SBI_REG_WRITE(dma->dma_wdma0_offset_addr, ssm->frame_size);
	SBI_REG_WRITE(dma->dma_rdma1_offset_addr, ssm->frame_size);
	cam_sbi_hw_task1_switch(sbi_hw, false);
	cam_sbi_hw_task3_enable(sbi_hw, false);

	cam_sbi_hw_rup_ready(sbi_hw, BIT(TASK1_RUP_RDY) |
			BIT(TASK2_RUP_RDY) | BIT(TASK3_RUP_RDY));
}

void cam_sbi_hw_dump_all_reg(struct cam_hw_info *sbi_hw, int idx)
{
	int i = 0;
	int read = 0;

	CAM_DBG(CAM_SBI, "%s e idx(%d)", __FUNCTION__, idx);

	for (i = 0; i < 322; i++) {
		read = SBI_REG_READ(i);
		CAM_DBG(CAM_SBI, "\t\t\t addr %08x\tdata %08x", i, read);
	}

	CAM_DBG(CAM_SBI, "%s x", __FUNCTION__);
	return;
}

void cam_sbi_hw_dump_specific_reg(struct cam_hw_info *sbi_hw, int param)
{
	int read = 0;
	int addr = 0;

	CAM_DBG(CAM_SBI, "%s e addr(%x)", __FUNCTION__, param);

	{
		addr = param + CAM_SBI_REG_OFFSET;
		read = cam_io_r_mb(sbi_hw->soc_info.reg_map[0].mem_base + addr);
		CAM_DBG(CAM_SBI, "\t\t\t addr %08x\tdata %08x", addr, read);
	}

	CAM_DBG(CAM_SBI, "%s x", __FUNCTION__);
	return;
}

int cam_sbi_hw_core_writereg(struct cam_hw_info *sbi_hw, int addr, int val)
{
	struct cam_sbi_core *sbi_core;
	struct cam_hw_soc_info *soc_info = &sbi_hw->soc_info;
	sbi_core = sbi_hw->core_info;

	addr += CAM_SBI_REG_OFFSET;
	CAM_DBG(CAM_SBI, "@@ mem_base : %p , addr : %x, val %x",
		soc_info->reg_map[0].mem_base, addr, val);
	cam_io_w_mb(val, soc_info->reg_map[0].mem_base + addr);

	return 0;
}

int cam_sbi_hw_core_readreg(struct cam_hw_info *sbi_hw, int addr)
{
	struct cam_sbi_core *sbi_core;
	struct cam_hw_soc_info *soc_info = &sbi_hw->soc_info;
	uint32_t read_val = 0;
	sbi_core = sbi_hw->core_info;

	addr += CAM_SBI_REG_OFFSET;
	read_val = cam_io_r_mb(soc_info->reg_map[0].mem_base + addr);
	CAM_DBG(CAM_SBI, "@@ mem_base : %p , addr : %x, read_val = %x",
		soc_info->reg_map[0].mem_base, addr, read_val);

	return 0;
}


// andy.yoo: todo
//static void cam_sbi_cdm_write_reg_val_pair(uint32_t *buffer,
//	uint32_t *index, uint32_t reg_offset, uint32_t reg_value)
//{
//	buffer[(*index)++] = reg_offset;
//	buffer[(*index)++] = reg_value;
//}

static void
cam_sbi_hw_util_fill_ssm_specific_reg(struct cam_sbi_hw_io_buffer *io_buf,
					uint32_t index, uint32_t *reg_val_pair,
					uint32_t *num_cmd,
					struct cam_sbi_hw_info *hw_info)
{
	uint32_t reg_val;

	/* 1. config buffer size */
	reg_val = io_buf->io_cfg->planes[0].width;
	reg_val |= (io_buf->io_cfg->planes[0].height << 16);

	// andy.yoo: todo
	//cam_sbi_cdm_write_reg_val_pair(reg_val_pair, num_cmd,
	//	hw_info->bus_rd_reg.bus_client_reg[index].rd_buffer_size,
	//	reg_val);

	CAM_DBG(CAM_SBI, "width %d", io_buf->io_cfg->planes[0].width);
	CAM_DBG(CAM_SBI, "height %d", io_buf->io_cfg->planes[0].height);

	/* 2. config image address */
	// andy.yoo: todo
	//cam_sbi_cdm_write_reg_val_pair(reg_val_pair, num_cmd,
	//	hw_info->bus_rd_reg.bus_client_reg[index].addr_image,
	//	io_buf->io_addr[0]);

	CAM_DBG(CAM_SBI, "io addr %llu", io_buf->io_addr[0]);

	/* 3. config stride */
	reg_val = io_buf->io_cfg->planes[0].plane_stride;
	// andy.yoo: todo
	//cam_sbi_cdm_write_reg_val_pair(reg_val_pair, num_cmd,
	//	hw_info->bus_rd_reg.bus_client_reg[index].rd_stride,
	//	reg_val);

	CAM_DBG(CAM_SBI, "plane_stride %d",
		io_buf->io_cfg->planes[0].plane_stride);

	/* 4. enable client */
	// andy.yoo: todo
	//cam_sbi_cdm_write_reg_val_pair(reg_val_pair, num_cmd,
	//	hw_info->bus_rd_reg.bus_client_reg[index].core_cfg, 0x1);
}

static int cam_sbi_hw_util_process_config_hw(
	struct cam_hw_info *sbi_hw,
	struct cam_sbi_hw_cmd_config_args *config_args)
{
	//int i;
	struct cam_hw_soc_info *soc_info = &sbi_hw->soc_info;
	struct cam_sbi_cdm_info *hw_cdm_info;
	uint32_t *cmd_buf_addr = config_args->cmd_buf_addr;
	uint32_t reg_val_pair[CAM_SBI_MAX_REG_PAIR_NUM];
	struct cam_sbi_hw_io_buffer *io_buf;
	struct cam_sbi_hw_info *hw_info =
		((struct cam_sbi_core *)sbi_hw->core_info)->hw_info;
	uint32_t num_cmd = 0;
	uint32_t size;
	uint32_t mem_base, available_size = config_args->size;
	//uint32_t output_res_mask = 0, input_res_mask = 0;

	if (!cmd_buf_addr) {
		CAM_ERR(CAM_SBI, "Invalid input args");
		return -EINVAL;
	}

	hw_cdm_info = ((struct cam_sbi_core *)sbi_hw->core_info)->hw_cdm_info;

	//cam_sbi_hw_dump_all_reg(sbi_hw, 2);

	//for (i = 0; i < CAM_SBI_MAX_IO_BUFFER; i++)
	{
		io_buf =
			&config_args->input_buf
				[0]; // andy.yoo ; input_buf[0]; ? input_buf[i];

		// andy.yoo: todo
		//if (io_buf->valid == false)
		//	break;

		if (io_buf->io_cfg->direction != CAM_BUF_INPUT) {
			CAM_ERR(CAM_SBI, "Incorrect direction %d %d",
				io_buf->io_cfg->direction, CAM_BUF_INPUT);
			return -EINVAL;
		}
		CAM_DBG(CAM_SBI, "resource_type %d",
			io_buf->io_cfg->resource_type);

		switch (io_buf->io_cfg->resource_type) {
		case CAM_SBI_IO_TYPE_TAR:
			cam_sbi_hw_util_fill_ssm_specific_reg(
				io_buf, 0, reg_val_pair, &num_cmd, hw_info);

			//input_res_mask |= CAM_SBI_INPUT_PORT_TYPE_TAR;
			break;
		case CAM_SBI_IO_TYPE_REF:
			cam_sbi_hw_util_fill_ssm_specific_reg(
				io_buf, 1, reg_val_pair, &num_cmd, hw_info);

			//input_res_mask |= CAM_SBI_INPUT_PORT_TYPE_REF;
			break;
		default:
			CAM_ERR(CAM_SBI, "wrong resource_type %d",
				io_buf->io_cfg->resource_type);
			return -EINVAL;
		}
	}

	// andy.yoo todo
	//for (i = 0; i < CAM_SBI_BUS_RD_MAX_CLIENTS; i++)
	//if (!((input_res_mask >> i) & 0x1))
	//cam_sbi_cdm_write_reg_val_pair(reg_val_pair, &num_cmd,
	//    hw_info->bus_rd_reg.bus_client_reg[0].core_cfg,
	//    0x0);

	size = hw_cdm_info->cdm_ops->cdm_required_size_changebase();
	if ((size * 4) > available_size) {
		CAM_ERR(CAM_SBI, "buf size:%d is not sufficient, expected: %d",
			available_size, size);
		return -EINVAL;
	}

	mem_base = CAM_SOC_GET_REG_MAP_CAM_BASE(soc_info, CAM_SBI_BASE_IDX);

	hw_cdm_info->cdm_ops->cdm_write_changebase(cmd_buf_addr, mem_base);
	cmd_buf_addr += size;
	available_size -= (size * 4);

	size = hw_cdm_info->cdm_ops->cdm_required_size_reg_random(num_cmd / 2);

	if ((size * 4) > available_size) {
		CAM_ERR(CAM_SBI, "buf size:%d is not sufficient, expected: %d",
			available_size, size);
		return -ENOMEM;
	}

	hw_cdm_info->cdm_ops->cdm_write_regrandom(cmd_buf_addr, num_cmd / 2,
						reg_val_pair);
	cmd_buf_addr += size;
	available_size -= (size * 4);

	config_args->config_buf_size = config_args->size - available_size;

	return 0;
}

static int cam_sbi_hw_util_submit_go(struct cam_hw_info *sbi_hw)
{
	struct cam_sbi_core *sbi_core;
	//struct cam_hw_soc_info *soc_info;// andy.yoo: todo
	struct cam_sbi_hw_info *hw_info;

	sbi_core = (struct cam_sbi_core *)sbi_hw->core_info;
	hw_info = sbi_core->hw_info;
	(void)hw_info; //andy.yoo : todo
	//soc_info = &sbi_hw->soc_info;

	// andy.yoo: todo
	//cam_io_w_mb(0x1, soc_info->reg_map[0].mem_base +
	//	hw_info->bus_rd_reg.common_reg.cmd);

	return 0;
}


enum cam_sbi_hw_irq_regs {
	CAM_SBI_IRQ_CAMIF_REG_STATUS = 0,
	CAM_SBI_IRQ_VIOLATION_STATUS = 1,
	CAM_SBI_IRQ_REGISTERS_MAX,
};


static int cam_sbi_hw_wait_frame_done(struct cam_hw_info *sbi_hw)
{
	struct cam_sbi_core *sbi_core;
	struct cam_sbi_hw_info *hw_info;
	long time_left = 0;
	int rc = 0;

	sbi_core = sbi_hw->core_info;
	hw_info = sbi_core->hw_info;

	if ((sbi_hw->hw_state == CAM_HW_STATE_POWER_DOWN) ||
		(sbi_core->state == CAM_SBI_CORE_STATE_IDLE))
	{
		CAM_DBG(CAM_SBI, "overridden");
		return 0;
	}

	reinit_completion(&sbi_core->t1_frame_done_complete);
	time_left = wait_for_completion_timeout(
		&sbi_core->t1_frame_done_complete,
		msecs_to_jiffies(CAM_SBI_HW_FRAME_DONE_TIMEOUT));
	if (time_left <= 0) {
		CAM_ERR(CAM_SBI, "t1 rup_done wait failed time_left=%ld",
			time_left);
		rc = -1;
	}

	reinit_completion(&sbi_core->t2_frame_done_complete);
	time_left = wait_for_completion_timeout(
		&sbi_core->t2_frame_done_complete,
		msecs_to_jiffies(CAM_SBI_HW_FRAME_DONE_TIMEOUT));
	if (time_left <= 0) {
		CAM_ERR(CAM_SBI, "t2 rup_done wait failed time_left=%ld",
			time_left);
		rc = -1;
	}

	return rc;
}


static int cam_sbi_hw_util_reset(struct cam_hw_info *sbi_hw,
				uint32_t reset_type, int ln )
{
	struct cam_sbi_core *sbi_core;
	struct cam_hw_soc_info *soc_info = &sbi_hw->soc_info;
	struct cam_sbi_hw_info *hw_info;
	long time_left;

	sbi_core = sbi_hw->core_info;
	hw_info = sbi_core->hw_info;

	if (sbi_hw->hw_state == CAM_HW_STATE_POWER_DOWN)
	{
		CAM_DBG(CAM_SBI, "overridden, reset_type(%d)", reset_type);
		return 0;
	}

	CAM_DBG(CAM_SBI, "reset_type = %d called ln %d", reset_type, ln);

	switch (reset_type) {
	case CAM_SBI_HW_RESET_TYPE_ALL_RESET:
		reinit_completion(&sbi_core->all_reset_complete);

		cam_io_w_mb(BIT(CORE_RST), soc_info->reg_map[0].mem_base +
			hw_info->irq_reg_info->reset_cmd_offset);
		time_left = wait_for_completion_timeout(
			&sbi_core->all_reset_complete,
			msecs_to_jiffies(CAM_SBI_HW_RESET_TIMEOUT));
		if (time_left <= 0) {
			CAM_ERR(CAM_SBI, "sync reset timeout time_left=%ld",
				time_left);
			return -ETIMEDOUT;
		}
		break;
	case CAM_SBI_HW_RESET_TYPE_TASK1_RESET:
		reinit_completion(&sbi_core->t1_reset_complete);

		cam_io_w_mb(BIT(TASK1_RST), soc_info->reg_map[0].mem_base +
			hw_info->irq_reg_info->reset_cmd_offset);
		time_left = wait_for_completion_timeout(
			&sbi_core->t1_reset_complete,
			msecs_to_jiffies(CAM_SBI_HW_RESET_TIMEOUT));
		if (time_left <= 0) {
			CAM_ERR(CAM_SBI, "task1 reset timeout time_left=%ld",
				time_left);
			return -ETIMEDOUT;
		}
		break;
	case CAM_SBI_HW_RESET_TYPE_TASK2_RESET:
		reinit_completion(&sbi_core->t2_reset_complete);

		cam_io_w_mb(BIT(TASK2_RST), soc_info->reg_map[0].mem_base +
			hw_info->irq_reg_info->reset_cmd_offset);
		time_left = wait_for_completion_timeout(
			&sbi_core->t2_reset_complete,
			msecs_to_jiffies(CAM_SBI_HW_RESET_TIMEOUT));
		if (time_left <= 0) {
			CAM_ERR(CAM_SBI, "task2 reset timeout time_left=%ld",
				time_left);
			return -ETIMEDOUT;
		}
		break;
	case CAM_SBI_HW_RESET_TYPE_TASK3_RESET:
		reinit_completion(&sbi_core->t3_reset_complete);
		cam_io_w_mb(BIT(TASK3_RST), soc_info->reg_map[0].mem_base +
			hw_info->irq_reg_info->reset_cmd_offset);

		time_left = wait_for_completion_timeout(
			&sbi_core->t3_reset_complete,
			msecs_to_jiffies(CAM_SBI_HW_RESET_TIMEOUT));
		if (time_left <= 0) {
			CAM_ERR(CAM_SBI, "task3 reset timeout time_left=%ld",
				time_left);
			return -ETIMEDOUT;
		}
		break;
	}

	return 0;
}

// ToDo
int cam_sbi_hw_util_get_caps(struct cam_hw_info *sbi_hw,
				struct cam_sbi_dev_cap_info *hw_caps)
{
	//struct cam_hw_soc_info *soc_info = &sbi_hw->soc_info;// andy.yoo : todo
	struct cam_sbi_hw_info *hw_info =
		((struct cam_sbi_core *)sbi_hw->core_info)->hw_info;
	//uint32_t reg_value;// andy.yoo : todo

	if (!hw_info) {
		CAM_ERR(CAM_SBI, "Invalid hw info data");
		return -EINVAL;
	}

	// andy.yoo: todo
	//reg_value = cam_io_r_mb(soc_info->reg_map[0].mem_base +
	//	hw_info->clc_reg.clc_hw_version);

	return 0;
}

static int cam_sbi_hw_util_submit_req(struct cam_sbi_core *sbi_core,
					struct cam_sbi_frame_request *frame_req)
{
	struct cam_sbi_cdm_info *hw_cdm_info = sbi_core->hw_cdm_info;
	struct cam_cdm_bl_request *cdm_cmd = hw_cdm_info->cdm_cmd;
	struct cam_hw_update_entry *cmd;
	int i, rc = 0;

	if (frame_req->num_hw_update_entries > 0) {
		cdm_cmd->cmd_arrary_count = frame_req->num_hw_update_entries;
		cdm_cmd->type = CAM_CDM_BL_CMD_TYPE_MEM_HANDLE;
		cdm_cmd->flag = false;
		cdm_cmd->userdata = NULL;
		cdm_cmd->cookie = 0;

		for (i = 0; i <= frame_req->num_hw_update_entries; i++) {
			cmd = (frame_req->hw_update_entries + i);
			cdm_cmd->cmd[i].bl_addr.mem_handle = cmd->handle;
			cdm_cmd->cmd[i].offset = cmd->offset;
			cdm_cmd->cmd[i].len = cmd->len;
		}

		rc = cam_cdm_submit_bls(hw_cdm_info->cdm_handle, cdm_cmd);
		if (rc) {
			CAM_ERR(CAM_SBI, "Failed to submit cdm commands");
			return -EINVAL;
		}
	} else {
		CAM_ERR(CAM_SBI, "No hw update entry");
		rc = -EINVAL;
	}

	return rc;
}



#if 0
static void cam_sbi_dump_registers(void __iomem *base)
{
	CAM_INFO(CAM_SBI, "%s e", __FUNCTION__);

	/* dump the clc registers */
	cam_io_dump(base, 0x60, (0xc0 - 0x60) / 0x4);
	/* dump the fe and we registers */
	cam_io_dump(base, 0x200, (0x29c - 0x200) / 0x4);
	cam_io_dump(base, 0x2f0, (0x330 - 0x2f0) / 0x4);
	cam_io_dump(base, 0x500, (0x5b4 - 0x500) / 0x4);
	cam_io_dump(base, 0x700, (0x778 - 0x700) / 0x4);
	cam_io_dump(base, 0x800, (0x878 - 0x800) / 0x4);
	/* dump sbi sw registers, interrupts */
	cam_io_dump(base, 0x900, (0x928 - 0x900) / 0x4);

	CAM_INFO(CAM_SBI, "%s x", __FUNCTION__);
}

static int cam_sbi_hw_util_process_err(struct cam_hw_info *sbi_hw)
{
	struct cam_sbi_core *sbi_core = sbi_hw->core_info;
	struct cam_sbi_frame_request *req_proc, *req_submit;
	struct cam_sbi_hw_cb_args cb_args;
	int rc;

	req_proc = sbi_core->req_proc;
	req_submit = sbi_core->req_submit;
	cb_args.cb_type = CAM_SBI_CB_ERROR;

	if ((sbi_core->state != CAM_SBI_CORE_STATE_PROCESSING) &&
		(sbi_core->state != CAM_SBI_CORE_STATE_REQ_PENDING) &&
		(sbi_core->state != CAM_SBI_CORE_STATE_REQ_PROC_PEND)) {
		CAM_ERR(CAM_SBI, "Get error irq in wrong state %d",
			sbi_core->state);
	}

	cam_sbi_dump_registers(sbi_hw->soc_info.reg_map[0].mem_base);

	CAM_ERR_RATE_LIMIT(CAM_SBI, "Start recovery");
	sbi_core->state = CAM_SBI_CORE_STATE_RECOVERY;
	rc = cam_sbi_hw_util_reset(sbi_hw, CAM_SBI_HW_RESET_TYPE_ALL_RESET);
	if (rc)
		CAM_ERR(CAM_SBI, "Failed to reset");

	sbi_core->req_proc = NULL;
	sbi_core->req_submit = NULL;
	if (!rc)
		sbi_core->state = CAM_SBI_CORE_STATE_IDLE;

	cb_args.frame_req = req_proc;
	sbi_core->hw_mgr_cb.cam_sbi_hw_mgr_cb(sbi_core->hw_mgr_cb.data,
						&cb_args);

	cb_args.frame_req = req_submit;
	sbi_core->hw_mgr_cb.cam_sbi_hw_mgr_cb(sbi_core->hw_mgr_cb.data,
						&cb_args);

	return rc;
}

static int
cam_sbi_hw_util_process_reg_update(struct cam_hw_info *sbi_hw,
				struct cam_sbi_hw_cb_args *cb_args)
{
	struct cam_sbi_core *sbi_core = sbi_hw->core_info;
	int rc = 0;

	cb_args->cb_type |= CAM_SBI_CB_COMP_REG_UPDATE;
	if (sbi_core->state == CAM_SBI_CORE_STATE_REQ_PENDING) {
		sbi_core->state = CAM_SBI_CORE_STATE_PROCESSING;
	} else {
		CAM_ERR(CAM_SBI, "Reg update in wrong state %d",
			sbi_core->state);
		rc = cam_sbi_hw_util_process_err(sbi_hw);
		if (rc)
			CAM_ERR(CAM_SBI, "Failed to reset");
		return -EINVAL;
	}

	sbi_core->req_proc = sbi_core->req_submit;
	sbi_core->req_submit = NULL;

	if (sbi_core->dump_flag)
		cam_sbi_dump_registers(sbi_hw->soc_info.reg_map[0].mem_base);

	return 0;
}

static int cam_sbi_hw_util_process_idle(struct cam_hw_info *sbi_hw,
					struct cam_sbi_hw_cb_args *cb_args)
{
	struct cam_sbi_core *sbi_core = sbi_hw->core_info;
	int rc = 0;

	cb_args->cb_type |= CAM_SBI_CB_BUF_DONE;
	switch (sbi_core->state) {
	case CAM_SBI_CORE_STATE_REQ_PROC_PEND:
		cam_sbi_hw_util_submit_go(sbi_hw);
		sbi_core->state = CAM_SBI_CORE_STATE_REQ_PENDING;
		break;

	case CAM_SBI_CORE_STATE_PROCESSING:
		sbi_core->state = CAM_SBI_CORE_STATE_IDLE;
		break;

	default:
		CAM_ERR(CAM_SBI, "Idle in wrong state %d", sbi_core->state);
		rc = cam_sbi_hw_util_process_err(sbi_hw);
		return rc;
	}
	cb_args->frame_req = sbi_core->req_proc;
	sbi_core->req_proc = NULL;

	return 0;
}
#endif

void cam_sbi_set_irq(struct cam_hw_info *sbi_hw, enum cam_sbi_irq_set set)
{
	struct cam_hw_soc_info *soc_info = &sbi_hw->soc_info;
	struct cam_sbi_core *sbi_core = sbi_hw->core_info;
	struct cam_sbi_hw_info *hw_info = sbi_core->hw_info;

	switch (set) {
	case CAM_SBI_IRQ_ENABLE:
		CAM_DBG(CAM_SBI, "cam_sbi_set_irq : enabeld");
		cam_io_w_mb(0xFFFF,
			soc_info->reg_map[0].mem_base +
			hw_info->irq_reg_info->irq_reg_set->mask_reg_offset);
		break;

	case CAM_SBI_IRQ_DISABLE:
		CAM_DBG(CAM_SBI, "cam_sbi_set_irq : disabeld");
		cam_io_w_mb(0x1e00,
			soc_info->reg_map[0].mem_base +
			hw_info->irq_reg_info->irq_reg_set->mask_reg_offset);
		break;
	}
}

static void cam_sbi_hw_process_irq_t1_frame_drop(struct cam_hw_info * sbi_hw)
{
	struct cam_sbi_core * core = (struct cam_sbi_core *)sbi_hw->core_info;
	cam_sbi_ssm_info *    ssm  = &core->ssm_info;
	int regs[] = { 0x88, 0x62c, 0x76c };
	//int drop_stat;
	CAM_INFO(CAM_SBI, "@@ TASK1_FRAME_DROP %s %s",
		make_regs_string(sbi_hw, regs, sizeof(regs)/sizeof(int)),
		(ssm->rec_write_enable) ?
			(RECORD960(ssm->frame_id) ? "" : "befor write") :
			"after write_done");

	if (RECORD960(ssm->frame_id) &&
		(ssm->total_written_frames < ssm->max_frames))
		ssm->frame_drop_count += ssm->batch_num;
}

int cam_sbi_hw_process_irq(void *priv, void *data)
{
	struct cam_sbi_hw_work_data *work_data;
	struct cam_hw_info *sbi_hw;
	struct cam_sbi_core *sbi_core;
	struct cam_hw_soc_info *soc_info;
	int rc = 0;
	uint32_t top_irq_status;
	struct cam_sbi_hw_cb_args cb_args;
	cam_sbi_ssm_info * ssm;

	if (!data || !priv) {
		CAM_DBG(CAM_SBI, "Invalid data %pK %pK", data, priv);
		return -EINVAL;
	}

	memset(&cb_args, 0, sizeof(struct cam_sbi_hw_cb_args));
	sbi_hw         = (struct cam_hw_info *)priv;
	soc_info       = &sbi_hw->soc_info;
	work_data      = (struct cam_sbi_hw_work_data *)data;
	sbi_core       = (struct cam_sbi_core *)sbi_hw->core_info;
	ssm            = &sbi_core->ssm_info;
	top_irq_status = work_data->top_irq_status;

	mutex_lock(&sbi_hw->hw_mutex);

	CAM_DBG(CAM_SBI, "@topirq 0x%x", top_irq_status);

	if (top_irq_status & BIT(TASK1_RUP_DONE)) {
		int regs[] = { 0x1f4, 0x1f8, 0x1ec, 0x1f0 };
		CAM_DBG(CAM_SBI, "@@ %s   %s", CONV_STR(TASK1_RUP_DONE),
			make_regs_string(sbi_hw, regs, sizeof(regs)/sizeof(int)));
		SBI_REG_WRITE(0x160, 1); SBI_REG_WRITE(0x160, 0); // clear 0x1f4
		SBI_REG_WRITE(0x7c, 0);
		CAM_DBG(CAM_SBI, "frame_id = %d", ssm->frame_id);
		if (ssm->write_done_info) {
			CAM_INFO(CAM_SBI, "SSM recording write stopped! total_written_frames = %d",
				ssm->total_written_frames);
			ssm->write_done_info = false;
		}
		if (ssm->make_960_info) {
			CAM_INFO(CAM_SBI, "Framerate is changed(60 > 960). "
				"We forcely handle the frames as 960");
			ssm->make_960_info = false;
		}
	}

	if (top_irq_status & BIT(TASK2_RUP_DONE)) {
		int regs[] = { 0x1f4, 0x1fc, 0x1ec };
		CAM_DBG(CAM_SBI, "@@ %s   %s", CONV_STR(TASK2_RUP_DONE),
			make_regs_string(sbi_hw, regs, sizeof(regs)/sizeof(int)));
		SBI_REG_WRITE(0x80, 0);

		// task2_action register update
		if ((ssm->is_ssm_mode == false) && (sbi_core->t2_act_val != 0)) {
			SBI_REG_WRITE(0x34, sbi_core->t2_act_val);
			sbi_core->t2_stat_check = false;
			CAM_INFO(CAM_SBI, "TASK2_ACTION UPDATE");
		}
	}

	if (top_irq_status & BIT(TASK3_RUP_DONE)) {
		int regs[] = { 0x1f8, 0x1ec, 0x1f0, 0x14 };
		CAM_DBG(CAM_SBI, "@@ %s   %s", CONV_STR(TASK3_RUP_DONE),
			make_regs_string(sbi_hw, regs, sizeof(regs)/sizeof(int)));
		SBI_REG_WRITE(0x84, 0);
	}

	if (top_irq_status & BIT(TASK1_FRAME_DONE)) {
		int regs[] = { 0x1f4, 0x1fc };
		CAM_DBG(CAM_SBI, "@@ %s %s", CONV_STR(TASK1_FRAME_DONE),
			make_regs_string(sbi_hw, regs, sizeof(regs)/sizeof(int)));
		if (ssm->make_60_info) {
			CAM_INFO(CAM_SBI, "Framerate is changed(960 > 60). total_read_frames = %d",
					ssm->total_read_frames);
			ssm->make_60_info = false;
		}
		if (ssm->task3_enable_info) {
			struct cam_sbi_hw_info * hw_info = sbi_core->hw_info;
			int val = SBI_REG_READ(0x14);
			int flag = val & 0x1;
			CAM_DBG(CAM_SBI, "TASK3 %s. val=0x%x", flag ? "enabled" : "disabled", val);
			val = REG_READ(hw_info->irq_reg_info->irq_reg_set->mask_reg_offset);
			CAM_DBG(CAM_SBI, "TASK3 interrupts are %s. interrupt mask=0x%x",
				flag ? "enabled" : "disabled", val);
			ssm->task3_enable_info = false;
		}
	}

	if (top_irq_status & BIT(TASK2_FRAME_DONE)) {
		int regs[] = { 0x1fc, 0x1ec, 0x1f0 };
		CAM_DBG(CAM_SBI, "@@ %s %s",  CONV_STR(TASK2_FRAME_DONE),
			make_regs_string(sbi_hw, regs, sizeof(regs)/sizeof(int)));

		if ((ssm->is_ssm_mode == false) && (sbi_core->t2_stat_check == true)) {
			int val = SBI_REG_READ(0x174);
			if (val & 0x1) {
				CAM_INFO(CAM_SBI, "check TASK2_FRAME_PAD");
				SBI_REG_WRITE(0x174, val & (~0x1));
			}
		}
	}

	if (top_irq_status & BIT(TASK3_FRAME_DONE)) {
		int regs[] = { 0x1fc };
		bool is_fake = (SBI_REG_READ(0x1f8) >> 24) == 0;
		CAM_DBG(CAM_SBI, "@@ %s %s",
			is_fake ? "TASK3_FRAME_DONE_fake" : "TASK3_FRAME_DONE",
			make_regs_string(sbi_hw, regs, sizeof(regs)/sizeof(int)));
		if (ssm->meet_last_frame_info) {
			CAM_INFO(CAM_SBI, "SSM recording RDMA reaches the last frame. "
				"total_read_frames = %d",
				ssm->total_read_frames);
			ssm->meet_last_frame_info = false;
		}
	}

	if (top_irq_status & BIT(TASK1_FRAME_DROP)) {
		cam_sbi_hw_process_irq_t1_frame_drop(sbi_hw);
	}
	if (top_irq_status & BIT(TASK2_FRAME_DROP)) {
		int regs[] = { 0x8c };
		CAM_INFO(CAM_SBI, "@@ %s %s", CONV_STR(TASK2_FRAME_DROP),
			make_regs_string(sbi_hw, regs, 1));
	}
	if (top_irq_status& BIT(TASK3_FRAME_DROP)) {
		int regs[] = { 0x90 };
		CAM_INFO(CAM_SBI, "@@ %s %s", CONV_STR(TASK3_FRAME_DROP),
			 make_regs_string(sbi_hw, regs, 1));
	}
	if (top_irq_status & BIT(CORE_RST_DONE)) {
		CAM_DBG(CAM_SBI, "@@ CORE_RST_DONE");
	}
	if (top_irq_status & BIT(TASK1_RST_DONE)) {
		CAM_DBG(CAM_SBI, "@@ TASK1_RST_DONE");
	}
	if (top_irq_status & BIT(TASK2_RST_DONE)) {
		CAM_DBG(CAM_SBI, "@@ TASK2_RST_DONE");
	}
	if (top_irq_status & BIT(TASK3_RST_DONE)) {
		CAM_DBG(CAM_SBI, "@@ TASK3_RST_DONE");
	}
	if (top_irq_status & BIT(TASK1_FRAME_PAD)) {
		int err_stat;
		int regs[] = { 0x174 };
		CAM_ERR(CAM_SBI, "@@ TASK1_FRAME_PAD %s ",
			make_regs_string(sbi_hw, regs, 1));

		err_stat = SBI_REG_READ(0x174);
		if (err_stat & BIT(0)) {
			CAM_ERR(CAM_SBI, "t1 DRAM back pressure");
		}
		if (err_stat & BIT(2)) {
			CAM_ERR(CAM_SBI, "t1 RUP_RDY missing");
		}

		SBI_REG_WRITE(0x160, 1); SBI_REG_WRITE(0x160, 0);
	}
	if (top_irq_status & BIT(TASK2_FRAME_PAD)) {
		int regs[] = { 0x174 };
		CAM_ERR(CAM_SBI, "@@ %s %s ", CONV_STR(TASK2_FRAME_PAD),
			make_regs_string(sbi_hw, regs, 1));

		SBI_REG_WRITE(0x160, 1); SBI_REG_WRITE(0x160, 0);
	}
	if (top_irq_status & BIT(TASK3_FRAME_PAD)) {
		int regs[] = { 0x174 };
		CAM_ERR(CAM_SBI, "@@ %s %s ", CONV_STR(TASK3_FRAME_PAD),
			make_regs_string(sbi_hw, regs, 1));

		SBI_REG_WRITE(0x160, 1); SBI_REG_WRITE(0x160, 0);
	}
	if (top_irq_status & BIT(TASK1_CCIF_VIOLATION)) {
		int regs[] = { 0x1fc };
		CAM_DBG(CAM_SBI, "@@ %s %s", CONV_STR(TASK1_CCIF_VIOLATION),
			make_regs_string(sbi_hw, regs, 1));
	}
	if (top_irq_status & BIT(TASK2_CCIF_VIOLATION)) {
		int regs[] = { 0x1fc };
		CAM_DBG(CAM_SBI, "@@ %s %s", CONV_STR(TASK2_CCIF_VIOLATION),
			make_regs_string(sbi_hw, regs, 1));
	}
	if (top_irq_status & BIT(TASK3_CCIF_VIOLATION)) {
		int regs[] = { 0x1fc };
		CAM_DBG(CAM_SBI, "@@ %s %s", CONV_STR(TASK3_CCIF_VIOLATION),
			make_regs_string(sbi_hw, regs, 1));
	}


#if 0
	if (0) {
		static int prev_frame_id = 9999;
		int val = SBI_REG_READ(0x170);
		if (val != prev_frame_id) {
			CAM_INFO(CAM_SBI, "@@ frame_id_meta = 0x%x", val);
			prev_frame_id = val;
		}
	}

	if (0) // show debug reg
	{
		int regs[] = { 0x174, 0x1f4, 0x1f8, 0x1fc, 0x59c, 0xac };

		SBI_REG_WRITE(0x160, 1); // clear 0x1f4
		SBI_REG_WRITE(0x160, 0); // clear 0x1f4
		CAM_DBG(CAM_SBI, "@@ dbg_reg %s",
			make_regs_string(soc_info, regs, sizeof(regs)/sizeof(int)));
	}

	// if (top_irq_status & (1 << 3)) {
	if (0) {
		CAM_DBG(CAM_SBI, "Error");
		rc = cam_sbi_hw_util_process_err(sbi_hw);
		if (rc)
			CAM_ERR(CAM_SBI, "Process error failed");
		goto end;
	}

	// if (we_irq_status[0] & (1 << 1)) {
	if (0) {
		CAM_DBG(CAM_SBI, "reg update");
		rc = cam_sbi_hw_util_process_reg_update(sbi_hw, &cb_args);
		if (rc) {
			CAM_ERR(CAM_SBI, "Process reg_update failed");
			goto end;
		}
	}

	// if (top_irq_status & (1 << 4)) {
	if (0){
		CAM_DBG(CAM_SBI, "IDLE");
		if (!sbi_core->req_proc) {
			CAM_DBG(CAM_SBI, "No frame request to process idle");
			goto end;
		}
		rc = cam_sbi_hw_util_process_idle(sbi_hw, &cb_args);
		if (rc) {
			CAM_ERR(CAM_SBI, "Process idle failed");
			goto end;
		}
	}
#endif

	if (sbi_core->hw_mgr_cb.cam_sbi_hw_mgr_cb) {
		sbi_core->hw_mgr_cb.cam_sbi_hw_mgr_cb(sbi_core->hw_mgr_cb.data,
							&cb_args);
	} else {
		CAM_DBG(CAM_SBI, "No hw mgr cb");
		rc = -EINVAL;
	}

	mutex_unlock(&sbi_hw->hw_mutex);
	return rc;
}

int cam_sbi_hw_start(void *hw_priv, void *hw_start_args, uint32_t arg_size)
{
	struct cam_hw_info *sbi_hw = (struct cam_hw_info *)hw_priv;
	int rc = 0;
	struct cam_sbi_core * sbi_core;
	bool is_ssm_recording_instance = false;
	cam_sbi_ssm_info * ssm;

	if (!sbi_hw) {
		CAM_ERR(CAM_SBI, "Invalid input params, sbi_hw %pK", sbi_hw);
		return -EINVAL;
	}

	if (hw_start_args != NULL)
		is_ssm_recording_instance = *(bool *)hw_start_args;

	sbi_core = (struct cam_sbi_core *)sbi_hw->core_info;
	ssm = &sbi_core->ssm_info;

	mutex_lock(&sbi_hw->hw_mutex);

	if (is_ssm_recording_instance) {
		CAM_INFO(CAM_SBI, "SSM recording instance activated");
		ssm->is_activated = true;
		goto start_unlock;
	}

	if (sbi_hw->open_count > 0) {
		sbi_hw->open_count++;

		CAM_DBG(CAM_SBI, "SBI is started before");
		goto start_unlock;
	}

	rc = cam_sbi_soc_enable_resources(sbi_hw);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed to enable soc resources");
		goto start_unlock;
	}

	rc = cam_sbi_hw_util_reset(sbi_hw, CAM_SBI_HW_RESET_TYPE_TASK1_RESET, __LINE__);
	rc = cam_sbi_hw_util_reset(sbi_hw, CAM_SBI_HW_RESET_TYPE_TASK2_RESET, __LINE__);
	rc = cam_sbi_hw_util_reset(sbi_hw, CAM_SBI_HW_RESET_TYPE_TASK3_RESET, __LINE__);
	rc = cam_sbi_hw_util_reset(sbi_hw, CAM_SBI_HW_RESET_TYPE_ALL_RESET, __LINE__);
	if (rc) {
		CAM_ERR(CAM_SBI, "Failed to reset hw");
		goto disable_soc;
	}

	sbi_hw->hw_state = CAM_HW_STATE_POWER_UP;
	sbi_hw->open_count++;
	sbi_core->state = CAM_SBI_CORE_STATE_IDLE;

	CAM_DBG(CAM_SBI, "open count %d", sbi_hw->open_count);
	mutex_unlock(&sbi_hw->hw_mutex);
	return rc;

disable_soc:
	if (cam_sbi_soc_disable_resources(sbi_hw))
		CAM_ERR(CAM_SBI, "Error in disable soc resources");
start_unlock:
	CAM_INFO(CAM_SBI, "SBI started. open count %d", sbi_hw->open_count);
	mutex_unlock(&sbi_hw->hw_mutex);
	return rc;
}

int cam_sbi_hw_stop(void *hw_priv, void * stop_hw_args, uint32_t arg_size)
{
	struct cam_hw_info *sbi_hw = (struct cam_hw_info *)hw_priv;
	int rc = 0;
	struct cam_sbi_core *sbi_core;
	bool is_ssm_recording_instance = false;
	cam_sbi_ssm_info * ssm;

	if (!sbi_hw) {
		CAM_ERR(CAM_SBI, "Invalid argument");
		return -EINVAL;
	}

	if (stop_hw_args != NULL)
		is_ssm_recording_instance = *(bool *)stop_hw_args;

	sbi_core = (struct cam_sbi_core *)sbi_hw->core_info;
	ssm = &sbi_core->ssm_info;

	mutex_lock(&sbi_hw->hw_mutex);

	if (is_ssm_recording_instance) {
		CAM_INFO(CAM_SBI, "SSM recording instance deactivated. total_read_frames = %d",
			ssm->total_read_frames);
		ssm->is_activated = false;
		ssm->error_status = CAM_SBI_NO_ERROR;
		ssm->error_notified = false;
		goto stop_unlock;
	}

	if (sbi_hw->open_count == 0 ||
		sbi_hw->hw_state == CAM_HW_STATE_POWER_DOWN) {
		mutex_unlock(&sbi_hw->hw_mutex);
		CAM_INFO(CAM_SBI, "Error Unbalanced stop");
		return 0;
	}
	sbi_hw->open_count--;

	//if (sbi_hw->open_count)
	{
		rc = cam_sbi_hw_util_reset(sbi_hw, CAM_SBI_HW_RESET_TYPE_TASK1_RESET, __LINE__);
		rc = cam_sbi_hw_wait_frame_done(sbi_hw);
		rc = cam_sbi_hw_util_reset(sbi_hw, CAM_SBI_HW_RESET_TYPE_ALL_RESET, __LINE__);

		// sbi addr queue reset
		CAM_DBG(CAM_SBI, "clear address queue");
		SBI_REG_WRITE(0x0000062c, 0x00000008);
		SBI_REG_WRITE(0x0000062c, 0x00000000);
		SBI_REG_WRITE(0x000006ac, 0x00000008);
		SBI_REG_WRITE(0x000006ac, 0x00000000);
		SBI_REG_WRITE(0x0000076c, 0x00000008);
		SBI_REG_WRITE(0x0000076c, 0x00000000);
		SBI_REG_WRITE(0x000007ac, 0x00000008);
		SBI_REG_WRITE(0x000007ac, 0x00000000);
	}

	sbi_core->req_proc = NULL;
	sbi_core->req_submit = NULL;

#if 0 // CDM not in use for SBI
	if (sbi_core->hw_cdm_info) {
		struct cam_sbi_cdm_info *hw_cdm_info = sbi_core->hw_cdm_info;

		rc = cam_cdm_stream_off(hw_cdm_info->cdm_handle);
		if (rc) {
			CAM_ERR(CAM_SBI,
				"Failed in CDM StreamOff, handle=0x%x, rc=%d",
				hw_cdm_info->cdm_handle, rc);
			goto stop_unlock;
		}
	}
#endif

	rc = cam_sbi_soc_disable_resources(sbi_hw);
	if (rc)
		CAM_ERR(CAM_SBI, "Failed in Disable SOC, rc=%d", rc);
	sbi_core->ssm_info.is_ssm_mode = 0;

	sbi_hw->hw_state = CAM_HW_STATE_POWER_DOWN;
	if (sbi_core->state == CAM_SBI_CORE_STATE_IDLE) {
		sbi_core->state = CAM_SBI_CORE_STATE_INIT;
	} else {
		CAM_ERR(CAM_SBI, "HW in wrong state %d", sbi_core->state);
		rc = -EINVAL;
	}

stop_unlock:
	mutex_unlock(&sbi_hw->hw_mutex);
	CAM_INFO(CAM_SBI, "SBI stoped.  open count %d", sbi_hw->open_count);
	return rc;
}

int cam_sbi_hw_submit_req(void *hw_priv, void *hw_submit_args,
			uint32_t arg_size)
{
	struct cam_hw_info *sbi_hw = (struct cam_hw_info *)hw_priv;
	struct cam_sbi_core *sbi_core;
	struct cam_sbi_hw_submit_args *args =
		(struct cam_sbi_hw_submit_args *)hw_submit_args;
	int rc = 0;
	struct cam_sbi_frame_request *frame_req;

	if (!hw_priv || !hw_submit_args) {
		CAM_ERR(CAM_SBI, "Invalid input");
		return -EINVAL;
	}

	if (sizeof(struct cam_sbi_hw_submit_args) != arg_size) {
		CAM_ERR(CAM_SBI, "size of args %zu, arg_size %d",
			sizeof(struct cam_sbi_hw_submit_args), arg_size);
		return -EINVAL;
	}

	frame_req = args->frame_req;

	mutex_lock(&sbi_hw->hw_mutex);

	if (sbi_hw->open_count == 0) {
		CAM_ERR(CAM_SBI, "HW is not open");
		mutex_unlock(&sbi_hw->hw_mutex);
		return -EINVAL;
	}

	sbi_core = (struct cam_sbi_core *)sbi_hw->core_info;
	if (sbi_core->state != CAM_SBI_CORE_STATE_IDLE &&
		sbi_core->state != CAM_SBI_CORE_STATE_PROCESSING) {
		mutex_unlock(&sbi_hw->hw_mutex);
		CAM_DBG(CAM_SBI, "device busy, can not submit, state %d",
			sbi_core->state);
		return -EBUSY;
	}

	if (sbi_core->req_submit != NULL) {
		mutex_unlock(&sbi_hw->hw_mutex);
		CAM_ERR(CAM_SBI, "req_submit is not NULL");
		return -EBUSY;
	}

	rc = cam_sbi_hw_util_submit_req(sbi_core, frame_req);
	if (rc) {
		CAM_ERR(CAM_SBI, "Submit req failed");
		goto error;
	}

	switch (sbi_core->state) {
	case CAM_SBI_CORE_STATE_PROCESSING:
		sbi_core->state = CAM_SBI_CORE_STATE_REQ_PROC_PEND;
		break;

	case CAM_SBI_CORE_STATE_IDLE:
		cam_sbi_hw_util_submit_go(sbi_hw);
		sbi_core->state = CAM_SBI_CORE_STATE_REQ_PENDING;
		break;

	default:
		CAM_ERR(CAM_SBI, "Wrong hw state");
		rc = -EINVAL;
		goto error;
	}

	sbi_core->req_submit = frame_req;

	mutex_unlock(&sbi_hw->hw_mutex);
	CAM_DBG(CAM_SBI, "Release lock, submit done for req %llu",
		frame_req->req_id);

	return 0;

error:
	mutex_unlock(&sbi_hw->hw_mutex);

	return rc;
}

int cam_sbi_hw_reset(void *hw_priv, void *reset_core_args, uint32_t arg_size)
{
	struct cam_hw_info *sbi_hw = hw_priv;
	struct cam_sbi_core *sbi_core;
	struct cam_sbi_hw_reset_args *sbi_reset_args = reset_core_args;
	int rc;

	if (!hw_priv) {
		CAM_ERR(CAM_SBI, "Invalid input args");
		return -EINVAL;
	}

	if (!reset_core_args ||
		sizeof(struct cam_sbi_hw_reset_args) != arg_size) {
		CAM_ERR(CAM_SBI, "Invalid reset args");
		return -EINVAL;
	}

	sbi_core = sbi_hw->core_info;

	mutex_lock(&sbi_hw->hw_mutex);
	if (sbi_core->state == CAM_SBI_CORE_STATE_RECOVERY) {
		mutex_unlock(&sbi_hw->hw_mutex);
		CAM_ERR(CAM_SBI, "Reset not allowed in %d state",
			sbi_core->state);
		return -EINVAL;
	}

	sbi_core->state = CAM_SBI_CORE_STATE_RECOVERY;

	rc = cam_sbi_hw_util_reset(sbi_hw, CAM_SBI_HW_RESET_TYPE_TASK1_RESET, __LINE__);
	rc = cam_sbi_hw_util_reset(sbi_hw, CAM_SBI_HW_RESET_TYPE_TASK2_RESET, __LINE__);
	rc = cam_sbi_hw_wait_frame_done(sbi_hw);

	rc = cam_sbi_hw_util_reset(sbi_hw, CAM_SBI_HW_RESET_TYPE_ALL_RESET, __LINE__);
	if (rc) {
		mutex_unlock(&sbi_hw->hw_mutex);
		CAM_ERR(CAM_SBI, "Failed to reset %d", sbi_reset_args->reset_type);
		return rc;
	}

	sbi_core->state = CAM_SBI_CORE_STATE_IDLE;

	mutex_unlock(&sbi_hw->hw_mutex);
	CAM_INFO(CAM_SBI, "SBI is reseted.");

	return 0;
}



int cam_sbi_hw_get_caps(void *hw_priv, void *get_hw_cap_args, uint32_t arg_size)
{
	struct cam_hw_info *sbi_hw;
	struct cam_sbi_core *sbi_core;
	struct cam_sbi_dev_cap_info *sbi_hw_caps =
		(struct cam_sbi_dev_cap_info *)get_hw_cap_args;

	if (!hw_priv || !get_hw_cap_args) {
		CAM_ERR(CAM_SBI, "Invalid input pointers %pK %pK", hw_priv,
			get_hw_cap_args);
		return -EINVAL;
	}

	sbi_hw = (struct cam_hw_info *)hw_priv;
	sbi_core = (struct cam_sbi_core *)sbi_hw->core_info;
	*sbi_hw_caps = sbi_core->hw_caps;

	return 0;
}

static void cam_sbi_hw_check_activation(struct cam_hw_info *sbi_hw)
{
	struct cam_sbi_core * core = (struct cam_sbi_core *)sbi_hw->core_info;
	cam_sbi_ssm_info *    ssm  = &core->ssm_info;
	static bool prev_activated = false;

	if (!prev_activated && ssm->is_activated) {
		cam_sbi_hw_init_dma_address(sbi_hw);
		cam_sbi_hw_task1_switch(sbi_hw, true);
		cam_sbi_hw_task3_enable(sbi_hw, true);
		cam_sbi_hw_reg_manual_update(sbi_hw);
	}
	else if (prev_activated && !ssm->is_activated) {
		cam_sbi_hw_task3_enable(sbi_hw, false);
		cam_sbi_hw_reg_manual_update(sbi_hw);
	}

	prev_activated = ssm->is_activated;
}

static void cam_sbi_hw_irq_task1_rup_done(
	struct cam_hw_info *sbi_hw, uint32_t irq_status)
{
	struct cam_sbi_core * core = (struct cam_sbi_core *)sbi_hw->core_info;
	cam_sbi_ssm_info *    ssm  = &core->ssm_info;

	ssm->frame_id = (SBI_REG_READ(0x170) & 0xFF00) >> 8;

	if (ssm->is_ssm_mode && (irq_status & BIT(TASK1_RUP_DONE))) {
		/*
		 * We setup some special registers to make 960fps from 60 fps.
		 * It's because the sensor's batch is not alway 16
		 */
		/* frame rate transition : 60 > 960 */
		if (SENSOR60(ssm->prev_frame_id) && SENSOR960(ssm->frame_id))
			cam_sbi_hw_t1_frame_done_make_960(sbi_hw, true);

		/* check write done */
		cam_sbi_hw_task1_switch_check(sbi_hw, irq_status);

		/* update WDMA adresses */
		cam_sbi_hw_update_wdma_address(sbi_hw);

		if (RECORD960(ssm->frame_id) && ssm->rec_write_enable) {
			ssm->total_written_frames += ssm->batch_num;
		}
	}

	cam_sbi_hw_rup_ready(sbi_hw, BIT(TASK1_RUP_RDY));
}

static void cam_sbi_hw_irq_task1_frame_done(struct cam_hw_info *sbi_hw)
{
	struct cam_sbi_core * core = (struct cam_sbi_core *)sbi_hw->core_info;
	cam_sbi_ssm_info *    ssm  = &core->ssm_info;

	complete(&core->t1_frame_done_complete);

	if (ssm->is_ssm_mode) {
		//cam_sbi_hw_rup_ready(sbi_hw, BIT(TASK1_RUP_RDY));
		/*
		 * We setup some special registers to make 60fps from 960fps.
		 * We are checking TASK1_FRAME_DONE twice to make sure detecting
		 */

		/* 1. check 960 > 60 transition */
		if (SENSOR960(ssm->prev_frame_id) && SENSOR60(ssm->frame_id)) {
			ssm->enter_960_60_transition = true;
		}

		/* 2. the 2nd frame_done after 960 > 60 transition */
		else if (ssm->enter_960_60_transition && SENSOR60(ssm->frame_id)) {
			cam_sbi_hw_t1_frame_done_make_960(sbi_hw, false);
			ssm->enter_960_60_transition = false;
			if (ssm->frame_drop_count > 0)
				ssm->error_status = CAM_SBI_SSM_WRITE_FRAME_DROP;
		}

		if (ssm->cue_option_changed) {
			int val = (ssm->cue_option == CAM_SBI_CUE_OPTION_MANUAL)? 1 : 0;
			SBI_REG_WRITE(0x128, val);
			cam_sbi_hw_init_dma_address(sbi_hw);
			ssm->cue_option_changed = false;
		}
		cam_sbi_hw_check_activation(sbi_hw);
		ssm->prev_frame_id = ssm->frame_id;
	}
}

static void cam_sbi_hw_irq_task2_rup_done(struct cam_hw_info *sbi_hw)
{
	cam_sbi_hw_rup_ready(sbi_hw, BIT(TASK1_RUP_RDY) | BIT(TASK2_RUP_RDY));
}

static void cam_sbi_hw_irq_task2_frame_done(struct cam_hw_info *sbi_hw)
{
	struct cam_sbi_core* core = (struct cam_sbi_core*)sbi_hw->core_info;
	complete(&core->t2_frame_done_complete);
	cam_sbi_hw_rup_ready(sbi_hw, BIT(TASK1_RUP_RDY));
}

static void cam_sbi_hw_irq_task3_rup_done(struct cam_hw_info *sbi_hw)
{
	cam_sbi_hw_rup_ready(sbi_hw, BIT(TASK1_RUP_RDY) | BIT(TASK3_RUP_RDY));
}

static void cam_sbi_hw_irq_task3_frame_done(struct cam_hw_info *sbi_hw)
{
	struct cam_sbi_core * core = (struct cam_sbi_core *)sbi_hw->core_info;
	cam_sbi_ssm_info *    ssm  = &core->ssm_info;

	if (ssm->is_ssm_mode) {
		if (RECORD960(ssm->frame_id)) {
			ssm->total_read_frames += ssm->batch_num;
		}
		cam_sbi_hw_update_rdma_address(sbi_hw);
	}
	cam_sbi_hw_rup_ready(sbi_hw, BIT(TASK1_RUP_RDY));
}

irqreturn_t cam_sbi_hw_irq(int irq_num, void *data)
{
	struct cam_hw_info *sbi_hw;
	struct cam_sbi_core *sbi_core;
	struct cam_sbi_hw_info *hw_info;
	cam_sbi_ssm_info * ssm;
	struct crm_workq_task *task;
	struct cam_sbi_hw_work_data *work_data;
	struct cam_sbi_irq_controller_reg_info * reg_info;
	struct cam_sbi_dma_reg_addr * dma;
	int rc;
	uint32_t top_irq_status;

	if (!data) {
		CAM_ERR(CAM_SBI, "Invalid data in IRQ callback");
		return IRQ_NONE;
	} else {
		//CAM_DBG(CAM_SBI, "@irq ");
	}

	sbi_hw    = (struct cam_hw_info *)data;
	sbi_core  = (struct cam_sbi_core *)sbi_hw->core_info;
	hw_info   = sbi_core->hw_info;
	reg_info  = hw_info->irq_reg_info;
	dma       = &hw_info->dma_reg;
	ssm       = &sbi_core->ssm_info;

	top_irq_status = REG_READ(reg_info->irq_reg_set->status_reg_offset);

	if (top_irq_status & BIT(TASK1_FRAME_DONE)) {
		cam_sbi_hw_irq_task1_frame_done(sbi_hw);
	}

	if ((top_irq_status & BIT(TASK1_RUP_DONE)) ||
		(top_irq_status & BIT(TASK1_FRAME_DROP))) {
		cam_sbi_hw_irq_task1_rup_done(sbi_hw, top_irq_status);
	}

	if ((top_irq_status & BIT(TASK2_RUP_DONE)) ||
		(top_irq_status & BIT(TASK2_FRAME_DROP))) {
		cam_sbi_hw_irq_task2_rup_done(sbi_hw);
	}

	if (top_irq_status & BIT(TASK2_FRAME_DONE)) {
		cam_sbi_hw_irq_task2_frame_done(sbi_hw);
	}

	if ((top_irq_status & BIT(TASK3_RUP_DONE)) ||
		(top_irq_status & BIT(TASK3_FRAME_DROP))) {
		cam_sbi_hw_irq_task3_rup_done(sbi_hw);
	}

	if (top_irq_status & BIT(TASK3_FRAME_DONE)) {
		cam_sbi_hw_irq_task3_frame_done(sbi_hw);
	}

	// IRQ clear
	REG_WRITE(reg_info->irq_reg_set->clear_reg_offset, top_irq_status);
	REG_WRITE(reg_info->global_clear_offset, 0x1);

	if (top_irq_status & BIT(CORE_RST_DONE)) {
		complete(&sbi_core->all_reset_complete);
	}
	if (top_irq_status & BIT(TASK1_RST_DONE)) {
		complete(&sbi_core->t1_reset_complete);
	}
	if (top_irq_status & BIT(TASK2_RST_DONE)) {
		complete(&sbi_core->t2_reset_complete);
	}
	if (top_irq_status & BIT(TASK3_RST_DONE)) {
		complete(&sbi_core->t3_reset_complete);
	}

	if (top_irq_status) {
		task = cam_req_mgr_workq_get_task(sbi_core->work);
		if (!task) {
			CAM_ERR(CAM_SBI, "no empty task available");
			return IRQ_NONE;
		}
		work_data = (struct cam_sbi_hw_work_data *)task->payload;
		work_data->top_irq_status = top_irq_status;
		task->process_cb = cam_sbi_hw_process_irq;
		rc = cam_req_mgr_workq_enqueue_task(task, data,
							CRM_TASK_PRIORITY_0);
		if (rc)
			CAM_ERR(CAM_SBI, "Failed in enqueue work task, rc=%d", rc);
	}

	return IRQ_HANDLED;
}

int cam_sbi_hw_process_cmd(void *hw_priv, uint32_t cmd_type, void *cmd_args,
			uint32_t arg_size)
{
	struct cam_hw_info *sbi_hw = (struct cam_hw_info *)hw_priv;
	int rc = 0;

	//CAM_DBG(CAM_SBI, "[SBI_TEST] cmd_type : %d, cmd_args : %p", cmd_type, cmd_args);

	switch (cmd_type) {
	case CAM_SBI_HW_CMD_PREPARE_HW_UPDATE: {

		if (0) {
		struct cam_sbi_hw_cmd_config_args *config_args;

		config_args = (struct cam_sbi_hw_cmd_config_args *)cmd_args;
		rc = cam_sbi_hw_util_process_config_hw(sbi_hw, config_args);
		}
		break;
	}

	case CAM_SBI_HW_CMD_REGISTER_CB: {
		struct cam_sbi_hw_cmd_set_cb *cb_args;
		struct cam_sbi_device *hw_device;
		struct cam_sbi_core *sbi_core =
			(struct cam_sbi_core *)sbi_hw->core_info;
		cb_args = (struct cam_sbi_hw_cmd_set_cb *)cmd_args;
		sbi_core->hw_mgr_cb.cam_sbi_hw_mgr_cb =
			cb_args->cam_sbi_hw_mgr_cb;
		sbi_core->hw_mgr_cb.data = cb_args->data;
		hw_device = cb_args->data;
		rc = 0;
		break;
	}

	case CAM_SBI_HW_CMD_SUBMIT: {
		struct cam_sbi_hw_submit_args *submit_args;

		submit_args = (struct cam_sbi_hw_submit_args *)cmd_args;
		rc = cam_sbi_hw_submit_req(hw_priv, submit_args, arg_size);
		break;
	}

	case CAM_SBI_HW_CMD_DUMP_REGISTER: {
		struct cam_sbi_core *sbi_core =
			(struct cam_sbi_core *)sbi_hw->core_info;
		sbi_core->dump_flag = *(bool *)cmd_args;
		CAM_DBG(CAM_SBI, "dump_flag %d", sbi_core->dump_flag);
		break;
	}

	case CAM_SBI_HW_CMD_DUMP_ALL_REG:
	{
		int num = *(int*)cmd_args;
		cam_sbi_hw_dump_all_reg(sbi_hw, num);
	}
	break;

	case CAM_SBI_HW_CMD_DUMP_SPECIFIC_REG:
	{
		int addr = *(int *)cmd_args;
		cam_sbi_hw_dump_specific_reg(sbi_hw, addr);
	}
	break;

	case CAM_SBI_HW_CMD_INIT_PREVIEW:
	{
		cam_sbi_ssm_info dma = *(cam_sbi_ssm_info *)cmd_args;
		CAM_DBG(CAM_SBI, "CAM_SBI_HW_CMD_WRITEREG_BASE_ADDR_S1");
		cam_sbi_hw_configure_mem_base_addr(sbi_hw, dma.addr_lr);
	}
	break;

	case CAM_SBI_HW_CMD_INIT_SSM:
	{
		struct cam_sbi_core *sbi_core = (struct cam_sbi_core *)sbi_hw->core_info;
		sbi_core->ssm_info = *(cam_sbi_ssm_info *)cmd_args;
		cam_sbi_hw_configure_mem_base_addr_for_ssm(sbi_hw);
	}
	break;

	case CAM_SBI_HW_CMD_WRITEREG_TASK2_SETUP:
	{
		uint32_t val = *(uint32_t *)cmd_args;
		CAM_DBG(CAM_SBI,
			"sbi_hw_process_cmd - CAM_SBI_HW_CMD_WRITEREG_TASK2_SETUP");

		cam_io_w_mb(val, sbi_hw->soc_info.reg_map[0].mem_base +
							0x10 + CAM_SBI_REG_OFFSET);
	}
	break;

	case CAM_SBI_HW_CMD_WRITEREG_TASK2_ACTION:
	{
		struct cam_sbi_core *sbi_core =
			(struct cam_sbi_core *)sbi_hw->core_info;
		uint32_t val = *(uint32_t *)cmd_args;

		if (sbi_core->prev_t2_act_val != val) {
			sbi_core->t2_act_val = val;
		} else {
			sbi_core->t2_act_val = 0;
		}
		sbi_core->prev_t2_act_val = val;
	}
	break;

	case CAM_SBI_HW_CMD_UPDATE_SSM_INFO:
	{
		struct cam_sbi_cmd_buf_type_1 * cmd = (struct cam_sbi_cmd_buf_type_1 *)cmd_args;
		struct cam_sbi_core *sbi_core = (struct cam_sbi_core *)sbi_hw->core_info;
		cam_sbi_ssm_info * ssm = &sbi_core->ssm_info;
		ssm->is_ssm_mode = true;

		if ((ssm->cue_option != cmd->cue_option) ||
			(ssm->ssm_framerate != cmd->ssm_framerate)) {

			if (cmd->ssm_framerate > 0) {
				ssm->ssm_framerate = cmd->ssm_framerate;

				ssm->max_frames = cmd->ssm_maxframes + (CAM_SBI_ADDR_FIFO_Q_NUM * 8);// 8 = batch num

				CAM_INFO(CAM_SBI, "real ssm->ssm_framerate:%d, max_frames:%d",
					ssm->ssm_framerate, ssm->max_frames);
			}

			// cue option update
			if ((ssm->cue_option != cmd->cue_option) &&
				(((ssm->cue_option > 0) && (cmd->cue_option == 0)) ||
				((ssm->cue_option == 0) && (cmd->cue_option > 0))))
			{
				ssm->cue_option = cmd->cue_option;
				ssm->cue_option_changed = true;
			}
		}
	}
	break;

	case CAM_SBI_HW_CMD_SET_INIT:
	{
		CAM_DBG(CAM_SBI, "sbi_hw_process_cmd - CAM_SBI_HW_CMD_SET_INIT");
		cam_sbi_hw_configure_init(sbi_hw, cmd_args, arg_size);
	}
	break;

	default:
		break;
	}

	return rc;
}
