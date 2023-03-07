/**
* SPDX-License-Identifier: GPL-2.0
*
* Copyright (C) 2023 Shepherd <shepherdsoft@outlook.com>.
*
* This kernel module's main task is to provide flight control by
* processing instructions via iocl and generating pwm on the gpio.
* It implements the out of band file operations of the Xenomai 4
* framework, aka EVL, to achieve real-time capabilities.
* It is aimed to work along with 4 ESCs and 4 brushless motors.
*/

//Kernel
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/printk.h>
#include <uapi/linux/gpio.h>

//Dovetail
#include <linux/dovetail.h>

//Xenomai4-evl
#include <uapi/evl/devices/gpio.h>
#include <evl/file.h>
#include <evl/flag.h>
#include <evl/sched.h>
#include <evl/thread.h>

//Project
#include "../common/common.h"
#include "motor_thread.h"

extern struct evl_kmutex evl_motors_mutex;

int armed = 0;
dev_t dev = 0;
static struct class *dev_class;
static struct cdev fcm_cdev;

char *commands;
unsigned long *data;
unsigned long *cmd_args;

static struct evl_kthread *fl_motor_kthread;
static struct evl_kthread *fr_motor_kthread;
static struct evl_kthread *rl_motor_kthread;
static struct evl_kthread *rr_motor_kthread;

static struct gpio_desc *mpu_cs;
static struct gpio_desc *bmp_cs;

//Spi slave info
struct spi_board_info mpu9250_spi_device_info = {
    .modalias = "mpu9250-spi",
    .max_speed_hz = 20000000,
    .bus_num = BUS_NUM,
    .chip_select = 0,
    .mode = SPI_MODE_3,
};

struct fcm_private_state {
       struct evl_flag eflag;
       struct evl_file efile;
};

////////////////////////////////////////////////////////////////////////Helpers
int decode_movement_data(char *datain, unsigned long *dataout)
{
    int ret = 0;

    if (datain[1] == '1') {
        dataout[1] = datain[1];
        goto end;
    }

    dataout[1] = datain[1];
    dataout[2] = 100 * datain[2] + 10 * datain[3] + datain[4] - 5328; //vert
    dataout[3] = datain[6]; //signx
    dataout[4] = 100 * datain[7] + 10 * datain[8] + datain[9] - 5328; //horx
    dataout[5] = datain[10]; //signy
    dataout[6] = 100 * datain[11] + 10 * datain[12] + datain[13] - 5328; //hory
    dataout[7] = datain[15]; //signyaw
    dataout[8] = 100 * datain[16] + 10 * datain[17] + datain[18] - 5328; //yaw


    if (dataout[2] > 100 || dataout[4] > 100 ||
        dataout[6] > 100 || dataout[8] > 100)
        return -1;

    if (((char) dataout[3] != '-' && (char) dataout[3] != '+') ||
        ((char) dataout[5] != '-' && (char) dataout[5] != '+') ||
        ((char) dataout[7] != '-' && (char) dataout[7] != '+'))
        return -1;
end:
    return ret;
}

int decode_cal_data(char *datain, char *dataout)
{
    int ret = 0;

    dataout[0] = datain[1];

    if ((char) dataout[0] != 'f' && (char) dataout[0] != 'b' &&
        (char) dataout[0] != 'r' && (char) dataout[0] != 'l' &&
        (char) dataout[0] != 'W' && (char) dataout[0] != 'w' && 
        (char) dataout[0] != 'z')
        return -1;
    
    return ret;
}
////////////////////////////////////////////////////////////////////End-Helpers

///////////////////////////////////////////////////////////////////////////FOPS
//Open with template provided by Xenomai documentation
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

	evl_init_flag(&p->eflag);
	filp->private_data = p;

	return 0;
}

