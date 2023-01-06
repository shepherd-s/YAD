#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include "command.h"
#include "common/common.h"

int main (int argc, char *argv[]) {
    int read_n;
    int fcm_fd;
    const char *fcm_path = "/dev/fcm";
    char *fcm_buffer = malloc(UBUFFER_SIZE);
    int select = 1;

    if (!fcm_buffer) {
        printf("Error allocating memory\n");
        exit(EXIT_FAILURE);
    }

    fcm_fd = open(fcm_path, O_RDWR);
    if (fcm_fd < 0) {
        printf("Error opening fcm file\n");
        exit(EXIT_FAILURE);
    }

    //Set initial configs of the mpu
    
    mpu_write(fcm_fd, PWR_MGMT_1, PM_1_RESET);//reset mpu, need delay 100ms
    usleep(200000);
    mpu_write(fcm_fd, PWR_MGMT_1, 0x01);//wake up and select clock source (1 autoselect best)
    mpu_write(fcm_fd, INT_ENABLE, 0x00);//disable interrupt
    mpu_write(fcm_fd, ACCEL_CONFIG_2, 0x03);//try 0x03 if don`t work
    mpu_write(fcm_fd, SMPLRT_DIV, 0x00);//sample rate divider to 0
    mpu_write(fcm_fd, INT_PIN_CFG, 0x00);//bypass to direct access to auxiliary i2c
    mpu_write(fcm_fd, CONFIG, 0x00);
    mpu_write(fcm_fd, ACCEL_CONFIG, ACCEL_CONFIG_SET);//full scale
    mpu_write(fcm_fd, GYRO_CONFIG, GYRO_CONFIG_SET);//full scale
    /*
    */


    for (int j = 0; j < 1000000; j++) {

    
    if (!read(fcm_fd, fcm_buffer, UBUFFER_SIZE)) {
        printf("Error reading fcm file\n");
    }
    for (int i = 0; i < 5; i+=2) {
        short measure = fcm_buffer[i] << 8 | fcm_buffer[i+1];
        printf("bytes %d%d -> %d\n",i, i + 1, measure/ACCEL16_NORM);
    }
    sleep(1);
    }
    /*float t = 18000;
    float dt = 1000;
    float speed = 1;
    int count = 250;

    while (count) {
        gpio_write(fcm_fd, GPIO_6, OUT, HIGH);
        usleep(1000 + speed * dt);
        gpio_write(fcm_fd, GPIO_6, OUT, LOW);
        usleep(1000 - speed * dt + t);
        count--;
    }

    count = 250;
    while (count) {
        gpio_write(fcm_fd, GPIO_6, OUT, HIGH);
        usleep(1000 + 0 * dt);
        gpio_write(fcm_fd, GPIO_6, OUT, LOW);
        usleep(1000 - 0 * dt + t);
        count--;
    }*/

    close(fcm_fd);
    free (fcm_buffer);
    exit(EXIT_SUCCESS);
}