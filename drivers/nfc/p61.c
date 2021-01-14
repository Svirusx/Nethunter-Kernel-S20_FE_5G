/*
 * Copyright (C) 2012-2020 NXP Semiconductors
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
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/spi/spi.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/regulator/consumer.h>
#include <linux/ioctl.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/spi/spidev.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/pm_wakeup.h>
#include "p61.h"
#include "pn547.h"
#include "cold_reset.h"
#include "./nfc_logger/nfc_logger.h"

extern long  pn547_dev_ioctl(struct file *filp, unsigned int cmd,
	unsigned long arg);
#ifdef CONFIG_NFC_FEATURE_SN100U
#define P61_SPI_CLOCK     12000000L
#else
#define P61_SPI_CLOCK     8000000L
#endif

/* size of maximum read/write buffer supported by driver */
#ifdef CONFIG_NFC_FEATURE_SN100U
#define MAX_BUFFER_SIZE   780U
#else
#define MAX_BUFFER_SIZE   258U
#endif

/* Different driver debug lever */
enum P61_DEBUG_LEVEL {
	P61_DEBUG_OFF,
	P61_FULL_DEBUG
};

/* Variable to store current debug level request by ioctl */
static unsigned char debug_level = P61_FULL_DEBUG;
static unsigned char pwr_req_on;
#define P61_DBG_MSG(msg...) {\
	switch (debug_level) {\
	case P61_DEBUG_OFF:\
		break;\
	case P61_FULL_DEBUG:\
		pr_info("[NXP-P61] " msg);\
		break;\
		/*fallthrough*/\
	default:\
		pr_err("[NXP-P61] Wrong debug level(%d)\n", debug_level);\
		break;\
	} \
}

#define P61_ERR_MSG(msg...) pr_err("[NXP-P61] " msg)

/* Device specific macro and structure */
struct p61_device {
	wait_queue_head_t read_wq; /* wait queue for read interrupt */
	struct mutex read_mutex; /* read mutex */
	struct mutex write_mutex; /* write mutex */
	struct spi_device *spi;  /* spi device structure */
	struct miscdevice miscdev; /* char device as misc driver */
	unsigned int rst_gpio; /* SW Reset gpio */
	unsigned int irq_gpio; /* P61 will interrupt DH for any ntf */
	bool irq_enabled; /* flag to indicate irq is used */
	unsigned char enable_poll_mode; /* enable the poll mode */
	spinlock_t irq_enabled_lock; /*spin lock for read irq */

	bool tz_mode;
	spinlock_t ese_spi_lock;
	bool isGpio_cfgDone;
	struct wakeup_source *ws;
	bool device_opened;

#ifdef ESE_PINCTRL
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinctrl_state[ESE_SPI_PCTRL_CNT];
#endif
#ifdef CONFIG_NFC_FEATURE_SN100U
	int spi_cs_gpio;
	int pid;
	bool pid_diff;
#endif
#ifdef CONFIG_ESE_SECURE
	struct clk *ese_spi_pclk;
	struct clk *ese_spi_sclk;
#endif
	const char *ap_vendor;
	unsigned char *buf;
};
static struct p61_device *p61_dev;

/* T==1 protocol specific global data */
const unsigned char SOF = 0xA5u;

#ifdef ESE_PINCTRL
static int ese_set_spi_configuration(enum ESE_SPI_PINCTRL status)
{
	int ret = 0;
	char *pin_status[] = {"sleep", "default", "lpm"};

	pr_info("%s [%s]\n", __func__, pin_status[status]);

	if (!p61_dev->pinctrl_state[status]) {
		pr_info("%s pinctrl_state is null\n", __func__);
		return -EINVAL;
	}

	ret = pinctrl_select_state(p61_dev->pinctrl, p61_dev->pinctrl_state[status]);
	if (ret < 0)
		pr_info("%s pinctrl_select_state[%s] failed\n", __func__, pin_status[status]);

	return ret;
}
#endif

#ifdef CONFIG_ESE_SECURE
/**
 * p61_spi_clk_max_rate: finds the nearest lower rate for a clk
 * @clk the clock for which to find nearest lower rate
 * @rate clock frequency in Hz
 * @return nearest lower rate or negative error value
 *
 * Public clock API extends clk_round_rate which is a ceiling function. This
 * function is a floor function implemented as a binary search using the
 * ceiling function.
 */
