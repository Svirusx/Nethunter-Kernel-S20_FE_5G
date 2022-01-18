/*
 * s2dos06.h
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef __LINUX_MFD_S2DOS06_H
#define __LINUX_MFD_S2DOS06_H

#include <linux/platform_device.h>
#include <linux/regmap.h>

#ifdef CONFIG_SEC_PM
#include <linux/sec_class.h>
#endif // CONFIG_SEC_PM

#ifdef CONFIG_SEC_PM
static struct s2dos06_data *s2dos06_info;
#endif /* CONFIG_SEC_PM */

#define MFD_DEV_NAME "s2dos06"

struct s2dos06_dev {
	struct device *dev;
	struct i2c_client *i2c; /* 0xB2; PMIC, Flash LED */
	struct mutex i2c_lock;

	u8 rev_num; /* pmic Rev */
	u8 adc_en_val;
	bool wakeup;
	int dp_pmic_irq;
	int adc_mode;
	int adc_sync_mode;
	int type;

	struct s2dos06_platform_data *pdata;
#ifdef CONFIG_DRV_SAMSUNG_PMIC
	struct device *powermeter_dev;
#endif
	struct adc_info *adc_meter;

#ifdef CONFIG_SEC_PM
	struct device *sec_disp_pmic_dev;
#endif /* CONFIG_SEC_PM */
};

/**
 * sec_regulator_data - regulator data
 * @id: regulator id
 * @initdata: regulator init data (contraints, supplies, ...)
 */
struct s2dos06_regulator_data {
	int id;
	struct regulator_init_data *initdata;
	struct device_node *reg_node;
};

struct s2dos06_platform_data {
	struct	s2dos06_regulator_data *regulators;
	bool wakeup;
	int num_regulators;
	int device_type;
	int dp_pmic_irq;
	int adc_mode; /* 0: not use, 1: current meter, 2: power meter */
	int adc_sync_mode; /* 1: sync mode, 2: async mode  */
#ifdef CONFIG_SEC_PM
	const char *sec_disp_pmic_name;
#endif /* CONFIG_SEC_PM */
};

struct s2dos06 {
	struct regmap *regmap;
};

/* S2DOS06 registers */
/* Slave address: 0xC0 */
enum S2DOS06_reg {
	S2DOS06_REG_STAT = 0x2,
	S2DOS06_REG_EN,
	S2DOS06_REG_LDO1_CFG,
	S2DOS06_REG_LDO2_CFG,
	S2DOS06_REG_LDO3_CFG,
	S2DOS06_REG_LDO4_CFG,
	S2DOS06_REG_BUCK_CFG,
	S2DOS06_REG_BUCK_VOUT,
	S2DOS06_REG_IRQ_MASK = 0x0D,
	S2DOS06_REG_IRQ = 0x11,
#ifdef CONFIG_SEC_PM
/*
	Note: To access the 0x20 address, you have to write 0x16[6], 0x17[6] register as '1'
		
		Example: FD1,2,3 enable with 0xC0 slave address
		0x16[6]='1' -> 0x17[6]='1' -> 0x20[3:2]='11' -> 0x16[6]='0' -> 0x17[6]='0'
*/
	S2DOS06_REG_17_SOFT_RESET = 0x16,	/* Bit[6]: Enabling AVDD, EL */
	S2DOS06_REG_20_SOFT_RESET = 0x17,	/* Bit[6]: VOUT3_EN & VOUT12_EN */
	S2DOS06_REG_FD = 0x20,			/* Bit[3]: AVDD(VO3), Bit[2]: ELVDD(VO1), ELVSS(VO2) */
#endif
	S2DOS06_REG_SSD = 0x24,
};

