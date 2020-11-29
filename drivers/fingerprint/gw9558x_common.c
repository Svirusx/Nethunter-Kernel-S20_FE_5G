/*
 *  Copyright (C) 2018, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include "fingerprint.h"
#include "gw9558x_common.h"

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

static const struct of_device_id gw9558_of_match[] = {
#ifndef CONFIG_SEC_FACTORY
	{ .compatible = "goodix,gw9558x", },
#else
	{ .compatible = "goodix,gw9558x_factory", },
#endif
	{},
};
MODULE_DEVICE_TABLE(of, gw9558_of_match);

extern int fingerprint_register(struct device *dev, void *drvdata,
	struct device_attribute *attributes[], char *name);
extern void fingerprint_unregister(struct device *dev,
	struct device_attribute *attributes[]);

static struct gf_device *g_data;

static ssize_t gw9558_read(struct file *filp, char __user *buf,
		size_t count, loff_t *f_pos)
{
	return -EFAULT;
}

static ssize_t gw9558_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *f_pos)
{
	return -EFAULT;
}

static long gw9558_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct gf_device *gf_dev = NULL;
	unsigned int onoff = 0;
	int retval = 0;
	u8 temp = 0;

	if (_IOC_TYPE(cmd) != GF_IOC_MAGIC)
		return -EINVAL;

	/* Check access direction once here; don't repeat below.
	 * IOC_DIR is from the user perspective, while access_ok is
	 * from the kernel perspective; so they look reversed.
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		retval = !access_ok(VERIFY_WRITE, (void __user *)arg,
				_IOC_SIZE(cmd));

	if (retval == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		retval = !access_ok(VERIFY_READ, (void __user *)arg,
				_IOC_SIZE(cmd));

	if (retval) {
		pr_err("access NOK\n");
		return -EINVAL;
	}

	gf_dev = (struct gf_device *)filp->private_data;
	if (!gf_dev) {
		pr_err("gf_dev IS NULL\n");
		return -EINVAL;
	}

	switch (cmd) {
	case GF_IOC_INIT:
		pr_debug("GF_IOC_INIT\n");
		if (copy_to_user((void __user *)arg, (void *)&temp, sizeof(temp)))
			retval = -EFAULT;
		break;

	case GF_IOC_RESET:
		pr_info("GF_IOC_RESET\n");
		gw9558_hw_reset(gf_dev, 0);
		break;

	case GF_IOC_ENABLE_SPI_CLK:
		if (copy_from_user(&onoff, (void __user *)arg,
					sizeof(onoff))) {
			pr_err("Failed to copy spi_speed value from user to kernel\n");
		}
		pr_info("GF_IOC_ENABLE_SPI_CLK : %d, fromUser : %d\n",
				gf_dev->spi_speed, onoff);
		gw9558_spi_clk_enable(gf_dev);
		break;

	case GF_IOC_DISABLE_SPI_CLK:
		pr_info("GF_IOC_DISABLE_SPI_CLK\n");
		gw9558_spi_clk_disable(gf_dev);
		break;

	case GF_IOC_ENABLE_POWER:
		pr_info("GF_IOC_ENABLE_POWER\n");
		gw9558_hw_power_enable(gf_dev, 1);
		break;

	case GF_IOC_DISABLE_POWER:
		pr_info("GF_IOC_DISABLE_POWER\n");
		gw9558_hw_power_enable(gf_dev, 0);
		break;

	case GF_IOC_POWER_CONTROL:
		if (copy_from_user(&onoff, (void __user *)arg,
					sizeof(onoff))) {
			pr_err("Failed to copy onoff value from user to kernel\n");
			retval = -EFAULT;
			break;
		}
		pr_info("GF_IOC_POWER_CONTROL %d\n", onoff);
		gw9558_hw_power_enable(gf_dev, onoff);
		break;

	case GF_IOC_GET_FW_INFO:
		temp = 1;
		pr_debug("GET_FW_INFO : 0x%x\n", temp);
		if (copy_to_user((void __user *)arg, (void *)&temp,
					sizeof(temp))) {
			pr_err("Failed to copy data to user\n");
			retval = -EFAULT;
		}
		break;

#ifndef ENABLE_SENSORS_FPRINT_SECURE
	case GF_IOC_TRANSFER_RAW_CMD:
		mutex_lock(&gf_dev->buf_lock);
		retval = gw9558_ioctl_transfer_raw_cmd(gf_dev, arg, (unsigned int)TANSFER_MAX_LEN);
		mutex_unlock(&gf_dev->buf_lock);
		break;
#endif /* !ENABLE_SENSORS_FPRINT_SECURE */
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	case GF_IOC_SET_SENSOR_TYPE:
		if (copy_from_user(&onoff, (void __user *)arg,
				   sizeof(onoff)) != 0) {
			pr_err("Failed to copy sensor type from user to kernel\n");
			return -EFAULT;
		}
		if ((int)onoff >= SENSOR_OOO && (int)onoff < SENSOR_MAXIMUM) {
			if ((int)onoff == SENSOR_OOO && gf_dev->sensortype == SENSOR_FAILED) {
				pr_err("Maintain type check from out of oder :%s\n",
					sensor_status[gf_dev->sensortype + 2]);
			} else {
				gf_dev->sensortype = (int)onoff;
				pr_info("SET_SENSOR_TYPE :%s\n",
					sensor_status[gf_dev->sensortype + 2]);
			}
		} else {
			pr_err("SET_SENSOR_TYPE : invalid value %d\n", (int)onoff);
			gf_dev->sensortype = SENSOR_UNKNOWN;
		}
		break;