static long p61_spi_clk_max_rate(struct clk *clk, unsigned long rate)
{
	long lowest_available, nearest_low, step_size, cur;
	long step_direction = -1;
	long guess = rate;
	int  max_steps = 10;

	cur =  clk_round_rate(clk, rate);
	if (cur == rate)
		return rate;

	/* if we got here then: cur > rate */
	lowest_available =  clk_round_rate(clk, 0);
	if (lowest_available > rate)
		return -EINVAL;

	step_size = (rate - lowest_available) >> 1;
	nearest_low = lowest_available;

	while (max_steps-- && step_size) {
		guess += step_size * step_direction;

		cur =  clk_round_rate(clk, guess);

		if ((cur < rate) && (cur > nearest_low))
			nearest_low = cur;

		/*
		 * if we stepped too far, then start stepping in the other
		 * direction with half the step size
		 */
		if (((cur > rate) && (step_direction > 0))
		 || ((cur < rate) && (step_direction < 0))) {
			step_direction = -step_direction;
			step_size >>= 1;
		}
	}
	return nearest_low;
}

static void p61_spi_clock_set(struct p61_device *p61_dev, unsigned long speed)
{
	long rate;

	if (!strcmp(p61_dev->ap_vendor, "qualcomm")) {
		/* finds the nearest lower rate for a clk */
		rate = p61_spi_clk_max_rate(p61_dev->ese_spi_sclk, speed);
		if (rate < 0) {
			pr_err("%s: no match found for requested clock: %lu\n",
				__func__, speed);
			return;
		}
		speed = rate;
		/*pr_info("%s speed:%lu\n", __func__, speed);*/
	} else if (!strcmp(p61_dev->ap_vendor, "slsi")) {
		/* There is half-multiplier */
		speed =  speed * 2;
	}

	clk_set_rate(p61_dev->ese_spi_sclk, speed);
}

static int p61_clk_control(struct p61_device *p61_dev, bool onoff)
{
	static bool old_value;

	if (old_value == onoff) {
		pr_info("%s: ALREADY %s\n", __func__,
			onoff ? "enabled" : "disabled");
		return 0;
	}

	if (onoff == true) {
		p61_spi_clock_set(p61_dev, P61_SPI_CLOCK);
		clk_prepare_enable(p61_dev->ese_spi_pclk);
		clk_prepare_enable(p61_dev->ese_spi_sclk);
		usleep_range(5000, 5100);
		pr_info("%s: clock:%lu\n", __func__,
			clk_get_rate(p61_dev->ese_spi_sclk));
	} else {
		clk_disable_unprepare(p61_dev->ese_spi_pclk);
		clk_disable_unprepare(p61_dev->ese_spi_sclk);
	}
	old_value = onoff;

	pr_info("%s: clock %s\n", __func__, onoff ? "enabled" : "disabled");
	return 0;
}

static int p61_clk_setup(struct device *dev, struct p61_device *p61_dev)
{
	p61_dev->ese_spi_pclk = clk_get(dev, "pclk");
	if (IS_ERR(p61_dev->ese_spi_pclk)) {
		pr_err("%s: Can't get %s\n", __func__, "pclk");
		p61_dev->ese_spi_pclk = NULL;
		goto err_pclk_get;
	}

	p61_dev->ese_spi_sclk = clk_get(dev, "sclk");
	if (IS_ERR(p61_dev->ese_spi_sclk)) {
		pr_err("%s: Can't get %s\n", __func__, "sclk");
		p61_dev->ese_spi_sclk = NULL;
		goto err_sclk_get;
	}

	return 0;
err_sclk_get:
	clk_put(p61_dev->ese_spi_pclk);
err_pclk_get:
	return -EPERM;
}
#endif

static int p61_xfer(struct p61_device *p61_dev,
			struct p61_ioctl_transfer *tr)
{
	int status = 0;
	struct spi_message m;
	struct spi_transfer t;
	/*For SDM845 & linux4.9: need to change spi buffer
	 * from stack to dynamic memory
	 */

	if (p61_dev == NULL || tr == NULL)
		return -EFAULT;

	if (tr->len > DEFAULT_BUFFER_SIZE || !tr->len)
		return -EMSGSIZE;

	spi_message_init(&m);
	memset(&t, 0, sizeof(t));

