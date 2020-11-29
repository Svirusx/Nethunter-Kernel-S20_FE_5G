
/*
* Copyright (c) 2020, STMicroelectronics - All Rights Reserved
*
* This file is part of VL53L5 Kernel Driver and is dual licensed,
* either 'STMicroelectronics Proprietary license'
* or 'BSD 3-clause "New" or "Revised" License' , at your option.
*
********************************************************************************
*
* 'STMicroelectronics Proprietary license'
*
********************************************************************************
*
* License terms: STMicroelectronics Proprietary in accordance with licensing
* terms at www.st.com/sla0081
*
* STMicroelectronics confidential
* Reproduction and Communication of this document is strictly prohibited unless
* specifically authorized in writing by STMicroelectronics.
*
*
********************************************************************************
*
* Alternatively, VL53L5 Kernel Driver may be distributed under the terms of
* 'BSD 3-clause "New" or "Revised" License', in which case the following
* provisions apply instead of the ones mentioned above :
*
********************************************************************************
*
* License terms: BSD 3-clause "New" or "Revised" License.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its contributors
* may be used to endorse or promote products derived from this software
* without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*
********************************************************************************
*
*/
#include <linux/slab.h>
#include "vl53l5_k_range_rotation.h"
#include "vl53l5_k_logging.h"
#include "vl53l5_k_error_codes.h"
#include "vl53l5_k_user_data.h"
#include "vl53l5_api_ranging.h"
#include "vl53l5_results_build_config.h"

static int vl53l5_k_rotation_init_zone_order_coeffs(
	int no_of_zones,
	int rotation_sel,
	struct vl53l5_k_rotation__coeffs_t  *pdata)
{

	int status = 0;
	int grid_size = 0;

	switch (no_of_zones) {
	case 64:
		grid_size = 8;
		break;
	case 16:
		grid_size = 4;
		break;
	default:
		vl53l5_k_log_error("Invalid Num Zones: %d\n",
			no_of_zones);
		status = VL53L5_K_ERROR_INVALID_NO_OF_ZONES;
		break;
	}

	if (status != 0) {
		goto exit;
	}

	pdata->no_of_zones = no_of_zones;
	pdata->grid_size = grid_size;

	switch (rotation_sel) {
	case VL53L5_K_ROTATION_SEL__NONE:
		pdata->coeff__col_m =  0;
		pdata->coeff__col_x =  1;
		pdata->coeff__row_m =  0;
		pdata->coeff__row_x =  grid_size;
		break;
	case VL53L5_K_ROTATION_SEL__CW_90:
		pdata->coeff__col_m = (grid_size - 1) * grid_size;
		pdata->coeff__col_x =  -grid_size;
		pdata->coeff__row_m =  0;
		pdata->coeff__row_x =  1;
		break;
	case VL53L5_K_ROTATION_SEL__CW_180:
		pdata->coeff__col_m =  0;
		pdata->coeff__col_x = -1;
		pdata->coeff__row_m = (grid_size * grid_size) - 1;
		pdata->coeff__row_x =  -grid_size;
		break;
	case VL53L5_K_ROTATION_SEL__CW_270:
		pdata->coeff__col_m =  0;
		pdata->coeff__col_x =  grid_size;
		pdata->coeff__row_m =  grid_size - 1;
		pdata->coeff__row_x =  -1;
		break;
	case VL53L5_K_ROTATION_SEL__MIRROR_X:
		pdata->coeff__col_m =  grid_size-1;
		pdata->coeff__col_x = -1;
		pdata->coeff__row_m =  0;
		pdata->coeff__row_x =  grid_size;
		break;
	case VL53L5_K_ROTATION_SEL__MIRROR_Y:
		pdata->coeff__col_m =  0;
		pdata->coeff__col_x =  1;
		pdata->coeff__row_m = (grid_size - 1) * grid_size;
		pdata->coeff__row_x =  -grid_size;
		break;
	default:
		vl53l5_k_log_error("Invalid: %d\n",
			rotation_sel);
		status = VL53L5_K_ERROR_INVALID_ROTATION;
		break;
	}

exit:
	return status;
}

