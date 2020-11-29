ccflags-y += -I$(src)			# needed for trace events
ccflags-y += -Wformat           # for checking printk bug

obj-$(CONFIG_ANDROID_BINDER_IPC)	+= binder.o binder_alloc.o
obj-$(CONFIG_ANDROID_BINDER_IPC_SELFTEST) += binder_alloc_selftest.o
