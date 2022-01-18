/*
 * linux/platform_data/cs40l2x.h -- Platform data for
 * CS40L20/CS40L25/CS40L25A/CS40L25B
 *
 * Copyright 2018 Cirrus Logic, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __CS40L2X_H
#define __CS40L2X_H

struct cs40l2x_platform_data {
	unsigned int boost_ind;
	unsigned int boost_cap;
	unsigned int boost_ipk;
	unsigned int boost_ctl;
	unsigned int boost_ovp;
	unsigned int boost_clab;
	bool refclk_gpio2;
	unsigned int f0_default;
	unsigned int f0_min;
	unsigned int f0_max;
	unsigned int redc_default;
	unsigned int redc_min;
	unsigned int redc_max;
	unsigned int q_default;
	unsigned int q_min;
	unsigned int q_max;
	bool redc_comp_disable;
	bool comp_disable;
	unsigned int gpio1_rise_index;
	unsigned int gpio1_fall_index;
	unsigned int gpio1_fall_timeout;
	unsigned int gpio1_mode;
	unsigned int gpio2_rise_index;
	unsigned int gpio2_fall_index;
	unsigned int gpio3_rise_index;
	unsigned int gpio3_fall_index;
	unsigned int gpio4_rise_index;
	unsigned int gpio4_fall_index;
	unsigned int gpio_indv_enable;
	unsigned int gpio_indv_pol;
	bool hiber_enable;
	unsigned int asp_bclk_freq;
	bool asp_bclk_inv;
	bool asp_fsync_inv;
	unsigned int asp_fmt;
	unsigned int asp_slot_num;
	unsigned int asp_slot_width;
	unsigned int asp_samp_width;
	unsigned int asp_timeout;
	bool vpbr_enable;
	bool vbbr_enable;
	unsigned int vpbr_thld1;
	unsigned int vbbr_thld1;
	unsigned int fw_id_remap;
	bool amp_gnd_stby;
	bool auto_recovery;
#ifdef CONFIG_CS40L2X_SAMSUNG_FEATURE
	bool folder_type;
	unsigned int dig_scale_default;
	unsigned int dig_scale_fc_sd;			/* FOLDER CLOSE,  SHORT DURATION */
	unsigned int dig_scale_fc_ld;			/* FOLDER CLOSE, LONG DURATION */
	unsigned int dig_scale_fo_sd;			/* FOLDER OPEN, SHORT DURATION */
	unsigned int dig_scale_fo_ld;			/* FOLDER OPEN, LONG DURATION */
	unsigned int dig_scale_fo_ld_low_temp;		/* FOLDER OPEN, LONG DURATION, LOW TEMP */
	unsigned int dig_scale_fo_ld_lower_temp;	/* FOLDER OPEN, LONG DURATION, LOWER TEMP */
	unsigned int dig_scale_high_temp;		/* For TOP model, united dig scale for HIGH TEMP */
	int low_temp;
	int lower_temp;
	int high_temp;
#endif
};

#endif /* __CS40L2X_H */