static int vl53l5_k_rotation_sequence_idx(
	int zone_id,
	struct vl53l5_k_rotation__coeffs_t *pdata)
{

	int r = 0U;
	int c = 0U;
	int sequence_idx = 0U;

	if (zone_id < 0 || zone_id > pdata->no_of_zones) {
		sequence_idx = -1;
		vl53l5_k_log_error("illegal zone_id %5d\n", zone_id);
		goto exit;
	}

	r = zone_id / pdata->grid_size;
	c = zone_id % pdata->grid_size;

	sequence_idx =  ((r * pdata->coeff__row_x) + pdata->coeff__row_m);
	sequence_idx += ((c * pdata->coeff__col_x) + pdata->coeff__col_m);

exit:
	return sequence_idx;
}

static void vl53l5_k_output_target_filter(
	int iz,
	int oz,
	unsigned int tgt_status_op_enables,
	struct vl53l5_range_results_t *p_results,
	struct vl53l5_range_results_t *p_copy)
{

	int i = 0;

	int t = 0;

	int it = 0;

	int ot = 0;

	int max_targets = (int)p_copy->common_data.rng__max_targets_per_zone;
	unsigned short wrap_dmax_mm =
		p_copy->common_data.wrap_dmax_mm;
	unsigned short system_dmax_mm =
		p_copy->per_zone_results.amb_dmax_mm[iz];
	unsigned int tgt_op_enable  = 0U;

	int no_of_targets =
		(int)p_copy->per_zone_results.rng__no_of_targets[iz];

	if (wrap_dmax_mm < system_dmax_mm) {
		system_dmax_mm = wrap_dmax_mm;
	}

	for (i = 0; i < max_targets; i++) {
		ot =  (oz * max_targets) + i;
		if (i == 0) {
			p_results->per_tgt_results.median_range_mm[ot] =
				(short)(system_dmax_mm << 2U);
		} else {
			p_results->per_tgt_results.median_range_mm[ot] = 0U;
		}
		p_results->per_tgt_results.target_status[ot] =
			DCI_TARGET_STATUS__NOUPDATE;
	}

	t = 0;
	for (i = 0; i < no_of_targets; i++) {

		it = (iz * max_targets) + i;

		tgt_op_enable = 1U <<
			(unsigned int)p_copy->per_tgt_results.target_status[it];

		if ((tgt_op_enable & tgt_status_op_enables) == 0)
			continue;

		ot = (oz * max_targets) + t;

#ifdef VL53L5_PEAK_RATE_KCPS_PER_SPAD_ON
		p_results->per_tgt_results.peak_rate_kcps_per_spad[ot] =
			p_copy->per_tgt_results.peak_rate_kcps_per_spad[it];
#endif
#ifdef VL53L5_MEDIAN_PHASE_ON
		p_results->per_tgt_results.median_phase[ot] =
			p_copy->per_tgt_results.median_phase[it];
#endif
#ifdef VL53L5_RATE_SIGMA_KCPS_PER_SPAD_ON
		p_results->per_tgt_results.rate_sigma_kcps_per_spad[ot] =
			p_copy->per_tgt_results.rate_sigma_kcps_per_spad[it];
#endif
#ifdef VL53L5_RANGE_SIGMA_MM_ON
		p_results->per_tgt_results.range_sigma_mm[ot] =
			p_copy->per_tgt_results.range_sigma_mm[it];
#endif
#ifdef VL53L5_MEDIAN_RANGE_MM_ON
		p_results->per_tgt_results.median_range_mm[ot] =
			p_copy->per_tgt_results.median_range_mm[it];
#endif
#ifdef VL53L5_MIN_RANGE_DELTA_MM_ON
		p_results->per_tgt_results.min_range_delta_mm[ot] =
			p_copy->per_tgt_results.min_range_delta_mm[it];
#endif
#ifdef VL53L5_MAX_RANGE_DELTA_MM_ON
		p_results->per_tgt_results.max_range_delta_mm[ot] =
			p_copy->per_tgt_results.max_range_delta_mm[it];
#endif
#ifdef VL53L5_TARGET_REFLECTANCE_EST_PC_ON
		p_results->per_tgt_results.target_reflectance_est_pc[ot] =
			p_copy->per_tgt_results.target_reflectance_est_pc[it];
#endif
#ifdef VL53L5_TARGET_STATUS_ON
		p_results->per_tgt_results.target_status[ot] =
			p_copy->per_tgt_results.target_status[it];
#endif
		t++;
	}

