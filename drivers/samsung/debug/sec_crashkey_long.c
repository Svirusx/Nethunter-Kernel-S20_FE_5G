// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/samsung/debug/sec_crashkey_long.c
 *
 * COPYRIGHT(C) 2019 Samsung Electronics Co., Ltd. All Right Reserved.
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

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/bitmap.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/version.h>

#include <linux/sec_debug.h>

#include "sec_debug_internal.h"
#include "sec_key_notifier.h"

#define EXPIRE_MSEC			6600

static unsigned int crashkey_long_used_event[] = {
	KEY_VOLUMEDOWN,
	KEY_POWER,
};

static size_t crashkey_long_nr_used_event =
	ARRAY_SIZE(crashkey_long_used_event);

struct long_pressed_event_state {
	unsigned long *bitmap_recieved;
	size_t sz_bitmap;
};

static struct long_pressed_event_state __long_key_state;
static struct long_pressed_event_state *long_key_state = &__long_key_state;

static inline void __reset_pon_s2_ctrl_reset(void)
{
	qpnp_control_s2_reset_onoff(0);
	udelay(1000);
	qpnp_control_s2_reset_onoff(1);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
static void sec_crashkey_long_timer_handler(struct timer_list *tl)
#else
static void sec_crashkey_long_timer_handler(unsigned long data)
#endif
{
	__pr_err("*** Force trigger kernel panic before triggering hard reset ***\n");

	__reset_pon_s2_ctrl_reset();
	panic(UPLOAD_MSG_LONG_KEY_PRESS);
}

static struct timer_list sec_crashkey_long_tl;

static int sec_crashkey_long_notifier_call(struct notifier_block *this,
		unsigned long type, void *data)
{
	struct sec_key_notifier_param *param = data;
	size_t i;
	bool matching;

	if (param->down)
		set_bit(param->keycode, long_key_state->bitmap_recieved);
	else
		clear_bit(param->keycode, long_key_state->bitmap_recieved);

	for (i = 0, matching = true; i < crashkey_long_nr_used_event; i++) {
		if (!test_bit(crashkey_long_used_event[i], long_key_state->bitmap_recieved)) {
			matching = false;
			break;
		}
	}

	if (matching) {
		if (sec_debug_get_upload_cause() == UPLOAD_CAUSE_INIT)
			sec_debug_set_upload_cause(UPLOAD_CAUSE_POWER_LONG_PRESS);

		if (unlikely(sec_debug_is_enabled())) {
			if (!timer_pending(&sec_crashkey_long_tl)) {
				timer_setup(&sec_crashkey_long_tl,
						sec_crashkey_long_timer_handler, 0);
				mod_timer(&sec_crashkey_long_tl,
						jiffies + msecs_to_jiffies(EXPIRE_MSEC));
				pr_info("long key timer - start");
			}
		}
	} else {
		if (sec_debug_get_upload_cause() == UPLOAD_CAUSE_POWER_LONG_PRESS)
			sec_debug_set_upload_cause(UPLOAD_CAUSE_INIT);

		if (timer_pending(&sec_crashkey_long_tl)) {
			del_timer(&sec_crashkey_long_tl);
			pr_info("long key timer - cancel");
		}
	}

	return NOTIFY_DONE;
}

static struct notifier_block sec_crashkey_long_notifier = {
	.notifier_call = sec_crashkey_long_notifier_call,
};

/* for retail group's request */
#if defined(CONFIG_SEC_PM)
void do_keyboard_notifier(int onoff)
{
	pr_info("%s: onoff(%d)\n", __func__, onoff);

	if (onoff)
		sec_kn_register_notifier(&sec_crashkey_long_notifier,
			crashkey_long_used_event, crashkey_long_nr_used_event);
	else
		sec_kn_unregister_notifier(&sec_crashkey_long_notifier,
			crashkey_long_used_event, crashkey_long_nr_used_event);
}
EXPORT_SYMBOL(do_keyboard_notifier);
#endif

static void __sec_crashkey_parse_dt_replace_keymap(void)
{
	struct device_node *parent, *node;
	int err;
	size_t i;
	u32 resin_keycode, pwr_keycode;

	parent = of_find_node_by_path("/soc");
	if (!parent)
		goto no_dt;

	node = of_find_node_by_name(parent, "sec_key_crash");
	if (!node)
		goto no_dt;

	err = of_property_read_u32(node, "resin-keycode", &resin_keycode);
	if (err)
		goto no_dt;

	err = of_property_read_u32(node, "pwr-keycode", &pwr_keycode);
	if (err)
		goto no_dt;

	for (i = 0; i < crashkey_long_nr_used_event; i++) {
		unsigned int *keycode = &crashkey_long_used_event[i];

		switch (*keycode) {
		case KEY_VOLUMEDOWN:
			if (resin_keycode != KEY_VOLUMEDOWN)
				*keycode = resin_keycode;
			break;
		case KEY_POWER:
			if (pwr_keycode != KEY_POWER)
				*keycode = pwr_keycode;
			break;
		}
	}
	pr_info("use dt keymap");
	return;

no_dt:
	pr_info("use default keymap");
}

static int sec_crashkey_long_init(void)
{
	unsigned long *bitmap_recieved;

	bitmap_recieved = bitmap_zalloc(KEY_MAX, GFP_KERNEL);
	if (!bitmap_recieved) {
		kfree(bitmap_recieved);
		return -ENOMEM;
	}

	__sec_crashkey_parse_dt_replace_keymap();

	long_key_state->bitmap_recieved = bitmap_recieved;
	long_key_state->sz_bitmap =
		BITS_TO_LONGS(KEY_MAX) * sizeof(unsigned long);

	sec_kn_register_notifier(&sec_crashkey_long_notifier,
			crashkey_long_used_event, crashkey_long_nr_used_event);

	return 0;
}
arch_initcall_sync(sec_crashkey_long_init);
