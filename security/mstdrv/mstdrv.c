/*
 * @file mstdrv.c
 * @brief MST drv Support
 * Copyright (c) 2015, Samsung Electronics Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
//#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/qseecom.h>
#include <linux/pinctrl/consumer.h>
#include <linux/err.h>
#include "mstdrv.h"
#include <linux/power_supply.h>
#include <linux/msm_pcie.h>
#include <linux/threads.h>
#include <linux/percpu-defs.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/sched.h>
#include <linux/sched/core_ctl.h>
#include <linux/workqueue.h>
#include <linux/msm-bus.h>
#if defined(CONFIG_MST_LPM_DISABLE)
#include <linux/pm_qos.h>
#endif
#if defined(CONFIG_MST_V2)
#include "../drivers/battery_v2/include/charger/mfc_charger.h"
#endif

/* defines */
#define ON                              1                   // On state
#define OFF                             0                   // Off state
#define TRACK1                          1                   // Track1 data
#define TRACK2                          2                   // Track2 data
#define CMD_MST_LDO_OFF                 '0'                 // MST LDO off
#define CMD_MST_LDO_ON                  '1'                 // MST LDO on
#define CMD_SEND_TRACK1_DATA            '2'                 // Send track1 test data
#define CMD_SEND_TRACK2_DATA            '3'                 // send track2 test data
#define CMD_HW_RELIABILITY_TEST_START   '4'                 // start HW reliability test
#define CMD_HW_RELIABILITY_TEST_STOP    '5'                 // stop HW reliability test
#define ERROR_VALUE                     -1                  // Error value
#define TEST_RESULT_LEN                 256                 // result array length
#define SVC_MST_ID                      0x000A0000          // need to check ID
#define MST_CREATE_CMD(x)               (SVC_MST_ID | x)    // Create MST commands
#define MST_TA                          "mst"


#if defined(CONFIG_MFC_CHARGER)
#define MST_MODE_ON                     1                   // ON Message to MFC ic
#define MST_MODE_OFF                    0                   // OFF Message to MFC ic
static int mfc_get_chip_id(void);
#if defined(CONFIG_MST_V2)
extern int mfc_reg_read(struct i2c_client *client, u16 reg, u8 *val);
extern int mfc_reg_write(struct i2c_client *client, u16 reg, u8 val);
extern int mfc_reg_multi_write(struct i2c_client *client, u16 reg, const u8 * val, int size);
extern int mfc_get_firmware_version(struct mfc_charger_data *charger, int firm_mode);
extern void mfc_set_cmd_l_reg(struct mfc_charger_data *charger, u8 val, u8 mask);
#else
#define psy_do_property(name, function, property, value) \
{    \
        struct power_supply *psy;    \
        int ret;    \
        psy = get_power_supply_by_name((name));    \
        if (!psy) {    \
                pr_err("%s: Fail to "#function" psy (%s)\n",    \
                                __func__, (name));    \
                value.intval = 0;    \
        } else {    \
                if (psy->desc->function##_property != NULL) { \
                        ret = psy->desc->function##_property(psy, (property), &(value)); \
                        if (ret < 0) {    \
                                pr_err("%s: Fail to %s "#function" (%d=>%d)\n", \
                                                __func__, name, (property), ret);    \
                                value.intval = 0;    \
                        }    \
                }    \
        }    \
}
#endif
#endif

/* enum definitions */
typedef enum {
        MST_CMD_TRACK1_TEST = MST_CREATE_CMD(0x00000000),
        MST_CMD_TRACK2_TEST = MST_CREATE_CMD(0x00000001),
        MST_CMD_UNKNOWN = MST_CREATE_CMD(0x7FFFFFFF)
} mst_cmd_type;

/* struct definitions */
struct qseecom_handle {
        void *dev;              /* in/out */
        unsigned char *sbuf;    /* in/out */
        uint32_t sbuf_len;      /* in/out */
};

typedef struct mst_req_s {
        mst_cmd_type cmd_id;
        uint32_t data;
} __attribute__((packed)) mst_req_t;

typedef struct mst_rsp_s {
        uint32_t data;
        uint32_t status;
} __attribute__((packed)) mst_rsp_t;

static struct msm_bus_paths ss_mst_usecases[] = {
        {
                .vectors = (struct msm_bus_vectors[]) {
                        {
                                .src = 1,
                                .dst = 512,
                                .ab = 0,
                                .ib = 0,
                        },
                },
                .num_paths = 1,
        },
        {
                .vectors = (struct msm_bus_vectors[]) {
                        {
                                .src = 1,
                                .dst = 512,
                                .ab = 4068000000,
                                .ib = 4068000000,
                        },
                },
                .num_paths = 1,
        },
};

static struct msm_bus_scale_pdata ss_mst_bus_client_pdata = {
        .usecase = ss_mst_usecases,
        .num_usecases = ARRAY_SIZE(ss_mst_usecases),
        .name = "ss_mst",
};

#if defined(CONFIG_MST_LPM_DISABLE)
static struct pm_qos_request mst_pm_qos_request;
uint32_t mst_lpm_tag;
#endif

uint32_t ss_mst_bus_hdl;

/* extern function declarations */
extern int qseecom_start_app(struct qseecom_handle **handle, char *app_name,
                uint32_t size);
extern int qseecom_shutdown_app(struct qseecom_handle **handle);
extern int qseecom_send_command(struct qseecom_handle *handle, void *send_buf,
                uint32_t sbuf_len, void *resp_buf,
                uint32_t rbuf_len);

/* function declarations */
static ssize_t show_mst_drv(struct device *dev,
                struct device_attribute *attr, char *buf);
