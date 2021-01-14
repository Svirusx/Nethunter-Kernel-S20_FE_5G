// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2010-2011, 2020, The Linux Foundation. All rights reserved.
 */

#include <linux/of.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#ifdef CONFIG_RTC_AUTO_PWRON
#include <linux/reboot.h>
#include <linux/wakelock.h>
#include <linux/alarmtimer.h>
#include <linux/time.h>
#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
#include <linux/sec_param.h>

#define SAPA_KPARAM_MAGIC	0x41504153
extern unsigned int sapa_param_time;
#endif
#define SAPA_START_POLL_TIME   (10LL * NSEC_PER_SEC) /* 10 sec */
#define SAPA_BOOTING_TIME      (5*60)
#define SAPA_POLL_TIME         (15*60)

enum {
	SAPA_DISTANT = 0,
	SAPA_NEAR,
	SAPA_EXPIRED,
	SAPA_OVER
};

#define TO_SECS(arr)        (arr[0] | (arr[1] << 8) | (arr[2] << 16) | \
                            (arr[3] << 24))

extern unsigned int lpcharge;
#endif

/* RTC Register offsets from RTC CTRL REG */
#define PM8XXX_ALARM_CTRL_OFFSET	0x01
#define PM8XXX_RTC_WRITE_OFFSET		0x02
#define PM8XXX_RTC_READ_OFFSET		0x06
#define PM8XXX_ALARM_RW_OFFSET		0x0A

/* RTC_CTRL register bit fields */
#define PM8xxx_RTC_ENABLE		BIT(7)
#define PM8xxx_RTC_ALARM_CLEAR		BIT(0)
#define PM8xxx_RTC_ALARM_ENABLE		BIT(7)

#define NUM_8_BIT_RTC_REGS		0x4

/**
 * struct pm8xxx_rtc_regs - describe RTC registers per PMIC versions
 * @ctrl: base address of control register
 * @write: base address of write register
 * @read: base address of read register
 * @alarm_ctrl: base address of alarm control register
 * @alarm_ctrl2: base address of alarm control2 register
 * @alarm_rw: base address of alarm read-write register
 * @alarm_en: alarm enable mask
 */
struct pm8xxx_rtc_regs {
	unsigned int ctrl;
	unsigned int write;
	unsigned int read;
	unsigned int alarm_ctrl;
	unsigned int alarm_ctrl2;
	unsigned int alarm_rw;
	unsigned int alarm_en;
};

/**
 * struct pm8xxx_rtc -  rtc driver internal structure
 * @rtc:		rtc device for this driver.
 * @regmap:		regmap used to access RTC registers
 * @allow_set_time:	indicates whether writing to the RTC is allowed
 * @rtc_alarm_irq:	rtc alarm irq number.
 * @ctrl_reg:		rtc control register.
 * @rtc_dev:		device structure.
 * @ctrl_reg_lock:	spinlock protecting access to ctrl_reg.
 */
struct pm8xxx_rtc {
	struct rtc_device *rtc;
	struct regmap *regmap;
	bool allow_set_time;
	int rtc_alarm_irq;
	const struct pm8xxx_rtc_regs *regs;
	struct device *rtc_dev;
	spinlock_t ctrl_reg_lock;
#ifdef CONFIG_RTC_AUTO_PWRON
	struct rtc_wkalrm   sapa;
	struct alarm        check_poll;
	struct work_struct  check_func;
	struct wake_lock    wakelock;
	int                 lpm_mode;
	unsigned char       triggered;
#endif
};

/*
 * Steps to write the RTC registers.
 * 1. Disable alarm if enabled.
 * 2. Disable rtc if enabled.
 * 3. Write 0x00 to LSB.
 * 4. Write Byte[1], Byte[2], Byte[3] then Byte[0].
 * 5. Enable rtc if disabled in step 2.
 * 6. Enable alarm if disabled in step 1.
 */
