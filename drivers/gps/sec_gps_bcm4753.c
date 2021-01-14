#include <linux/init.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#if defined(CONFIG_DRV_SAMSUNG)
#include <linux/sec_class.h>
#else
extern struct class *sec_class;
#endif

static struct device *gps_dev;
static unsigned int gps_pwr_on;

#if defined(CONFIG_DRV_SAMSUNG)
#else
static atomic_t sec_gps_dev;
#endif



int check_gps_op(void)
{
	/* This pin is high when gps is working */
	int gps_is_running = gpio_get_value(gps_pwr_on);

	pr_debug("LPA : %s(%d)\n", __func__, gps_is_running);

	return gps_is_running;
}
EXPORT_SYMBOL(check_gps_op);

static struct attribute *sec_gps_attrs[] = {
	NULL,
};

static struct attribute_group sec_gps_group = {
	.attrs = sec_gps_attrs,
};

static int __init gps_bcm4753_init(void)
{
    int err = 0;
	int ret = 0;
	const char *gps_node = "samsung,exynos54xx-bcm4753";

	struct device_node *root_node = NULL;

#if defined(CONFIG_DRV_SAMSUNG)
		gps_dev = sec_device_create(NULL, "gps");
#else
		gps_dev = device_create(sec_class, NULL, atomic_inc_return(&sec_gps_dev),
            NULL, "gps");
#endif

	if (IS_ERR(gps_dev))
		pr_err("Failed to create device %s %ld\n", "gps", PTR_ERR(gps_dev));
	else
		pr_debug("%s : %s : %d\n", __func__, "gps", gps_dev->devt);

    if (IS_ERR(gps_dev)) {
        pr_err("Failed to create device(gps_dev)!\n");
    } else {
        int rc = sysfs_create_group(&gps_dev->kobj, &sec_gps_group);
        if (rc)
            pr_err("Failed to create sysfs group, %d\n",rc);
    }

	BUG_ON(!gps_dev);

	root_node = of_find_compatible_node(NULL, NULL, gps_node);
	if (!root_node) {
		WARN(1, "failed to get device node of bcm4753\n");
		ret = -ENODEV;
		goto err_sec_device_create;
	}

	gps_pwr_on = of_get_gpio(root_node, 0);
    WARN(1, "gps_pwr_on gpio pin : %d\n", gps_pwr_on);
	if (!gpio_is_valid(gps_pwr_on)) {
		WARN(1, "Invalied gpio pin : %d\n", gps_pwr_on);
		ret = -ENODEV;
		goto err_sec_device_create;
	}

    err = gpio_request(gps_pwr_on, "GPS_PWR_EN");
	if (err) {
		WARN(1, "fail to request gpio(GPS_PWR_EN) err = %d\n", err);
		ret = -ENODEV;
		goto err_sec_device_create;
	}
	gpio_direction_output(gps_pwr_on, 0);
	gpio_export(gps_pwr_on, 1);
	gpio_export_link(gps_dev, "GPS_PWR_EN", gps_pwr_on);

	return 0;

err_sec_device_create:
	sec_device_destroy(gps_dev->devt);
	return ret;
}

device_initcall(gps_bcm4753_init);
