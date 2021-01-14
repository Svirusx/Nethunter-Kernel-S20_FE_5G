/*
 * LED driver for Samsung S2MPB02
 *
 * Copyright (C) 2014 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This driver is based on leds-max77804.c
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/mfd/samsung/s2mpb02.h>
#include <linux/mfd/samsung/s2mpb02-regulator.h>
#include <linux/leds-s2mpb02.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

extern struct class *camera_class; /*sys/class/camera*/

struct device *s2mpb02_led_dev;
struct s2mpb02_led_data **global_led_datas;
bool sysfs_flash_op;
bool flash_config_factory;

#ifdef S2MPB02_FLED_CHANNEL_1
#define S2MPB02_REG_FLED_CTRL S2MPB02_REG_FLED_CTRL1
#define S2MPB02_REG_FLED_CUR S2MPB02_REG_FLED_CUR1
#define S2MPB02_REG_FLED_TIME S2MPB02_REG_FLED_TIME1
#else
#define S2MPB02_REG_FLED_CTRL S2MPB02_REG_FLED_CTRL2
#define S2MPB02_REG_FLED_CUR S2MPB02_REG_FLED_CUR2
#define S2MPB02_REG_FLED_TIME S2MPB02_REG_FLED_TIME2
#endif

//#define DEBUG_READ_REGISTER //To dump register values with /sys/kernel/debug/s2mpb02-led-regs
//#define DEBUG_WRITE_REGISTER //To write register with sysfs

struct s2mpb02_led_data {
	struct led_classdev led;
	struct s2mpb02_dev *s2mpb02;
	struct s2mpb02_led *data;
	struct i2c_client *i2c;
	struct work_struct work;
	struct mutex lock;
	spinlock_t value_lock;
	int brightness;
	int test_brightness;
};

static u8 leds_mask[S2MPB02_LED_MAX] = {
	S2MPB02_FLASH_MASK,
	S2MPB02_TORCH_MASK,
};

static u8 leds_shift[S2MPB02_LED_MAX] = {
	4,
	0,
};

#if defined(DEBUG_READ_REGISTER)
#if 0
static void print_all_reg_value(struct i2c_client *client)
{
	int ret;
	u8 value;
	u8 i;

	for (i = 0; i <= S2MPB02_REG_LDO_DSCH3; i++) {
		ret = s2mpb02_read_reg(client, i, &value);
		if (unlikely(ret < 0))
			pr_err("[s2mpb02-LED][%s] read failed", __func__);
		pr_err("[s2mpb02-LED] register(%x) = %x\n", i, value);
		value = 0;
	}
}
#endif
static int s2mpb02_debugfs_show(struct seq_file *s, void *data)
{
	struct s2mpb02_dev *ldata = s->private;
	u8 reg;
	u8 reg_data;
	int ret;

	seq_printf(s, "s2mpb02 IF PMIC :\n");
	seq_printf(s, "=============\n");
	for (reg = 0; reg <= S2MPB02_REG_LDO_DSCH3; reg++) {
		ret = s2mpb02_read_reg(ldata->i2c, reg, &reg_data);
		if (unlikely(ret < 0)) {
			pr_err("[s2mpb02-LED][%s] read failed", __func__);
		}
		seq_printf(s, "0x%02x:\t0x%02x\n", reg, reg_data);
	}
	//print_all_reg_value(ldata->i2c);
	seq_printf(s, "\n");

	return 0;
}

static int s2mpb02_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, s2mpb02_debugfs_show, inode->i_private);
}

static const struct file_operations s2mpb02_debugfs_fops = {
	.open			= s2mpb02_debugfs_open,
	.read			= seq_read,
	.llseek 		= seq_lseek,
	.release		= single_release,
};
#endif

static int s2mpb02_set_bits(struct i2c_client *client, const u8 reg,
				 const u8 mask, const u8 inval)
{
	int ret;
	u8 value;

	ret = s2mpb02_read_reg(client, reg, &value);
	if (unlikely(ret < 0))
		return ret;

	value = (value & ~mask) | (inval & mask);

	ret = s2mpb02_write_reg(client, reg, value);

	return ret;
}

