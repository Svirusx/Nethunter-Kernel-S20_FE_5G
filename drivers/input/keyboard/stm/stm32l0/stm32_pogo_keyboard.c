/******************** (C) COPYRIGHT 2019 Samsung Electronics ********************
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
 * PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
 * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
 * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
 * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
 *******************************************************************************/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif
#include <linux/input/matrix_keypad.h>
#include <linux/input/pogo_i2c_notifier.h>
#include "stm32_pogo_i2c.h"

#define STM32KPD_DRV_NAME			"stm32_kpd"

#define INPUT_VENDOR_ID_SAMSUNG			0x04E8
#define INPUT_PRODUCT_ID_POGO_KEYBOARD		0xA035

#define STM32_KPC_ROW_SHIFT			5
#define STM32_KPC_DATA_PRESS			1

struct stm32_keyevent_data {
	union {
		struct {
			u16 press:1;
			u16 col:5;
			u16 row:3;
			u16 reserved:7;
		} __packed;
		u16 data[1];
	};
};

struct stm32_keypad_dtdata {
	struct matrix_keymap_data		*keymap_data1;
	struct matrix_keymap_data		*keymap_data2;
	int					num_row;
	int					num_column;
	const char				*input_name[2];
};

struct stm32_keypad_dev {
	struct platform_device			*pdev;
	struct input_dev			*input_dev;
	struct mutex				dev_lock;

	unsigned short				*keycode;
	int					keycode_entries;

	struct stm32_keypad_dtdata		*dtdata;
	int					*key_state;
	struct notifier_block			pogo_nb;
	char					key_name[714][10];
};

static void stm32_release_all_key(struct stm32_keypad_dev *stm32)
{
	int i, j;

	if (!stm32->input_dev)
		return;

	input_info(true, &stm32->pdev->dev, "%s\n", __func__);

	for (i = 0; i < stm32->dtdata->num_row; i++) {
		if (!stm32->key_state[i])
			continue;

		for (j = 0; j < stm32->dtdata->num_column; j++) {
			if (stm32->key_state[i] & (1 << j)) {
				int code = MATRIX_SCAN_CODE(i, j, STM32_KPC_ROW_SHIFT);
				input_event(stm32->input_dev, EV_MSC, MSC_SCAN, code);
				input_report_key(stm32->input_dev,
						stm32->keycode[code], 0);
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
				input_info(true, &stm32->pdev->dev, "RA code(0x%X|%d) R:C(%d:%d)\n",
						stm32->keycode[code], stm32->keycode[code], i, j);
#else
				input_info(true, &stm32->pdev->dev, "RA (%d:%d)\n", i, j);
#endif
				stm32->key_state[i] &= ~(1 << j);
			}
		}
	}
	input_sync(stm32->input_dev);
}

static void stm32_pogo_kpd_event(struct stm32_keypad_dev *stm32, u16 *key_values, int len)
{
	int i, event_count;

	if (!stm32->input_dev) {
		input_err(true, &stm32->pdev->dev, "%s: input dev is null\n", __func__);
		return;
	}

	if (!key_values || !len) {
		input_err(true, &stm32->pdev->dev, "%s: no event data\n", __func__);
		return;
	}

	event_count = len / sizeof(struct stm32_keyevent_data);
	for (i = 0; i < event_count; i++) {
		struct stm32_keyevent_data keydata;
		int code;

		keydata.data[0] = key_values[i];
		code = MATRIX_SCAN_CODE(keydata.row, keydata.col, STM32_KPC_ROW_SHIFT);

		if (keydata.row >= stm32->dtdata->num_row || keydata.col >= stm32->dtdata->num_column)
			continue;

		if (keydata.press != STM32_KPC_DATA_PRESS)	/* Release */
			stm32->key_state[keydata.row] &= ~(1 << keydata.col);
		else						/* Press */
			stm32->key_state[keydata.row] |= (1 << keydata.col);

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
		input_info(true, &stm32->pdev->dev, "%s '%s' 0x%02X(%d) R:C(%d:%d)\n",
				keydata.press != STM32_KPC_DATA_PRESS ? "R" : "P",
				stm32->key_name[stm32->keycode[code]],
				stm32->keycode[code], stm32->keycode[code],
				keydata.row, keydata.col);
#else
		input_info(true, &stm32->pdev->dev, "%s\n",
				keydata.press != STM32_KPC_DATA_PRESS ? "R" : "P");
#endif
		input_event(stm32->input_dev, EV_MSC, MSC_SCAN, code);
		input_report_key(stm32->input_dev, stm32->keycode[code], keydata.press);
		input_sync(stm32->input_dev);
	}
	input_dbg(false, &stm32->pdev->dev, "%s--\n", __func__);
}

