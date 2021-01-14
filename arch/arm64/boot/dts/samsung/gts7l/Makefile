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

# GTS7L EUR
SEC_GTS7L_EUR_DTBO := \
		kona-sec-gts7l-eur-overlay-r00.dtbo \
		kona-sec-gts7l-eur-overlay-r01.dtbo \
		kona-sec-gts7l-eur-overlay-r02.dtbo \
		kona-sec-gts7l-eur-overlay-r03.dtbo \
		kona-sec-gts7l-eur-overlay-r04.dtbo \
		kona-sec-gts7l-eur-overlay-r05.dtbo \
		kona-sec-gts7l-eur-overlay-r06.dtbo \
		kona-sec-gts7l-eur-overlay-r07.dtbo

# GTS7L WIFI EUR
SEC_GTS7LWIFI_EUR_DTBO := \
		kona-sec-gts7lwifi-eur-overlay-r02.dtbo \
		kona-sec-gts7lwifi-eur-overlay-r03.dtbo \
		kona-sec-gts7lwifi-eur-overlay-r04.dtbo \
		kona-sec-gts7lwifi-eur-overlay-r05.dtbo \
		kona-sec-gts7lwifi-eur-overlay-r06.dtbo \
		kona-sec-gts7lwifi-eur-overlay-r07.dtbo

# GTS7L KOR
SEC_GTS7L_KOR_DTBO := \
		kona-sec-gts7l-kor-overlay-r00.dtbo \
		kona-sec-gts7l-kor-overlay-r01.dtbo \
		kona-sec-gts7l-kor-overlay-r02.dtbo \
		kona-sec-gts7l-kor-overlay-r03.dtbo \
		kona-sec-gts7l-kor-overlay-r04.dtbo \
		kona-sec-gts7l-kor-overlay-r05.dtbo \
		kona-sec-gts7l-kor-overlay-r06.dtbo \
		kona-sec-gts7l-kor-overlay-r07.dtbo

# GTS7L USA
SEC_GTS7L_USA_DTBO := \
		kona-sec-gts7l-usa-overlay-r02.dtbo \
		kona-sec-gts7l-usa-overlay-r03.dtbo \
		kona-sec-gts7l-usa-overlay-r04.dtbo \
		kona-sec-gts7l-usa-overlay-r05.dtbo \
		kona-sec-gts7l-usa-overlay-r06.dtbo \
		kona-sec-gts7l-usa-overlay-r07.dtbo

$(eval $(call sec_dtbo_build, \
		$(SEC_GTS7L_EUR_DTBO),$(CONFIG_MACH_GTS7L_EUR_OPEN),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_GTS7LWIFI_EUR_DTBO),$(CONFIG_MACH_GTS7LWIFI_EUR_OPEN),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_GTS7L_KOR_DTBO),$(CONFIG_MACH_GTS7L_KOR_OPEN),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_GTS7L_USA_DTBO),$(CONFIG_MACH_GTS7L_USA_SINGLE),$(SEC_KONA_BASE_DTB)))

endif
