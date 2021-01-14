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
#include <linux/input/mt.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif
#include <linux/input/pogo_i2c_notifier.h>

#define STM32_TOUCHPAD_DRV_NAME			"stm32_tpd"

#define STM32_TOUCH_MAX_FINGER_NUM		3

#define stm32_bit_test(val, n)			((val) & (1<<(n)))

/* bit value of status in point_data */
#define BIT_PT_CNT_CHANGE	0
#define BIT_DOWN		1
#define BIT_MOVE		2
#define BIT_UP			3
#define BIT_PALM		4
#define BIT_PALM_REJECT		5
#define BIT_GESTURE		6
#define RESERVED_1		7
#define BIT_WEIGHT_CHANGE	8
#define BIT_PT_NO_CHANGE	9
#define BIT_REJECT		10
#define BIT_PT_EXIST		11
#define RESERVED_2		12
#define BIT_MUST_ZERO		13
#define BIT_DEBUG		14
#define BIT_ICON_EVENT		15

/* button */
#define BIT_O_ICON0_DOWN	0

/* bit value of sub_status in coord */
#define SUB_BIT_EXIST		0
#define SUB_BIT_DOWN		1
#define SUB_BIT_MOVE		2
#define SUB_BIT_UP		3
#define SUB_BIT_UPDATE		4
#define SUB_BIT_WAIT		5

#define STAYING_RADIUS (130 * 130) // change as variable
#define RELEASE_SHORTLY_COUNT 2500 // 2.5s
#define ACCEPT_USER_MOVE_COUNT 300 // 300ms

#define X_EDGE_SIZE 220
#define Y_EDGE_SIZE 120

enum touched_area {
	TOUCHED_AREA_RELEASED		= 0,
	TOUCHED_AREA_NOTEDGE		= 1,
	TOUCHED_AREA_EDGE			= 2,
	TOUCHED_AREA_GRIP			= 3,
};

enum touch_action {
	TOUCH_ACTION_RELEASE = 0,
	TOUCH_ACTION_PRESS = 1,
	TOUCH_ACTION_MOVE = 2,
};

enum generate_event {
	GENERATE_NONE		= 0,
	GENERATE_PRESS		= 1,
	GENERATE_MOVE		= 2,
};

enum key_event {
	ICON_BUTTON_UP = 0,
	ICON_BUTTON_DOWN,
};

struct stm32_edge_touch {
	u8 id;
	u8 action;
	int x;
	int y;
	int w;
	ktime_t current_time;
};

struct stm32_coord {
	u16 x;
	u16 y;
	u8 w;
	u8 sub_status;
} __packed;

struct stm32_point_data {
	u16 status;
	u8 finger_cnt;
	u8 button_info;
	struct stm32_coord coords[STM32_TOUCH_MAX_FINGER_NUM];
} __packed;

struct stm32_touchpad_dtdata {
	int max_x;
	int max_y;
	bool xy_switch;
	bool x_invert;
	bool y_invert;
};

struct stm32_touchpad_dev {
	struct platform_device			*pdev;
	struct input_dev			*input_dev;
	struct mutex				dev_lock;

	struct stm32_touchpad_dtdata		*dtdata;
	struct notifier_block			pogo_nb;

	struct stm32_point_data			prev_touch_info;
	int					move_count[STM32_TOUCH_MAX_FINGER_NUM];
	int					touch_count;
	u8					button;
	u8					button_state;
	int					button_code;
	bool					btn_area_flag[STM32_TOUCH_MAX_FINGER_NUM];
	struct stm32_edge_touch			edge_touch[STM32_TOUCH_MAX_FINGER_NUM];
	struct stm32_edge_touch			coord[STM32_TOUCH_MAX_FINGER_NUM];
};

extern void pogo_get_tc_resolution(int *x, int *y);

