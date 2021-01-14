obj-$(CONFIG_B43)		+= b43/
obj-$(CONFIG_B43LEGACY)		+= b43legacy/

obj-$(CONFIG_BRCMFMAC)	+= brcm80211/
obj-$(CONFIG_BRCMSMAC)	+= brcm80211/

ifneq ($(filter c2% c1%, $(TARGET_PRODUCT)), )
ifeq ($(SEC_BUILD_OPTION_HW_REVISION),01)
CONFIG_BCM4389=y
CONFIG_BCM4375=n
endif
endif
$(warning CONFIG_BCM4389 = $(CONFIG_BCM4389))
$(warning CONFIG_BCM4375 = $(CONFIG_BCM4375))

ifeq ($(CONFIG_BCM4389),y)
obj-$(CONFIG_BCM4389)   += bcmdhd_101_10/
obj-$(CONFIG_ARCH_KONA) += bcmdhd_101_10/dhd_custom_memprealloc.o
obj-$(CONFIG_ARCH_KONA) += bcmdhd_101_10/dhd_custom_msm.o
endif

ifeq ($(CONFIG_BCM4375),y)
obj-$(CONFIG_BCM4375)   += bcmdhd_101_16/
obj-$(CONFIG_ARCH_KONA) += bcmdhd_101_16/dhd_custom_memprealloc.o
obj-$(CONFIG_ARCH_KONA) += bcmdhd_101_16/dhd_custom_msm.o
endif