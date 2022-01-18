#include "fingerprint.h"
#include "gw9558x_common.h"

#ifndef ENABLE_SENSORS_FPRINT_SECURE
int gw9558_spi_read_bytes(struct gf_device *gf_dev, u16 addr,
		u32 data_len, u8 *rx_buf)
{
	struct spi_message msg;
	struct spi_transfer *xfer = NULL;
	u8 *tmp_buf = NULL;

	xfer = kzalloc(sizeof(*xfer) * 2, GFP_KERNEL);
	if (xfer == NULL)
		return -ENOMEM;

	tmp_buf = gf_dev->spi_buffer;

	spi_message_init(&msg);
	*tmp_buf = 0xF0;
	*(tmp_buf + 1) = (u8)((addr >> 8) & 0xFF);
	*(tmp_buf + 2) = (u8)(addr & 0xFF);
	xfer[0].tx_buf = tmp_buf;
	xfer[0].len = 3;
	xfer[0].delay_usecs = 5;
	spi_message_add_tail(&xfer[0], &msg);
	spi_sync(gf_dev->spi, &msg);

	spi_message_init(&msg);
	/* memset((tmp_buf + 4), 0x00, data_len + 1); */
	/* 4 bytes align */
	*(tmp_buf + 4) = 0xF1;
	xfer[1].tx_buf = tmp_buf + 4;
	xfer[1].rx_buf = tmp_buf + 4;
	xfer[1].len = data_len + 1;
	xfer[1].delay_usecs = 5;
	spi_message_add_tail(&xfer[1], &msg);
	spi_sync(gf_dev->spi, &msg);

	memcpy(rx_buf, (tmp_buf + 5), data_len);

	kfree(xfer);
	if (xfer != NULL)
		xfer = NULL;

	return 0;
}

int gw9558_spi_write_bytes(struct gf_device *gf_dev, u16 addr,
		u32 data_len, u8 *tx_buf)
{
	struct spi_message msg;
	struct spi_transfer *xfer = NULL;
	u8 *tmp_buf = NULL;

	xfer = kzalloc(sizeof(*xfer), GFP_KERNEL);
	if (xfer == NULL)
		return -ENOMEM;

	tmp_buf = gf_dev->spi_buffer;

	spi_message_init(&msg);
	*tmp_buf = 0xF0;
	*(tmp_buf + 1) = (u8)((addr >> 8) & 0xFF);
	*(tmp_buf + 2) = (u8)(addr & 0xFF);
	memcpy(tmp_buf + 3, tx_buf, data_len);
	xfer[0].len = data_len + 3;
	xfer[0].tx_buf = tmp_buf;
	xfer[0].delay_usecs = 5;
	spi_message_add_tail(&xfer[0], &msg);
	spi_sync(gf_dev->spi, &msg);

	kfree(xfer);
	if (xfer != NULL)
		xfer = NULL;

	return 0;
}

int gw9558_spi_read_byte(struct gf_device *gf_dev, u16 addr, u8 *value)
{
	struct spi_message msg;
	struct spi_transfer *xfer = NULL;

	xfer = kzalloc(sizeof(*xfer) * 2, GFP_KERNEL);
	if (xfer == NULL)
		return -ENOMEM;

	spi_message_init(&msg);
	*gf_dev->spi_buffer = 0xF0;
	*(gf_dev->spi_buffer + 1) = (u8)((addr >> 8) & 0xFF);
	*(gf_dev->spi_buffer + 2) = (u8)(addr & 0xFF);

	xfer[0].tx_buf = gf_dev->spi_buffer;
	xfer[0].len = 3;
	xfer[0].delay_usecs = 5;
	spi_message_add_tail(&xfer[0], &msg);
	spi_sync(gf_dev->spi, &msg);

	spi_message_init(&msg);
	/* 4 bytes align */
	*(gf_dev->spi_buffer + 4) = 0xF1;
	xfer[1].tx_buf = gf_dev->spi_buffer + 4;
	xfer[1].rx_buf = gf_dev->spi_buffer + 4;
	xfer[1].len = 2;
	xfer[1].delay_usecs = 5;
	spi_message_add_tail(&xfer[1], &msg);
	spi_sync(gf_dev->spi, &msg);

	*value = *(gf_dev->spi_buffer + 5);

	kfree(xfer);
	if (xfer != NULL)
		xfer = NULL;

	return 0;
}