/* S2DOS06 regulator ids */
enum S2DOS06_regulators {
	S2DOS06_LDO1,
	S2DOS06_LDO2,
	S2DOS06_LDO3,
	S2DOS06_LDO4,
	S2DOS06_BUCK1,
#ifdef CONFIG_SEC_PM
	S2DOS06_ELVSS,
#endif
	S2DOS06_REG_MAX,
};
/* IRQ_MASK */
#define S2DOS06_IRQ_PWRMT_MASK		(1 << 7)
#define S2DOS06_IRQ_OCL_MASK		(1 << 6)
#define S2DOS06_IRQ_OVP_MASK		(1 << 5)
#define S2DOS06_IRQ_TSD_MASK		(1 << 4)
#define S2DOS06_IRQ_PRE_TSD_MASK	(1 << 3)
#define S2DOS06_IRQ_SSD_MASK		(1 << 2)
#define S2DOS06_IRQ_SCP_MASK		(1 << 1)
#define S2DOS06_IRQ_UVLO_MASK		(1 << 0)

#define S2DOS06_BUCK_MIN1		506250
#define S2DOS06_LDO_MIN1		1500000
#define S2DOS06_LDO_MIN2		2700000
#define S2DOS06_BUCK_STEP1		6250
#define S2DOS06_LDO_STEP1		25000
#define S2DOS06_LDO_VSEL_MASK		0x7F
#define S2DOS06_LDO_FD_MASK			0x80
#define S2DOS06_BUCK_VSEL_MASK		0xFF
#define S2DOS06_BUCK_FD_MASK		0x08
#ifdef CONFIG_SEC_PM
#define S2DOS06_ELVSS_SSD_SET_MASK	(1 << 4)
#define S2DOS06_ELVSS_SSD_EN_MASK	(3 << 0)
#endif
/* Control signal */
#define S2DOS06_ENABLE_MASK_L1		(1 << 0)
#define S2DOS06_ENABLE_MASK_L2		(1 << 1)
#define S2DOS06_ENABLE_MASK_L3		(1 << 2)
#define S2DOS06_ENABLE_MASK_L4		(1 << 3)
#define S2DOS06_ENABLE_MASK_B1		(1 << 4)

#define S2DOS06_RAMP_DELAY		12000
#define S2DOS06_ENABLE_TIME_LDO		50
#define S2DOS06_ENABLE_TIME_BUCK	350

#define S2DOS06_LDO_N_VOLTAGES		(S2DOS06_LDO_VSEL_MASK + 1)
#define S2DOS06_BUCK_N_VOLTAGES 	(S2DOS06_BUCK_VSEL_MASK + 1)

/* Power meter registers */
#define S2DOS06_REG_PWRMT_CTRL1		0x0A
#define S2DOS06_REG_PWRMT_CTRL2		0x0B
#define S2DOS06_REG_PWRMT_DATA		0x0C
#define S2DOS06_REG_IRQ_MASK		0x0D
/* Current 1bit resolution */
#define CURRENT_ELVDD			4080
#define CURRENT_ELVSS			4080
#define CURRENT_AVDD			612
#define CURRENT_BUCK			1220
#define CURRENT_L1			2000
#define CURRENT_L2			2000
#define CURRENT_L3			2000
#define CURRENT_L4			2000
/* Power 1bit resolution */
#define POWER_ELVDD			48000
#define	POWER_ELVSS			48000
#define POWER_AVDD			3060
#define POWER_BUCK			1525
#define POWER_L1			5000
#define POWER_L2			5000
#define POWER_L3			5000
#define POWER_L4			5000
/* Power meter MASK */
#define ADC_EN_MASK			0x80
#define ADC_ASYNCRD_MASK		0x80
#define ADC_PTR_MASK			0x0F
#define ADC_PGEN_MASK			0x30
#define CURRENT_MODE			0x00
#define POWER_MODE			0x10
#define RAWCURRENT_MODE			0x20
#define SMPNUM_MASK			0x0F
#define PWRMT_EN_CHK			(1 << 6)

#define S2DOS06_REGULATOR_MAX		6
#define S2DOS06_MAX_ADC_CHANNEL		8

extern void s2dos06_powermeter_init(struct s2dos06_dev *s2dos06);
extern void s2dos06_powermeter_deinit(struct s2dos06_dev *s2dos06);

/* S2DOS06 shared i2c API function */
extern int s2dos06_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest);
extern int s2dos06_write_reg(struct i2c_client *i2c, u8 reg, u8 value);
extern int s2dos06_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask);
#endif /*  __LINUX_MFD_S2DOS06_H */
