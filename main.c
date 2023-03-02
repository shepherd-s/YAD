#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>

#include <evl/sched.h>
#include <evl/thread.h>
#include <evl/evl.h>
#include <evl/sys.h>

#include "common/common.h"
#include "command.h"
#include "server_thread.h"

extern pthread_mutex_t command_mutex;
extern char command[XBUFFER_SIZE];

/**
* @brief Takes a string in the specific commands format in ASCII or UTF-8
*           and extract the percentage as an integer
*
* @param data A character pointer containing the encoded string
*/

int main (int argc, char *argv[]) {
    int fcm_fd;
    int evl_tfd;
    int server_thr_id;
    char buf[XBUFFER_SIZE] = {0};
    int timeout = 0;
    const char *fcm_path = "/dev/fcm";

    pthread_t server_thr;
    struct sched_param param;
    
    //Open the fcm driver file
    fcm_fd = open(fcm_path, O_RDWR);
    if (fcm_fd < 0) {
        printf("Error opening fcm file\n");
        exit(fcm_fd);
    }

    //Trigger initialization secuence on motors and wait 6 seconds to finish
    motor_initialization(fcm_fd, 3, 20, 2, 1, ALL_MOTOR_GPIO);
    sleep(6);

    //Initialize the UDP server socket to listen commands
    server_thr_id = pthread_create(&server_thr, NULL, server_init_fnptr, NULL);
    if (server_thr_id) {
        printf("Error creating server thread\n");
        exit(server_thr_id);
    }

    //Set priority on this thread 
	param.sched_priority = 90;
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    //Make the current thread an evl one (Real Time)
    evl_tfd = evl_attach_self("fcm-main-thread:%d", getpid());
    if (evl_tfd < 0) {
        printf("Error %d: attaching this thread\n", evl_tfd);
        exit(evl_tfd);
    }

    pthread_mutex_lock(&command_mutex);
    command[0] = 'T';
    pthread_mutex_unlock(&command_mutex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////OOB-STAGE
    while (1) {
        pthread_mutex_lock(&command_mutex);
        copy_to((char*) &command, (char*) &buf, XBUFFER_SIZE);
        pthread_mutex_unlock(&command_mutex);

        if (command[0] != 'T') {
            oob_motor_control(fcm_fd, (unsigned long*) &buf);
            command[0] = 'T';
            timeout = 0;
        }
        else {
            timeout++;
            if (timeout > 1000) {
                buf[1] = '1';
                oob_motor_control(fcm_fd, (unsigned long*) &buf);
            }
        }
       
        evl_usleep(3000);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////END-OOB-STAGE

    //Set initial configs of the mpu
    
    /*mpu_write(fcm_fd, PWR_MGMT_1, PM_1_RESET);//reset mpu, need delay 100ms
    usleep(200000);
    mpu_write(fcm_fd, PWR_MGMT_1, 0x01);//wake up and select clock source (1 autoselect best)
    mpu_write(fcm_fd, INT_ENABLE, 0x00);//disable interrupt
    mpu_write(fcm_fd, ACCEL_CONFIG_2, 0x03);//try 0x03 if don`t work
    mpu_write(fcm_fd, SMPLRT_DIV, 0x00);//sample rate divider to 0
    mpu_write(fcm_fd, INT_PIN_CFG, 0x00);//bypass to direct access to auxiliary i2c
    mpu_write(fcm_fd, CONFIG, 0x00);
    mpu_write(fcm_fd, ACCEL_CONFIG, ACCEL_CONFIG_SET);//full scale
    mpu_write(fcm_fd, GYRO_CONFIG, GYRO_CONFIG_SET);//full scale*/

    close(fcm_fd);
    exit(EXIT_SUCCESS);
}