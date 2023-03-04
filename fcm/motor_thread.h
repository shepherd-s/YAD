#pragma once

#include <linux/mutex.h>
#include <linux/ktime.h>

#include <evl/mutex.h>

struct motor_desc {
    struct gpio_desc *gpio;
    ktime_t s_period;
    ktime_t l_period;
    long vel_correction;
    unsigned long velocity; //current velocity in percentage !!! do not set directly, use set_velocity
};

extern unsigned long mct;
extern unsigned long maxd;
extern unsigned long mind;
extern unsigned long norm;
extern unsigned long base_velocity;

extern struct mutex motors_mutex;
extern struct evl_kmutex evl_motors_mutex;

extern struct motor_desc fl_motor;
extern struct motor_desc fr_motor;
extern struct motor_desc rl_motor;
extern struct motor_desc rr_motor;

extern int buf_free;//mutex

void evl_init_motor_fnptr(void *evl_args);

void set_velocity(unsigned long vel, struct motor_desc *motor);

void set_base_velocity(unsigned long vel);

void acc_vel_correction(long correction, struct motor_desc *motor);

void move(char chill_byte, unsigned long percent_vert, char sign_x, unsigned long percent_x, 
            char sign_y, unsigned long percent_y, char sign_yaw, unsigned long percent_yaw);

void calibrate(char direction);

void init_motor(unsigned long sec_seconds, unsigned long max_cycle_time,
                unsigned long max_duty, unsigned long min_duty, struct motor_desc *motor);

void run_motor(struct motor_desc *motor);