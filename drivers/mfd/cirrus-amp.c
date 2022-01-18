/*
 * Extended support for CS35L41 Amp
 *
 * Copyright 2017 Cirrus Logic
 *
 * Author:      David Rhodes    <david.rhodes@cirrus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mfd/cs35l41/registers.h>
#include <linux/mfd/cs35l41/calibration.h>
#include <linux/mfd/cs35l41/big_data.h>
#include <linux/mfd/cs35l41/power.h>

#define CIRRUS_AMP_CLASS_NAME "cirrus"

struct class *cirrus_amp_class;

static int cirrus_amp_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *amp_node;
	const char **mfd_suffixes;
	const char **bd_suffixes;
	bool *v_val_separate;
	int ret, num_amps, i, j;

	if (np) {
		ret = of_property_read_u32(np, "cirrus,num-amps", &num_amps);

		if (ret < 0) {
			dev_err(&pdev->dev,
				"cirrus_amp: failed to parse num-amps\n");
			return -EINVAL;
		}

		mfd_suffixes = kzalloc(num_amps * sizeof(char *), GFP_KERNEL);
		if (mfd_suffixes == NULL) {
			dev_err(&pdev->dev,
				"Failed to allocate\n");
			return -ENOMEM;
		}

		bd_suffixes = kzalloc(num_amps * sizeof(char *), GFP_KERNEL);
		if (bd_suffixes == NULL) {
			dev_err(&pdev->dev,
				"Failed to allocate\n");
			return -ENOMEM;
		}

		v_val_separate = kzalloc(num_amps * sizeof(bool), GFP_KERNEL);
		if (v_val_separate == NULL) {
			dev_err(&pdev->dev,
				"Failed to allocate\n");
			return -ENOMEM;
		}

		for (i = 0; i < num_amps; i++) {
			amp_node = of_parse_phandle(np, "cirrus,amps", i);
			dev_dbg(&pdev->dev, "Found linked amp: %s\n",
					amp_node->full_name);

			ret = of_property_read_string(amp_node,
						"cirrus,mfd-suffix",
						&mfd_suffixes[i]);
			if (ret < 0)
				dev_err(&pdev->dev,
					"No MFD suffix found for amp: %s\n",
					amp_node->full_name);

			ret = of_property_read_string(amp_node,
						"cirrus,bd-suffix",
						&bd_suffixes[i]);
			if (ret < 0)
				dev_dbg(&pdev->dev,
					"No BD suffix found for amp: %s\n",
					amp_node->full_name);

			v_val_separate[i] = of_property_read_bool(amp_node,
						"cirrus,v-val-separate");

			of_node_put(amp_node);
		}

		for (i = 0; i < num_amps; i++) {
			for (j = 0; j < num_amps; j++) {
				if (mfd_suffixes[i] == NULL ||
						mfd_suffixes[j] == NULL) {
					dev_err(&pdev->dev,
					  "MFD suffixes incomplete\n");
					kfree(mfd_suffixes);
					return -EINVAL;
				}

				if (strcmp(mfd_suffixes[i],
						mfd_suffixes[j]) == 0 &&
					i != j) {
					dev_err(&pdev->dev,
					  "MFD suffixes must be unique\n");
					dev_err(&pdev->dev,
					  "Found duplicate suffix: %s\n",
					  mfd_suffixes[i]);
					kfree(mfd_suffixes);
					return -EINVAL;
				}

				/*bd_suffixes can be empty but must be unique*/
				if (bd_suffixes[i] && bd_suffixes[j]) {
					if (strcmp(bd_suffixes[i],
							bd_suffixes[j]) == 0 &&
								i != j) {
						dev_err(&pdev->dev,
						"BD suffixes must be unique\n");
						dev_err(&pdev->dev,
						  "Found duplicate suffix: %s\n",
						  bd_suffixes[i]);
						kfree(bd_suffixes);
						kfree(mfd_suffixes);
						return -EINVAL;
					}
				}
			}

			dev_info(&pdev->dev,
				"Found MFD suffix: %s\n", mfd_suffixes[i]);
			if (bd_suffixes[i])
				dev_info(&pdev->dev,
				"Found BD suffix: %s\n", bd_suffixes[i]);
		}
	} else {
		dev_err(&pdev->dev,
			"cirrus_amp: failed to parse cirrus-amp DT\n");
		return -EINVAL;
	}

	cirrus_amp_class = class_create(THIS_MODULE,
						CIRRUS_AMP_CLASS_NAME);
        if (IS_ERR(cirrus_amp_class)) {
                dev_err(&pdev->dev, "Failed to register cirrus amp class\n");
                return -EINVAL;
        }

	cirrus_cal_init(cirrus_amp_class, num_amps, mfd_suffixes, v_val_separate);
	cirrus_bd_init(cirrus_amp_class, num_amps, mfd_suffixes, bd_suffixes);
	cirrus_pwr_init(cirrus_amp_class, num_amps, mfd_suffixes);

	kfree(mfd_suffixes);
	return 0;
}

static int cirrus_amp_remove(struct platform_device *pdev)
{
	cirrus_cal_exit();
	cirrus_bd_exit();
	cirrus_pwr_exit();

	return 0;
}

static const struct of_device_id simple_of_match[] = {
	{ .compatible = "cirrus-amp", },
	{},
};
MODULE_DEVICE_TABLE(of, simple_of_match);

static struct platform_driver cirrus_amp = {
	.driver = {
		.name = "cirrus-amp",
		.of_match_table = simple_of_match,
	},
	.probe = cirrus_amp_probe,
	.remove = cirrus_amp_remove,
};

module_platform_driver(cirrus_amp);

MODULE_AUTHOR("David Rhodes <David.Rhodes@cirrus.com>");
MODULE_DESCRIPTION("Cirrus Amp driver");
MODULE_LICENSE("GPL");