	memset(p61_dev->buf, 0, tr->len); /*memset 0 for read */
	if (tr->tx_buffer != NULL) { /*write */
		NFC_LOG_REC("%s...write\n", __func__);
		if (copy_from_user(p61_dev->buf, tr->tx_buffer, tr->len) != 0)
			return -EFAULT;
	}

	t.rx_buf = p61_dev->buf;
	t.tx_buf = p61_dev->buf;
	t.len = tr->len;

	spi_message_add_tail(&t, &m);

	status = spi_sync(p61_dev->spi, &m);
	if (status == 0) {
		if (tr->rx_buffer != NULL) { /*read */
			unsigned long missing = 0;

			NFC_LOG_REC("%s...read\n", __func__);
			missing = copy_to_user(tr->rx_buffer, p61_dev->buf, tr->len);
			if (missing != 0)
				tr->len = tr->len - (unsigned int)missing;
		}
	}
	NFC_LOG_REC("%s: length=%d\n", __func__, tr->len);
	return status;

} /* vfsspi_xfer */

static int p61_rw_spi_message(struct p61_device *p61_dev,
				 unsigned long arg)
{
	struct p61_ioctl_transfer   *dup = NULL;
	int err = 0;

	dup = kmalloc(sizeof(struct p61_ioctl_transfer), GFP_KERNEL);
	if (dup == NULL)
		return -ENOMEM;

	if (copy_from_user(dup, (void *)arg,
			   sizeof(struct p61_ioctl_transfer)) != 0) {
		kfree(dup);
		return -EFAULT;
	}

	err = p61_xfer(p61_dev, dup);
	if (err != 0) {
		kfree(dup);
		NFC_LOG_ERR("%s: p61_xfer failed!\n", __func__);
		return err;
	}

	if (copy_to_user((void *)arg, dup,
			 sizeof(struct p61_ioctl_transfer)) != 0) {
		kfree(dup);
		return -EFAULT;
	}
	kfree(dup);
	return 0;
}

/**
 * \ingroup spi_driver
 * \brief Called from SPI LibEse to initilaize the P61 device
 *
 * \param[in]       struct inode *
 * \param[in]       struct file *
 *
 * \retval 0 if ok.
 */
static int p61_dev_open(struct inode *inode, struct file *filp)
{
	struct p61_device *p61_dev = container_of(filp->private_data,
				struct p61_device, miscdev);
	struct spi_device *spidev = NULL;
	struct wakeup_source *ws = p61_dev->ws;

	spidev = spi_dev_get(p61_dev->spi);

	filp->private_data = p61_dev;
	if (p61_dev->device_opened) {
		NFC_LOG_ERR("%s: already opened!\n", __func__);
		return -EBUSY;
	}
	NFC_LOG_INFO("%s: Major No: %d, Minor No: %d\n", __func__,
			imajor(inode), iminor(inode));

	if (!ws->active) {
		NFC_LOG_INFO("%s: wake lock.\n", __func__);
		__pm_stay_awake(ws);
	}

#ifdef CONFIG_ESE_SECURE
	p61_clk_control(p61_dev, true);
#endif

	p61_dev->device_opened = true;
#ifdef CONFIG_NFC_FEATURE_SN100U
	p61_dev->pid = task_pid_nr(current);
	NFC_LOG_INFO("%s: pid:%d\n", __func__, p61_dev->pid);
#endif

	return 0;
}

/**
 * \ingroup spi_driver
 * \brief To configure the P61_SET_PWR/P61_SET_DBG/P61_SET_POLL
 * \n	P61_SET_PWR - hard reset (arg=2), soft reset (arg=1)
 * \n	P61_SET_DBG - Enable/Disable (based on arg value) the driver logs
 * \n	P61_SET_POLL - Configure the driver in poll (arg = 1),
 *							interrupt (arg = 0) based read operation
 * \param[in]       struct file *
 * \param[in]       unsigned int
 * \param[in]       unsigned long
 *
 * \retval 0 if ok.
 *
 */
