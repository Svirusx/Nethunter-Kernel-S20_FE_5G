obj-$(CONFIG_SEC_DEBUG)			+= sec_debug.o \
					   sec_debug_node.o \
					   sec_debug_partition.o \
					   sec_hw_param.o \
					   sec_key_notifier.o \
					   sec_crashkey.o \
					   sec_crashkey_long.o

obj-$(CONFIG_SEC_LOG_BUF)		+= sec_log_buf.o \
					   sec_init_log.o

obj-$(CONFIG_SEC_DEBUG_SCHED_LOG)	+= sec_debug_sched_log.o
__SEC_DEBUG_SUMMARY_BUILD_ROOT	:= $(shell echo $(srctree) | sed -e 's|/android/kernel/.\+$$||' -e 's|^.\+/||')
ccflags-$(CONFIG_SEC_DEBUG_SUMMARY)	+= -D__SEC_DEBUG_SUMMARY_BUILD_ROOT=$(__SEC_DEBUG_SUMMARY_BUILD_ROOT)

obj-$(CONFIG_SEC_DEBUG_SUMMARY)		+= sec_debug_summary.o sec_debug_summary_coreinfo.o

obj-$(CONFIG_SEC_DEBUG_FORCE_ERROR)	+= sec_debug_force_err.o

obj-$(CONFIG_SEC_DEBUG_PWDT)		+= sec_debug_pwdt.o

obj-$(CONFIG_SEC_BOOTSTAT)		+= sec_bootstat.o

obj-$(CONFIG_SEC_USER_RESET_DEBUG)	+= sec_debug_user_reset.o

obj-$(CONFIG_KERNEL_MODE_NEON_DEBUG)	+= sec_kernel_mode_neon_debug.o

obj-$(CONFIG_SEC_SLUB_DEBUG)		+= sec_slub_debug.o
