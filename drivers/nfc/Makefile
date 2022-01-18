# SPDX-License-Identifier: GPL-2.0
#
# Makefile for nfc devices
#
ifneq ($(SEC_BUILD_CONF_USE_ESE_TZ), false)
	ccflags-y += -DENABLE_ESE_SPI_SECURED
endif

obj-$(CONFIG_NFC_FDP)		+= fdp/
obj-$(CONFIG_NFC_PN544)		+= pn544/
obj-$(CONFIG_NFC_MICROREAD)	+= microread/
obj-$(CONFIG_NFC_PN533)		+= pn533/
obj-$(CONFIG_NFC_MEI_PHY)	+= mei_phy.o
obj-$(CONFIG_NFC_SIM)		+= nfcsim.o
obj-$(CONFIG_NFC_PORT100)	+= port100.o
obj-$(CONFIG_NFC_MRVL)		+= nfcmrvl/
obj-$(CONFIG_NFC_TRF7970A)	+= trf7970a.o
obj-$(CONFIG_NFC_ST21NFCA)  += st21nfca/
obj-$(CONFIG_NFC_ST_NCI)	+= st-nci/
obj-$(CONFIG_NFC_NXP_NCI)	+= nxp-nci/
obj-$(CONFIG_NFC_S3FWRN5)	+= s3fwrn5/
obj-$(CONFIG_NFC_ST95HF)	+= st95hf/
obj-$(CONFIG_NFC_NQ)		+= nq-nci.o
obj-$(CONFIG_NFC_PN547)		+= pn547.o
obj-$(CONFIG_NFC_PN547_ESE_SUPPORT)		+= p61.o
obj-$(CONFIG_NFC_FEATURE_SN100U)	+= cold_reset.o
obj-$(CONFIG_SEC_NFC)		+= sec_nfc.o
obj-$(CONFIG_SEC_NFC_V2)	+= sec_nfc_v2.o
obj-$(CONFIG_ESE_P3_LSI)    += ese_p3.o
obj-$(CONFIG_ESE_P3_V2_LSI)    += ese_p3_v2.o
obj-y                       += nfc_logger/
