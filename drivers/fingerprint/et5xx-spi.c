/*
 * Copyright (C) 2016 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */
#include "fingerprint.h"
#include "et5xx.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
static DECLARE_BITMAP(minors, N_SPI_MINORS);
static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);
static int gpio_irq;
static struct etspi_data *g_data;
static DECLARE_WAIT_QUEUE_HEAD(interrupt_waitq);
static unsigned int bufsiz = 1024;
module_param(bufsiz, uint, 0444);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");
#ifdef FP_DDR_FREQ_CONTROL
static void fill_bus_vector(void)
{
	int i = 0;
	for (i = 0; i < NUM_BUS_TABLE; i++) {
		fpsensor_reg_bus_vectors[i].src = MSM_BUS_MASTER_AMPSS_M0;
		fpsensor_reg_bus_vectors[i].dst = MSM_BUS_SLAVE_EBI_CH0;
		fpsensor_reg_bus_vectors[i].ab = ab_ib_bus_vectors[i][0];
		fpsensor_reg_bus_vectors[i].ib = MHZ_TO_BPS(ab_ib_bus_vectors[i][1], BUS_W);
	}
}
#endif

void etspi_pin_control(struct etspi_data *etsspi, bool pin_set)
{
	int status = 0;
	etsspi->p->state = NULL;
	if (pin_set) {
		if (!IS_ERR(etsspi->pins_idle)) {
			status = pinctrl_select_state(etsspi->p,
				etsspi->pins_idle);
			if (status)
				pr_err("%s: can't set pin default state\n",
					__func__);
			pr_debug("%s idle\n", __func__);
		}
	} else {
		if (!IS_ERR(etsspi->pins_sleep)) {
			status = pinctrl_select_state(etsspi->p,
				etsspi->pins_sleep);
			if (status)
				pr_err("%s: can't set pin sleep state\n",
					__func__);
			pr_debug("%s sleep\n", __func__);
		}
	}
}
static irqreturn_t etspi_fingerprint_interrupt(int irq, void *dev_id)
{
	struct etspi_data *etspi = (struct etspi_data *)dev_id;
	etspi->int_count++;
	etspi->finger_on = 1;
	disable_irq_nosync(gpio_irq);
	wake_up_interruptible(&interrupt_waitq);
	wake_lock_timeout(&etspi->fp_signal_lock, 1 * HZ);
	pr_info("%s FPS triggered.int_count(%d) On(%d)\n", __func__,
		etspi->int_count, etspi->finger_on);
	etspi->interrupt_count++;
	return IRQ_HANDLED;
}
int etspi_Interrupt_Init(
		struct etspi_data *etspi,
		int int_ctrl,
		int detect_period,
		int detect_threshold)
{
	int status = 0;
	etspi->finger_on = 0;
	etspi->int_count = 0;
	pr_info("%s int_ctrl = %d detect_period = %d detect_threshold = %d\n",
				__func__,
				int_ctrl,
				detect_period,
				detect_threshold);
	etspi->detect_period = detect_period;
	etspi->detect_threshold = detect_threshold;
	gpio_irq = gpio_to_irq(etspi->drdyPin);
	if (gpio_irq < 0) {
		pr_err("%s gpio_to_irq failed\n", __func__);
		status = gpio_irq;
		goto done;
	}
	if (etspi->drdy_irq_flag == DRDY_IRQ_DISABLE) {
		if (request_irq
			(gpio_irq, etspi_fingerprint_interrupt
			, int_ctrl, "etspi_irq", etspi) < 0) {
			pr_err("%s drdy request_irq failed\n", __func__);
			status = -EBUSY;
			goto done;
		} else {
			enable_irq_wake(gpio_irq);
			etspi->drdy_irq_flag = DRDY_IRQ_ENABLE;
		}
	}
done:
	return status;
}
int etspi_Interrupt_Free(struct etspi_data *etspi)
{
	pr_info("%s\n", __func__);
	if (etspi != NULL) {
		if (etspi->drdy_irq_flag == DRDY_IRQ_ENABLE) {
			if (!etspi->int_count)
				disable_irq_nosync(gpio_irq);
			disable_irq_wake(gpio_irq);
			free_irq(gpio_irq, etspi);
			etspi->drdy_irq_flag = DRDY_IRQ_DISABLE;
		}
		etspi->finger_on = 0;
		etspi->int_count = 0;
	}
	return 0;
}
void etspi_Interrupt_Abort(struct etspi_data *etspi)
{
	wake_up_interruptible(&interrupt_waitq);
}
unsigned int etspi_fps_interrupt_poll(
		struct file *file,
		struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	struct etspi_data *etspi = file->private_data;
	pr_debug("%s FPS fps_interrupt_poll, finger_on(%d), int_count(%d)\n",
		__func__, etspi->finger_on, etspi->int_count);
	if (!etspi->finger_on)
		poll_wait(file, &interrupt_waitq, wait);
	if (etspi->finger_on) {
		mask |= POLLIN | POLLRDNORM;
		etspi->finger_on = 0;
	}
	return mask;
}
/*-------------------------------------------------------------------------*/
static void etspi_reset(struct etspi_data *etspi)
{
	pr_info("%s\n", __func__);
	gpio_set_value(etspi->sleepPin, 0);
	usleep_range(1050, 1100);
	gpio_set_value(etspi->sleepPin, 1);
	etspi->reset_count++;
}

