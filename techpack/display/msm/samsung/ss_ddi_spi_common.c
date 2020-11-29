/*
 * =================================================================
 *
 *
 *	Description:  samsung display common file
 *
 *	Author: samsung display driver team
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */

#include "ss_ddi_spi_common.h"

#define SPI_CTRL_RX 0x00

int ss_spi_write(struct spi_device *spi, int tx_bpw, u8 *tx_buf, int tx_size)
{
	u64 i;
	u8 *tbuf = NULL;
	struct samsung_display_driver_data *vdd = NULL;
	struct spi_message msg;
	int ret = 0;

	struct spi_transfer xfer[] = {
		{ .bits_per_word = tx_bpw,	.len = tx_size,	},
	};

	if (!spi) {
		LCD_ERR("no spi device..\n");
		return -EINVAL;
	}

	vdd = container_of(spi->dev.driver,
				struct samsung_display_driver_data,
				spi_driver.driver);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return -ENODEV;
	}

	mutex_lock(&vdd->ss_spi_lock);

	tbuf = kmalloc(tx_size, GFP_KERNEL | GFP_DMA);
	if (tbuf == NULL) {
		LCD_ERR("fail to alloc tx_buf..\n");
		goto err;
	}
	memcpy(tbuf, tx_buf, tx_size);
	xfer[0].tx_buf = tbuf;

	LCD_DEBUG("tx len = %d\n", xfer[0].len);

	if (vdd->ddi_spi_status == DDI_SPI_SUSPEND) {
		LCD_DEBUG("ddi spi is suspend..\n");
		ret = -EINVAL;
		goto err;
	}

	LCD_DEBUG("++\n");

	spi_message_init(&msg);

	for (i = 0; i < ARRAY_SIZE(xfer); i++)
		spi_message_add_tail(&xfer[i], &msg);

	ret = spi_sync(spi, &msg);
	if (ret) {
		pr_err("[spi] %s : spi_sync fail..\n", __func__);
		goto err;
	}

err:
	mutex_unlock(&vdd->ss_spi_lock);

	if (tbuf)
		kfree(tbuf);

	LCD_DEBUG("--\n");

	return ret;
}

int ss_spi_read(struct spi_device *spi, u8 *buf,
				int tx_bpw, int rx_bpw, u8 *tx_buf, int tx_size, int rx_size)
{
	u64 i;
	u8 *rbuf = NULL;
	u8 *tbuf = NULL;
#if defined(CONFIG_SEC_FACTORY)
	u8 *dummy_buf = NULL;
#endif
	struct samsung_display_driver_data *vdd = NULL;
	struct spi_message msg;
	int ret = 0;

	struct spi_transfer xfer[] = {
		{ .bits_per_word = tx_bpw,	.len = tx_size,	},
		{ .bits_per_word = rx_bpw,	.len = rx_size,	},
#if defined(CONFIG_SEC_FACTORY)
		{ .bits_per_word = rx_bpw,	.len = 1,		},
#endif
	};

	if (!spi) {
		LCD_ERR("no spi device..\n");
		return -EINVAL;
	}

	vdd = container_of(spi->dev.driver,
				struct samsung_display_driver_data,
				spi_driver.driver);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return -ENODEV;
	}

	mutex_lock(&vdd->ss_spi_lock);

	tbuf = kmalloc(tx_size, GFP_KERNEL | GFP_DMA);
	if (tbuf == NULL) {
		LCD_ERR("fail to alloc tx_buf..\n");
		goto err;
	}
	memcpy(&tbuf[0], &tx_buf[0], tx_size);
	xfer[0].tx_buf = tbuf;

	rbuf = kmalloc(rx_size, GFP_KERNEL | GFP_DMA);
	if (rbuf == NULL) {
		LCD_ERR("fail to alloc rx_buf..\n");
		goto err;
	}
	xfer[1].rx_buf = rbuf;