static int pm8xxx_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	int rc, i;
	unsigned long secs, irq_flags;
	u8 value[NUM_8_BIT_RTC_REGS], alarm_enabled = 0, rtc_disabled = 0;
	unsigned int ctrl_reg, rtc_ctrl_reg;
	struct pm8xxx_rtc *rtc_dd = dev_get_drvdata(dev);
	const struct pm8xxx_rtc_regs *regs = rtc_dd->regs;

	if (!rtc_dd->allow_set_time)
		return -EACCES;

	rtc_tm_to_time(tm, &secs);

	dev_dbg(dev, "Seconds value to be written to RTC = %lu\n", secs);

	for (i = 0; i < NUM_8_BIT_RTC_REGS; i++) {
		value[i] = secs & 0xFF;
		secs >>= 8;
	}

	spin_lock_irqsave(&rtc_dd->ctrl_reg_lock, irq_flags);

	rc = regmap_read(rtc_dd->regmap, regs->alarm_ctrl, &ctrl_reg);
	if (rc)
		goto rtc_rw_fail;

	if (ctrl_reg & regs->alarm_en) {
		alarm_enabled = 1;
		ctrl_reg &= ~regs->alarm_en;
		rc = regmap_write(rtc_dd->regmap, regs->alarm_ctrl, ctrl_reg);
		if (rc) {
			dev_err(dev, "Write to RTC Alarm control register failed\n");
			goto rtc_rw_fail;
		}
	}

	/* Disable RTC H/w before writing on RTC register */
	rc = regmap_read(rtc_dd->regmap, regs->ctrl, &rtc_ctrl_reg);
	if (rc)
		goto rtc_rw_fail;

	if (rtc_ctrl_reg & PM8xxx_RTC_ENABLE) {
		rtc_disabled = 1;
		rtc_ctrl_reg &= ~PM8xxx_RTC_ENABLE;
		rc = regmap_write(rtc_dd->regmap, regs->ctrl, rtc_ctrl_reg);
		if (rc) {
			dev_err(dev, "Write to RTC control register failed\n");
			goto rtc_rw_fail;
		}
	}

	/* Write 0 to Byte[0] */
	rc = regmap_write(rtc_dd->regmap, regs->write, 0);
	if (rc) {
		dev_err(dev, "Write to RTC write data register failed\n");
		goto rtc_rw_fail;
	}

	/* Write Byte[1], Byte[2], Byte[3] */
	rc = regmap_bulk_write(rtc_dd->regmap, regs->write + 1,
			       &value[1], sizeof(value) - 1);
	if (rc) {
		dev_err(dev, "Write to RTC write data register failed\n");
		goto rtc_rw_fail;
	}

	/* Write Byte[0] */
	rc = regmap_write(rtc_dd->regmap, regs->write, value[0]);
	if (rc) {
		dev_err(dev, "Write to RTC write data register failed\n");
		goto rtc_rw_fail;
	}

	/* Enable RTC H/w after writing on RTC register */
	if (rtc_disabled) {
		rtc_ctrl_reg |= PM8xxx_RTC_ENABLE;
		rc = regmap_write(rtc_dd->regmap, regs->ctrl, rtc_ctrl_reg);
		if (rc) {
			dev_err(dev, "Write to RTC control register failed\n");
			goto rtc_rw_fail;
		}
	}

	if (alarm_enabled) {
		ctrl_reg |= regs->alarm_en;
		rc = regmap_write(rtc_dd->regmap, regs->alarm_ctrl, ctrl_reg);
		if (rc) {
			dev_err(dev, "Write to RTC Alarm control register failed\n");
			goto rtc_rw_fail;
		}
	}

#ifdef CONFIG_RTC_AUTO_PWRON
	pr_info("%s : secs = %lu, h:m:s == %d:%d:%d, d/m/y = %d/%d/%d\n", __func__,
			secs, tm->tm_hour, tm->tm_min, tm->tm_sec,
			tm->tm_mday, tm->tm_mon, tm->tm_year);
#endif
	
rtc_rw_fail:
	spin_unlock_irqrestore(&rtc_dd->ctrl_reg_lock, irq_flags);

	return rc;
}

