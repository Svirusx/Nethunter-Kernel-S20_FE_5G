
#ifndef __POGO_I2C_NOTIFIER_H__
#define __POGO_I2C_NOTIFIER_H__

#include <linux/notifier.h>

typedef enum {
	POGO_NOTIFY_DEV_TOUCHPAD = 0,
	POGO_NOTIFY_DEV_KEYPAD,
	POGO_NOTIFY_DEV_HALLIC,
} pogo_notifier_device_t;

enum STM32_ED_ID {
	ID_MCU = 1,
	ID_TOUCHPAD = 2,
	ID_KEYPAD = 3,
	ID_HALL = 4,
};

typedef enum {
	POGO_NOTIFIER_EVENTID_MCU = ID_MCU,
	POGO_NOTIFIER_EVENTID_TOUCHPAD = ID_TOUCHPAD,
	POGO_NOTIFIER_EVENTID_KEYPAD = ID_KEYPAD,
	POGO_NOTIFIER_EVENTID_HALL = ID_HALL,

	/* add new device upper here */
	POGO_NOTIFIER_EVENTID_MAX,

	POGO_NOTIFIER_ID_ATTACHED = 0xE0,
	POGO_NOTIFIER_ID_DETACHED,
	POGO_NOTIFIER_ID_RESET,
} pogo_notifier_id_t;

#define SET_POGO_NOTIFIER_BLOCK(nb, fn, dev) do {	\
		(nb)->notifier_call = (fn);		\
		(nb)->priority = (dev);			\
	} while (0)

#define DESTROY_POGO_NOTIFIER_BLOCK(nb)			\
		SET_POGO_NOTIFIER_BLOCK(nb, NULL, -1)

struct stm32_pogo_notifier {
	int conn;
	struct blocking_notifier_head pogo_notifier_call_chain;
};

struct pogo_data_struct {
	u8 size;
	char *data;
	int module_id;
};

int pogo_notifier_register(struct notifier_block *nb, notifier_fn_t notifier, pogo_notifier_device_t listener);
int pogo_notifier_unregister(struct notifier_block *nb);

#endif /* __POGO_I2C_NOTIFIER_H__*/
