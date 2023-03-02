#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <evl/evl.h>
#include <evl/heap.h>

#include "command.h"
#include "common/common.h"

/**
*@brief write a 8-bit word to the mpu register given by address
*
*@param address the mpu register address to write
*@param conf_value the value of the 8-bit word to write
*/
int mpu_write(int fd, char address, char conf_value)
{
    char com_flag = MPU_CONFIG_FLAG;
    char *buf = malloc(XBUFFER_SIZE);
    int ret = 0;

    if (!buf) {
        printf("Error allocating memory for mpu write\n");
        ret = -1;
    }

    buf[0] = com_flag;
    buf[1] = address;
    buf[2] = conf_value;
    if (write(fd, buf, XBUFFER_SIZE) < 0) {
        printf("Error writing to fcm_file\n");
        ret = -2;
    }

    free(buf);
    return ret;
}

int motor_initialization(int fd,
                        unsigned long seconds,
                        unsigned long max_cycle_ms,
                        unsigned long max_duty_ms,
                        unsigned long min_duty_ms,
                        unsigned long gpio_pin)
{
    int ret = 0;
    unsigned long buf[XBUFFER_SIZE];

    buf[0] = seconds;
    buf[1] = max_cycle_ms;
    buf[2] = max_duty_ms;
    buf[3] = min_duty_ms;
    buf[4] = gpio_pin;

    ret = ioctl(fd, MOTOR_CONFIG_FLAG, &buf);
    if (ret < 0) {
        evl_printf("Error ioctl-writing to fcm file\n");
        return ret;
    }
    
    return ret;
}

int oob_motor_control(int fd, unsigned long *command)
{
    int ret = 0;

    if (oob_ioctl(fd, MOTOR_CONTROL_FLAG, command)) {
        printf("Error writing to fcm file\n");
        ret = -2;
    }

    return ret;
}