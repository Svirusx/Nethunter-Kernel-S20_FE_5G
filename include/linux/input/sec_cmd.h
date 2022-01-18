
#ifndef _SEC_CMD_H_
#define _SEC_CMD_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <linux/input.h>
#include <linux/sched/clock.h>
#if defined(CONFIG_SEC_SYSFS)
#include <linux/sec_sysfs.h>
#elif defined(CONFIG_DRV_SAMSUNG)
#include <linux/sec_class.h>
#else
//struct class *tsp_sec_class;
#endif

#ifndef CONFIG_SEC_FACTORY
#define USE_SEC_CMD_QUEUE
#include <linux/kfifo.h>
#endif

#define SEC_CLASS_DEVT_TSP		10
#define SEC_CLASS_DEVT_TKEY		11
#define SEC_CLASS_DEVT_WACOM		12
#define SEC_CLASS_DEVT_SIDEKEY		13

#define SEC_CLASS_DEV_NAME_TSP		"tsp"
#define SEC_CLASS_DEV_NAME_TKEY		"sec_touchkey"
#define SEC_CLASS_DEV_NAME_WACOM	"sec_epen"
#define SEC_CLASS_DEV_NAME_SIDEKEY	"sec_sidekey"

#ifdef CONFIG_TOUCHSCREEN_DUAL_FOLDABLE
#define SEC_CLASS_DEVT_TSP1		15
#define SEC_CLASS_DEVT_TSP2		16

#define SEC_CLASS_DEV_NAME_TSP1		"tsp1"
#define SEC_CLASS_DEV_NAME_TSP2		"tsp2"
#endif

#define SEC_CMD(name, func)		.cmd_name = name, .cmd_func = func
#define SEC_CMD_H(name, func)		.cmd_name = name, .cmd_func = func, .cmd_log = 1

#define SEC_CMD_BUF_SIZE		(4096 - 1)
#define SEC_CMD_STR_LEN			256
#define SEC_CMD_RESULT_STR_LEN		(4096 - 1)
#define SEC_CMD_RESULT_STR_LEN_EXPAND	SEC_CMD_RESULT_STR_LEN * 2
#define SEC_CMD_PARAM_NUM		8

struct sec_cmd {
	struct list_head	list;
	const char		*cmd_name;
	void			(*cmd_func)(void *device_data);
	int			cmd_log;
};

enum SEC_CMD_STATUS {
	SEC_CMD_STATUS_WAITING = 0,
	SEC_CMD_STATUS_RUNNING,		// = 1
	SEC_CMD_STATUS_OK,		// = 2
	SEC_CMD_STATUS_FAIL,		// = 3
	SEC_CMD_STATUS_EXPAND,		// = 4
	SEC_CMD_STATUS_NOT_APPLICABLE,	// = 5
};

#ifdef USE_SEC_CMD_QUEUE
#define SEC_CMD_MAX_QUEUE	10

struct command {
	char	cmd[SEC_CMD_STR_LEN];
};
#endif

struct sec_cmd_data {
	struct device		*fac_dev;
	struct list_head	cmd_list_head;
	u8			cmd_state;
	char			cmd[SEC_CMD_STR_LEN];
	int			cmd_param[SEC_CMD_PARAM_NUM];
	char			*cmd_result;
	int			cmd_result_expand;
	int			cmd_result_expand_count;
	int			cmd_buffer_size;
	volatile bool		cmd_is_running;
	struct mutex		cmd_lock;
	struct mutex		fs_lock;
#ifdef USE_SEC_CMD_QUEUE
	struct kfifo		cmd_queue;
	struct mutex		fifo_lock;
	struct delayed_work	cmd_work;
#endif
	int item_count;
	char cmd_result_all[SEC_CMD_RESULT_STR_LEN];
	u8 cmd_all_factory_state;

};

extern void sec_cmd_set_cmd_exit(struct sec_cmd_data *data);
extern void sec_cmd_set_default_result(struct sec_cmd_data *data);
extern void sec_cmd_set_cmd_result(struct sec_cmd_data *data, char *buff, int len);
extern void sec_cmd_set_cmd_result_all(struct sec_cmd_data *data, char *buff, int len, char *item);
extern int sec_cmd_init(struct sec_cmd_data *data, struct sec_cmd *cmds, int len, int devt);
extern void sec_cmd_exit(struct sec_cmd_data *data, int devt);
extern void sec_cmd_send_event_to_user(struct sec_cmd_data *data, char *test, char *result);

#ifdef CONFIG_TOUCHSCREEN_DUAL_FOLDABLE
extern void sec_virtual_tsp_register(struct sec_cmd_data *sec);
#endif

typedef enum {
	NOTIFIER_NOTHING = 0,
	NOTIFIER_MAIN_TOUCH_ON,
	NOTIFIER_MAIN_TOUCH_OFF,
	NOTIFIER_SUB_TOUCH_ON,
	NOTIFIER_SUB_TOUCH_OFF,
	NOTIFIER_SECURE_TOUCH_ENABLE,		/* secure touch is enabled */
	NOTIFIER_SECURE_TOUCH_DISABLE,		/* secure touch is disabled */
	NOTIFIER_TSP_BLOCKING_REQUEST,		/* wacom called tsp block enable */
	NOTIFIER_TSP_BLOCKING_RELEASE,		/* wacom called tsp block disable */
	NOTIFIER_WACOM_PEN_CHARGING_FINISHED,	/* to tsp: pen charging finished */
	NOTIFIER_WACOM_PEN_CHARGING_STARTED,	/* to tsp: pen charging started */
	NOTIFIER_WACOM_PEN_INSERT,		/* to tsp: pen is inserted */
	NOTIFIER_WACOM_PEN_REMOVE,		/* to tsp: pen is removed */
	NOTIFIER_LCD_VRR_LFD_LOCK_REQUEST,	/* to LCD: set LFD min lock */
	NOTIFIER_LCD_VRR_LFD_LOCK_RELEASE,	/* to LCD: unset LFD min lock */
	NOTIFIER_LCD_VRR_LFD_OFF_REQUEST,	/* to LCD: set LFD OFF */
	NOTIFIER_LCD_VRR_LFD_OFF_RELEASE,	/* to LCD: unset LFD OFF */
	NOTIFIER_WACOM_PEN_HOVER_IN,
	NOTIFIER_WACOM_PEN_HOVER_OUT,
	NOTIFIER_VALUE_MAX,
} sec_input_notify_t;

#define MAIN_TOUCHSCREEN	0
#define SUB_TOUCHSCREEN		1

struct sec_input_notify_data {
	u8 dual_policy;
};

void sec_input_register_notify(struct notifier_block *nb, notifier_fn_t notifier_call, int priority);
void sec_input_unregister_notify(struct notifier_block *nb);
int sec_input_notify(struct notifier_block *nb, unsigned long noti, void *v);
int sec_input_self_request_notify(struct notifier_block *nb);
#endif /* _SEC_CMD_H_ */


