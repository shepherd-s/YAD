/**
* Copyright (C) 2023 Shepherd.
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/slab.h>

#include <linux/dovetail.h>
#include <evl/file.h>
#include <evl/flag.h>
#include <evl/sched.h>
#include <evl/thread.h>

#include "../common/common.h"
#include "fcm_ops.h"

static char *commands;
static char *data;
static struct spi_controller *mpu9250_spi_controller;
static struct spi_device *mpu9250_spi_device;

static struct evl_kthread *fl_motor_kthread;
static struct evl_kthread *fr_motor_kthread;
static struct evl_kthread *rl_motor_kthread;
static struct evl_kthread *rr_motor_kthread;

//spi slave info
struct spi_board_info mpu9250_spi_device_info = {
    .modalias = "mpu9250_spi",
    .max_speed_hz = 20000000,
    .bus_num = BUS_NUM,
    .chip_select = 0,
    .mode = SPI_MODE_3,
};

struct fcm_private_state {
       /* ... */
       struct evl_flag eflag;
       struct evl_file efile;
};

///////////////////////////////////////////////////////////////////////////////////////////////////FOPS
//standard open and close, needs to be implemented, don't need a body
static int fcm_open(struct inode *inode, struct file *filp)
{
    struct fcm_private_state *p;
	int ret;

	p = kzalloc(sizeof(*p), GFP_KERNEL);
	if (p == NULL)
		return -ENOMEM;
	
	ret = evl_open_file(&p->efile, filp);
	if (ret) {
	   	 kfree(p);
		 return ret;		 
	}

	filp->private_data = p;
    return 0;
}

static int fcm_close(struct inode *inode, struct file *filp)
{
    struct fcm_private_state *p = filp->private_data;

	/*
	 * Flush and destroy the flag some out-of-band operation(s)
	 * might still be pending on. Some EVL thread(s) might be
	 * unblocked from oob_read() as a result of this call.
	 */
	evl_destroy_flag(&p->eflag);

	/*
	 * Synchronize with these operations. Unblocked threads will drop
	 * any pending reference on the file being released, eventually
	 * allowing this call to return.
	 */
	evl_release_file(&p->efile);

	/*
	 * Neither in-band nor out-of-band users of this file anymore
	 * for sure, we can free the per-file private context.
	 */
	kfree(p);

    return 0;
}

static ssize_t fcm_write(struct file *filp, const char __user *user_buffer, size_t user_len, loff_t *ppos)
{
    size_t ret = user_len;
    

    if (unlikely(ret > UBUFFER_SIZE)) {
        printk("fcm: Error max buffer exceeded\n");
        return -EINVAL;
    }

    if (copy_from_user(commands, user_buffer, ret)) {
        printk("fcm: Error during copy_from_user\n");
        return -EFAULT;
    }

    switch (commands[0]) {
        case MOTOR_CONFIG_FLAG:
            if (run_oob_call(init_motor_fnptr, commands)/*evl_run_kthread_on_cpu(fl_motor_kthread,
                                        0,
                                        init_motor_fnptr,
                                        commands,
                                        99,
                                        EVL_CLONE_PRIVATE,
                                        "evl_motor_init_thread")*/) {
                printk("fcm: Error running fl_motor_thread\n");
            }
        break;

        case MOTOR_RUN_FLAG:
        
        break;

        default:
        break;
    }

    return ret;
}

static ssize_t oob_fcm_write(struct file *filp, const char __user *user_buffer, size_t user_len)
{
    size_t ret = user_len;
    

    if (unlikely(ret > UBUFFER_SIZE)) {
        printk("fcm: Error max buffer exceeded\n");
        return -EINVAL;
    }

    if (copy_from_user(commands, user_buffer, ret)) {
        printk("fcm: Error during copy_from_user\n");
        return -EFAULT;
    }

    switch (commands[0]) {
        case MOTOR_CONFIG_FLAG:
            if (run_oob_call(init_motor_fnptr, commands)/*evl_run_kthread_on_cpu(fl_motor_kthread,
                                        0,
                                        init_motor_fnptr,
                                        commands,
                                        99,
                                        EVL_CLONE_PRIVATE,
                                        "evl_motor_init_thread")*/) {
                printk("fcm: Error running fl_motor_thread\n");
            }
        break;

        case MOTOR_RUN_FLAG:
        
        break;

        default:
        break;
    }

    return ret;
}

static ssize_t fcm_read(struct file *filp, char __user *user_buffer, size_t user_len, loff_t *ppos)
{
    size_t ret = user_len;
    
    if (unlikely(user_len < UBUFFER_SIZE)) {
        printk("fcm: Error not enough user buffer size\n");
        return -EINVAL;
    }

    if (copy_to_user(user_buffer, data, UBUFFER_SIZE)) {
        printk("fcm: Error during copy_to_user\n");
        return -EFAULT;
    }

    return ret;
}
///////////////////////////////////////////////////////////////////////////////////////////////////END-FOPS

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = fcm_read,
    .write = fcm_write,
    .oob_write = oob_fcm_write,
    .open = fcm_open,
    .release = fcm_close,
};

static struct miscdevice fcm_device = {
    .name = "fcm",
    .minor = MISC_DYNAMIC_MINOR,
    .mode = 0666,
    .fops = &fops,
};

//*************************************************************************************************INIT
/**
 * @brief This function is called when the module is loaded into the kernel
 */
static int __init fcm_init(void)
{
    int status;

    commands = kzalloc(UBUFFER_SIZE, GFP_KERNEL);
    data = kzalloc(UBUFFER_SIZE, GFP_KERNEL);
    if (unlikely(!data || !commands)) {
        printk("fcm: Error allocating memory for rw data\n");
	    return -ENOMEM;
    }

    //register as misc device
    status = misc_register(&fcm_device);
    if (unlikely(status)) {
        printk("fcm: Error during Register misc device\n");
        return -status;
    }

    //set gpio 7 pin for MPU chip select and 21 for BMP, they are kept
    //until this modules is removed to prevent it's modification
    gpio_request(7, "mpu9250_cs_gpio");
    gpio_request(21, "bmp280_cs_gpio");
    gpio_direction_output(7, 0);
    gpio_direction_output(21, 0);

    printk("fcm: Device registered\n");
    return 0;
}
//*************************************************************************************************END-INIT

/**
 * @brief This function is called, when the module is removed from the kernel
 */
static void __exit fcm_exit(void)
{
    printk("fcm: Device deregistered\n");

    //deregister spi device
    if (likely(mpu9250_spi_device)) {
        spi_unregister_device(mpu9250_spi_device);
    }

    //deregister the misc device module
    misc_deregister(&fcm_device);

    //free the chip select gpios
    gpio_free(7);
    gpio_free(21);

    kfree(commands);
    kfree(data);
}

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shepherd <shepherdsoft@outlook.com>");
MODULE_DESCRIPTION("Flight control module");

module_init(fcm_init);
module_exit(fcm_exit);