int gw9558_spi_write_byte(struct gf_device *gf_dev, u16 addr, u8 value)
{
	struct spi_message msg;
	struct spi_transfer *xfer = NULL;

	xfer = kzalloc(sizeof(*xfer), GFP_KERNEL);
	if (xfer == NULL)
		return -ENOMEM;

	spi_message_init(&msg);
	*gf_dev->spi_buffer = 0xF0;
	*(gf_dev->spi_buffer + 1) = (u8)((addr >> 8) & 0xFF);
	*(gf_dev->spi_buffer + 2) = (u8)(addr & 0xFF);
	*(gf_dev->spi_buffer + 3) = value;

	xfer[0].tx_buf = gf_dev->spi_buffer;
	xfer[0].len = 3 + 1;
	xfer[0].delay_usecs = 5;
	spi_message_add_tail(&xfer[0], &msg);
	spi_sync(gf_dev->spi, &msg);

	kfree(xfer);
	if (xfer != NULL)
		xfer = NULL;

	return 0;
}

static int gw9558_spi_transfer_raw(struct gf_device *gf_dev, u8 *tx_buf,
		u8 *rx_buf, u32 len)
{
	struct spi_message msg;
	struct spi_transfer xfer;

	spi_message_init(&msg);
	memset(&xfer, 0, sizeof(struct spi_transfer));

	xfer.tx_buf = tx_buf;
	xfer.rx_buf = rx_buf;
	xfer.len = len;
	spi_message_add_tail(&xfer, &msg);
	spi_sync(gf_dev->spi, &msg);

	return 0;
}

int gw9558_ioctl_transfer_raw_cmd(struct gf_device *gf_dev,
		unsigned long arg, unsigned int bufsiz)
{
	struct gf_ioc_transfer_raw ioc_xraw;
	int retval = 0;
	uint32_t len;

	do {
		if (copy_from_user(&ioc_xraw, (void __user *)arg,
				sizeof(struct gf_ioc_transfer_raw)))
		{
			pr_err("Failed to copy gf_ioc_transfer_raw from user to kernel\n");
			retval = -EFAULT;
			break;
		}

		if ((ioc_xraw.len > bufsiz) || (ioc_xraw.len == 0)) {
			pr_err("request transfer length larger than maximum buffer\n");
			retval = -EINVAL;
			break;
		}

		if (ioc_xraw.read_buf == NULL || ioc_xraw.write_buf == NULL) {
			pr_err("read buf and write buf can not equal to NULL simultaneously.\n");
			retval = -EINVAL;
			break;
		}

		/* change speed and set transfer mode */
		gw9558_spi_setup_conf(gf_dev, ioc_xraw.bits_per_word);

		len = ioc_xraw.len;

		if (copy_from_user(gf_dev->tx_buf, (void __user *)ioc_xraw.write_buf,
					ioc_xraw.len)) {
			pr_err("Failed to copy gf_ioc_transfer from user to kernel\n");
			retval = -EFAULT;
			break;
		}

		gw9558_spi_transfer_raw(gf_dev, gf_dev->tx_buf, gf_dev->rx_buf, len);

		if (copy_to_user((void __user *)ioc_xraw.read_buf,
					gf_dev->rx_buf, ioc_xraw.len)) {
			pr_err("Failed to copy gf_ioc_transfer_raw from kernel to user\n");
			retval = -EFAULT;
		}

	} while (0);

	return retval;
}

int gw9558_init_buffer(struct gf_device *gf_dev)
{
	int retval = 0;
	int len = TANSFER_MAX_LEN;

	gf_dev->spi_buffer = kzalloc(len, GFP_KERNEL);
	if (NULL == gf_dev->spi_buffer) {
		pr_err("failed to allocate spi buffer\n");
		retval = -ENOMEM;
		goto alloc_failed;
	}

	gf_dev->tx_buf = kzalloc(len, GFP_KERNEL);
	if (NULL == gf_dev->tx_buf) {
		pr_err("failed to allocate raw tx buffer\n");
		retval = -ENOMEM;
		goto alloc_failed;
	}

	gf_dev->rx_buf = kzalloc(len, GFP_KERNEL);
	if (NULL == gf_dev->rx_buf) {
		kfree(gf_dev->tx_buf);
		pr_err("failed to allocate raw rx buffer\n");
		retval = -ENOMEM;
	}
alloc_failed:
	return retval;
}

int gw9558_free_buffer(struct gf_device *gf_dev)
{
	kfree(gf_dev->spi_buffer);
	kfree(gf_dev->tx_buf);
	kfree(gf_dev->rx_buf);
	return 0;
}
#endif

