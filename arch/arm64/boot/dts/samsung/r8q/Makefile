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

# R8Q USA
SEC_R8Q_USA_DTBO := \
		kona-sec-r8q-usa-overlay-r00.dtbo \
		kona-sec-r8q-usa-overlay-r01.dtbo \
		kona-sec-r8q-usa-overlay-r02.dtbo \
		kona-sec-r8q-usa-overlay-r03.dtbo \
		kona-sec-r8q-usa-overlay-r04.dtbo \
		kona-sec-r8q-usa-overlay-r05.dtbo \
		kona-sec-r8q-usa-overlay-r06.dtbo

# R8Q EUR
SEC_R8Q_EUR_DTBO := \
		kona-sec-r8q-eur-overlay-r00.dtbo \
		kona-sec-r8q-eur-overlay-r01.dtbo \
		kona-sec-r8q-eur-overlay-r02.dtbo \
		kona-sec-r8q-eur-overlay-r03.dtbo \
		kona-sec-r8q-eur-overlay-r04.dtbo \
		kona-sec-r8q-eur-overlay-r05.dtbo \
		kona-sec-r8q-eur-overlay-r06.dtbo

# R8Q KOR
SEC_R8Q_KOR_DTBO := \
		kona-sec-r8q-kor-overlay-r00.dtbo \
		kona-sec-r8q-kor-overlay-r01.dtbo \
		kona-sec-r8q-kor-overlay-r02.dtbo \
		kona-sec-r8q-kor-overlay-r03.dtbo \
		kona-sec-r8q-kor-overlay-r04.dtbo \
		kona-sec-r8q-kor-overlay-r05.dtbo \
		kona-sec-r8q-kor-overlay-r06.dtbo

# R8Q CHN
SEC_R8Q_CHN_DTBO := \
		kona-sec-r8q-chn-overlay-r00.dtbo \
		kona-sec-r8q-chn-overlay-r01.dtbo \
		kona-sec-r8q-chn-overlay-r02.dtbo \
		kona-sec-r8q-chn-overlay-r03.dtbo \
		kona-sec-r8q-chn-overlay-r04.dtbo \
		kona-sec-r8q-chn-overlay-r05.dtbo \
		kona-sec-r8q-chn-overlay-r06.dtbo

$(eval $(call sec_dtbo_build, \
		$(SEC_R8Q_USA_DTBO),$(CONFIG_MACH_R8Q_USA_SINGLE),$(SEC_KONA_BASE_DTB)))

$(eval $(call sec_dtbo_build, \
		$(SEC_R8Q_EUR_DTBO),$(CONFIG_MACH_R8Q_EUR_OPEN),$(SEC_KONA_BASE_DTB)))

$(eval $(call sec_dtbo_build, \
		$(SEC_R8Q_KOR_DTBO),$(CONFIG_MACH_R8Q_KOR_SINGLE),$(SEC_KONA_BASE_DTB)))

$(eval $(call sec_dtbo_build, \
		$(SEC_R8Q_CHN_DTBO),$(CONFIG_MACH_R8Q_CHN_OPEN),$(SEC_KONA_BASE_DTB)))

endif