#if defined(CONFIG_SEC_FACTORY)
	dummy_buf = kmalloc(xfer[2].len, GFP_KERNEL | GFP_DMA);
	if (dummy_buf == NULL) {
		LCD_ERR("fail to alloc dummy_buf..\n");
		goto err;
	}
	xfer[2].rx_buf = dummy_buf;
#endif

	if (vdd->ddi_spi_status == DDI_SPI_SUSPEND) {
		LCD_DEBUG("ddi spi is suspend..\n");
		ret = -EINVAL;
		goto err;
	}

	LCD_DEBUG("++\n");

	LCD_DEBUG("tx_size(%d) (%02x %02x %02x %02x) \n", xfer[0].len,
		tbuf[0], tbuf[1],tbuf[2], tbuf[3]);
	LCD_DEBUG("rx_size(%d)\n", xfer[1].len);

	spi_message_init(&msg);

	for (i = 0; i < ARRAY_SIZE(xfer); i++)
		spi_message_add_tail(&xfer[i], &msg);

	ret = spi_sync(spi, &msg);
	if (ret) {
		pr_err("[spi] %s : spi_sync fail..\n", __func__);
		goto err;
	}

	memcpy(buf, rbuf, rx_size);

	if (vdd->ddi_spi_cs_high_gpio_for_gpara > 0) {
		LCD_INFO("%s wait \n", dev_name(spi->controller->dev.parent));

		/* max wait for 4 second */
		for (i = 0; i < 200; i++) {
			if (pm_runtime_status_suspended(spi->controller->dev.parent))
				break;

			msleep(20);
		}

		if (!pm_runtime_status_suspended(spi->controller->dev.parent)) {
			LCD_INFO("%s is not suspend for 4second\n", dev_name(spi->controller->dev.parent));

			pm_runtime_barrier(spi->controller->dev.parent);

			LCD_INFO("%s suspend status : %d \n", dev_name(spi->controller->dev.parent),
				pm_runtime_status_suspended(spi->controller->dev.parent));
		}

		LCD_INFO("%s end \n", dev_name(spi->controller->dev.parent));
	}

err:
	mutex_unlock(&vdd->ss_spi_lock);

	if (rbuf)
		kfree(rbuf);
	if (tbuf)
		kfree(tbuf);
#if defined(CONFIG_SEC_FACTORY)
	if (dummy_buf)
		kfree(dummy_buf);
#endif

	LCD_DEBUG("--\n");

	return ret;
}