static int pm8xxx_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	int rc;
	u8 value[NUM_8_BIT_RTC_REGS];
	unsigned long secs;
	unsigned int reg;
	struct pm8xxx_rtc *rtc_dd = dev_get_drvdata(dev);
	const struct pm8xxx_rtc_regs *regs = rtc_dd->regs;

	rc = regmap_bulk_read(rtc_dd->regmap, regs->read, value, sizeof(value));
	if (rc) {
		dev_err(dev, "RTC read data register failed\n");
		return rc;
	}

	/*
	 * Read the LSB again and check if there has been a carry over.
	 * If there is, redo the read operation.
	 */
	rc = regmap_read(rtc_dd->regmap, regs->read, &reg);
	if (rc < 0) {
		dev_err(dev, "RTC read data register failed\n");
		return rc;
	}

	if (unlikely(reg < value[0])) {
		rc = regmap_bulk_read(rtc_dd->regmap, regs->read,
				      value, sizeof(value));
		if (rc) {
			dev_err(dev, "RTC read data register failed\n");
			return rc;
		}
	}

	secs = value[0] | (value[1] << 8) | (value[2] << 16) |
	       ((unsigned long)value[3] << 24);

	rtc_time_to_tm(secs, tm);

	dev_dbg(dev, "secs = %lu, h:m:s == %d:%d:%d, d/m/y = %d/%d/%d\n",
		secs, tm->tm_hour, tm->tm_min, tm->tm_sec,
		tm->tm_mday, tm->tm_mon, tm->tm_year);

	return 0;
}

static int pm8xxx_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	int rc, i;
	u8 value[NUM_8_BIT_RTC_REGS];
	unsigned int ctrl_reg;
	unsigned long secs, irq_flags;
	struct pm8xxx_rtc *rtc_dd = dev_get_drvdata(dev);
	const struct pm8xxx_rtc_regs *regs = rtc_dd->regs;

	rtc_tm_to_time(&alarm->time, &secs);

	for (i = 0; i < NUM_8_BIT_RTC_REGS; i++) {
		value[i] = secs & 0xFF;
		secs >>= 8;
	}

	spin_lock_irqsave(&rtc_dd->ctrl_reg_lock, irq_flags);

	rc = regmap_bulk_write(rtc_dd->regmap, regs->alarm_rw, value,
			       sizeof(value));
	if (rc) {
		dev_err(dev, "Write to RTC ALARM register failed\n");
		goto rtc_rw_fail;
	}

	rc = regmap_read(rtc_dd->regmap, regs->alarm_ctrl, &ctrl_reg);
	if (rc)
		goto rtc_rw_fail;

	if (alarm->enabled)
		ctrl_reg |= regs->alarm_en;
	else
		ctrl_reg &= ~regs->alarm_en;

	rc = regmap_write(rtc_dd->regmap, regs->alarm_ctrl, ctrl_reg);
	if (rc) {
		dev_err(dev, "Write to RTC alarm control register failed\n");
		goto rtc_rw_fail;
	}

	dev_dbg(dev, "Alarm Set for h:r:s=%d:%d:%d, d/m/y=%d/%d/%d\n",
		alarm->time.tm_hour, alarm->time.tm_min,
		alarm->time.tm_sec, alarm->time.tm_mday,
		alarm->time.tm_mon, alarm->time.tm_year);
rtc_rw_fail:
	spin_unlock_irqrestore(&rtc_dd->ctrl_reg_lock, irq_flags);
	return rc;
}

static int pm8xxx_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	int rc;
	u8 value[NUM_8_BIT_RTC_REGS];
	unsigned long secs;
	struct pm8xxx_rtc *rtc_dd = dev_get_drvdata(dev);
	const struct pm8xxx_rtc_regs *regs = rtc_dd->regs;

	rc = regmap_bulk_read(rtc_dd->regmap, regs->alarm_rw, value,
			      sizeof(value));
	if (rc) {
		dev_err(dev, "RTC alarm time read failed\n");
		return rc;
	}

	secs = value[0] | (value[1] << 8) | (value[2] << 16) |
	       ((unsigned long)value[3] << 24);

	rtc_time_to_tm(secs, &alarm->time);

	rc = rtc_valid_tm(&alarm->time);
	if (rc < 0) {
		dev_err(dev, "Invalid alarm time read from RTC\n");
		return rc;
	}

	dev_dbg(dev, "Alarm set for - h:r:s=%d:%d:%d, d/m/y=%d/%d/%d\n",
		alarm->time.tm_hour, alarm->time.tm_min,
		alarm->time.tm_sec, alarm->time.tm_mday,
		alarm->time.tm_mon, alarm->time.tm_year);

	rc = regmap_bulk_read(rtc_dd->regmap, regs->alarm_ctrl, value, 1);
	if (rc) {
		dev_err(dev, "Read from ALARM CTRL1 failed\n");
		return rc;
	}

	alarm->enabled = !!(value[0] & PM8xxx_RTC_ALARM_ENABLE);

	return 0;
}

