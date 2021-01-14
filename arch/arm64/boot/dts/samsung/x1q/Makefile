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

# X1Q KOR
SEC_X1Q_KOR_DTBO := \
		kona-sec-x1q-kor-overlay-r13.dtbo \
		kona-sec-x1q-kor-overlay-r14.dtbo

# X1Q USA
SEC_X1Q_USA_DTBO := \
		kona-sec-x1q-usa-overlay-r04.dtbo \
		kona-sec-x1q-usa-overlay-r07.dtbo \
		kona-sec-x1q-usa-overlay-r08.dtbo \
		kona-sec-x1q-usa-overlay-r09.dtbo \
		kona-sec-x1q-usa-overlay-r12.dtbo \
		kona-sec-x1q-usa-overlay-r13.dtbo \
		kona-sec-x1q-usa-overlay-r14.dtbo \
		kona-sec-x1q-usa-overlay-r17.dtbo \
		kona-sec-x1q-usa-overlay-r18.dtbo \
		kona-sec-x1q-usa-overlay-r19.dtbo \
		kona-sec-x1q-usa-overlay-r20.dtbo

# X1Q CHN
SEC_X1Q_CHN_DTBO := \
		kona-sec-x1q-chn-overlay-r07.dtbo \
		kona-sec-x1q-chn-overlay-r08.dtbo \
		kona-sec-x1q-chn-overlay-r09.dtbo \
		kona-sec-x1q-chn-overlay-r12.dtbo \
		kona-sec-x1q-chn-overlay-r13.dtbo \
		kona-sec-x1q-chn-overlay-r14.dtbo

# X1Q JPN
SEC_X1Q_JPN_DTBO := \
		kona-sec-x1q-jpn-overlay-r07.dtbo \
		kona-sec-x1q-jpn-overlay-r08.dtbo \
		kona-sec-x1q-jpn-overlay-r09.dtbo \
		kona-sec-x1q-jpn-overlay-r10.dtbo \
		kona-sec-x1q-jpn-overlay-r12.dtbo \
		kona-sec-x1q-jpn-overlay-r13.dtbo \
		kona-sec-x1q-jpn-overlay-r14.dtbo

$(eval $(call sec_dtbo_build, \
		$(SEC_X1Q_KOR_DTBO),$(CONFIG_MACH_X1Q_KOR_SINGLE),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_X1Q_USA_DTBO),$(CONFIG_MACH_X1Q_USA_SINGLE),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_X1Q_CHN_DTBO),$(CONFIG_MACH_X1Q_CHN_OPEN),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_X1Q_JPN_DTBO),$(CONFIG_MACH_X1Q_JPN_SINGLE),$(SEC_KONA_BASE_DTB)))

endif
