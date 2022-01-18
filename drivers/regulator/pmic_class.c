#include <linux/device.h>
#include <linux/err.h>
#include <linux/regulator/pmic_class.h>

/* CAUTION : Do not be declared as external pmic_class  */
static struct class *pmic_class;
static atomic_t pmic_dev;

struct device *pmic_device_create(void *drvdata, const char *fmt)
{
	struct device *dev;

	if (!pmic_class) {
		pr_err("Not yet created class(pmic)!\n");
		BUG();
	}

	if (IS_ERR(pmic_class)) {
		pr_err("Failed to create class(pmic) %ld\n", PTR_ERR(pmic_class));
		BUG();
	}

	dev = device_create(pmic_class, NULL, atomic_inc_return(&pmic_dev),
			drvdata, "%s", fmt);
	if (IS_ERR(dev))
		pr_err("Failed to create device %s %ld\n", fmt, PTR_ERR(dev));
	else
		pr_debug("%s : %s : %d\n", __func__, fmt, dev->devt);

	return dev;
}
EXPORT_SYMBOL(pmic_device_create);

void pmic_device_destroy(dev_t devt)
{
	pr_info("%s : %d\n", __func__, devt);
	device_destroy(pmic_class, devt);
}
EXPORT_SYMBOL(pmic_device_destroy);

static int __init pmic_class_create(void)
{
	pmic_class = class_create(THIS_MODULE, "pmic");

	if (IS_ERR(pmic_class)) {
		pr_err("Failed to create class(pmic) %ld\n", PTR_ERR(pmic_class));
		return PTR_ERR(pmic_class);
	}

	return 0;
}
arch_initcall_sync(pmic_class_create);