static ssize_t store_mst_drv(struct device *dev,
                struct device_attribute *attr, const char *buf,
                size_t count);
static int mst_ldo_device_probe(struct platform_device *pdev);
#if defined(CONFIG_MFC_CHARGER)
#if defined(CONFIG_MST_V2)
#else
static inline struct power_supply *get_power_supply_by_name(char *name)
{
        if (!name)
                return (struct power_supply *)NULL;
        else
                return power_supply_get_by_name(name);
}
#endif
#endif

static void of_mst_hw_onoff(bool on);
static int boost_enable(void);
static int boost_disable(void);

/* global variables */
static int mst_pwr_en;                      // MST_PWR_EN pin
#if defined(CONFIG_MST_SUPPORT_GPIO)
static int mst_support_check;
#endif
static int escape_loop = 1;                 // for HW reliability test
static struct class *mst_drv_class;         // mst_drv class
struct device *mst_drv_dev;                 // mst_drv driver handle
//static struct wake_lock mst_wakelock;       // wake_lock used for HW reliability test
static DEVICE_ATTR(transmit, 0770, show_mst_drv, store_mst_drv);        // define device attribute
static struct qseecom_handle *qhandle;
static int nfc_state;
#if defined(CONFIG_MFC_CHARGER)
static int wpc_det;
#endif

/* cpu freq control variables */
struct workqueue_struct *cluster_freq_ctrl_wq;
struct delayed_work dwork;

static uint32_t mode_set_wait = 100;
static uint32_t idt_i2c_command = 0;

DEFINE_MUTEX(mst_mutex);
DEFINE_MUTEX(transmit_mutex);

/* device driver structures */
static struct of_device_id mst_match_ldo_table[] = {
        {.compatible = "sec-mst", },
        { },
};

static int mst_ldo_device_suspend(struct platform_device *dev, pm_message_t state)
{
        uint8_t is_mst_pwr_on;

        is_mst_pwr_on = gpio_get_value(mst_pwr_en);
        if (is_mst_pwr_on == 1) {
                pr_info("%s: mst power is on, is_mst_pwr_on = %d\n", __func__, is_mst_pwr_on);
                pr_info("%s: mst power off\n", __func__);
                of_mst_hw_onoff(OFF);
        } else {
                pr_info("%s: mst power is off, is_mst_pwr_on = %d\n", __func__, is_mst_pwr_on);
        }

        return 0;
}

static struct platform_driver sec_mst_ldo_driver = {
        .driver = {
                .owner = THIS_MODULE,
                .name = "mstldo",
                .of_match_table = mst_match_ldo_table,
        },
        .probe = mst_ldo_device_probe,
        .suspend = mst_ldo_device_suspend,
};

EXPORT_SYMBOL_GPL(mst_drv_dev);

/**
 * boost_enable - set all CPUs freq upto Max
 * boost_disable - set all CPUs freq back to Normal
 */
static void mst_cluster_freq_ctrl_worker(struct work_struct *work)
{

        uint8_t is_mst_pwr_on;

        is_mst_pwr_on = gpio_get_value(mst_pwr_en);
        if (is_mst_pwr_on == 1) {
                pr_info("%s: mst power is on, is_mst_pwr_on = %d\n", __func__, is_mst_pwr_on);
                pr_info("%s: mst power off\n", __func__);
                of_mst_hw_onoff(OFF);
        } else {
                pr_info("%s: mst power is off, is_mst_pwr_on = %d\n", __func__, is_mst_pwr_on);
        }

        return;
}

static int boost_enable(void)
{
        pr_info("boost_enable: bump up snoc clock request\n");
        if (0 == ss_mst_bus_hdl) {
                ss_mst_bus_hdl =
                        msm_bus_scale_register_client(&ss_mst_bus_client_pdata);
                if (ss_mst_bus_hdl) {
                        if (msm_bus_scale_client_update_request(ss_mst_bus_hdl, 1)) {
                                pr_err("[SS_MST] fail to update request!\n");
                                WARN_ON(1);
                                msm_bus_scale_unregister_client(ss_mst_bus_hdl);
                                ss_mst_bus_hdl = 0;
                        }
                } else {
                        pr_err("[SS_MST] fail to register snoc clock config! ss_mst_bus_hdl = %d\n", ss_mst_bus_hdl);
                }
        }

        return ss_mst_bus_hdl;
}

static int boost_disable(void)
{
        pr_info("boost_disable: bump up snoc clock remove\n");
        if (ss_mst_bus_hdl) {
                if (msm_bus_scale_client_update_request(ss_mst_bus_hdl, 0))
                        WARN_ON(1);

                msm_bus_scale_unregister_client(ss_mst_bus_hdl);
                ss_mst_bus_hdl = 0;
        } else {
                pr_err("[SS_MST] fail to unregister snoc clock config! ss_mst_bus_hdl = %d\n", ss_mst_bus_hdl);
        }

        return ss_mst_bus_hdl;
}