#endif
	case GF_IOC_SPEEDUP:
		if (copy_from_user(&onoff, (void __user *)arg,
				   sizeof(onoff)) != 0) {
			pr_err("Failed to copy speedup from user to kernel\n");
			return -EFAULT;
		}
		retval = gw9558_set_cpu_speedup(gf_dev, onoff);
		break;

	case GF_MODEL_INFO:
		pr_info("GF_MODEL_INFO : %s\n", gf_dev->model_info);
		if (copy_to_user((void __user *)arg, gf_dev->model_info, 10)) {
			pr_err("Failed to copy data to user\n");
			retval = -EFAULT;
		}
		break;

	case GF_IOC_RESERVED01:
	case GF_IOC_RESERVED02:
	case GF_IOC_RESERVED03:
	case GF_IOC_RESERVED04:
	case GF_IOC_RESERVED05:
	case GF_IOC_RESERVED06:
	case GF_IOC_RESERVED07:
		break;

	default:
		pr_err("doesn't support this command(%x)\n", cmd);
		break;
	}

	return retval;
}

#ifdef CONFIG_COMPAT
static long gw9558_compat_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	int retval = 0;

	retval = filp->f_op->unlocked_ioctl(filp, cmd, arg);

	return retval;
}
#endif

static int gw9558_open(struct inode *inode, struct file *filp)
{
	struct gf_device *gf_dev = NULL;
	int retval = -ENXIO;

	pr_info("Entry\n");
	mutex_lock(&device_list_lock);
	list_for_each_entry(gf_dev, &device_list, device_entry) {
		if (gf_dev->devno == inode->i_rdev) {
			pr_info("Found\n");
			retval = 0;
			break;
		}
	}
	mutex_unlock(&device_list_lock);

	if (retval == 0) {
		filp->private_data = gf_dev;
		nonseekable_open(inode, filp);
		pr_info("Success to open device\n");
	} else {
		pr_err("No device for minor %d\n",iminor(inode));
	}
	return retval;
}

static int gw9558_release(struct inode *inode, struct file *filp)
{
	struct gf_device *gf_dev = NULL;

	pr_info("Entry\n");
	gf_dev = filp->private_data;
	return 0;
}

static const struct file_operations gw9558_fops = {
	.owner =	THIS_MODULE,
	.write =	gw9558_write,
	.read =		gw9558_read,
	.unlocked_ioctl = gw9558_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = gw9558_compat_ioctl,
#endif
	.open =		gw9558_open,
	.release =	gw9558_release,
};

