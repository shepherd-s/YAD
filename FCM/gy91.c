/*
**********************************************************
* This program is a driver module for the Gy-91 board wich
* consist of two units:
* - 10 DOF MPU 9250 (IvenSense)
* - BMP 280 multisensor (Bosch)
*
* Author: Francisco García
* Contact: shepherdsoft@outlook.com
**********************************************************
*/

#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <asm/param.h>
#include <linux/sched.h>
#include <linux/ratelimit.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/gpio/driver.h>
#include <linux/uaccess.h>

#include "gy91.h"
#include "common.h"

#define OURMODNAME "gy91"

MODULE_AUTHOR("Francisco García");
MODULE_DESCRIPTION("Gy-91 Driver Module");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

struct drv_ctx {
	struct device *dev;
	int tx, rx, err, myword;
	u32 config1, config2;
	u64 config3;
	char word[GY91MAXBYTES];
};
static struct drv_ctx *ctx;


static int open_gy91(struct inode *inode, struct file *filp)
{
    char *buf = kzalloc(PATH_MAX, GFP_KERNEL);

	if (unlikely(!buf))
		return -ENOMEM;
			// displays process (or atomic) context info

	pr_info(" opening \"%s\" now; wrt open file: f_flags = 0x%x\n",
		file_path(filp, buf, PATH_MAX), filp->f_flags);

	kfree(buf);
	return nonseekable_open(inode, filp);
}

static ssize_t read_gy91(struct file *filp, char __user *ubuf, size_t count, loff_t *off)
{
	pr_info("to read %zd bytes\n", count);
	return count;
}

static ssize_t write_gy91(struct file *filp, const char __user *ubuf,
			     size_t count, loff_t *off)
{
	pr_info("to write %zd bytes\n", count);
	return count;
}

static int close_gy91(struct inode *inode, struct file *filp)
{
	char *buf = kzalloc(PATH_MAX, GFP_KERNEL);

	if (unlikely(!buf))
		return -ENOMEM;
	pr_info("closing \"%s\"\n", file_path(filp, buf, PATH_MAX));
	kfree(buf);

	return 0;
}

static const struct file_operations gy91_misc_fops = {
	.open = open_gy91,
	.read = read_gy91,
	.write = write_gy91,
	.release = close_gy91,                                                          
	.llseek = no_llseek,
};

static struct miscdevice gy91_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,	/* kernel dynamically assigns a free minor# */
	.name = "gy91_miscdrv",	/* when misc_register() is invoked, the kernel
				 * will auto-create device file as /dev/llkd_miscdrv ;
				 * also populated within /sys/class/misc/ and /sys/devices/virtual/misc/ */
	.mode = 0666,				/* ... dev node perms set as specified here */
	.fops = &gy91_misc_fops,	/* connect to this driver's 'functionality' */
};

static int __init miscdrv_init(void)
{
	int ret;
	struct device *dev;

	ret = misc_register(&gy91_miscdev);
	
	if (ret != 0) {
		pr_notice("misc device registration failed, aborting\n");
		return ret;
	}

	/* Retrieve the device pointer for this device */
	dev = gy91_miscdev.this_device;
	pr_info("gy91 misc driver (major # 10) registered, minor# = %d,"
		" dev node is /dev/%s\n", gy91_miscdev.minor, gy91_miscdev.name);

	dev_info(dev, "sample dev_info(): minor# = %d\n", gy91_miscdev.minor);

	ctx = devm_kzalloc(dev, sizeof(struct drv_ctx), GFP_KERNEL);
	if (unlikely(!ctx)) {
		return -ENOMEM;
	}

	ctx->dev = dev;
	strscpy(ctx->word, "h", 2);

	return 0;		/* success */
}

static void __exit miscdrv_exit(void)
{
	misc_deregister(&gy91_miscdev);
	pr_info("gy91 misc driver deregistered, bye\n");
}

module_init(miscdrv_init);
module_exit(miscdrv_exit);