static int s2mpb02_led_get_en_value(struct s2mpb02_led_data *led_data, int on)
{
	if (on) {
		if (led_data->data->id == S2MPB02_FLASH_LED_1)
			return ((S2MPB02_FLED_ENABLE << S2MPB02_FLED_ENABLE_SHIFT) |
				(S2MPB02_FLED_FLASH_MODE << S2MPB02_FLED_MODE_SHIFT));
				/* Turn on FLASH by I2C */
		else
			return ((S2MPB02_FLED_ENABLE << S2MPB02_FLED_ENABLE_SHIFT) |
				(S2MPB02_FLED_TORCH_MODE << S2MPB02_FLED_MODE_SHIFT));
				/* Turn on TORCH by I2C */
	} else
		return (S2MPB02_FLED_DISABLE << S2MPB02_FLED_ENABLE_SHIFT);
				/* controlled by GPIO */
}

static void s2mpb02_led_set(struct led_classdev *led_cdev,
						enum led_brightness value)
{
	unsigned long flags;
	struct s2mpb02_led_data *led_data
		= container_of(led_cdev, struct s2mpb02_led_data, led);

	pr_debug("[LED] %s\n", __func__);

	spin_lock_irqsave(&led_data->value_lock, flags);
	led_data->test_brightness = min_t(int, (int)value, (int)led_cdev->max_brightness);
	spin_unlock_irqrestore(&led_data->value_lock, flags);

	schedule_work(&led_data->work);
}

static void led_set(struct s2mpb02_led_data *led_data, enum s2mpb02_led_turn_way turn_way)
{
	int ret;
	struct s2mpb02_led *data = led_data->data;
	int id = data->id;
	int value = 0, i = 0;

	if (turn_way == S2MPB02_LED_TURN_WAY_GPIO) {
		/* Turn way LED by GPIO */
		value = s2mpb02_led_get_en_value(led_data, 0);
		ret = s2mpb02_set_bits(led_data->i2c,
				S2MPB02_REG_FLED_CTRL1, S2MPB02_FLED_ENABLE_MODE_MASK, value);
		if (unlikely(ret))
			goto error_set_bits;

		if (led_data->test_brightness == LED_OFF) {
			/* set current */
			ret = s2mpb02_set_bits(led_data->i2c, S2MPB02_REG_FLED_CUR1,
					  leds_mask[id], data->brightness << leds_shift[id]);
			if (unlikely(ret))
				goto error_set_bits;

			for (i = 0; i < S2MPB02_LED_MAX; i++) {
				if (global_led_datas[i] && global_led_datas[i]->data) {
					gpio_request(global_led_datas[i]->data->gpio, NULL);
					gpio_direction_output(global_led_datas[i]->data->gpio, 0);
					gpio_free(global_led_datas[i]->data->gpio);
				}
			}
		} else {
			/* set current */
			ret = s2mpb02_set_bits(led_data->i2c, S2MPB02_REG_FLED_CUR1,
					  leds_mask[id], data->brightness << leds_shift[id]);
			if (unlikely(ret))
				goto error_set_bits;

			if (global_led_datas[id] && global_led_datas[id]->data) {
				gpio_request(global_led_datas[id]->data->gpio, NULL);
				gpio_direction_output(global_led_datas[id]->data->gpio, 1);
				gpio_free(global_led_datas[id]->data->gpio);
			}
		}
	} else {
		if (led_data->test_brightness == LED_OFF) {
			value = s2mpb02_led_get_en_value(led_data, 0);
			ret = s2mpb02_set_bits(led_data->i2c,
						S2MPB02_REG_FLED_CTRL1, S2MPB02_FLED_ENABLE_MODE_MASK, value);
			if (unlikely(ret))
				goto error_set_bits;

			/* set current */
			ret = s2mpb02_set_bits(led_data->i2c, S2MPB02_REG_FLED_CUR1,
						  leds_mask[id], data->brightness << leds_shift[id]);
			if (unlikely(ret))
				goto error_set_bits;
		} else {
			/* set current */
			ret = s2mpb02_set_bits(led_data->i2c, S2MPB02_REG_FLED_CUR1,
						  leds_mask[id], data->brightness << leds_shift[id]);
			if (unlikely(ret))
				goto error_set_bits;

			/* Turn on LED by I2C */
			value = s2mpb02_led_get_en_value(led_data, 1);
			ret = s2mpb02_set_bits(led_data->i2c,
						S2MPB02_REG_FLED_CTRL1, S2MPB02_FLED_ENABLE_MODE_MASK, value);
			if (unlikely(ret))
				goto error_set_bits;
		}
	}

	return;

error_set_bits:
	pr_err("%s: can't set led level %d\n", __func__, ret);

	return;
}