#if defined(CONFIG_MST_V2)
static int mfc_get_property(union power_supply_propval *val) {
        struct power_supply *psy;
        struct mfc_charger_data *charger;
        u8 mst_mode, reg_data;

        psy = get_power_supply_by_name("mfc-charger");
	if (psy == NULL) {
		pr_err("%s cannot get power supply!\n", __func__);
		return -1;
	}

        charger = power_supply_get_drvdata(psy);
        if (charger == NULL) {
                pr_err("%s cannot get charger drvdata!\n", __func__);
                return -1;
        }

        val->intval = mfc_reg_read(charger->client, MFC_MST_MODE_SEL_REG, &mst_mode);
        if (val->intval < 0) {
                pr_info("%s mst mode(0x2) i2c write failed, val->intval = %d\n",
                                __func__, val->intval);
                return -ENODATA;
        }

        val->intval = mfc_reg_read(charger->client, MFC_SYS_OP_MODE_REG, &reg_data);
        if (val->intval < 0) {
                pr_info("%s mst mode change irq(0x4) read failed, val->intval = %d\n",
                                __func__, val->intval);
                return -ENODATA;
        }
        reg_data &= 0x0C; /* use only [3:2]bit of sys_op_mode register for MST */

        pr_info("%s mst mode check: mst_mode = %d, reg_data = %d\n",
                        __func__, mst_mode, reg_data);
        val->intval = 0;
        if (reg_data == 0x4)
                val->intval = mst_mode;

        return 0;
}
#endif

/**
 * mst_ctrl_of_mst_hw_onoff - function to disable/enable MST whenever samsung pay app wants
 * @on: on/off value
 */
extern void mst_ctrl_of_mst_hw_onoff(bool on)
{
#if defined(CONFIG_MFC_CHARGER)
        union power_supply_propval value;        /* power_supply prop */
#endif
        int ret = 0;
#if defined(CONFIG_MST_SUPPORT_GPIO)
        uint8_t is_mst_support = 0;

        is_mst_support = gpio_get_value(mst_support_check);
        if (is_mst_support == 0) {
                printk(KERN_ERR "MST not supported, no need to mst_ctrl control, %d\n", is_mst_support);
                return;
        }
#endif
        printk("[MST] mst-drv : mst_ctrl : mst_power_onoff : %d\n", on);

        if (on) {
                printk("[MST] %s : nfc_status gets back to 0(unlock)\n", __func__);
                nfc_state = 0;
        } else {
                gpio_set_value(mst_pwr_en, 0);
                printk("%s : mst_pwr_en gets the LOW\n", __func__);

                usleep_range(800, 1000);
                printk("%s : msleep(1)\n", __func__);

#if defined(CONFIG_MFC_CHARGER)
                value.intval = MST_MODE_OFF;
                psy_do_property("mfc-charger", set, POWER_SUPPLY_PROP_TECHNOLOGY, value);
                printk("%s : MST_MODE_OFF notify : %d\n", __func__, value.intval);

                value.intval = 0;
                psy_do_property("mfc-charger", set, POWER_SUPPLY_EXT_PROP_WPC_EN_MST, value);
                printk("%s : MFC_IC Disable notify : %d\n", __func__, value.intval);
#endif
                /* Boost Disable */
                printk("%s : boost disable to back to Normal", __func__);
                mutex_lock(&mst_mutex);
                ret = boost_disable();
                if (ret)
                        printk("%s : boost disable is failed, ret = %d\n", __func__, ret);
#if defined(CONFIG_MST_LPM_CONTROL)
                /* PCIe LPM Enable */
                sec_pcie_l1ss_enable(L1SS_MST);
#endif

#if defined(CONFIG_MST_LPM_DISABLE)
                /* CPU LPM Enable */
                if (mst_lpm_tag) {
                        printk("%s : pm_qos remove\n", __func__);
                        pm_qos_remove_request(&mst_pm_qos_request);
                        printk("%s : core online lock disable\n", __func__);
                        core_ctl_set_boost(false);
                        mst_lpm_tag = 0;
                }
#endif

                mutex_unlock(&mst_mutex);

                printk("%s : nfc_status gets 1(lock)\n", __func__);
                nfc_state = 1;
        }
}

/**
 * of_mst_hw_onoff - Enable/Disable MST LDO GPIO pin (or Regulator)
 * @on: on/off value
 */
