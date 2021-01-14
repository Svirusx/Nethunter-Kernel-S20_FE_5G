# SPDX-License-Identifier: GPL-2.0

define __sec_dtbo_build
dtbo-$(2) += $(1)
$(1)-base := $(3)
endef

define sec_dtbo_build
$(foreach dtbo, $(1), $(eval $(call __sec_dtbo_build, $(dtbo),$(2),$(3))))
endef

# KONA BASE DTB
QCOM_DTB := ../../vendor/qcom
SEC_KONA_BASE_DTB := $(QCOM_DTB)/kona.dtb $(QCOM_DTB)/kona-v2.dtb $(QCOM_DTB)/kona-v2.1.dtb

ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)

# GTS7XL EUR
SEC_GTS7XL_EUR_DTBO := \
		kona-sec-gts7xl-eur-overlay-r00.dtbo \
		kona-sec-gts7xl-eur-overlay-r01.dtbo \
		kona-sec-gts7xl-eur-overlay-r02.dtbo \
		kona-sec-gts7xl-eur-overlay-r03.dtbo \
		kona-sec-gts7xl-eur-overlay-r04.dtbo \
		kona-sec-gts7xl-eur-overlay-r05.dtbo \
		kona-sec-gts7xl-eur-overlay-r06.dtbo

# GTS7XLWIFI EUR
SEC_GTS7XLWIFI_EUR_DTBO := \
		kona-sec-gts7xlwifi-eur-overlay-r02.dtbo \
		kona-sec-gts7xlwifi-eur-overlay-r03.dtbo \
		kona-sec-gts7xlwifi-eur-overlay-r04.dtbo \
		kona-sec-gts7xlwifi-eur-overlay-r05.dtbo \
		kona-sec-gts7xlwifi-eur-overlay-r06.dtbo

# GTS7XL KOR
SEC_GTS7XL_KOR_DTBO := \
		kona-sec-gts7xl-kor-overlay-r01.dtbo \
		kona-sec-gts7xl-kor-overlay-r02.dtbo \
		kona-sec-gts7xl-kor-overlay-r03.dtbo \
		kona-sec-gts7xl-kor-overlay-r04.dtbo \
		kona-sec-gts7xl-kor-overlay-r05.dtbo \
		kona-sec-gts7xl-kor-overlay-r06.dtbo

# GTS7XL USA
SEC_GTS7XL_USA_DTBO := \
		kona-sec-gts7xl-usa-overlay-r01.dtbo \
		kona-sec-gts7xl-usa-overlay-r02.dtbo \
		kona-sec-gts7xl-usa-overlay-r03.dtbo \
		kona-sec-gts7xl-usa-overlay-r04.dtbo \
		kona-sec-gts7xl-usa-overlay-r05.dtbo \
		kona-sec-gts7xl-usa-overlay-r06.dtbo

$(eval $(call sec_dtbo_build, \
		$(SEC_GTS7XL_EUR_DTBO),$(CONFIG_MACH_GTS7XL_EUR_OPEN),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_GTS7XLWIFI_EUR_DTBO),$(CONFIG_MACH_GTS7XLWIFI_EUR_OPEN),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_GTS7XL_KOR_DTBO),$(CONFIG_MACH_GTS7XL_KOR_OPEN),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_GTS7XL_USA_DTBO),$(CONFIG_MACH_GTS7XL_USA_SINGLE),$(SEC_KONA_BASE_DTB)))

endif
