// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
 */

#include "dp_catalog.h"
#include "dp_reg.h"
#include "dp_debug.h"
#ifdef CONFIG_SEC_DISPLAYPORT
#include "secdp.h"
#endif

#define dp_catalog_get_priv_v420(x) ({ \
	struct dp_catalog *catalog; \
	catalog = container_of(x, struct dp_catalog, x); \
	container_of(catalog->sub, \
		struct dp_catalog_private_v420, sub); \
})

#define dp_read(x) ({ \
	catalog->sub.read(catalog->dpc, io_data, x); \
})

#define dp_write(x, y) ({ \
	catalog->sub.write(catalog->dpc, io_data, x, y); \
})

#define MAX_VOLTAGE_LEVELS 4
#define MAX_PRE_EMP_LEVELS 4

#ifndef CONFIG_SEC_DISPLAYPORT
static u8 const vm_pre_emphasis[MAX_VOLTAGE_LEVELS][MAX_PRE_EMP_LEVELS] = {
	{0x00, 0x0E, 0x16, 0xFF},       /* pe0, 0 db */
	{0x00, 0x0E, 0x16, 0xFF},       /* pe1, 3.5 db */
	{0x00, 0x0E, 0xFF, 0xFF},       /* pe2, 6.0 db */
	{0xFF, 0xFF, 0xFF, 0xFF}        /* pe3, 9.5 db */
};

/* voltage swing, 0.2v and 1.0v are not support */
static u8 const vm_voltage_swing[MAX_VOLTAGE_LEVELS][MAX_PRE_EMP_LEVELS] = {
	{0x07, 0x0F, 0x16, 0xFF}, /* sw0, 0.4v  */
	{0x11, 0x1E, 0x1F, 0xFF}, /* sw1, 0.6 v */
	{0x1A, 0x1F, 0xFF, 0xFF}, /* sw1, 0.8 v */
	{0xFF, 0xFF, 0xFF, 0xFF}  /* sw1, 1.2 v, optional */
};

static u8 const dp_pre_emp_hbr2_hbr3[MAX_VOLTAGE_LEVELS][MAX_PRE_EMP_LEVELS] = {
	{0x00, 0x0C, 0x15, 0x1A}, /* pe0, 0 db */
	{0x02, 0x0E, 0x16, 0xFF}, /* pe1, 3.5 db */
	{0x02, 0x11, 0xFF, 0xFF}, /* pe2, 6.0 db */
	{0x04, 0xFF, 0xFF, 0xFF}  /* pe3, 9.5 db */
};

static u8 const dp_swing_hbr2_hbr3[MAX_VOLTAGE_LEVELS][MAX_PRE_EMP_LEVELS] = {
	{0x02, 0x12, 0x16, 0x1A}, /* sw0, 0.4v  */
	{0x09, 0x19, 0x1F, 0xFF}, /* sw1, 0.6v */
	{0x10, 0x1F, 0xFF, 0xFF}, /* sw1, 0.8v */
	{0x1F, 0xFF, 0xFF, 0xFF}  /* sw1, 1.2v */
};

static u8 const dp_pre_emp_hbr_rbr[MAX_VOLTAGE_LEVELS][MAX_PRE_EMP_LEVELS] = {
	{0x00, 0x0E, 0x15, 0x1A}, /* pe0, 0 db */
	{0x00, 0x0E, 0x15, 0xFF}, /* pe1, 3.5 db */
	{0x00, 0x0E, 0xFF, 0xFF}, /* pe2, 6.0 db */
	{0x04, 0xFF, 0xFF, 0xFF}  /* pe3, 9.5 db */
};

static u8 const dp_swing_hbr_rbr[MAX_VOLTAGE_LEVELS][MAX_PRE_EMP_LEVELS] = {
	{0x08, 0x0F, 0x16, 0x1F}, /* sw0, 0.4v */
	{0x11, 0x1E, 0x1F, 0xFF}, /* sw1, 0.6v */
	{0x16, 0x1F, 0xFF, 0xFF}, /* sw1, 0.8v */
	{0x1F, 0xFF, 0xFF, 0xFF}  /* sw1, 1.2v */
};
#else
#ifdef SECDP_CALIBRATE_VXPX
#include "inc/secdp_params_test.h"	/* for DP param tuning */
#else
#include "inc/secdp_params.h"		/* actual DP params as per project */
#endif
#endif