static void gw9558_work_func_debug(struct work_struct *work)
{
	struct gf_device *gf_dev = NULL;
	u8 rst_value = -1;

	gf_dev = container_of(work, struct gf_device, work_debug);
	if (gf_dev->reset_gpio)
		rst_value = gpio_get_value(gf_dev->reset_gpio);

	pr_info("ldo: %d, sleep: %d, tz: %d type: %s\n",
		gf_dev->ldo_onoff, rst_value, gf_dev->tz_mode,
		sensor_status[gf_dev->sensortype + 2]);
}

static void gw9558_enable_debug_timer(struct gf_device *gf_dev)
{
	mod_timer(&g_data->dbg_timer,
		round_jiffies_up(jiffies + FPSENSOR_DEBUG_TIMER_SEC));
}

static void gw9558_disable_debug_timer(struct gf_device *gf_dev)
{
	del_timer_sync(&g_data->dbg_timer);
	cancel_work_sync(&g_data->work_debug);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
static void gw9558_timer_func(unsigned long ptr)
#else
static void gw9558_timer_func(struct timer_list *t)
#endif
{
	queue_work(g_data->wq_dbg, &g_data->work_debug);
	mod_timer(&g_data->dbg_timer,
		round_jiffies_up(jiffies + FPSENSOR_DEBUG_TIMER_SEC));
}

static int gw9558_set_timer(struct gf_device *gf_dev)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
	setup_timer(&gf_dev->dbg_timer, gw9558_timer_func, (unsigned long)gf_dev);
#else
	timer_setup(&gf_dev->dbg_timer, gw9558_timer_func, 0);
#endif
	gf_dev->wq_dbg = create_singlethread_workqueue("gw9558_debug_wq");
	if (!gf_dev->wq_dbg) {
		pr_err("could not create workqueue\n");
		return -ENOMEM;
	}
	INIT_WORK(&gf_dev->work_debug, gw9558_work_func_debug);
	return 0;
}

void gw9558_hw_power_enable(struct gf_device *gf_dev, u8 onoff)
{
	int retval = 0;

	if (onoff && !gf_dev->ldo_onoff) {
		gw9558_pin_control(gf_dev, 1);
		if (gf_dev->pwr_gpio) {
			gpio_set_value(gf_dev->pwr_gpio, 1);
		} else	if (gf_dev->regulator_3p3 != NULL) {
			retval = regulator_enable(gf_dev->regulator_3p3);
			if (retval) {
				pr_err("regulator enable failed, rc=%d\n", retval);
				goto regulator_failed;
			}
		}
		if (gf_dev->reset_gpio) {
			usleep_range(11000, 11050);
			gpio_set_value(gf_dev->reset_gpio, 1);
		}
		gf_dev->ldo_onoff = 1;
	} else if (!onoff && gf_dev->ldo_onoff) {
		if (gf_dev->reset_gpio) {
			gpio_set_value(gf_dev->reset_gpio, 0);
			usleep_range(11000, 11050);
		}
		if (gf_dev->pwr_gpio) {
			gpio_set_value(gf_dev->pwr_gpio, 0);
		} else if (gf_dev->regulator_3p3 != NULL) {
			retval = regulator_disable(gf_dev->regulator_3p3);
			if (retval) {
				pr_err("regulator disable failed, rc=%d\n", retval);
				if (gf_dev->reset_gpio)
					gpio_set_value(gf_dev->reset_gpio, 1);
				goto regulator_failed;
			}
		}
		gf_dev->ldo_onoff = 0;
		gw9558_pin_control(gf_dev, 0);
	} else if (onoff == 0 || onoff == 1) {
		pr_err("power is already %s\n",
				(gf_dev->ldo_onoff ? "Enabled" : "Disabled"));
	} else {
		pr_err("can't support this value:%d\n", onoff);
	}
	pr_debug("status = %d\n", gf_dev->ldo_onoff);
regulator_failed:
	return;
}

void gw9558_hw_reset(struct gf_device *gf_dev, u8 delay)
{
	if (gf_dev->reset_gpio) {
		gpio_set_value(gf_dev->reset_gpio, 0);
		usleep_range(3000, 3050);
		gpio_set_value(gf_dev->reset_gpio, 1);
		usleep_range(delay * 1000, delay * 1000 + 50);
		gf_dev->reset_count++;
	}
}

