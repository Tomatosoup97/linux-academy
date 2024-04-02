#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#define DUMMY_CHAR_ERASE  _IO('d', 0x01)
#define DUMMY_CHAR_SIZE   _IOR('d', 0x02, size_t)

struct dummy_char {
  struct miscdevice misc;
  size_t buff_size;
  char __iomem *buff;
};

struct dummy_char_file {
  struct dummy_char *driver;
  size_t buff_size;
  char *buff;
};

static ssize_t my_dummy_char_read(
    struct file *file,
    char __user *user_buff,
    size_t size,
    loff_t *offset
) {
  struct dummy_char *priv;
  ssize_t len, off;

  printk(KERN_NOTICE "my_dummy_char_read: size=%zu, offset=%llu\n", size, *offset);

  priv = container_of(file->private_data, struct dummy_char, misc);

  off = *offset;
  len = min(priv->buff_size - off, size);
  if (len <= 0)
    return 0;

  if (copy_to_user(user_buff, priv->buff + off, len))
    return -EFAULT;

  *offset += len;
  return len;
}

static ssize_t my_dummy_char_write(
    struct file *file,
    const char __user *user_buff,
    size_t size,
    loff_t *offset
) {
  struct dummy_char *priv;
  ssize_t len, off;

  printk(KERN_NOTICE "my_dummy_char_write: size=%zu, offset=%llu\n", size, *offset);

  priv = container_of(file->private_data, struct dummy_char, misc);

  off = *offset;
  len = min(priv->buff_size - off, size);
  if (len <= 0)
    return 0;

  if (copy_from_user(priv->buff + off, user_buff, len))
    return -EFAULT;

  *offset += len;
  return len;
}

static int my_dummy_char_release(struct inode *inode, struct file *file) {
  printk(KERN_NOTICE "my_dummy_char_release\n");
  file->private_data = NULL;
  return 0;
}

static int my_dummy_char_open(struct inode *inode, struct file *file) {
  printk(KERN_NOTICE "my_dummy_char_open\n");
  return 0;
}

static long my_dummy_char_ioctl(
    struct file *file,
    unsigned int cmd,
    unsigned long arg
) {
  struct dummy_char *priv;

  printk(KERN_NOTICE "my_dummy_char_ioctl: %u\n", cmd);

  priv = container_of(file->private_data, struct dummy_char, misc);

  switch (cmd) {
    case DUMMY_CHAR_ERASE:
      memset_io(priv->buff, 0, priv->buff_size);
      break;
    case DUMMY_CHAR_SIZE:
      if (copy_to_user(
            (void __user *) arg,
            &priv->buff_size,
            sizeof(priv->buff_size)
        )
      )
        return -EFAULT;
      break;
    default:
      return -EINVAL;
  }

  return 0;
}

loff_t my_dummy_char_llseek(struct file *file, loff_t offset, int whence) {
  struct dummy_char *priv;
  loff_t new_pos;

  printk(KERN_NOTICE "my_dummy_char_llseek: offset=%lld, whence=%d\n", offset, whence);

  priv = container_of(file->private_data, struct dummy_char, misc);

  switch (whence) {
    case SEEK_SET:
      new_pos = offset;
      break;
    case SEEK_CUR:
      new_pos = file->f_pos + offset;
      break;
    case SEEK_END:
      new_pos = priv->buff_size + offset;
      break;
    default:
      return -EINVAL;
  }
  if ((new_pos < 0) || (new_pos > priv->buff_size))
    return -EINVAL;

  file->f_pos = new_pos;
  return new_pos;
}

const struct file_operations my_dummy_char_fops = {
  .owner = THIS_MODULE,
  .read = my_dummy_char_read,
  .open = my_dummy_char_open,
  .write = my_dummy_char_write,
  .release = my_dummy_char_release,
  .unlocked_ioctl = my_dummy_char_ioctl,
  .llseek = my_dummy_char_llseek,
};

static int my_dummy_char_probe(struct platform_device *pdev) {
  struct device *dev = &pdev->dev;
  struct device_node *memory_node;
  struct resource res;
  struct dummy_char *priv;
  int ret;

  dev_err(dev, "my_dummy_char_probe\n");

  priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
  if (!priv)
    return -ENOMEM;

  memory_node = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
  if (!memory_node) {
    dev_err(dev, "Unable to find a memory-region property\n");
    return -ENODEV;
  }

  ret = of_address_to_resource(memory_node, 0, &res);
  if (ret) {
    dev_err(dev, "No memory address assigned to memory region\n");
    return ret;
  }

  priv->buff_size = resource_size(&res);
  if (!priv->buff_size) {
    dev_err(dev, "Invalid memory region size\n");
    return -EINVAL;
  }

  priv->buff = devm_ioremap_resource(dev, &res);
  if (IS_ERR(priv->buff)) {
    dev_err(dev, "Unable to ioremap buffer memory\n");
    return PTR_ERR((void *) priv->buff);
  }

  priv->misc.parent = dev;
  priv->misc.name = "my-dummy";
  priv->misc.minor = MISC_DYNAMIC_MINOR;
  priv->misc.fops = &my_dummy_char_fops;

  ret = misc_register(&priv->misc);
  if (ret) {
    dev_err(dev, "Unable to register misc device\n");
    return ret;
  }

	dev_set_drvdata(dev, priv);

  return 0;
}

static int my_dummy_char_remove(struct platform_device *pdev) {
	struct device *dev = &pdev->dev;
  struct dummy_char *priv = dev_get_drvdata(dev);

  printk(KERN_NOTICE "my_dummy_char_remove: dev=%p, priv=%p, misc=%p\n", dev, priv, &priv->misc);

  misc_deregister(&priv->misc);
  return 0;
}

static const struct of_device_id my_dummy_char_of_match[] = {
  { .compatible = "my-dummy-char" },
  {},
};
MODULE_DEVICE_TABLE(of, my_dummy_char_of_match);


static struct platform_driver my_dummy_char_driver = {
  .driver = {
    .name = "my-dummy-char",
    .of_match_table = my_dummy_char_of_match,
  },
  .probe = my_dummy_char_probe,
  .remove = my_dummy_char_remove,
};

module_platform_driver(my_dummy_char_driver);

MODULE_DESCRIPTION("My dummy chardev driver for educational purposes");
MODULE_AUTHOR("Mateusz Urbańczyk <urbanczyk@google.com>");
MODULE_LICENSE("GPL");