static int pm8xxx_rtc_alarm_irq_enable(struct device *dev, unsigned int enable)
{
	int rc;
	unsigned long irq_flags;
	struct pm8xxx_rtc *rtc_dd = dev_get_drvdata(dev);
	const struct pm8xxx_rtc_regs *regs = rtc_dd->regs;
	unsigned int ctrl_reg;
	u8 value[NUM_8_BIT_RTC_REGS] = {0};

#ifdef CONFIG_RTC_AUTO_PWRON
	pr_info("[SAPA] Alarm irq=%d\n", enable);
#endif
	
	spin_lock_irqsave(&rtc_dd->ctrl_reg_lock, irq_flags);

	rc = regmap_read(rtc_dd->regmap, regs->alarm_ctrl, &ctrl_reg);
	if (rc)
		goto rtc_rw_fail;

	if (enable)
		ctrl_reg |= regs->alarm_en;
	else
		ctrl_reg &= ~regs->alarm_en;

	rc = regmap_write(rtc_dd->regmap, regs->alarm_ctrl, ctrl_reg);
	if (rc) {
		dev_err(dev, "Write to RTC control register failed\n");
		goto rtc_rw_fail;
	}

	/* Clear Alarm register */
	if (!enable) {
		rc = regmap_bulk_write(rtc_dd->regmap, regs->alarm_rw, value,
					sizeof(value));
		if (rc) {
			dev_err(dev, "Write to RTC ALARM register failed\n");
			goto rtc_rw_fail;
		}
	}

rtc_rw_fail:
	spin_unlock_irqrestore(&rtc_dd->ctrl_reg_lock, irq_flags);
	return rc;
}

#ifdef CONFIG_RTC_AUTO_PWRON
static void
sapa_normalize_alarm(struct rtc_wkalrm *alarm)
{
	if (!alarm->enabled) {
		/* 50 years after RTC reset = 1580518864 = 0x5e34cdd0 */
		alarm->time.tm_year = 70 + 50;
		alarm->time.tm_mon = 1;
		alarm->time.tm_mday = 1;
		alarm->time.tm_hour = 1;
		alarm->time.tm_min = 1;
		alarm->time.tm_sec = 4;
	}
}

#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
static void
sapa_save_kparam(struct pm8xxx_rtc *rtc_dd)
{
	unsigned long secs_pwron;
	unsigned int sapa[3];
	int rc;

	sapa_normalize_alarm(&rtc_dd->sapa);
	rtc_tm_to_time(&rtc_dd->sapa.time, &secs_pwron);
	sapa[0] = SAPA_KPARAM_MAGIC;
	sapa[1] = (unsigned int)rtc_dd->sapa.enabled;
	sapa[2] = (unsigned int)secs_pwron;

	rc = sec_set_param(param_index_sapa, sapa);
	pr_info("sapa: %s rc=%d, enabled=%d, alarm=%u\n",
		__func__, rc, sapa[1], sapa[2]);
}

#endif


static int sapa_is_testalarm(struct rtc_wkalrm *alarm)
{
	unsigned long alm_sec;

	rtc_tm_to_time(&alarm->time, &alm_sec);
	return (alm_sec % 2);
}


static int
sapa_rtc_getalarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	struct pm8xxx_rtc *rtc_dd = dev_get_drvdata(dev);

	alarm->enabled = rtc_dd->triggered;
	return 1;
}

static int
sapa_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	struct pm8xxx_rtc *rtc_dd = dev_get_drvdata(dev);

	memcpy(&rtc_dd->sapa, alarm, sizeof(struct rtc_wkalrm));