#ifdef CONFIG_OF
static int stm32_parse_dt(struct device *dev,
		struct stm32_keypad_dev *device_data)
{
	struct device_node *np = dev->of_node;
	struct matrix_keymap_data *keymap_data1;
	struct matrix_keymap_data *keymap_data2;
	int ret, keymap_len1, keymap_len2, i;
	u32 *keymap1, *keymap2, temp;
	const __be32 *map1;
	const __be32 *map2;
	int count;

	count = of_property_count_strings(np, "keypad,input_name");
	if (count < 0)
		count = 1;

	ret = of_property_read_string_array(np, "keypad,input_name", device_data->dtdata->input_name, count);
	if (ret < 0) {
		input_err(true, dev, "unable to get model_name\n");
		return ret;
	}

	ret = of_property_read_u32(np, "keypad,num-rows", &temp);
	if (ret) {
		input_err(true, &device_data->pdev->dev, "%s unable to get num-rows\n", __func__);
		return ret;
	}
	device_data->dtdata->num_row = temp;

	ret = of_property_read_u32(np, "keypad,num-columns", &temp);
	if (ret) {
		input_err(true, &device_data->pdev->dev, "%s unable to get num-columns\n", __func__);
		return ret;
	}
	device_data->dtdata->num_column = temp;

	map1 = of_get_property(np, "linux,keymap1", &keymap_len1);

	if (!map1) {
		input_err(true, &device_data->pdev->dev, "%s Keymap not specified\n", __func__);
		return -EINVAL;
	}

	map2 = of_get_property(np, "linux,keymap2", &keymap_len2);

	if (!map2) {
		input_err(true, &device_data->pdev->dev, "%s Keymap not specified\n", __func__);
		return -EINVAL;
	}

	keymap_data1 = devm_kzalloc(dev, sizeof(*keymap_data1), GFP_KERNEL);
	if (!keymap_data1) {
		input_err(true, &device_data->pdev->dev, "%s Unable to allocate memory\n", __func__);
		return -ENOMEM;
	}

	keymap_data2 = devm_kzalloc(dev, sizeof(*keymap_data2), GFP_KERNEL);
	if (!keymap_data2) {
		input_err(true, &device_data->pdev->dev, "%s Unable to allocate memory\n", __func__);
		return -ENOMEM;
	}

	keymap_data1->keymap_size = (unsigned int)(keymap_len1 / sizeof(u32));
	keymap_data2->keymap_size = (unsigned int)(keymap_len2 / sizeof(u32));

	keymap1 = devm_kzalloc(dev,
			sizeof(uint32_t) * keymap_data1->keymap_size, GFP_KERNEL);
	if (!keymap1) {
		input_err(true, &device_data->pdev->dev, "%s could not allocate memory for keymap\n", __func__);
		return -ENOMEM;
	}

