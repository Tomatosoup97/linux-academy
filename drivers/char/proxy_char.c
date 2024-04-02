#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

struct proxy_char {
  struct miscdevice misc;
  struct file *dev_file;
};

#define BUFFER_SIZE 32
static char buffer[BUFFER_SIZE];

static ssize_t proxy_char_read(
    struct file *file,
    char __user *user_buff,
    size_t size,
    loff_t *offset
) {
  printk(KERN_NOTICE "proxy_char_read\n");
  // TODO

  return 0;
}

static int proxy_char_release(struct inode *inode, struct file *file) {
  printk(KERN_NOTICE "proxy_char_release\n");
  file->private_data = NULL;
  return 0;
}

static int proxy_char_open(struct inode *inode, struct file *file) {
  struct file *filp = NULL;

  /* const char *path = "/home/pi/test_file"; // works */
  const char *path = "/dev/console";       // works as well
  /* const char *path = "/dev/my-dummy";      // hangs */

  ssize_t bytes_read;
  loff_t pos = 0;

  printk(KERN_NOTICE "proxy_char_open\n");

  filp = filp_open(path, O_RDWR, 0);

  if (IS_ERR(filp)) {
      printk(KERN_ERR "Failed to open file %s\n", path);
      return PTR_ERR(filp);
  }

  printk(KERN_NOTICE "opened\n");

  // Read file contents into buffer
  bytes_read = kernel_read(filp, buffer, BUFFER_SIZE, &pos);
  if (bytes_read < 0) {
      printk(KERN_ERR "Failed to read file %s\n", path);
      filp_close(filp, NULL);
      return bytes_read;
  }
  printk(KERN_NOTICE "bytes read\n");

  /* // Print file contents to kernel log */
  /* printk(KERN_NOTICE "File contents of %s:\n", path); */
  /* printk(KERN_NOTICE "%.*s\n", (int)bytes_read, buffer); */

  filp_close(filp, NULL);

  return 0;
}

const struct file_operations proxy_char_fops = {
  .owner = THIS_MODULE,
  .read = proxy_char_read,
  .open = proxy_char_open,
  .release = proxy_char_release,
};

static int proxy_char_probe(struct platform_device *pdev) {
  struct device *dev = &pdev->dev;
  struct proxy_char *priv;
  int ret;

  dev_err(dev, "proxy_char_probe\n");

  priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
  if (!priv)
    return -ENOMEM;

  priv->misc.parent = dev;
  priv->misc.name = "proxy-char";
  priv->misc.minor = MISC_DYNAMIC_MINOR;
  priv->misc.fops = &proxy_char_fops;

  ret = misc_register(&priv->misc);
  if (ret) {
    dev_err(dev, "Unable to register misc device\n");
    return ret;
  }

	dev_set_drvdata(dev, priv);

  return 0;
}

static int proxy_char_remove(struct platform_device *pdev) {
	struct device *dev = &pdev->dev;
  struct proxy_char *priv = dev_get_drvdata(dev);

  printk(KERN_NOTICE "proxy_char_remove\n");

  misc_deregister(&priv->misc);
  return 0;
}

static const struct of_device_id proxy_char_of_match[] = {
  { .compatible = "proxy-char" },
  {},
};
MODULE_DEVICE_TABLE(of, proxy_char_of_match);


static struct platform_driver proxy_char_driver = {
  .driver = {
    .name = "proxy-char",
    .of_match_table = proxy_char_of_match,
  },
  .probe = proxy_char_probe,
  .remove = proxy_char_remove,
};

module_platform_driver(proxy_char_driver);

MODULE_DESCRIPTION("Proxy char driver");
MODULE_AUTHOR("Mateusz Urbańczyk <urbanczyk@google.com>");
MODULE_LICENSE("GPL");
