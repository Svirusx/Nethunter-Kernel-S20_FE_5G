/* drivers/muic/muic-core.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/platform_data/i2c-gpio.h>
#include <linux/gpio.h>
//#include <mach/irqs.h>

/* switch device header */
#ifdef CONFIG_SWITCH
#include <linux/switch.h>
#endif /* CONFIG_SWITCH */

#if defined(CONFIG_USE_SAFEOUT)
#include <linux/regulator/consumer.h>
#endif

#if defined(CONFIG_USB_HW_PARAM)
#include <linux/usb_notify.h>
#endif

#if defined(CONFIG_MFD_S2MU005)
#include <linux/mfd/samsung/s2mu005.h>
#include <linux/mfd/samsung/s2mu005-private.h>
#endif

#if defined(CONFIG_MFD_MAX77804) || defined(CONFIG_MFD_MAX77804K)
#include <linux/mfd/max77804.h>
#include <linux/mfd/max77804-private.h>
#elif defined(CONFIG_MFD_MAX77828)
#include <linux/mfd/max77828.h>
#include <linux/mfd/max77828-private.h>
#elif defined(CONFIG_MFD_MAX77843)
#include <linux/mfd/max77843.h>
#include <linux/mfd/max77843-private.h>
#elif defined(CONFIG_MFD_MAX77833)
#include <linux/mfd/max77833.h>
#include <linux/mfd/max77833-private.h>
#elif defined(CONFIG_MFD_MAX77888)
#include <linux/mfd/max77888.h>
#include <linux/mfd/max77888-private.h>
#elif defined(CONFIG_MFD_MAX77854)
#include <linux/mfd/max77854.h>
#include <linux/mfd/max77854-private.h>
#endif

#include <linux/muic/muic.h>

#if defined(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/muic_notifier.h>
#endif /* CONFIG_MUIC_NOTIFIER */

#if defined(CONFIG_MUIC_SUPPORT_CCIC) && defined(CONFIG_CCIC_NOTIFIER)
#include <linux/ccic/ccic_notifier.h>
#endif

#ifdef CONFIG_SWITCH
static struct switch_dev switch_dock = {
	.name = "dock",
};

#ifdef CONFIG_UART3
static struct switch_dev switch_uart3 = {
	.name = "uart3",	/* /sys/class/switch/uart3/state */
};
#endif

#if defined(CONFIG_SEC_FACTORY)
struct switch_dev switch_attached_muic_cable = {
	.name = "attached_muic_cable",	/* sys/class/switch/attached_muic_cable/state */
};
#endif

struct switch_dev switch_uart3 = {
	.name = "uart3",	/* sys/class/switch/uart3/state */
};
EXPORT_SYMBOL(switch_uart3);
#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
static struct switch_dev switch_earjack = {
	.name = "h2w",          /* /sys/class/switch/h2w/state */
};

static struct switch_dev switch_earjackkey = {
	.name = "send_end",     /* /sys/class/switch/send_end/state */
};
#endif
#endif /* CONFIG_SWITCH */

int muic_wakeup_noti = 1;

#if defined(CONFIG_MUIC_NOTIFIER)
static struct notifier_block dock_notifier_block;
static struct notifier_block cable_data_notifier_block;

extern struct muic_platform_data muic_pdata;

static void muic_jig_uart_cb(int jig_state)
{
	pr_info("%s: MUIC uart type(%d)\n", __func__, jig_state);
#ifdef CONFIG_SWITCH
	switch_set_state(&switch_uart3, jig_state);
#endif
}

void muic_send_dock_intent(int type)
{
	pr_info("%s: MUIC dock type(%d)\n", __func__, type);
#ifdef CONFIG_SWITCH
	switch_set_state(&switch_dock, type);
#endif
}

#if defined(CONFIG_SEC_FACTORY)
void muic_send_attached_muic_cable_intent(int type)
{
	pr_info("%s: MUIC attached_muic_cable type(%d)\n", __func__, type);
#ifdef CONFIG_SWITCH
	switch_set_state(&switch_attached_muic_cable, type);
#endif
}
#endif