/* GPIO pins reference */
int gw9558_get_gpio_dts_info(struct device *dev, struct gf_device *gf_dev)
{
	struct device_node *np = dev->of_node;
	int retval = 0;

    /* get pwr resource */
	gf_dev->pwr_gpio = of_get_named_gpio(np, "goodix,gpio_pwr", 0);
	if (!gpio_is_valid(gf_dev->pwr_gpio)) {
		pr_info("not use PWR GPIO\n");
		gf_dev->pwr_gpio = 0;
	} else {
		pr_info("goodix_pwr:%d\n", gf_dev->pwr_gpio);
		retval = gpio_request(gf_dev->pwr_gpio, "goodix_pwr");
		if (retval < 0) {
			pr_err("Failed to request PWR GPIO = %d\n", retval);
			return retval;
		}
		gpio_direction_output(gf_dev->pwr_gpio, 0);
	}

	if (of_property_read_string(np, "goodix,btp-regulator", &gf_dev->btp_vdd) < 0) {
		pr_info("not use btp_regulator\n");
		gf_dev->btp_vdd = NULL;
	} else {
		gf_dev->regulator_3p3 = regulator_get(NULL, gf_dev->btp_vdd);
		if (IS_ERR(gf_dev->regulator_3p3) || (gf_dev->regulator_3p3) == NULL) {
			pr_info("not use regulator_3p3\n");
			gf_dev->regulator_3p3 = NULL;
		} else {
			pr_info("btp_regulator ok\n");
		}
	}

    /* get reset resource */
	gf_dev->reset_gpio = of_get_named_gpio(np, "goodix,gpio_reset", 0);
	if (!gpio_is_valid(gf_dev->reset_gpio)) {
		pr_err("RESET GPIO is invalid.\n");
		gf_dev->reset_gpio = 0;
		return -EINVAL;
	} else {
		pr_info("goodix_reset:%d\n", gf_dev->reset_gpio);
		retval = gpio_request(gf_dev->reset_gpio, "goodix_reset");
		if (retval < 0) {
			pr_err("Failed to request RESET GPIO = %d\n", retval);
			return retval;
		}
		gpio_direction_output(gf_dev->reset_gpio, 0);
	}

	if (of_property_read_string_index(np, "goodix,chip_id", 0,
			(const char **)&gf_dev->chipid))
		gf_dev->chipid = "NULL";
	pr_info("Chip ID:%s\n", gf_dev->chipid);

	if (of_property_read_string_index(np, "goodix,position", 0,
			(const char **)&gf_dev->sensor_position)) {
		gf_dev->sensor_position = "14.31,0.00,9.10,9.10,14.80,14.80,12.00,12.00,5.00";
	}
	pr_info("position: %s\n", gf_dev->sensor_position);

	if (of_property_read_string_index(np, "goodix,rb", 0,
			(const char **)&gf_dev->rb)) {
		gf_dev->rb = "525,-1,-1";
	}
	pr_info("rb: %s\n", gf_dev->rb);

	if (of_property_read_string_index(np, "goodix,modelinfo", 0,
			(const char **)&gf_dev->model_info)) {
		gf_dev->model_info = "NONE";
	}
	pr_info("modelinfo: %s\n", gf_dev->model_info);

	gf_dev->p = pinctrl_get_select_default(dev);
	if (IS_ERR(gf_dev->p)) {
		pr_err("failed pinctrl_get\n");
		return -EINVAL;
	}
#ifndef ENABLE_SENSORS_FPRINT_SECURE
	gf_dev->pins_poweroff = pinctrl_lookup_state(gf_dev->p, "pins_poweroff");
	if (IS_ERR(gf_dev->pins_poweroff)) {
		pr_err("could not get pins sleep_state (%li)\n",
			PTR_ERR(gf_dev->pins_poweroff));
	}

	gf_dev->pins_poweron = pinctrl_lookup_state(gf_dev->p, "pins_poweron");
	if (IS_ERR(gf_dev->pins_poweron)) {
		pr_err("could not get pins idle_state (%li)\n",
			PTR_ERR(gf_dev->pins_poweron));
	}
#endif
	return retval;
}

