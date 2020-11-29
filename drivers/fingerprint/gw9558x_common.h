#ifndef __GF_SPI_DRIVER_H
#define __GF_SPI_DRIVER_H

#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/wakelock.h>
#include <linux/spi/spidev.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/timer.h>
#include <linux/err.h>
#include <linux/spi/spi.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/version.h>
#include <linux/pm_qos.h>
#include <linux/cpufreq.h>
#ifdef ENABLE_SENSORS_FPRINT_SECURE
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_dma.h>
#include <linux/amba/bus.h>
#endif
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#define GF_IOC_MAGIC	'g'

#define G3_MCU_ID  0x12011303
#define GX_MCU_ID  0x10041306
#define GW9558_SPI_BAUD_RATE 25000000
#define TANSFER_MAX_LEN (512*1024)

#define GF_DEV_NAME "goodix_fp"
#define GF_DEV_MAJOR 0	/* assigned */
#define GF_CLASS_NAME "goodix_fp"

#ifndef ENABLE_SENSORS_FPRINT_SECURE
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
#endif

/* define commands */
#define GF_IOC_INIT             		_IOR(GF_IOC_MAGIC, 0, u8)
#define GF_IOC_RESET            		_IO(GF_IOC_MAGIC, 2)
#define GF_IOC_ENABLE_SPI_CLK   		_IOW(GF_IOC_MAGIC, 5, uint32_t)
#define GF_IOC_DISABLE_SPI_CLK  		_IO(GF_IOC_MAGIC, 6)
#define GF_IOC_ENABLE_POWER     		_IO(GF_IOC_MAGIC, 7)
#define GF_IOC_DISABLE_POWER 			_IO(GF_IOC_MAGIC, 8)
#define GF_IOC_GET_FW_INFO 				_IOR(GF_IOC_MAGIC, 11, u8)
#ifndef ENABLE_SENSORS_FPRINT_SECURE
#define GF_IOC_TRANSFER_CMD 			_IOWR(GF_IOC_MAGIC, 15, struct gf_ioc_transfer)
#define GF_IOC_TRANSFER_RAW_CMD 		_IOWR(GF_IOC_MAGIC, 16, struct gf_ioc_transfer_raw)
#else
#define GF_IOC_SET_SENSOR_TYPE 			_IOW(GF_IOC_MAGIC, 18, unsigned int)
#endif
#define GF_IOC_POWER_CONTROL 			_IOW(GF_IOC_MAGIC, 19, unsigned int)
#define GF_IOC_SPEEDUP 					_IOW(GF_IOC_MAGIC, 20, unsigned int)
#define GF_MODEL_INFO 					_IOR(GF_IOC_MAGIC, 23, unsigned int)
#define GF_IOC_RESERVED01 				_IO(GF_IOC_MAGIC, 1)
#define GF_IOC_RESERVED02 				_IO(GF_IOC_MAGIC, 3)
#define GF_IOC_RESERVED03 				_IO(GF_IOC_MAGIC, 4)
#define GF_IOC_RESERVED04 				_IO(GF_IOC_MAGIC, 10)
#define GF_IOC_RESERVED05 				_IO(GF_IOC_MAGIC, 12)
#define GF_IOC_RESERVED06 				_IOW(GF_IOC_MAGIC, 21, unsigned int)
#define GF_IOC_RESERVED07 				_IOR(GF_IOC_MAGIC, 22, unsigned int)

#define GF_IOC_MAXNR 					24 /* THIS MACRO IS NOT USED NOW... */

struct gf_device {
	dev_t devno;
	struct cdev cdev;
	struct device *fp_device;
	struct class *class;
	struct spi_device *spi;
	struct list_head device_entry;
	int device_count;
	spinlock_t spi_lock;
	u32 pwr_gpio;
	u32 reset_gpio;
	int prev_bits_per_word;
	unsigned int min_cpufreq_limit;
	struct pm_qos_request pm_qos;
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
	u32 spi_speed;
	int sensortype;
	int reset_count;
	int interrupt_count;
	bool ldo_onoff;
	bool tz_mode;
	const char *chipid;
	const char *sensor_position;
	const char *rb;
	struct wake_lock wake_lock;
	const char *btp_vdd;
	struct regulator *regulator_3p3;
	struct pinctrl *p;
	struct pinctrl_state *pins_poweron;
	struct pinctrl_state *pins_poweroff;
	const char *model_info;
};


int gw9558_get_gpio_dts_info(struct device *dev, struct gf_device *gf_dev);
void gw9558_cleanup_info(struct gf_device *gf_dev);
void gw9558_hw_power_enable(struct gf_device *gf_dev, u8 onoff);
int gw9558_spi_clk_enable(struct gf_device *gf_dev);
int gw9558_spi_clk_disable(struct gf_device *gf_dev);
void gw9558_hw_reset(struct gf_device *gf_dev, u8 delay);
void gw9558_spi_setup_conf(struct gf_device *gf_dev, u32 bits);
int gw9558_pin_control(struct gf_device *gf_dev, bool pin_set);
int gw9558_register_platform_variable(struct gf_device *gf_dev);
int gw9558_unregister_platform_variable(struct gf_device *gf_dev);
int gw9558_set_cpu_speedup(struct gf_device *gf_dev, int onoff);

#ifndef ENABLE_SENSORS_FPRINT_SECURE
int gw9558_spi_read_bytes(struct gf_device *gf_dev, u16 addr,
		u32 data_len, u8 *rx_buf);
int gw9558_spi_write_bytes(struct gf_device *gf_dev, u16 addr,
		u32 data_len, u8 *tx_buf);
int gw9558_spi_read_byte(struct gf_device *gf_dev, u16 addr, u8 *value);
int gw9558_spi_write_byte(struct gf_device *gf_dev, u16 addr, u8 value);
int gw9558_ioctl_transfer_raw_cmd(struct gf_device *gf_dev,
		unsigned long arg, unsigned int bufsiz);
int gw9558_init_buffer(struct gf_device *gf_dev);
int gw9558_free_buffer(struct gf_device *gf_dev);
#endif

#endif	/* __GF_SPI_DRIVER_H */
