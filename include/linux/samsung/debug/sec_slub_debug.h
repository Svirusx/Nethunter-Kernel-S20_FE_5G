#ifndef __SEC_SLUB_DEBUG_INDIRECT
#warning "samsung/debug/sec_slub_debug.h is included directly."
#error "please include sec_debug.h instead of this file"
#endif

#ifndef __INDIRECT__SEC_SLUB_DEBUG_H__
#define __INDIRECT__SEC_SLUB_DEBUG_H__

#if IS_ENABLED(CONFIG_SEC_SLUB_DEBUG)
/* called @ mm/slub.c*/
extern void sec_slub_debug_save_free_track(struct kmem_cache *s, void *x);

/* called @ mm/slub.c*/
extern void sec_slub_debug_panic_on_fp_corrupted(struct kmem_cache *s, void *x, void *fp);
#else
static inline void sec_slub_debug_save_free_track(struct kmem_cache *s, void *x) {}
static inline void sec_slub_debug_panic_on_fp_corrupted(struct kmem_cache *s, void *x, void *fp) {}
#endif /* CONFIG_SEC_SLUB_DEBUG */

#endif /* __INDIRECT__SEC_SLUB_DEBUG_H__ */