static void etspi_power_set(struct etspi_data *etspi, int status)
{
	int rc = 0;
	if (etspi->ldo_pin) {
		pr_info("%s, ldo\n", __func__);
		if (status == 1) {
			if (etspi->ldo_pin) {
				gpio_set_value(etspi->ldo_pin, 1);
				etspi->ldo_enabled = 1;
			}
		} else if (status == 0) {
			if (etspi->ldo_pin) {
				gpio_set_value(etspi->ldo_pin, 0);
				etspi->ldo_enabled = 0;
			}
		} else {
			pr_err("%s can't support this value. %d\n",
				__func__, status);
		}
	} else if ((etspi->regulator_3p3 != NULL) && (etspi->regulator_1p8 != NULL)) {
		pr_info("%s, regulator 3.3V & 1.8V, status %d\n"
			, __func__, status);
		if (status == 1) {
			if (regulator_is_enabled(etspi->regulator_3p3) == 0) {
				rc = regulator_enable(etspi->regulator_3p3);
				if (rc)
					pr_err("%s regulator_3p3 enable failed, rc=%d\n",
						__func__, rc);
				else
					etspi->ldo_enabled = 1;
			} else
				pr_info("%s, regulator_3p3 is already enabled\n"
					, __func__);

			if (regulator_is_enabled(etspi->regulator_1p8) == 0) {
				rc = regulator_enable(etspi->regulator_1p8);
				if (rc)
					pr_err("%s regulator_1p8 enable failed, rc=%d\n",
						__func__, rc);
			} else
				pr_info("%s, regulator_1p8 is already enabled\n"
					, __func__);
		} else if (status == 0) {
			if (regulator_is_enabled(etspi->regulator_1p8)) {
				rc = regulator_disable(etspi->regulator_1p8);
				if (rc)
					pr_err("%s regulator_1p8 disable failed, rc=%d\n",
						__func__, rc);
			} else
				pr_info("%s, regulator_1p8 is already disabled\n"
					, __func__);

			if (regulator_is_enabled(etspi->regulator_3p3)) {
				rc = regulator_disable(etspi->regulator_3p3);
				if (rc)
					pr_err("%s regulator_3p3 disable failed, rc=%d\n",
						__func__, rc);
				else
					etspi->ldo_enabled = 0;
			} else
				pr_info("%s, regulator_3p3 is already disabled\n"
					, __func__);
		} else {
			pr_err("%s can't support this value. %d\n",
				__func__, status);
		}
	} else if ((etspi->regulator_3p3 != NULL) && (etspi->regulator_1p8 != NULL)) {
		pr_info("%s, regulator 3.3V & 1.8V, status %d\n"
			, __func__, status);
		if (status == 1) {
			if (regulator_is_enabled(etspi->regulator_3p3) == 0) {
				rc = regulator_enable(etspi->regulator_3p3);
				if (rc)
					pr_err("%s regulator_3p3 enable failed, rc=%d\n",
						__func__, rc);
				else
					etspi->ldo_enabled = 1;
			} else
				pr_info("%s, regulator_3p3 is already enabled\n"
					, __func__);

			if (regulator_is_enabled(etspi->regulator_1p8) == 0) {
				rc = regulator_enable(etspi->regulator_1p8);
				if (rc)
					pr_err("%s regulator_1p8 enable failed, rc=%d\n",
						__func__, rc);
			} else
				pr_info("%s, regulator_1p8 is already enabled\n"
					, __func__);
		} else if (status == 0) {
			if (regulator_is_enabled(etspi->regulator_1p8)) {
				rc = regulator_disable(etspi->regulator_1p8);
				if (rc)
					pr_err("%s regulator_1p8 disable failed, rc=%d\n",
						__func__, rc);
			} else
				pr_info("%s, regulator_1p8 is already disabled\n"
					, __func__);

			if (regulator_is_enabled(etspi->regulator_3p3)) {
				rc = regulator_disable(etspi->regulator_3p3);
				if (rc)
					pr_err("%s regulator_3p3 disable failed, rc=%d\n",
						__func__, rc);
				else
					etspi->ldo_enabled = 0;
			} else
				pr_info("%s, regulator_3p3 is already disabled\n"
					, __func__);
		} else {
			pr_err("%s can't support this value. %d\n",
				__func__, status);
		}
	} else if (etspi->regulator_3p3 != NULL) {
		pr_info("%s, regulator, status %d\n"

			, __func__, status);
		if (status == 1) {
			if (regulator_is_enabled(etspi->regulator_3p3) == 0) {
				rc = regulator_enable(etspi->regulator_3p3);
				if (rc)
					pr_err("%s regulator enable failed, rc=%d\n",
						__func__, rc);
				else
					etspi->ldo_enabled = 1;
			} else
				pr_info("%s, regulator is already enabled\n"

					, __func__);
		} else if (status == 0) {
			if (regulator_is_enabled(etspi->regulator_3p3)) {
				rc = regulator_disable(etspi->regulator_3p3);
				if (rc)
					pr_err("%s regulator disable failed, rc=%d\n",
						__func__, rc);
				else
					etspi->ldo_enabled = 0;
			} else
				pr_info("%s, regulator is already disabled\n"

					, __func__);
		} else {
			pr_err("%s can't support this value. %d\n",
				__func__, status);
		}
	} else {
		pr_info("%s This HW revision does not support a power control\n",
			__func__);
	}
}
static void etspi_power_control(struct etspi_data *etspi, int status)
{
	pr_info("%s status = %d\n", __func__, status);
	if (status == 1) {
		etspi_power_set(etspi, 1);
//		etspi_pin_control(etspi, 1);
		usleep_range(1600, 1650);
		if (etspi->sleepPin)
			gpio_set_value(etspi->sleepPin, 1);
		usleep_range(12000, 12050);
	} else if (status == 0) {
		if (etspi->sleepPin)
			gpio_set_value(etspi->sleepPin, 0);
		etspi_power_set(etspi, 0);
//		etspi_pin_control(etspi, 0);
	} else {
		pr_err("%s can't support this value. %d\n", __func__, status);
	}
}
static ssize_t etspi_read(struct file *filp,
						char __user *buf,
						size_t count,
						loff_t *f_pos)
{
	/*Implement by vendor if needed*/
	return 0;
}
static ssize_t etspi_write(struct file *filp,
						const char __user *buf,
						size_t count,
						loff_t *f_pos)
{
/*Implement by vendor if needed*/
	return 0;
}

