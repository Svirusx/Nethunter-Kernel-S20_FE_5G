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

# Z3Q KOR
SEC_Z3Q_KOR_DTBO := \
		kona-sec-z3q-kor-overlay-r13.dtbo \
		kona-sec-z3q-kor-overlay-r14.dtbo \
		kona-sec-z3q-kor-overlay-r15.dtbo \
		kona-sec-z3q-kor-overlay-r16.dtbo

# Z3Q USA
SEC_Z3Q_USA_DTBO := \
		kona-sec-z3q-usa-overlay-r04.dtbo \
		kona-sec-z3q-usa-overlay-r07.dtbo \
		kona-sec-z3q-usa-overlay-r08.dtbo \
		kona-sec-z3q-usa-overlay-r09.dtbo \
		kona-sec-z3q-usa-overlay-r12.dtbo \
		kona-sec-z3q-usa-overlay-r13.dtbo \
		kona-sec-z3q-usa-overlay-r14.dtbo \
		kona-sec-z3q-usa-overlay-r15.dtbo \
		kona-sec-z3q-usa-overlay-r16.dtbo

# Z3Q CHN
SEC_Z3Q_CHN_DTBO := \
		kona-sec-z3q-chn-overlay-r07.dtbo \
		kona-sec-z3q-chn-overlay-r08.dtbo \
		kona-sec-z3q-chn-overlay-r09.dtbo \
		kona-sec-z3q-chn-overlay-r12.dtbo \
		kona-sec-z3q-chn-overlay-r13.dtbo \
		kona-sec-z3q-chn-overlay-r14.dtbo \
		kona-sec-z3q-chn-overlay-r15.dtbo \
		kona-sec-z3q-chn-overlay-r16.dtbo

# Z3Q JPN
SEC_Z3Q_JPN_DTBO := \
		kona-sec-z3q-jpn-overlay-r15.dtbo

$(eval $(call sec_dtbo_build, \
		$(SEC_Z3Q_KOR_DTBO),$(CONFIG_MACH_Z3Q_KOR_SINGLE),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_Z3Q_USA_DTBO),$(CONFIG_MACH_Z3Q_USA_SINGLE),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_Z3Q_CHN_DTBO),$(CONFIG_MACH_Z3Q_CHN_OPEN),$(SEC_KONA_BASE_DTB)))
$(eval $(call sec_dtbo_build, \
		$(SEC_Z3Q_JPN_DTBO),$(CONFIG_MACH_Z3Q_JPN_SINGLE),$(SEC_KONA_BASE_DTB)))

endif