#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
static int muic_earjack_intent(int state)
{
	pr_info("%s: MUIC earjack(%d)\n", __func__, state);
#ifdef CONFIG_SWITCH
	switch_set_state(&switch_earjack, state);
#endif
	return NOTIFY_OK;
}
static int muic_earjackkey_intent(int state)
{
	pr_info("%s: MUIC earjackkey(%d)\n", __func__, state);
#ifdef CONFIG_SWITCH
	switch_set_state(&switch_earjackkey, state);
#endif
	return NOTIFY_OK;
}
#endif
static int muic_dock_attach_notify(int type, const char *name)
{
	pr_info("%s: %s\n", __func__, name);
	muic_send_dock_intent(type);

	return NOTIFY_OK;
}

static int muic_dock_detach_notify(void)
{
	pr_info("%s\n", __func__);
	muic_send_dock_intent(MUIC_DOCK_DETACHED);

	return NOTIFY_OK;
}

static int muic_handle_dock_notification(struct notifier_block *nb,
			unsigned long action, void *data)
{
#if defined(CONFIG_CCIC_NOTIFIER) && defined(CONFIG_MUIC_SUPPORT_CCIC)
	CC_NOTI_ATTACH_TYPEDEF *pnoti = (CC_NOTI_ATTACH_TYPEDEF *)data;
	muic_attached_dev_t attached_dev = pnoti->cable_type;
#else
	muic_attached_dev_t attached_dev = *(muic_attached_dev_t *)data;
#endif
	int type = MUIC_DOCK_DETACHED;
	const char *name;

	switch (attached_dev) {
	case ATTACHED_DEV_DESKDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_VB_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH) {
			type = MUIC_DOCK_DESKDOCK;
			name = "Desk Dock Attach";
			return muic_dock_attach_notify(type, name);
		}
		else if (action == MUIC_NOTIFY_CMD_DETACH)
			return muic_dock_detach_notify();
#if defined(CONFIG_SEC_FACTORY)
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
#endif
		break;
	case ATTACHED_DEV_CARDOCK_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH) {
			type = MUIC_DOCK_CARDOCK;
			name = "Car Dock Attach";
			return muic_dock_attach_notify(type, name);
		}
		else if (action == MUIC_NOTIFY_CMD_DETACH)
			return muic_dock_detach_notify();
		break;
#ifdef CONFIG_UART3	//XO Shutdown
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_VB_MUIC:
		/* write value at "sys/class/switch/uart3/state" */
		if (action == MUIC_NOTIFY_CMD_ATTACH || action == MUIC_NOTIFY_CMD_DETACH) {
			switch_set_state(&switch_uart3, action);
			printk(KERN_DEBUG "[muic] %s: set %ld to sys/class/switch/uart3/state\n", __func__, action);
			return NOTIFY_OK;
		}
		break;
#endif
	case ATTACHED_DEV_SMARTDOCK_MUIC:
	case ATTACHED_DEV_SMARTDOCK_VB_MUIC:
	case ATTACHED_DEV_SMARTDOCK_TA_MUIC:
	case ATTACHED_DEV_SMARTDOCK_USB_MUIC:
		if (action == MUIC_NOTIFY_CMD_LOGICALLY_ATTACH) {
			type = MUIC_DOCK_SMARTDOCK;
			name = "Smart Dock Attach";
			return muic_dock_attach_notify(type, name);
		}
		else if (action == MUIC_NOTIFY_CMD_LOGICALLY_DETACH)
			return muic_dock_detach_notify();
		break;
	case ATTACHED_DEV_UNIVERSAL_MMDOCK_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH) {
			type = MUIC_DOCK_SMARTDOCK;
			name = "Universal MMDock Attach";
			return muic_dock_attach_notify(type, name);
		}
		else if (action == MUIC_NOTIFY_CMD_DETACH)
			return muic_dock_detach_notify();
		break;
	case ATTACHED_DEV_AUDIODOCK_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH) {
			type = MUIC_DOCK_AUDIODOCK;
			name = "Audio Dock Attach";
			return muic_dock_attach_notify(type, name);
		}
		else if (action == MUIC_NOTIFY_CMD_DETACH)
			return muic_dock_detach_notify();
		break;
	case ATTACHED_DEV_HMT_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH) {
			type = MUIC_DOCK_HMT;
			name = "HMT Attach";
			return muic_dock_attach_notify(type, name);
		}
		else if (action == MUIC_NOTIFY_CMD_DETACH)
			return muic_dock_detach_notify();
		break;
	case ATTACHED_DEV_GAMEPAD_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH) {
			type = MUIC_DOCK_GAMEPAD;
			name = "Gamepad Attach";
			return muic_dock_attach_notify(type, name);
		} else if (action == MUIC_NOTIFY_CMD_DETACH)
			return muic_dock_detach_notify();
		break;
