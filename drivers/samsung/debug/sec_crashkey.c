// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/samsung/debug/sec_crashkey.c
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

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/ratelimit.h>
#include <linux/slab.h>
#include <linux/suspend.h>

#include <linux/sec_debug.h>

#include "sec_debug_internal.h"
#include "sec_key_notifier.h"

#define EVENT_KEY_PRESS(__keycode)					\
	{ .keycode = __keycode, .down = true, }
#define EVENT_KEY_RELEASE(__keycode)					\
	{ .keycode = __keycode, .down = false, }
#define EVENT_KEY_PRESS_AND_RELEASE(__keycode)				\
	EVENT_KEY_PRESS(__keycode),					\
	EVENT_KEY_RELEASE(__keycode)

struct event_pattern {
	unsigned int keycode;
	bool down;
};

struct event_state {
	struct ratelimit_state rs;
	struct event_pattern *desired_pattern;
	struct event_pattern *received_pattern;
	size_t nr_pattern;
	const char *msg;
	int interval;
	size_t sequence;
};

static struct event_pattern crashkey_pattern[] = {
	EVENT_KEY_PRESS(KEY_VOLUMEDOWN),
	EVENT_KEY_PRESS_AND_RELEASE(KEY_POWER),
	EVENT_KEY_PRESS_AND_RELEASE(KEY_POWER),
};

static struct event_pattern received_pattern[ARRAY_SIZE(crashkey_pattern)];

static struct event_state crashkey_state = {
	.desired_pattern = crashkey_pattern,
	.received_pattern = received_pattern,
	.nr_pattern = ARRAY_SIZE(crashkey_pattern),
	.msg = UPLOAD_MSG_CRASH_KEY,
	.interval = 1 * HZ,
};

static struct event_pattern crashkey_user_pattern[] = {
	EVENT_KEY_PRESS(KEY_VOLUMEDOWN),		/* initial trigger */
	EVENT_KEY_PRESS_AND_RELEASE(KEY_POWER),		/* 1 */
	EVENT_KEY_PRESS_AND_RELEASE(KEY_POWER),		/* 2 */
	EVENT_KEY_PRESS_AND_RELEASE(KEY_POWER),		/* 3 */
	EVENT_KEY_PRESS_AND_RELEASE(KEY_POWER),		/* 4 */
	EVENT_KEY_PRESS_AND_RELEASE(KEY_POWER),		/* 5 */
	EVENT_KEY_PRESS_AND_RELEASE(KEY_POWER),		/* 6 */
	EVENT_KEY_PRESS_AND_RELEASE(KEY_POWER),		/* 7 */
	EVENT_KEY_PRESS_AND_RELEASE(KEY_POWER),		/* 8 */
	EVENT_KEY_PRESS_AND_RELEASE(KEY_POWER),		/* 9 */
	EVENT_KEY_PRESS_AND_RELEASE(KEY_VOLUMEUP),	/* 1 */
	EVENT_KEY_PRESS_AND_RELEASE(KEY_VOLUMEUP),	/* 2 */
	EVENT_KEY_PRESS_AND_RELEASE(KEY_VOLUMEUP),	/* 3 */
	EVENT_KEY_PRESS_AND_RELEASE(KEY_VOLUMEUP),	/* 4 */
	EVENT_KEY_PRESS_AND_RELEASE(KEY_VOLUMEUP),	/* 5 */
	EVENT_KEY_PRESS_AND_RELEASE(KEY_POWER),		/* 1 */
	EVENT_KEY_PRESS_AND_RELEASE(KEY_POWER),		/* 2 */
	EVENT_KEY_PRESS_AND_RELEASE(KEY_POWER),		/* 3 */
};

static struct event_pattern received_user_pattern[ARRAY_SIZE(crashkey_user_pattern)];

static struct event_state crashkey_user_state = {
	.desired_pattern = crashkey_user_pattern,
	.received_pattern = received_user_pattern,
	.nr_pattern = ARRAY_SIZE(crashkey_user_pattern),
	.msg = UPLOAD_MSG_USER_CRASH_KEY,
	/* TODO: set this value to '0' to ignore 'ratelimit' */
	.interval = 19 * HZ,	/* COVID-19 */
};

static struct event_state *key_event_state;

