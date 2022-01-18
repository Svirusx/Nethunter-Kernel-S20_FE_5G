ifeq ($(CONFIG_SAMSUNG_PRODUCT_SHIP), y)
  obj-$(CONFIG_TOUCHSCREEN_SLSI_Y79A)   += slsi_cmd.o slsi_fw.o slsi_fn.o slsi_ioctrl.o slsi_main.o
else
  obj-$(CONFIG_TOUCHSCREEN_SLSI_Y79A)   += slsi_cmd.o slsi_fw.o slsi_fn.o slsi_ioctrl.o slsi_main.o slsi_vendor.o
endif