static void s2mpb02_led_work(struct work_struct *work)
{
	struct s2mpb02_led_data *led_data
		= container_of(work, struct s2mpb02_led_data, work);

	pr_debug("[LED] %s\n", __func__);

	mutex_lock(&led_data->lock);
	led_set(led_data, S2MPB02_LED_TURN_WAY_I2C);
	mutex_unlock(&led_data->lock);
}

static int s2mpb02_led_setup(struct s2mpb02_led_data *led_data)
{
	int ret = 0;
	struct s2mpb02_led *data = led_data->data;
	int id = data->id;
	int value;

	/* Disable Low Voltage operating mode control */
	ret |= s2mpb02_update_reg(led_data->i2c, S2MPB02_REG_FLED_CTRL1,
				S2MPB02_FLED_CTRL1_LV_DISABLE, S2MPB02_FLED_CTRL1_LV_EN_MASK);

	/* set operating minimum voltage */
	ret |= s2mpb02_update_reg(led_data->i2c, S2MPB02_REG_FLED_CTRL1,
				S2MPB02_LV_SEL_VOLT(3000), S2MPB02_LV_SEL_VOUT_MASK);

	/* set current & timeout */
	ret |= s2mpb02_update_reg(led_data->i2c, S2MPB02_REG_FLED_CUR1,
				data->brightness << leds_shift[id], leds_mask[id]);
	ret |= s2mpb02_update_reg(led_data->i2c, S2MPB02_REG_FLED_TIME1,
				data->timeout << leds_shift[id], leds_mask[id]);

	value = s2mpb02_led_get_en_value(led_data, 0);
	ret |= s2mpb02_update_reg(led_data->i2c,
				S2MPB02_REG_FLED_CTRL1, value, S2MPB02_FLED_ENABLE_MODE_MASK);

#if defined(CONFIG_SAMSUNG_SECURE_CAMERA)
	ret |= s2mpb02_ir_led_init();
#endif

	return ret;
}

void s2mpb02_led_get_status(struct led_classdev *led_cdev, bool status, bool onoff)
{
	int ret = 0;
	u8 value[6] = {0, };
	struct s2mpb02_led_data *led_data
		= container_of(led_cdev, struct s2mpb02_led_data, led);

	ret = s2mpb02_read_reg(led_data->i2c, 0x12, &value[0]); //Fled_ctrl1
	ret |= s2mpb02_read_reg(led_data->i2c, 0x13, &value[1]); //Fled_ctrl2
	ret |= s2mpb02_read_reg(led_data->i2c, 0x14, &value[2]); //Fled_cur1
	ret |= s2mpb02_read_reg(led_data->i2c, 0x15, &value[3]); //Fled_time1
	ret |= s2mpb02_read_reg(led_data->i2c, 0x16, &value[4]); //Fled_cur2
	ret |= s2mpb02_read_reg(led_data->i2c, 0x17, &value[5]); //Fled_time2
	if (unlikely(ret < 0)) {
		printk("%s : error to get dt node\n", __func__);
	}

	printk("%s[%d, %d] : Fled_ctrl1 = 0x%12x, Fled_ctrl2 = 0x%13x, Fled_cur1 = 0x%14x, "
		"Fled_time1 = 0x%15x, Fled_cur2 = 0x%16x, Fled_time2 = 0x%17x\n",
		__func__, status, onoff, value[0], value[1], value[2], value[3], value[4], value[5]);
}

int s2mpb02_led_en(int mode, int onoff, enum s2mpb02_led_turn_way turn_way)
{
	int ret = 0;
	int i = 0;

	if (sysfs_flash_op) {
		pr_warn("%s : The camera led control is not allowed"
			"because sysfs led control already used it\n", __FUNCTION__);
		return 0; //no error
	}

	if (global_led_datas == NULL) {
		pr_err("<%s> global_led_datas is NULL\n", __func__);
		return -1;
	}

	for (i = 0; i < S2MPB02_LED_MAX; i++) {
		if (global_led_datas[i] == NULL) {
			pr_err("<%s> global_led_datas[%d] is NULL\n", __func__, i);
			return -1;
		}
	}

	if (onoff > 0) {/* enable */
		pr_info("<%s> enable %d, %d\n", __func__, onoff, mode);
		if (mode == S2MPB02_TORCH_LED_1) {
			if (onoff >= S2MPB02_TORCH_OUT_I_MAX)
				onoff = S2MPB02_TORCH_OUT_I_MAX-1;
		} else if (mode == S2MPB02_FLASH_LED_1) {
			if (onoff >= S2MPB02_FLASH_OUT_I_MAX)
				onoff = S2MPB02_FLASH_OUT_I_MAX-1;
		} else {
			pr_err("<%s> mode %d is invalid\n", __func__, mode);
			return -1;
		}
		global_led_datas[mode]->data->brightness = onoff;
		global_led_datas[mode]->test_brightness = LED_FULL;
	} else {/* disable */
		pr_info("<%s> disable %d, %d\n", __func__, onoff, mode);
		global_led_datas[mode]->test_brightness = LED_OFF;
	}

	led_set(global_led_datas[mode], turn_way);

	return ret;
}
EXPORT_SYMBOL(s2mpb02_led_en);