static void stm32_release_all_finger(struct stm32_touchpad_dev *stm32)
{
	int i;

	if (!stm32->input_dev)
		return;

	input_info(true, &stm32->pdev->dev, "%s\n", __func__);
	stm32->button = ICON_BUTTON_UP;
	if (stm32->button_state == ICON_BUTTON_DOWN) {
		stm32->button_state = ICON_BUTTON_UP;
		input_report_key(stm32->input_dev, stm32->button_code, 0);
		input_info(true, &stm32->pdev->dev, "[BRA] 0x%X\n", stm32->button_code);
	}
	stm32->button_code = BTN_LEFT;

	for (i = 0; i < STM32_TOUCH_MAX_FINGER_NUM; i++) {
		input_mt_slot(stm32->input_dev, i);
		input_mt_report_slot_state(stm32->input_dev, MT_TOOL_FINGER, 0);

		if (stm32_bit_test(stm32->prev_touch_info.coords[i].sub_status, SUB_BIT_EXIST)) {
			input_info(true, &stm32->pdev->dev,
					"[RA] tID:%d mc:%d tc:%d\n",
					i, stm32->move_count[i], stm32->touch_count);
			stm32->prev_touch_info.coords[i].sub_status = 0;
			stm32->move_count[i] = 0;
		}
		stm32->edge_touch[i].action = TOUCHED_AREA_RELEASED;
		stm32->coord[i].action = TOUCH_ACTION_RELEASE;
		stm32->btn_area_flag[i] = false;
	}
	stm32->touch_count = 0;

	input_report_key(stm32->input_dev, BTN_TOUCH, 0);
	input_report_key(stm32->input_dev, BTN_TOOL_FINGER, 0);

	input_sync(stm32->input_dev);
}

static void stm32_pogo_xy_convert(struct stm32_touchpad_dev *stm32, int *x, int *y)
{
	if (stm32->dtdata->x_invert)
		*x = stm32->dtdata->max_x - 1 - *x;
	if (stm32->dtdata->y_invert)
		*y = stm32->dtdata->max_y - 1 - *y;

	if (stm32->dtdata->xy_switch) {
		u16 temp;

		temp = *x;
		*x = *y;
		*y = temp;
	}
}

void stm32_print_event(struct stm32_touchpad_dev *ts, u8 t_id, u8 gen_type)
{
	if (gen_type == GENERATE_NONE) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		input_info(true, &ts->pdev->dev,
				"[R] tID:%d lx:%d ly:%d mc:%d tc:%d\n",
				t_id, ts->coord[t_id].x, ts->coord[t_id].y,
				ts->move_count[t_id], ts->touch_count);
#else
		input_info(true, &ts->pdev->dev,
				"[R] tID:%d mc:%d tc:%d\n",
				t_id, ts->move_count[t_id], ts->touch_count);
#endif
	} else if (gen_type == GENERATE_PRESS) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		input_info(true, &ts->pdev->dev,
				"[P] tID:%d x:%d y:%d tc:%d\n",
				t_id, ts->coord[t_id].x, ts->coord[t_id].y, ts->touch_count);
#else
		input_info(true, &ts->pdev->dev,
				"[P] tID:%d tc:%d\n",
				t_id, ts->touch_count);
#endif
	} else if (gen_type == GENERATE_MOVE) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		input_info(true, &ts->pdev->dev,
				"[M] tID:%d x:%d y:%d mc:%d, tc:%d\n",
				t_id, ts->coord[t_id].x, ts->coord[t_id].y,
				ts->move_count[t_id], ts->touch_count);
#else
		input_info(true, &ts->pdev->dev,
				"[M] tID:%d mc:%d, tc:%d\n",
				t_id, ts->move_count[t_id], ts->touch_count);
#endif
	} else {
		input_info(true, &ts->pdev->dev, "Wrong event type to print\n");
	}
}

bool stm32_touch_at_edge_boundary(struct stm32_touchpad_dev *ts, int t_id)
{
	int res_x, res_y;

	pogo_get_tc_resolution(&res_x, &res_y);

	if (ts->coord[t_id].x < X_EDGE_SIZE)
		return true;
	else if (ts->coord[t_id].x > (res_x - X_EDGE_SIZE))
		return true;

	if (ts->coord[t_id].y < Y_EDGE_SIZE)
		return true;

	return false;
}

