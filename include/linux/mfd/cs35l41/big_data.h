/*
 * big_data.h  --  Big Data defines for Cirrus Logic CS35L41 codec
 *
 * Copyright 2017 Cirrus Logic
 *
 * Author:	David Rhodes	<david.rhodes@cirrus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* These values are specific to Playback 5.00.5 */

#define CS35L41_BD_MAX_TEMP		0x2800380
#define CS35L41_BD_MAX_EXC		0x2800384
#define CS35L41_BD_OVER_TEMP_COUNT	0x2800388
#define CS35L41_BD_OVER_EXC_COUNT	0x280038c
#define CS35L41_BD_ABNORMAL_MUTE	0x2800390

#define CS35L41_BD_TEMP_RADIX		14
#define CS35L41_BD_EXC_RADIX		19

#define CIRRUS_BD_NUM_ATTRS_BASE	1
#define CIRRUS_BD_NUM_ATTRS_AMP		7

struct cirrus_bd_ext {
	unsigned int *max_exc;
	unsigned int *over_exc_count;
	unsigned int *max_temp;
	unsigned int *max_temp_keep;
	unsigned int *over_temp_count;
	unsigned int *abnm_mute;
};

extern struct cirrus_bd_ext cirrus_bd_data;

int cirrus_bd_get_index_from_suffix(const char *suffix);
void cirrus_bd_store_values(const char *mfd_suffix);
int cirrus_bd_amp_add(struct regmap *regmap_new, const char *mfd_suffix,
					const char *dsp_part_name,
					int bd_max_temp);
int cirrus_bd_init(struct class *cirrus_amp_class, int num_amps,
					const char **mfd_suffixes,
					const char **bd_suffixes);
void cirrus_bd_exit(void);