static void of_mst_hw_onoff(bool on)
{
#if defined(CONFIG_MFC_CHARGER)
        union power_supply_propval value;        /* power_supply prop */
        int retry_cnt = 8;
#endif
        int ret;

        printk("[MST] mst-drv : mst_power_onoff : %d\n", on);

        if (nfc_state == 1) {
                printk("[MST] %s : nfc_state is on!!!\n", __func__);
                return;
        }

        if (on) {
#if defined(CONFIG_MFC_CHARGER)
                printk("%s : MFC_IC Enable notify start\n", __func__);
                value.intval = 1;
                psy_do_property("mfc-charger", set, POWER_SUPPLY_EXT_PROP_WPC_EN_MST, value);
                printk("%s : MFC_IC Enable notified : %d\n", __func__, value.intval);

                printk("%s : MST_MODE_ON notify start\n", __func__);
                value.intval = MST_MODE_ON;
                psy_do_property("mfc-charger", set, POWER_SUPPLY_PROP_TECHNOLOGY, value);
                printk("%s : MST_MODE_ON notified : %d\n", __func__, value.intval);
#endif
                gpio_set_value(mst_pwr_en, 1);
                printk("%s : mst_pwr_en gets the HIGH\n", __func__);

                /* Boost Enable */
                cancel_delayed_work_sync(&dwork);
                printk("%s : boost enable for performacne", __func__);
                mutex_lock(&mst_mutex);
                ret = boost_enable();
                if (!ret)
                        printk("%s : boost enable is failed, ret = %d\n", __func__, ret);

                queue_delayed_work(cluster_freq_ctrl_wq, &dwork, 90 * HZ);

#if defined(CONFIG_MST_LPM_CONTROL)
                /* PCIe LPM Disable */
                sec_pcie_l1ss_disable(L1SS_MST);
#endif

#if defined(CONFIG_MST_LPM_DISABLE)
                /* CPU LPM Disable */
                if (mst_lpm_tag == 0) {
                        printk("%s : pm_qos add\n", __func__);
                        pm_qos_add_request(&mst_pm_qos_request, PM_QOS_CPU_DMA_LATENCY, PM_QOS_DEFAULT_VALUE);
                        pm_qos_update_request(&mst_pm_qos_request, 1);
                        printk("%s : core online lock enable\n", __func__);
                        core_ctl_set_boost(true);
                        mst_lpm_tag = 1;
                }
#endif

                mutex_unlock(&mst_mutex);

                msleep(mode_set_wait);

#if defined(CONFIG_MFC_CHARGER)
                while (--retry_cnt) {
#if defined(CONFIG_MST_V2)
                        mfc_get_property(&value);
#else
                        psy_do_property("mfc-charger", get, POWER_SUPPLY_PROP_TECHNOLOGY, value);
#endif
                        //printk("%s : check mst mode status : %d\n", __func__, value.intval);
                        if (value.intval == 0x02) {
                                printk("%s : mst mode set!!! : %d\n", __func__, value.intval);
                                retry_cnt = 1;
                                break;
                        }
                        usleep_range(3600, 4000);
                }

                if (!retry_cnt) {
                        printk("%s : timeout !!! : %d\n", __func__, value.intval);
                }
#endif

        } else {
                gpio_set_value(mst_pwr_en, 0);
                printk("%s : mst_pwr_en gets the LOW\n", __func__);

                usleep_range(800, 1000);
                printk("%s : msleep(1)\n", __func__);

#if defined(CONFIG_MFC_CHARGER)
                value.intval = MST_MODE_OFF;
                psy_do_property("mfc-charger", set, POWER_SUPPLY_PROP_TECHNOLOGY, value);
                printk("%s : MST_MODE_OFF notify : %d\n", __func__, value.intval);

                value.intval = 0;
                psy_do_property("mfc-charger", set, POWER_SUPPLY_EXT_PROP_WPC_EN_MST, value);
                printk("%s : MFC_IC Disable notify : %d\n", __func__, value.intval);
#endif

                /* Boost Disable */
                printk("%s : boost disable to back to Normal", __func__);
                mutex_lock(&mst_mutex);
                ret = boost_disable();
                if (ret)
                        printk("%s : boost disable is failed, ret = %d\n", __func__, ret);

#if defined(CONFIG_MST_LPM_CONTROL)
                /* PCIe LPM Enable */
                sec_pcie_l1ss_enable(L1SS_MST);
#endif

#if defined(CONFIG_MST_LPM_DISABLE)
                /* CPU LPM Enable */
                if (mst_lpm_tag) {
                        printk("%s : pm_qos remove\n", __func__);
                        pm_qos_remove_request(&mst_pm_qos_request);
                        printk("%s : core online lock disable\n", __func__);
                        core_ctl_set_boost(false);
                        mst_lpm_tag = 0;
                }
#endif
                mutex_unlock(&mst_mutex);
        }
}

/**
 * transmit_mst_data - Transmit test track data
 * @track: 1:track1, 2:track2
 */
static int transmit_mst_data(int track)
{
        int ret = 0;
        int qsee_ret = 0;
        char app_name[MAX_APP_NAME_SIZE];
        mst_req_t *kreq = NULL;
        mst_rsp_t *krsp = NULL;
        int req_len = 0, rsp_len = 0;
        // Core Affinity
        struct cpumask cpumask;
        uint32_t cpu;

        if (!mutex_trylock(&transmit_mutex)) {
            printk("[MST] failed to acquire transmit_mutex!\n");
            return ERROR_VALUE;
        }
        snprintf(app_name, MAX_APP_NAME_SIZE, "%s", MST_TA);
        if (NULL == qhandle) {
                /* start the mst tzapp only when it is not loaded. */
                qsee_ret = qseecom_start_app(&qhandle, app_name, 1024);
        }

        if (NULL == qhandle) {
                /* qhandle is still NULL. It seems we couldn't start mst tzapp. */
                printk("[MST] cannot get tzapp handle from kernel.\n");
                ret = ERROR_VALUE;
                goto exit;        /* leave the function now. */
        }

        kreq = (struct mst_req_s *)qhandle->sbuf;

        switch (track) {
                case TRACK1:
                        kreq->cmd_id = MST_CMD_TRACK1_TEST;
                        break;

                case TRACK2:
                        kreq->cmd_id = MST_CMD_TRACK2_TEST;
                        break;

                default:
                        ret = ERROR_VALUE;
                        goto exit;
                        break;
        }

        req_len = sizeof(mst_req_t);
        krsp = (struct mst_rsp_s *)(qhandle->sbuf + req_len);
        rsp_len = sizeof(mst_rsp_t);

        // Core Affinity
        printk("[MST] sched_setaffinity not to run on core0");
        if (num_online_cpus() < 2) {
            cpumask_setall(&cpumask);
            for_each_cpu(cpu, &cpumask) {
                if (cpu == 0)
                    continue;
                cpu_up(cpu);
                break;
            }
        }
        cpumask_clear(&cpumask);
        cpumask_copy(&cpumask, cpu_online_mask);
        cpumask_clear_cpu(0, &cpumask);
        sched_setaffinity(0, &cpumask);

        printk("[MST] cmd_id = %x, req_len = %d, rsp_len = %d\n", kreq->cmd_id, req_len, rsp_len);

        trace_printk("tracing mark write: MST transmission Start\n");
        qsee_ret = qseecom_send_command(qhandle, kreq, req_len, krsp, rsp_len);
        trace_printk("tracing mark write: MST transmission End\n");

        if (qsee_ret) {
                ret = ERROR_VALUE;
                printk("[MST] failed to send cmd to qseecom; qsee_ret = %d.\n", qsee_ret);
        }

        if (krsp->status) {
                ret = ERROR_VALUE;
                printk("[MST] generate sample track data from TZ -- failed. %d\n", krsp->status);
        }

        printk("[MST] shutting down the tzapp.\n");
        qsee_ret = qseecom_shutdown_app(&qhandle);
        if (qsee_ret) {
                printk("[MST] failed to shut down the tzapp.\n");
        } else {
                qhandle = NULL;
        }
exit:
        mutex_unlock(&transmit_mutex);
        return ret;
}