bool stm32_move_out_of_circle(struct stm32_touchpad_dev *ts, int t_id)
{
	u16 deltax, deltay;
	u64 dist;

	if (ts->edge_touch[t_id].x > ts->coord[t_id].x)
		deltax = ts->edge_touch[t_id].x - ts->coord[t_id].x;
	else
		deltax = ts->coord[t_id].x - ts->edge_touch[t_id].x;

	if (ts->edge_touch[t_id].y > ts->coord[t_id].y)
		deltay = ts->edge_touch[t_id].y - ts->coord[t_id].y;
	else
		deltay = ts->coord[t_id].y - ts->edge_touch[t_id].y;

	dist = deltax*deltax + deltay*deltay;

	if (dist > STAYING_RADIUS)
		return true;
	else
		return false;
}

static bool stm32_check_skip_release(struct stm32_touchpad_dev *stm32, int t_id)
{
	bool skip_release = false;

	if (stm32->edge_touch[t_id].action == TOUCHED_AREA_EDGE) {
		input_info(true, &stm32->pdev->dev,
				"Released after staying within a certain area. Discarded. tID:%d\n", t_id);
		skip_release = true;
	} else if (stm32->edge_touch[t_id].action == TOUCHED_AREA_GRIP) {
		input_info(true, &stm32->pdev->dev,
				"Released after holding it by hand, Discarded. tID:%d\n", t_id);
		skip_release = true;
	} else if (stm32->edge_touch[t_id].action == TOUCHED_AREA_RELEASED) {
		skip_release = true;
	}

	stm32->edge_touch[t_id].action = TOUCHED_AREA_RELEASED;
	return skip_release;
}

static bool stm32_check_skip_press(struct stm32_touchpad_dev *stm32, int t_id)
{
	if (stm32_touch_at_edge_boundary(stm32, t_id)) {
		stm32->edge_touch[t_id].x = stm32->coord[t_id].x;
		stm32->edge_touch[t_id].y = stm32->coord[t_id].y;
		stm32->edge_touch[t_id].action = TOUCHED_AREA_EDGE;
		stm32->edge_touch[t_id].current_time = ktime_get();
		input_info(true, &stm32->pdev->dev,
				"Pressed in the Edge area. just store event tID:%d\n", t_id);
		return true;
	}

	stm32->edge_touch[t_id].action = TOUCHED_AREA_NOTEDGE;
	return false;
}

static bool stm32_check_skip_move(struct stm32_touchpad_dev *stm32, int t_id)
{
	s64 time_diff;

	if (stm32->edge_touch[t_id].action != TOUCHED_AREA_NOTEDGE && stm32->touch_count == 3) {
		input_info(true, &stm32->pdev->dev,
				"3 finger touch gesture. Converted to normal touch. tID:%d\n", t_id);
		stm32->edge_touch[t_id].action = TOUCHED_AREA_NOTEDGE;
		stm32->coord[t_id].action = TOUCH_ACTION_PRESS;
		return false;
	}

	if (stm32->edge_touch[t_id].action == TOUCHED_AREA_EDGE) {
		if (stm32_move_out_of_circle(stm32, t_id)) {
			input_info(true, &stm32->pdev->dev,
					"Moves outside a specific boundary area. "
					"Converted to a normal touch. tID:%d\n", t_id);
			stm32->edge_touch[t_id].action = TOUCHED_AREA_NOTEDGE;
			stm32->coord[t_id].action = TOUCH_ACTION_PRESS;
		} else {
			time_diff = ktime_ms_delta(ktime_get(), stm32->edge_touch[t_id].current_time);
			if (time_diff >= RELEASE_SHORTLY_COUNT) {
				input_info(true, &stm32->pdev->dev,
						"Stays for a long time within a specific boundary area. "
						"Converted to a grip touch. tID:%d\n", t_id);
				stm32->edge_touch[t_id].action = TOUCHED_AREA_GRIP;
			}
			return true;
		}
	} else if (stm32->edge_touch[t_id].action == TOUCHED_AREA_GRIP) {
		return true;
	} else if (stm32->edge_touch[t_id].action == TOUCHED_AREA_RELEASED) {
		/* In this case, a touch input is received within 200ms while typing the keyboard.*/
		time_diff = ktime_ms_delta(ktime_get(), stm32->coord[t_id].current_time);
		if (time_diff > ACCEPT_USER_MOVE_COUNT) {
			input_info(true, &stm32->pdev->dev,
					"Move events over 300ms. tID:%d\n", t_id);
			stm32->coord[t_id].action = TOUCH_ACTION_PRESS;
			return stm32_check_skip_press(stm32, t_id);
		} else {
			input_dbg(true, &stm32->pdev->dev, "Move events without Press, Discard tID:%d\n", t_id);
			return true;
		}
	}

	return false;
}