	p_results->per_zone_results.rng__no_of_targets[oz] = (char)t;
}

int vl53l5_k_range_rotation(
	int rotation_sel,
	unsigned int tgt_status_op_enables,
	struct vl53l5_range_results_t *p_results)
{

	int status = STATUS_OK;
	int no_of_zones = 0;
	int iz = 0;

	int oz = 0;

	struct vl53l5_range_results_t *p_copy;
	struct vl53l5_k_rotation__coeffs_t coeffs = {0};
	struct vl53l5_k_rotation__coeffs_t *pcoeffs = &coeffs;

	no_of_zones = (int)p_results->common_data.rng__no_of_zones;

	p_copy = kzalloc(sizeof(struct vl53l5_range_results_t), GFP_KERNEL);
	if (p_copy == NULL) {
		vl53l5_k_log_error("Allocate Failed");
		status = VL53L5_K_ERROR_FAILED_TO_ALLOCATE_RANGE_DATA;
		goto out;
	}

	status = vl53l5_k_rotation_init_zone_order_coeffs(
			no_of_zones,
			rotation_sel,
			pcoeffs);
	if (status != STATUS_OK) {
		goto out_free;
	}

	memcpy(p_copy, p_results, sizeof(struct vl53l5_range_results_t));

	for (oz = 0; oz < no_of_zones; oz++) {

		iz = vl53l5_k_rotation_sequence_idx(oz, pcoeffs);
		if (iz < 0) {
			status = -1;
			goto out_free;
		}

#ifdef VL53L5_AMB_RATE_KCPS_PER_SPAD_ON
		p_results->per_zone_results.amb_rate_kcps_per_spad[oz] =
			p_copy->per_zone_results.amb_rate_kcps_per_spad[iz];
#endif
#ifdef VL53L5_EFFECTIVE_SPAD_COUNT_ON
		p_results->per_zone_results.rng__effective_spad_count[oz] =
			p_copy->per_zone_results.rng__effective_spad_count[iz];
#endif
#ifdef VL53L5_AMB_DMAX_MM_ON
		p_results->per_zone_results.amb_dmax_mm[oz] =
			p_copy->per_zone_results.amb_dmax_mm[iz];
#endif
#ifdef VL53L5_SILICON_TEMP_DEGC_START_ON
		p_results->per_zone_results.silicon_temp_degc__start[oz] =
			p_copy->per_zone_results.silicon_temp_degc__start[iz];
#endif
#ifdef VL53L5_SILICON_TEMP_DEGC_END_ON
		p_results->per_zone_results.silicon_temp_degc__end[oz] =
			p_copy->per_zone_results.silicon_temp_degc__end[iz];
#endif
#ifdef VL53L5_NO_OF_TARGETS_ON
		p_results->per_zone_results.rng__no_of_targets[oz] =
			p_copy->per_zone_results.rng__no_of_targets[iz];
#endif
#ifdef VL53L5_ZONE_ID_ON
		p_results->per_zone_results.rng__zone_id[oz] =
			p_copy->per_zone_results.rng__zone_id[iz];
#endif
#ifdef VL53L5_SEQUENCE_IDX_ON
		p_results->per_zone_results.rng__sequence_idx[oz] =
			p_copy->per_zone_results.rng__sequence_idx[iz];
#endif

		vl53l5_k_output_target_filter(
			iz, oz, tgt_status_op_enables, p_results, p_copy);
	}

out_free:
	kfree(p_copy);
out:
	return status;
}