/**
 * show_mst_drv - device attribute show sysfs operation
 */
static ssize_t show_mst_drv(struct device *dev,
                struct device_attribute *attr, char *buf)
{
        if (!dev)
                return -ENODEV;

        // todo
        if (escape_loop == 0) {
                return sprintf(buf, "%s\n", "activating");
        } else {
                return sprintf(buf, "%s\n", "waiting");
        }
}

/**
 * store_mst_drv - device attribute store sysfs operation
 */
static ssize_t store_mst_drv(struct device *dev,
                struct device_attribute *attr, const char *buf,
                size_t count)
{
        char test_result[TEST_RESULT_LEN] = { 0, };
#if defined(CONFIG_MFC_CHARGER)
        struct device_node *np;
        enum of_gpio_flags irq_gpio_flags;
#endif

        sscanf(buf, "%20s\n", test_result);

#if defined(CONFIG_MFC_CHARGER)
        if (wpc_det < 0) {
                np = of_find_node_by_name(NULL, "battery");
                if (!np) {
                        pr_err("%s np NULL\n", __func__);
                } else {
                        /* wpc_det */
                        wpc_det = of_get_named_gpio_flags(np, "battery,wpc_det", 0, &irq_gpio_flags);
                        if (wpc_det < 0) {
                                dev_err(dev, "%s : can't get wpc_det = %d\n", __FUNCTION__, wpc_det);
                        }
                }
        }

        if (wpc_det && (gpio_get_value(wpc_det) == 1)) {
                printk("[MST] Wireless charging is ongoing, do not proceed MST work\n");
                return count;
        }
#endif

        switch (test_result[0]) {

                case CMD_MST_LDO_OFF:
                        of_mst_hw_onoff(OFF);
                        break;

                case CMD_MST_LDO_ON:
                        of_mst_hw_onoff(ON);
                        break;

                case CMD_SEND_TRACK1_DATA:
#if !defined(CONFIG_MFC_CHARGER)
                        of_mst_hw_onoff(ON);
#endif
                        if (transmit_mst_data(TRACK1))
                                printk("[MST] Send track1 data --> failed\n");
                        else
                                printk("[MST] Send track1 data --> successful\n");
#if !defined(CONFIG_MFC_CHARGER)
                        of_mst_hw_onoff(OFF);
#endif
                        break;

                case CMD_SEND_TRACK2_DATA:
#if !defined(CONFIG_MFC_CHARGER)
                        of_mst_hw_onoff(ON);
#endif
                        if (transmit_mst_data(TRACK2))
                                printk("[MST] Send track2 data --> failed\n");
                        else
                                printk("[MST] Send track2 data --> successful\n");
#if !defined(CONFIG_MFC_CHARGER)
                        of_mst_hw_onoff(OFF);
#endif
                        break;

                case CMD_HW_RELIABILITY_TEST_START:
//                        if (escape_loop) {
//                                wake_lock_init(&mst_wakelock, WAKE_LOCK_SUSPEND, "mst_wakelock");
//                                wake_lock(&mst_wakelock);
//                        }
                        escape_loop = 0;
                        while (1) {
                                if (escape_loop == 1)
                                        break;
#if !defined(CONFIG_MFC_CHARGER)
                                of_mst_hw_onoff(ON);
#endif
                                msleep(10);
                                if (transmit_mst_data(TRACK2)) {
                                        printk("[MST] Send track2 data --> failed\n");
                                        break;
                                }
                                printk("[MST] Send track2 data --> successful\n");
#if !defined(CONFIG_MFC_CHARGER)
                                of_mst_hw_onoff(OFF);
#endif
                                msleep(1000);
                        }
                        break;

                case CMD_HW_RELIABILITY_TEST_STOP:
//                        if (!escape_loop)
//                                wake_lock_destroy(&mst_wakelock);
                        escape_loop = 1;
                        printk("[MST] MST escape_loop value = 1\n");
                        break;

                default:
                        printk(KERN_ERR "[MST] MST invalid value : %s\n", test_result);
                        break;
        }
        return count;
}

/**
 * sec_mst_gpio_init - Initialize GPIO pins used by driver
 * @dev: driver handle
 */
