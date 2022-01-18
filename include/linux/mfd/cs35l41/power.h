/*
 * power.h - Power-management defines for Cirrus Logic CS35L41 amplifier
 *
 * Copyright 2018 Cirrus Logic
 *
 * Author:	David Rhodes	<david.rhodes@cirrus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define CIRRUS_PWR_CSPL_PASSPORT_ENABLE		0x2800394
#define CIRRUS_PWR_CSPL_OUTPUT_POWER_SQ		0x2800398

#define CIRRUS_PWR_NUM_ATTRS_BASE	6
#define CIRRUS_PWR_NUM_ATTRS_AMP	5

void cirrus_pwr_start(const char *mfd_suffix);
void cirrus_pwr_stop(const char *mfd_suffix);
int cirrus_pwr_amp_add(struct regmap *regmap_new, const char *mfd_suffix,
					const char *dsp_part_name);
int cirrus_pwr_set_params(bool global_enable, const char *mfd_suffix,
			unsigned int target_temp, unsigned int exit_temp);
int cirrus_pwr_init(struct class *cirrus_amp_class, int num_amps,
					const char **mfd_suffixes);
void cirrus_pwr_exit(void);
