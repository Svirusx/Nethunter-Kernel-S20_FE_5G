/*
 * s2dos06.c - Regulator driver for the Samsung s2dos06
 *
 * Copyright (C) 2019 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/module.h>
#include <linux/bug.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/s2dos06.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/pmic_class.h>
#include <linux/string.h>

struct s2dos06_data {
	struct regulator_dev *rdev[S2DOS06_REGULATOR_MAX];
	struct s2dos06_dev *iodev;
	int num_regulators;
	int opmode[S2DOS06_REGULATOR_MAX];
#ifdef CONFIG_DRV_SAMSUNG_PMIC
	u8 read_addr;
	u8 read_val;
	struct device *dev;
#endif
};

int s2dos06_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct s2dos06_data *info = i2c_get_clientdata(i2c);
	struct s2dos06_dev *s2dos06 = info->iodev;
	int ret = 0;

	mutex_lock(&s2dos06->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	mutex_unlock(&s2dos06->i2c_lock);
	if (ret < 0) {
		pr_info("%s:%s reg(0x%02hhx), ret(%d)\n",
			 MFD_DEV_NAME, __func__, reg, ret);
		return ret;
	}

	ret &= 0xff;
	*dest = ret;

	return 0;
}

int s2dos06_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct s2dos06_data *info = i2c_get_clientdata(i2c);
	struct s2dos06_dev *s2dos06 = info->iodev;
	int ret = 0;

	mutex_lock(&s2dos06->i2c_lock);
	ret = i2c_smbus_read_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&s2dos06->i2c_lock);
	if (ret < 0)
		return ret;

	return 0;
}

int s2dos06_read_word(struct i2c_client *i2c, u8 reg)
{
	struct s2dos06_data *info = i2c_get_clientdata(i2c);
	struct s2dos06_dev *s2dos06 = info->iodev;
	int ret = 0;

	mutex_lock(&s2dos06->i2c_lock);
	ret = i2c_smbus_read_word_data(i2c, reg);
	mutex_unlock(&s2dos06->i2c_lock);
	if (ret < 0)
		return ret;

	return ret;
}

int s2dos06_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct s2dos06_data *info = i2c_get_clientdata(i2c);
	struct s2dos06_dev *s2dos06 = info->iodev;
	int ret = 0;

	mutex_lock(&s2dos06->i2c_lock);
	ret = i2c_smbus_write_byte_data(i2c, reg, value);
	mutex_unlock(&s2dos06->i2c_lock);
	if (ret < 0)
		pr_info("%s:%s reg(0x%02hhx), ret(%d)\n",
			MFD_DEV_NAME, __func__, reg, ret);

	return ret;
}

int s2dos06_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct s2dos06_data *info = i2c_get_clientdata(i2c);
	struct s2dos06_dev *s2dos06 = info->iodev;
	int ret = 0;

	mutex_lock(&s2dos06->i2c_lock);
	ret = i2c_smbus_write_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&s2dos06->i2c_lock);
	if (ret < 0)
		return ret;

	return 0;
}

int s2dos06_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct s2dos06_data *info = i2c_get_clientdata(i2c);
	struct s2dos06_dev *s2dos06 = info->iodev;
	int ret = 0;
	u8 old_val = 0, new_val = 0;

	mutex_lock(&s2dos06->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret >= 0) {
		old_val = ret & 0xff;
		new_val = (val & mask) | (old_val & (~mask));
		ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
	}
	mutex_unlock(&s2dos06->i2c_lock);

	return ret;
}

static int s2m_enable(struct regulator_dev *rdev)
{
	struct s2dos06_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;

	return s2dos06_update_reg(i2c, rdev->desc->enable_reg,
				  info->opmode[rdev_get_id(rdev)],
				  rdev->desc->enable_mask);
}

static int s2m_disable_regmap(struct regulator_dev *rdev)
{
	struct s2dos06_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	u8 val = 0;

	if (rdev->desc->enable_is_inverted)
		val = rdev->desc->enable_mask;
	else
		val = 0;

	return s2dos06_update_reg(i2c, rdev->desc->enable_reg,
				   val, rdev->desc->enable_mask);
}

static int s2m_is_enabled_regmap(struct regulator_dev *rdev)
{
	struct s2dos06_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret = 0;
	u8 val = 0;

	ret = s2dos06_read_reg(i2c, rdev->desc->enable_reg, &val);
	if (ret < 0)
		return ret;

	if (rdev->desc->enable_is_inverted)
		return (val & rdev->desc->enable_mask) == 0;
	else
		return (val & rdev->desc->enable_mask) != 0;
}

static int s2m_get_voltage_sel_regmap(struct regulator_dev *rdev)
{
	struct s2dos06_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret = 0;
	u8 val = 0;

	ret = s2dos06_read_reg(i2c, rdev->desc->vsel_reg, &val);
	if (ret < 0)
		return ret;

	val &= rdev->desc->vsel_mask;

	return val;
}

static int s2m_set_voltage_sel_regmap(struct regulator_dev *rdev, unsigned sel)
{
	struct s2dos06_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret = 0;

	ret = s2dos06_update_reg(i2c, rdev->desc->vsel_reg,
				 sel, rdev->desc->vsel_mask);
	if (ret < 0)
		goto out;

	if (rdev->desc->apply_bit)
		ret = s2dos06_update_reg(i2c, rdev->desc->apply_reg,
					 rdev->desc->apply_bit,
					 rdev->desc->apply_bit);

	return ret;
out:
	pr_warn("%s: failed to set voltage_sel_regmap\n", rdev->desc->name);

	return ret;
}

static int s2m_set_voltage_sel_regmap_buck(struct regulator_dev *rdev,
					   unsigned sel)
{
	struct s2dos06_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret = 0;

	ret = s2dos06_write_reg(i2c, rdev->desc->vsel_reg, sel);
	if (ret < 0)
		goto out;

	if (rdev->desc->apply_bit)
		ret = s2dos06_update_reg(i2c, rdev->desc->apply_reg,
					 rdev->desc->apply_bit,
					 rdev->desc->apply_bit);

	return ret;
out:
	pr_warn("%s: failed to set voltage_sel_regmap\n", rdev->desc->name);

	return ret;
}

static int s2m_set_voltage_time_sel(struct regulator_dev *rdev,
				    unsigned int old_selector,
				    unsigned int new_selector)
{
	int old_volt = 0, new_volt = 0;

	/* sanity check */
	if (!rdev->desc->ops->list_voltage)
		return -EINVAL;

	old_volt = rdev->desc->ops->list_voltage(rdev, old_selector);
	new_volt = rdev->desc->ops->list_voltage(rdev, new_selector);

	if (old_selector < new_selector)
		return DIV_ROUND_UP(new_volt - old_volt, S2DOS06_RAMP_DELAY);

	return 0;
}