static long p61_dev_ioctl(struct file *filp, unsigned int cmd,
	unsigned long arg)
{
	int ret = 0;
	struct p61_device *p61_dev = NULL;

	if (_IOC_TYPE(cmd) != P61_MAGIC) {
		NFC_LOG_ERR("%s: invalid magic. cmd=0x%X Received=0x%X Expected=0x%X\n",
			__func__, cmd, _IOC_TYPE(cmd), P61_MAGIC);
		return -ENOTTY;
	}
	NFC_LOG_REC("%s: entered %x\n", __func__, cmd);
	p61_dev = filp->private_data;

	switch (cmd) {
	case P61_SET_PWR:
		if (arg == 2)
			NFC_LOG_INFO("%s: P61_SET_PWR. No Action.\n", __func__);
		break;

	case P61_SET_DBG:
		debug_level = (unsigned char)arg;
		P61_DBG_MSG(KERN_INFO"[NXP-P61] -  Debug level %d",
			debug_level);
		break;
	case P61_SET_POLL:
		p61_dev->enable_poll_mode = (unsigned char)arg;
		if (p61_dev->enable_poll_mode == 0) {
			P61_DBG_MSG(KERN_INFO"[NXP-P61] - IRQ Mode is set\n");
		} else {
			P61_DBG_MSG(KERN_INFO"[NXP-P61] - Poll Mode is set\n");
			p61_dev->enable_poll_mode = 1;
		}
		break;

#if !defined(CONFIG_NFC_FEATURE_SN100U)
	case P61_SET_SPI_CONFIG:
		NFC_LOG_INFO("%s P61_SET_SPI_CONFIG. No Action.\n", __func__);
		break;
	case P61_ENABLE_SPI_CLK:
		NFC_LOG_INFO("%s P61_ENABLE_SPI_CLK. No Action.\n", __func__);
		break;
	case P61_DISABLE_SPI_CLK:
		NFC_LOG_INFO("%s P61_DISABLE_SPI_CLK. No Action.\n", __func__);
		break;
#endif

	case P61_RW_SPI_DATA:
#ifndef CONFIG_ESE_SECURE
		ret = p61_rw_spi_message(p61_dev, arg);
#endif
		break;

	case P61_SET_SPM_PWR:
		NFC_LOG_INFO("%s: P61_SET_SPM_PWR: enter\n", __func__);
		ret = pn547_dev_ioctl(filp, P61_SET_SPI_PWR, arg);
		if (arg == 0 || arg == 1 || arg == 3)
			pwr_req_on = arg;
		NFC_LOG_INFO("%s: P61_SET_SPM_PWR: exit\n", __func__);
		break;

	case P61_GET_SPM_STATUS:
		NFC_LOG_INFO("%s: P61_GET_SPM_STATUS: enter\n", __func__);
		ret = pn547_dev_ioctl(filp, P61_GET_PWR_STATUS, arg);
		NFC_LOG_INFO("%s: P61_GET_SPM_STATUS: exit\n", __func__);
		break;

	case P61_GET_ESE_ACCESS:
		/*P61_DBG_MSG(KERN_ALERT " P61_GET_ESE_ACCESS: enter");*/
		ret = pn547_dev_ioctl(filp, P547_GET_ESE_ACCESS, arg);
		NFC_LOG_INFO("%s: P61_GET_ESE_ACCESS ret: %d exit\n", __func__, ret);
		break;

	case P61_SET_DWNLD_STATUS:
		P61_DBG_MSG(KERN_ALERT "P61_SET_DWNLD_STATUS: enter\n");
		ret = pn547_dev_ioctl(filp, PN547_SET_DWNLD_STATUS, arg);
		NFC_LOG_INFO("%s: P61_SET_DWNLD_STATUS: =%lu exit\n", __func__, arg);
		break;

#ifdef CONFIG_NFC_FEATURE_SN100U
	case ESE_PERFORM_COLD_RESET:
		P61_DBG_MSG(KERN_ALERT " ESE_PERFORM_COLD_RESET: enter");
		ret = ese_cold_reset(ESE_COLD_RESET_SOURCE_SPI);
		P61_DBG_MSG(KERN_ALERT " P61_INHIBIT_PWR_CNTRL ret: %d exit", ret);
		break;

	case PERFORM_RESET_PROTECTION:
		P61_DBG_MSG(KERN_ALERT " PERFORM_RESET_PROTECTION: enter");
		ret = do_reset_protection((arg == 1 ? true : false));
		P61_DBG_MSG(KERN_ALERT " PERFORM_RESET_PROTECTION ret: %d exit", ret);
	break;
#endif

	default:
		NFC_LOG_INFO("%s: no matching ioctl!\n", __func__);
		ret = -EINVAL;
	}

	return ret;
}