static int sec_mst_gpio_init(struct device *dev)
{
#if defined(CONFIG_MFC_CHARGER)
        union power_supply_propval value;        /* power_supply prop */
        struct device_node *np;
        enum of_gpio_flags irq_gpio_flags;
        int ret = 0;
#endif

        mst_pwr_en =
                of_get_named_gpio(dev->of_node, "sec-mst,mst-pwr-gpio", OFF);

        /* check if gpio pin is inited */
        if (mst_pwr_en < 0) {
                printk("%s : Cannot create the gpio\n", __func__);
                //return 1;
        }

        /* gpio request */
        if (gpio_request(mst_pwr_en, "sec-mst,mst-pwr-gpio")) {
                printk("%s : failed to get pwr gpio : %d\n", __func__, mst_pwr_en);
        }

        /* set gpio direction */
        if (mst_pwr_en > 0) {
                gpio_direction_output(mst_pwr_en, OFF);
                printk("%s : Send Output\n", __func__);
        }

#if defined(CONFIG_MFC_CHARGER)
        /* get wireless chraging check gpio */
        np = of_find_node_by_name(NULL, "battery");
        if (!np) {
                pr_err("%s np NULL\n", __func__);
        } else {
                /* wpc_det */
                wpc_det = of_get_named_gpio_flags(np, "battery,wpc_det", 0, &irq_gpio_flags);
                if (wpc_det < 0) {
                        dev_err(dev, "%s : can't get wpc_det = %d\n", __FUNCTION__, wpc_det);
                }
        }

	if (wpc_det && (gpio_get_value(wpc_det) != 1)) {
		pr_info("%s: Not wireless charging! MFC IC Enable notify start.\n", __func__);
		value.intval = 1;
		psy_do_property("mfc-charger", set, POWER_SUPPLY_EXT_PROP_WPC_EN_MST, value);

		pr_info("%s: Set mst-pwr-en to HIGH\n", __func__);
		gpio_set_value(mst_pwr_en, 1);
	}
	msleep(30);

	ret = mfc_get_chip_id();
	if (ret == -1) {
		pr_err("%s : failed to read sec-mst,mfc-ic-chip-vendor. set wait time to default\n", __func__);
	} else {
		if (ret == MFC_CHIP_ID_LSI) {
			mode_set_wait = 100;
		} else if (ret == MFC_CHIP_ID_IDT) {
			idt_i2c_command = 1; // apply additional i2c command to IDT ic chip
		}
		pr_info("%s : set wait time after mst-pwr-en HIGH to %u, i2c command for IDT %d\n", __func__, mode_set_wait, idt_i2c_command);
	}

	if (wpc_det && (gpio_get_value(wpc_det) != 1)) {
		pr_info("%s: Not wireless charging, set mst-pwr-en to LOW\n", __func__);
		gpio_set_value(mst_pwr_en, 0);

		value.intval = 0;
		psy_do_property("mfc-charger", set, POWER_SUPPLY_EXT_PROP_WPC_EN_MST, value);
		printk("%s : MFC_IC Disable notify : %d\n", __func__, value.intval);
	}
#endif

        return 0;
}

#if defined(CONFIG_MFC_CHARGER)
static int mfc_get_chip_id()
{
	u8 chip_id = 0;
	int retry_cnt = 3;
	int ret = 0;
	struct power_supply *psy = NULL;
	struct mfc_charger_data *charger = NULL;

	while (retry_cnt-- > 0) {
		msleep(30);
		
		psy = get_power_supply_by_name("mfc-charger");
		if (psy == NULL) {
			pr_err("%s cannot get power supply!\n", __func__);
			continue;
		}

		charger = power_supply_get_drvdata(psy);
		if (charger == NULL) {
			pr_err("%s cannot get charger drvdata!\n", __func__);
			continue;
		} else {
			pr_info("%s success get charger drvdata!\n", __func__);
			break;
		}
	}
	if (charger == NULL) {
		pr_info("%s : failed to get MFC IC chip ID !!!\n", __func__);
		return -1;
	}

	ret = mfc_reg_read(charger->client, MFC_CHIP_ID_L_REG, &chip_id);
	if (ret >= 0) {
		if (chip_id == MFC_CHIP_ID_S2MIW04) {
			ret = MFC_CHIP_ID_LSI;
			pr_info("%s: MFC IC chip vendor is LSI(0x%x)\n", __func__, chip_id);
		} else { /* 0x20 */
			ret = MFC_CHIP_ID_IDT;
			pr_info("%s: MFC IC chip vendor is IDT(0x%x)\n", __func__, chip_id);
		}
	} else
		return -1;
	return ret;
}

static ssize_t show_mfc(struct device *dev,
                struct device_attribute *attr, char *buf)
{
        if (!dev)
                return -ENODEV;

        return sprintf(buf, "%s\n", "mfc_charger");
}

static ssize_t store_mfc(struct device *dev,
                struct device_attribute *attr, const char *buf,
                size_t count)
{
        return count;
}

static DEVICE_ATTR(mfc, 0770, show_mfc, store_mfc);

#if defined(CONFIG_MST_V2)
static ssize_t show_mst_switch_test(struct device *dev,
                struct device_attribute *attr, char *buf)
{
        struct power_supply *psy;
        struct mfc_charger_data *charger;
        int ret = 0;

        psy = get_power_supply_by_name("mfc-charger");
	if (psy == NULL) {
		pr_err("%s cannot get power supply!\n", __func__);
		return -1;
	}

        charger = power_supply_get_drvdata(psy);
        if (charger == NULL) {
                pr_err("%s cannot get charger drvdata!\n", __func__);
                return -1;
        }

        if (!dev)
                return -ENODEV;

        /* This is the WA codes that reduces VRECT invalid case. */
        if (gpio_is_valid(charger->pdata->mst_pwr_en)) {
                charger->mst_off_lock = 1;
                gpio_direction_output(charger->pdata->mst_pwr_en, 1);
                usleep_range(3600, 4000);
                gpio_direction_output(charger->pdata->mst_pwr_en, 0);
                msleep(50);
                charger->mst_off_lock = 0;
    
                gpio_direction_output(charger->pdata->mst_pwr_en, 1);
                msleep(charger->pdata->mst_switch_delay);
                ret = mfc_get_firmware_version(charger, MFC_RX_FIRMWARE);
                pr_info("%s: check f/w revision, mst power on (0x%x)\n", __func__, ret);
                gpio_direction_output(charger->pdata->mst_pwr_en, 0);
        } else {
                pr_info("%s: MST_SWITCH_VERIFY, invalid gpio(mst_pwr_en)\n", __func__);
                ret = -1;
        }

