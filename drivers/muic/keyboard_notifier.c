/* drivers/muic/keyboard_notifier.c 
  * 
  * Copyright (c) 2016 Samsung Electronics Co., Ltd. 
  *        http://www.samsung.com 
  * 
  * This program is free software; you can redistribute it and/or modify 
  * it under the terms of the GNU General Public License version 2 as 
  * published by the Free Software Foundation. 
  */ 

#include <linux/device.h>

#include <linux/notifier.h>
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#include <linux/sec_class.h>

#define NOTI_ADDR_SRC (1 << 1)
#define NOTI_ADDR_DST (0xf)

/* ATTACH Noti. ID */
#define NOTI_ID_ATTACH (1)


#define SET_KEYBOARD_NOTIFIER_BLOCK(nb, fn, dev) do {	\
		(nb)->notifier_call = (fn);		\
		(nb)->priority = (dev);			\
	} while (0)

#define DESTROY_KEYBOARD_NOTIFIER_BLOCK(nb)			\
		SET_KEYBOARD_NOTIFIER_BLOCK(nb, NULL, -1)

static struct keyboard_notifier_struct keyboard_notifier;

static void __set_noti_cxt(int attach)
{
	keyboard_notifier.cmd = attach;
}

int keyboard_notifier_register(struct notifier_block *nb, notifier_fn_t notifier,
			keyboard_notifier_device_t listener)
{
	int ret = 0;

	pr_info("%s: listener=%d register\n", __func__, listener);

	SET_KEYBOARD_NOTIFIER_BLOCK(nb, notifier, listener);
	ret = blocking_notifier_chain_register(&(keyboard_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_register error(%d)\n",
				__func__, ret);

	/* current muic's attached_device status notify */
	nb->notifier_call(nb, keyboard_notifier.cmd, 0);

	return ret;
}

int keyboard_notifier_unregister(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("%s: listener=%d unregister\n", __func__, nb->priority);

	ret = blocking_notifier_chain_unregister(&(keyboard_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_unregister error(%d)\n",
				__func__, ret);
	DESTROY_KEYBOARD_NOTIFIER_BLOCK(nb);

	return ret;
}

static int keyboard_notifier_notify(void)
{
	int ret = 0;

	pr_info("%s: CMD=%d\n", __func__, keyboard_notifier.cmd);

	ret = blocking_notifier_call_chain(&(keyboard_notifier.notifier_call_chain),
			keyboard_notifier.cmd, 0);

	switch (ret) {
	case NOTIFY_STOP_MASK:
	case NOTIFY_BAD:
		pr_err("%s: notify error occur(0x%x)\n", __func__, ret);
		break;
	case NOTIFY_DONE:
	case NOTIFY_OK:
		pr_info("%s: notify done(0x%x)\n", __func__, ret);
		break;
	default:
		pr_info("%s: notify status unknown(0x%x)\n", __func__, ret);
		break;
	}

	return ret;
}

void keyboard_notifier_attach(void)
{
	pr_info("%s\n", __func__);

	__set_noti_cxt(MUIC_NOTIFY_CMD_ATTACH);

	keyboard_notifier_notify();
}

void keyboard_notifier_detach(void)
{
	pr_info("%s\n", __func__);

	__set_noti_cxt(MUIC_NOTIFY_CMD_DETACH);

	keyboard_notifier_notify();
}

static int __init keyboard_notifier_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	BLOCKING_INIT_NOTIFIER_HEAD(&(keyboard_notifier.notifier_call_chain));
	__set_noti_cxt(0);

	return ret;
}
device_initcall(keyboard_notifier_init);