static long fcm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long int ret = 0;

    switch (cmd) {
    case MOTOR_CONFIG_FLAG:
        if (copy_from_user(cmd_args, (unsigned long*) arg, XBUFFER_SIZE)) {
            pr_err("FCM: Error in oob_ioctl copy from user");
            return -EFAULT;
        }

        if (armed)
            goto warn;
        /* If there are motor kthreads running, they should stop now to avoid
         * conflicts with subsecuent evl_run_kthread calls
         */
        if (fl_motor_kthread->status) {
            evl_stop_kthread(fl_motor_kthread);
        }
        if (fr_motor_kthread->status) {
            evl_stop_kthread(fr_motor_kthread);
        }
        if (rl_motor_kthread->status) {
            evl_stop_kthread(rl_motor_kthread);
        }
        if (rr_motor_kthread->status) {
            evl_stop_kthread(rr_motor_kthread);
        }

        if (cmd_args[4] == ALL_MOTOR_GPIO) {

            /* The initialization secuence is made using spinlock and mutex
             * to make it secuential and safe
             */
            while (!buf_free) {}
            buf_free = 0;
            cmd_args[4] = FL_MOTOR_GPIO;
            ret = evl_run_kthread_on_cpu(fl_motor_kthread, 0, evl_init_motor_fnptr,
                                    cmd_args, 95, EVL_CLONE_PUBLIC, "kt-fl-motor");
            if (ret) {
                mutex_unlock(&motors_mutex);
                return -ESRCH;
            }
            fl_motor_kthread->status = 1;
            while (!buf_free) {}
            buf_free = 0;
            cmd_args[4] = FR_MOTOR_GPIO;
            ret = evl_run_kthread_on_cpu(fr_motor_kthread, 1, evl_init_motor_fnptr,
                                    cmd_args, 95, EVL_CLONE_PUBLIC, "kt-fr-motor");
            if (ret) {
                mutex_unlock(&motors_mutex);
                return -ESRCH;
            }
            fr_motor_kthread->status = 1;
            while (!buf_free) {}
            buf_free = 0;
            cmd_args[4] = RL_MOTOR_GPIO;
            ret = evl_run_kthread_on_cpu(rl_motor_kthread, 2, evl_init_motor_fnptr,
                                    cmd_args, 95, EVL_CLONE_PUBLIC, "kt-rl-motor");
            if (ret) {
                mutex_unlock(&motors_mutex);
                return -ESRCH;
            }
            rl_motor_kthread->status = 1;
            while (!buf_free) {}
            buf_free = 0;
            cmd_args[4] = RR_MOTOR_GPIO;
            ret = evl_run_kthread_on_cpu(rr_motor_kthread, 3, evl_init_motor_fnptr,
                                    cmd_args, 95, EVL_CLONE_PUBLIC, "kt-rr-motor");
            if (ret) {
                mutex_unlock(&motors_mutex);
                return -ESRCH;
            }
            rr_motor_kthread->status = 1;
        }

        armed = 1;
       
        break;

    default:
        break;
    }

    return ret;

    warn:

    pr_warn("FCM: Motors already armed");
    return ret;
}

static ssize_t fcm_read(struct file *filp, 
                        char __user *user_buffer, 
                        size_t user_len, 
                        loff_t *ppos)
{
    size_t ret = user_len;
    
    if (unlikely(user_len < XBUFFER_SIZE)) {
        pr_err("FCM: Error not enough user buffer size");
        return -EINVAL;
    }

    if (copy_to_user(user_buffer, data, XBUFFER_SIZE)) {
        pr_err("FCM: Error during copy_to_user");
        return -EFAULT;
    }

    return ret;
}

static ssize_t fcm_write(struct file *filp, 
                        const char __user *user_buffer, 
                        size_t user_len, 
                        loff_t *ppos)
{
    size_t ret = 0;

    if (unlikely(user_len > XBUFFER_SIZE)) {
        pr_err("FCM: Error max buffer exceeded");
        return -EINVAL;
    }

    if (copy_from_user(commands, user_buffer, XBUFFER_SIZE)) {
        pr_err("FCM: Error during copy_from_user");
        return -EFAULT;
    }

    return ret;
}

static long oob_fcm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    char cal_char = 'z';

    switch (cmd) {
    case MOTOR_CONTROL_FLAG:
        if (raw_copy_from_user(cmd_args, (unsigned long*) arg, XBUFFER_SIZE)) {
            pr_err("FCM: Error in oob_ioctl copy from user");
            return -EFAULT;
        }
        if (decode_movement_data((char*) cmd_args, data)) {
            pr_err("FCM: Error in oob_ioctl decode movement data");
            return -EFAULT;
        }
        
        //Evl exclusive access to motors data
        evl_lock_kmutex(&evl_motors_mutex);
        move((char) data[1], data[2], (char) data[3], data[4],
                (char) data[5],  data[6], (char) data[7], data[8]);
        evl_unlock_kmutex(&evl_motors_mutex);
        
        break;
    case MOTOR_CALIBRATION_FLAG:
        if (raw_copy_from_user(cmd_args, (unsigned long*) arg, XBUFFER_SIZE)) {
            pr_err("FCM: Error in oob_ioctl copy from user");
            return -EFAULT;
        }
        if (decode_cal_data((char*) cmd_args, (char*) &cal_char)) {
            pr_err("FCM: Error in oob_ioctl decode calibration data");
            return -EFAULT;
        }

        //Evl exclusive access to motors data
        evl_lock_kmutex(&evl_motors_mutex);
        calibrate(cal_char);
        evl_unlock_kmutex(&evl_motors_mutex);
        break;
    
    case MOTOR_KMUTEX_FLAG:
        evl_init_kmutex(&evl_motors_mutex);
        break;
    default:
        break;
    }

    return ret;
}

static ssize_t oob_fcm_read(struct file *filp, 
                            char __user *user_buffer, 
                            size_t user_len)
{
    size_t ret = 0;
    
    if (user_len < XBUFFER_SIZE) {
        pr_err("FCM: Error not enough buffer size");
        return -EINVAL;
    }

    return ret;
}

static ssize_t oob_fcm_write(struct file *filp, 
                            const char __user *user_buffer, 
                            size_t user_len)
{
    size_t ret = 0;
    
    if (user_len > XBUFFER_SIZE)
        pr_err("FCM: Too much buffer size");
        return -EINVAL;
    
    return ret;
}