	keymap2 = devm_kzalloc(dev,
			sizeof(uint32_t) * keymap_data2->keymap_size, GFP_KERNEL);
	if (!keymap2) {
		input_err(true, &device_data->pdev->dev, "%s could not allocate memory for keymap\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < keymap_data1->keymap_size; i++) {
		unsigned int key = be32_to_cpup(map1 + i);
		int keycode, row, col;

		row = (key >> 24) & 0xff;
		col = (key >> 16) & 0xff;
		keycode = key & 0xffff;
		keymap1[i] = KEY(row, col, keycode);
	}

	for (i = 0; i < keymap_data2->keymap_size; i++) {
		unsigned int key2 = be32_to_cpup(map2 + i);
		int keycode2, row2, col2;

		row2 = (key2 >> 24) & 0xff;
		col2 = (key2 >> 16) & 0xff;
		keycode2 = key2 & 0xffff;
		keymap2[i] = KEY(row2, col2, keycode2);
	}

	/* short keymap for easy finding key */
	snprintf(device_data->key_name[KEY_1], 10, "1");
	snprintf(device_data->key_name[KEY_2], 10, "2");
	snprintf(device_data->key_name[KEY_3], 10, "3");
	snprintf(device_data->key_name[KEY_4], 10, "4");
	snprintf(device_data->key_name[KEY_5], 10, "5");
	snprintf(device_data->key_name[KEY_6], 10, "6");
	snprintf(device_data->key_name[KEY_7], 10, "7");
	snprintf(device_data->key_name[KEY_8], 10, "8");
	snprintf(device_data->key_name[KEY_9], 10, "9");
	snprintf(device_data->key_name[KEY_0], 10, "0");
	snprintf(device_data->key_name[KEY_MINUS], 10, "-");
	snprintf(device_data->key_name[KEY_EQUAL], 10, "=");
	snprintf(device_data->key_name[KEY_BACKSPACE], 10, "Backspc");
	snprintf(device_data->key_name[KEY_TAB], 10, "Tab");
	snprintf(device_data->key_name[KEY_Q], 10, "q");
	snprintf(device_data->key_name[KEY_W], 10, "w");
	snprintf(device_data->key_name[KEY_E], 10, "e");
	snprintf(device_data->key_name[KEY_R], 10, "r");
	snprintf(device_data->key_name[KEY_T], 10, "t");
	snprintf(device_data->key_name[KEY_Y], 10, "y");
	snprintf(device_data->key_name[KEY_U], 10, "u");
	snprintf(device_data->key_name[KEY_I], 10, "i");
	snprintf(device_data->key_name[KEY_O], 10, "o");
	snprintf(device_data->key_name[KEY_P], 10, "p");
	snprintf(device_data->key_name[KEY_LEFTBRACE], 10, "[");
	snprintf(device_data->key_name[KEY_RIGHTBRACE], 10, "]");
	snprintf(device_data->key_name[KEY_ENTER], 10, "Enter");
	snprintf(device_data->key_name[KEY_LEFTCTRL], 10, "L CTRL");
	snprintf(device_data->key_name[KEY_A], 10, "a");
	snprintf(device_data->key_name[KEY_S], 10, "s");
	snprintf(device_data->key_name[KEY_D], 10, "d");
	snprintf(device_data->key_name[KEY_F], 10, "f");
	snprintf(device_data->key_name[KEY_G], 10, "g");
	snprintf(device_data->key_name[KEY_H], 10, "h");
	snprintf(device_data->key_name[KEY_J], 10, "j");
	snprintf(device_data->key_name[KEY_K], 10, "k");
	snprintf(device_data->key_name[KEY_L], 10, "l");
	snprintf(device_data->key_name[KEY_SEMICOLON], 10, ";");
	snprintf(device_data->key_name[KEY_APOSTROPHE], 10, "\'");
	snprintf(device_data->key_name[KEY_GRAVE], 10, "`");
	snprintf(device_data->key_name[KEY_LEFTSHIFT], 10, "L Shift");
	snprintf(device_data->key_name[KEY_BACKSLASH], 10, "\\"); /* US : backslash , UK : pound*/
	snprintf(device_data->key_name[KEY_Z], 10, "z");
	snprintf(device_data->key_name[KEY_X], 10, "x");
	snprintf(device_data->key_name[KEY_C], 10, "c");
	snprintf(device_data->key_name[KEY_V], 10, "v");
	snprintf(device_data->key_name[KEY_B], 10, "b");
	snprintf(device_data->key_name[KEY_N], 10, "n");
	snprintf(device_data->key_name[KEY_M], 10, "m");
	snprintf(device_data->key_name[KEY_COMMA], 10, ",");
	snprintf(device_data->key_name[KEY_DOT], 10, ".");
	snprintf(device_data->key_name[KEY_SLASH], 10, "/");
	snprintf(device_data->key_name[KEY_RIGHTSHIFT], 10, "R Shift");
	snprintf(device_data->key_name[KEY_LEFTALT], 10, "L ALT");
	snprintf(device_data->key_name[KEY_SPACE], 10, " ");
	snprintf(device_data->key_name[KEY_CAPSLOCK], 10, "CAPSLOCK");
	snprintf(device_data->key_name[KEY_102ND], 10, "\\"); /* only UK : backslash */
	snprintf(device_data->key_name[KEY_RIGHTALT], 10, "R ALT");
	snprintf(device_data->key_name[KEY_UP], 10, "Up");
	snprintf(device_data->key_name[KEY_LEFT], 10, "Left");
	snprintf(device_data->key_name[KEY_RIGHT], 10, "Right");
	snprintf(device_data->key_name[KEY_DOWN], 10, "Down");
	snprintf(device_data->key_name[KEY_HANGEUL], 10, "Lang");
	snprintf(device_data->key_name[KEY_LEFTMETA], 10, "L Meta");
	snprintf(device_data->key_name[KEY_NUMERIC_POUND], 10, "#");
	snprintf(device_data->key_name[706], 10, "SIP"); /* SIP_ON_OFF */
	snprintf(device_data->key_name[710], 10, "Sfinder"); /* SIP_ON_OFF */
	snprintf(device_data->key_name[KEY_ESC], 10, "ESC");
	snprintf(device_data->key_name[KEY_FN], 10, "Fn");
	snprintf(device_data->key_name[KEY_F1], 10, "F1");
	snprintf(device_data->key_name[KEY_F2], 10, "F2");
	snprintf(device_data->key_name[KEY_F3], 10, "F3");
	snprintf(device_data->key_name[KEY_F4], 10, "F4");
	snprintf(device_data->key_name[KEY_F5], 10, "F5");
	snprintf(device_data->key_name[KEY_F6], 10, "F6");
	snprintf(device_data->key_name[KEY_F7], 10, "F7");
	snprintf(device_data->key_name[KEY_F8], 10, "F8");
	snprintf(device_data->key_name[KEY_F9], 10, "F9");
	snprintf(device_data->key_name[KEY_F10], 10, "F10");
	snprintf(device_data->key_name[KEY_F11], 10, "F11");
	snprintf(device_data->key_name[KEY_F12], 10, "F12");
	snprintf(device_data->key_name[KEY_DEX_ON], 10, "DEX");
	snprintf(device_data->key_name[KEY_DELETE], 10, "DEL");
	snprintf(device_data->key_name[KEY_HOMEPAGE], 10, "HOME");
	snprintf(device_data->key_name[KEY_END], 10, "END");
	snprintf(device_data->key_name[KEY_PAGEUP], 10, "PgUp");
	snprintf(device_data->key_name[KEY_PAGEDOWN], 10, "PgDown");
	snprintf(device_data->key_name[KEY_TOUCHPAD_ON], 10, "Tpad ON");
	snprintf(device_data->key_name[KEY_TOUCHPAD_OFF], 10, "Tpad OFF");
	snprintf(device_data->key_name[KEY_VOLUMEDOWN], 10, "VOL DOWN");
	snprintf(device_data->key_name[KEY_VOLUMEUP], 10, "VOL UP");
	snprintf(device_data->key_name[KEY_MUTE], 10, "VOL MUTE");
	snprintf(device_data->key_name[705], 10, "APPS");//apps
	snprintf(device_data->key_name[KEY_BRIGHTNESSDOWN], 10, "B-DOWN");
	snprintf(device_data->key_name[KEY_BRIGHTNESSUP], 10, "B-UP");
	snprintf(device_data->key_name[KEY_PLAYPAUSE], 10, "PLAYPAUSE");
	snprintf(device_data->key_name[KEY_NEXTSONG], 10, "NEXT");
	snprintf(device_data->key_name[KEY_PREVIOUSSONG], 10, "PREV");
	snprintf(device_data->key_name[KEY_SYSRQ], 10, "SYSRQ");
	snprintf(device_data->key_name[709], 10, "SETTING");
	snprintf(device_data->key_name[713], 10, "KBD SHARE");

	keymap_data1->keymap = keymap1;
	device_data->dtdata->keymap_data1 = keymap_data1;
	keymap_data2->keymap = keymap2;
	device_data->dtdata->keymap_data2 = keymap_data2;
	input_info(true, &device_data->pdev->dev, "%s, %s row:%d, col:%d, keymap1 size:%d keymap2 size:%d count:%d\n",
			device_data->dtdata->input_name[0], device_data->dtdata->input_name[1],
			device_data->dtdata->num_row, device_data->dtdata->num_column,
			device_data->dtdata->keymap_data1->keymap_size,
			device_data->dtdata->keymap_data2->keymap_size, count);
	return 0;
}
#else
static int stm32_parse_dt(struct device *dev,
		struct stm32_keypad_dev *device_data)
{
	return -ENODEV;
}
#endif

static int stm32_keypad_set_input_dev(struct stm32_keypad_dev *device_data, int module_id)
{
	struct input_dev *input_dev;
	int ret = 0;

	if (device_data->input_dev) {
		input_err(true, &device_data->pdev->dev, "%s input dev already exist\n", __func__);
		return ret;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		input_err(true, &device_data->pdev->dev, "%s: Failed to allocate memory for input device\n", __func__);
		return -ENOMEM;
	}

	device_data->input_dev = input_dev;
	device_data->input_dev->dev.parent = &device_data->pdev->dev;
	if (device_data->dtdata->input_name[0] || device_data->dtdata->input_name[1])
		device_data->input_dev->name = device_data->dtdata->input_name[module_id];
	else
		device_data->input_dev->name = "Book Cover Keyboard";

	input_info(true, &device_data->pdev->dev, "%s: input_name: %s\n", __func__, device_data->input_dev->name);
	device_data->input_dev->id.bustype = BUS_I2C;
	device_data->input_dev->id.vendor = INPUT_VENDOR_ID_SAMSUNG;
	device_data->input_dev->id.product = INPUT_PRODUCT_ID_POGO_KEYBOARD;
	device_data->input_dev->flush = NULL;
	device_data->input_dev->event = NULL;

	input_set_drvdata(device_data->input_dev, device_data);
	input_set_capability(device_data->input_dev, EV_MSC, MSC_SCAN);
	set_bit(EV_KEY, device_data->input_dev->evbit);

	device_data->input_dev->keycode = device_data->keycode;
	device_data->input_dev->keycodesize = sizeof(unsigned short);
	device_data->input_dev->keycodemax = device_data->keycode_entries;

	if (module_id == 1)
		matrix_keypad_build_keymap(device_data->dtdata->keymap_data2, NULL,
				device_data->dtdata->num_row, device_data->dtdata->num_column,
				device_data->keycode, device_data->input_dev);
	else
		matrix_keypad_build_keymap(device_data->dtdata->keymap_data1, NULL,
			device_data->dtdata->num_row, device_data->dtdata->num_column,
			device_data->keycode, device_data->input_dev);

	ret = input_register_device(device_data->input_dev);
	if (ret) {
		input_err(true, &device_data->pdev->dev, "%s: Failed to register input device\n", __func__);
		device_data->input_dev = NULL;
		return ret;
	}
	input_info(true, &device_data->pdev->dev, "%s: done\n", __func__);
	return ret;
}

static int stm32_kpd_pogo_notifier(struct notifier_block *nb,
		unsigned long action, void *data)
{
	struct stm32_keypad_dev *stm32 = container_of(nb, struct stm32_keypad_dev, pogo_nb);
	struct pogo_data_struct pogo_data =  *(struct pogo_data_struct *)data;

	mutex_lock(&stm32->dev_lock);

	switch (action) {
	case POGO_NOTIFIER_ID_ATTACHED:
		stm32_keypad_set_input_dev(stm32, pogo_data.module_id);
		break;
	case POGO_NOTIFIER_ID_DETACHED:
		stm32_release_all_key(stm32);
		if (stm32->input_dev) {
			input_unregister_device(stm32->input_dev);
			stm32->input_dev = NULL;
			input_info(true, &stm32->pdev->dev, "%s: input dev unregistered\n", __func__);
		}
		break;
	case POGO_NOTIFIER_ID_RESET:
		stm32_release_all_key(stm32);
		break;
	case POGO_NOTIFIER_EVENTID_KEYPAD:
		stm32_pogo_kpd_event(stm32, (u16 *)pogo_data.data, pogo_data.size);
		break;
	};

	mutex_unlock(&stm32->dev_lock);

	return NOTIFY_DONE;
}

static int stm32_keypad_probe(struct platform_device *pdev)
{
	struct stm32_keypad_dev *device_data;
	int ret = 0;

	input_info(true, &pdev->dev, "%s\n", __func__);

	device_data = kzalloc(sizeof(struct stm32_keypad_dev), GFP_KERNEL);
	if (!device_data) {
		input_err(true, &pdev->dev, "%s: Failed to allocate memory\n", __func__);
		return -ENOMEM;
	}

	device_data->pdev = pdev;

	platform_set_drvdata(pdev, device_data);

	mutex_init(&device_data->dev_lock);

	if (pdev->dev.of_node) {
		device_data->dtdata = kzalloc(sizeof(struct stm32_keypad_dtdata), GFP_KERNEL);
		if (!device_data->dtdata) {
			input_err(true, &pdev->dev, "%s: Failed to allocate memory\n", __func__);
			ret = -ENOMEM;
			goto err_config;
		}
		ret = stm32_parse_dt(&pdev->dev, device_data);
		if (ret) {
			input_err(true, &pdev->dev, "%s: Failed to use device tree\n", __func__);
			ret = -EIO;
			goto err_config;
		}

	} else {
		input_err(true, &device_data->pdev->dev, "Not use device tree\n");
		device_data->dtdata = pdev->dev.platform_data;
		if (!device_data->dtdata) {
			input_err(true, &pdev->dev, "%s: failed to get platform data\n", __func__);
			ret = -ENOENT;
			goto err_config;
		}
	}

	device_data->keycode_entries = max(device_data->dtdata->keymap_data1->keymap_size, device_data->dtdata->keymap_data2->keymap_size);
	input_info(true, &device_data->pdev->dev, "keycode entries (%d)\n", device_data->keycode_entries);
	input_info(true, &device_data->pdev->dev, "keymap size1 (%d) keymap size2 (%d)\n",
			device_data->dtdata->keymap_data1->keymap_size, device_data->dtdata->keymap_data2->keymap_size);

	device_data->key_state = kzalloc(device_data->dtdata->num_row * sizeof(int),
			GFP_KERNEL);
	if (!device_data->key_state) {
		input_err(true, &pdev->dev, "%s: key_state kzalloc memory error\n", __func__);
		ret = -ENOMEM;
		goto err_keystate;
	}

	device_data->keycode = kzalloc(device_data->keycode_entries * sizeof(unsigned short),
			GFP_KERNEL);
	if (!device_data->keycode) {
		input_err(true, &pdev->dev, "%s: keycode kzalloc memory error\n", __func__);
		ret = -ENOMEM;
		goto err_keycode;
	}

	pogo_notifier_register(&device_data->pogo_nb,
			stm32_kpd_pogo_notifier, POGO_NOTIFY_DEV_KEYPAD);

	input_info(true, &device_data->pdev->dev, "%s: done\n", __func__);

	return ret;

	//kfree(device_data->keycode);
err_keycode:
	kfree(device_data->key_state);
err_keystate:
err_config:
	mutex_destroy(&device_data->dev_lock);

	kfree(device_data);

	input_err(true, &pdev->dev, "Error at %s\n", __func__);

	return ret;
}

static int stm32_keypad_remove(struct platform_device *pdev)
{
	struct stm32_keypad_dev *device_data = platform_get_drvdata(pdev);

	pogo_notifier_unregister(&device_data->pogo_nb);

	input_free_device(device_data->input_dev);

	mutex_destroy(&device_data->dev_lock);

	kfree(device_data->keycode);
	kfree(device_data->key_state);
	kfree(device_data);
	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id stm32_match_table[] = {
	{ .compatible = "stm,keypad",},
	{ },
};
#else
#define stm32_match_table NULL
#endif

static struct platform_driver stm32_pogo_kpd_driver = {
	.driver = {
		.name = STM32KPD_DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = stm32_match_table,
	},
	.probe = stm32_keypad_probe,
	.remove = stm32_keypad_remove,
};

static int __init stm32_pogo_kpd_init(void)
{
	pr_info("%s %s\n", SECLOG, __func__);

	return platform_driver_register(&stm32_pogo_kpd_driver);
}
static void __exit stm32_pogo_kpd_exit(void)
{
	pr_info("%s %s\n", SECLOG, __func__);
	platform_driver_unregister(&stm32_pogo_kpd_driver);
}
//module_init(stm32_pogo_kpd_init);
late_initcall(stm32_pogo_kpd_init);
module_exit(stm32_pogo_kpd_exit);


MODULE_DESCRIPTION("STM32 Keypad Driver for POGO Keyboard");
MODULE_AUTHOR("Samsung");
MODULE_LICENSE("GPL");