static void stm32_pogo_touchpad_event(struct stm32_touchpad_dev *stm32, char *event, int len)
{
	struct stm32_point_data touch_info;
	int i;
	static int latest_id = 0;
	u8 sub_status, prev_sub_status;
	int x, y, w, lx, ly;
	int res_x, res_y;
	bool skip_touch_event;

	if (!stm32->input_dev) {
		input_err(true, &stm32->pdev->dev, "%s: input dev is null\n", __func__);
		return;
	}

	if (!event || !len) {
		input_err(true, &stm32->pdev->dev, "%s: no event data\n", __func__);
		return;
	}

	if (len != sizeof(struct stm32_point_data)) {
		input_err(true, &stm32->pdev->dev,
				"%s: event size is wrong %d\n", __func__, len);
		return;
	}

	memcpy(&touch_info, event, len);
	pogo_get_tc_resolution(&res_x, &res_y);

	if (stm32_bit_test(touch_info.status, BIT_ICON_EVENT)) {
		if (stm32_bit_test(touch_info.button_info, BIT_O_ICON0_DOWN)) {
			stm32->button = ICON_BUTTON_DOWN;
		} else {
			stm32->button = ICON_BUTTON_UP;
			if (stm32->button_state == ICON_BUTTON_DOWN)
				input_info(true, &stm32->pdev->dev,
						"[BR] 0x%X\n", stm32->button_code);
			stm32->button_state = ICON_BUTTON_UP;
			input_report_key(stm32->input_dev, stm32->button_code, ICON_BUTTON_UP);
		}
	}

	if (!stm32_bit_test(touch_info.status, BIT_PT_EXIST)) {
		/* all finger is released */
		for (i = 0; i < STM32_TOUCH_MAX_FINGER_NUM; i++) {
			prev_sub_status = stm32->prev_touch_info.coords[i].sub_status;
			lx = stm32->prev_touch_info.coords[i].x;
			ly = stm32->prev_touch_info.coords[i].y;

			stm32_pogo_xy_convert(stm32, &lx, &ly);

			if (stm32_bit_test(prev_sub_status, SUB_BIT_EXIST)) {
				if (stm32->touch_count > 0)
					stm32->touch_count--;
				if (stm32->touch_count == 0)
					input_report_key(stm32->input_dev, BTN_TOUCH, 0);
				else
					latest_id = (i == 0) ? 1 : 0;

				input_mt_slot(stm32->input_dev, i);
				input_mt_report_slot_state(stm32->input_dev, MT_TOOL_FINGER, 0);

				skip_touch_event = stm32_check_skip_release(stm32, i);
				if (skip_touch_event == false) {
					stm32_print_event(stm32, i, GENERATE_NONE);
				}

				stm32->coord[i].action = TOUCH_ACTION_RELEASE;
				stm32->move_count[i] = 0;
				stm32->btn_area_flag[i] = false;
			}
		}
		memset(&stm32->prev_touch_info, 0x0, sizeof(struct stm32_point_data));
		goto out_sync;
	}

	for (i = 0; i < STM32_TOUCH_MAX_FINGER_NUM; i++) {
		sub_status = touch_info.coords[i].sub_status;
		prev_sub_status = stm32->prev_touch_info.coords[i].sub_status;

		if (stm32_bit_test(sub_status, SUB_BIT_EXIST)) {
			x = stm32->coord[i].x = touch_info.coords[i].x;
			y = stm32->coord[i].y = touch_info.coords[i].y;
			w = stm32->coord[i].w = touch_info.coords[i].w;

			if (x < 0 || y < 0 || x >= stm32->dtdata->max_x || y >= stm32->dtdata->max_y) {
				input_err(true, &stm32->pdev->dev,
						"Invalid coord tID:%d raw_x:%d raw_y:%d\n", i, x, y);
				continue;
			}

			stm32_pogo_xy_convert(stm32, &x, &y);

			skip_touch_event = true;

			if (stm32_bit_test(sub_status, SUB_BIT_DOWN)) {
				/* press */
				stm32->touch_count++;
				latest_id = i;

				if ((touch_info.coords[i].x < res_x / 2) || (touch_info.coords[i].y < (res_y * 3) / 4))
					stm32->btn_area_flag[i] = true;

				stm32->coord[i].action = TOUCH_ACTION_PRESS;
				stm32->coord[i].current_time = ktime_get();

				skip_touch_event = stm32_check_skip_press(stm32, i);
			} else if (stm32_bit_test(sub_status, SUB_BIT_MOVE)) {
				/* move */
				stm32->move_count[i]++;
				if ((touch_info.coords[i].x < res_x / 2) || (touch_info.coords[i].y < (res_y * 3) / 4))
					stm32->btn_area_flag[i] = true;

				/* touch moved without press */
				if (stm32->coord[i].action == TOUCH_ACTION_RELEASE) {
					stm32->touch_count++;
					latest_id = i;
					stm32->coord[i].current_time = ktime_get();
				}
				stm32->coord[i].action = TOUCH_ACTION_MOVE;

				skip_touch_event = stm32_check_skip_move(stm32, i);
			}

			if (skip_touch_event == false) {
				if (stm32->coord[i].action == TOUCH_ACTION_PRESS)
					stm32_print_event(stm32, i, GENERATE_PRESS);
				//else
				//	stm32_print_event(stm32, i, GENERATE_MOVE);

				if (w == 0)
					w = 1;

				input_mt_slot(stm32->input_dev, i);
				input_mt_report_slot_state(stm32->input_dev, MT_TOOL_FINGER, 1);

				input_report_abs(stm32->input_dev, ABS_MT_TOUCH_MAJOR, (u32)w);
				input_report_abs(stm32->input_dev, ABS_MT_POSITION_X, x);
				input_report_abs(stm32->input_dev, ABS_MT_POSITION_Y, y);
				input_report_key(stm32->input_dev, BTN_TOUCH, 1);
			}
		} else if (stm32_bit_test(sub_status, SUB_BIT_UP) ||
				stm32_bit_test(prev_sub_status, SUB_BIT_EXIST)) {
			/* release */
			lx = stm32->prev_touch_info.coords[i].x;
			ly = stm32->prev_touch_info.coords[i].y;

			stm32_pogo_xy_convert(stm32, &lx, &ly);

			if (stm32->touch_count > 0)
				stm32->touch_count--;
			if (stm32->touch_count == 0)
				input_report_key(stm32->input_dev, BTN_TOUCH, 0);
			else
				latest_id = (i == 0) ? 1 : 0;

			input_mt_slot(stm32->input_dev, i);
			input_mt_report_slot_state(stm32->input_dev, MT_TOOL_FINGER, 0);

			skip_touch_event = stm32_check_skip_release(stm32, i);
			if (skip_touch_event == false) {
				stm32_print_event(stm32, i, GENERATE_NONE);
			}

			stm32->coord[i].action = TOUCH_ACTION_RELEASE;
			stm32->btn_area_flag[i] = false;
			memset(&touch_info.coords[i], 0x0, sizeof(struct stm32_coord));
			stm32->move_count[i] = 0;
		} else {
			memset(&touch_info.coords[i], 0x0, sizeof(struct stm32_coord));
		}
	}
	memcpy((char *)&stm32->prev_touch_info, (char *)&touch_info,
		sizeof(struct stm32_point_data));

out_sync:
	/* decide LEFT/RIGHT button event based on coordinate of last touch id */
	if (stm32->button_state == ICON_BUTTON_UP) {
		if (stm32->touch_count == 0) {
			stm32->button_code = BTN_LEFT;
			latest_id = 0;
		} else {
			if (stm32->btn_area_flag[latest_id])
				stm32->button_code = BTN_LEFT;
			else
				stm32->button_code = BTN_RIGHT;

			if (stm32->button == ICON_BUTTON_DOWN) {
				stm32->button_state = ICON_BUTTON_DOWN;
				input_mt_slot(stm32->input_dev, latest_id);
				input_mt_report_slot_state(stm32->input_dev, MT_TOOL_FINGER, 1);
				input_report_key(stm32->input_dev, BTN_TOUCH, 1);
				input_report_key(stm32->input_dev, stm32->button_code, ICON_BUTTON_DOWN);
				input_info(true, &stm32->pdev->dev,
						"[BP] tID:%d 0x%X btn_f:%d\n", latest_id, stm32->button_code,
						stm32->btn_area_flag[latest_id]);
			}
		}
	}

	input_sync(stm32->input_dev);
}