#ifdef DEBUG_WRITE_REGISTER
ssize_t s2mpb02_write(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	unsigned int value	= 0;
	unsigned char reg = 0;
	unsigned char data = 0;

	if (buf == NULL || kstrtouint(buf, 16, &value)) {
		pr_err("[%s] error buf is NULL\n", __func__);
		return -1;
	}

	reg = value >> 8;
	data = value & 0xFF;
	pr_info("[%s] reg: %x, data: %x\n", __func__, reg, data);

	s2mpb02_write_reg(global_led_datas[0]->i2c, reg, data);

	return count;
}
static DEVICE_ATTR(s2mpb02_cam_reg, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
	NULL, s2mpb02_write);
#endif

#if defined(CONFIG_SAMSUNG_SECURE_CAMERA)
int s2mpb02_ir_led_init(void)
{
	int ret = 0;

	ret |= s2mpb02_write_reg(global_led_datas[0]->i2c, S2MPB02_REG_FLED_CTRL2, 0x38);
	ret |= s2mpb02_write_reg(global_led_datas[0]->i2c, S2MPB02_REG_FLED_CUR2, 0xAF);

	ret |= s2mpb02_write_reg(global_led_datas[0]->i2c, S2MPB02_REG_FLED_TIME2, 0x34);
	ret |= s2mpb02_write_reg(global_led_datas[0]->i2c, S2MPB02_REG_FLED_TIME2, 0x35);

	ret |= s2mpb02_write_reg(global_led_datas[0]->i2c, S2MPB02_REG_FLED_IRON1, 0x19);
	ret |= s2mpb02_write_reg(global_led_datas[0]->i2c, S2MPB02_REG_FLED_IRON2, 0x0B);
	ret |= s2mpb02_write_reg(global_led_datas[0]->i2c, S2MPB02_REG_FLED_IRD1, 0x00);
	ret |= s2mpb02_write_reg(global_led_datas[0]->i2c, S2MPB02_REG_FLED_IRD2, 0X2C);

#if 1 //TEMP_845
    s2mpb02_ir_led_current(5);
    s2mpb02_ir_led_pulse_width(240);
    s2mpb02_ir_led_pulse_delay(0);
    s2mpb02_ir_led_max_time(0);
#endif

	return ret;
}
EXPORT_SYMBOL(s2mpb02_ir_led_init);

int s2mpb02_ir_led_current(int32_t current_value)
{
	int ret = 0;
	unsigned int value = 0;
	unsigned char data = 0;

	if (current_value > 0)
		value  = current_value - 1;

	pr_info("[%s] led current value : %u [current_value::%d]\n", __func__, value, current_value);

	data = ((value & 0x0F) << 4) | 0x0F;

	ret = s2mpb02_write_reg(global_led_datas[0]->i2c, S2MPB02_REG_FLED_CUR2, data);
	if (ret < 0)
		pr_err("[%s] i2c write error", __func__);

	return ret;
}
EXPORT_SYMBOL(s2mpb02_ir_led_current);

int s2mpb02_ir_led_pulse_width(int32_t width)
{
	unsigned int value	= width;
	unsigned char iron1 = 0;
	unsigned char iron2 = 0;
	int ret = 0;

	pr_info("[%s] led pulse_width value : %u\n", __func__, value);

	iron1 = (value >> 2) & 0xFF;
	iron2 = ((value & 0x03) << 6) | 0x0B;

	pr_info("[%s] IRON1(0x%02x), IRON2(0x%02x)\n", __func__, iron1, iron2);

	/* set 0x18, 0x19 */
	ret |= s2mpb02_write_reg(global_led_datas[0]->i2c, S2MPB02_REG_FLED_IRON1, iron1);
	ret |= s2mpb02_write_reg(global_led_datas[0]->i2c, S2MPB02_REG_FLED_IRON2, iron2);
	if (ret < 0)
		pr_err("[%s] i2c write error", __func__);

	return ret;

}
EXPORT_SYMBOL(s2mpb02_ir_led_pulse_width);

