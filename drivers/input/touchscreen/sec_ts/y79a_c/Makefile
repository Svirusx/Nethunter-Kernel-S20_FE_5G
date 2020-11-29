
ifeq ($(CONFIG_SAMSUNG_PRODUCT_SHIP), y)
  obj-$(CONFIG_TOUCHSCREEN_SEC_TS_Y79A_C)   += sec_ts.o sec_ts_fw.o sec_ts_fn.o sec_ts_ioctl.o
  obj-$(CONFIG_TOUCHSCREEN_SEC_TS_Y771_SUB)   += sec_ts.o sec_ts_fw.o sec_ts_fn.o sec_ts_ioctl.o
else
  obj-$(CONFIG_TOUCHSCREEN_SEC_TS_Y79A_C)   += sec_ts.o sec_ts_fw.o sec_ts_fn.o sec_ts_ioctl.o sec_ts_only_vendor.o
  obj-$(CONFIG_TOUCHSCREEN_SEC_TS_Y771_SUB)   += sec_ts.o sec_ts_fw.o sec_ts_fn.o sec_ts_ioctl.o sec_ts_only_vendor.o
endif
ccflags-y += -Wformat