#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
	sapa_save_kparam(rtc_dd);
#endif

	return 0;
}

static int
sapa_check_state(struct pm8xxx_rtc *rtc_dd, unsigned long *data)
{
	unsigned long rtc_secs;
	unsigned long secs_pwron;
	u8 value[NUM_8_BIT_RTC_REGS];
	const struct pm8xxx_rtc_regs *regs = rtc_dd->regs;
	int rc;
	int res = SAPA_NEAR;

	rc = regmap_bulk_read(rtc_dd->regmap, regs->read, value, sizeof(value));
	if (rc)
		pr_err("%s: rtc read failed.\n", __func__);
	rtc_secs = TO_SECS(value);

	rtc_tm_to_time(&rtc_dd->sapa.time, &secs_pwron);

	if (rtc_secs < secs_pwron) {
		if (secs_pwron - rtc_secs > SAPA_POLL_TIME)
			res = SAPA_DISTANT;
		if (data)
			*data = secs_pwron - rtc_secs;
	} else if (rtc_secs <= secs_pwron+SAPA_BOOTING_TIME) {
		res = SAPA_EXPIRED;
		if (data)
			*data = rtc_secs + 10;
	} else
		res = SAPA_OVER;

	pr_info("%s: rtc:%lu, alrm:%lu[%d]\n", __func__, rtc_secs, secs_pwron, res);
	return res;
}

static void
sapa_check_func(struct work_struct *work)
{
	struct pm8xxx_rtc *rtc_dd = container_of(work, struct pm8xxx_rtc, check_func);
	int res;
	unsigned long remain;

	res = sapa_check_state(rtc_dd, &remain);
	if (res <=  SAPA_NEAR) {
		ktime_t kt;

		if (res == SAPA_DISTANT)
			remain = SAPA_POLL_TIME;
		kt = ns_to_ktime((u64)remain * NSEC_PER_SEC);
		alarm_start_relative(&rtc_dd->check_poll, kt);
		pr_info("%s: next %lu s\n", __func__, remain);
	} else if (res == SAPA_EXPIRED) {
		wake_lock(&rtc_dd->wakelock);
		rtc_dd->triggered = 1;
	}
}

static enum alarmtimer_restart
sapa_check_callback(struct alarm *alarm, ktime_t now)
{
	struct pm8xxx_rtc *rtc_dd = container_of(alarm, struct pm8xxx_rtc, check_poll);

	schedule_work(&rtc_dd->check_func);
	return ALARMTIMER_NORESTART;
}

static void
sapa_load_alarm(struct pm8xxx_rtc *rtc_dd, u8 ctrl_reg)
{
	unsigned long alarm_secs;
	u8 value[NUM_8_BIT_RTC_REGS];
	const struct pm8xxx_rtc_regs *regs = rtc_dd->regs;
	int rc;

	rc = regmap_bulk_read(rtc_dd->regmap, regs->alarm_ctrl, value, sizeof(value));
	if (rc) {
		pr_err("%s: alarm read failed\n", __func__);
		return;
	}
	alarm_secs = TO_SECS(value);

#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
	pr_info("%s: param=%u\n", __func__, sapa_param_time);
	rtc_time_to_tm(sapa_param_time, &rtc_dd->sapa.time);
	rtc_dd->sapa.enabled = (sapa_param_time) ? 1 : 0;
#else
	rtc_time_to_tm(alarm_secs, &rtc_dd->sapa.time);
	rtc_dd->sapa.enabled = (ctrl_reg & BIT_RTC_ALARM_ENABLE) ? 1 : 0;
#endif

	pr_info("%s: alarm_reg=%02x, pmic=%lu\n", __func__, ctrl_reg, alarm_secs);
}

static void
sapa_init(struct pm8xxx_rtc *rtc_dd)
{
	ktime_t kt;

	rtc_dd->lpm_mode = lpcharge;
	rtc_dd->triggered = 0;
	
	if (rtc_dd->lpm_mode && rtc_dd->sapa.enabled) {
		wake_lock_init(&rtc_dd->wakelock, WAKE_LOCK_SUSPEND, "SAPA");

		alarm_init(&rtc_dd->check_poll, ALARM_REALTIME, sapa_check_callback);
		INIT_WORK(&rtc_dd->check_func, sapa_check_func);

		kt = ns_to_ktime(SAPA_START_POLL_TIME);
		alarm_start_relative(&rtc_dd->check_poll, kt);
	}
}