#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
	case ATTACHED_DEV_SEND_MUIC:
	case ATTACHED_DEV_VOLDN_MUIC:
	case ATTACHED_DEV_VOLUP_MUIC:
			return muic_earjackkey_intent(1);
	case ATTACHED_DEV_EARJACK_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH) {
			return muic_earjack_intent(1);
		} else if (action == MUIC_NOTIFY_CMD_DETACH)
			return muic_earjack_intent(0);
		break;
#endif
	default:
		break;
	}

	pr_info("%s: ignore(%d)\n", __func__, attached_dev);
	return NOTIFY_DONE;
}
#endif /* CONFIG_MUIC_NOTIFIER */

static int muic_handle_cable_data_notification(struct notifier_block *nb,
			unsigned long action, void *data)
{
#if defined(CONFIG_CCIC_NOTIFIER) && defined(CONFIG_MUIC_SUPPORT_CCIC)
	CC_NOTI_ATTACH_TYPEDEF *pnoti = (CC_NOTI_ATTACH_TYPEDEF *)data;
	muic_attached_dev_t attached_dev = pnoti->cable_type;
#else
	muic_attached_dev_t attached_dev = *(muic_attached_dev_t *)data;
#endif
	int jig_state = 0;
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif

	switch (attached_dev) {
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:		/* VBUS enabled */
	case ATTACHED_DEV_JIG_UART_OFF_VB_OTG_MUIC:	/* for otg test */
	case ATTACHED_DEV_JIG_UART_OFF_VB_FG_MUIC:	/* for fg test */
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_VB_MUIC:		/* VBUS enabled */
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH)
			jig_state = 1;
		break;
#if defined(CONFIG_USB_HW_PARAM)
	case ATTACHED_DEV_TIMEOUT_OPEN_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH && o_notify)
			inc_hw_param(o_notify, USB_MUIC_DCD_TIMEOUT_COUNT);
		break;
#if defined(CONFIG_MUIC_UNIVERSAL_SM5705_AFC)
	case ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC:
		if (action == MUIC_NOTIFY_CMD_ATTACH && o_notify)
			inc_hw_param(o_notify, USB_MUIC_AFC_ERROR_COUNT);
		break;
#endif
#endif
	default:
		jig_state = 0;
		break;
	}

	pr_info("%s: MUIC uart type(%d)\n", __func__, jig_state);
#ifdef CONFIG_SWITCH
	switch_set_state(&switch_uart3, jig_state);
#endif
	return NOTIFY_DONE;
}

#if defined(CONFIG_USE_SAFEOUT)
int muic_set_safeout(int safeout_path)
{
	struct regulator *regulator;
	int ret;

	pr_info("%s:MUIC safeout path=%d\n", __func__, safeout_path);

	if (safeout_path == MUIC_PATH_USB_CP) {
		regulator = regulator_get(NULL, "safeout1_range");
		if (IS_ERR(regulator) || regulator == NULL)
			return -ENODEV;

		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		regulator_put(regulator);

		regulator = regulator_get(NULL, "safeout2_range");
		if (IS_ERR(regulator) || regulator == NULL)
			return -ENODEV;

		if (!regulator_is_enabled(regulator)) {
			ret = regulator_enable(regulator);
			if (ret)
				goto err;
		}
		regulator_put(regulator);
	} else if (safeout_path == MUIC_PATH_USB_AP) {
		regulator = regulator_get(NULL, "safeout1_range");
		if (IS_ERR(regulator) || regulator == NULL)
			return -ENODEV;

		if (!regulator_is_enabled(regulator)) {
			ret = regulator_enable(regulator);
			if (ret)
				goto err;
		}
		regulator_put(regulator);

		regulator = regulator_get(NULL, "safeout2_range");
		if (IS_ERR(regulator) || regulator == NULL)
			return -ENODEV;

		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		regulator_put(regulator);
	}  else {
		pr_info("%s: not control safeout(%d)\n", __func__, safeout_path);
		return -EINVAL;
	}

	return 0;
err:
	pr_info("%s: cannot regulator_enable (%d)\n", __func__, ret);
	regulator_put(regulator);
	return ret;
}
#endif /* CONFIG_USE_SAFEOUT */

