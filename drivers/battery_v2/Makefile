subdir-ccflags-y := -Wformat

ifeq ($(CONFIG_BATTERY_SAMSUNG_MHS), y)
obj-$(CONFIG_BATTERY_SAMSUNG)	+= sec_battery_mhs.o
else
obj-$(CONFIG_BATTERY_SAMSUNG)	+= sec_battery.o
endif
obj-$(CONFIG_BATTERY_SAMSUNG)	+= sec_battery_sysfs.o
obj-$(CONFIG_BATTERY_SAMSUNG)	+= sec_battery_dt.o
obj-$(CONFIG_OF)				+= sec_adc.o
obj-$(CONFIG_MULTI_CHARGING)	+= sec_multi_charger.o
obj-$(CONFIG_DUAL_BATTERY)	+= sec_dual_battery.o
obj-$(CONFIG_STEP_CHARGING)	+= sec_step_charging.o
obj-$(CONFIG_BATTERY_CISD)	+= sec_cisd.o
obj-$(CONFIG_WIRELESS_AUTH)	+= sec_battery_misc.o
obj-$(CONFIG_DIRECT_CHARGING)	+= sec_direct_charger.o
obj-$(CONFIG_UPDATE_BATTERY_DATA)	+= sec_battery_data.o
obj-$(CONFIG_BATTERY_NOTIFIER)	+= battery_notifier.o
obj-$(CONFIG_CCIC_NOTIFIER)		+= sec_pd.o

obj-$(CONFIG_FUELGAUGE_MAX17042)	+= max17042_fuelgauge.o sec_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_MAX17048)	+= max17048_fuelgauge.o sec_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_MAX17050)	+= max17050_fuelgauge.o sec_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_MAX77823)	+= max77823_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_MAX77833)	+= max77833_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_MAX77843)	+= max77843_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_MAX77849)	+= max77849_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_MAX77854)	+= max77854_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_STC3117)         += stc3117_fuelgauge.o sec_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_RT5033)		+= rt5033_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_S2MU003)		+= s2mu003_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_SM5703)		+= sm5703_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_SM5705)		+= sm5705_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_SM5720)		+= sm5720_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_MAX77705)	+= max77705_fuelgauge.o

obj-$(CONFIG_CHARGER_MAX8903)	+= max8903_charger.o
obj-$(CONFIG_CHARGER_SMB328)	+= smb328_charger.o
obj-$(CONFIG_CHARGER_SMB347)	+= smb347_charger.o
obj-$(CONFIG_CHARGER_SMB358)	+= smb358_charger.o
obj-$(CONFIG_CHARGER_BQ24157)	+= bq24157_charger.o sec_charger.o
obj-$(CONFIG_CHARGER_BQ24190)	+= bq24190_charger.o
obj-$(CONFIG_CHARGER_BQ24191)	+= bq24190_charger.o
obj-$(CONFIG_CHARGER_BQ24260)	+= bq24260_charger.o
obj-$(CONFIG_CHARGER_BQ25898S)	+= bq25898s_charger.o
obj-$(CONFIG_CHARGER_MAX77693)	+= max77693_charger.o
obj-$(CONFIG_CHARGER_MAX77803)	+= max77803_charger.o
obj-$(CONFIG_CHARGER_MAX77804K)	+= max77804k_charger.o
obj-$(CONFIG_CHARGER_MAX77823)	+= max77823_charger.o
obj-$(CONFIG_CHARGER_MAX77833)	+= max77833_charger.o
obj-$(CONFIG_CHARGER_MAX77843)  += max77843_charger.o
obj-$(CONFIG_CHARGER_MAX77849)  += max77849_charger.o
obj-$(CONFIG_CHARGER_MAX77854)  += max77854_charger.o
obj-$(CONFIG_CHARGER_MAX77888)  += max77888_charger.o
obj-$(CONFIG_CHARGER_SM5701)    += sm5701_charger.o
obj-$(CONFIG_CHARGER_RT5033)    += rt5033_charger.o
obj-$(CONFIG_CHARGER_SM5703)    += sm5703_charger.o
obj-$(CONFIG_CHARGER_SM5705)	+= sm5705_charger.o sm5705_charger_oper.o
obj-$(CONFIG_CHARGER_SM5720)    += sm5720_charger.o sm5720_charger_oper.o
obj-$(CONFIG_CHARGER_MAX77705)	+= max77705_charger.o
obj-$(CONFIG_CHARGER_S2MU003)   += s2mu003_charger.o
obj-$(CONFIG_CHARGER_SM5418)    += sm5418_charger.o
obj-$(CONFIG_CHARGER_DA9155)    += da9155_charger.o
obj-$(CONFIG_CHARGER_SMB1351)   += smb1351_charger.o

obj-$(CONFIG_WIRELESS_CHARGER_HAL_MFC)	+= mfc_hal_charger.o
obj-$(CONFIG_WIRELESS_CHARGER_BQ51221)	+= bq51221_charger.o
obj-$(CONFIG_WIRELESS_CHARGER_P9220)	+= p9220_charger.o
obj-$(CONFIG_WIRELESS_CHARGER_MFC)	+= mfc_charger.o
obj-$(CONFIG_WIRELESS_CHARGER_MFC_S2MIW04)	+= mfc_s2miw04_charger.o

obj-$(CONFIG_SWITCHING_S2ASL01)   += s2asl01_switching.o
obj-$(CONFIG_CHARGER_PCA9468)	+= pca9468_charger.o

obj-$(CONFIG_KUNIT)			+= kunit_test/
GCOV_PROFILE_sec_battery.o		:=y
