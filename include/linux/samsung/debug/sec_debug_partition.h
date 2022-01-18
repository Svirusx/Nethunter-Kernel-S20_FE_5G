#ifndef __SEC_DEBUG_PARTITION_INDIRECT
#warning "samsung/debug/sec_debug_partition.h is included directly."
#error "please include sec_debug.h instead of this file"
#endif

#ifndef __INDIRECT__SEC_DEBUG_PARTITION_H__
#define __INDIRECT__SEC_DEBUG_PARTITION_H__

#include "sec_debug_partition_type.h"

#ifdef CONFIG_SEC_DEBUG
extern bool read_debug_partition(enum debug_partition_index index, void *value);
extern bool write_debug_partition(enum debug_partition_index index, void *value);

/* called @ drivers/edac/kryo_arm64_edac.c */
ap_health_t* ap_health_data_read(void);

/* called @ drivers/edac/kryo_arm64_edac.c */
int ap_health_data_write(ap_health_t *data);

/* called @ drivers/edac/kryo_arm64_edac.c */
int dbg_partition_notifier_register(struct notifier_block *nb);

/* called @ sec_debug_user_reset.c */
uint32_t dbg_parttion_get_part_size(uint32_t idx);
#else
static inline bool read_debug_partition(enum debug_partition_index index, void *value) { return true; }
static inline bool write_debug_partition(enum debug_partition_index index, void *value) { return true; }
static inline ap_health_t* ap_health_data_read(void) { return NULL; }
static inline int ap_health_data_write(ap_health_t *data) { return 0; }
static inline int dbg_partition_notifier_register(struct notifier_block *nb) { return 0; }
static inline uint32_t dbg_parttion_get_part_size(uint32_t idx) { return 0; }
#endif

#endif /* __INDIRECT__SEC_DEBUG_PARTITION_H__ */
