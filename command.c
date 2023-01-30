#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <evl/evl.h>

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
    char *buf = malloc(UBUFFER_SIZE);
    int ret = 0;

    if (!buf) {
        printf("Error allocating memory for mpu write\n");
        ret = -1;
    }

    buf[0] = com_flag;
    buf[1] = address;
    buf[2] = conf_value;
    if (write(fd, buf, UBUFFER_SIZE) < 0) {
        printf("Error writing to fcm_file\n");
        ret = -2;
    }

    free(buf);
    return ret;
}

int gpio_write(int fd, char pin, char direction, char value)
{
    char com_flag = GPIO_CONFIG_FLAG;
    char *buf = malloc(UBUFFER_SIZE);
    int ret = 0;

    if (!buf) {
        printf("Error allocating memmory for gpio write\n");
        ret = -1;
    }

    buf[0] = com_flag;
    buf[1] = pin;
    buf[2] = direction;
    buf[3] = value;
    if (write(fd, buf, UBUFFER_SIZE) < 0) {
        printf("Error writing to fcm file\n");
        ret = -2;
    }

    free(buf);
    return ret;
}

int motor_initialization(int fd, char seconds,
                                 char max_cycle_ms,
                                 char max_duty_ms,
                                 char min_duty_ms,
                                 char gpio_pin)
{
    char com_flag = MOTOR_CONFIG_FLAG;
    char *buf = malloc(UBUFFER_SIZE);
    int ret = 0;

    if (!buf) {
        printf("Error allocating memmory for motor_initialization\n");
        ret = -1;
    }

    buf[0] = com_flag;
    buf[1] = seconds;
    buf[2] = max_cycle_ms;
    buf[3] = max_duty_ms;
    buf[4] = min_duty_ms;
    buf[5] = gpio_pin;
    if (oob_write(fd, buf, UBUFFER_SIZE) < 0) {
        printf("Error writing to fcm file\n");
        ret = -2;
    }

    free(buf);
    return ret;
}

int motor_control(int fd, char direction, int percent)
{
    char com_flag = MOTOR_CONFIG_FLAG;
    char *buf = malloc(UBUFFER_SIZE);
    int ret = 0;

    if (percent > 100 || percent < 0) {
        printf("Error percentage out of range\n");
        ret = -1;
    }

    buf[0] = com_flag;
    buf[1] = direction;
    buf[2] = (char) percent;

    if (write(fd, buf, UBUFFER_SIZE)) {
        printf("Error writing to fcm file\n");
        ret = -2;
    }

    free(buf);
    return ret;
}

/**
*@brief read 16-bit data word from the mpu register given by address
*
*@param address the mpu register address to read
*@param com_flag specifies what data should be read
*/
uint16_t mpu_read(int fd, char address)
{
    char com_flag;
    return 0;
}