static __always_inline int __crashkey_test_pattern(size_t len)
{
	return memcmp(key_event_state->desired_pattern,
		      key_event_state->received_pattern,
		      len * sizeof(struct event_pattern));
}

static __always_inline void __crashkey_clear_received_pattern(void)
{
	key_event_state->sequence = 0;
	memset(key_event_state->received_pattern, 0x0,
	       key_event_state->nr_pattern * sizeof(struct event_pattern));
}

static int sec_crashkey_notifier_call(struct notifier_block *this,
		unsigned long type, void *data)
{
	struct sec_key_notifier_param *param = data;
	size_t idx = key_event_state->sequence;

	if (idx >= key_event_state->nr_pattern)
		goto clear_state;

	key_event_state->received_pattern[idx].keycode = param->keycode;
	key_event_state->received_pattern[idx].down = !!param->down;
	key_event_state->sequence++;

	if (__crashkey_test_pattern(key_event_state->sequence))
		goto clear_state;

	if (!key_event_state->interval ||
	    !__ratelimit(&(key_event_state->rs))) {
		if (key_event_state->sequence == key_event_state->nr_pattern) {
#ifdef CONFIG_SEC_USER_RESET_DEBUG
			sec_debug_store_extc_idx(false);
#endif
			panic("%s", key_event_state->msg);
		} else if (key_event_state->interval)
			goto clear_state;
	}

	return NOTIFY_DONE;

clear_state:
	__crashkey_clear_received_pattern();

	return NOTIFY_DONE;
}

static struct notifier_block sec_crashkey_notifier = {
	.notifier_call = sec_crashkey_notifier_call,
};

static unsigned int *carashkey_used_event;
static size_t crashkey_nr_used_event;

static int __init __crashkey_init_used_event(void)
{
	bool is_new;
	size_t i;
	size_t j;

	carashkey_used_event = kmalloc_array(key_event_state->nr_pattern,
			sizeof(*carashkey_used_event), GFP_KERNEL);
	if (!carashkey_used_event)
		return -ENOMEM;

	carashkey_used_event[0] =
		key_event_state->desired_pattern[0].keycode;
	crashkey_nr_used_event = 1;

	for (i = 1; i < key_event_state->nr_pattern; i++) {
		for (j = 0, is_new = true; j < crashkey_nr_used_event; j++) {
			if (carashkey_used_event[j] ==
			    key_event_state->desired_pattern[i].keycode)
				is_new = false;
		}

		if (is_new)
			carashkey_used_event[crashkey_nr_used_event++] =
					key_event_state->desired_pattern[i].keycode;
	}

	return 0;
}

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

	for (i = 0; i < key_event_state->nr_pattern; i++) {
		struct event_pattern *desired_pattern =
				&key_event_state->desired_pattern[i];

		switch (desired_pattern->keycode) {
		case KEY_VOLUMEDOWN:
			if (resin_keycode != KEY_VOLUMEDOWN)
				desired_pattern->keycode = resin_keycode;
			break;
		case KEY_POWER:
			if (pwr_keycode != KEY_POWER)
				desired_pattern->keycode = pwr_keycode;
			break;
		}
	}
	pr_info("use dt keymap");
	return;

no_dt:
	pr_info("use default keymap");
}

static int sec_crashkey_pm_notifier_call(struct notifier_block *nb,
		unsigned long action, void *data)
{
	switch (action) {
	case PM_SUSPEND_PREPARE:
	case PM_POST_SUSPEND:
		__crashkey_clear_received_pattern();
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block sec_crashkey_pm_notifier = {
	.notifier_call = sec_crashkey_pm_notifier_call,
};

static int __init sec_crashkey_init(void)
{
	int err;

	if (!sec_debug_is_enabled())
		key_event_state = &crashkey_user_state;
	else
		key_event_state = &crashkey_state;

	__sec_crashkey_parse_dt_replace_keymap();

	err = __crashkey_init_used_event();
	if (err) {
		pr_warn("crashkey can not be enabled! (%d)", err);
		return err;
	}

	ratelimit_state_init(&(key_event_state->rs),
			key_event_state->interval,
			key_event_state->nr_pattern - 1);

	sec_kn_register_notifier(&sec_crashkey_notifier,
			carashkey_used_event, crashkey_nr_used_event);

	register_pm_notifier(&sec_crashkey_pm_notifier);

	return 0;
}
arch_initcall_sync(sec_crashkey_init);