static long etspi_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0, retval = 0;
	struct etspi_data *etspi;
	struct spi_device *spi;
	u32 tmp;
	struct egis_ioc_transfer *ioc = NULL;
	u8 *buf, *address, *result, *fr;
	/* Check type and command number */
	if (_IOC_TYPE(cmd) != EGIS_IOC_MAGIC) {
		pr_err("%s _IOC_TYPE(cmd) != EGIS_IOC_MAGIC", __func__);
		return -ENOTTY;
	}
	/* Check access direction once here; don't repeat below.
	 * IOC_DIR is from the user perspective, while access_ok is
	 * from the kernel perspective; so they look reversed.
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,
						(void __user *)arg,
						_IOC_SIZE(cmd));
	if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,
						(void __user *)arg,
						_IOC_SIZE(cmd));
	if (err) {
		pr_err("%s err", __func__);
		return -EFAULT;
	}
	/* guard against device removal before, or while,
	 * we issue this ioctl.
	 */
	etspi = filp->private_data;
	spin_lock_irq(&etspi->spi_lock);
	spi = spi_dev_get(etspi->spi);
	spin_unlock_irq(&etspi->spi_lock);
	if (spi == NULL) {
		pr_err("%s spi == NULL", __func__);
		return -ESHUTDOWN;
	}
	mutex_lock(&etspi->buf_lock);
	/* segmented and/or full-duplex I/O request */
	if (_IOC_NR(cmd) != _IOC_NR(EGIS_IOC_MESSAGE(0))
					|| _IOC_DIR(cmd) != _IOC_WRITE) {
		retval = -ENOTTY;
		goto out;
	}
	tmp = _IOC_SIZE(cmd);
	if ((tmp == 0) || (tmp % sizeof(struct egis_ioc_transfer)) != 0) {
		pr_err("%s ioc size error\n", __func__);
		retval = -EINVAL;
		goto out;
	}
	/* copy into scratch area */
	ioc = kmalloc(tmp, GFP_KERNEL);
	if (!ioc) {
		retval = -ENOMEM;
		goto out;
	}
	if (__copy_from_user(ioc, (void __user *)arg, tmp)) {
		pr_err("%s __copy_from_user error\n", __func__);
		retval = -EFAULT;
		goto out;
	}
	switch (ioc->opcode) {
	/*
	 * Read register
	 * tx_buf include register address will be read
	 */
	case FP_REGISTER_READ:
		address = ioc->tx_buf;
		result = ioc->rx_buf;
		pr_debug("etspi FP_REGISTER_READ\n");
		retval = etspi_io_read_register(etspi, address, result);
		if (retval < 0)	{
			pr_err("%s FP_REGISTER_READ error retval = %d\n"
			, __func__, retval);
		}
		break;
	/*
	 * Write data to register
	 * tx_buf includes address and value will be wrote
	 */
	case FP_REGISTER_WRITE:
		buf = ioc->tx_buf;
		pr_debug("%s FP_REGISTER_WRITE\n", __func__);
		retval = etspi_io_write_register(etspi, buf);
		if (retval < 0) {
			pr_err("%s FP_REGISTER_WRITE error retval = %d\n"
			, __func__, retval);
		}
		break;
	case FP_REGISTER_MREAD:
		address = ioc->tx_buf;
		result = ioc->rx_buf;
		pr_debug("%s FP_REGISTER_MREAD\n", __func__);
		retval = etspi_io_read_registerex(etspi, address, result,
				ioc->len);
		if (retval < 0) {
			pr_err("%s FP_REGISTER_MREAD error retval = %d\n"
			, __func__, retval);
		}
		break;
	case FP_REGISTER_BREAD:
		pr_debug("%s FP_REGISTER_BREAD\n", __func__);
		retval = etspi_io_burst_read_register(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_REGISTER_BREAD error retval = %d\n"
			, __func__, retval);
		}
		break;
	case FP_REGISTER_BWRITE:
		pr_debug("%s FP_REGISTER_BWRITE\n", __func__);
		retval = etspi_io_burst_write_register(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_REGISTER_BWRITE error retval = %d\n"
			, __func__, retval);
		}
		break;
	case FP_REGISTER_BREAD_BACKWARD:
		pr_debug("%s FP_REGISTER_BREAD_BACKWARD\n", __func__);
		retval = etspi_io_burst_read_register_backward(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_REGISTER_BREAD_BACKWARD error retval = %d\n"
				, __func__, retval);
		}
		break;
	case FP_REGISTER_BWRITE_BACKWARD:
		pr_debug("%s FP_REGISTER_BWRITE_BACKWARD\n", __func__);
		retval = etspi_io_burst_write_register_backward(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_REGISTER_BWRITE_BACKWARD error retval = %d\n"
				, __func__, retval);
		}
		break;
	case FP_NVM_READ:
		pr_debug("%s FP_NVM_READ, (%d)\n", __func__, spi->max_speed_hz);
		retval = etspi_io_nvm_read(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_NVM_READ error retval = %d\n"
			, __func__, retval);
		}
		retval = etspi_io_nvm_off(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_NVM_OFF error retval = %d\n"
			, __func__, retval);
		} else {
			pr_debug("%s FP_NVM_OFF\n", __func__);
		}
		break;
	case FP_NVM_WRITE:
		pr_debug("%s FP_NVM_WRITE, (%d)\n", __func__,
				spi->max_speed_hz);
		retval = etspi_io_nvm_write(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_NVM_WRITE error retval = %d\n"
			, __func__, retval);
		}
		retval = etspi_io_nvm_off(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_NVM_OFF error retval = %d\n"
			, __func__, retval);
		} else {
			pr_debug("%s FP_NVM_OFF\n", __func__);
		}
		break;
	case FP_NVM_WRITEEX:
		pr_debug("%s FP_NVM_WRITEEX, (%d)\n", __func__,
				spi->max_speed_hz);
		retval = etspi_io_nvm_writeex(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_NVM_WRITEEX error retval = %d\n"
			, __func__, retval);
		}
		retval = etspi_io_nvm_off(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_NVM_OFF error retval = %d\n"
			, __func__, retval);
		} else {
			pr_debug("%s FP_NVM_OFF\n", __func__);
		}
		break;
	case FP_NVM_OFF:
		pr_debug("%s FP_NVM_OFF\n", __func__);
		retval = etspi_io_nvm_off(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_NVM_OFF error retval = %d\n"
			, __func__, retval);
		}
		break;
	case FP_VDM_READ:
		pr_debug("%s FP_VDM_READ\n", __func__);
		retval = etspi_io_vdm_read(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_VDM_READ error retval = %d\n"
			, __func__, retval);
		} else {
			pr_debug("%s FP_VDM_READ finished.\n", __func__);
		}
		break;
	case FP_VDM_WRITE:
		pr_debug("%s FP_VDM_WRITE\n", __func__);
		retval = etspi_io_vdm_write(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_VDM_WRITE error retval = %d\n"
			, __func__, retval);
		} else {
			pr_debug("%s FP_VDM_WRTIE finished.\n", __func__);
		}
		break;
	/*
	 * Get one frame data from sensor
	 */
	case FP_GET_ONE_IMG:
		fr = ioc->rx_buf;
		pr_debug("%s FP_GET_ONE_IMG\n", __func__);
		retval = etspi_io_get_frame(etspi, fr, ioc->len);
		if (retval < 0) {
			pr_err("%s FP_GET_ONE_IMG error retval = %d\n"
			, __func__, retval);
		}
		break;
	case FP_NBM_READ:
		pr_info("%s FP_NBM_READ\n", __func__);
		retval = etspi_io_nbm_read(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_NBM_READ error retval = %d\n"
			, __func__, retval);
		} else {
			pr_debug("%s FP_NBM_READ finished.\n", __func__);
		}
		break;
	case FP_CLB_READ:
		pr_info("%s FP_CLB_READ\n", __func__);
		retval = etspi_io_clb_read(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_CLB_READ error retval = %d\n"
			, __func__, retval);
		} else {
			pr_debug("%s FP_CLB_READ finished.\n", __func__);
		}
		break;
	case FP_CLB_WRITE:
		pr_info("%s FP_CLB_WRITE\n", __func__);
		retval = etspi_io_clb_write(etspi, ioc);
		if (retval < 0) {
			pr_err("%s FP_CLB_WRITE error retval = %d\n"
			, __func__, retval);
		} else {
			pr_debug("%s FP_CLB_WRITE finished.\n", __func__);
		}
		break;
	case FP_SENSOR_RESET:
		pr_info("%s FP_SENSOR_RESET\n", __func__);
		etspi_reset(etspi);
		break;
	case FP_RESET_SET:
		break;

	case FP_POWER_CONTROL:
	case FP_POWER_CONTROL_ET5XX:
		pr_info("%s FP_POWER_CONTROL, status = %d\n", __func__,
				ioc->len);
		etspi_power_control(etspi, ioc->len);
		break;
	case FP_SET_SPI_CLOCK:
		pr_info("%s FP_SET_SPI_CLOCK, clock = %d\n", __func__,
				ioc->speed_hz);