static int s2m_set_active_discharge(struct regulator_dev *rdev,
					bool enable)
{
	struct s2dos06_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret;
	u8 val;

	if (enable)
		val = rdev->desc->active_discharge_on;
	else
		val = rdev->desc->active_discharge_off;

	ret = s2dos06_update_reg(i2c, rdev->desc->active_discharge_reg,
				val, rdev->desc->active_discharge_mask);
	return ret;
}

#ifdef CONFIG_SEC_PM
static int s2m_set_elvss_short_detection(struct regulator_dev *rdev,
					 bool enable, int lv_uA)
{
	struct s2dos06_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret = -ENODEV;
	u8 lv = 0, val, mask;

	if (enable) {
		lv = lv_uA / 2000 - 1;

		if (lv > 3)
			return -EINVAL;
	}

	val = (!enable << 4) | lv;
	mask = S2DOS06_ELVSS_SSD_EN_MASK | S2DOS06_ELVSS_SSD_SET_MASK;

	ret = s2dos06_update_reg(i2c, S2DOS06_REG_SSD, val, mask);
	return ret;
}

static struct regulator_ops s2dos06_elvss_ops = {
	.set_short_detection	= s2m_set_elvss_short_detection,
};
#endif /* CONFIG_SEC_PM */

static struct regulator_ops s2dos06_ldo_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.is_enabled		= s2m_is_enabled_regmap,
	.enable			= s2m_enable,
	.disable		= s2m_disable_regmap,
	.get_voltage_sel	= s2m_get_voltage_sel_regmap,
	.set_voltage_sel	= s2m_set_voltage_sel_regmap,
	.set_voltage_time_sel	= s2m_set_voltage_time_sel,
	.set_active_discharge	= s2m_set_active_discharge,	
};

static struct regulator_ops s2dos06_buck_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.is_enabled		= s2m_is_enabled_regmap,
	.enable			= s2m_enable,
	.disable		= s2m_disable_regmap,
	.get_voltage_sel	= s2m_get_voltage_sel_regmap,
	.set_voltage_sel	= s2m_set_voltage_sel_regmap_buck,
	.set_voltage_time_sel	= s2m_set_voltage_time_sel,
	.set_active_discharge	= s2m_set_active_discharge,
};

#define _BUCK(macro)	S2DOS06_BUCK##macro
#define _buck_ops(num)	s2dos06_buck_ops##num
#define _LDO(macro)	S2DOS06_LDO##macro
#define _REG(ctrl)	S2DOS06_REG##ctrl
#define _ldo_ops(num)	s2dos06_ldo_ops##num
#define _MASK(macro)	S2DOS06_ENABLE_MASK##macro
#define _TIME(macro)	S2DOS06_ENABLE_TIME##macro