static void
sapa_exit(struct pm8xxx_rtc *rtc_dd)
{
	struct rtc_wkalrm *alarm;
	int rc;

	pr_info("%s\n", __func__);

	if (rtc_dd->lpm_mode && rtc_dd->sapa.enabled) {
		cancel_work_sync(&rtc_dd->check_func);
		alarm_cancel(&rtc_dd->check_poll);
		wake_lock_destroy(&rtc_dd->wakelock);
	}

	if (!rtc_dd->triggered) {
		if (rtc_dd->sapa.enabled) {
			unsigned long next_power_on;
			int res = sapa_check_state(rtc_dd, &next_power_on);

			if (res == SAPA_EXPIRED && !sapa_is_testalarm(&rtc_dd->sapa)) {
				rtc_time_to_tm(next_power_on, &rtc_dd->sapa.time);
				pr_info("%s: adjust %lu\n", __func__, next_power_on);
			} else if (res >= SAPA_EXPIRED) {
				rtc_dd->sapa.enabled = 0;
				pr_info("%s: over - clear\n", __func__);
			}
		}
	} else {
		rtc_dd->sapa.enabled = 0;
	}

	alarm = &rtc_dd->sapa;
	sapa_normalize_alarm(alarm);
	rc = pm8xxx_rtc_set_alarm(rtc_dd->rtc_dev, alarm);
	if (rc < 0)
		pr_err("%s: err=%d\n", __func__, rc);

	rc = pm8xxx_rtc_read_alarm(rtc_dd->rtc_dev, alarm);
	if (!rc) {
		pr_info("%s: %d-%02d-%02d %02d:%02d:%02d\n", __func__,
			alarm->time.tm_year, alarm->time.tm_mon, alarm->time.tm_mday,
			alarm->time.tm_hour, alarm->time.tm_min, alarm->time.tm_sec);
	}
}
#endif /*CONFIG_RTC_AUTO_PWRON*/

static const struct rtc_class_ops pm8xxx_rtc_ops = {
	.read_time	= pm8xxx_rtc_read_time,
	.set_time	= pm8xxx_rtc_set_time,
	.set_alarm	= pm8xxx_rtc_set_alarm,
	.read_alarm	= pm8xxx_rtc_read_alarm,
#ifdef CONFIG_RTC_AUTO_PWRON
	.read_bootalarm = sapa_rtc_getalarm,
	.set_bootalarm  = sapa_rtc_setalarm,
#endif /*CONFIG_RTC_AUTO_PWRON*/
	.alarm_irq_enable = pm8xxx_rtc_alarm_irq_enable,
};

static irqreturn_t pm8xxx_alarm_trigger(int irq, void *dev_id)
{
	struct pm8xxx_rtc *rtc_dd = dev_id;
	const struct pm8xxx_rtc_regs *regs = rtc_dd->regs;
	unsigned int ctrl_reg;
	int rc;
	unsigned long irq_flags;

	rtc_update_irq(rtc_dd->rtc, 1, RTC_IRQF | RTC_AF);

	spin_lock_irqsave(&rtc_dd->ctrl_reg_lock, irq_flags);

	/* Clear the alarm enable bit */
	rc = regmap_read(rtc_dd->regmap, regs->alarm_ctrl, &ctrl_reg);
	if (rc) {
		spin_unlock_irqrestore(&rtc_dd->ctrl_reg_lock, irq_flags);
		goto rtc_alarm_handled;
	}

	ctrl_reg &= ~regs->alarm_en;

	rc = regmap_write(rtc_dd->regmap, regs->alarm_ctrl, ctrl_reg);
	if (rc) {
		spin_unlock_irqrestore(&rtc_dd->ctrl_reg_lock, irq_flags);
		dev_err(rtc_dd->rtc_dev,
			"Write to alarm control register failed\n");
		goto rtc_alarm_handled;
	}

	spin_unlock_irqrestore(&rtc_dd->ctrl_reg_lock, irq_flags);

	/* Clear RTC alarm register */
	rc = regmap_read(rtc_dd->regmap, regs->alarm_ctrl2, &ctrl_reg);
	if (rc) {
		dev_err(rtc_dd->rtc_dev,
			"RTC Alarm control2 register read failed\n");
		goto rtc_alarm_handled;
	}

	ctrl_reg |= PM8xxx_RTC_ALARM_CLEAR;
	rc = regmap_write(rtc_dd->regmap, regs->alarm_ctrl2, ctrl_reg);
	if (rc)
		dev_err(rtc_dd->rtc_dev,
			"Write to RTC Alarm control2 register failed\n");

rtc_alarm_handled:
	return IRQ_HANDLED;
}