#ifdef ENABLE_SENSORS_FPRINT_SECURE
		if (etspi->enabled_clk) {
			if (spi->max_speed_hz != ioc->speed_hz) {
				pr_info("%s already enabled. DISABLE_SPI_CLOCK\n",
					__func__);
				wake_unlock(&etspi->fp_spi_lock);
				etspi->enabled_clk = false;
			} else {
				pr_info("%s already enabled same clock.\n",
					__func__);
				break;
			}
		}
		spi->max_speed_hz = ioc->speed_hz;
		wake_lock(&etspi->fp_spi_lock);
		etspi->enabled_clk = true;
#else
		spi->max_speed_hz = ioc->speed_hz;
#endif
		break;
	/*
	 * Trigger initial routine
	 */
	case INT_TRIGGER_INIT:
		pr_debug("%s Trigger function init\n", __func__);
		retval = etspi_Interrupt_Init(
				etspi,
				(int)ioc->pad[0],
				(int)ioc->pad[1],
				(int)ioc->pad[2]);
		break;
	/* trigger */
	case INT_TRIGGER_CLOSE:
		pr_debug("%s Trigger function close\n", __func__);
		retval = etspi_Interrupt_Free(etspi);
		break;
	/* Poll Abort */
	case INT_TRIGGER_ABORT:
		pr_debug("%s Trigger function abort\n", __func__);
		etspi_Interrupt_Abort(etspi);
		break;
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	case FP_DISABLE_SPI_CLOCK:
		pr_info("%s FP_DISABLE_SPI_CLOCK\n", __func__);
		if (etspi->enabled_clk) {
			pr_info("%s DISABLE_SPI_CLOCK\n", __func__);
			wake_unlock(&etspi->fp_spi_lock);
			etspi->enabled_clk = false;
		}
		break;
	case FP_CPU_SPEEDUP:
		pr_info("%s FP_CPU_SPEEDUP\n", __func__);
		if (ioc->len) {
			u8 retry_cnt = 0;
			pr_info("%s FP_CPU_SPEEDUP ON:%d, retry: %d\n",
					__func__, ioc->len, retry_cnt);
#ifdef FP_DDR_FREQ_CONTROL
			if (bus_hdl) {
				retval = msm_bus_scale_client_update_request(bus_hdl, MHZ_1555);
				if (retval)
					pr_info("%s Failed Bus clk up%d\n",
						__func__, retval);
			} else {
				pr_info("%s Failed Bus clk up not registered\n", __func__);
			}
#endif
			if (etspi->min_cpufreq_limit) {
				pm_qos_add_request(&etspi->pm_qos,
					PM_QOS_CPU_DMA_LATENCY, 0);
				do {
					retval = set_freq_limit(DVFS_FINGER_ID,
						etspi->min_cpufreq_limit);
					retry_cnt++;
					if (retval) {
						pr_err("%s: booster start failed. (%d) retry: %d\n"
							, __func__, retval,
							retry_cnt);
						usleep_range(500, 510);
					}
				} while (retval && retry_cnt < 7);
			}
		} else {
			pr_info("%s FP_CPU_SPEEDUP OFF\n", __func__);
#ifdef FP_DDR_FREQ_CONTROL
			if (bus_hdl) {
				retval = msm_bus_scale_client_update_request(bus_hdl, MHZ_NONE);
				if (retval)
					pr_info("%s Failed Bus clk none%d\n",
						__func__, retval);
			} else {
				pr_info("%s Failed Bus clk none not registered\n", __func__);
			}
#endif
			retval = set_freq_limit(DVFS_FINGER_ID, -1);
			if (retval)
				pr_err("%s: booster stop failed. (%d)\n"
					, __func__, retval);
			pm_qos_remove_request(&etspi->pm_qos);
		}
		break;
	case FP_SET_SENSOR_TYPE:
		if ((int)ioc->len >= SENSOR_OOO &&
				(int)ioc->len < SENSOR_MAXIMUM) {
			if ((int)ioc->len == SENSOR_OOO &&
					etspi->sensortype == SENSOR_FAILED) {
				pr_info("%s maintain type check from out of order :%s\n",
					__func__,
					sensor_status[g_data->sensortype + 2]);
			} else {
				etspi->sensortype = (int)ioc->len;
				pr_info("%s FP_SET_SENSOR_TYPE :%s\n",
					__func__,
					sensor_status[g_data->sensortype + 2]);
			}
		} else {
			pr_err("%s FP_SET_SENSOR_TYPE invalid value %d\n",
					__func__, (int)ioc->len);
			etspi->sensortype = SENSOR_UNKNOWN;
		}
		break;
	case FP_SET_LOCKSCREEN:
		pr_info("%s FP_SET_LOCKSCREEN\n", __func__);
		break;
	case FP_SET_WAKE_UP_SIGNAL:
		pr_info("%s FP_SET_WAKE_UP_SIGNAL\n", __func__);
		break;
#endif
	case FP_SENSOR_ORIENT:
		pr_info("%s: orient is %d\n", __func__, etspi->orient);
		retval = put_user(etspi->orient, (u8 __user *) (uintptr_t)ioc->rx_buf);
		if (retval != 0)
			pr_err("%s FP_SENSOR_ORIENT put_user fail: %d\n"

				, __func__, retval);
		break;
	case FP_SPI_VALUE:
		etspi->spi_value = ioc->len;
		pr_info("%s spi_value: 0x%x\n", __func__, etspi->spi_value);
			break;
	case FP_IOCTL_RESERVED_01:
	case FP_IOCTL_RESERVED_02:
			break;
	default:
		pr_info("%s undefined case\n", __func__);
		retval = -EFAULT;
		break;
	}