#define BUCK_DESC(_name, _id, _ops, m, s, v, e, em, t, a)	{	\
	.name		= _name,				\
	.id		= _id,					\
	.ops		= _ops,					\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= m,					\
	.uV_step	= s,					\
	.n_voltages	= S2DOS06_BUCK_N_VOLTAGES,		\
	.vsel_reg	= v,					\
	.vsel_mask	= S2DOS06_BUCK_VSEL_MASK,		\
	.enable_reg	= e,					\
	.enable_mask	= em,					\
	.enable_time	= t,			\
	.active_discharge_off = 0,				\
	.active_discharge_on = S2DOS06_BUCK_FD_MASK,		\
	.active_discharge_reg	= a,				\
	.active_discharge_mask	= S2DOS06_BUCK_FD_MASK		\
}

#define LDO_DESC(_name, _id, _ops, m, s, v, e, em, t, a)	{	\
	.name		= _name,				\
	.id		= _id,					\
	.ops		= _ops,					\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= m,					\
	.uV_step	= s,					\
	.n_voltages	= S2DOS06_LDO_N_VOLTAGES,		\
	.vsel_reg	= v,					\
	.vsel_mask	= S2DOS06_LDO_VSEL_MASK,		\
	.enable_reg	= e,					\
	.enable_mask	= em,					\
	.enable_time	= t,			\
	.active_discharge_off = 0,				\
	.active_discharge_on = S2DOS06_LDO_FD_MASK,		\
	.active_discharge_reg	= a,				\
	.active_discharge_mask	= S2DOS06_LDO_FD_MASK		\
}

#ifdef CONFIG_SEC_PM
#define ELVSS_DESC(_name, _id)				{	\
	.name		= _name,				\
	.id		= _id,					\
	.ops		= &s2dos06_elvss_ops,			\
	.type		= REGULATOR_CURRENT,			\
	.owner		= THIS_MODULE,				\
}
#endif /* CONFIG_SEC_PM */

static struct regulator_desc regulators[S2DOS06_REGULATOR_MAX] = {
	/* name, id, ops, min_uv, uV_step, vsel_reg, enable_reg */
	LDO_DESC("s2dos06-ldo1", _LDO(1), &_ldo_ops(), _LDO(_MIN1),
		_LDO(_STEP1), _REG(_LDO1_CFG),
		_REG(_EN), _MASK(_L1), _TIME(_LDO),_REG(_LDO1_CFG)),
	LDO_DESC("s2dos06-ldo2", _LDO(2), &_ldo_ops(), _LDO(_MIN1),
		_LDO(_STEP1), _REG(_LDO2_CFG),
		_REG(_EN), _MASK(_L2), _TIME(_LDO), _REG(_LDO2_CFG)),
	LDO_DESC("s2dos06-ldo3", _LDO(3), &_ldo_ops(), _LDO(_MIN2),
		_LDO(_STEP1), _REG(_LDO3_CFG),
		_REG(_EN), _MASK(_L3), _TIME(_LDO), _REG(_LDO3_CFG)),
	LDO_DESC("s2dos06-ldo4", _LDO(4), &_ldo_ops(), _LDO(_MIN2),
		_LDO(_STEP1), _REG(_LDO4_CFG),
		_REG(_EN), _MASK(_L4), _TIME(_LDO), _REG(_LDO4_CFG)),
	BUCK_DESC("s2dos06-buck1", _BUCK(1), &_buck_ops(), _BUCK(_MIN1),
		_BUCK(_STEP1), _REG(_BUCK_VOUT),
		_REG(_EN), _MASK(_B1), _TIME(_BUCK), _REG(_BUCK_CFG)),
#ifdef CONFIG_SEC_PM
	ELVSS_DESC("s2dos06-elvss", S2DOS06_ELVSS),
#endif /* CONFIG_SEC_PM */
};

static irqreturn_t s2dos06_irq_thread(int irq, void *irq_data)
{
	struct s2dos06_data *s2dos06 = irq_data;
	u8 val = 0;
	const char *irq_bit[] = {"UVLO", "SCP", "SSD", "PRE_TSD",
				 "TSD", "OVP", "OCL", "PWRMT"};
	char irq_name[32] = {0};
	ssize_t ret = 0;
	unsigned long bit = 0, tmp = 0;

	s2dos06_read_reg(s2dos06->iodev->i2c, S2DOS06_REG_IRQ, &val);
	pr_info("%s: irq(%d) S2DOS06_REG_IRQ : 0x%02hhx\n", __func__, irq, val);

	tmp = val;
	for_each_set_bit(bit, &tmp, ARRAY_SIZE(irq_bit))
		ret += sprintf(irq_name + ret, " %s", irq_bit[bit]);
	pr_info("%s: IRQ:%s\n", __func__, irq_name);

	return IRQ_HANDLED;
}