#ifdef CONFIG_OF
static int stm32_parse_dt(struct device *dev,
		struct stm32_touchpad_dev *device_data)
{
	struct device_node *np = dev->of_node;
	int ret;
	int temp[3];

	ret = of_property_read_u32_array(np, "touchpad,max_coords", temp, 2);
	if (ret) {
		input_err(true, &device_data->pdev->dev,
				"failed to get max_coords\n");
		return -ENOENT;
	} else {
		device_data->dtdata->max_x = temp[0];
		device_data->dtdata->max_y = temp[1];
	}

	memset(temp, 0x00, 3);

	ret = of_property_read_u32_array(np, "touchpad,invert", temp, 3);
	if (!ret) {
		device_data->dtdata->x_invert = temp[0];
		device_data->dtdata->y_invert = temp[1];
		device_data->dtdata->xy_switch = temp[2];
	}

	input_info(true, &device_data->pdev->dev, "max_x:%d, max_y:%d, invert:%d,%d, switch:%d\n",
			device_data->dtdata->max_x, device_data->dtdata->max_y,
			device_data->dtdata->x_invert, device_data->dtdata->y_invert,
			device_data->dtdata->xy_switch);
	return 0;
}
#else
static int stm32_parse_dt(struct device *dev,
		struct stm32_touchpad_dev *device_data)
{
	return -ENODEV;
}
#endif