void gw9558_cleanup_info(struct gf_device *gf_dev)
{
	if (gpio_is_valid(gf_dev->reset_gpio)) {
		gpio_free(gf_dev->reset_gpio);
		pr_debug("remove reset_gpio\n");
	}
	if (gpio_is_valid(gf_dev->pwr_gpio)) {
		gpio_free(gf_dev->pwr_gpio);
		pr_debug("remove pwr_gpio\n");
	}
	if (gf_dev->regulator_3p3) {
		regulator_put(gf_dev->regulator_3p3);
		pr_debug("remove regulator\n");
	}
}

#ifndef ENABLE_SENSORS_FPRINT_SECURE
int gw9558_type_check(struct gf_device *gf_dev)
{
	int retval = -ENODEV;
	unsigned char mcuid[4] = {0x00, 0x00, 0x00, 0x00};
	u32 mcuid32 = 0;

	gw9558_hw_power_enable(gf_dev, 1);
	usleep_range(4950, 5000);
	gw9558_hw_reset(gf_dev, 50);

	gw9558_spi_read_bytes(gf_dev, 0x0000, 4, mcuid);
	mcuid32 = (mcuid[2] << 24 | mcuid[3] << 16 |
			mcuid[0] << 8 | mcuid[1]);
	pr_info("Sensor read : %x %x %x %x\n",
			mcuid[0], mcuid[1], mcuid[2], mcuid[3]);
	pr_info("Sensor read : 0x%8x\n", mcuid32);
	if (mcuid32 == G3_MCU_ID) {
		gf_dev->sensortype = SENSOR_GOODIXOPTICAL;
		pr_info("sensor type is G3 %s\n",
				sensor_status[gf_dev->sensortype + 2]);
		retval = 0;
	} else if (mcuid32 == GX_MCU_ID) {
		gf_dev->sensortype = SENSOR_GOODIXOPTICAL;
		pr_info("sensor type is G3X %s\n",
				sensor_status[gf_dev->sensortype + 2]);
		retval = 0;
	} else {
		gf_dev->sensortype = SENSOR_FAILED;
		pr_err("sensor type is FAILED 0x%8x\n", mcuid32);
	}
	gw9558_hw_power_enable(gf_dev, 0);

	return retval;
}
#endif

static ssize_t gw9558_bfs_values_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct gf_device *gf_dev = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "\"FP_SPICLK\":\"%d\"\n",
			gf_dev->spi_speed);
}

static ssize_t gw9558_type_check_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct gf_device *gf_dev = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", gf_dev->sensortype);
}

static ssize_t gw9558_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", "GOODIX");
}

static ssize_t gw9558_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gf_device *gf_dev = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%s\n", gf_dev->chipid);
}

static ssize_t gw9558_adm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", DETECT_ADM);
}

static ssize_t gw9558_intcnt_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct gf_device *gf_dev = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", gf_dev->interrupt_count);
}

static ssize_t gw9558_intcnt_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	struct gf_device *gf_dev = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "c")) {
		gf_dev->interrupt_count = 0;
		pr_info("initialization is done\n");
	}
	return size;
}

static ssize_t gw9558_resetcnt_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct gf_device *gf_dev = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", gf_dev->reset_count);
}

static ssize_t gw9558_resetcnt_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	struct gf_device *gf_dev = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "c")) {
		gf_dev->reset_count = 0;
		pr_info("initialization is done\n");
	}
	return size;
}

static ssize_t gw9558_position_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gf_device *gf_dev = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%s\n", gf_dev->sensor_position);
}

static ssize_t gw9558_rb_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gf_device *gf_dev = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%s\n", gf_dev->rb);
}

