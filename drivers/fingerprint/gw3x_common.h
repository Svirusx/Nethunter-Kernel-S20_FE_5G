#ifndef __GF_SPI_DRIVER_H
#define __GF_SPI_DRIVER_H

#include <linux/types.h>
#include <linux/netlink.h>
#include <linux/cdev.h>
#include <linux/notifier.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/workqueue.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/timer.h>
#include <linux/err.h>
#include <linux/fb.h>
#include <net/sock.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#endif

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#ifdef ENABLE_SENSORS_FPRINT_SECURE
#include <linux/wakelock.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/of_device.h>
#include <linux/of_dma.h>
#include <linux/amba/bus.h>
#if defined(CONFIG_SECURE_OS_BOOSTER_API)
#include <mach/secos_booster.h>
#elif defined(CONFIG_TZDEV_BOOST)
#include <../drivers/misc/tzdev/tz_boost.h>
#endif
struct sec_spi_info {
	int		port;
	unsigned long	speed;
};
#endif

#include <linux/wakelock.h>
#include <linux/cpufreq.h>
#include <linux/pinctrl/consumer.h>
#include "../pinctrl/core.h"
#include <linux/pm_qos.h>
#include <linux/version.h>

/*
 * This feature is temporary for exynos AP only.
 * It's for control GPIO config on enabled TZ before enable GPIO protection.
 * If it's still defined this feature after enable GPIO protection,
 * it will be happened kernel panic
 * So it should be un-defined after enable GPIO protection
 */
#undef DISABLED_GPIO_PROTECTION

#define GF_IOC_MAGIC	'g'

#define GF_GW32J_CHIP_ID	0x00220e
#define GF_GW32N_CHIP_ID	0x002215
#define GF_GW36H_CHIP_ID	0x002504
#define GF_GW36C_CHIP_ID	0x002502
#define GF_GW36T_CHIP_ID	0x004a0f

#define gw3x_SPI_BAUD_RATE 9600000
#define TANSFER_MAX_LEN (512*1024)

enum gf_netlink_cmd {
	GF_NETLINK_TEST = 0,
	GF_NETLINK_IRQ = 1,
	GF_NETLINK_SCREEN_OFF,
	GF_NETLINK_SCREEN_ON
};

struct gf_ioc_transfer {
	u8 cmd;    /* spi read = 0, spi  write = 1 */
	u8 reserved;
	u16 addr;
	u32 len;
	u8 *buf;
};

struct gf_ioc_transfer_raw {
	u32 len;
	u8 *read_buf;
	u8 *write_buf;
	uint32_t high_time;
	uint32_t bits_per_word;
};

/* define commands */
#define GF_IOC_INIT             _IOR(GF_IOC_MAGIC, 0, u8)
#define GF_IOC_EXIT             _IO(GF_IOC_MAGIC, 1)
#define GF_IOC_RESET            _IO(GF_IOC_MAGIC, 2)
#define GF_IOC_ENABLE_IRQ       _IO(GF_IOC_MAGIC, 3)
#define GF_IOC_DISABLE_IRQ      _IO(GF_IOC_MAGIC, 4)
#define GF_IOC_ENABLE_SPI_CLK   _IOW(GF_IOC_MAGIC, 5, uint32_t)
#define GF_IOC_DISABLE_SPI_CLK  _IO(GF_IOC_MAGIC, 6)
#define GF_IOC_ENABLE_POWER     _IO(GF_IOC_MAGIC, 7)
#define GF_IOC_DISABLE_POWER    _IO(GF_IOC_MAGIC, 8)
#define GF_IOC_ENTER_SLEEP_MODE _IO(GF_IOC_MAGIC, 10)
#define GF_IOC_GET_FW_INFO      _IOR(GF_IOC_MAGIC, 11, u8)
#define GF_IOC_REMOVE           _IO(GF_IOC_MAGIC, 12)

/* for SPI REE transfer */
#ifndef ENABLE_SENSORS_FPRINT_SECURE
#define GF_IOC_TRANSFER_CMD     _IOWR(GF_IOC_MAGIC, 15, \
		struct gf_ioc_transfer)
#define GF_IOC_TRANSFER_RAW_CMD _IOWR(GF_IOC_MAGIC, 16, \
		struct gf_ioc_transfer_raw)
