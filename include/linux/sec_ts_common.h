#ifndef	__SEC_TS_COMMON_H__
#define __SEC_TS_COMMON_H__

#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
struct tsp_dump_callbacks {
	void (*inform_dump)(void);
};
#endif

#endif /* __SEC_TS_COMMON_H__ */
