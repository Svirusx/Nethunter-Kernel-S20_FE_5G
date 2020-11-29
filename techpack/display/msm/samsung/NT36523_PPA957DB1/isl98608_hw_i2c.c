/*
 * isl98608_hw_i2c.c - Platform data for isl98608 buck driver
 *
 * Copyright (C) 2011 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/kernel.h>
#include <linux/of_gpio.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/device.h>

#include "../ss_dsi_panel_common.h"

struct isl98608_buck_info {
	struct i2c_client			*client;
};

static struct isl98608_buck_info buck_pinfo;

static int buck_i2c_read(struct i2c_client *client, u8 addr,  u8 *value)
{
	int retry = 3;
	u8 wr[] = {addr};

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.buf = wr,
			.len = 1
		}, {
			.addr = client->addr,
			.flags = I2C_M_RD,
			.buf = value,
			.len = 1
		},
	};
	int ret;

	do {
		ret = i2c_transfer(client->adapter, msg, 2);
		if (ret != 2) {
			LCD_ERR("client->addr 0x%02x read_addr 0x%02x error (ret == %d)\n", client->addr, addr, ret);
		} else
			break;
	} while (--retry);

	return ret;
}

static int buck_i2c_write(struct i2c_client *client, u8 addr,  u8 value)
{
	int retry = 3;
	u8 wr[] = {addr, value};
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.buf = wr,
			.len = 2
		}
	};
	int ret;

	do {
		ret = i2c_transfer(client->adapter, msg, 1);
		if (ret != 1) {
			LCD_ERR("addr 0x%02x value 0x%02x error (ret == %d)\n",
				 addr, value, ret);
		} else
			break;
	} while (--retry);

	return ret;
}

int isl98608_buck_control(struct samsung_display_driver_data *vdd)
{
	u8 buf[2];
	u8 value = 0x02; //VN & VP 5.5V

	if (!buck_pinfo.client) {
		LCD_ERR("error pinfo\n");
		return 0;
	}

	if (unlikely(vdd->is_factory_mode)) {
		if (ub_con_det_status(PRIMARY_DISPLAY_NDX)) {
			LCD_ERR("Do not panel power on..\n");
			return 0;
		}
	}

	buck_i2c_write(buck_pinfo.client, 0x08, value);
	buck_i2c_write(buck_pinfo.client, 0x09, value);

	buck_i2c_read(buck_pinfo.client, 0x08, &buf[0]);
	buck_i2c_read(buck_pinfo.client, 0x09, &buf[1]);

	if ((value != buf[0]) || (value != buf[1]))
		LCD_INFO("0x%x 0x%x fail\n", buf[0], buf[1]);
	else
		LCD_INFO("0x%x 0x%x success\n", buf[0], buf[1]);

	return 0;
}

static int isl98608_buck_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

	LCD_INFO("\n");

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		LCD_INFO("i2c check fail\n");
		return -EIO;
	}

	buck_pinfo.client = client;

	i2c_set_clientdata(client, &buck_pinfo);

	return 0;
}

static int isl98608_buck_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id isl98608_buck_id[] = {
	{"isl98608", 0},
};
MODULE_DEVICE_TABLE(i2c, isl98608_buck_id);

static struct of_device_id isl98608_buck_match_table[] = {
	{ .compatible = "isl98608,display_buck",},
};

struct i2c_driver isl98608_buck_driver = {
	.probe = isl98608_buck_probe,
	.remove = isl98608_buck_remove,
	.driver = {
		.name = "isl98608",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(isl98608_buck_match_table),
#endif
		   },
	.id_table = isl98608_buck_id,
};

static int __init isl98608_buck_init(void)
{

	int ret = 0;

	ret = i2c_add_driver(&isl98608_buck_driver);
	if (ret) {
		LCD_ERR("isl98608_buck_init registration failed. ret= %d\n", ret);
	}

	return ret;
}

static void __exit isl98608_buck_exit(void)
{
	i2c_del_driver(&isl98608_buck_driver);
}

module_init(isl98608_buck_init);
module_exit(isl98608_buck_exit);

MODULE_DESCRIPTION("isl98608_hw_i2c driver");
MODULE_LICENSE("GPL");