out:
	if (ioc != NULL)
		kfree(ioc);
	mutex_unlock(&etspi->buf_lock);
	spi_dev_put(spi);
	if (retval < 0)
		pr_err("%s retval = %d\n", __func__, retval);
	return retval;
}
#ifdef CONFIG_COMPAT
static long etspi_compat_ioctl(struct file *filp,
	unsigned int cmd,
	unsigned long arg)
{
	return etspi_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#else
#define etspi_compat_ioctl NULL
#endif
/* CONFIG_COMPAT */
static int etspi_open(struct inode *inode, struct file *filp)
{
	struct etspi_data *etspi;
	int	status = -ENXIO;
	pr_info("%s\n", __func__);
	mutex_lock(&device_list_lock);
	list_for_each_entry(etspi, &device_list, device_entry) {
		if (etspi->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}
	if (status == 0) {
		if (etspi->buf == NULL) {
			etspi->buf = kmalloc(bufsiz, GFP_KERNEL);
			if (etspi->buf == NULL) {
				dev_dbg(&etspi->spi->dev, "open/ENOMEM\n");
				status = -ENOMEM;
			}
		}
		if (status == 0) {
			etspi->users++;
			filp->private_data = etspi;
			nonseekable_open(inode, filp);
			etspi->bufsiz = bufsiz;
		}
	} else
		pr_debug("%s nothing for minor %d\n"
			, __func__, iminor(inode));
	mutex_unlock(&device_list_lock);
	return status;
}
static int etspi_release(struct inode *inode, struct file *filp)
{
	struct etspi_data *etspi;
	pr_info("%s\n", __func__);
	mutex_lock(&device_list_lock);
	etspi = filp->private_data;
	filp->private_data = NULL;
	/* last close? */
	etspi->users--;
	if (etspi->users == 0) {
		int	dofree;
		kfree(etspi->buf);
		etspi->buf = NULL;
		/* ... after we unbound from the underlying device? */
		spin_lock_irq(&etspi->spi_lock);
		dofree = (etspi->spi == NULL);
		spin_unlock_irq(&etspi->spi_lock);
		if (dofree)
			kfree(etspi);
	}
	mutex_unlock(&device_list_lock);
	return 0;
}
int etspi_platformInit(struct etspi_data *etspi)
{
	int status = 0;
	pr_info("%s\n", __func__);
	/* gpio setting for ldo, ldo2, sleep, drdy pin */
	if (etspi != NULL) {
		etspi->drdy_irq_flag = DRDY_IRQ_DISABLE;
		if (etspi->ldo_pin) {
			status = gpio_request(etspi->ldo_pin, "etspi_ldo_en");
			if (status < 0) {
				pr_err("%s gpio_request etspi_ldo_en failed\n",
					__func__);
				goto etspi_platformInit_ldo_failed;
			}
			gpio_direction_output(etspi->ldo_pin, 0);
			etspi->ldo_enabled = 0;
			pr_info("%s ldo en value =%d\n",
				__func__, gpio_get_value(etspi->ldo_pin));
		}
		status = gpio_request(etspi->sleepPin, "etspi_sleep");
		if (status < 0) {
			pr_err("%s gpio_requset etspi_sleep failed\n",
				__func__);
			goto etspi_platformInit_sleep_failed;
		}
		gpio_direction_output(etspi->sleepPin, 0);
		if (status < 0) {
			pr_err("%s gpio_direction_output SLEEP failed\n",
					__func__);
			status = -EBUSY;
			goto etspi_platformInit_sleep_failed;
		}
		pr_info("%s sleep value =%d\n",
				__func__, gpio_get_value(etspi->sleepPin));
		status = gpio_request(etspi->drdyPin, "etspi_drdy");
		if (status < 0) {
			pr_err("%s gpio_request etspi_drdy failed\n",
				__func__);
			goto etspi_platformInit_drdy_failed;
		}
		status = gpio_direction_input(etspi->drdyPin);
		if (status < 0) {
			pr_err("%s gpio_direction_input DRDY failed\n",
				__func__);
			goto etspi_platformInit_gpio_init_failed;
		}
	} else {
		status = -EFAULT;
	}
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	wake_lock_init(&etspi->fp_spi_lock,
		WAKE_LOCK_SUSPEND, "etspi_wake_lock");
#endif
	wake_lock_init(&etspi->fp_signal_lock,
				WAKE_LOCK_SUSPEND, "etspi_sigwake_lock");
	pr_info("%s successful status=%d\n", __func__, status);
	return status;
etspi_platformInit_gpio_init_failed:
	gpio_free(etspi->drdyPin);
etspi_platformInit_drdy_failed:
	gpio_free(etspi->sleepPin);
etspi_platformInit_sleep_failed:
	gpio_free(etspi->ldo_pin);
etspi_platformInit_ldo_failed:
	pr_err("%s is failed\n", __func__);
	return status;
}
void etspi_platformUninit(struct etspi_data *etspi)
{
	pr_info("%s\n", __func__);
	if (etspi != NULL) {
		disable_irq_wake(gpio_irq);
		disable_irq(gpio_irq);
//		etspi_pin_control(etspi, false);
		free_irq(gpio_irq, etspi);
		etspi->drdy_irq_flag = DRDY_IRQ_DISABLE;
		if (etspi->ldo_pin)
			gpio_free(etspi->ldo_pin);
		gpio_free(etspi->sleepPin);
		gpio_free(etspi->drdyPin);
#ifdef ENABLE_SENSORS_FPRINT_SECURE
		wake_lock_destroy(&etspi->fp_spi_lock);
#endif
		wake_lock_destroy(&etspi->fp_signal_lock);
	}
}
static int etspi_parse_dt(struct device *dev,
	struct etspi_data *data)
{
	struct device_node *np = dev->of_node;
	enum of_gpio_flags flags;
	int errorno = 0;
	int gpio;
	gpio = of_get_named_gpio_flags(np, "etspi-sleepPin",
		0, &flags);
	if (gpio < 0) {
		errorno = gpio;
		goto dt_exit;
	} else {
		data->sleepPin = gpio;
		pr_info("%s: sleepPin=%d\n",
			__func__, data->sleepPin);
	}
	gpio = of_get_named_gpio_flags(np, "etspi-drdyPin",
		0, &flags);
	if (gpio < 0) {
		errorno = gpio;
		goto dt_exit;
	} else {
		data->drdyPin = gpio;
		pr_info("%s: drdyPin=%d\n",
			__func__, data->drdyPin);
	}
	gpio = of_get_named_gpio_flags(np, "etspi-ldoPin",
		0, &flags);
	if (gpio < 0) {
		data->ldo_pin = 0;
		pr_err("%s: fail to get ldo_pin\n", __func__);
	} else {
		data->ldo_pin = gpio;
		pr_info("%s: ldo_pin=%d\n",
			__func__, data->ldo_pin);
	}
	if (of_property_read_string(np, "etspi-regulator", &data->btp_vcc) < 0) {
		pr_info("%s not use btp_regulator\n", __func__);
		data->btp_vcc = NULL;
	} else {
		data->regulator_3p3 = regulator_get(NULL, data->btp_vcc);
		if (IS_ERR(data->regulator_3p3) ||
				(data->regulator_3p3) == NULL) {
			pr_info("%s not use regulator\n", __func__);
			data->regulator_3p3 = NULL;
		} else {
			pr_info("%s btp_regulator ok\n", __func__);
		}
	}

	if (of_property_read_string(np, "etspi-regulator1p8", &data->btp_vdd) < 0) {
		pr_info("%s not use btp_regulator1p8\n", __func__);
		data->btp_vdd = NULL;
	} else {
		data->regulator_1p8 = regulator_get(NULL, data->btp_vdd);
		if (IS_ERR(data->regulator_1p8) ||
				(data->regulator_1p8) == NULL) {
			pr_info("%s not use regulator1p8\n", __func__);
			data->regulator_1p8 = NULL;
		} else {
			pr_info("%s btp_regulator ok\n", __func__);
		}
	}

	if (of_property_read_string(np, "etspi-regulator1p8", &data->btp_vdd) < 0) {
		pr_info("%s not use btp_regulator1p8\n", __func__);
		data->btp_vdd = NULL;
	} else {
		data->regulator_1p8 = regulator_get(NULL, data->btp_vdd);
		if (IS_ERR(data->regulator_1p8) ||
				(data->regulator_1p8) == NULL) {
			pr_info("%s not use regulator1p8\n", __func__);
			data->regulator_1p8 = NULL;
		} else {
			pr_info("%s btp_regulator1p8 ok\n", __func__);
		}
	}
	if (of_property_read_u32(np, "etspi-min_cpufreq_limit",
		&data->min_cpufreq_limit))
		data->min_cpufreq_limit = 0;
	if (of_property_read_string_index(np, "etspi-chipid", 0,
			(const char **)&data->chipid)) {
		data->chipid = NULL;
	}
	pr_info("%s: chipid: %s\n", __func__, data->chipid);
	if (of_property_read_u32(np, "etspi-orient", &data->orient))
		data->orient = 0;
	pr_info("%s: orient: %d\n", __func__, data->orient);
	data->p = pinctrl_get_select(dev, "default");
	if (IS_ERR(data->p)) {
		errorno = -EINVAL;
		pr_err("%s: failed pinctrl_get\n", __func__);
		goto dt_exit;
	}
	data->pins_sleep = pinctrl_lookup_state(data->p, "sleep");
	if (IS_ERR(data->pins_sleep)) {
		errorno = -EINVAL;
		pr_err("%s : could not get pins sleep_state (%li)\n",
			__func__, PTR_ERR(data->pins_sleep));
		goto fail_pinctrl_get;
	}
	data->pins_idle = pinctrl_lookup_state(data->p, "idle");
	if (IS_ERR(data->pins_idle)) {
		errorno = -EINVAL;
		pr_err("%s : could not get pins idle_state (%li)\n",
			__func__, PTR_ERR(data->pins_idle));
		goto fail_pinctrl_get;
	}
	etspi_pin_control(data, false);
	pr_info("%s is successful\n", __func__);
	return errorno;
fail_pinctrl_get:
	pinctrl_put(data->p);
dt_exit:
	pr_err("%s is failed\n", __func__);
	return errorno;
}
static const struct file_operations etspi_fops = {
	.owner = THIS_MODULE,
	.write = etspi_write,
	.read = etspi_read,
	.unlocked_ioctl = etspi_ioctl,
	.compat_ioctl = etspi_compat_ioctl,
	.open = etspi_open,
	.release = etspi_release,
	.llseek = no_llseek,
	.poll = etspi_fps_interrupt_poll
};
#ifndef ENABLE_SENSORS_FPRINT_SECURE
static int etspi_type_check(struct etspi_data *etspi)
{
	u8 buf1, buf2, buf3, buf4, buf5, buf6, buf7;
	etspi_power_control(g_data, 1);
	msleep(20);
	etspi_read_register(etspi, 0x00, &buf1);
	/*
	 * ET5xx  : 0xAA
	 * ET6xx  : 0xA8
	 */
	if ((buf1 != 0xAA) && (buf1 != 0xA8)) {
		etspi->sensortype = SENSOR_FAILED;
		pr_info("%s sensor not ready, status = %x\n", __func__, buf1);
		etspi_power_control(g_data, 0);
		return -ENODEV;
	}
	etspi_read_register(etspi, 0xFD, &buf1);
	etspi_read_register(etspi, 0xFE, &buf2);
	etspi_read_register(etspi, 0xFF, &buf3);
	etspi_read_register(etspi, 0x20, &buf4);
	etspi_read_register(etspi, 0x21, &buf5);
	etspi_read_register(etspi, 0x23, &buf6);
	etspi_read_register(etspi, 0x24, &buf7);
	etspi_power_control(g_data, 0);
	pr_info("%s buf1-7: %x, %x, %x, %x, %x, %x, %x\n",
		__func__, buf1, buf2, buf3, buf4, buf5, buf6, buf7);
	/*
	 * type check return value
	 * ET510C : 0X00 / 0X66 / 0X00 / 0X33
	 * ET510D : 0x03 / 0x0A / 0x05
	 * ET512B : 0x01 / 0x0C / 0x05
	 * ET516A : 0x00 / 0x10 / 0x05
	 * ET520  : 0x03 / 0x14 / 0x05
	 * ET520E  : 0x04 / 0x14 / 0x05
	 * ET523  : 0x00 / 0x17 / 0x05
	 * ET603  : 0x00 / 0x03 / 0x06
	 */
	if ((buf1 == 0x00) && (buf2 == 0x10) && (buf3 == 0x05)) {
		etspi->sensortype = SENSOR_EGIS;
		pr_info("%s sensor type is EGIS ET516A sensor\n", __func__);
	} else if ((buf1 == 0x03) && (buf2 == 0x0A) && (buf3 == 0x05)) {
		etspi->sensortype = SENSOR_EGIS;
		pr_info("%s sensor type is EGIS ET510D sensor\n", __func__);
	} else if ((buf1 == 0x01) && (buf2 == 0x0C) && (buf3 == 0x05)) {
		etspi->sensortype = SENSOR_EGIS;
		pr_info("%s sensor type is EGIS ET512B sensor\n", __func__);
	} else if ((buf1 == 0x03) && (buf2 == 0x14) && (buf3 == 0x05)) {
		etspi->sensortype = SENSOR_EGIS;
		pr_info("%s sensor type is EGIS ET520 sensor\n", __func__);
	} else if ((buf1 == 0x04) && (buf2 == 0x14) && (buf3 == 0x05)) {
		etspi->sensortype = SENSOR_EGIS;
		pr_info("%s sensor type is EGIS ET520E sensor\n", __func__);
	} else if ((buf1 == 0x00) && (buf2 == 0x17) && (buf3 == 0x05)) {
		etspi->sensortype = SENSOR_EGIS;
		pr_info("%s sensor type is EGIS ET523 sensor\n", __func__);
	} else if ((buf1 == 0x00) && (buf2 == 0x03) && (buf3 == 0x06)) {
		etspi->sensortype = SENSOR_EGIS;
		pr_info("%s sensor type is EGIS ET603 sensor\n", __func__);
	} else {
		if ((buf4 == 0x00) && (buf5 == 0x66)
				&& (buf6 == 0x00) && (buf7 == 0x33)) {
			etspi->sensortype = SENSOR_EGIS;
			pr_info("%s sensor type is EGIS ET510C sensor\n",
					__func__);
		} else {
			etspi->sensortype = SENSOR_FAILED;
			pr_info("%s sensor type is FAILED\n", __func__);
			return -ENODEV;
		}
	}
	return 0;
}
#endif
static ssize_t etspi_bfs_values_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct etspi_data *data = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "\"FP_SPICLK\":\"%d\"\n",
			data->spi->max_speed_hz);
}
static ssize_t etspi_type_check_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct etspi_data *data = dev_get_drvdata(dev);
#ifndef ENABLE_SENSORS_FPRINT_SECURE
	int retry = 0;
	int status = 0;
	do {
		status = etspi_type_check(data);
		pr_info("%s type (%u), retry (%d)\n"
			, __func__, data->sensortype, retry);
	} while (!data->sensortype && ++retry < 3);
	if (status == -ENODEV)
		pr_info("%s type check fail\n", __func__);
