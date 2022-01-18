/*
 * Copyright (C) 2018 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "fingerprint.h"
#include "qbt2000_common.h"

#include <linux/msm-bus.h>
#include <linux/msm-bus-board.h>

#define MHZ_TO_BPS(mhz, w) ((uint64_t)mhz * 1000 * 1000 * w)

#define NUM_BUS_TABLE 11
#define BUS_W 4	/* SDM845 DDR Voting('w' for DDR is 4) */

enum {
	MHZ_NONE = 0,
	MHZ_200,
	MHZ_300,
	MHZ_451,
	MHZ_547,
	MHZ_681,
	MHZ_768,
	MHZ_1017,
	MHZ_1296,
	MHZ_1555,
	MHZ_1803
};

static int ab_ib_bus_vectors[NUM_BUS_TABLE][2] = {
{0, 0},		/* 0 */
{0, 200},	/* 1 */
{0, 300},	/* 2 */
{0, 451},	/* 3 */
{0, 547},	/* 4 */
{0, 681},	/* 5 */
{0, 768},	/* 6 */
{0, 1017},	/* 7 */
{0, 1296},	/* 8 */
{0, 1555},	/* 9 */
{0, 1803}	/* 10 */
};

static struct msm_bus_vectors fpsensor_reg_bus_vectors[NUM_BUS_TABLE];

static struct msm_bus_paths fpsensor_reg_bus_usecases[ARRAY_SIZE(
            fpsensor_reg_bus_vectors)];
 
static struct msm_bus_scale_pdata fpsensor_reg_bus_scale_table = {
      .usecase = fpsensor_reg_bus_usecases,
      .num_usecases = ARRAY_SIZE(fpsensor_reg_bus_usecases),
      .name = "fpsensor_bw",
};

static u32 bus_hdl;

static void fill_bus_vector(void)
{
	int i = 0;

	for (i = 0; i < NUM_BUS_TABLE; i++) {
		fpsensor_reg_bus_vectors[i].src = MSM_BUS_MASTER_AMPSS_M0;
		fpsensor_reg_bus_vectors[i].dst = MSM_BUS_SLAVE_EBI_CH0;
		fpsensor_reg_bus_vectors[i].ab = ab_ib_bus_vectors[i][0];
		fpsensor_reg_bus_vectors[i].ib = MHZ_TO_BPS(ab_ib_bus_vectors[i][1], BUS_W);
	}
}


int qbt2000_set_clk(struct qbt2000_drvdata *drvdata, bool onoff)
{
	int rc = 0;

	if (drvdata->enabled_clk == onoff) {
		pr_err("%s: already %s\n", __func__, onoff ? "enabled" : "disabled");
		return rc;
	}

	if (onoff) {
		wake_lock(&drvdata->fp_spi_lock);
		drvdata->enabled_clk = true;
	} else {
		wake_unlock(&drvdata->fp_spi_lock);
		drvdata->enabled_clk = false;
	}
	return rc;
}

int qbt2000_register_platform_variable(struct qbt2000_drvdata *drvdata)
{
	int i = 0;
	// register ddr freq setting table
	fill_bus_vector();
	for (i = 0; i < fpsensor_reg_bus_scale_table.num_usecases; i++) {
		fpsensor_reg_bus_usecases[i].num_paths = 1;
		fpsensor_reg_bus_usecases[i].vectors =
			&fpsensor_reg_bus_vectors[i];
	}
	
	bus_hdl = msm_bus_scale_register_client(&fpsensor_reg_bus_scale_table);
	return 0;
}

int qbt2000_unregister_platform_variable(struct qbt2000_drvdata *drvdata)
{
	msm_bus_scale_unregister_client(bus_hdl);

	return 0;
}

int qbt2000_set_cpu_speedup(struct qbt2000_drvdata *drvdata, int onoff)
{
	int rc = 0;

	if (onoff)
		msm_bus_scale_client_update_request(bus_hdl, MHZ_1555);
	else
		msm_bus_scale_client_update_request(bus_hdl, MHZ_NONE);

#if defined(ENABLE_SENSORS_FPRINT_SECURE)	
	if (drvdata->min_cpufreq_limit) {
		if (onoff) {
			u8 retry_cnt = 0;

			pr_info("%s: FP_CPU_SPEEDUP ON:%d, retry: %d\n",
					__func__, onoff, retry_cnt);
			
			pm_qos_add_request(&drvdata->pm_qos, PM_QOS_CPU_DMA_LATENCY, 0);
			do {
				rc = set_freq_limit(DVFS_FINGER_ID, drvdata->min_cpufreq_limit);
				retry_cnt++;
				if (rc) {
					pr_err("%s: booster start failed. (%d) retry: %d\n", __func__
						, rc, retry_cnt);
					usleep_range(500, 510);
				}
			} while (rc && retry_cnt < 7);
		} else {
			pr_info("%s: FP_CPU_SPEEDUP OFF\n", __func__);
			rc = set_freq_limit(DVFS_FINGER_ID, -1);
			if (rc)
				pr_err("%s: booster stop failed. (%d)\n", __func__, rc);
			pm_qos_remove_request(&drvdata->pm_qos);
		}
	}
#endif
	return rc;
}
