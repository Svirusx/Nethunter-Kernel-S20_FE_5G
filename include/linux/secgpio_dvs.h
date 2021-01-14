/*
 * secgpio_dvs.h -- Samsung GPIO debugging and verification system
 */

#ifndef __SECGPIO_DVS_H
#define __SECGPIO_DVS_H

#include <linux/types.h>

enum gdvs_phone_status {
	PHONE_INIT = 0,
	PHONE_SLEEP,
	GDVS_PHONE_STATUS_MAX
};

enum gdvs_io_value {
	GDVS_IO_FUNC = 0x00,
	GDVS_IO_IN,
	GDVS_IO_OUT,
};

enum gdvs_pupd_value {
	GDVS_PUPD_NP = 0x00,
	GDVS_PUPD_PD,
	GDVS_PUPD_PU,
	GDVS_PUPD_KEEPER,
	GDVS_PUPD_ERR = 0x3F
};

struct gpiomap_result {
	unsigned char *init;
	unsigned char *sleep;
};

#ifdef SECGPIO_SLEEP_DEBUGGING
enum sleepdata_phase {
	SLDATA_START = 0,
	SLDATA_DATA,
	SLDATA_END
};

struct sleepdebug_gpioinfo {
	int gpio_num;
	unsigned char io;
	unsigned char pupd;
	unsigned char lh;
};

struct sleepdebug_gpiotable {
	bool active;
	int gpio_count;
	struct sleepdebug_gpioinfo *gpioinfo;
};
#endif /* SECGPIO_SLEEP_DEBUGGING */

struct gpio_dvs {
	struct gpiomap_result *result;
	unsigned int count;
	bool check_init;
	bool check_sleep;
	void (*check_gpio_status)(unsigned char phonestate);
#ifdef SECGPIO_SLEEP_DEBUGGING
	struct sleepdebug_gpiotable *sdebugtable;
	void (*set_sleepgpio)(void);
	void (*undo_sleepgpio)(void);
#endif /* SECGPIO_SLEEP_DEBUGGING */
};

void gpio_dvs_check_initgpio(void);
void gpio_dvs_check_sleepgpio(void);
#ifdef SECGPIO_SLEEP_DEBUGGING
void gpio_dvs_set_sleepgpio(void);
void gpio_dvs_undo_sleepgpio(void);
#endif /* SECGPIO_SLEEP_DEBUGGING */

#endif /* __SECGPIO_DVS_H */