static void muic_init_switch_dev_cb(void)
{
#ifdef CONFIG_SWITCH
	int ret;

	/* for DockObserver */
	ret = switch_dev_register(&switch_dock);
	if (ret < 0) {
		pr_err("%s: Failed to register dock switch(%d)\n",
				__func__, ret);
		return;
	}
#ifdef CONFIG_UART3
	/* for JigUartOnObserver */
	ret = switch_dev_register(&switch_uart3);
	if (ret < 0) {
		printk(KERN_ERR "[muic] %s: Failed to register uart3 switch(%d)\n",
				__func__, ret);
		return;
	}
#endif
#if defined(CONFIG_SEC_FACTORY)
	ret = switch_dev_register(&switch_attached_muic_cable);
	if (ret < 0) {
		pr_err("%s: Failed to register attached_muic_cable switch(%d)\n",
				__func__, ret);
		return;
	}
#endif
	ret = switch_dev_register(&switch_uart3);
	if (ret < 0) {
		pr_err("%s : Failed to register switch_uart3 device\n", __func__);
		goto err_switch_uart3_dev_register;
	}
#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
        ret = switch_dev_register(&switch_earjack);
        if (ret < 0) {
                pr_info("%s : Failed to register switch device\n", __func__);
                goto err_switch_dev_register;
        }

        ret = switch_dev_register(&switch_earjackkey);
        if (ret < 0) {
                pr_info("%s : Failed to register switch device\n", __func__);
                goto err_switch_dev_register;
        }
#endif
#endif /* CONFIG_SWITCH */

#if defined(CONFIG_MUIC_NOTIFIER)
	muic_notifier_register(&dock_notifier_block,
			muic_handle_dock_notification, MUIC_NOTIFY_DEV_DOCK);
	muic_notifier_register(&cable_data_notifier_block,
			muic_handle_cable_data_notification, MUIC_NOTIFY_DEV_CABLE_DATA);
#endif /* CONFIG_MUIC_NOTIFIER */

	pr_info("%s: done\n", __func__);
	return;
#ifdef CONFIG_SWITCH
#if defined(CONFIG_MUIC_SUPPORT_EARJACK)
err_switch_dev_register:
	switch_dev_unregister(&switch_earjack);
	switch_dev_unregister(&switch_earjackkey);
#endif
	switch_dev_unregister(&switch_uart3);
err_switch_uart3_dev_register:
	switch_dev_unregister(&switch_dock);
#endif
}

static void muic_cleanup_switch_dev_cb(void)
{
#if defined(CONFIG_MUIC_NOTIFIER)
	muic_notifier_unregister(&dock_notifier_block);
	muic_notifier_unregister(&cable_data_notifier_block);
#endif /* CONFIG_MUIC_NOTIFIER */

	pr_info("%s: done\n", __func__);
}

/* func : set_switch_sel
 * switch_sel value get from bootloader comand line
 * switch_sel data consist 8 bits (xxxxyyyyzzzz)
 * first 4bits(zzzz) mean path infomation.
 * next 4bits(yyyy) mean if pmic version info
 * next 4bits(xxxx) mean afc disable info
 */
static int set_switch_sel(char *str)
{
	get_option(&str, &muic_pdata.switch_sel);
	muic_pdata.switch_sel = (muic_pdata.switch_sel) & 0xfff;
	pr_info("%s: switch_sel: 0x%03x\n", __func__,
			muic_pdata.switch_sel);

	return muic_pdata.switch_sel;
}
__setup("pmic_info=", set_switch_sel);

int get_switch_sel(void)
{
	return muic_pdata.switch_sel;
}

/* afc_mode:
 *   0x31 : Disabled
 *   0x30 : Enabled
 */