struct dp_catalog_private_v420 {
	struct device *dev;
	struct dp_catalog_sub sub;
	struct dp_catalog_io *io;
	struct dp_catalog *dpc;
};

static void dp_catalog_aux_setup_v420(struct dp_catalog_aux *aux,
		struct dp_aux_cfg *cfg)
{
	struct dp_catalog_private_v420 *catalog;
	struct dp_io_data *io_data;
	int i = 0;

	if (!aux || !cfg) {
		DP_ERR("invalid input\n");
		return;
	}

	DP_DEBUG("+++\n");

	catalog = dp_catalog_get_priv_v420(aux);

	io_data = catalog->io->dp_phy;
	dp_write(DP_PHY_PD_CTL, 0x67);
	wmb(); /* make sure PD programming happened */

	/* Turn on BIAS current for PHY/PLL */
	io_data = catalog->io->dp_pll;
	dp_write(QSERDES_COM_BIAS_EN_CLKBUFLR_EN, 0x17);
	wmb(); /* make sure BIAS programming happened */

	io_data = catalog->io->dp_phy;
	/* DP AUX CFG register programming */
	for (i = 0; i < PHY_AUX_CFG_MAX; i++) {
		DP_DEBUG("%s: offset=0x%08x, value=0x%08x\n",
			dp_phy_aux_config_type_to_string(i),
			cfg[i].offset, cfg[i].lut[cfg[i].current_index]);
		dp_write(cfg[i].offset, cfg[i].lut[cfg[i].current_index]);
	}
	wmb(); /* make sure DP AUX CFG programming happened */

	dp_write(DP_PHY_AUX_INTERRUPT_MASK_V420, 0x1F);
}

static void dp_catalog_aux_clear_hw_int_v420(struct dp_catalog_aux *aux)
{
	struct dp_catalog_private_v420 *catalog;
	struct dp_io_data *io_data;
	u32 data = 0;

	if (!aux) {
		DP_ERR("invalid input\n");
		return;
	}

	catalog = dp_catalog_get_priv_v420(aux);
	io_data = catalog->io->dp_phy;

	data = dp_read(DP_PHY_AUX_INTERRUPT_STATUS_V420);

	dp_write(DP_PHY_AUX_INTERRUPT_CLEAR_V420, 0x1f);
	wmb(); /* make sure 0x1f is written before next write */

	dp_write(DP_PHY_AUX_INTERRUPT_CLEAR_V420, 0x9f);
	wmb(); /* make sure 0x9f is written before next write */

	dp_write(DP_PHY_AUX_INTERRUPT_CLEAR_V420, 0);
	wmb(); /* make sure register is cleared */
}

static void dp_catalog_panel_config_msa_v420(struct dp_catalog_panel *panel,
					u32 rate, u32 stream_rate_khz)
{
	u32 pixel_m, pixel_n;
	u32 mvid, nvid, reg_off = 0, mvid_off = 0, nvid_off = 0;
	u32 const nvid_fixed = 0x8000;
	u32 const link_rate_hbr2 = 540000;
	u32 const link_rate_hbr3 = 810000;
	struct dp_catalog_private_v420 *catalog;
	struct dp_io_data *io_data;

	if (!panel || !rate) {
		DP_ERR("invalid input\n");
		return;
	}

	if (panel->stream_id >= DP_STREAM_MAX) {
		DP_ERR("invalid stream id:%d\n", panel->stream_id);
		return;
	}

	catalog = dp_catalog_get_priv_v420(panel);
	io_data = catalog->io->dp_mmss_cc;

	if (panel->stream_id == DP_STREAM_1)
		reg_off = MMSS_DP_PIXEL1_M_V420 - MMSS_DP_PIXEL_M_V420;

	pixel_m = dp_read(MMSS_DP_PIXEL_M_V420 + reg_off);
	pixel_n = dp_read(MMSS_DP_PIXEL_N_V420 + reg_off);
	DP_DEBUG("pixel_m=0x%x, pixel_n=0x%x\n", pixel_m, pixel_n);

	mvid = (pixel_m & 0xFFFF) * 5;
	nvid = (0xFFFF & (~pixel_n)) + (pixel_m & 0xFFFF);

	if (nvid < nvid_fixed) {
		u32 temp;

		temp = (nvid_fixed / nvid) * nvid;
		mvid = (nvid_fixed / nvid) * mvid;
		nvid = temp;
	}

	DP_DEBUG("rate = %d\n", rate);

	if (panel->widebus_en)
		mvid <<= 1;

	if (link_rate_hbr2 == rate)
		nvid *= 2;

	if (link_rate_hbr3 == rate)
		nvid *= 3;

	io_data = catalog->io->dp_link;

	if (panel->stream_id == DP_STREAM_1) {
		mvid_off = DP1_SOFTWARE_MVID - DP_SOFTWARE_MVID;
		nvid_off = DP1_SOFTWARE_NVID - DP_SOFTWARE_NVID;
	}

	DP_DEBUG("mvid=0x%x, nvid=0x%x\n", mvid, nvid);
	dp_write(DP_SOFTWARE_MVID + mvid_off, mvid);
	dp_write(DP_SOFTWARE_NVID + nvid_off, nvid);
}

