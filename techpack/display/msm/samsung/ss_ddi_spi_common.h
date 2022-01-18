#ifndef SAMSUNG_SPI_COMMON_H
#define SAMSUNG_SPI_COMMON_H

#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include "ss_dsi_panel_common.h"

#define SPI_PAGE_PROGRAM 0x02
#define SPI_SECTOR_ERASE_CMD 0x20
#define SPI_32K_ERASE_CMD 0x52
#define SPI_64K_ERASE_CMD 0xD8

enum DDI_SPI_STATUS {
	DDI_SPI_SUSPEND = 0,
	DDI_SPI_RESUME,
};

enum spi_cmd_set_type {
	TX_WRITE_ENABLE = 0,
	TX_WRITE_PAGE_PROGRAM,
	TX_WRITE_STATUS_REG1,
	TX_WRITE_STATUS_REG2,
	TX_WRITE_STATUS_REG1_END,
	TX_WRITE_STATUS_REG2_END,
	TX_ERASE,
	
	SS_SPI_RX_START,
	RX_DATA,
	RX_STATUS_REG1,
	RX_FLASH_MANUFACTURE_ID,
	SS_SPI_CMD_SET_MAX,
};

struct ddi_spi_cmd_set {
	u8 tx_bpw;
	u8 rx_bpw;
	u32 tx_size;
	u8 *tx_buf;
	u32 tx_addr;
	u32 rx_size;
	u32 rx_addr;
//	u8 *rx_buf;
};

struct samsung_display_driver_data;

int ss_spi_write(struct spi_device *spi, int tx_bpw, u8 *tx_buf, int tx_size);
int ss_spi_read(struct spi_device *spi, u8 *buf,
				int tx_bpw, int rx_bpw, u8 *tx_buf, int tx_size, int rx_size);

int ss_spi_sync(struct spi_device *spi, u8 *buf, enum spi_cmd_set_type type);

void ss_panel_parse_spi_cmd(struct device_node *np,
		struct samsung_display_driver_data *vdd);
struct ddi_spi_cmd_set *ss_get_spi_cmds(struct samsung_display_driver_data *vdd, 
	enum spi_cmd_set_type type);
int ss_spi_init(struct samsung_display_driver_data *vdd);
void ss_set_spi_speed(struct samsung_display_driver_data *vdd, int speed);

#endif
