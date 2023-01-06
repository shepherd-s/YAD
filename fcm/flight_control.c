/**
*@brief kernel module for flight control using
* mpu9250 through spi interface as a misc device
*
*@author Frankie
*
* 
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>

#include "../common/common.h"
#include "fcm_ops.h"

static char *commands;
static char *gy91_data;
static struct spi_master *fcm_master;
static struct spi_device *fcm_slave;

//spi slave info
struct spi_board_info fcm_slave_info = {
    .modalias = "fcm_slave",
    .max_speed_hz = 1000000,
    .bus_num = BUS_NUM,
    .chip_select = 0,
    .mode = 3,
};

///////////////////////////////////////////////////////////////////////////////////////////////////FOPS
//standard open and close, needs to be implemented, don't need a body
static int fcm_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int fcm_close(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t fcm_write(struct file *file, const char __user *user_buffer, size_t user_len, loff_t *ppos)
{
    size_t ret = user_len;
    char *buf = kmalloc(2, GFP_KERNEL);

    if (unlikely(!buf)) {
        printk("fcm: Error allocating memory for buf variable\n");
        return -ENOMEM;
    }

    if (unlikely(ret > UBUFFER_SIZE)) {
        printk("fcm: Error max buffer exceeded\n");
        return -EINVAL;
    }

    if (copy_from_user(commands, user_buffer, ret)) {
        printk("fcm: Error during copy_from_user\n");
        return -EFAULT;
    }
    
    //check what the com_flag is to perform corresponding write operation
    switch (commands[0]) {
        case MPU_CONFIG_FLAG:
            buf[0] = commands[1];
            buf[1] = commands[2];
            if (spi_write(fcm_slave, buf, 2) < 0) {
                printk("fcm: Error writing to MPU\n");
            }

            kfree(buf);
        break;

        case GPIO_CONFIG_FLAG: //pin, direction, value

            if (gpio_request((int)commands[1], "gpio-req")) {
                printk("fcm: Error requesting gpio\n");
            }

            if (commands[2] == OUT) {
                if (gpio_direction_output((int)commands[1], 0)) {
                    printk("fcm: Error setting direction gpio\n");
                }
            }

            if (commands[2] == IN) {
                    if (gpio_direction_input((int)commands[1])) {
                    printk("fcm: Error setting direction gpio\n");
                }
            }

            //set gpio value
            gpio_set_value((int)commands[1], (int)commands[3]);

            gpio_free((int)commands[1]);//see final value on module exit
        break;

        case MOTOR_CONFIG_FLAG:
            
        default:break;
    }

    return ret;
}

static ssize_t fcm_read(struct file *file, char __user *user_buffer, size_t user_len, loff_t *ppos)
{
    size_t ret = user_len;

    gy91_data[0] = spi_w8r8(fcm_slave, ACCEL_XOUT_H | MPU_READ);
    gy91_data[1] = spi_w8r8(fcm_slave, ACCEL_XOUT_L | MPU_READ);
    gy91_data[2] = spi_w8r8(fcm_slave, ACCEL_YOUT_H | MPU_READ);
    gy91_data[3] = spi_w8r8(fcm_slave, ACCEL_YOUT_L | MPU_READ);
    gy91_data[4] = spi_w8r8(fcm_slave, ACCEL_ZOUT_H | MPU_READ);
    gy91_data[5] = spi_w8r8(fcm_slave, ACCEL_ZOUT_L | MPU_READ);

    gy91_data[6] = spi_w8r8(fcm_slave, GYRO_XOUT_H | MPU_READ);
    gy91_data[7] = spi_w8r8(fcm_slave, GYRO_XOUT_L | MPU_READ);
    gy91_data[8] = spi_w8r8(fcm_slave, GYRO_YOUT_H | MPU_READ);
    gy91_data[9] = spi_w8r8(fcm_slave, GYRO_YOUT_L | MPU_READ);
    gy91_data[10] = spi_w8r8(fcm_slave, GYRO_ZOUT_H | MPU_READ);
    gy91_data[11] = spi_w8r8(fcm_slave, GYRO_ZOUT_L | MPU_READ);
    
    printk("fcm: accel z -> %x%x", gy91_data[4], gy91_data[5]);
    if (unlikely(user_len < UBUFFER_SIZE)) {
        printk("fcm: Error not enough user buffer size\n");
        return -EINVAL;
    }

    if (copy_to_user(user_buffer, gy91_data, UBUFFER_SIZE)) {
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
 * @brief This function is called, when the module is loaded into the kernel
 */
static int __init fcm_init(void)
{
    int status;

    printk("fcm: Registering misc device\n");

    commands = kzalloc(UBUFFER_SIZE, GFP_KERNEL);
    gy91_data = kzalloc(UBUFFER_SIZE, GFP_KERNEL);
    if (unlikely(!gy91_data || !commands)) {
        printk("fcm: Error allocating memory for rw data\n");
	    return -ENOMEM;
    }

    //get bus for spi master
    fcm_master = spi_busnum_to_master(fcm_slave_info.bus_num);
    if (unlikely(!fcm_master)) {
        printk("fcm: MASTER not found\n");
        return -ENODEV;
    }

    //create new spi slave 
    fcm_slave = spi_new_device(fcm_master, &fcm_slave_info);
    if (unlikely(!fcm_slave)) {
        printk("fcm: FAILED to create slave\n");
        return -ENODEV;
    }
    
    //set mandatory word length
    fcm_slave->bits_per_word = 8;
    
    //setup spi slave
    status = spi_setup(fcm_slave);
    if (unlikely(status)) {
        printk("fcm: FAILED to setup slave.\n");
        spi_unregister_device(fcm_slave);
        return -ENODEV;
    }

    status = misc_register(&fcm_device);
    if (unlikely(status)) {
        printk("fcm: Error during Register misc device\n");
        return -status;
    }

    return 0;
}
//*************************************************************************************************END-INIT

/**
 * @brief This function is called, when the module is removed from the kernel
 */
static void __exit fcm_exit(void)
{
    printk("fcm: Deregistering misc device\n");

    if (likely(fcm_slave)) {
        spi_unregister_device(fcm_slave);
    }
    misc_deregister(&fcm_device);

    kfree(commands);
    kfree(gy91_data);
}

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Frankie");
MODULE_DESCRIPTION("flight control module of YAD");

module_init(fcm_init);
module_exit(fcm_exit);