static void dp_catalog_ctrl_phy_lane_cfg_v420(struct dp_catalog_ctrl *ctrl,
		bool flipped, u8 ln_cnt)
{
	u32 info = 0x0;
	struct dp_catalog_private_v420 *catalog;
	struct dp_io_data *io_data;
	u8 orientation = BIT(!!flipped);

	if (!ctrl) {
		DP_ERR("invalid input\n");
		return;
	}

	catalog = dp_catalog_get_priv_v420(ctrl);
	io_data = catalog->io->dp_phy;

	info |= (ln_cnt & 0x0F);
	info |= ((orientation & 0x0F) << 4);
	DP_DEBUG("Shared Info = 0x%x\n", info);

	dp_write(DP_PHY_SPARE0_V420, info);
}

#ifdef SECDP_CALIBRATE_VXPX
static char g_prsht_val0 = 0xFF;
static char g_prsht_val1 = 0xFF;
#endif

static void dp_catalog_ctrl_update_vx_px_v420(struct dp_catalog_ctrl *ctrl,
		u8 v_level, u8 p_level, bool high)
{
	struct dp_catalog_private_v420 *catalog;
	struct dp_io_data *io_data;
	u8 value0, value1;
	u32 version;

	if (!ctrl || !((v_level < MAX_VOLTAGE_LEVELS)
		&& (p_level < MAX_PRE_EMP_LEVELS))) {
		DP_ERR("invalid input\n");
		return;
	}

	DP_DEBUG("hw: v=%d p=%d, high=%d\n", v_level, p_level, high);

	catalog = dp_catalog_get_priv_v420(ctrl);

	io_data = catalog->io->dp_ahb;
	version = dp_read(DP_HW_VERSION);

	/*
	 * For DP controller versions 1.2.3 and 1.2.4
	 */
	if ((version == 0x10020003) || (version == 0x10020004)) {
		if (high) {
			value0 = dp_swing_hbr2_hbr3[v_level][p_level];
			value1 = dp_pre_emp_hbr2_hbr3[v_level][p_level];
		} else {
			value0 = dp_swing_hbr_rbr[v_level][p_level];
			value1 = dp_pre_emp_hbr_rbr[v_level][p_level];
		}
	} else {
		value0 = vm_voltage_swing[v_level][p_level];
		value1 = vm_pre_emphasis[v_level][p_level];
	}

#ifdef SECDP_SELF_TEST
	if (secdp_self_test_status(ST_VOLTAGE_TUN) >= 0) {
		u8 val = secdp_self_test_get_arg(ST_VOLTAGE_TUN)[v_level*4 + p_level];

		DP_INFO("value0 : 0x%02x => 0x%02x\n", value0, val);
		value0 = val;
	}

	if (secdp_self_test_status(ST_PREEM_TUN) >= 0) {
		u8 val = secdp_self_test_get_arg(ST_PREEM_TUN)[v_level*4 + p_level];

		DP_INFO("value0 : 0x%02x => 0x%02x\n", value1, val);
		value1 = val;
	}
#endif

	/* program default setting first */
	io_data = catalog->io->dp_ln_tx0;
	dp_write(TXn_TX_DRV_LVL_V420, 0x2A);
	dp_write(TXn_TX_EMP_POST1_LVL, 0x20);

	io_data = catalog->io->dp_ln_tx1;
	dp_write(TXn_TX_DRV_LVL_V420, 0x2A);
	dp_write(TXn_TX_EMP_POST1_LVL, 0x20);

	/* Enable MUX to use Cursor values from these registers */
	value0 |= BIT(5);
	value1 |= BIT(5);

	/* Configure host and panel only if both values are allowed */
	if (value0 != 0xFF && value1 != 0xFF) {
		io_data = catalog->io->dp_ln_tx0;
		dp_write(TXn_TX_DRV_LVL_V420, value0);
		dp_write(TXn_TX_EMP_POST1_LVL, value1);

		io_data = catalog->io->dp_ln_tx1;
		dp_write(TXn_TX_DRV_LVL_V420, value0);
		dp_write(TXn_TX_EMP_POST1_LVL, value1);

		DP_DEBUG("hw: vx_value=0x%x px_value=0x%x\n",
			value0, value1);
	} else {
		DP_ERR("invalid vx (0x%x=0x%x), px (0x%x=0x%x\n",
			v_level, value0, p_level, value1);
	}

#ifdef SECDP_CALIBRATE_VXPX
	DP_INFO("[g_prsht_val0] 0x%02X\n", g_prsht_val0);
	if (g_prsht_val0 != 0xFF) {
		io_data = catalog->io->dp_ln_tx0;
		g_prsht_val0 |= BIT(5);
		/*USB3_DP_PHY_DP_QSERDES_TX0_PRE_EMPH*/
		dp_write(0x108, g_prsht_val0);
		DP_INFO("[g_prsht_val0] 0x%02X write done!\n", g_prsht_val0);
	}

	DP_INFO("[g_prsht_val1] 0x%02X\n", g_prsht_val1);
	if (g_prsht_val1 != 0xFF) {
		io_data = catalog->io->dp_ln_tx1;
		g_prsht_val1 |= BIT(5);
		/*USB3_DP_PHY_DP_QSERDES_TX1_PRE_EMPH*/
		dp_write(0x108, g_prsht_val1);
		DP_INFO("[g_prsht_val1] 0x%02X write done!\n", g_prsht_val1);
	}
#endif
}

