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

# BLOOMXQ EUR
SEC_BLOOMXQ_EUR_DTBO := \
		kona-sec-bloomxq-eur-overlay-r00.dtbo \
		kona-sec-bloomxq-eur-overlay-r01.dtbo \
		kona-sec-bloomxq-eur-overlay-r02.dtbo \
		kona-sec-bloomxq-eur-overlay-r03.dtbo \
		kona-sec-bloomxq-eur-overlay-r04.dtbo
# BLOOMXQ USA
SEC_BLOOMXQ_USA_DTBO := \
		kona-sec-bloomxq-usa-overlay-r00.dtbo \
		kona-sec-bloomxq-usa-overlay-r01.dtbo \
		kona-sec-bloomxq-usa-overlay-r02.dtbo \
		kona-sec-bloomxq-usa-overlay-r03.dtbo \
		kona-sec-bloomxq-usa-overlay-r04.dtbo \
		kona-sec-bloomxq-usa-overlay-r05.dtbo
# BLOOMXQ KOR
SEC_BLOOMXQ_KOR_DTBO := \
		kona-sec-bloomxq-kor-overlay-r00.dtbo \
		kona-sec-bloomxq-kor-overlay-r01.dtbo \
		kona-sec-bloomxq-kor-overlay-r02.dtbo \
		kona-sec-bloomxq-kor-overlay-r03.dtbo \
		kona-sec-bloomxq-kor-overlay-r04.dtbo

# BLOOMXQ CHN
SEC_BLOOMXQ_CHN_DTBO := \
		kona-sec-bloomxq-chn-overlay-r00.dtbo \
		kona-sec-bloomxq-chn-overlay-r01.dtbo \
		kona-sec-bloomxq-chn-overlay-r02.dtbo \
		kona-sec-bloomxq-chn-overlay-r03.dtbo \
		kona-sec-bloomxq-chn-overlay-r04.dtbo \
		kona-sec-bloomxq-chn-overlay-r05.dtbo

# BLOOMXQ JPN
SEC_BLOOMXQ_JPN_DTBO := \
		kona-sec-bloomxq-jpn-overlay-r04.dtbo

$(eval $(call sec_dtbo_build, \
		$(SEC_BLOOMXQ_EUR_DTBO),$(CONFIG_MACH_BLOOMXQ_EUR_OPEN),$(SEC_KONA_BASE_DTB)))

$(eval $(call sec_dtbo_build, \
		$(SEC_BLOOMXQ_USA_DTBO),$(CONFIG_MACH_BLOOMXQ_USA_SINGLE),$(SEC_KONA_BASE_DTB)))

$(eval $(call sec_dtbo_build, \
		$(SEC_BLOOMXQ_KOR_DTBO),$(CONFIG_MACH_BLOOMXQ_KOR_SINGLE),$(SEC_KONA_BASE_DTB)))

$(eval $(call sec_dtbo_build, \
		$(SEC_BLOOMXQ_CHN_DTBO),$(CONFIG_MACH_BLOOMXQ_CHN_OPEN),$(SEC_KONA_BASE_DTB)))

$(eval $(call sec_dtbo_build, \
		$(SEC_BLOOMXQ_JPN_DTBO),$(CONFIG_MACH_BLOOMXQ_JPN_SINGLE),$(SEC_KONA_BASE_DTB)))
endif