#ifdef CONFIG_OF
static int s2dos06_pmic_dt_parse_pdata(struct device *dev,
				       struct s2dos06_platform_data *pdata)
{
	struct device_node *pmic_np, *regulators_np, *reg_np;
	struct s2dos06_regulator_data *rdata;
	size_t i = 0;
	int ret = 0;
	u32 val = 0;

	pmic_np = dev->of_node;
	if (!pmic_np) {
		dev_err(dev, "could not find pmic sub-node\n");
		return -ENODEV;
	}

	pdata->dp_pmic_irq = of_get_named_gpio(pmic_np,
					       "s2dos06,s2dos06_int", 0);
	if (pdata->dp_pmic_irq < 0)
		pr_err("%s error reading s2dos06_irq = %d\n",
			__func__, pdata->dp_pmic_irq);

	pdata->wakeup = of_property_read_bool(pmic_np, "s2dos06,wakeup");

#ifdef CONFIG_SEC_PM
	if (!of_property_read_string(pmic_np, "sec_disp_pmic_name",
				&pdata->sec_disp_pmic_name))
		dev_info(dev, "sec_disp_pmic_name: %s\n",
				pdata->sec_disp_pmic_name);
#endif /* CONFIG_SEC_PM */

	pdata->adc_mode = 0;
	ret = of_property_read_u32(pmic_np, "adc_mode", &val);
	if (ret)
		return -EINVAL;
	pdata->adc_mode = val;

	pdata->adc_sync_mode = 0;
	ret = of_property_read_u32(pmic_np, "adc_sync_mode", &val);
	if (ret)
		return -EINVAL;
	pdata->adc_sync_mode = val;

	regulators_np = of_find_node_by_name(pmic_np, "regulators");
	if (!regulators_np) {
		dev_err(dev, "could not find regulators sub-node\n");
		return -EINVAL;
	}

	/* count the number of regulators to be supported in pmic */
	pdata->num_regulators = 0;
	for_each_child_of_node(regulators_np, reg_np) {
		pdata->num_regulators++;
	}

	rdata = devm_kzalloc(dev, sizeof(*rdata) *
			     pdata->num_regulators, GFP_KERNEL);
	if (!rdata) {
		dev_err(dev, "could not allocate memory for regulator data\n");
		return -ENOMEM;
	}

	pdata->regulators = rdata;
	for_each_child_of_node(regulators_np, reg_np) {
		for (i = 0; i < ARRAY_SIZE(regulators); i++)
			if (!of_node_cmp(reg_np->name, regulators[i].name))
				break;

		if (i == ARRAY_SIZE(regulators)) {
			dev_warn(dev,
				 "don't know how to configure regulator %s\n",
				 reg_np->name);
			continue;
		}

		rdata->id = i;
		rdata->initdata = of_get_regulator_init_data(dev, reg_np,
							     &regulators[i]);
		rdata->reg_node = reg_np;
		rdata++;
	}
	of_node_put(regulators_np);

	return 0;
}
#else
static int s2dos06_pmic_dt_parse_pdata(struct s2dos06_dev *iodev,
				       struct s2dos06_platform_data *pdata)
{
	return 0;
}
#endif /* CONFIG_OF */
#ifdef CONFIG_DRV_SAMSUNG_PMIC
static ssize_t s2dos06_read_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	struct s2dos06_data *s2dos06 = dev_get_drvdata(dev);
	int ret = 0;
	u8 val = 0, reg_addr = 0;

	if (buf == NULL) {
		pr_info("%s: empty buffer\n", __func__);
		return -1;
	}

	ret = kstrtou8(buf, 0, &reg_addr);
	if (ret < 0)
		pr_info("%s: fail to transform i2c address\n", __func__);

	ret = s2dos06_read_reg(s2dos06->iodev->i2c, reg_addr, &val);
	if (ret < 0)
		pr_info("%s: fail to read i2c address\n", __func__);

	pr_info("%s: reg(0x%02hhx) data(0x%02hhx)\n", __func__, reg_addr, val);
	s2dos06->read_addr = reg_addr;
	s2dos06->read_val = val;

	return size;
}

static ssize_t s2dos06_read_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct s2dos06_data *s2dos06 = dev_get_drvdata(dev);
	return sprintf(buf, "0x%02hhx: 0x%02hhx\n", s2dos06->read_addr,
		       s2dos06->read_val);
}

