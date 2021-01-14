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

#+++++++++++++++++++++++++++++++++++++++++++++++++++#
#                      KOR                          #
#+++++++++++++++++++++++++++++++++++++++++++++++++++#
# Y2Q KOR
SEC_Y2Q_KOR_DTBO := \
		kona-sec-y2q-kor-overlay-r13.dtbo \
		kona-sec-y2q-kor-overlay-r14.dtbo

#+++++++++++++++++++++++++++++++++++++++++++++++++++#
#                      USA                          #
#+++++++++++++++++++++++++++++++++++++++++++++++++++#
# Y2Q USA
SEC_Y2Q_USA_DTBO := \
		kona-sec-y2q-usa-overlay-r00-b.dtbo \
		kona-sec-y2q-usa-overlay-r01-b.dtbo \
		kona-sec-y2q-usa-overlay-r02.dtbo \
		kona-sec-y2q-usa-overlay-r03.dtbo \
		kona-sec-y2q-usa-overlay-r04.dtbo \
		kona-sec-y2q-usa-overlay-r05.dtbo \
		kona-sec-y2q-usa-overlay-r07.dtbo \
		kona-sec-y2q-usa-overlay-r07-sub.dtbo \
		kona-sec-y2q-usa-overlay-r08.dtbo \
		kona-sec-y2q-usa-overlay-r09.dtbo \
		kona-sec-y2q-usa-overlay-r11.dtbo \
		kona-sec-y2q-usa-overlay-r12.dtbo \
		kona-sec-y2q-usa-overlay-r13.dtbo \
		kona-sec-y2q-usa-overlay-r14.dtbo

#+++++++++++++++++++++++++++++++++++++++++++++++++++#
#                      CHN                          #
#+++++++++++++++++++++++++++++++++++++++++++++++++++#
# Y2Q CHN
SEC_Y2Q_CHN_DTBO := \
		kona-sec-y2q-chn-overlay-r07.dtbo \
		kona-sec-y2q-chn-overlay-r08.dtbo \
		kona-sec-y2q-chn-overlay-r09.dtbo \
		kona-sec-y2q-chn-overlay-r11.dtbo \
		kona-sec-y2q-chn-overlay-r12.dtbo \
		kona-sec-y2q-chn-overlay-r13.dtbo \
		kona-sec-y2q-chn-overlay-r14.dtbo


#+++++++++++++++++++++++++++++++++++++++++++++++++++#
#                      JPN                          #
#+++++++++++++++++++++++++++++++++++++++++++++++++++#
# Y2Q JPN
SEC_Y2Q_JPN_DTBO := \
		kona-sec-y2q-jpn-overlay-r08.dtbo \
		kona-sec-y2q-jpn-overlay-r10.dtbo \
		kona-sec-y2q-jpn-overlay-r13.dtbo \
		kona-sec-y2q-jpn-overlay-r14.dtbo \
		kona-sec-y2q-jpn-overlay-r15.dtbo

$(eval $(call sec_dtbo_build, \
		$(SEC_Y2Q_KOR_DTBO),$(CONFIG_MACH_Y2Q_KOR_SINGLE),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_Y2Q_USA_DTBO),$(CONFIG_MACH_Y2Q_USA_SINGLE),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_Y2Q_CHN_DTBO),$(CONFIG_MACH_Y2Q_CHN_OPEN),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_Y2Q_JPN_DTBO),$(CONFIG_MACH_Y2Q_JPN_SINGLE),$(SEC_KONA_BASE_DTB)))

endif
