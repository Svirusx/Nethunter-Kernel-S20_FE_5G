#
# Makefile for the Novatek NT36xxx touchscreen driver.
#

# Each configuration option enables a list of files.
ifeq ($(CONFIG_SAMSUNG_PRODUCT_SHIP), y)
	obj-$(CONFIG_TOUCHSCREEN_NOVATEK_NT36523) += nt36xxx.o nt36xxx_fw_update.o nt36xxx_sec_fn.o
else
	obj-$(CONFIG_TOUCHSCREEN_NOVATEK_NT36523) += nt36xxx.o nt36xxx_fw_update.o nt36xxx_sec_fn.o nvt_ts_only_vendor.o
endif
ccflags-y += -Wformat
