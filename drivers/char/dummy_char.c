// SPDX-License-Identifier: GPL-2.0
/*
 * Dummy chardev driver for educational purposes.
 * Copyright (c) 2021 Semihalf
 * Artur Rojek <ar@semihalf.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>

struct dummy_char {
	char buf[20];
};

static int dummy_char_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dummy_char *priv;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	strncpy(priv->buf, "Hello world!", sizeof(priv->buf));

	dev_set_drvdata(dev, priv);

	return 0;
}

static int dummy_char_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dummy_char *priv = dev_get_drvdata(dev);

	dev_info(dev, "priv->buf: %s\n", priv->buf);

	return 0;
}

static const struct of_device_id dummy_char_of_match[] = {
	{ .compatible = "dummy-char", },
	{ }
};
MODULE_DEVICE_TABLE(of, dummy_char_of_match);

static struct platform_driver dummy_char_driver = {
	.driver = {
		.name = "dummy_char",
		.of_match_table = dummy_char_of_match,
	},
	.probe = dummy_char_probe,
	.remove = dummy_char_remove,
};

module_platform_driver(dummy_char_driver);

MODULE_DESCRIPTION("Dummy chardev driver for educational purposes");
MODULE_AUTHOR("Artur Rojek <ar@semihalf.com>");
MODULE_LICENSE("GPL");