static ssize_t s2dos06_write_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t size)
{
	struct s2dos06_data *s2dos06 = dev_get_drvdata(dev);
	int ret = 0;
	u8 reg = 0, data = 0;

	if (buf == NULL) {
		pr_info("%s: empty buffer\n", __func__);
		return size;
	}

	ret = sscanf(buf, "0x%02hhx 0x%02hhx", &reg, &data);
	if (ret != 2) {
		pr_info("%s: input error\n", __func__);
		return size;
	}

	pr_info("%s: reg(0x%02hhx) data(0x%02hhx)\n", __func__, reg, data);

	ret = s2dos06_write_reg(s2dos06->iodev->i2c, reg, data);
	if (ret < 0)
		pr_info("%s: fail to write i2c addr/data\n", __func__);

	return size;
}

static ssize_t s2dos06_write_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	return sprintf(buf, "echo (register addr.) (data) > s2dos06_write\n");
}
static DEVICE_ATTR(s2dos06_write, 0644, s2dos06_write_show, s2dos06_write_store);
static DEVICE_ATTR(s2dos06_read, 0644, s2dos06_read_show, s2dos06_read_store);

#ifdef CONFIG_SEC_FACTORY
#define VALID_REG	S2DOS06_REG_EN	/* register address for validation */
#define VALID_MASK	0xE0		/* NA(reserved) bit */
static ssize_t validation_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct s2dos06_data *s2dos06 = dev_get_drvdata(dev);
	u8 val = 0xff;
	bool result = false;

	s2dos06_read_reg(s2dos06->iodev->i2c, VALID_REG, &val);
	pr_info("%s: %s: init state: reg(0x%02x) data(0x%02x)\n",
				MFD_DEV_NAME, __func__, VALID_REG, val);

	s2dos06_update_reg(s2dos06->iodev->i2c, VALID_REG, VALID_MASK, VALID_MASK);

	s2dos06_read_reg(s2dos06->iodev->i2c, VALID_REG, &val);
	pr_info("%s: %s: updated state: reg(0x%02x) data(0x%02x)\n",
				MFD_DEV_NAME, __func__, VALID_REG, val);

	if ((val & VALID_MASK) == VALID_MASK)
		result = true;

	/* no need change to init value(0x00), but, do it */
	s2dos06_update_reg(s2dos06->iodev->i2c, VALID_REG, 0x00, VALID_MASK);
	s2dos06_read_reg(s2dos06->iodev->i2c, VALID_REG, &val);

	pr_err("%s: %s: i2c operation is %s\n", MFD_DEV_NAME, __func__,
						result ? "ok" : "not ok");

	return sprintf(buf, "%d\n", result);
}
static DEVICE_ATTR(validation, 0444, validation_show, NULL);
#endif // CONFIG_SEC_FACTORY

int create_s2dos06_sysfs(struct s2dos06_data *s2dos06)
{
	struct device *s2dos06_pmic;
	struct device *dev = s2dos06->iodev->dev;
	char device_name[32] = {0, };
	int err = -ENODEV;

	pr_info("%s: display pmic sysfs start\n", __func__);
	s2dos06->read_addr = 0;
	s2dos06->read_val = 0;

	/* Dynamic allocation for device name */
	snprintf(device_name, sizeof(device_name) - 1, "%s@%s",
		 dev_driver_string(dev), dev_name(dev));

	s2dos06_pmic = pmic_device_create(s2dos06, device_name);
	s2dos06->dev = s2dos06_pmic;

	err = device_create_file(s2dos06_pmic, &dev_attr_s2dos06_write);
	if (err) {
		pr_err("s2dos06_sysfs: failed to create device file, %s\n",
			dev_attr_s2dos06_write.attr.name);
		goto err_sysfs;
	}

	err = device_create_file(s2dos06_pmic, &dev_attr_s2dos06_read);
	if (err) {
		pr_err("s2dos06_sysfs: failed to create device file, %s\n",
			dev_attr_s2dos06_read.attr.name);
		goto err_sysfs;
	}

	return 0;

err_sysfs:
	return err;
}
#endif

#ifdef CONFIG_SEC_PM
static ssize_t enable_fd_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct i2c_client *i2c = s2dos06_info->iodev->i2c;
	u8 fd = 0, fd_mask = 0xf, fd_value = 0xf;
	int ret = 0;
	bool enabled;

	s2dos06_update_reg(i2c, S2DOS06_REG_17_SOFT_RESET, 0x40, 0x40);
	s2dos06_update_reg(i2c, S2DOS06_REG_20_SOFT_RESET, 0x40, 0x40);

	ret = s2dos06_read_reg(i2c, S2DOS06_REG_FD, &fd);
	if (ret < 0) {
		dev_err(dev, "%s: Failed to read FD(%d)\n", __func__, ret);
		return ret;
	}

	s2dos06_update_reg(i2c, S2DOS06_REG_17_SOFT_RESET, 0, 0x40);
	s2dos06_update_reg(i2c, S2DOS06_REG_20_SOFT_RESET, 0, 0x40);

	enabled = ((fd & fd_mask) == fd_value);

	return sprintf(buf, "%s\n", enabled ? "enabled" : "disabled");
}

