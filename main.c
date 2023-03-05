/**
* SPDX-License-Identifier: GPL-2.0
*
* Copyright (C) 2023 Shepherd <shepherdsoft@outlook.com>.
*/

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

int main (int argc, char *argv[]) {
    int fcm_fd;
    int evl_tfd;
    int server_thr_id;    
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

    //Set priority on this thread and server's one
	param.sched_priority = 90;
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    pthread_setschedparam((pthread_t) &server_thr, SCHED_FIFO, &param);

    //Make the current thread an evl one (Real Time)
    evl_tfd = evl_attach_self("fcm-main-thread:%d", getpid());
    if (evl_tfd < 0) {
        printf("Error %d: attaching this thread\n", evl_tfd);
        exit(evl_tfd);
    }

    pthread_mutex_lock(&command_mutex);
    command[0] = 'T';
    pthread_mutex_unlock(&command_mutex);

    //////////////////////////////////////////////////////////////////OOB-STAGE
    oob_ioctl(fcm_fd, MOTOR_KMUTEX_FLAG, NULL);

    while (1) {
        pthread_mutex_lock(&command_mutex);

        if (command[0] != 'T') {
            if (command[0] == 'C') {
                oob_motor_cal(fcm_fd, (unsigned long*) &command);
            }
            else if (command[0] == 'V') {
                oob_motor_control(fcm_fd, (unsigned long*) &command);
            }
            command[0] = 'T';
            timeout = 0;
        }
        else {         
            if (timeout > 1000) {
                timeout = 1001;
                command[1] = '1';
                oob_motor_control(fcm_fd, (unsigned long*) &command);
            }
            timeout++;
        }
        pthread_mutex_unlock(&command_mutex);
        evl_usleep(3000);
    }
    //////////////////////////////////////////////////////////////END-OOB-STAGE

    close(fcm_fd);
    exit(EXIT_SUCCESS);
}