#ifdef CONFIG_NFC_FEATURE_SN100U
/* this function is defined temporarily to fix build error. this function is used by uwb */
long p61_cold_reset(void)
{
	int ret;

	NFC_LOG_INFO("UWB ESE_COLD_RESET: enter");
	ret = ese_cold_reset(ESE_COLD_RESET_SOURCE_UWB);
	NFC_LOG_INFO("ret: %d exit", ret);

	return ret;
}
#endif

/*
 * Called when a process closes the device file.
 */
static int p61_dev_release(struct inode *inode, struct file *file)
{
	struct p61_device *p61_dev = file->private_data;
	struct wakeup_source *ws = p61_dev->ws;
#ifdef CONFIG_NFC_FEATURE_SN100U
	int pid;
#endif

	NFC_LOG_INFO("%s: ++\n", __func__);

#ifdef CONFIG_NFC_FEATURE_SN100U
	do_reset_protection(false);
#endif

#ifdef CONFIG_ESE_SECURE
	p61_clk_control(p61_dev, false);
#endif
#ifdef CONFIG_NFC_FEATURE_SN100U
	pid = task_pid_nr(current);
	NFC_LOG_INFO("%s: open pid :%d, close pid :%d\n", __func__, p61_dev->pid, pid);
	if (pid != p61_dev->pid)
		p61_dev->pid_diff = true;
	else
		p61_dev->pid_diff = false;
#endif

	if (ws->active) {
		NFC_LOG_INFO("%s: wake unlock.\n", __func__);
		__pm_relax(ws);
	}

	if (pwr_req_on && (pwr_req_on != 5)) {
		NFC_LOG_INFO("%s: release spi session.\n", __func__);
		pwr_req_on = 0;
		pn547_dev_ioctl(file, P61_SET_SPI_PWR, 0);
		pn547_dev_ioctl(file, P61_SET_SPI_PWR, 5);
	}
	p61_dev->device_opened = false;

	return 0;
}

/**
 * \ingroup spi_driver
 * \brief Write data to P61 on SPI
 *
 * \param[in]       struct file *
 * \param[in]       const char *
 * \param[in]       size_t
 * \param[in]       loff_t *
 *
 * \retval data size
 *
 */
static ssize_t p61_dev_write(struct file *filp, const char *buf, size_t count,
	loff_t *offset)
{
	int ret = -1;
	struct p61_device *p61_dev;

	P61_DBG_MSG("%s: -Enter count %zu\n", __func__, count);

#ifdef CONFIG_ESE_SECURE
	return 0;
#endif

	p61_dev = filp->private_data;

	mutex_lock(&p61_dev->write_mutex);
	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	memset(p61_dev->buf, 0, count);
	if (copy_from_user(p61_dev->buf, &buf[0], count)) {
		P61_ERR_MSG("%s: failed to copy from user space\n", __func__);
		mutex_unlock(&p61_dev->write_mutex);
		return -EFAULT;
	}

	/* Write data */
	ret = spi_write(p61_dev->spi, p61_dev->buf, count);
	if (ret < 0)
		ret = -EIO;
	else
		ret = count;

	mutex_unlock(&p61_dev->write_mutex);
	NFC_LOG_INFO("%s: -count %zu  %d- Exit\n", __func__, count, ret);

	return ret;
}

/**
 * \ingroup spi_driver
 * \brief Used to read data from P61 in Poll/interrupt mode configured using
 *  ioctl call
 *
 * \param[in]       struct file *
 * \param[in]       char *
 * \param[in]       size_t
 * \param[in]       loff_t *
 *
 * \retval read size
 *
 */