        return sprintf(buf, "%x\n", ret);
}

static ssize_t store_mst_switch_test(struct device *dev,
                struct device_attribute *attr, const char *buf,
                size_t count)
{
        return count;
}
static DEVICE_ATTR(mst_switch_test, 0664, show_mst_switch_test, store_mst_switch_test);

int mfc_send_mst_cmd(int cmd, struct mfc_charger_data *charger, u8 irq_src_l, u8 irq_src_h) {
        u8 sBuf[2] = {0, };
        pr_info("%s: (called by %ps)\n", __func__,  __builtin_return_address(0));

        switch(cmd) {
        case MST_MODE_ON:
                /* clear intterupt */
                mfc_reg_write(charger->client, MFC_INT_A_CLEAR_L_REG, irq_src_l); // clear int
                mfc_reg_write(charger->client, MFC_INT_A_CLEAR_H_REG, irq_src_h); // clear int
                mfc_set_cmd_l_reg(charger, 0x20, MFC_CMD_CLEAR_INT_MASK); // command

                if (idt_i2c_command) {
                    pr_info("%s: Execute i2c command for LDO\n", __func__);
                    mfc_reg_write(charger->client, MFC_MST_LDO_CONFIG_1, 0xA5); /* MST LDO config 1 */
                    mfc_reg_write(charger->client, MFC_MST_LDO_CONFIG_2, 0x03); /* MST LDO config 2 */
                    mfc_reg_write(charger->client, MFC_MST_LDO_CONFIG_3, 0x14); /* MST LDO config 3 */
                    mfc_reg_write(charger->client, MFC_MST_LDO_CONFIG_4, 0x0A); /* MST LDO config 4 */
                    mfc_reg_write(charger->client, MFC_MST_LDO_CONFIG_5, 0x02); /* MST LDO config 5 */
                    /* MST LDO config 6 */
                    sBuf[0] = 0xFF;
                    sBuf[1] = 0x01;
                    mfc_reg_multi_write(charger->client, MFC_MST_LDO_CONFIG_6, sBuf, sizeof(sBuf));
                    mfc_reg_write(charger->client, MFC_MST_LDO_TURN_ON, 0x08); /* MST LDO config 7 */
                    mfc_reg_write(charger->client, MFC_MST_LDO_TURN_ON, 0x09); /* MST LDO turn on */
                    msleep(10);
                    mfc_reg_write(charger->client, MFC_MST_MODE_SEL_REG, 0x02); /* set MST mode2 */
                    mfc_reg_write(charger->client, MFC_MST_LDO_CONFIG_8, 0x08); /* MST LDO config 8 */
                    mfc_reg_write(charger->client, MFC_MST_OVER_TEMP_INT, 0x04); /* Enable Over temperature INT */
                } else {
                    mfc_reg_write(charger->client, MFC_MST_MODE_SEL_REG, 0x02); /* set MST mode2 */
                }
                pr_info("%s 2AC Missing ! : MST on REV : %d\n", __func__, charger->pdata->wc_ic_rev);
    
                /* clear intterupt */
                mfc_reg_write(charger->client, MFC_INT_A_CLEAR_L_REG, irq_src_l); // clear int
                mfc_reg_write(charger->client, MFC_INT_A_CLEAR_H_REG, irq_src_h); // clear int
                mfc_set_cmd_l_reg(charger, 0x20, MFC_CMD_CLEAR_INT_MASK); // command
    
                msleep(10);
                break;
        case MST_MODE_OFF:
                if (idt_i2c_command) {
                    mfc_reg_write(charger->client, MFC_MST_MODE_SEL_REG, 0x00); /* Exit MST mode */
                    mfc_reg_write(charger->client, MFC_MST_LDO_CONFIG_1, 0x00); /* MST LDO config 7 */
                }
                pr_info("%s: set MST mode off\n", __func__);
                break;
        default:
                break;
        }

        return 0;
}
EXPORT_SYMBOL(mfc_send_mst_cmd);
#endif
#endif

/* MST support node */
static ssize_t show_support(struct device *dev,
                struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_MST_SUPPORT_GPIO)
        uint8_t is_mst_support = 0;
#endif

        if (!dev)
                return -ENODEV;

#if defined(CONFIG_MST_SUPPORT_GPIO)
        is_mst_support = gpio_get_value(mst_support_check);
        if (is_mst_support == 1) {
                printk(KERN_ERR "MST_DRV]]] This device supports MST, %d\n", is_mst_support);
                return sprintf(buf, "%d\n", 1);
        } else {
                printk(KERN_ERR "MST_DRV]]] This device doesn't supports MST, %d\n", is_mst_support);
                return sprintf(buf, "%d\n", 0);
        }
#else
        printk("%s no support gpio, bug MST_LDO is enabled, supports MST\n", __func__);
        return sprintf(buf, "%d\n", 1);
#endif
}

static ssize_t store_support(struct device *dev,
                struct device_attribute *attr, const char *buf,
                size_t count)
{
        return count;
}
static DEVICE_ATTR(support, 0444, show_support, store_support);

/**
 * mst_ldo_device_probe - Driver probe function
 */