static void dp_catalog_ctrl_lane_pnswap_v420(struct dp_catalog_ctrl *ctrl,
						u8 ln_pnswap)
{
	struct dp_catalog_private_v420 *catalog;
	struct dp_io_data *io_data;
	u32 cfg0, cfg1;

	catalog = dp_catalog_get_priv_v420(ctrl);

	cfg0 = 0x0a;
	cfg1 = 0x0a;

	cfg0 |= ((ln_pnswap >> 0) & 0x1) << 0;
	cfg0 |= ((ln_pnswap >> 1) & 0x1) << 2;
	cfg1 |= ((ln_pnswap >> 2) & 0x1) << 0;
	cfg1 |= ((ln_pnswap >> 3) & 0x1) << 2;

	io_data = catalog->io->dp_ln_tx0;
	dp_write(TXn_TX_POL_INV_V420, cfg0);

	io_data = catalog->io->dp_ln_tx1;
	dp_write(TXn_TX_POL_INV_V420, cfg1);
}

#ifdef SECDP_CALIBRATE_VXPX
int secdp_catalog_prsht0_show(char *val)
{
	*val = g_prsht_val0;

	DP_DEBUG("*val: 0x%02x\n", *val);

	return 0;
}

int secdp_catalog_prsht0_store(char val)
{
	int rc = 0;

	g_prsht_val0 = val;

	DP_DEBUG("g_prsht_val0: 0x%02x\n", g_prsht_val0);

	return rc;
}

int secdp_catalog_prsht1_show(char *val)
{
	*val = g_prsht_val1;

	DP_DEBUG("*val: 0x%02x\n", *val);

	return 0;
}

int secdp_catalog_prsht1_store(char val)
{
	int rc = 0;

	g_prsht_val1 = val;

	DP_DEBUG("g_prsht_val1: 0x%02x\n", g_prsht_val1);

	return rc;
}

