#include <asm/param.h>
#include <linux/sched.h>
#include <linux/ratelimit.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>

static int open_gy91(struct inode *inode, struct file *flip);

static ssize_t read_gy91(struct file *flip, char __user *ubuf,
                                    size_t count, loff_t *off);

static ssize_t write_gy91(struct file *flip, const char __user *ubuf,
			                                size_t count, loff_t *off);

static int close_gy91(struct inode *inode, struct file *flip);