static int stm32_touchpad_set_input_dev(struct stm32_touchpad_dev *device_data)
{
	struct input_dev *input_dev;
	int ret = 0, i;

	if (device_data->input_dev) {
		input_err(true, &device_data->pdev->dev,
				"%s input dev already exist\n", __func__);
		return ret;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		input_err(true, &device_data->pdev->dev,
				"%s: Failed to allocate memory for input device\n", __func__);
		return -ENOMEM;
	}

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(INPUT_PROP_POINTER, input_dev->propbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);
	set_bit(BTN_LEFT, input_dev->keybit);
	set_bit(BTN_RIGHT, input_dev->keybit);
	device_data->button_code = BTN_LEFT;

	if (device_data->dtdata->xy_switch) {
		input_set_abs_params(input_dev, ABS_MT_POSITION_X,
				0, device_data->dtdata->max_y - 1, 0, 0);
		input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
				0, device_data->dtdata->max_x - 1, 0, 0);
	} else {
		input_set_abs_params(input_dev, ABS_MT_POSITION_X,
				0, device_data->dtdata->max_x - 1, 0, 0);
		input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
				0, device_data->dtdata->max_y - 1, 0, 0);
	}
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_mt_init_slots(input_dev, STM32_TOUCH_MAX_FINGER_NUM, INPUT_MT_POINTER);

	for (i = 0; i < STM32_TOUCH_MAX_FINGER_NUM; i++) {
		device_data->edge_touch[i].action = TOUCHED_AREA_RELEASED;
		device_data->coord[i].action = TOUCH_ACTION_RELEASE;
	}

	device_data->input_dev = input_dev;
	device_data->input_dev->dev.parent = &device_data->pdev->dev;
	device_data->input_dev->name = "sec_touchpad_pogo";
	input_set_drvdata(device_data->input_dev, device_data);

	ret = input_register_device(device_data->input_dev);
	if (ret) {
		input_err(true, &device_data->pdev->dev,
				"%s: Failed to register input device\n", __func__);
		device_data->input_dev = NULL;
		return ret;
	}
	input_info(true, &device_data->pdev->dev, "%s: done\n", __func__);
	return ret;
}