int ss_spi_sync(struct spi_device *spi, u8 *buf, enum spi_cmd_set_type type)
{
	struct samsung_display_driver_data *vdd = NULL;
	struct ddi_spi_cmd_set *cmd_set = NULL;
	struct spi_message msg;
	u8 *rbuf = NULL;
	int ret = 0;

	struct spi_transfer tx_xfer = { .tx_buf = NULL };
	struct spi_transfer rx_xfer = { .rx_buf = NULL };

	if (!spi) {
		LCD_ERR("no spi device..\n");
		return -EINVAL;
	}

	vdd = container_of(spi->dev.driver,
				struct samsung_display_driver_data,
				spi_driver.driver);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return -ENODEV;
	}

	mutex_lock(&vdd->ss_spi_lock);

	cmd_set = ss_get_spi_cmds(vdd, type);
	if (cmd_set == NULL) {
		LCD_ERR("cmd_set is null..\n");
		ret = -EINVAL;
		goto err;
	}

	spi_message_init(&msg);

	/* tx xfer */
	if (cmd_set->tx_size) {
		/* set address when cmd is write/erase cmd */
		if (cmd_set->tx_buf[0] == SPI_PAGE_PROGRAM ||
			cmd_set->tx_buf[0] == SPI_SECTOR_ERASE_CMD ||
			cmd_set->tx_buf[0] == SPI_32K_ERASE_CMD ||
			cmd_set->tx_buf[0] == SPI_64K_ERASE_CMD) {
			cmd_set->tx_buf[1] = (cmd_set->tx_addr & 0xFF0000) >> 16;
			cmd_set->tx_buf[2] = (cmd_set->tx_addr & 0x00FF00) >> 8;
			cmd_set->tx_buf[3] = (cmd_set->tx_addr & 0x0000FF);
		}

		tx_xfer.tx_buf = cmd_set->tx_buf;
		tx_xfer.len = cmd_set->tx_size;
		tx_xfer.bits_per_word = cmd_set->tx_bpw;
		spi_message_add_tail(&tx_xfer, &msg);
	} else {
		LCD_ERR("No tx_size (%d)\n", cmd_set->tx_size);
		ret = -EINVAL;
		goto err;
	}

	/* rx xfer */
	if (cmd_set->rx_size) {
		rbuf = kmalloc(cmd_set->rx_size, GFP_KERNEL | GFP_DMA);
		if (rbuf == NULL) {
			LCD_ERR("fail to alloc rx_buf..\n");
			ret = -ENODEV;
			goto err;
		}

		if (type == RX_DATA && (cmd_set->tx_size == 4)) {
			/* set address when cmd is read cmd */
			cmd_set->tx_buf[1] = (cmd_set->rx_addr & 0xFF0000) >> 16;
			cmd_set->tx_buf[2] = (cmd_set->rx_addr & 0x00FF00) >> 8;
			cmd_set->tx_buf[3] = (cmd_set->rx_addr & 0x0000FF);
		}

		rx_xfer.rx_buf = rbuf;
		rx_xfer.len = cmd_set->rx_size;
		rx_xfer.bits_per_word = cmd_set->rx_bpw;
		spi_message_add_tail(&rx_xfer, &msg);
	}

	if (vdd->ddi_spi_status == DDI_SPI_SUSPEND) {
		LCD_DEBUG("ddi spi is suspend..\n");
		ret = -EINVAL;
		goto err;
	}

	ret = spi_sync(spi, &msg);
	if (ret) {
		pr_err("[spi] %s : spi_sync fail..\n", __func__);
		goto err;
	}

	if (cmd_set->rx_size)
		memcpy(buf, rbuf, cmd_set->rx_size);

err:
	mutex_unlock(&vdd->ss_spi_lock);

	if (rbuf)
		kfree(rbuf);

	return ret;
}

struct ddi_spi_cmd_set *ss_get_spi_cmds(struct samsung_display_driver_data *vdd,
	enum spi_cmd_set_type type)
{
	struct ddi_spi_cmd_set *cmd_set = NULL;

	if (type >= SS_SPI_CMD_SET_MAX) {
		LCD_ERR("type is not valid.. %d/%d\n", type, SS_SPI_CMD_SET_MAX);
		return NULL;
	}

	cmd_set = &vdd->spi_cmd_set[type];

	return cmd_set;
}

static char *spi_cmd_set_map[SS_SPI_CMD_SET_MAX] = {
	"samsung,flash_spi_wr_enable",
	"samsung,flash_spi_wr_page_program",
	"samsung,flash_spi_wr_status_reg1",
	"samsung,flash_spi_wr_status_reg2",
	"samsung,flash_spi_wr_status_reg1_end",
	"samsung,flash_spi_wr_status_reg2_end",
	"samsung,flash_spi_er",
	"SS_SPI_RX_START",
	"samsung,flash_spi_rd_data",
	"samsung,flash_spi_rd_status_reg1",
	"samsung,flash_spi_rd_manufacture_id",
};

void ss_panel_parse_spi_cmd(struct device_node *np,
		struct samsung_display_driver_data *vdd)
{
	int i,j;
	const char *data;
	struct ddi_spi_cmd_set *cmd_set;
	int len;

	vdd->spi_cmd_set = kzalloc(sizeof(struct ddi_spi_cmd_set) * SS_SPI_CMD_SET_MAX, GFP_KERNEL);
	if (vdd->spi_cmd_set == NULL) {
		LCD_ERR("fail to kmalloc for vdd->spi_cmd_set..\n");
		goto err;
	}