//Close with template provided by Xenomai documentation
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
///////////////////////////////////////////////////////////////////////END-FOPS

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = fcm_open,
    .unlocked_ioctl = fcm_ioctl,
    .read = fcm_read,
    .write = fcm_write,
    .oob_ioctl = oob_fcm_ioctl,
    .oob_read = oob_fcm_read,
    .oob_write = oob_fcm_write,
    .release = fcm_close,
};

//*************************************************************************INIT
/**
 * @brief This function is called when the module is loaded into the kernel
 */
static int __init fcm_init(void)
{
    //Allocating major number
    if ((alloc_chrdev_region(&dev, 0, 1, "fcm_Dev")) < 0) {
        pr_err("FCM: Error cannot allocate major number");
        return -1;
    }

    //Creating cdev structure
    cdev_init(&fcm_cdev, &fops);

    //Adding character device to the system
    if((cdev_add(&fcm_cdev, dev, 1)) < 0){
        pr_err("FCM: Error cannot add the device to the system");
        goto r_class;
    }

    //Creating struct class
    if(IS_ERR(dev_class = class_create(THIS_MODULE, "fcm_class"))){
        pr_err("FCM: Error cannot create the struct class");
        goto r_class;
    }

    //Creating device
    if(IS_ERR(device_create(dev_class, NULL, dev, NULL, "fcm"))){
        pr_err("FCM: Error cannot create the Device");
        goto r_device;
    }

    //Memory allocation and resource request///////////////////////////////////
    fl_motor_kthread = kzalloc(sizeof (*fl_motor_kthread), GFP_KERNEL);
    fr_motor_kthread = kzalloc(sizeof (*fr_motor_kthread), GFP_KERNEL);
    rl_motor_kthread = kzalloc(sizeof (*rl_motor_kthread), GFP_KERNEL);
    rr_motor_kthread = kzalloc(sizeof (*rr_motor_kthread), GFP_KERNEL);
	if (fl_motor_kthread == NULL || fr_motor_kthread == NULL || 
        rl_motor_kthread == NULL || rr_motor_kthread == NULL) {
        pr_err("FCM: Error allocating memory for kthreads");
		return -ENOMEM;
    }

    commands = kzalloc(XBUFFER_SIZE, GFP_KERNEL);
    data = kzalloc(XBUFFER_SIZE, GFP_KERNEL);
    cmd_args = kzalloc(XBUFFER_SIZE, GFP_KERNEL);
    if (unlikely(data == NULL || commands == NULL || cmd_args == NULL)) {
        pr_err("FCM: Error allocating memory for data variables");
	    return -ENOMEM;
    }

    fl_motor.gpio = gpio_to_desc(FL_MOTOR_GPIO);
    fr_motor.gpio = gpio_to_desc(FR_MOTOR_GPIO);
    rl_motor.gpio = gpio_to_desc(RL_MOTOR_GPIO);
    rr_motor.gpio = gpio_to_desc(RR_MOTOR_GPIO);
    mpu_cs = gpio_to_desc(7);
    bmp_cs = gpio_to_desc(21);
    
    gpiod_direction_output(fl_motor.gpio, 0);
    gpiod_direction_output(fr_motor.gpio, 0);
    gpiod_direction_output(rl_motor.gpio, 0);
    gpiod_direction_output(rr_motor.gpio, 0);
    gpiod_direction_output(mpu_cs, 0);
    gpiod_direction_output(bmp_cs, 0);
    ///////////////////////////////////////////////////////////////////////////

    pr_info("FCM: Device registered");
    return 0;

r_device:
    class_destroy(dev_class);
r_class:
    unregister_chrdev_region(dev, 1);
    return -1;
}
//*********************************************************************END-INIT

/**
 * @brief This function is called, when the module is removed from the kernel
 */
static void __exit fcm_exit(void)
{
    //Deregister char device
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&fcm_cdev);
    unregister_chrdev_region(dev, 1);

    //Stop motors threads
    if (fl_motor_kthread->status) {
        evl_stop_kthread(fl_motor_kthread);
    }
    if (fr_motor_kthread->status) {
        evl_stop_kthread(fr_motor_kthread);
    }
    if (rl_motor_kthread->status) {
        evl_stop_kthread(rl_motor_kthread);
    }
    if (rr_motor_kthread->status) {
        evl_stop_kthread(rr_motor_kthread);
    }

    //Free gpios
    gpiod_put(fl_motor.gpio);
    gpiod_put(fr_motor.gpio);
    gpiod_put(rl_motor.gpio);
    gpiod_put(rr_motor.gpio);
    gpiod_put(mpu_cs);
    gpiod_put(bmp_cs);

    //Free memory
    kfree(fl_motor_kthread);
    kfree(fr_motor_kthread);
    kfree(rl_motor_kthread);
    kfree(rr_motor_kthread);
    kfree(cmd_args);
    kfree(commands);
    kfree(data);

    pr_info("FCM: Device deregistered\n");
}

module_init(fcm_init);
module_exit(fcm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shepherd <shepherdsoft@outlook.com>");
MODULE_DESCRIPTION("Flight control module");
MODULE_VERSION("1.0");