int secdp_catalog_vx_show(char *buf, int len)
{
	u8 value0, value1, value2, value3;
	int i;

	DP_DEBUG("+++\n");

	for (i = 0; i < MAX_VOLTAGE_LEVELS; i++) {
		value0 = vm_voltage_swing[i][0];
		value1 = vm_voltage_swing[i][1];
		value2 = vm_voltage_swing[i][2];
		value3 = vm_voltage_swing[i][3];
		DP_INFO("%02x,%02x,%02x,%02x\n", value0, value1, value2, value3);
	}

	memcpy(buf, vm_voltage_swing, len);
	return len;
}

int secdp_catalog_vx_store(int *val, int size)
{
	int rc = 0;

	DP_DEBUG("+++\n");

	vm_voltage_swing[0][0] = val[0];
	vm_voltage_swing[0][1] = val[1];
	vm_voltage_swing[0][2] = val[2];
	vm_voltage_swing[0][3] = val[3];

	vm_voltage_swing[1][0] = val[4];
	vm_voltage_swing[1][1] = val[5];
	vm_voltage_swing[1][2] = val[6];
	vm_voltage_swing[1][3] = val[7];

	vm_voltage_swing[2][0] = val[8];
	vm_voltage_swing[2][1] = val[9];
	vm_voltage_swing[2][2] = val[10];
	vm_voltage_swing[2][3] = val[11];

	vm_voltage_swing[3][0] = val[12];
	vm_voltage_swing[3][1] = val[13];
	vm_voltage_swing[3][2] = val[14];
	vm_voltage_swing[3][3] = val[15];

	return rc;
}

int secdp_catalog_px_show(char *buf, int len)
{
	u8 value0, value1, value2, value3;
	int i;

	DP_DEBUG("+++\n");

	for (i = 0; i < MAX_PRE_EMP_LEVELS; i++) {
		value0 = vm_pre_emphasis[i][0];
		value1 = vm_pre_emphasis[i][1];
		value2 = vm_pre_emphasis[i][2];
		value3 = vm_pre_emphasis[i][3];
		DP_INFO("%02x,%02x,%02x,%02x\n", value0, value1, value2, value3);
	}

	memcpy(buf, vm_pre_emphasis, len);
	return len;
}

int secdp_catalog_px_store(int *val, int size)
{
	int rc = 0;

	DP_DEBUG("+++\n");

	vm_pre_emphasis[0][0] = val[0];
	vm_pre_emphasis[0][1] = val[1];
	vm_pre_emphasis[0][2] = val[2];
	vm_pre_emphasis[0][3] = val[3];

	vm_pre_emphasis[1][0] = val[4];
	vm_pre_emphasis[1][1] = val[5];
	vm_pre_emphasis[1][2] = val[6];
	vm_pre_emphasis[1][3] = val[7];

	vm_pre_emphasis[2][0] = val[8];
	vm_pre_emphasis[2][1] = val[9];
	vm_pre_emphasis[2][2] = val[10];
	vm_pre_emphasis[2][3] = val[11];

	vm_pre_emphasis[3][0] = val[12];
	vm_pre_emphasis[3][1] = val[13];
	vm_pre_emphasis[3][2] = val[14];
	vm_pre_emphasis[3][3] = val[15];

	return rc;
}

int secdp_catalog_vx_hbr2_hbr3_show(char *buf, int len)
{
	u8 value0, value1, value2, value3;
	int i;

	DP_DEBUG("+++\n");

	for (i = 0; i < MAX_VOLTAGE_LEVELS; i++) {
		value0 = dp_swing_hbr2_hbr3[i][0];
		value1 = dp_swing_hbr2_hbr3[i][1];
		value2 = dp_swing_hbr2_hbr3[i][2];
		value3 = dp_swing_hbr2_hbr3[i][3];
		DP_INFO("%02x,%02x,%02x,%02x\n", value0, value1, value2, value3);
	}

	memcpy(buf, dp_swing_hbr2_hbr3, len);
	return len;
}