	for (i = 0; i < SS_SPI_CMD_SET_MAX; i++) {
		data = of_get_property(np, spi_cmd_set_map[i], &len);
		if (!data || !len) {
			LCD_ERR("Unable to read table %s %d\n", spi_cmd_set_map[i], len);
			continue;
		}

		cmd_set = &vdd->spi_cmd_set[i];

		j = 0;

		/* tx */
		cmd_set->tx_bpw = data[j++];
		cmd_set->tx_size = (data[j++] << 8);
		cmd_set->tx_size |= data[j++];

		cmd_set->tx_buf = kmalloc(cmd_set->tx_size,  GFP_KERNEL | GFP_DMA);
		if (cmd_set->tx_buf == NULL) {
			LCD_ERR("fail to kmalloc for tx_buf..\n");
			goto err;
		}

		memcpy(cmd_set->tx_buf, &data[j], cmd_set->tx_size);

		j += cmd_set->tx_size;

		/* rx */
		if (i >= SS_SPI_RX_START) {
			cmd_set->rx_bpw = data[j++];
			cmd_set->rx_size = (data[j++] << 8);
			cmd_set->rx_size |= data[j++];
		}

		LCD_ERR("success to parse [%s] tx - bpw(%d) size(%d) / rx - bpw(%d) size(%d)\n",
			spi_cmd_set_map[i], cmd_set->tx_bpw, cmd_set->tx_size, cmd_set->rx_bpw, cmd_set->rx_size);
	}

err:
	return;
}


static int ss_spi_parse_dt(struct spi_device *spi_dev)
{
	struct device_node *np;
	int val, ret;

	np = spi_dev->dev.of_node;
	if (!np) {
		LCD_ERR("of_node is null..\n");
		return -ENODEV;
	}

	LCD_ERR("np name : %s\n", np->full_name);

	ret = of_property_read_u32(np, "spi-max-frequency", &val);
	if (!ret)
		spi_dev->max_speed_hz = val;
	else
		LCD_ERR("No spi-max-frequency..\n");

	ret = of_property_read_u32(np, "spi-bpw", &val);
	if (!ret)
		spi_dev->bits_per_word = val;
	else
		LCD_ERR("No spi-bpw..\n");

	ret = of_property_read_u32(np, "spi-mode", &val);
	if (!ret)
		spi_dev->mode = (val > 3) ? 0 : val;
	else
		LCD_ERR("No spi-mode..\n");

	LCD_INFO("max speed (%d), bpw (%d), mode (%d) \n",
			spi_dev->max_speed_hz, spi_dev->bits_per_word, spi_dev->mode);

	return ret;
}

void ss_set_spi_speed(struct samsung_display_driver_data *vdd, int speed)
{
	int ret = 0;

	if (!vdd || !vdd->spi_dev) {
		LCD_ERR("dev is null..\n");
		return;
	}

	vdd->spi_dev->max_speed_hz = speed;

	ret = spi_setup(vdd->spi_dev);
	if (ret < 0) {
		LCD_ERR("%s : spi_setup error (%d)\n", __func__, ret);
		return;
	}

	return;
}

static int ss_spi_probe(struct spi_device *client)
{
	struct samsung_display_driver_data *vdd;
	int ret = 0;

	if (client == NULL) {
		LCD_ERR("%s : ss_spi_probe spi is null\n", __func__);
		return -EINVAL;
	}

	vdd = container_of(client->dev.driver,
						struct samsung_display_driver_data,
						spi_driver.driver);

	LCD_INFO("chip select(%d), bus number(%d)\n",
			client->chip_select, client->master->bus_num);

	ret = ss_spi_parse_dt(client);
	if (ret) {
		LCD_ERR("can not parse from ddi spi dt..\n");
		return ret;
	}

	ret = spi_setup(client);
	if (ret < 0) {
		LCD_ERR("%s : spi_setup error (%d)\n", __func__, ret);
		return ret;
	}

	vdd->spi_dev = client;
	dev_set_drvdata(&client->dev, vdd);

	LCD_ERR("%s : --\n", __func__);
	return ret;
}

