#
# Makefile for ccic devices
#

subdir-ccflags-y := -Wformat
obj-$(CONFIG_CCIC_NOTIFIER)		+= ccic_notifier.o ccic_sysfs.o ccic_core.o ccic_misc.o
obj-$(CONFIG_CCIC_S2MM003)		+= s2mm003.o
obj-$(CONFIG_CCIC_S2MM005)		+= s2mm005_fw.o s2mm005_cc.o s2mm005_pd.o s2mm005.o s2mm005_alternate.o
obj-$(CONFIG_CCIC_MAX77705)		+= max77705_cc.o max77705_pd.o max77705_usbc.o max77705_alternate.o
obj-$(CONFIG_CCIC_MAX77705_DEBUG)	+= max77705_debug.o