static ssize_t enable_fd_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client *i2c = s2dos06_info->iodev->i2c;
	int ret;
	u8 fd;
	bool enable;

	ret = strtobool(buf, &enable);
	if (ret)
		return ret;

	dev_info(&i2c->dev, "%s: enable(%d)\n", __func__, enable);

	fd = (enable)? 0xf : 0x3;

	s2dos06_update_reg(i2c, S2DOS06_REG_17_SOFT_RESET, 0x40, 0x40);
	s2dos06_update_reg(i2c, S2DOS06_REG_20_SOFT_RESET, 0x40, 0x40);

	ret = s2dos06_update_reg(i2c, S2DOS06_REG_FD, fd, fd);
	if (ret < 0) {
		dev_err(dev, "%s: Failed to update FD3(%d)\n", __func__, ret);
		return ret;
	}
	s2dos06_read_reg(i2c, S2DOS06_REG_FD, &fd);
	dev_info(&i2c->dev, "%s: fd(0x%02X)\n", __func__, fd);

	s2dos06_update_reg(i2c, S2DOS06_REG_17_SOFT_RESET, 0, 0x40);
	s2dos06_update_reg(i2c, S2DOS06_REG_20_SOFT_RESET, 0, 0x40);

	return count;
}

static DEVICE_ATTR(enable_fd, 0664, enable_fd_show, enable_fd_store);

static int s2dos06_sec_pm_init(struct s2dos06_data *info)
{
	struct s2dos06_dev *iodev = info->iodev;
	struct device *dev = &iodev->i2c->dev;
	const char *sec_disp_pmic_name = iodev->pdata->sec_disp_pmic_name;
	int ret = 0;
#ifndef CONFIG_SEC_FACTORY
	struct i2c_client *i2c = iodev->i2c;
	u8 fd = 0;
#endif

	if (sec_disp_pmic_name)
		iodev->sec_disp_pmic_dev =
			sec_device_create(info, sec_disp_pmic_name);
	else
		iodev->sec_disp_pmic_dev =
			sec_device_create(info, "disp_pmic");

	if (unlikely(IS_ERR(iodev->sec_disp_pmic_dev))) {
		ret = PTR_ERR(iodev->sec_disp_pmic_dev);
		dev_err(dev, "%s: Failed to create disp_pmic(%d)\n", __func__,
				ret);
		return ret;
	}

#ifndef CONFIG_SEC_FACTORY
	/* FD should be disabled by default at normal binary (Factory binary: default on) */
	dev_info(dev, "%s: Disable FD\n", __func__);

	s2dos06_update_reg(i2c, S2DOS06_REG_17_SOFT_RESET, 0x40, 0x40);
	s2dos06_update_reg(i2c, S2DOS06_REG_20_SOFT_RESET, 0x40, 0x40);

	ret = s2dos06_update_reg(i2c, S2DOS06_REG_FD, 0x3, 0x3);
	if (ret < 0) {
		dev_err(dev, "%s: Failed to disable FD(%d)\n", __func__, ret);
		goto remove_sec_disp_pmic_dev;
	}
	s2dos06_read_reg(i2c, S2DOS06_REG_FD, &fd);
	dev_info(&i2c->dev, "%s: fd(0x%02X)\n", __func__, fd);

	s2dos06_update_reg(i2c, S2DOS06_REG_17_SOFT_RESET, 0, 0x40);
	s2dos06_update_reg(i2c, S2DOS06_REG_20_SOFT_RESET, 0, 0x40);
#endif /* CONFIG_SEC_FACTORY */

	ret = device_create_file(iodev->sec_disp_pmic_dev, &dev_attr_enable_fd);
	if (ret) {
		dev_err(dev, "%s: Failed to create enable_fd(%d)\n", __func__,
				ret);
		goto remove_sec_disp_pmic_dev;
	}

#ifdef CONFIG_SEC_FACTORY
	ret = device_create_file(iodev->sec_disp_pmic_dev, &dev_attr_validation);
	if (ret) {
		pr_err("s2dos06_sysfs: failed to create device file, %s\n",
			dev_attr_validation.attr.name);
		goto remove_sec_disp_pmic_dev;
	}
#endif // CONFIG_SEC_FACTORY

	return 0;

remove_sec_disp_pmic_dev:
	sec_device_destroy(iodev->sec_disp_pmic_dev->devt);

	return ret;
}

static void s2dos06_sec_pm_deinit(struct s2dos06_data *info)
{
	struct s2dos06_dev *iodev = info->iodev;

	device_remove_file(iodev->sec_disp_pmic_dev, &dev_attr_enable_fd);
	sec_device_destroy(info->iodev->sec_disp_pmic_dev->devt);
}
#endif // CONFIG_SEC_PM