/* for p61 only */
static ssize_t p61_dev_read(struct file *filp, char *buf, size_t count,
	loff_t *offset)
{
	int ret = -EIO;
	struct p61_device *p61_dev = filp->private_data;
	unsigned char sof = 0x00;
	int total_count = 0;
	//unsigned char rx_buffer[MAX_BUFFER_SIZE];

	P61_DBG_MSG("%s: count %zu - Enter\n", __func__, count);

#ifdef CONFIG_ESE_SECURE
	return 0;
#endif
	if (count < 1) {
		P61_ERR_MSG("Invalid length (min : 258) [%zu]\n", count);
		return -EINVAL;
	}
	mutex_lock(&p61_dev->read_mutex);
	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	//memset(&rx_buffer[0], 0x00, sizeof(rx_buffer));
	memset(p61_dev->buf, 0x00, MAX_BUFFER_SIZE);

	P61_DBG_MSG("%s: Poll Mode Enabled\n", __func__);
	do {
		sof = 0x00;
		ret = spi_read(p61_dev->spi, (void *)&sof, 1);
		if (ret < 0) {
			P61_ERR_MSG("spi_read failed [SOF]\n");
			goto fail;
		}
		//P61_DBG_MSG(KERN_INFO"SPI_READ returned 0x%x\n", sof);
		/* if SOF not received, give some time to P61 */
		/* RC put the conditional delay only if SOF not received */
		if (sof != SOF)
			usleep_range(5000, 5100);
	} while (sof != SOF);
	P61_DBG_MSG("SPI_READ returned 0x%x...\n", sof);

	total_count = 1;
	//rx_buffer[0] = sof;
	*p61_dev->buf = sof;
	/* Read the HEADR of Two bytes*/
	ret = spi_read(p61_dev->spi, p61_dev->buf + 1, 2);
	if (ret < 0) {
		P61_ERR_MSG("spi_read fails after [PCB]\n");
		ret = -EIO;
		goto fail;
	}

	total_count += 2;
	/* Get the data length */
	//count = rx_buffer[2];
	count = *(p61_dev->buf + 2);
	NFC_LOG_REC("Data Length = %zu", count);
	/* Read the available data along with one byte LRC */
	ret = spi_read(p61_dev->spi, (void *)(p61_dev->buf + 3), (count+1));
	if (ret < 0) {
		NFC_LOG_ERR("%s: spi_read failed\n", __func__);
		ret = -EIO;
		goto fail;
	}
	total_count = (total_count + (count+1));
	P61_DBG_MSG(KERN_INFO"total_count = %d", total_count);

	if (copy_to_user(buf, p61_dev->buf, total_count)) {
		P61_ERR_MSG("%s: failed to copy to user space\n", __func__);
		ret = -EFAULT;
		goto fail;
	}
	ret = total_count;
	P61_DBG_MSG("%s: ret %d Exit\n", __func__, ret);

	mutex_unlock(&p61_dev->read_mutex);

	return ret;

fail:
	NFC_LOG_INFO("%s: count %zu  %d- Exit\n", __func__, count, ret);

	mutex_unlock(&p61_dev->read_mutex);
	return ret;
}

/**
 * \ingroup spi_driver
 * \brief Set the P61 device specific context for future use.
 * \param[in]       struct spi_device *
 * \param[in]       void *
 * \retval void
 */
static inline void p61_set_data(struct spi_device *spi, void *data)
{
	dev_set_drvdata(&spi->dev, data);
}

/**
 * \ingroup spi_driver
 * \brief Get the P61 device specific context.
 * \param[in]       const struct spi_device *
 * \retval Device Parameters
 */
static inline void *p61_get_data(const struct spi_device *spi)
{
	return dev_get_drvdata(&spi->dev);
}

static const struct file_operations p61_dev_fops = {
	.owner = THIS_MODULE,
	.read = p61_dev_read,
	.write = p61_dev_write,
	.open = p61_dev_open,
	.unlocked_ioctl = p61_dev_ioctl,
	.release = p61_dev_release,
};

static int p61_parse_dt(struct device *dev,
	struct p61_device *p61_dev)
{
	struct device_node *np = dev->of_node;
#ifdef ESE_PINCTRL
	struct device_node *spi_device_node;
	struct platform_device *spi_pdev;
	char *pin_status[] = {"sleep", "default", "lpm"};
	int i;
#endif
	int ese_det_gpio;

	if (!of_property_read_string(np, "p61-ap_vendor",
		&p61_dev->ap_vendor)) {
		NFC_LOG_INFO("%s: ap_vendor - %s\n", __func__, p61_dev->ap_vendor);
	}