int s2mpb02_ir_led_pulse_delay(int32_t delay)
{
	unsigned int value	= delay;
	unsigned char ird1 = 0;
	unsigned char ird2 = 0;
	int ret = 0;

	pr_info("[%s] led pulse_delay value : %u\n", __func__, value);

	ird1 = (value >> 2) & 0xFF;
	ird2 = ((value & 0x03) << 6) | 0x2C; /* value 0x2C means RSVD[5:0] Reserved */

	pr_info("[%s] IRD1(0x%02x), IRD2(0x%02x)\n", __func__, ird1, ird2);

	/* set 0x18, 0x19 */
	ret |= s2mpb02_write_reg(global_led_datas[0]->i2c, S2MPB02_REG_FLED_IRD1, ird1);
	ret |= s2mpb02_write_reg(global_led_datas[0]->i2c, S2MPB02_REG_FLED_IRD2, ird2);
	if (ret < 0)
		pr_err("[%s] i2c write error", __func__);

	return ret;
}
EXPORT_SYMBOL(s2mpb02_ir_led_pulse_delay);

int s2mpb02_ir_led_max_time(int32_t max_time)
{
	int ret = 0;

	pr_info("[%s] led max_time value : %u\n", __func__, max_time);

	ret |= s2mpb02_set_bits(global_led_datas[0]->i2c, S2MPB02_REG_FLED_CTRL2,
		S2MPB02_FLED2_MAX_TIME_CLEAR_MASK, 0x00);
	ret |= s2mpb02_set_bits(global_led_datas[0]->i2c, S2MPB02_REG_FLED_TIME2,
		S2MPB02_FLED2_MAX_TIME_EN_MASK, 0x00);
	if (max_time > 0) {
		ret |= s2mpb02_set_bits(global_led_datas[0]->i2c, S2MPB02_REG_FLED_TIME2,
			S2MPB02_FLED2_MAX_TIME_EN_MASK, 0x01);

		ret |= s2mpb02_set_bits(global_led_datas[0]->i2c, S2MPB02_REG_FLED_IRON2,
			S2MPB02_FLED2_MAX_TIME_MASK, (u8) max_time - 1);
	}

	return ret;
}
EXPORT_SYMBOL(s2mpb02_ir_led_max_time);
#endif

