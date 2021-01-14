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

# C2Q USA
SEC_C2Q_USA_DTBO := \
		kona-sec-c2q-usa-overlay-r00.dtbo \
		kona-sec-c2q-usa-overlay-r01.dtbo \
		kona-sec-c2q-usa-overlay-r03.dtbo \
		kona-sec-c2q-usa-overlay-r04.dtbo \
		kona-sec-c2q-usa-overlay-r06.dtbo \
		kona-sec-c2q-usa-overlay-r07.dtbo \
		kona-sec-c2q-usa-overlay-r08.dtbo \
		kona-sec-c2q-usa-overlay-r09.dtbo \
		kona-sec-c2q-usa-overlay-r10.dtbo

# C2Q KOR
SEC_C2Q_KOR_DTBO := \
		kona-sec-c2q-kor-overlay-r00.dtbo \
		kona-sec-c2q-kor-overlay-r01.dtbo \
		kona-sec-c2q-kor-overlay-r04.dtbo \
		kona-sec-c2q-kor-overlay-r06.dtbo \
		kona-sec-c2q-kor-overlay-r07.dtbo \
		kona-sec-c2q-kor-overlay-r08.dtbo \
		kona-sec-c2q-kor-overlay-r09.dtbo \
		kona-sec-c2q-kor-overlay-r10.dtbo

# C2Q JPN
SEC_C2Q_JPN_DTBO := \
		kona-sec-c2q-jpn-overlay-r01.dtbo \
		kona-sec-c2q-jpn-overlay-r03.dtbo \
		kona-sec-c2q-jpn-overlay-r04.dtbo \
		kona-sec-c2q-jpn-overlay-r06.dtbo \
		kona-sec-c2q-jpn-overlay-r07.dtbo \
		kona-sec-c2q-jpn-overlay-r08.dtbo \
		kona-sec-c2q-jpn-overlay-r09.dtbo \
		kona-sec-c2q-jpn-overlay-r10.dtbo

# C2Q CHN
SEC_C2Q_CHN_DTBO := \
		kona-sec-c2q-chn-overlay-r03.dtbo \
		kona-sec-c2q-chn-overlay-r06.dtbo \
		kona-sec-c2q-chn-overlay-r07.dtbo \
		kona-sec-c2q-chn-overlay-r08.dtbo \
		kona-sec-c2q-chn-overlay-r09.dtbo \
		kona-sec-c2q-chn-overlay-r10.dtbo

$(eval $(call sec_dtbo_build, \
		$(SEC_C2Q_USA_DTBO),$(CONFIG_MACH_C2Q_USA_SINGLE),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_C2Q_KOR_DTBO),$(CONFIG_MACH_C2Q_KOR_SINGLE),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_C2Q_JPN_DTBO),$(CONFIG_MACH_C2Q_JPN_SINGLE),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_C2Q_CHN_DTBO),$(CONFIG_MACH_C2Q_CHN_OPEN),$(SEC_KONA_BASE_DTB)))

endif