static int afc_mode = 0;
static int __init set_afc_mode(char *str)
{
	get_option(&str, &afc_mode);
	pr_info("%s: afc_mode is 0x%02x\n", __func__, afc_mode);

	return 0;
}
early_param("afc_disable", set_afc_mode);

int get_afc_mode(void)
{
	return afc_mode;
}

static int uart_mode = 0;
static int __init set_uart_mode(char *str)
{
	get_option(&str, &uart_mode);
	pr_info("%s: uart_mode is 0x%02x\n", __func__, uart_mode);

	return 0;
}
early_param("uart_sel", set_uart_mode);

int get_uart_mode(void)
{
	return uart_mode;
}

bool is_muic_usb_path_ap_usb(void)
{
	if (MUIC_PATH_USB_AP == muic_pdata.usb_path) {
		pr_info("%s: [%d]\n", __func__, muic_pdata.usb_path);
		return true;
	}

	return false;
}

bool is_muic_usb_path_cp_usb(void)
{
	if (MUIC_PATH_USB_CP == muic_pdata.usb_path) {
		pr_info("%s: [%d]\n", __func__, muic_pdata.usb_path);
		return true;
	}

	return false;
}

static int muic_init_gpio_cb(int switch_sel)
{
	struct muic_platform_data *pdata = &muic_pdata;
	const char *usb_mode;
	const char *uart_mode;
	int ret = 0;

	pr_info("%s (%d)\n", __func__, switch_sel);

	if (1) {
		pdata->usb_path = MUIC_PATH_USB_AP;
		usb_mode = "PDA";
	} else {
		pdata->usb_path = MUIC_PATH_USB_CP;
		usb_mode = "MODEM";
	}

	if (pdata->set_gpio_usb_sel)
		ret = pdata->set_gpio_usb_sel(pdata->uart_path);

	if (1) {	//if (switch_sel & SWITCH_SEL_UART_MASK) {
		pdata->uart_path = MUIC_PATH_UART_AP;
		uart_mode = "AP";
	} else {
		pdata->uart_path = MUIC_PATH_UART_CP;
		uart_mode = "CP";
	}

	/* These flags MUST be updated again from probe function */
	pdata->rustproof_on = false;

	pdata->afc_disable = false;

	if (pdata->set_gpio_uart_sel)
		ret = pdata->set_gpio_uart_sel(pdata->uart_path);

	pr_info("%s: usb_path(%s), uart_path(%s)\n", __func__,
			usb_mode, uart_mode);

	return ret;
}

int muic_afc_set_voltage(int voltage)
{
	struct muic_platform_data *pdata = &muic_pdata;

	if (pdata && pdata->muic_afc_set_voltage_cb)
		return pdata->muic_afc_set_voltage_cb(voltage);

	pr_err("%s: cannot supported\n", __func__);
	return -ENODEV;
}

int muic_hv_charger_disable(bool en)
{
	struct muic_platform_data *pdata = &muic_pdata;
	int ret = -ENODEV;

	pr_info("%s %sable\n", __func__, en ? "en" : "dis");

	if (pdata && pdata->muic_hv_charger_disable_cb)
		ret = pdata->muic_hv_charger_disable_cb(en);

	return ret;
}

int muic_hv_charger_init(void)
{
	struct muic_platform_data *pdata = &muic_pdata;

	if (pdata && pdata->muic_hv_charger_init_cb)
		return pdata->muic_hv_charger_init_cb();

	pr_err("%s: cannot supported\n", __func__);
	return -ENODEV;
}

int muic_set_hiccup_mode(int on_off)
{
	struct muic_platform_data *pdata = &muic_pdata;

	if (pdata && pdata->muic_set_hiccup_mode_cb)
		return pdata->muic_set_hiccup_mode_cb(on_off);

	pr_err("%s: cannot supported\n", __func__);
	return -ENODEV;
}

struct muic_platform_data muic_pdata = {
	.init_switch_dev_cb	= muic_init_switch_dev_cb,
	.cleanup_switch_dev_cb	= muic_cleanup_switch_dev_cb,
	.init_gpio_cb		= muic_init_gpio_cb,
	.jig_uart_cb		= muic_jig_uart_cb,
#if defined(CONFIG_USE_SAFEOUT)
	.set_safeout		= muic_set_safeout,
#endif /* CONFIG_USE_SAFEOUT */
};