ssize_t s2mpb02_store(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	int i = 0, ret = 0;
	int onoff = -1;
	sysfs_flash_op = 0;

	if (buf == NULL || kstrtouint(buf, 10, &onoff))
		return -1;

	if (global_led_datas == NULL) {
		pr_err("<%s> global_led_datas is NULL\n", __func__);
		return -1;
	}

	for (i = 0; i < S2MPB02_LED_MAX; i++) {
		if (global_led_datas[i] == NULL) {
			pr_err("<%s> global_led_datas[%d] is NULL\n", __func__, i);
			return -1;
		}
	}

	pr_info("<%s> sysfs torch/flash value %d\n", __func__, onoff);
	if (onoff == 0) {
		// Torch OFF
		onoff = 0;
	} else if (onoff == 1) {
		// Torch ON
		onoff = S2MPB02_TORCH_OUT_I_60MA;
	} else if (onoff == 100) {
		// Factory Torch ON
		onoff = S2MPB02_TORCH_OUT_I_240MA;
	} else if (onoff == 200) {
		pr_info("<%s> sysfs flash value %d\n", __func__, onoff);

		/* Factory mode Turn on Flash */
		/* set reserved reg. 0x63 for continuous flash on */
		flash_config_factory = true;
		ret = s2mpb02_write_reg(global_led_datas[S2MPB02_FLASH_LED_1]->i2c, 0x63, 0x5F);
		if (ret < 0)
			pr_info("[LED]%s , failed set flash register setting\n", __func__);
		onoff = S2MPB02_FLASH_OUT_I_300MA;
	} else if (onoff == 1001) {
		// level 1 (Flashlight level 1)
		onoff = S2MPB02_TORCH_OUT_I_40MA;
#if defined(CONFIG_SEC_BEYONDXQ_PROJECT)
		onoff = S2MPB02_TORCH_OUT_I_60MA;
#endif
	} else if (onoff == 1002) {
		// level 2 (Flashlight level 2)
		onoff = S2MPB02_TORCH_OUT_I_60MA;
#if defined(CONFIG_SEC_BEYONDXQ_PROJECT)
		onoff = S2MPB02_TORCH_OUT_I_120MA;
#endif
	} else if (onoff == 1003) {
		// level 3
		onoff = S2MPB02_TORCH_OUT_I_80MA;
	} else if (onoff == 1004) {
		// level 4 (Flashlight level 3)
		onoff = S2MPB02_TORCH_OUT_I_100MA;
#if defined(CONFIG_SEC_BEYONDXQ_PROJECT)
		onoff = S2MPB02_TORCH_OUT_I_180MA;
#endif
	} else if (onoff == 1005) {
		// level 5
		onoff = S2MPB02_TORCH_OUT_I_120MA;
	} else if (onoff == 1006) {
		// level 6 (Flashlight level 4)
		onoff = S2MPB02_TORCH_OUT_I_160MA;
#if defined(CONFIG_SEC_BEYONDXQ_PROJECT)
		onoff = S2MPB02_TORCH_OUT_I_240MA;
#endif
	} else if (onoff == 1007) {
		// level 7
		onoff = S2MPB02_TORCH_OUT_I_180MA;
	} else if (onoff == 1008) {
		// level 8
		onoff = S2MPB02_TORCH_OUT_I_180MA;
	} else if (onoff == 1009) {
		// level 9 (Flashlight level 5)
		onoff = S2MPB02_TORCH_OUT_I_200MA;
#if defined(CONFIG_SEC_BEYONDXQ_PROJECT)
		onoff = S2MPB02_TORCH_OUT_I_300MA;
#endif
	} else if (onoff == 1010) {
		// level 10
		onoff = S2MPB02_TORCH_OUT_I_200MA;
	} else if ((2001 <= onoff) && (onoff <= 2015)) {
		// Torch ON for tunning : Step 20mA ~ 300mA
		onoff = onoff - 2000;
		pr_info("<%s> torch level %d\n", __func__, onoff);
	} else {
		pr_err("<%s> value %d is invalid\n", __func__, onoff);
		onoff = 0;
	}

	if (flash_config_factory) {
		if (onoff == 0) {
			s2mpb02_write_reg(global_led_datas[S2MPB02_FLASH_LED_1]->i2c, 0x63, 0x7F);
			flash_config_factory = false;
		}
		s2mpb02_led_en(S2MPB02_FLASH_LED_1, onoff, S2MPB02_LED_TURN_WAY_GPIO);
	} else
		s2mpb02_led_en(S2MPB02_TORCH_LED_1, onoff, S2MPB02_LED_TURN_WAY_GPIO);

	if (onoff)
		sysfs_flash_op = 1;

	return count;
}

ssize_t s2mpb02_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int i = 0;

	if (global_led_datas == NULL) {
		pr_err("<%s> global_led_datas is NULL\n", __func__);
		return sprintf(buf, "%d\n", -1);
	}

	for (i = 0; i < S2MPB02_LED_MAX; i++) {
		if (global_led_datas[i] == NULL) {
			pr_err("<%s> global_led_datas[%d] is NULL\n", __func__, i);
			return sprintf(buf, "%d\n", -1);
		}
	}

	if (global_led_datas[S2MPB02_TORCH_LED_1]->test_brightness == LED_OFF) {
		return sprintf(buf, "%d\n", 0);
	} else {
		return sprintf(buf, "%d\n", global_led_datas[S2MPB02_TORCH_LED_1]->data->brightness);
	}
}

static DEVICE_ATTR(rear_flash, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
	s2mpb02_show, s2mpb02_store);

#if defined(CONFIG_OF)
static int of_s2mpb02_led_dt(struct s2mpb02_dev *iodev,
					struct s2mpb02_led_platform_data *pdata)
{
	struct device_node *led_np, *np, *c_np;
	int ret;
	u32 temp;
	const char *temp_str;
	int index;

	led_np = iodev->dev->of_node;
	if (!led_np) {
		pr_err("<%s> could not find led sub-node\n", __func__);
		return -ENODEV;
	}