static int pm8xxx_rtc_enable(struct pm8xxx_rtc *rtc_dd)
{
	const struct pm8xxx_rtc_regs *regs = rtc_dd->regs;
	unsigned int ctrl_reg;
	int rc;

	/* Check if the RTC is on, else turn it on */
	rc = regmap_read(rtc_dd->regmap, regs->ctrl, &ctrl_reg);
	if (rc)
		return rc;

	if (!(ctrl_reg & PM8xxx_RTC_ENABLE)) {
		ctrl_reg |= PM8xxx_RTC_ENABLE;
		rc = regmap_write(rtc_dd->regmap, regs->ctrl, ctrl_reg);
		if (rc)
			return rc;
	}

	return 0;
}

static const struct pm8xxx_rtc_regs pm8921_regs = {
	.ctrl		= 0x11d,
	.write		= 0x11f,
	.read		= 0x123,
	.alarm_rw	= 0x127,
	.alarm_ctrl	= 0x11d,
	.alarm_ctrl2	= 0x11e,
	.alarm_en	= BIT(1),
};

static const struct pm8xxx_rtc_regs pm8058_regs = {
	.ctrl		= 0x1e8,
	.write		= 0x1ea,
	.read		= 0x1ee,
	.alarm_rw	= 0x1f2,
	.alarm_ctrl	= 0x1e8,
	.alarm_ctrl2	= 0x1e9,
	.alarm_en	= BIT(1),
};

static const struct pm8xxx_rtc_regs pm8941_regs = {
	.ctrl		= 0x6046,
	.write		= 0x6040,
	.read		= 0x6048,
	.alarm_rw	= 0x6140,
	.alarm_ctrl	= 0x6146,
	.alarm_ctrl2	= 0x6148,
	.alarm_en	= BIT(7),
};

static const struct pm8xxx_rtc_regs pmk8350_regs = {
	.ctrl		= 0x6146,
	.write		= 0x6140,
	.read		= 0x6148,
	.alarm_rw	= 0x6240,
	.alarm_ctrl	= 0x6246,
	.alarm_ctrl2	= 0x6248,
	.alarm_en	= BIT(7),
};

/*
 * Hardcoded RTC bases until IORESOURCE_REG mapping is figured out
 */
static const struct of_device_id pm8xxx_id_table[] = {
	{ .compatible = "qcom,pm8921-rtc", .data = &pm8921_regs },
	{ .compatible = "qcom,pm8018-rtc", .data = &pm8921_regs },
	{ .compatible = "qcom,pm8058-rtc", .data = &pm8058_regs },
	{ .compatible = "qcom,pm8941-rtc", .data = &pm8941_regs },
	{ .compatible = "qcom,pmk8350-rtc", .data = &pmk8350_regs },
	{ },
};
MODULE_DEVICE_TABLE(of, pm8xxx_id_table);

