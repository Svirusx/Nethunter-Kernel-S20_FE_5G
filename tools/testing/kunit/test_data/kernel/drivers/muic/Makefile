#
# Makefile for muic devices
#
subdir-ccflags-y := -Wformat

obj-y					+= muic-core.o
obj-$(CONFIG_MUIC_NOTIFIER)		+= muic_notifier.o
obj-$(CONFIG_MUIC_MAX77705)		+= max77705-muic.o
obj-$(CONFIG_HV_MUIC_MAX77705_AFC)	+= max77705-muic-afc.o
obj-$(CONFIG_MUIC_MAX77705_CCIC)	+= max77705-muic-ccic.o