	np = of_find_node_by_name(led_np, "torch");
	if (!np) {
		pr_err("<%s> could not find led sub-node\n",
								__func__);
		return -EINVAL;
	}

	pdata->num_leds = of_get_child_count(np);

	for_each_child_of_node(np, c_np) {
		ret = of_property_read_u32(c_np, "id", &temp);
		if (ret < 0)
			goto dt_err;
		index = temp;
		pdata->leds[index].id = temp;

		ret = of_property_read_string(c_np, "ledname", &temp_str);
		if (ret < 0)
			goto dt_err;
		pdata->leds[index].name = temp_str;

		ret = of_property_read_u32(c_np, "brightness", &temp);
		if (ret < 0)
			goto dt_err;
		if (temp > S2MPB02_FLASH_TORCH_CURRENT_MAX)
			temp = S2MPB02_FLASH_TORCH_CURRENT_MAX;
		pdata->leds[index].brightness = temp;

		ret = of_property_read_u32(c_np, "timeout", &temp);
		if (ret < 0)
			goto dt_err;
		if (temp > S2MPB02_TIMEOUT_MAX)
			temp = S2MPB02_TIMEOUT_MAX;
		pdata->leds[index].timeout = temp;

		ret = of_property_read_string(c_np, "default-trigger", &temp_str);
		if (ret < 0)
			goto dt_err;
		pdata->leds[index].default_trigger = temp_str;

		temp = of_gpio_count(c_np);
		if (temp > 0) {
			pdata->leds[index].gpio = of_get_gpio(c_np, 0);
		}
	}
	of_node_put(led_np);

	return 0;
dt_err:
	pr_err("%s failed to get a dt info\n", __func__);
	return ret;
}
#endif /* CONFIG_OF */

static int s2mpb02_led_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i;
	struct s2mpb02_dev *s2mpb02 = dev_get_drvdata(pdev->dev.parent);
#ifndef CONFIG_OF
	struct s2mpb02_platform_data *s2mpb02_pdata
		= dev_get_platdata(s2mpb02->dev);
#endif
	struct s2mpb02_led_platform_data *pdata;
	struct s2mpb02_led_data *led_data;
	struct s2mpb02_led *data;
	struct s2mpb02_led_data **led_datas;

#ifdef CONFIG_OF
	pdata = kzalloc(sizeof(struct s2mpb02_led_platform_data), GFP_KERNEL);
	if (!pdata) {
		pr_err("%s: failed to allocate driver data\n", __func__);
		return -ENOMEM;
	}
	ret = of_s2mpb02_led_dt(s2mpb02, pdata);
	if (ret < 0) {
		pr_err("s2mpb02-torch : %s not found torch dt! ret[%d]\n",
				 __func__, ret);
		kfree(pdata);
		return ret;
	}
#else
	pdata = s2mpb02_pdata->led_data;
	if (pdata == NULL) {
		pr_err("[LED] no platform data for this led is found\n");
		return -EFAULT;
	}
#endif

	sysfs_flash_op = 0; //default off
	global_led_datas = kzalloc(sizeof(struct s2mpb02_led_data *)*S2MPB02_LED_MAX, GFP_KERNEL);

	led_datas = kzalloc(sizeof(struct s2mpb02_led_data *)
				* S2MPB02_LED_MAX, GFP_KERNEL);
	if (unlikely(!led_datas)) {
		pr_err("[LED] memory allocation error %s", __func__);
		kfree(pdata);
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, led_datas);

	pr_info("[LED] %s %d leds\n", __func__, pdata->num_leds);

	for (i = 0; i != pdata->num_leds; ++i) {
		pr_info("%s led%d setup ...\n", __func__, i);

		data = kzalloc(sizeof(struct s2mpb02_led), GFP_KERNEL);
		if (unlikely(!data)) {
			pr_err("[LED] memory allocation error %s\n", __func__);
			ret = -ENOMEM;
			continue;
		}

		memcpy(data, &(pdata->leds[i]), sizeof(struct s2mpb02_led));

		led_data = kzalloc(sizeof(struct s2mpb02_led_data),
				   GFP_KERNEL);

		global_led_datas[i] = led_data;
		led_datas[i] = led_data;
		if (unlikely(!led_data)) {
			pr_err("[LED] memory allocation error %s\n", __func__);
			ret = -ENOMEM;
			kfree(data);
			continue;
		}

		led_data->s2mpb02 = s2mpb02;
		led_data->i2c = s2mpb02->i2c;
		led_data->data = data;
		led_data->led.name = data->name;
		led_data->led.brightness_set = s2mpb02_led_set;
		led_data->led.brightness = LED_OFF;
		led_data->brightness = data->brightness;
		led_data->led.flags = 0;
		led_data->led.max_brightness = LED_FULL;
		led_data->led.default_trigger = data->default_trigger;

		mutex_init(&led_data->lock);
		spin_lock_init(&led_data->value_lock);
		INIT_WORK(&led_data->work, s2mpb02_led_work);

		ret = led_classdev_register(&pdev->dev, &led_data->led);
		if (unlikely(ret)) {
			pr_err("unable to register LED\n");
			cancel_work_sync(&led_data->work);
			mutex_destroy(&led_data->lock);
			kfree(data);
			kfree(led_data);
			led_datas[i] = NULL;
			ret = -EFAULT;
			continue;
		}

		ret = s2mpb02_led_setup(led_data);
		if (unlikely(ret)) {
			pr_err("unable to register LED\n");
			cancel_work_sync(&led_data->work);
			mutex_destroy(&led_data->lock);
			led_classdev_unregister(&led_data->led);
			kfree(data);
			kfree(led_data);
			led_datas[i] = NULL;
			ret = -EFAULT;
		}
	}