static DEVICE_ATTR(bfs_values, 0444, gw9558_bfs_values_show, NULL);
static DEVICE_ATTR(type_check, 0444, gw9558_type_check_show, NULL);
static DEVICE_ATTR(vendor, 0444,	gw9558_vendor_show, NULL);
static DEVICE_ATTR(name, 0444, gw9558_name_show, NULL);
static DEVICE_ATTR(adm, 0444, gw9558_adm_show, NULL);
static DEVICE_ATTR(intcnt, 0664, gw9558_intcnt_show, gw9558_intcnt_store);
static DEVICE_ATTR(resetcnt, 0664, gw9558_resetcnt_show, gw9558_resetcnt_store);
static DEVICE_ATTR(position, 0444, gw9558_position_show, NULL);
static DEVICE_ATTR(rb, 0444, gw9558_rb_show, NULL);

static struct device_attribute *fp_attrs[] = {
	&dev_attr_bfs_values,
	&dev_attr_type_check,
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_adm,
	&dev_attr_intcnt,
	&dev_attr_resetcnt,
	&dev_attr_position,
	&dev_attr_rb,
	NULL,
};

static int gw9558_probe_common(struct device *dev, struct gf_device *gf_dev)
{
	int retval = -EINVAL;
#ifndef ENABLE_SENSORS_FPRINT_SECURE
	int retry = 0;
#endif
	pr_info("Entry\n");

	spin_lock_init(&gf_dev->spi_lock);
	INIT_LIST_HEAD(&gf_dev->device_entry);

	/* Initialize the driver data */
	gf_dev->device_count = 0;
	gf_dev->ldo_onoff = 0;
	gf_dev->reset_count = 0;
	gf_dev->interrupt_count = 0;
	g_data = gf_dev;

	/* get gpio info from dts or defination */
	retval = gw9558_get_gpio_dts_info(dev, gf_dev);
	if (retval < 0) {
		pr_err("Failed to get gpio info:%d\n", retval);
		goto gw9558_probe_get_gpio;
	}
	gw9558_spi_setup_conf(gf_dev, 1);

	/* set AP spectific configuration */
	retval = gw9558_register_platform_variable(gf_dev);
	if (retval < 0) {
		pr_err("Failed to set platform_variable:%d\n", retval);
		goto gw9558_probe_set_platform;
	}
	/* create class */
	gf_dev->class = class_create(THIS_MODULE, GF_CLASS_NAME);
	if (IS_ERR(gf_dev->class)) {
		pr_err("Failed to create class.\n");
		retval = -ENODEV;
		goto gw9558_probe_class_create;
	}

	/* get device no */
	if (GF_DEV_MAJOR > 0) {
		gf_dev->devno = MKDEV(GF_DEV_MAJOR, gf_dev->device_count++);
		retval = register_chrdev_region(gf_dev->devno, 1, GF_DEV_NAME);
	} else {
		retval = alloc_chrdev_region(&gf_dev->devno,
				gf_dev->device_count++, 1, GF_DEV_NAME);
	}
	if (retval < 0) {
		pr_err("Failed to alloc devno.\n");
		goto gw9558_probe_devno;
	} else {
		pr_info("major=%d, minor=%d\n",
				MAJOR(gf_dev->devno), MINOR(gf_dev->devno));
	}

	/* create device */
	gf_dev->fp_device = device_create(gf_dev->class, dev,
			gf_dev->devno, gf_dev, GF_DEV_NAME);
	if (IS_ERR(gf_dev->fp_device)) {
		pr_err("Failed to create device.\n");
		retval = -ENODEV;
		goto gw9558_probe_device;
	} else {
		mutex_lock(&device_list_lock);
		list_add(&gf_dev->device_entry, &device_list);
		mutex_unlock(&device_list_lock);
		pr_info("device create success.\n");
	}

#ifndef ENABLE_SENSORS_FPRINT_SECURE
	do {
		retval = gw9558_type_check(gf_dev);
		pr_info("type (%u), retry (%d)\n", gf_dev->sensortype, retry);
	} while (!gf_dev->sensortype && ++retry < 3);

	if (retval == -ENODEV)
		pr_err("type_check failed\n");
#endif

	/* create sysfs */
	retval = fingerprint_register(gf_dev->fp_device,
		gf_dev, fp_attrs, "fingerprint");
	if (retval) {
		pr_err("sysfs register failed\n");
		goto gw9558_probe_sysfs;
	}

	/* cdev init and add */
	cdev_init(&gf_dev->cdev, &gw9558_fops);
	gf_dev->cdev.owner = THIS_MODULE;
	retval = cdev_add(&gf_dev->cdev, gf_dev->devno, 1);
	if (retval) {
		pr_err("Failed to add cdev.\n");
		goto gw9558_probe_cdev;
	}

	wake_lock_init(&gf_dev->wake_lock, WAKE_LOCK_SUSPEND, "gw9558_wake_lock");

	retval = gw9558_set_timer(gf_dev);
	if (retval)
		goto gw9558_probe_debug_timer;
	gw9558_enable_debug_timer(gf_dev);

	pr_info("probe finished\n");
	return 0;

gw9558_probe_debug_timer:
	cdev_del(&gf_dev->cdev);
gw9558_probe_cdev:
	fingerprint_unregister(gf_dev->fp_device, fp_attrs);
gw9558_probe_sysfs:
	device_destroy(gf_dev->class, gf_dev->devno);
	list_del(&gf_dev->device_entry);
gw9558_probe_device:
	unregister_chrdev_region(gf_dev->devno, 1);
gw9558_probe_devno:
	class_destroy(gf_dev->class);
gw9558_probe_class_create:
gw9558_probe_set_platform:
gw9558_probe_get_gpio:
	gw9558_unregister_platform_variable(gf_dev);
	pr_err("failed. %d", retval);
	return retval;
}

