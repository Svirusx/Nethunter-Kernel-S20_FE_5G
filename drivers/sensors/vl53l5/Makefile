#
# Copyright (c) 2020, STMicroelectronics - All Rights Reserved
# This file is part of VL53L5 Kernel Driver and is dual licensed,
# either 'STMicroelectronics Proprietary license'
# or 'BSD 3-clause "New" or "Revised" License' , at your option.
#
# 'STMicroelectronics Proprietary license'
#
# License terms: STMicroelectronics Proprietary in accordance with licensing
# terms at www.st.com/sla0081
#
# STMicroelectronics confidential
# Reproduction and Communication of this document is strictly prohibited unless
# specifically authorized in writing by STMicroelectronics.
#
# Alternatively, VL53L5 Kernel Driver may be distributed under the terms of
# 'BSD 3-clause "New" or "Revised" License', in which case the following
# provisions apply instead of the ones mentioned above :
#
# License terms: BSD 3-clause "New" or "Revised" License.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
# may be used to endorse or promote products derived from this software
# without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.



# Parameters
# Configure the build type : RELEASE, DEBUG
BUILD_TYPE = RELEASE
# Specify the comms type : SPI, I2C
COMMS_TYPE = SPI
# Specify the I2C bus adapter number for use with COMMS_TYPE=I2C
I2C_ADAPTER_NUM := 1
# Specify the SPI comms speed in KHz for use with COMMS_TYPE=SPI
COMMS_SPEED_KHZ := 7000
# Specify the GPIO pin number for SPI chip select.
GPIO_CHIP_SELECT := 2
# Specify the GPIO pin number for VL53L5 comms select. Set to -1 to ignore.
GPIO_COMMS_SELECT := 23
# Specify the GPIO pin number for VL53L5 power enable. Set to -1 to ignore.
GPIO_POWER_ENABLE := 12
# Specify the GPIO pin number for VL53L5 low power. Set to -1 to ignore.
# Note: Low power mode is not currently available. However, this pin must be
#       set high in order to enable power.
GPIO_LOW_POWER := 19

# Sets the servicing mode to use i.e. INTERRUPT, WORKER or DEFAULT
RANGE_SERVICING = DEFAULT

# Set to TRUE to enable TCDM dump
TCDM_DUMP_ENABLE = FALSE

# Set to TRUE to enable offset calibration
OFFSET_CAL_ENABLE = FALSE

# Set to TRUE to enable code for internal testing
LEGACY_CODE = FALSE

# Set to TRUE to enable logging functions.
LOG_ENABLE = TRUE

# Set to TRUE to enable stress test functions.
TEST_ENABLE = FALSE

ifeq "$(RANGE_SERVICING)" "INTERRUPT"
GPIO_INTERRUPT := 16
else
GPIO_INTERRUPT := -1
endif

# Set to TRUE to enable logging functions.
LOG_ENABLE = TRUE

ifeq "$(LOG_ENABLE)" "TRUE"
  $(warning LOG_ENABLE=$(LOG_ENABLE))
  ccflags-y += -DVL53L5_KERNEL_LOG
endif

KDIR = /lib/modules/$(shell uname -r)/build

ccflags-y	+= -I$(src)/inc
ccflags-y	+= -I$(src)/bare_driver/common/inc
ccflags-y	+= -I$(src)/bare_driver/api/inc
ccflags-y	+= -I$(src)/bare_driver/dci/inc
ccflags-y	+= -I$(src)/platform/inc
ccflags-y	+= -Wall -Werror -Wno-missing-braces

ifeq "$(RANGE_SERVICING)" "INTERRUPT"
ccflags-y += -DVL53L5_INTERRUPT
endif

ifeq "$(OFFSET_CAL_ENABLE)" "TRUE"
  $(warning OFFSET_CAL_ENABLE=$(OFFSET_CAL_ENABLE))
  ccflags-y += -DVL53L5_OFFSET_CAL
endif

ifeq "$(RANGE_SERVICING)" "WORKER"
ccflags-y += -DVL53L5_WORKER
endif

ifeq "$(BUILD_TYPE)" "DEBUG"
  $(warning BUILD_TYPE=$(BUILD_TYPE))
  ccflags-y += -DDEBUG
endif

ifeq "$(BUILD_TYPE)" "RELEASE"
  $(warning BUILD_TYPE=$(BUILD_TYPE))
endif


KERNEL_DRIVER_OBJS = \
			src/vl53l5_k_ioctl_controls.o \
			src/vl53l5_k_gpio_utils.o \
			src/vl53l5_k_error_converter.o \
			src/vl53l5_k_range_wait_handler.o \
			src/vl53l5_k_range_rotation.o

PLATFORM_OBJS =		platform/src/vl53l5_platform.o \
			platform/src/vl53l5_platform_init.o \
			platform/src/vl53l5_platform_log.o