#if defined(DEBUG_READ_REGISTER)
	(void)debugfs_create_file("s2mpb02-led-regs", S_IRUGO, NULL,
					(void *)s2mpb02, &s2mpb02_debugfs_fops);
#endif

#ifdef CONFIG_OF
	kfree(pdata);
#endif

	if (camera_class == NULL) {
		camera_class = class_create(THIS_MODULE, "camera");
		if (IS_ERR(camera_class))
			pr_err("failed to create device cam_dev_rear!\n");
	}

	if (camera_class != NULL) {
		s2mpb02_led_dev = device_create(camera_class, NULL, 0, NULL, "flash");
		if (IS_ERR(s2mpb02_led_dev)) {
			pr_err("<%s> Failed to create device(flash)!\n", __func__);
			return -ENODEV;
		}

		if (device_create_file(s2mpb02_led_dev, &dev_attr_rear_flash) < 0) {
			pr_err("<%s> failed to create device file, %s\n",
					__func__, dev_attr_rear_flash.attr.name);
			return -ENOENT;
		}

#ifdef DEBUG_WRITE_REGISTER
		if (device_create_file(s2mpb02_led_dev, &dev_attr_s2mpb02_cam_reg) < 0) {
			pr_err("<%s> Failed to create device file!(%s)!\n", __func__,
				dev_attr_s2mpb02_cam_reg.attr.name);
			return -ENOENT;
		}
#endif
	}
	pr_err("<%s> end\n", __func__);
	return ret;
}

static int s2mpb02_led_remove(struct platform_device *pdev)
{
	struct s2mpb02_led_data **led_datas = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i != S2MPB02_LED_MAX; ++i) {
		if (led_datas[i] == NULL)
			continue;

		cancel_work_sync(&led_datas[i]->work);
		mutex_destroy(&led_datas[i]->lock);
		led_classdev_unregister(&led_datas[i]->led);
		kfree(led_datas[i]->data);
		kfree(led_datas[i]);
	}
	kfree(led_datas);

	if (global_led_datas != NULL)
		kfree(global_led_datas);

	device_remove_file(s2mpb02_led_dev, &dev_attr_rear_flash);


#ifdef DEBUG_WRITE_REGISTER
	device_remove_file(s2mpb02_led_dev, &dev_attr_s2mpb02_cam_reg);
#endif

	return 0;
}

static struct platform_driver s2mpb02_led_driver = {
	.probe		= s2mpb02_led_probe,
	.remove		= s2mpb02_led_remove,
	.driver		= {
		.name	= "s2mpb02-led",
		.owner	= THIS_MODULE,
		.suppress_bind_attrs = true,
	},
};

static int __init s2mpb02_led_init(void)
{
	return platform_driver_register(&s2mpb02_led_driver);
}
module_init(s2mpb02_led_init);

static void __exit s2mpb02_led_exit(void)
{
	platform_driver_unregister(&s2mpb02_led_driver);
}
module_exit(s2mpb02_led_exit);

MODULE_DESCRIPTION("S2MPB02 LED driver");
MODULE_LICENSE("GPL");