#endif
	return snprintf(buf, PAGE_SIZE, "%d\n", data->sensortype);
}
static ssize_t etspi_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}
static ssize_t etspi_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", g_data->chipid);
}
static ssize_t etspi_adm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", DETECT_ADM);
}
static ssize_t etspi_intcnt_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct etspi_data *data = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d\n", data->interrupt_count);
}
static ssize_t etspi_intcnt_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	struct etspi_data *data = dev_get_drvdata(dev);
	if (sysfs_streq(buf, "c")) {
		data->interrupt_count = 0;
		pr_info("initialization is done\n");
	}
	return size;
}
static ssize_t etspi_resetcnt_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct etspi_data *data = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d\n", data->reset_count);
}
static ssize_t etspi_resetcnt_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	struct etspi_data *data = dev_get_drvdata(dev);
	if (sysfs_streq(buf, "c")) {
		data->reset_count = 0;
		pr_info("initialization is done\n");
	}
	return size;
}
static DEVICE_ATTR(bfs_values, 0444, etspi_bfs_values_show, NULL);
static DEVICE_ATTR(type_check, 0444, etspi_type_check_show, NULL);
static DEVICE_ATTR(vendor, 0444, etspi_vendor_show, NULL);
static DEVICE_ATTR(name, 0444, etspi_name_show, NULL);
static DEVICE_ATTR(adm, 0444, etspi_adm_show, NULL);
static DEVICE_ATTR(intcnt, 0664, etspi_intcnt_show, etspi_intcnt_store);
static DEVICE_ATTR(resetcnt, 0664, etspi_resetcnt_show, etspi_resetcnt_store);
static struct device_attribute *fp_attrs[] = {
	&dev_attr_bfs_values,
	&dev_attr_type_check,
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_adm,
	&dev_attr_intcnt,
	&dev_attr_resetcnt,
	NULL,
};
static void etspi_work_func_debug(struct work_struct *work)
{
	pr_info("%s ldo: %d, sleep: %d, tz: %d, spi_value: 0x%x, type: %s\n",
		__func__,
		g_data->ldo_enabled, gpio_get_value(g_data->sleepPin),
		g_data->tz_mode, g_data->spi_value,
		sensor_status[g_data->sensortype + 2]);
}
static void etspi_enable_debug_timer(void)
{
	mod_timer(&g_data->dbg_timer,
		round_jiffies_up(jiffies + FPSENSOR_DEBUG_TIMER_SEC));
}
static void etspi_disable_debug_timer(void)
{
	del_timer_sync(&g_data->dbg_timer);
	cancel_work_sync(&g_data->work_debug);
}
static void etspi_timer_func(struct timer_list *t)
{
	queue_work(g_data->wq_dbg, &g_data->work_debug);
	mod_timer(&g_data->dbg_timer,
		round_jiffies_up(jiffies + FPSENSOR_DEBUG_TIMER_SEC));
}
static int etspi_set_timer(struct etspi_data *etspi)
{
	int status = 0;
	timer_setup(&etspi->dbg_timer, etspi_timer_func, 0);
	etspi->wq_dbg =
		create_singlethread_workqueue("etspi_debug_wq");
	if (!etspi->wq_dbg) {
		status = -ENOMEM;
		pr_err("%s could not create workqueue\n", __func__);
		return status;
	}
	INIT_WORK(&etspi->work_debug, etspi_work_func_debug);
	return status;
}
/*-------------------------------------------------------------------------*/
static struct class *etspi_class;
/*-------------------------------------------------------------------------*/
static int etspi_probe(struct spi_device *spi)
{
	struct etspi_data *etspi;
	int status;
	unsigned long minor;
#ifndef ENABLE_SENSORS_FPRINT_SECURE
	int retry = 0;
#endif
#ifdef FP_DDR_FREQ_CONTROL
	int i = 0;
#endif
	pr_info("%s\n", __func__);
	/* Allocate driver data */
	etspi = kzalloc(sizeof(*etspi), GFP_KERNEL);
	if (!etspi)
		return -ENOMEM;
	/* device tree call */
	if (spi->dev.of_node) {
		status = etspi_parse_dt(&spi->dev, etspi);
		if (status) {
			pr_err("%s - Failed to parse DT\n", __func__);
			goto etspi_probe_parse_dt_failed;
		}
	}
	/* Initialize the driver data */
	etspi->spi = spi;
	g_data = etspi;
	spin_lock_init(&etspi->spi_lock);
	mutex_init(&etspi->buf_lock);
	mutex_init(&device_list_lock);
	INIT_LIST_HEAD(&etspi->device_entry);
	/* platform init */
	status = etspi_platformInit(etspi);
	if (status != 0) {
		pr_err("%s platforminit failed\n", __func__);
		goto etspi_probe_platformInit_failed;
	}
	spi->bits_per_word = 8;
	spi->max_speed_hz = SLOW_BAUD_RATE;
	spi->mode = SPI_MODE_0;
	spi->chip_select = 0;
#ifndef ENABLE_SENSORS_FPRINT_SECURE
	status = spi_setup(spi);
	if (status != 0) {
		pr_err("%s spi_setup() is failed. status : %d\n",
			__func__, status);
		return status;
	}
#endif
	etspi->spi_value = 0;
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	etspi->sensortype = SENSOR_UNKNOWN;
#else
	/* sensor hw type check */
	do {
		status = etspi_type_check(etspi);
		pr_info("%s type (%u), retry (%d)\n"
			, __func__, etspi->sensortype, retry);
	} while (!etspi->sensortype && ++retry < 3);
	if (status == -ENODEV)
		pr_info("%s type check fail\n", __func__);
#endif
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	etspi->tz_mode = true;
#endif
	etspi->reset_count = 0;
	etspi->interrupt_count = 0;
	/* If we can allocate a minor number, hook up this device.
	 * Reusing minors is fine so long as udev or mdev is working.
	 */
	//delete this condition to make device live always
	//if (etspi->sensortype != SENSOR_FAILED) {
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;
		etspi->devt = MKDEV(ET5XX_MAJOR, minor);
		dev = device_create(etspi_class, &spi->dev,
				etspi->devt, etspi, "esfp0");
		status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
	} else {
		dev_dbg(&spi->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&etspi->device_entry, &device_list);
	}
	mutex_unlock(&device_list_lock);
	if (status == 0)
		spi_set_drvdata(spi, etspi);
	else
		goto etspi_probe_failed;
	status = fingerprint_register(etspi->fp_device,
		etspi, fp_attrs, "fingerprint");
	if (status) {
		pr_err("%s sysfs register failed\n", __func__);
		goto etspi_probe_failed;
	}
	status = etspi_set_timer(etspi);
	if (status)
		goto etspi_sysfs_failed;
	etspi_enable_debug_timer();