int secdp_catalog_vx_hbr2_hbr3_store(int *val, int size)
{
	int rc = 0;

	DP_DEBUG("+++\n");

	dp_swing_hbr2_hbr3[0][0] = val[0];
	dp_swing_hbr2_hbr3[0][1] = val[1];
	dp_swing_hbr2_hbr3[0][2] = val[2];
	dp_swing_hbr2_hbr3[0][3] = val[3];

	dp_swing_hbr2_hbr3[1][0] = val[4];
	dp_swing_hbr2_hbr3[1][1] = val[5];
	dp_swing_hbr2_hbr3[1][2] = val[6];
	dp_swing_hbr2_hbr3[1][3] = val[7];

	dp_swing_hbr2_hbr3[2][0] = val[8];
	dp_swing_hbr2_hbr3[2][1] = val[9];
	dp_swing_hbr2_hbr3[2][2] = val[10];
	dp_swing_hbr2_hbr3[2][3] = val[11];

	dp_swing_hbr2_hbr3[3][0] = val[12];
	dp_swing_hbr2_hbr3[3][1] = val[13];
	dp_swing_hbr2_hbr3[3][2] = val[14];
	dp_swing_hbr2_hbr3[3][3] = val[15];

	return rc;
}

int secdp_catalog_px_hbr2_hbr3_show(char *buf, int len)
{
	u8 value0, value1, value2, value3;
	int i;

	DP_DEBUG("+++\n");

	for (i = 0; i < MAX_PRE_EMP_LEVELS; i++) {
		value0 = dp_pre_emp_hbr2_hbr3[i][0];
		value1 = dp_pre_emp_hbr2_hbr3[i][1];
		value2 = dp_pre_emp_hbr2_hbr3[i][2];
		value3 = dp_pre_emp_hbr2_hbr3[i][3];
		DP_INFO("%02x,%02x,%02x,%02x\n", value0, value1, value2, value3);
	}

	memcpy(buf, dp_pre_emp_hbr2_hbr3, len);
	return len;
}

int secdp_catalog_px_hbr2_hbr3_store(int *val, int size)
{
	int rc = 0;

	DP_DEBUG("+++\n");

	dp_pre_emp_hbr2_hbr3[0][0] = val[0];
	dp_pre_emp_hbr2_hbr3[0][1] = val[1];
	dp_pre_emp_hbr2_hbr3[0][2] = val[2];
	dp_pre_emp_hbr2_hbr3[0][3] = val[3];

	dp_pre_emp_hbr2_hbr3[1][0] = val[4];
	dp_pre_emp_hbr2_hbr3[1][1] = val[5];
	dp_pre_emp_hbr2_hbr3[1][2] = val[6];
	dp_pre_emp_hbr2_hbr3[1][3] = val[7];

	dp_pre_emp_hbr2_hbr3[2][0] = val[8];
	dp_pre_emp_hbr2_hbr3[2][1] = val[9];
	dp_pre_emp_hbr2_hbr3[2][2] = val[10];
	dp_pre_emp_hbr2_hbr3[2][3] = val[11];

	dp_pre_emp_hbr2_hbr3[3][0] = val[12];
	dp_pre_emp_hbr2_hbr3[3][1] = val[13];
	dp_pre_emp_hbr2_hbr3[3][2] = val[14];
	dp_pre_emp_hbr2_hbr3[3][3] = val[15];

	return rc;
}

int secdp_catalog_vx_hbr_rbr_show(char *buf, int len)
{
	u8 value0, value1, value2, value3;
	int i;

	DP_DEBUG("+++\n");

	for (i = 0; i < MAX_VOLTAGE_LEVELS; i++) {
		value0 = dp_swing_hbr_rbr[i][0];
		value1 = dp_swing_hbr_rbr[i][1];
		value2 = dp_swing_hbr_rbr[i][2];
		value3 = dp_swing_hbr_rbr[i][3];
		DP_INFO("%02x,%02x,%02x,%02x\n", value0, value1, value2, value3);
	}

	memcpy(buf, dp_swing_hbr_rbr, len);
	return len;
}

int secdp_catalog_vx_hbr_rbr_store(int *val, int size)
{
	int rc = 0;

	DP_DEBUG("+++\n");

	dp_swing_hbr_rbr[0][0] = val[0];
	dp_swing_hbr_rbr[0][1] = val[1];
	dp_swing_hbr_rbr[0][2] = val[2];
	dp_swing_hbr_rbr[0][3] = val[3];

	dp_swing_hbr_rbr[1][0] = val[4];
	dp_swing_hbr_rbr[1][1] = val[5];
	dp_swing_hbr_rbr[1][2] = val[6];
	dp_swing_hbr_rbr[1][3] = val[7];

	dp_swing_hbr_rbr[2][0] = val[8];
	dp_swing_hbr_rbr[2][1] = val[9];
	dp_swing_hbr_rbr[2][2] = val[10];
	dp_swing_hbr_rbr[2][3] = val[11];

	dp_swing_hbr_rbr[3][0] = val[12];
	dp_swing_hbr_rbr[3][1] = val[13];
	dp_swing_hbr_rbr[3][2] = val[14];
	dp_swing_hbr_rbr[3][3] = val[15];

	return rc;
}