static int s2dos06_set_interrupt(struct s2dos06_data *s2dos06,
				 struct s2dos06_platform_data *pdata)
{
	struct s2dos06_dev *iodev = s2dos06->iodev;
	int ret = 0;

	/* Unmask interrupts */
	ret = s2dos06_update_reg(iodev->i2c, S2DOS06_REG_IRQ_MASK,
				 ~(S2DOS06_IRQ_SCP_MASK|S2DOS06_IRQ_SSD_MASK),
				 0xFF);
	if (ret < 0) {
		pr_err("%s: Failed to unmask IRQ MASK address\n", __func__);
		goto err;
	}

	/* Request irq */
	if (pdata->dp_pmic_irq > 0) {
		iodev->dp_pmic_irq = gpio_to_irq(pdata->dp_pmic_irq);
		pr_info("%s : dp_pmic_irq = %d\n", __func__, iodev->dp_pmic_irq);

		if (iodev->dp_pmic_irq > 0) {
			ret = devm_request_threaded_irq(iodev->dev, iodev->dp_pmic_irq,
							NULL, s2dos06_irq_thread,
							IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
							"dp-pmic-irq", s2dos06);
			if (ret) {
				pr_err("%s: Failed to Request IRQ(%d)\n",
					__func__, ret);
				goto err;
			}

			ret = enable_irq_wake(iodev->dp_pmic_irq);
			if (ret < 0) {
				pr_err("%s: Failed to Enable Wakeup Source(%d)\n",
					__func__, ret);
				goto err;
			}
		} else {
			pr_err("%s: Failed gpio_to_irq(%d)\n",
				__func__, iodev->dp_pmic_irq);
			goto err;
		}
	}
	return 0;

err:
	return -1;
}