#ifdef FP_DDR_FREQ_CONTROL
	// register ddr freq setting table
	fill_bus_vector();
	for (i = 0; i < fpsensor_reg_bus_scale_table.num_usecases; i++) {
		fpsensor_reg_bus_usecases[i].num_paths = 1;
		fpsensor_reg_bus_usecases[i].vectors =
			&fpsensor_reg_bus_vectors[i];
	}
	bus_hdl = msm_bus_scale_register_client(&fpsensor_reg_bus_scale_table);
	if (!bus_hdl)

		pr_info("%s msm_bus_scale_register_client failed %d\n"
			, __func__, bus_hdl);
#endif
	pr_info("%s is successful\n", __func__);
	return status;
etspi_sysfs_failed:
	fingerprint_unregister(etspi->fp_device, fp_attrs);
etspi_probe_failed:
	device_destroy(etspi_class, etspi->devt);
	class_destroy(etspi_class);
	etspi_platformUninit(etspi);
etspi_probe_platformInit_failed:
etspi_probe_parse_dt_failed:
	kfree(etspi);
	pr_err("%s is failed\n", __func__);
	return status;
}
static int etspi_remove(struct spi_device *spi)
{
	struct etspi_data *etspi = spi_get_drvdata(spi);
	pr_info("%s\n", __func__);
	if (etspi != NULL) {
#ifdef FP_DDR_FREQ_CONTROL
		if (bus_hdl) {
			msm_bus_scale_unregister_client(bus_hdl);
			pr_info("%s msm_bus_scale_unregister_client\n"

				, __func__);
		}
#endif
		etspi_disable_debug_timer();
		etspi_platformUninit(etspi);
		/* make sure ops on existing fds can abort cleanly */
		spin_lock_irq(&etspi->spi_lock);
		etspi->spi = NULL;
		spi_set_drvdata(spi, NULL);
		spin_unlock_irq(&etspi->spi_lock);
		/* prevent new opens */
		mutex_lock(&device_list_lock);
		fingerprint_unregister(etspi->fp_device, fp_attrs);
		list_del(&etspi->device_entry);
		device_destroy(etspi_class, etspi->devt);
		clear_bit(MINOR(etspi->devt), minors);
		if (etspi->users == 0)
			kfree(etspi);
		mutex_unlock(&device_list_lock);
	}
	return 0;
}
static int etspi_pm_suspend(struct device *dev)
{
	pr_info("%s\n", __func__);
	if (g_data != NULL) {
		etspi_disable_debug_timer();
		if (!g_data->drdy_irq_flag) {
			g_data->drdy_irq_flag = DRDY_IRQ_DISABLE;
		}
	}
	return 0;
}
static int etspi_pm_resume(struct device *dev)
{
	pr_info("%s\n", __func__);
	if (g_data != NULL) {
		etspi_enable_debug_timer();
	}
	return 0;
}
static const struct dev_pm_ops etspi_pm_ops = {
	.suspend = etspi_pm_suspend,
	.resume = etspi_pm_resume
};
static const struct of_device_id etspi_match_table[] = {
	{ .compatible = "etspi,et5xx",},
	{},
};
static struct spi_driver etspi_spi_driver = {
	.driver = {
		.name =	"egis_fingerprint",
		.owner = THIS_MODULE,
		.pm = &etspi_pm_ops,
		.of_match_table = etspi_match_table
	},
	.probe = etspi_probe,
	.remove = etspi_remove,
};
/*-------------------------------------------------------------------------*/
static int __init etspi_init(void)
{
	int status;
	pr_info("%s\n", __func__);
	/* Claim our 256 reserved device numbers.  Then register a class
	 * that will key udev/mdev to add/remove /dev nodes.  Last, register
	 * the driver which manages those device numbers.
	 */
	BUILD_BUG_ON(N_SPI_MINORS > 256);
	status = register_chrdev(ET5XX_MAJOR, "egis_fingerprint", &etspi_fops);
	if (status < 0) {
		pr_err("%s register_chrdev error.status:%d\n", __func__,
				status);
		return status;
	}
	etspi_class = class_create(THIS_MODULE, "egis_fingerprint");
	if (IS_ERR(etspi_class)) {
		pr_err("%s class_create error.\n", __func__);
		unregister_chrdev(ET5XX_MAJOR, etspi_spi_driver.driver.name);
		return PTR_ERR(etspi_class);
	}
	status = spi_register_driver(&etspi_spi_driver);
	if (status < 0) {
		pr_err("%s spi_register_driver error.\n", __func__);
		class_destroy(etspi_class);
		unregister_chrdev(ET5XX_MAJOR, etspi_spi_driver.driver.name);
		return status;
	}
	pr_info("%s is successful\n", __func__);
	return status;
}
static void __exit etspi_exit(void)
{
	pr_info("%s\n", __func__);
	spi_unregister_driver(&etspi_spi_driver);
	class_destroy(etspi_class);
	unregister_chrdev(ET5XX_MAJOR, etspi_spi_driver.driver.name);
}
module_init(etspi_init);
module_exit(etspi_exit);
MODULE_AUTHOR("Wang YuWei, <robert.wang@egistec.com>");
MODULE_DESCRIPTION("SPI Interface for ET5XX");
MODULE_LICENSE("GPL");