#ifdef ENABLE_SENSORS_FPRINT_SECURE
static int gw9558_probe(struct platform_device *pdev)
{
	int retval = 0;
	struct gf_device *gf_dev;

	pr_info("platform_driver Entry\n");
	/* Allocate driver data */
	gf_dev = kzalloc(sizeof(struct gf_device), GFP_KERNEL);
	if (!gf_dev)
		return -ENOMEM;

	gf_dev->enabled_clk = 0;
	gf_dev->tz_mode = true;
	gf_dev->sensortype = SENSOR_UNKNOWN;
	gf_dev->spi_speed = GW9558_SPI_BAUD_RATE;

	retval = gw9558_probe_common(&pdev->dev, gf_dev);
	if(retval)
		goto gw9558_platform_probe_failed;

	pr_info("is successful\n");
	return retval;

gw9558_platform_probe_failed:
	kfree(gf_dev);
	gf_dev = NULL;
	pr_err("is failed : %d\n", retval);
	return retval;
}
#else
static int gw9558_probe(struct spi_device *spi)
{
	int retval = 0;
	struct gf_device *gf_dev;

	pr_info("spi_driver Entry\n");
	/* Allocate driver data */
	gf_dev = kzalloc(sizeof(struct gf_device), GFP_KERNEL);
	if (!gf_dev)
		return -ENOMEM;

	spi->mode = SPI_MODE_0;
	spi->max_speed_hz = GW9558_SPI_BAUD_RATE;
	spi->bits_per_word = 8;
	gf_dev->spi_speed = GW9558_SPI_BAUD_RATE;
	gf_dev->prev_bits_per_word = 8;
	gf_dev->tz_mode = false;
	gf_dev->spi = spi;
	spi_set_drvdata(spi, gf_dev);
	mutex_init(&gf_dev->buf_lock);

	if (spi_setup(gf_dev->spi)) {
		pr_err("failed to setup spi conf\n");
		goto gw9558_spi_probe_spi_setup;
	} else {
		pr_info("setup spi success.\n");
	}

	/* init transfer buffer */
	retval = gw9558_init_buffer(gf_dev);
	if (retval < 0) {
		pr_err("Failed to Init transfer buffer.\n");
		goto gw9558_spi_probe_transfer_buffer_init;
	}

	retval = gw9558_probe_common(&spi->dev, gf_dev);
	if(retval)
		goto gw9558_spi_probe_failed;

	pr_info("is successful\n");
	return retval;

gw9558_spi_probe_failed:
	gw9558_free_buffer(gf_dev);
gw9558_spi_probe_transfer_buffer_init:
gw9558_spi_probe_spi_setup:
	mutex_destroy(&gf_dev->buf_lock);
	spi_set_drvdata(spi, NULL);
	gf_dev->spi = NULL;
	kfree(gf_dev);
	gf_dev = NULL;
	pr_err("is failed : %d\n", retval);
	return retval;
}
#endif