int secdp_catalog_px_hbr_rbr_show(char *buf, int len)
{
	u8 value0, value1, value2, value3;
	int i;

	DP_DEBUG("+++\n");

	for (i = 0; i < MAX_PRE_EMP_LEVELS; i++) {
		value0 = dp_pre_emp_hbr_rbr[i][0];
		value1 = dp_pre_emp_hbr_rbr[i][1];
		value2 = dp_pre_emp_hbr_rbr[i][2];
		value3 = dp_pre_emp_hbr_rbr[i][3];
		DP_INFO("%02x,%02x,%02x,%02x\n", value0, value1, value2, value3);
	}

	memcpy(buf, dp_pre_emp_hbr_rbr, len);
	return len;
}

int secdp_catalog_px_hbr_rbr_store(int *val, int size)
{
	int rc = 0;

	DP_DEBUG("+++\n");

	dp_pre_emp_hbr_rbr[0][0] = val[0];
	dp_pre_emp_hbr_rbr[0][1] = val[1];
	dp_pre_emp_hbr_rbr[0][2] = val[2];
	dp_pre_emp_hbr_rbr[0][3] = val[3];

	dp_pre_emp_hbr_rbr[1][0] = val[4];
	dp_pre_emp_hbr_rbr[1][1] = val[5];
	dp_pre_emp_hbr_rbr[1][2] = val[6];
	dp_pre_emp_hbr_rbr[1][3] = val[7];

	dp_pre_emp_hbr_rbr[2][0] = val[8];
	dp_pre_emp_hbr_rbr[2][1] = val[9];
	dp_pre_emp_hbr_rbr[2][2] = val[10];
	dp_pre_emp_hbr_rbr[2][3] = val[11];

	dp_pre_emp_hbr_rbr[3][0] = val[12];
	dp_pre_emp_hbr_rbr[3][1] = val[13];
	dp_pre_emp_hbr_rbr[3][2] = val[14];
	dp_pre_emp_hbr_rbr[3][3] = val[15];

	return rc;
}
#endif

static void dp_catalog_put_v420(struct dp_catalog *catalog)
{
	struct dp_catalog_private_v420 *catalog_priv;

	if (!catalog)
		return;

	catalog_priv = container_of(catalog->sub,
			struct dp_catalog_private_v420, sub);
	devm_kfree(catalog_priv->dev, catalog_priv);
}

struct dp_catalog_sub *dp_catalog_get_v420(struct device *dev,
		struct dp_catalog *catalog, struct dp_catalog_io *io)
{
	struct dp_catalog_private_v420 *catalog_priv;

	if (!dev || !catalog) {
		DP_ERR("invalid input\n");
		return ERR_PTR(-EINVAL);
	}

	DP_DEBUG("+++\n");

	catalog_priv = devm_kzalloc(dev, sizeof(*catalog_priv), GFP_KERNEL);
	if (!catalog_priv)
		return ERR_PTR(-ENOMEM);

	catalog_priv->dev = dev;
	catalog_priv->io = io;
	catalog_priv->dpc = catalog;

	catalog_priv->sub.put      = dp_catalog_put_v420;

	catalog->aux.setup         = dp_catalog_aux_setup_v420;
	catalog->aux.clear_hw_interrupts = dp_catalog_aux_clear_hw_int_v420;
	catalog->panel.config_msa  = dp_catalog_panel_config_msa_v420;
	catalog->ctrl.phy_lane_cfg = dp_catalog_ctrl_phy_lane_cfg_v420;
	catalog->ctrl.update_vx_px = dp_catalog_ctrl_update_vx_px_v420;
	catalog->ctrl.lane_pnswap = dp_catalog_ctrl_lane_pnswap_v420;

	return &catalog_priv->sub;
}
