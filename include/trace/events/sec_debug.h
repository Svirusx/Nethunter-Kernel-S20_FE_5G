#undef TRACE_SYSTEM
#define TRACE_SYSTEM sec_debug

#if !defined(_TRACE_SEC_DEBUG_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_SEC_DEBUG_H

#include <linux/tracepoint.h>

DECLARE_EVENT_CLASS(timer_log,

	TP_PROTO(int int_lock, void *fn, int type),

	TP_ARGS(int_lock, fn, type),

	TP_STRUCT__entry(
		__field(	int,	int_lock)
		__field(	void *,	fn	)
		__field(	int,	type	)
	),

	TP_fast_assign(
		__entry->int_lock	= int_lock;
		__entry->fn	= fn;
		__entry->type	= type;
	),

	TP_printk("type=%d fn=%pf int_lock=%d",
		__entry->type, __entry->fn, __entry->int_lock)
);
DEFINE_EVENT(timer_log, timer_log_entry,

	TP_PROTO(int int_lock, void *fn, int type),

	TP_ARGS(int_lock, fn, type)
);
DEFINE_EVENT(timer_log, timer_log_exit,

	TP_PROTO(int int_lock, void *fn, int type),

	TP_ARGS(int_lock, fn, type)
);

TRACE_EVENT(secure_log,

	TP_PROTO(u32 svc_id, u32 cmd_id),

	TP_ARGS(svc_id, cmd_id),

	TP_STRUCT__entry(
		__field(	int,	svc_id	)
		__field(	int,	cmd_id	)
	),

	TP_fast_assign(
		__entry->svc_id	= svc_id;
		__entry->cmd_id	= cmd_id;
	),

	TP_printk("svc_id=%u cmd_id=%u",
		  __entry->svc_id, __entry->cmd_id)
);

TRACE_EVENT(msm_rtb_log,

	TP_PROTO(unsigned char type, uint64_t caller, uint64_t data),

	TP_ARGS(type, caller, data),

	TP_STRUCT__entry(
		__field(unsigned char, log_type)
		__field(uint64_t, caller)
		__field(uint64_t, data)
	),

	TP_fast_assign(
		__entry->log_type	= type;
		__entry->caller	= caller;
		__entry->data	= data;
	),

#ifdef __aarch64__
	TP_printk("log_type=%d caller=%pf data=0x%llx",
		__entry->log_type , (void *)__entry->caller, __entry->data)
#else
	TP_printk("log_type=%d caller=0x%llx data=0x%llx",
		__entry->log_type , __entry->caller, __entry->data)
#endif
);

#endif
/* This part must be outside protection */
#include <trace/define_trace.h>