static int gw9558_remove_common(struct device *dev)
{
	struct gf_device *gf_dev = dev_get_drvdata(dev);

	pr_info("Entry\n");

	gw9558_hw_power_enable(gf_dev, 0);
	gw9558_unregister_platform_variable(gf_dev);
	gw9558_disable_debug_timer(gf_dev);
	wake_lock_destroy(&gf_dev->wake_lock);
	gw9558_cleanup_info(gf_dev);
	fingerprint_unregister(gf_dev->fp_device, fp_attrs);
	cdev_del(&gf_dev->cdev);
	device_destroy(gf_dev->class, gf_dev->devno);
	list_del(&gf_dev->device_entry);

	unregister_chrdev_region(gf_dev->devno, 1);
	class_destroy(gf_dev->class);
	return 0;
}

#ifdef ENABLE_SENSORS_FPRINT_SECURE
static int gw9558_remove(struct platform_device *pdev)
{
	struct gf_device *gf_dev = dev_get_drvdata(&pdev->dev);

	gw9558_remove_common(&pdev->dev);

	kfree(gf_dev);
	gf_dev = NULL;
	return 0;
}
#else
static int gw9558_remove(struct spi_device *spi)
{
	struct gf_device *gf_dev = spi_get_drvdata(spi);

	gw9558_free_buffer(gf_dev);
	gw9558_remove_common(&spi->dev);

	mutex_destroy(&gf_dev->buf_lock);
	spin_lock_irq(&gf_dev->spi_lock);
	spi_set_drvdata(spi, NULL);
	gf_dev->spi = NULL;
	spin_unlock_irq(&gf_dev->spi_lock);
	kfree(gf_dev);
	gf_dev = NULL;
	return 0;
}
#endif

static int gw9558_pm_suspend(struct device *dev)
{
	struct gf_device *gf_dev = dev_get_drvdata(dev);

	pr_info("Entry\n");
	gw9558_disable_debug_timer(gf_dev);
	return 0;
}

static int gw9558_pm_resume(struct device *dev)
{
	struct gf_device *gf_dev = dev_get_drvdata(dev);

	pr_info("Entry\n");
	gw9558_enable_debug_timer(gf_dev);
	return 0;
}

static const struct dev_pm_ops gw9558_pm_ops = {
	.suspend = gw9558_pm_suspend,
	.resume = gw9558_pm_resume
};

#ifndef ENABLE_SENSORS_FPRINT_SECURE
static struct spi_driver gw9558_spi_driver = {
#else
static struct platform_driver gw9558_spi_driver = {
#endif
	.driver = {
		.name = GF_DEV_NAME,
/*#ifndef ENABLE_SENSORS_FPRINT_SECURE
		.bus = &spi_bus_type,
#endif*/
		.owner = THIS_MODULE,
		.pm = &gw9558_pm_ops,
		.of_match_table = gw9558_of_match,
	},
	.probe = gw9558_probe,
	.remove = gw9558_remove,
};

static int __init gw9558_init(void)
{
	int retval = 0;

	pr_info("Entry\n");
#ifndef ENABLE_SENSORS_FPRINT_SECURE
	retval = spi_register_driver(&gw9558_spi_driver);
#else
	retval = platform_driver_register(&gw9558_spi_driver);
#endif
	if (retval < 0) {
		pr_err("Failed to register SPI driver.\n");
		return -EINVAL;
	}
	return retval;
}
module_init(gw9558_init);

static void __exit gw9558_exit(void)
{
	pr_info("Entry\n");
#ifndef ENABLE_SENSORS_FPRINT_SECURE
	spi_unregister_driver(&gw9558_spi_driver);
#else
	platform_driver_unregister(&gw9558_spi_driver);
#endif
}
module_exit(gw9558_exit);

MODULE_AUTHOR("fp.sec@samsung.com");
MODULE_DESCRIPTION("Samsung Electronics Inc. GW9558 driver");
MODULE_LICENSE("GPL");
