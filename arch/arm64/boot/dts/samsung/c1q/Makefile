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

# C1Q USA
SEC_C1Q_USA_DTBO := \
		kona-sec-c1q-usa-overlay-r02.dtbo \
		kona-sec-c1q-usa-overlay-r03.dtbo \
		kona-sec-c1q-usa-overlay-r04.dtbo \
		kona-sec-c1q-usa-overlay-r05.dtbo \
		kona-sec-c1q-usa-overlay-r06.dtbo \
		kona-sec-c1q-usa-overlay-r07.dtbo \
		kona-sec-c1q-usa-overlay-r08.dtbo \
		kona-sec-c1q-usa-overlay-r09.dtbo \
		kona-sec-c1q-usa-overlay-r10.dtbo \
		kona-sec-c1q-usa-overlay-r11.dtbo

# C1Q KOR
SEC_C1Q_KOR_DTBO := \
		kona-sec-c1q-kor-overlay-r02.dtbo \
		kona-sec-c1q-kor-overlay-r06.dtbo \
		kona-sec-c1q-kor-overlay-r07.dtbo \
		kona-sec-c1q-kor-overlay-r08.dtbo \
		kona-sec-c1q-kor-overlay-r09.dtbo \
		kona-sec-c1q-kor-overlay-r10.dtbo \
		kona-sec-c1q-kor-overlay-r11.dtbo

# C1Q CHN
SEC_C1Q_CHN_DTBO := \
		kona-sec-c1q-chn-overlay-r03.dtbo \
		kona-sec-c1q-chn-overlay-r04.dtbo \
		kona-sec-c1q-chn-overlay-r05.dtbo \
		kona-sec-c1q-chn-overlay-r06.dtbo \
		kona-sec-c1q-chn-overlay-r07.dtbo \
		kona-sec-c1q-chn-overlay-r08.dtbo \
		kona-sec-c1q-chn-overlay-r09.dtbo \
		kona-sec-c1q-chn-overlay-r10.dtbo \
		kona-sec-c1q-chn-overlay-r11.dtbo

$(eval $(call sec_dtbo_build, \
		$(SEC_C1Q_USA_DTBO),$(CONFIG_MACH_C1Q_USA_SINGLE),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_C1Q_KOR_DTBO),$(CONFIG_MACH_C1Q_KOR_SINGLE),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_C1Q_CHN_DTBO),$(CONFIG_MACH_C1Q_CHN_OPEN),$(SEC_KONA_BASE_DTB)))

endif