BARE_DRIVER_OBJS = \
			bare_driver/api/src/vl53l5_api_core.o \
			bare_driver/api/src/vl53l5_api_power.o \
			bare_driver/api/src/vl53l5_api_ranging.o \
			bare_driver/api/src/vl53l5_api_range_decode.o \
			bare_driver/dci/src/vl53l5_core_decode.o\
			bare_driver/dci/src/vl53l5_calibration_decode.o\
			bare_driver/dci/src/vl53l5_dci_core.o \
			bare_driver/dci/src/vl53l5_dci_decode.o \
			bare_driver/dci/src/vl53l5_dci_helpers.o \
			bare_driver/dci/src/vl53l5_dci_ranging.o \
			bare_driver/dci/src/vl53l5_dci_utils.o \
			bare_driver/dci/src/vl53l5_decode_switch.o\
			bare_driver/dci/src/vl53l5_results_decode.o \
			bare_driver/common/src/vl53l5_checks.o \
			bare_driver/common/src/vl53l5_commands.o \
			bare_driver/common/src/vl53l5_error_handler.o \
			bare_driver/common/src/vl53l5_load_firmware.o \
			bare_driver/common/src/vl53l5_register_utils.o \
			bare_driver/common/src/vl53l5_rom_boot.o

SIMULATION_OBJS =	simulation/wrapper/vl53l5_api.o

ifeq "$(RANGE_SERVICING)" "INTERRUPT"
  $(warning RANGE_SERVICING=$(RANGE_SERVICING))
  ccflags-y	+= -DVL53L5_INTERRUPT
  KERNEL_DRIVER_OBJS += src/vl53l5_k_interrupt.o
endif

ifeq "$(RANGE_SERVICING)" "WORKER"
  $(warning RANGE_SERVICING=$(RANGE_SERVICING))
  ccflags-y	+= -DVL53L5_WORKER
  KERNEL_DRIVER_OBJS += src/vl53l5_k_work_handler.o
endif

ifeq "$(LOG_ENABLE)" "TRUE"
  $(warning LOG_ENABLE=$(LOG_ENABLE))
  ccflags-y += -DVL53L5_KERNEL_LOG

endif

ifeq "$(TCDM_DUMP_ENABLE)" "TRUE"
  $(warning TCDM_DUMP_ENABLE=$(TCDM_DUMP_ENABLE))
  ccflags-y += -DVL53L5_TCDM_DUMP
  BARE_DRIVER_OBJS += bare_driver/common/src/vl53l5_tcdm_dump.o
endif

ifeq "$(LEGACY_CODE)" "TRUE"
  $(warning LEGACY_CODE=$(LEGACY_CODE))
  ccflags-y += -DSTM_VL53L5_SUPPORT_LEGACY_CODE
endif

ifeq "$(TEST_ENABLE)" "TRUE"
  $(warning TEST_ENABLE=$(TEST_ENABLE))
  ccflags-y += -DVL53L5_TEST_ENABLE

endif

MODULE := vl53l5

obj-$(CONFIG_SENSORS_VL53L5)	+= $(MODULE).o
$(MODULE)-objs		:= src/vl53l5_k_module.o

# # Kernel level objects
$(MODULE)-objs	+= 	$(KERNEL_DRIVER_OBJS)

# Bare driver objects
$(MODULE)-objs +=	$(BARE_DRIVER_OBJS)
# Bare driver platform objects
$(MODULE)-objs +=	$(PLATFORM_OBJS)

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

# Added any pre-insertion setup to this target. The primary usage is to unexport
# GPIO's from userspace so they are free for the kernel.
setup:
	-sudo echo ${GPIO_POWER_ENABLE} > /sys/class/gpio/unexport
	-sudo echo ${GPIO_LOW_POWER} > /sys/class/gpio/unexport
	-sudo echo ${GPIO_COMMS_SELECT} > /sys/class/gpio/unexport
ifeq "$(RANGE_SERVICING)" "INTERRUPT"
	-sudo echo ${GPIO_INTERRUPT} > /sys/class/gpio/unexport
endif

# Added any post-removal reset to this target. The primary usage is to export
# GPIO's to userspace after they are no longer required by the kernel so as to
# allow any userspace drivers/applications to interface to the device.
reset:
	-sudo echo ${GPIO_POWER_ENABLE} > /sys/class/gpio/export
	-sudo echo ${GPIO_LOW_POWER} > /sys/class/gpio/export
	-sudo echo ${GPIO_COMMS_SELECT} > /sys/class/gpio/export
ifeq "$(RANGE_SERVICING)" "INTERRUPT"
	-sudo echo ${GPIO_INTERRUPT} > /sys/class/gpio/export
endif

# Insert the module using insmod with the configured module parameters
insert:
	sudo insmod $(MODULE).ko

dtb:
	dtc -I dts -O dtb -o $(MODULE).dtb $(MODULE).dts
	sudo cp $(MODULE).dtb /boot/overlays/

clean_dtb:
	sudo rm $(MODULE).dtb
	sudo rm /boot/overlays/$(MODULE).dtb
# Remove the module using rmmod
remove:
	-sudo rmmod $(MODULE)