#else
#define GF_IOC_SET_SENSOR_TYPE _IOW(GF_IOC_MAGIC, 18, unsigned int)
#endif
#define GF_IOC_POWER_CONTROL _IOW(GF_IOC_MAGIC, 19, unsigned int)
#define GF_IOC_SPEEDUP			_IOW(GF_IOC_MAGIC, 20, unsigned int)
#define GF_IOC_SET_LOCKSCREEN	_IOW(GF_IOC_MAGIC, 21, unsigned int)
#define GF_IOC_GET_ORIENT		_IOR(GF_IOC_MAGIC, 22, unsigned int)

#define GF_IOC_MAXNR    23  /* THIS MACRO IS NOT USED NOW... */

struct gf_device {
	dev_t devno;
	struct cdev cdev;
	struct device *fp_device;
	struct class *class;
	struct spi_device *spi;
	int device_count;

	spinlock_t spi_lock;
	struct list_head device_entry;
	struct mutex release_lock;
	struct sock *nl_sk;
	u8 buf_status;
	struct notifier_block notifier;
	u8 irq_enabled;
	u8 sig_count;
	u8 system_status;

	u32 pwr_gpio;
	u32 reset_gpio;
	int prev_bits_per_word;
	u32 irq_gpio;
	u32 irq;
	u8  need_update;
	/* for netlink use */
	int pid;

	struct work_struct work_debug;
	struct workqueue_struct *wq_dbg;
	struct timer_list dbg_timer;

#ifdef ENABLE_SENSORS_FPRINT_SECURE
	bool enabled_clk;
	struct clk *fp_spi_pclk;
	struct clk *fp_spi_sclk;
#else
	u8 *spi_buffer;
	u8 *tx_buf;
	u8 *rx_buf;
	struct mutex buf_lock;
#endif
	unsigned int current_spi_speed;
	unsigned int orient;
	unsigned int min_cpufreq_limit;
	u32 spi_speed;
	int sensortype;
	int reset_count;
	int interrupt_count;
	bool ldo_onoff;
	bool tz_mode;
	const char *chipid;
	struct wake_lock wake_lock;
	const char *btp_vdd;
	struct regulator *regulator_3p3;
	struct pinctrl *p;
	struct pinctrl_state *pins_poweron;
	struct pinctrl_state *pins_poweroff;
	struct pm_qos_request pm_qos;
};


int gw3x_get_gpio_dts_info(struct device *dev, struct gf_device *gf_dev);
void gw3x_cleanup_info(struct gf_device *gf_dev);
void gw3x_hw_power_enable(struct gf_device *gf_dev, u8 onoff);
int gw3x_spi_clk_enable(struct gf_device *gf_dev);
int gw3x_spi_clk_disable(struct gf_device *gf_dev);
void gw3x_hw_reset(struct gf_device *gf_dev, u8 delay);
void gw3x_spi_setup_conf(struct gf_device *gf_dev, u32 speed);
int gw3x_pin_control(struct gf_device *gf_dev, bool pin_set);
int gw3x_register_platform_variable(struct gf_device *gf_dev);
int gw3x_unregister_platform_variable(struct gf_device *gf_dev);
int gw3x_set_cpu_speedup(struct gf_device *gf_dev, int onoff);

#ifndef ENABLE_SENSORS_FPRINT_SECURE
int gw3x_spi_read_bytes(struct gf_device *gf_dev, u16 addr,
		u32 data_len, u8 *rx_buf);
int gw3x_spi_write_bytes(struct gf_device *gf_dev, u16 addr,
		u32 data_len, u8 *tx_buf);
int gw3x_spi_read_byte(struct gf_device *gf_dev, u16 addr, u8 *value);
int gw3x_spi_write_byte(struct gf_device *gf_dev, u16 addr, u8 value);
int gw3x_ioctl_transfer_raw_cmd(struct gf_device *gf_dev,
		unsigned long arg, unsigned int bufsiz);
int gw3x_init_buffer(struct gf_device *gf_dev);
int gw3x_free_buffer(struct gf_device *gf_dev);
#endif /* !ENABLE_SENSORS_FPRINT_SECURE */

#endif	/* __GF_SPI_DRIVER_H */
