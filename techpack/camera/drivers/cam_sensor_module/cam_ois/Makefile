# SPDX-License-Identifier: GPL-2.0-only

ccflags-y += -I$(srctree)/techpack/camera/drivers/cam_utils
ccflags-y += -I$(srctree)/techpack/camera/drivers/cam_cpas/include
ccflags-y += -I$(srctree)/techpack/camera/drivers/cam_sensor_module/cam_sensor_io
ccflags-y += -I$(srctree)/techpack/camera/drivers/cam_sensor_module/cam_res_mgr
ccflags-y += -I$(srctree)/techpack/camera/drivers/cam_sensor_module/cam_sensor_utils
ccflags-y += -I$(srctree)/techpack/camera/drivers/cam_req_mgr
ccflags-y += -I$(srctree)/techpack/camera/drivers/cam_sensor_module/cam_cci
ccflags-y += -I$(srctree)/techpack/camera/drivers/cam_smmu
ccflags-y += -I$(srctree)/techpack/camera/drivers/cam_core/
ccflags-y += -I$(srctree)/techpack/camera/drivers/cam_sensor_module/cam_eeprom

obj-$(CONFIG_SPECTRA_CAMERA) += cam_ois_dev.o cam_ois_core.o cam_ois_soc.o
obj-$(CONFIG_SAMSUNG_OIS_MCU_STM32) += cam_ois_mcu_stm32g.o cam_ois_thread.o
obj-$(CONFIG_SAMSUNG_OIS_RUMBA_S4) += cam_ois_rumba_s4.o cam_ois_thread.o