static int pm8xxx_rtc_probe(struct platform_device *pdev)
{
	int rc;
	struct pm8xxx_rtc *rtc_dd;
	const struct of_device_id *match;

	match = of_match_node(pm8xxx_id_table, pdev->dev.of_node);
	if (!match)
		return -ENXIO;

	rtc_dd = devm_kzalloc(&pdev->dev, sizeof(*rtc_dd), GFP_KERNEL);
	if (rtc_dd == NULL)
		return -ENOMEM;

	/* Initialise spinlock to protect RTC control register */
	spin_lock_init(&rtc_dd->ctrl_reg_lock);

	rtc_dd->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!rtc_dd->regmap) {
		dev_err(&pdev->dev, "Parent regmap unavailable.\n");
		return -ENXIO;
	}

	rtc_dd->rtc_alarm_irq = platform_get_irq(pdev, 0);
	if (rtc_dd->rtc_alarm_irq < 0) {
		dev_err(&pdev->dev, "Alarm IRQ resource absent!\n");
		return -ENXIO;
	}

	rtc_dd->allow_set_time = of_property_read_bool(pdev->dev.of_node,
						      "allow-set-time");

	rtc_dd->regs = match->data;
	rtc_dd->rtc_dev = &pdev->dev;

	rc = pm8xxx_rtc_enable(rtc_dd);
	if (rc)
		return rc;

	platform_set_drvdata(pdev, rtc_dd);

	device_init_wakeup(&pdev->dev, 1);

#ifdef CONFIG_RTC_AUTO_PWRON
	sapa_load_alarm(rtc_dd, rtc_dd->regs->alarm_ctrl);
#endif
	
	/* Register the RTC device */
	rtc_dd->rtc = devm_rtc_device_register(&pdev->dev, "pm8xxx_rtc",
					       &pm8xxx_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc_dd->rtc)) {
		dev_err(&pdev->dev, "%s: RTC registration failed (%ld)\n",
			__func__, PTR_ERR(rtc_dd->rtc));
		return PTR_ERR(rtc_dd->rtc);
	}

	/* Request the alarm IRQ */
	rc = devm_request_any_context_irq(&pdev->dev, rtc_dd->rtc_alarm_irq,
					  pm8xxx_alarm_trigger,
					  IRQF_TRIGGER_RISING,
					  "pm8xxx_rtc_alarm", rtc_dd);
	if (rc < 0) {
		dev_err(&pdev->dev, "Request IRQ failed (%d)\n", rc);
		return rc;
	}

#ifdef CONFIG_RTC_AUTO_PWRON
	sapa_init(rtc_dd);
#endif
	
	dev_dbg(&pdev->dev, "Probe success !!\n");

	return 0;
}

#ifdef CONFIG_RTC_AUTO_PWRON
static void pm8xxx_rtc_shutdown(struct platform_device *pdev)
{
	struct pm8xxx_rtc *rtc_dd;

	if (!pdev) {
		pr_err("%s: spmi device not found\n", __func__);
		return;
	}

	rtc_dd = dev_get_drvdata(&pdev->dev);

	if (!rtc_dd) {
		pr_err("%s: rtc driver data not found\n", __func__);
		return;
	}
	
	sapa_exit(rtc_dd);
}
#endif

#ifdef CONFIG_PM_SLEEP
static int pm8xxx_rtc_resume(struct device *dev)
{
	struct pm8xxx_rtc *rtc_dd = dev_get_drvdata(dev);

	if (device_may_wakeup(dev))
		disable_irq_wake(rtc_dd->rtc_alarm_irq);

	return 0;
}

static int pm8xxx_rtc_suspend(struct device *dev)
{
	struct pm8xxx_rtc *rtc_dd = dev_get_drvdata(dev);

	if (device_may_wakeup(dev))
		enable_irq_wake(rtc_dd->rtc_alarm_irq);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(pm8xxx_rtc_pm_ops,
			 pm8xxx_rtc_suspend,
			 pm8xxx_rtc_resume);

static struct platform_driver pm8xxx_rtc_driver = {
	.probe		= pm8xxx_rtc_probe,
#ifdef CONFIG_RTC_AUTO_PWRON
	.shutdown	= pm8xxx_rtc_shutdown,
#endif
	.driver	= {
		.name		= "rtc-pm8xxx",
		.pm		= &pm8xxx_rtc_pm_ops,
		.of_match_table	= pm8xxx_id_table,
	},
};

module_platform_driver(pm8xxx_rtc_driver);

MODULE_ALIAS("platform:rtc-pm8xxx");
MODULE_DESCRIPTION("PMIC8xxx RTC driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Anirudh Ghayal <aghayal@codeaurora.org>");
