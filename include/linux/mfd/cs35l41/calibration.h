/*
 * calibration.h  --  Calibration defines for Cirrus Logic CS35L41 codec
 *
 * Copyright 2017 Cirrus Logic
 *
 * Author:	David Rhodes	<david.rhodes@cirrus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/notifier.h>
#include <sound/soc.h>

#define CS35L41_NG_ENABLE_MASK	0x00010000

/* These values are specific to Playback 5.01.3 */
#define CS35L41_PROTECT_TEMP	0x0280021c
#define CS35L41_CAL_RDC		0x02800224
#define CS35L41_CAL_AMBIENT	0x02800228
#define CS35L41_CAL_STATUS	0x0280022C
#define CS35L41_CAL_CHECKSUM	0x02800230
#define CS35L41_CAL_SET_STATUS	0x02800234
#define CS35L41_YM_CONFIG_ADDR	0x0340002c
#define CIRRUS_PWR_CSPL_I_PEAK	0x0280039c
#define CIRRUS_PWR_CSPL_V_PEAK	0x028003a0

#define CS35L41_HALO_STATE			0x02800050
#define CS35L41_HALO_HEARTBEAT			0x02800054
#define CS35L41_CSPL_STATE_RUNNING	0x00000000
#define CS35L41_CSPL_STATE_ERROR	0x00000001

#define CS35L41_VIMON_CAL_STATUS	0x0280006c
#define CS35L41_VIMON_CAL_VSC		0x02800070
#define CS35L41_VIMON_CAL_ISC		0x02800074
#define CS35L41_VIMON_CAL_CLASSH_DELAY	0x02800080
#define CS35L41_VIMON_CAL_CLASSD_DELAY	0x02800084
#define CS35L41_VBST_CTL_11		0xAA
#define CS35L41_CLASSH_DELAY_50MS	0x32
#define CS35L41_CLASSD_DELAY_50MS	0x32

#define CS35L41_CAL_VIMON_STATUS_INVALID	1
#define CS35L41_CAL_VIMON_STATUS_SUCCESS	2

#define CS35L41_CSPL_STATUS_OUT_OF_RANGE	0x00000003
#define CS35L41_CSPL_STATUS_INCOMPLETE		0x00000002

#define CS35L41_CAL_AMP_CONSTANT	5.85714
#define CS35L41_CAL_AMP_CONSTANT_NUM	292857
#define CS35L41_CAL_AMP_CONSTANT_DENOM	50000
#define CS35L41_CAL_RDC_RADIX		13

#define CIRRUS_CAL_VFS_MV		12300
#define CIRRUS_CAL_IFS_MA		2100

#define CIRRUS_CAL_V_VAL_UB_MV		2000
#define CIRRUS_CAL_V_VAL_LB_MV		50

#define CS35L41_VIMON_CAL_VSC_UB	0x00010624
#define CS35L41_VIMON_CAL_VSC_LB	0x00FEF9DC
#define CS35L41_VIMON_CAL_ISC_UB	0x00004189
#define CS35L41_VIMON_CAL_ISC_LB	0x00FFBE77

#define CS35L41_MPU_UNLOCK_CODE_0		0x5555
#define CS35L41_MPU_UNLOCK_CODE_1		0xaaaa

#ifdef CS35L41_FACTORY_RECOVERY_SYSFS
#define CIRRUS_CAL_NUM_ATTRS_BASE	5
#else
#define CIRRUS_CAL_NUM_ATTRS_BASE	4
#endif

#define CIRRUS_CAL_NUM_ATTRS_AMP	8

static const unsigned int cs35l41_halo_mpu_access[18] = {
	CS35L41_DSP1_MPU_WNDW_ACCESS0,
	CS35L41_DSP1_MPU_XREG_ACCESS0,
	CS35L41_DSP1_MPU_YREG_ACCESS0,
	CS35L41_DSP1_MPU_XM_ACCESS1,
	CS35L41_DSP1_MPU_YM_ACCESS1,
	CS35L41_DSP1_MPU_WNDW_ACCESS1,
	CS35L41_DSP1_MPU_XREG_ACCESS1,
	CS35L41_DSP1_MPU_YREG_ACCESS1,
	CS35L41_DSP1_MPU_XM_ACCESS2,
	CS35L41_DSP1_MPU_YM_ACCESS2,
	CS35L41_DSP1_MPU_WNDW_ACCESS2,
	CS35L41_DSP1_MPU_XREG_ACCESS2,
	CS35L41_DSP1_MPU_YREG_ACCESS2,
	CS35L41_DSP1_MPU_XM_ACCESS3,
	CS35L41_DSP1_MPU_YM_ACCESS3,
	CS35L41_DSP1_MPU_WNDW_ACCESS3,
	CS35L41_DSP1_MPU_XREG_ACCESS3,
	CS35L41_DSP1_MPU_YREG_ACCESS3,
};

int cirrus_cal_apply(const char *mfd_suffix);

#ifdef CS35L41_FACTORY_RECOVERY_SYSFS
int cirrus_cal_component_add(struct snd_soc_component *component, const char *mfd_suffix);
#endif /* CS35L41_FACTORY_RECOVERY_SYSFS */

int cirrus_cal_amp_add(struct regmap *regmap_new, const char *mfd_suffix,
					const char *dsp_part_name,
					bool calibration_disable);
int cirrus_cal_init(struct class *cirrus_amp_class, int num_amps,
					const char **mfd_suffixes,
					bool *v_val_separate);
void cirrus_cal_exit(void);

