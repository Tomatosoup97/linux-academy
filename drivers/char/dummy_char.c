// SPDX-License-Identifier: GPL-2.0
/*
 * Dummy chardev driver for educational purposes.
 * Copyright (c) 2021 Semihalf
 * Artur Rojek <ar@semihalf.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>

static int dummy_char_init(void)
{
	printk("Dummy char init\n");

	return 0;
}

static void dummy_char_exit(void)
{
	printk("Dummy char exit\n");
}

module_init(dummy_char_init);
module_exit(dummy_char_exit);

MODULE_DESCRIPTION("Dummy chardev driver for educational purposes");
MODULE_AUTHOR("Artur Rojek <ar@semihalf.com>");
MODULE_LICENSE("GPL");
