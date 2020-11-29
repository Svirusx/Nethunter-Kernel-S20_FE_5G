/*
 * combo redriver for ps5169.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/sec_class.h>
#include <linux/combo_redriver/ps5169.h>

struct ps5169_data *redrv_data;
struct device *redriver_device;

static char REDRV_MODE_Print[6][19] = {
	{"WORK_MODE"},
	{"USB_ONLY_MODE"},
	{"DP_ONLY_MODE"},
	{"DP2_LANE_USB_MODE"},
	{"CLEAR_STATE"},
	{"CHECK_EXIST"},
};

int ps5169_i2c_check() 
{

	int value = 0;

	usleep_range(10000, 10100);
	value = i2c_smbus_read_byte_data(redrv_data->i2c, Chip_ID1);
	pr_info("%s: INIT_MODE read Chip_ID1 (%x)\n", __func__, value);
	value = i2c_smbus_read_byte_data(redrv_data->i2c, Chip_ID2);
	pr_info("%s: INIT_MODE read Chip ID2 (%x)\n", __func__, value);

	return value;
}

int ps5169_config(int config, int is_DFP)
{
	int is_front = 0;
	int value;

	if (!redrv_data) {
		pr_err("%s: Invalid redrv_data\n", __func__);
		return -2;
	}

	if (!redrv_data->i2c) {
		pr_err("%s: Invalid redrv_data->i2c\n", __func__);
		return -1;
	}
	is_front = !gpio_get_value(redrv_data->con_sel);
	pr_info("%s: config(%s)(%s)(%s)\n", __func__, REDRV_MODE_Print[config], (is_DFP ? "DFP":"UFP"), (is_front ? "FRONT":"REAR"));

	switch (config) {
	case WORK_MODE:
		usleep_range(10000, 10100);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x9d, 0x80);
		usleep_range(10000, 10100);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x9d, 0x00);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x40, 0x80);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0xa0, 0x02);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x8d, 0x01);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x90, 0x01);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x51, 0x40);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x50, 0x20);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x54, 0x00);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x5d, 0x44);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x55, 0x00);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x56, 0x00);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x57, 0x00);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x58, 0x00);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x59, 0x00);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x5a, 0x00);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x5b, 0x00);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x5f, 0x00);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x60, 0x00);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x61, 0x03);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x65, 0x40);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x66, 0x00);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x67, 0x03);

#if defined(CONFIG_SEC_GTS7L_PROJECT) || defined(CONFIG_SEC_GTS7XL_PROJECT)
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x52, 0x30);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x5e, 0x06);
#else
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x52, 0x20);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x5e, 0x06);
#endif
		break;

	case USB_ONLY_MODE:
		if (is_front)
			i2c_smbus_write_byte_data(redrv_data->i2c, 0x40, 0xc0);
		else
			i2c_smbus_write_byte_data(redrv_data->i2c, 0x40, 0xd0);

		i2c_smbus_write_byte_data(redrv_data->i2c, 0xa0, 0x02);

		value = i2c_smbus_read_byte_data(redrv_data->i2c, 0x40);
		pr_info("%s: USB3_ONLY_MODE read 0x40 (%x) (%s)\n",
				__func__, value, (is_front ? "FRONT":"REAR"));
		break;

	case DP_ONLY_MODE:
		if (is_front)
			i2c_smbus_write_byte_data(redrv_data->i2c, 0x40, 0xa0);
		else
			i2c_smbus_write_byte_data(redrv_data->i2c, 0x40, 0xb0);

		i2c_smbus_write_byte_data(redrv_data->i2c, 0xa0, 0x00);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0xa1, 0x04);

		value = i2c_smbus_read_byte_data(redrv_data->i2c, 0x40);
		pr_info("%s: DP4_LANE_MODE read 0x40 (%x) (%s)\n",
				__func__, value, (is_front ? "FRONT":"REAR"));
		break;

	case DP2_LANE_USB_MODE:
		if (is_front)
			i2c_smbus_write_byte_data(redrv_data->i2c, 0x40, 0xe0);
		else
			i2c_smbus_write_byte_data(redrv_data->i2c, 0x40, 0xf0);

		i2c_smbus_write_byte_data(redrv_data->i2c, 0xa0, 0x00);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0xa1, 0x04);

		value = i2c_smbus_read_byte_data(redrv_data->i2c, 0x40);
		pr_info("%s: DP2_LANE_USB3_MODE read 0x40 (%x) (%s)\n",
				__func__, value, (is_front ? "FRONT":"REAR"));
		break;

	case CLEAR_STATE:
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x40, 0x80);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0xa0, 0x02);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0xa1, 0x00);

		value = i2c_smbus_read_byte_data(redrv_data->i2c, 0x40);
		pr_info("%s: CLEAR_STATE read 0x40 (%x) (%s)\n",
				__func__, value, (is_front ? "FRONT":"REAR"));
		break;

	case CHECK_EXIST:
		pr_err("%s: dummy\n", __func__);
		break;

	default:
		pr_err("unknown %d\n", config);
	}

	return 0;
}
EXPORT_SYMBOL(ps5169_config);


static int ps5169_set_gpios(struct device *dev)
{
	struct device_node *np = dev->of_node;
	int ret = 0;
	int val = 0;

	if (!redrv_data) {
		pr_err("%s: Invalid redrv_data\n", __func__);
		return -ENOMEM;
	}

	redrv_data->con_sel = of_get_named_gpio(np, "combo,con_sel", 0);

	ret = of_property_read_u32(np, "combo,enable_control", &val);
	if (ret) {
		pr_info("%s: enable_control is not specified\n", __func__);
	} else {
		pr_info("%s: enable_control: %d\n", __func__, val);

		if (val) {
			redrv_data->redriver_en = of_get_named_gpio(np, "combo,redriver_en", 0);
			if (gpio_is_valid(redrv_data->redriver_en)) {
				ret = gpio_request(redrv_data->redriver_en, "ps5169_redriver_en");
				pr_info("%s: ps5169_redriver_en(%d) ret gpio_request (%d)\n",
					__func__, redrv_data->redriver_en, ret);
				ret = gpio_direction_output(redrv_data->redriver_en, 1);
				pr_info("%s: ps5169_redriver_en ret gpio_direction_output (%d)\n", __func__, ret);
				pr_info("%s: after set values, redriver_en = (%d)\n",
					__func__, gpio_get_value(redrv_data->redriver_en));
			} else
				pr_info("%s: ps5169_redriver_en gpio is invalid\n", __func__);
		} else
			pr_info("%s: skip ps5169_redriver_en gpio control\n", __func__);
	}

	np = of_find_all_nodes(NULL);
	return 0;
}

int ps5169_i2c_read(u8 command)
{
	if (!redrv_data) {
		pr_err("%s: Invalid redrv_data\n", __func__);
		return -ENOMEM;
	}
	return i2c_smbus_read_byte_data(redrv_data->i2c, command);
}

#ifdef SUPPORT_PS5169_ADBTUNE
//0x50
static ssize_t reg50_show(struct device *dev,
		struct device_attribute *attr, char *buff)
{
	return sprintf(buff, "read 0x50: 0x%02X\n",
				i2c_smbus_read_byte_data(redrv_data->i2c, 0x50));
}

static ssize_t reg50_store(struct device *dev,
		struct device_attribute *attr, const char *buff, size_t size)
{
	u8 value;

	sscanf(buff, "%hhx", &value);

	if (value < 0 || value > 0xFF)
		value = value & 0xFF;

	pr_info("write 0x50 to value 0x%02X\n", value);
	i2c_smbus_write_byte_data(redrv_data->i2c, 0x50, value);

	return size;
}

static DEVICE_ATTR(reg50, S_IRUGO | S_IWUSR, reg50_show, reg50_store);

//0x51
static ssize_t reg51_show(struct device *dev,
		struct device_attribute *attr, char *buff)
{
	return sprintf(buff, "read 0x51: 0x%02X\n",
				i2c_smbus_read_byte_data(redrv_data->i2c, 0x51));
}

static ssize_t reg51_store(struct device *dev,
		struct device_attribute *attr, const char *buff, size_t size)
{
	u8 value;

	sscanf(buff, "%hhx", &value);

	if (value < 0 || value > 0xFF)
		value = value & 0xFF;

	pr_info("write 0x51 to value 0x%02X\n", value);
	i2c_smbus_write_byte_data(redrv_data->i2c, 0x51, value);

	return size;
}

static DEVICE_ATTR(reg51, S_IRUGO | S_IWUSR, reg51_show, reg51_store);

//0x52
static ssize_t reg52_show(struct device *dev,
		struct device_attribute *attr, char *buff)
{
	return sprintf(buff, "read 0x52: 0x%02X\n",
				i2c_smbus_read_byte_data(redrv_data->i2c, 0x52));
}

static ssize_t reg52_store(struct device *dev,
		struct device_attribute *attr, const char *buff, size_t size)
{
	u8 value;

	sscanf(buff, "%hhx", &value);

	if (value < 0 || value > 0xFF)
		value = value & 0xFF;

	pr_info("write 0x52 to value 0x%02X\n", value);
	i2c_smbus_write_byte_data(redrv_data->i2c, 0x52, value);

	return size;
}

static DEVICE_ATTR(reg52, S_IRUGO | S_IWUSR, reg52_show, reg52_store);

//0x54
static ssize_t reg54_show(struct device *dev,
		struct device_attribute *attr, char *buff)
{
	return sprintf(buff, "read 0x54: 0x%02X\n",
				i2c_smbus_read_byte_data(redrv_data->i2c, 0x54));
}

static ssize_t reg54_store(struct device *dev,
		struct device_attribute *attr, const char *buff, size_t size)
{
	u8 value;

	sscanf(buff, "%hhx", &value);

	if (value < 0 || value > 0xFF)
		value = value & 0xFF;

	pr_info("write 0x54 to value 0x%02X\n", value);
	i2c_smbus_write_byte_data(redrv_data->i2c, 0x54, value);

	return size;
}

static DEVICE_ATTR(reg54, S_IRUGO | S_IWUSR, reg54_show, reg54_store);

//0x5D
static ssize_t reg5d_show(struct device *dev,
		struct device_attribute *attr, char *buff)
{
	return sprintf(buff, "read 0x5d: 0x%02X\n",
				i2c_smbus_read_byte_data(redrv_data->i2c, 0x5d));
}

static ssize_t reg5d_store(struct device *dev,
		struct device_attribute *attr, const char *buff, size_t size)
{
	u8 value;

	sscanf(buff, "%hhx", &value);

	if (value < 0 || value > 0xFF)
		value = value & 0xFF;

	pr_info("write 0x5d to value 0x%02X\n", value);
	i2c_smbus_write_byte_data(redrv_data->i2c, 0x5d, value);

	return size;
}

static DEVICE_ATTR(reg5d, S_IRUGO | S_IWUSR, reg5d_show, reg5d_store);

//0x5E
static ssize_t reg5e_show(struct device *dev,
		struct device_attribute *attr, char *buff)
{
	return sprintf(buff, "read 0x5e: 0x%02X\n",
				i2c_smbus_read_byte_data(redrv_data->i2c, 0x5e));
}

static ssize_t reg5e_store(struct device *dev,
		struct device_attribute *attr, const char *buff, size_t size)
{
	u8 value;

	sscanf(buff, "%hhx", &value);

	if (value < 0 || value > 0xFF)
		value = value & 0xFF;

	pr_info("write 0x5e to value 0x%02X\n", value);
	i2c_smbus_write_byte_data(redrv_data->i2c, 0x5e, value);

	return size;
}

static DEVICE_ATTR(reg5e, S_IRUGO | S_IWUSR, reg5e_show, reg5e_store);

//REG READ
u32 reg_addr = 0;

static ssize_t regrd_show(struct device *dev,
		struct device_attribute *attr, char *buff)
{
	if (reg_addr < 0 || reg_addr > 0xFF)
		reg_addr = reg_addr & 0xFF;
	return sprintf(buff, "read reg 0X%02X: 0x%02X\n", reg_addr,
				i2c_smbus_read_byte_data(redrv_data->i2c, reg_addr));
}

static ssize_t regrd_store(struct device *dev,
		struct device_attribute *attr, const char *buff, size_t size)
{
	u8 value;

	sscanf(buff, "%hhx", &value);

	if (value < 0 || value > 0xFF)
		reg_addr = value & 0xFF;
	else
		reg_addr = value;

	pr_info("read address = 0x%02X\n", reg_addr);

	return size;
}

static DEVICE_ATTR(regrd, S_IRUGO | S_IWUSR, regrd_show,
					       regrd_store);
#endif

static int ps5169_probe(struct i2c_client *i2c,
			       const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(i2c->dev.parent);
	int ret = 0;

	pr_info("%s\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&i2c->dev, "i2c functionality check error\n");
		return -EIO;
	}

	redrv_data = devm_kzalloc(&i2c->dev, sizeof(*redrv_data), GFP_KERNEL);

	if (!redrv_data)
		return -ENOMEM;

	if (i2c->dev.of_node)
		ps5169_set_gpios(&i2c->dev);
	else
		dev_err(&i2c->dev, "not found ps5169 dt ret:%d\n", ret);

	redrv_data->dev = &i2c->dev;
	redrv_data->i2c = i2c;
	i2c_set_clientdata(i2c, redrv_data);

	mutex_init(&redrv_data->i2c_mutex);

	if (ps5169_i2c_check() < 0) {
		pr_info("%s: i2c transfer failed. stop to try i2c transfer\n", __func__);
		redrv_data = NULL;
	} else 
		ps5169_config(WORK_MODE, 0);

	return ret;
}

static int ps5169_remove(struct i2c_client *i2c)
{
	struct ps5169_data *redrv_data = i2c_get_clientdata(i2c);

	if (redrv_data->i2c) {
		mutex_destroy(&redrv_data->i2c_mutex);
		i2c_set_clientdata(redrv_data->i2c, NULL);
	}
	return 0;
}


static void ps5169_shutdown(struct i2c_client *i2c)
{
	;
}

#define PS5169_DEV_NAME  "ps5169"


static const struct i2c_device_id ps5169_id[] = {
	{ PS5169_DEV_NAME, 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, ps5169_id);


static const struct of_device_id ps5169_i2c_dt_ids[] = {
	{ .compatible = "ps5169_driver" },
	{ }
};


static struct i2c_driver ps5169_driver = {
	.driver = {
		.name	= PS5169_DEV_NAME,
		.of_match_table	= ps5169_i2c_dt_ids,
	},
	.probe		= ps5169_probe,
	.remove		= ps5169_remove,
	.shutdown	= ps5169_shutdown,
	.id_table	= ps5169_id,
};

#ifdef SUPPORT_PS5169_ADBTUNE
static struct attribute *ps5169_attributes[] = {
	&dev_attr_reg50.attr,
	&dev_attr_reg51.attr,
	&dev_attr_reg52.attr,
	&dev_attr_reg54.attr,
	&dev_attr_reg5d.attr,
	&dev_attr_reg5e.attr,
	&dev_attr_regrd.attr,
	NULL
};

const struct attribute_group ps5169_sysfs_group = {
	.attrs = ps5169_attributes,
};
#endif

static int __init ps5169_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&ps5169_driver);
	pr_info("%s ret is %d\n", __func__, ret);

#ifdef SUPPORT_PS5169_ADBTUNE
	redriver_device = sec_device_create(NULL, "redriver");
	if (IS_ERR(redriver_device))
		pr_err("%s Failed to create device(redriver)!\n", __func__);

	ret = sysfs_create_group(&redriver_device->kobj, &ps5169_sysfs_group);
	if (ret)
		pr_err("%s: sysfs_create_group fail, ret %d", __func__, ret);
#endif
	return ret;
}
module_init(ps5169_init);

static void __exit ps5169_exit(void)
{
	i2c_del_driver(&ps5169_driver);
}
module_exit(ps5169_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ps5169 combo redriver driver");
