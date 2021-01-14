/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 */

#if defined(CONFIG_QTI_RPM_STATS_LOG)

void msm_rpmh_master_stats_update(void);

#else

static inline void msm_rpmh_master_stats_update(void) {}

#endif

#ifdef CONFIG_DSP_SLEEP_RECOVERY
extern void subsystem_update_sleep_time(char *annotation,
	char *rpmh_master_name, uint64_t accumulated_duration);
extern void subsystem_monitor_sleep_issue(void);
#endif
