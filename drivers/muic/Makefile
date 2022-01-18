#
# Makefile for muic devices
#

subdir-ccflags-y := -Wformat
obj-$(CONFIG_USE_MUIC)			+= muic-core.o
obj-$(CONFIG_MUIC_NOTIFIER)		+= muic_notifier.o
obj-$(CONFIG_MUIC_MAX77804)		+= max77804-muic.o
obj-$(CONFIG_MUIC_MAX77804K)		+= max77804k-muic.o
obj-$(CONFIG_MUIC_MAX77828)		+= max77828-muic.o
obj-$(CONFIG_MUIC_MAX77843)		+= max77843-muic.o
obj-$(CONFIG_HV_MUIC_MAX77843_AFC)	+= max77843-muic-afc.o
obj-$(CONFIG_MUIC_MAX77833)		+= max77833-muic.o
obj-$(CONFIG_HV_MUIC_MAX77833_AFC)  += max77833-muic-afc.o
obj-$(CONFIG_MUIC_MAX77854)		+= max77854-muic.o
obj-$(CONFIG_HV_MUIC_MAX77854_AFC)  += max77854-muic-afc.o
obj-$(CONFIG_MUIC_MAX77888)		+= max77888-muic.o
obj-$(CONFIG_MUIC_MAX77705)		+= max77705-muic.o
obj-$(CONFIG_HV_MUIC_MAX77705_AFC)	+= max77705-muic-afc.o
obj-$(CONFIG_MUIC_MAX77705_CCIC)	+= max77705-muic-ccic.o
obj-$(CONFIG_MUIC_FSA9480)		+= fsa9480.o
obj-$(CONFIG_MUIC_S2MM001)		+= s2mm001.o
obj-$(CONFIG_MUIC_SM5508)		+= sm5508-muic.o
obj-$(CONFIG_MUIC_SUPPORT_KEYBOARDDOCK)	+= keyboard_notifier.o

# MUIC_UNIVERSAL
obj-$(CONFIG_MUIC_UNIVERSAL)		+= universal/
ifeq ($(CONFIG_MUIC_UNIVERSAL_MAX77854), y)
	obj-$(CONFIG_MUIC_HV)	+= universal/muic_hv_max77854.o
endif
ifeq ($(CONFIG_MUIC_UNIVERSAL_SM5720), y)
	obj-$(CONFIG_MUIC_HV)	+= universal/muic_sm5720_afc.o
endif