	ese_det_gpio = of_get_named_gpio(np, "ese-det-gpio", 0);
	if (!gpio_is_valid(ese_det_gpio)) {
		NFC_LOG_INFO("%s: ese-det-gpio is not set\n", __func__);
	} else {
		gpio_request(ese_det_gpio, "ese_det_gpio");
		gpio_direction_input(ese_det_gpio);
		if (!gpio_get_value(ese_det_gpio)) {
			NFC_LOG_INFO("%s: ese is not supported\n", __func__);
			return -ENODEV;
		}
		NFC_LOG_INFO("%s: ese is supported\n", __func__);
	}

#ifdef CONFIG_NFC_FEATURE_SN100U
	p61_dev->spi_cs_gpio = of_get_named_gpio(np, "ese-spi_cs-gpio", 0);
	if (!gpio_is_valid(p61_dev->spi_cs_gpio))
		NFC_LOG_INFO("%s: cs_gpio is not set\n", __func__);
	else
		NFC_LOG_INFO("%s: cs_gpio is %d\n", __func__, p61_dev->spi_cs_gpio);
#endif
#ifdef ESE_PINCTRL
	spi_device_node = of_parse_phandle(np,
			"p61-spi_node", 0);
	if (!IS_ERR_OR_NULL(spi_device_node)) {
		spi_pdev = of_find_device_by_node(spi_device_node);
		if (!spi_pdev) {
			NFC_LOG_INFO("%s: of_find_device_by_node failed\n", __func__);
			goto end;
		}

		p61_dev->pinctrl = devm_pinctrl_get(&spi_pdev->dev);
		if (IS_ERR(p61_dev->pinctrl)) {
			NFC_LOG_INFO("%s: devm_pinctrl_get failed\n", __func__);
			goto end;
		}

		for (i = 0; i < ESE_SPI_PCTRL_CNT; i++) {
			p61_dev->pinctrl_state[i] = pinctrl_lookup_state(p61_dev->pinctrl, pin_status[i]);
			if (IS_ERR(p61_dev->pinctrl_state[i])) {
				NFC_LOG_INFO("%s: pinctrl_lookup_state[%s] failed\n", __func__, pin_status[i]);
				p61_dev->pinctrl_state[i] = NULL;
			}
		}
	} else {
		NFC_LOG_INFO("%s: of_parse_phandle failed\n", __func__);
	}
end:
#endif
	return 0;
}

#if defined(CONFIG_NFC_FEATURE_SN100U)
static void p61_shutdown(void)
{
	if (p61_dev && !p61_dev->device_opened && !p61_dev->pid_diff) {
		pr_info("%s : pid_diff %s\n", __func__, p61_dev->pid_diff ? "true" : "false");
		p61_dev->device_opened = true;
		ese_set_spi_configuration(ESE_SPI_LPM);
		P61_DBG_MSG("%s\n", __func__);
	}
}
#endif

/**
 * \ingroup spi_driver
 * \brief To probe for P61 SPI interface. If found initialize the SPI clock,
 * bit rate & SPI mode. It will create the dev entry (P61) for user space.
 *
 * \param[in]       struct spi_device *
 *
 * \retval 0 if ok.
 *
 */