static int mst_ldo_device_probe(struct platform_device *pdev)
{
        int retval = 0;
#if defined(CONFIG_MST_SUPPORT_GPIO)
        struct device *dev = &pdev->dev;
        uint8_t is_mst_support = 0;
#endif

        printk("[MST] %s init start\n", __func__);

#if defined(CONFIG_MST_SUPPORT_GPIO)
        /* MST support/non-support node check gpio */
        mst_support_check = of_get_named_gpio(dev->of_node, "sec-mst,mst-support-gpio", 0);
        printk("[MST] mst_support_check Value : %d\n", mst_support_check);
        if (mst_support_check < 0) {
                printk(KERN_ERR "%s : Cannot create the gpio\n", __func__);
                return -1;
        }
        printk(KERN_ERR "MST_DRV]]] gpio support_check inited\n");

        is_mst_support = gpio_get_value(mst_support_check);
        if (is_mst_support == 1) {
                printk(KERN_ERR "MST_DRV]]] This device supports MST, %d\n", is_mst_support);
        } else {
                printk(KERN_ERR "MST_DRV]]] This device doesn't supports MST, %d\n", is_mst_support);

                mst_drv_class = class_create(THIS_MODULE, "mstldo");
                if (IS_ERR(mst_drv_class)) {
                        retval = PTR_ERR(mst_drv_class);
                        printk(KERN_ERR "[MST] %s: driver initialization failed, retval : %d\n", __FILE__, retval);
                        goto done;
                }

                mst_drv_dev = device_create(mst_drv_class,
                                NULL /* parent */ , 0 /* dev_t */ ,
                                NULL /* drvdata */ ,
                                MST_DRV_DEV);
                if (IS_ERR(mst_drv_dev)) {
                        retval = PTR_ERR(mst_drv_dev);
                        kfree(mst_drv_dev);
                        device_destroy(mst_drv_class, 0);
                        printk(KERN_ERR "[MST] %s: driver initialization failed, retval : %d\n", __FILE__, retval);
                        goto done;
                }

                retval = device_create_file(mst_drv_dev, &dev_attr_support);
                if (retval) {
                        kfree(mst_drv_dev);
                        device_destroy(mst_drv_class, 0);
                        printk(KERN_ERR "[MST] %s: (support)driver initialization failed, retval : %d\n", __FILE__, retval);
                        goto done;
                }

                return -1;
        }
#endif

        if (sec_mst_gpio_init(&pdev->dev)) {
                retval = ERROR_VALUE;
                printk(KERN_ERR "[MST] %s: driver initialization failed, retval : %d\n", __FILE__, retval);
                goto done;
        }

        mst_drv_class = class_create(THIS_MODULE, "mstldo");
        if (IS_ERR(mst_drv_class)) {
                retval = PTR_ERR(mst_drv_class);
                printk(KERN_ERR "[MST] %s: driver initialization failed, retval : %d\n", __FILE__, retval);
                goto done;
        }

        mst_drv_dev = device_create(mst_drv_class,
                        NULL /* parent */ , 0 /* dev_t */ ,
                        NULL /* drvdata */ ,
                        MST_DRV_DEV);
        if (IS_ERR(mst_drv_dev)) {
                retval = PTR_ERR(mst_drv_dev);
                kfree(mst_drv_dev);
                device_destroy(mst_drv_class, 0);
                printk(KERN_ERR "[MST] %s: driver initialization failed, retval : %d\n", __FILE__, retval);
                goto done;
        }

        /* register this mst device with the driver core */
        retval = device_create_file(mst_drv_dev, &dev_attr_transmit);
        if (retval) {
                kfree(mst_drv_dev);
                device_destroy(mst_drv_class, 0);
                printk(KERN_ERR "[MST] %s: (transmit)driver initialization failed, retval : %d\n", __FILE__, retval);
                goto done;
        }

        retval = device_create_file(mst_drv_dev, &dev_attr_support);
        if (retval) {
                kfree(mst_drv_dev);
                device_destroy(mst_drv_class, 0);
                printk(KERN_ERR "[MST] %s: (support)driver initialization failed, retval : %d\n", __FILE__, retval);
                goto done;
        }

#if defined(CONFIG_MFC_CHARGER)
        retval = device_create_file(mst_drv_dev, &dev_attr_mfc);
        if (retval) {
                kfree(mst_drv_dev);
                device_destroy(mst_drv_class, 0);
                printk(KERN_ERR "[MST] %s: (mfc)driver initialization failed, retval : %d\n", __FILE__, retval);
                goto done;
        }

#if defined(CONFIG_MST_V2)
        retval = device_create_file(mst_drv_dev, &dev_attr_mst_switch_test);
        if (retval) {
                kfree(mst_drv_dev);
                device_destroy(mst_drv_class, 0);
                printk(KERN_ERR "[MST] %s: (mst_switch_test)driver initialization failed, retval : %d\n", __FILE__, retval);
                goto done;
        }
#endif
#endif

        printk(KERN_DEBUG "[MST] MST drv driver (%s) is initialized.\n", MST_DRV_DEV);

done:
        return retval;
}

/**
 * mst_drv_init - Driver init function
 */
static int __init mst_drv_init(void)
{
        int ret = 0;
        printk(KERN_ERR "[MST] %s\n", __func__);
        ret = platform_driver_register(&sec_mst_ldo_driver);
        printk(KERN_ERR "[MST] MST_LDO_DRV]]] init , ret val : %d\n", ret);

        cluster_freq_ctrl_wq =
                create_singlethread_workqueue("mst_cluster_freq_ctrl_wq");
        INIT_DELAYED_WORK(&dwork, mst_cluster_freq_ctrl_worker);

        return ret;
}

/**
 * mst_drv_exit - Driver exit function
 */
static void __exit mst_drv_exit(void)
{
        class_destroy(mst_drv_class);
        printk(KERN_ALERT "[MST] %s\n", __func__);
}

MODULE_AUTHOR("JASON KANG, j_seok.kang@samsung.com");
MODULE_DESCRIPTION("MST drv driver");
MODULE_VERSION("0.1");

late_initcall(mst_drv_init);
module_exit(mst_drv_exit);