static int s2dos06_pmic_probe(struct i2c_client *i2c,
			      const struct i2c_device_id *dev_id)
{
	struct s2dos06_dev *iodev;
	struct s2dos06_platform_data *pdata = i2c->dev.platform_data;
	struct regulator_config config = { };
	struct s2dos06_data *s2dos06;
	size_t i = 0;
	int ret = 0;

	pr_info("%s: %s\n", MFD_DEV_NAME, __func__);

	iodev = devm_kzalloc(&i2c->dev, sizeof(struct s2dos06_dev), GFP_KERNEL);
	if (!iodev) {
		dev_err(&i2c->dev,
			"%s: Failed to alloc mem for s2dos06\n", __func__);
		return -ENOMEM;
	}

	if (i2c->dev.of_node) {
		pdata = devm_kzalloc(&i2c->dev,
				     sizeof(struct s2dos06_platform_data),
				     GFP_KERNEL);
		if (!pdata) {
			dev_err(&i2c->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_pdata;
		}

		ret = s2dos06_pmic_dt_parse_pdata(&i2c->dev, pdata);
		if (ret < 0) {
			dev_err(&i2c->dev, "Failed to get device of_node\n");
			goto err_pdata;
		}

		i2c->dev.platform_data = pdata;
	} else
		pdata = i2c->dev.platform_data;

	iodev->dev = &i2c->dev;
	iodev->i2c = i2c;

	if (pdata) {
		iodev->pdata = pdata;
		iodev->wakeup = pdata->wakeup;
	} else {
		ret = -EINVAL;
		goto err_pdata;
	}
	mutex_init(&iodev->i2c_lock);

	s2dos06 = devm_kzalloc(&i2c->dev, sizeof(struct s2dos06_data),
				GFP_KERNEL);
	if (!s2dos06) {
		pr_info("[%s:%d] if (!s2dos06)\n", __FILE__, __LINE__);
		ret = -ENOMEM;
		goto err_s2dos06_data;
	}

	i2c_set_clientdata(i2c, s2dos06);
	s2dos06->iodev = iodev;
	s2dos06->num_regulators = pdata->num_regulators;
	
	for (i = 0; i < pdata->num_regulators; i++) {
		int id = pdata->regulators[i].id;
		config.dev = &i2c->dev;
		config.init_data = pdata->regulators[i].initdata;
		config.driver_data = s2dos06;
		config.of_node = pdata->regulators[i].reg_node;
		s2dos06->opmode[id] = regulators[id].enable_mask;
		s2dos06->rdev[i] = devm_regulator_register(&i2c->dev,
							   &regulators[id], &config);
		if (IS_ERR(s2dos06->rdev[i])) {
			ret = PTR_ERR(s2dos06->rdev[i]);
			dev_err(&i2c->dev, "regulator init failed for %d\n", id);
			s2dos06->rdev[i] = NULL;
			goto err_s2dos06_data;
		}
	}

#ifdef CONFIG_SEC_PM
	s2dos06_info = s2dos06;
	ret = s2dos06_sec_pm_init(s2dos06);
	if (ret < 0)
		goto err_s2dos06_data;
#endif /* CONFIG_SEC_PM */

	/* Init powermeter */
	iodev->adc_mode = pdata->adc_mode;
	iodev->adc_sync_mode = pdata->adc_sync_mode;
	if (iodev->adc_mode > 0)
		s2dos06_powermeter_init(iodev);
	
	/* Set interrupts */
	ret = s2dos06_set_interrupt(s2dos06, pdata);
	if (ret < 0)
		goto err_s2dos06_data;

#ifdef CONFIG_DRV_SAMSUNG_PMIC
	/* Create sysfs */
	ret = create_s2dos06_sysfs(s2dos06);
	if (ret < 0)
		goto err_s2dos06_data;
#endif

	return ret;

err_s2dos06_data:
	mutex_destroy(&iodev->i2c_lock);
err_pdata:
	pr_info("[%s:%d] err\n", __func__, __LINE__);

	return ret;
}

#if defined(CONFIG_OF)
static struct of_device_id s2dos06_i2c_dt_ids[] = {
	{ .compatible = "samsung,s2dos06pmic" },
	{ },
};
#endif /* CONFIG_OF */

static int s2dos06_pmic_remove(struct i2c_client *i2c)
{
	struct s2dos06_data *info = i2c_get_clientdata(i2c);

	dev_info(&i2c->dev, "%s\n", __func__);

	s2dos06_powermeter_deinit(info->iodev);
#ifdef CONFIG_DRV_SAMSUNG_PMIC
	pmic_device_destroy(info->dev->devt);
#endif
#ifdef CONFIG_SEC_PM
	s2dos06_sec_pm_deinit(info);
#endif /* CONFIG_SEC_PM */
	return 0;
}

#if defined(CONFIG_PM)
static int s2dos06_pmic_suspend(struct device *dev)
{
	struct i2c_client *i2c = to_i2c_client(dev);
	struct s2dos06_data *info = i2c_get_clientdata(i2c);
	struct s2dos06_dev *s2dos06 = info->iodev;

	pr_info("%s adc_mode: %d\n", __func__, s2dos06->adc_mode);
	if (s2dos06->adc_mode > 0) {
		s2dos06_read_reg(s2dos06->i2c,
				 S2DOS06_REG_PWRMT_CTRL2, &s2dos06->adc_en_val);
		if (s2dos06->adc_en_val & 0x80)
			s2dos06_update_reg(s2dos06->i2c, S2DOS06_REG_PWRMT_CTRL2,
					   0, ADC_EN_MASK);
	}

	return 0;
}

static int s2dos06_pmic_resume(struct device *dev)
{
	struct i2c_client *i2c = to_i2c_client(dev);
	struct s2dos06_data *info = i2c_get_clientdata(i2c);
	struct s2dos06_dev *s2dos06 = info->iodev;

	pr_info("%s adc_mode : %d\n", __func__, s2dos06->adc_mode);
	if (s2dos06->adc_mode > 0)
		s2dos06_update_reg(s2dos06->i2c, S2DOS06_REG_PWRMT_CTRL2,
				   s2dos06->adc_en_val & 0x80, ADC_EN_MASK);

	return 0;
}
#else
#define s2dos06_pmic_suspend	NULL
#define s2dos06_pmic_resume	NULL
#endif /* CONFIG_PM */

const struct dev_pm_ops s2dos06_pmic_pm = {
	.suspend = s2dos06_pmic_suspend,
	.resume = s2dos06_pmic_resume,
};

#if defined(CONFIG_OF)
static const struct i2c_device_id s2dos06_pmic_id[] = {
	{"s2dos06-regulator", 0},
	{"s2dos06-regulator1", 0},
	{},
};
#endif

static struct i2c_driver s2dos06_i2c_driver = {
	.driver = {
		.name = "s2dos06-regulator",
		.owner = THIS_MODULE,
		.pm = &s2dos06_pmic_pm,
#if defined(CONFIG_OF)
		.of_match_table	= s2dos06_i2c_dt_ids,
#endif /* CONFIG_OF */
		.suppress_bind_attrs = true,
	},
	.probe = s2dos06_pmic_probe,
	.remove = s2dos06_pmic_remove,
	.id_table = s2dos06_pmic_id,
};

static int __init s2dos06_i2c_init(void)
{
	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);
	return i2c_add_driver(&s2dos06_i2c_driver);
}
subsys_initcall(s2dos06_i2c_init);

static void __exit s2dos06_i2c_exit(void)
{
	i2c_del_driver(&s2dos06_i2c_driver);
}
module_exit(s2dos06_i2c_exit);

MODULE_DESCRIPTION("SAMSUNG s2dos06 Regulator Driver");
MODULE_LICENSE("GPL");