static int p61_probe(struct spi_device *spi)
{
	int ret = -1;

	pr_info("%s: chip select(%d), bus number(%d)\n",
		__func__, spi->chip_select, spi->master->bus_num);

	nfc_logger_init();

	p61_dev = kzalloc(sizeof(*p61_dev), GFP_KERNEL);
	if (p61_dev == NULL) {
		P61_ERR_MSG("failed to allocate memory for module data\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	ret = p61_parse_dt(&spi->dev, p61_dev);
	if (ret) {
		NFC_LOG_ERR("%s: Failed to parse DT\n", __func__);
		goto p61_parse_dt_failed;
	}
	NFC_LOG_INFO("%s: tz_mode=%d, isGpio_cfgDone:%d\n", __func__,
			p61_dev->tz_mode, p61_dev->isGpio_cfgDone);

	spi->bits_per_word = 8;
	spi->mode = SPI_MODE_0;
	spi->max_speed_hz = P61_SPI_CLOCK;
#ifndef CONFIG_ESE_SECURE
	ret = spi_setup(spi);
	if (ret < 0) {
		P61_ERR_MSG("failed to do spi_setup()\n");
		goto p61_spi_setup_failed;
	}
#else
	pr_info("%s: eSE Secured system\n", __func__);
	ret = p61_clk_setup(&spi->dev, p61_dev);
	if (ret)
		pr_err("%s - Failed to do clk_setup\n", __func__);
#endif
	p61_dev->spi = spi;
	p61_dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	p61_dev->miscdev.name = "p61";
	p61_dev->miscdev.fops = &p61_dev_fops;
	p61_dev->miscdev.parent = &spi->dev;

	dev_set_drvdata(&spi->dev, p61_dev);
#ifdef ESE_PINCTRL
	if (lpcharge)
		ese_set_spi_configuration(ESE_SPI_LPM);
	else
		ese_set_spi_configuration(ESE_SPI_SLEEP);
#endif

	/* init mutex and queues */
	init_waitqueue_head(&p61_dev->read_wq);
	mutex_init(&p61_dev->read_mutex);
	mutex_init(&p61_dev->write_mutex);
	spin_lock_init(&p61_dev->ese_spi_lock);

	p61_dev->ws = wakeup_source_register(&spi->dev, "ese_ws");
	p61_dev->device_opened = false;

	if (!lpcharge) {
		ret = misc_register(&p61_dev->miscdev);
		if (ret < 0) {
			P61_ERR_MSG("misc_register failed! %d\n", ret);
			goto err_exit0;
		}
	}

	p61_dev->enable_poll_mode = 1; /* No USE? */

	p61_dev->buf = kzalloc(sizeof(unsigned char) * MAX_BUFFER_SIZE, GFP_KERNEL);
	if (p61_dev->buf == NULL) {
		P61_ERR_MSG("failed to allocate for spi buffer\n");
		ret = -ENOMEM;
		goto err_exit0;
	}
#if defined(CONFIG_NFC_FEATURE_SN100U)
	pn547_register_ese_shutdown(p61_shutdown);
#endif

	NFC_LOG_INFO("%s: finished\n", __func__);
	return ret;

err_exit0:
	mutex_destroy(&p61_dev->read_mutex);
	mutex_destroy(&p61_dev->write_mutex);
	wakeup_source_remove(p61_dev->ws);

#ifndef CONFIG_ESE_SECURE
p61_spi_setup_failed:
#endif
p61_parse_dt_failed:
	if (p61_dev != NULL)
		kfree(p61_dev);
err_exit:
	P61_DBG_MSG("ERROR: Exit : %s ret %d\n", __func__, ret);
	return ret;
}

/**
 * \ingroup spi_driver
 * \brief Will get called when the device is removed to release the resources.
 * \param[in]       struct spi_device
 * \retval 0 if ok.
 */
static int p61_remove(struct spi_device *spi)
{
	struct p61_device *p61_dev = p61_get_data(spi);

	P61_DBG_MSG("Entry : %s\n", __func__);
	mutex_destroy(&p61_dev->read_mutex);
	misc_deregister(&p61_dev->miscdev);
	wakeup_source_remove(p61_dev->ws);
	kfree(p61_dev->buf);
	kfree(p61_dev);

	P61_DBG_MSG("Exit : %s\n", __func__);
	return 0;
}

static const struct of_device_id p61_match_table[] = {
	{ .compatible = "p61",},
	{},
};

static struct spi_driver p61_driver = {
	.driver = {
		.name = "p61",
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = p61_match_table,
#endif
	},
	.probe =  p61_probe,
	.remove = p61_remove,
};

/**
 * \ingroup spi_driver
 * \brief Module init interface
 *
 * \param[in]       void
 *
 * \retval handle
 *
 */
static int __init p61_dev_init(void)
{
	debug_level = P61_FULL_DEBUG;

	P61_DBG_MSG("Entry : %s\n", __func__);

	return spi_register_driver(&p61_driver);

	P61_DBG_MSG("Exit : %s\n", __func__);
}

/**
 * \ingroup spi_driver
 * \brief Module exit interface
 *
 * \param[in]       void
 *
 * \retval void
 *
 */
static void __exit p61_dev_exit(void)
{
	P61_DBG_MSG("Entry : %s\n", __func__);
	spi_unregister_driver(&p61_driver);
	P61_DBG_MSG("Exit : %s\n", __func__);
}

/* module_init(p61_dev_init); */
late_initcall(p61_dev_init);
module_exit(p61_dev_exit);

MODULE_AUTHOR("BHUPENDRA PAWAR");
MODULE_DESCRIPTION("NXP P61 SPI driver");
MODULE_LICENSE("GPL");
