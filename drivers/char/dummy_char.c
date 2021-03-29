// SPDX-License-Identifier: GPL-2.0
/*
 * Dummy chardev driver for educational purposes.
 * Copyright (c) 2021 Semihalf
 * Artur Rojek <ar@semihalf.com>
 */

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#define DUMMY_CHAR_ERASE	_IO('d', 0x01)
#define DUMMY_CHAR_SIZE		_IOR('d', 0x02, size_t)

struct dummy_char {
	struct miscdevice misc;
	char __iomem *buf;
	size_t buf_size;
};

static ssize_t dummy_char_read(struct file *file, char __user *user_buf,
			       size_t size, loff_t *offset)
{
	struct dummy_char *priv;
	ssize_t off, len;

	priv = container_of(file->private_data, struct dummy_char, misc);

	off = *offset;
	len = min(priv->buf_size - off, size);
	if (len <= 0)
		return 0;

	if (copy_to_user(user_buf, priv->buf + off, len))
		return -EFAULT;

	*offset += len;

	return len;
}

static ssize_t dummy_char_write(struct file *file, const char __user *user_buf,
				size_t size, loff_t *offset)
{
	struct dummy_char *priv;
	ssize_t off, len;

	priv = container_of(file->private_data, struct dummy_char, misc);

	off = *offset;
	len = min(priv->buf_size - off, size);
	if (len <= 0)
		return 0;

	if (copy_from_user(priv->buf + off, user_buf, len))
		return -EFAULT;

	*offset += len;

	return len;
}

static int dummy_char_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;

	return 0;
}

static long dummy_char_ioctl(struct file *file, unsigned int cmd,
			     unsigned long arg)
{
	struct dummy_char *priv;

	priv = container_of(file->private_data, struct dummy_char, misc);

	switch (cmd) {
	case DUMMY_CHAR_ERASE:
		memset_io(priv->buf, 0, priv->buf_size);
		break;
	case DUMMY_CHAR_SIZE:
		if (copy_to_user((void __user *)arg, &priv->buf_size,
				 sizeof(size_t)))
			return -EFAULT;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

const struct file_operations dummy_char_fops = {
	.owner = THIS_MODULE,
	.read = dummy_char_read,
	.write = dummy_char_write,
	.release = dummy_char_release,
	.unlocked_ioctl = dummy_char_ioctl,
};

static int dummy_char_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dummy_char *priv;
	struct device_node *mem_node;
	struct resource res;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	mem_node = of_parse_phandle(dev->of_node, "memory-region", 0);
	if (!mem_node) {
		dev_err(dev, "Unable to find a memory-region property\n");
		return -ENODEV;
	}

	ret = of_address_to_resource(mem_node, 0, &res);
	if (ret) {
		dev_err(dev, "No memory address assigned to memory region\n");
		return ret;
	}

	priv->buf_size = resource_size(&res);
	if (!priv->buf_size) {
		dev_err(dev, "Invalid memory region size\n");
		return -EINVAL;
	}

	priv->buf = devm_ioremap_resource(&pdev->dev, &res);
	if (IS_ERR(priv->buf)) {
		dev_err(dev, "Unable to ioremap buffer memory\n");
		return PTR_ERR((void *)priv->buf);
	}

	priv->misc.parent = dev;
	priv->misc.name = "dummy";
	priv->misc.minor = MISC_DYNAMIC_MINOR;
	priv->misc.fops = &dummy_char_fops;

	ret = misc_register(&priv->misc);
	if (ret) {
		dev_err(dev, "Unable to register misc device\n");
		return ret;
	}

	dev_set_drvdata(dev, priv);

	return 0;
}

static int dummy_char_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dummy_char *priv = dev_get_drvdata(dev);

	misc_deregister(&priv->misc);

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
