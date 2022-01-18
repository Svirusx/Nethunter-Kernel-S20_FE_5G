#
# Makefile for the sensors drivers.
#

# Each configuration option enables a list of files.

ccflags-y := $(KBUILD_FP_SENSOR_CFLAGS)
ccflags-y += -Wformat

obj-$(CONFIG_SENSORS_FINGERPRINT)	+= fingerprint_sysfs.o
obj-$(CONFIG_SENSORS_QBT2000)		+= qbt2000_common.o qbt2000_platform.o qbt2000_spidev.o
obj-$(CONFIG_SENSORS_ET5XX)			+= et5xx-spi.o et5xx-spi_data_transfer.o
obj-$(CONFIG_SENSORS_ET7XX)			+= et7xx-spi.o et7xx-spi_data_transfer.o
obj-$(CONFIG_SENSORS_GW3X)			+= gw3x_common.o gw3x_platform.o gw3x_spidev.o
obj-$(CONFIG_SENSORS_GW9558X)		+= gw9558x_common.o gw9558x_platform.o gw9558x_spidev.o