static int ss_spi_remove(struct spi_device *spi)
{
	LCD_ERR("%s : remove \n", __func__);
	return 0;
}

static int ddi_spi_suspend(struct device *dev)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return -ENODEV;
	}

	mutex_lock(&vdd->ss_spi_lock);
	vdd->ddi_spi_status = DDI_SPI_SUSPEND;
	LCD_DEBUG(" %d\n", vdd->ddi_spi_status);
	mutex_unlock(&vdd->ss_spi_lock);

	return 0;
}

static int ddi_spi_resume(struct device *dev)
{
	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return -ENODEV;
	}

	mutex_lock(&vdd->ss_spi_lock);
	vdd->ddi_spi_status = DDI_SPI_RESUME;
	LCD_DEBUG(" %d\n", vdd->ddi_spi_status);
	mutex_unlock(&vdd->ss_spi_lock);

	return 0;
}

static int ddi_spi_reboot_cb(struct notifier_block *nb,
			unsigned long action, void *data)
{
	struct samsung_display_driver_data *vdd = container_of(nb,
		struct samsung_display_driver_data, spi_notif);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return -ENODEV;
	}

	mutex_lock(&vdd->ss_spi_lock);
	vdd->ddi_spi_status = DDI_SPI_SUSPEND;
	LCD_ERR(" %d\n", vdd->ddi_spi_status);
	mutex_unlock(&vdd->ss_spi_lock);

	return NOTIFY_OK;
}

static const struct dev_pm_ops ddi_spi_pm_ops = {
	.suspend = ddi_spi_suspend,
	.resume = ddi_spi_resume,
};

static const struct of_device_id ddi_spi_match_table[] = {
	{   .compatible = "ss,ddi-spi",
	},
	{}
};

#if 0
static struct spi_driver ddi_spi_driver = {
	.driver = {
		.name		= DRIVER_NAME,
		.owner		= THIS_MODULE,
		.of_match_table = ddi_spi_match_table,
		.pm	= &ddi_spi_pm_ops,
	},
	.probe		= ss_spi_probe,
	.remove		= ss_spi_remove,
};

static struct notifier_block ss_spi_reboot_notifier = {
	.notifier_call = ddi_spi_reboot_cb,
};
#endif

int ss_spi_init(struct samsung_display_driver_data *vdd)
{
	int ret;
	char drivername[10];

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return -ENODEV;
	}


	if(!vdd->samsung_support_ddi_spi) {
		LCD_ERR("%s : No support for ddi spi\n", __func__);
		return 0;
	}

	if (vdd->ndx == PRIMARY_DISPLAY_NDX)
		sprintf(drivername, "ddi_spi");
	else
		sprintf(drivername, "ddi_spi%d", vdd->ndx);

	LCD_ERR("%s : ++ %s\n", __func__, drivername);

	vdd->spi_driver.driver.name = drivername;
	vdd->spi_driver.driver.owner = THIS_MODULE;
	vdd->spi_driver.driver.of_match_table = ddi_spi_match_table;
	vdd->spi_driver.driver.pm = &ddi_spi_pm_ops;
	vdd->spi_driver.probe = ss_spi_probe;
	vdd->spi_driver.remove = ss_spi_remove;
	vdd->spi_notif.notifier_call = ddi_spi_reboot_cb;
	mutex_lock(&vdd->ss_spi_lock);
	vdd->ddi_spi_status = DDI_SPI_RESUME;
	mutex_unlock(&vdd->ss_spi_lock);

	ret = spi_register_driver(&vdd->spi_driver);
	if (ret)
		LCD_ERR("%s : ddi spi register fail : %d\n", __func__, ret);

	register_reboot_notifier(&vdd->spi_notif);

	LCD_ERR("%s : --\n", __func__);
	return ret;
}