static int stm32_touchpad_pogo_notifier(struct notifier_block *nb,
		unsigned long action, void *data)
{
	struct stm32_touchpad_dev *stm32 = container_of(nb, struct stm32_touchpad_dev, pogo_nb);
	struct pogo_data_struct pogo_data =  *(struct pogo_data_struct *)data;

	mutex_lock(&stm32->dev_lock);
	switch (action) {
	case POGO_NOTIFIER_ID_ATTACHED:
		stm32_touchpad_set_input_dev(stm32);
		break;
	case POGO_NOTIFIER_ID_DETACHED:
		stm32_release_all_finger(stm32);
		if (stm32->input_dev) {
			input_unregister_device(stm32->input_dev);
			stm32->input_dev = NULL;
			input_info(true, &stm32->pdev->dev,
					"%s: input dev unregistered\n", __func__);
		}
		break;
	case POGO_NOTIFIER_ID_RESET:
		stm32_release_all_finger(stm32);
		break;
	case POGO_NOTIFIER_EVENTID_TOUCHPAD:
		stm32_pogo_touchpad_event(stm32, pogo_data.data, pogo_data.size);
		break;
	};
	mutex_unlock(&stm32->dev_lock);

	return NOTIFY_DONE;
}

static int stm32_touchpad_probe(struct platform_device *pdev)
{
	struct stm32_touchpad_dev *device_data;
	int ret = 0;

	input_info(true, &pdev->dev, "%s\n", __func__);

	device_data = kzalloc(sizeof(struct stm32_touchpad_dev), GFP_KERNEL);
	if (!device_data) {
		input_err(true, &pdev->dev, "%s: Failed to allocate memory\n", __func__);
		return -ENOMEM;
	}

	device_data->pdev = pdev;

	platform_set_drvdata(pdev, device_data);

	if (pdev->dev.of_node) {
		device_data->dtdata = kzalloc(sizeof(struct stm32_touchpad_dtdata), GFP_KERNEL);
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
			input_err(true, &pdev->dev,
					"%s: failed to get platform data\n", __func__);
			ret = -ENOENT;
			goto err_config;
		}
	}

	mutex_init(&device_data->dev_lock);

	pogo_notifier_register(&device_data->pogo_nb,
			stm32_touchpad_pogo_notifier, POGO_NOTIFY_DEV_TOUCHPAD);

	input_info(true, &device_data->pdev->dev, "%s: done\n", __func__);

	return ret;

err_config:
	kfree(device_data);

	input_err(true, &pdev->dev, "Error at %s\n", __func__);
	return ret;
}

static int stm32_touchpad_remove(struct platform_device *pdev)
{
	struct stm32_touchpad_dev *device_data = platform_get_drvdata(pdev);

	pogo_notifier_unregister(&device_data->pogo_nb);

	input_free_device(device_data->input_dev);

	mutex_destroy(&device_data->dev_lock);

	kfree(device_data);
	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id stm32_match_table[] = {
	{ .compatible = "stm,touchpad",},
	{ },
};
#else
#define stm32_match_table NULL
#endif

static struct platform_driver stm32_pogo_touchpad_driver = {
	.driver = {
		.name = STM32_TOUCHPAD_DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = stm32_match_table,
	},
	.probe = stm32_touchpad_probe,
	.remove = stm32_touchpad_remove,
};

static int __init stm32_pogo_touchpad_init(void)
{
	pr_info("%s %s\n", SECLOG, __func__);

	return platform_driver_register(&stm32_pogo_touchpad_driver);
}
static void __exit stm32_pogo_touchpad_exit(void)
{
	pr_info("%s %s\n", SECLOG, __func__);
	platform_driver_unregister(&stm32_pogo_touchpad_driver);
}
//module_init(stm32_pogo_touchpad_init);
late_initcall(stm32_pogo_touchpad_init);
module_exit(stm32_pogo_touchpad_exit);


MODULE_DESCRIPTION("STM32 Touchpad Driver for POGO Keyboard");
MODULE_AUTHOR("Samsung");
MODULE_LICENSE("GPL");
