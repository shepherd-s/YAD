#pragma once

struct motor_desc {
    struct gpio_desc *gpio;
    ktime_t s_period;
    ktime_t l_period;
    long vel_correction;
    unsigned long velocity; //current velocity in percentage !!! do not set directly, use set_velocity
};

extern volatile unsigned long mct;
extern volatile unsigned long maxd;
extern volatile unsigned long mind;
extern volatile unsigned long norm;
extern volatile unsigned long base_velocity;

extern volatile struct motor_desc fl_motor;
extern volatile struct motor_desc fr_motor;
extern volatile struct motor_desc rl_motor;
extern volatile struct motor_desc rr_motor;

extern int buf_free;//mutex

void evl_init_motor_fnptr(void *evl_args);

void set_velocity(unsigned long vel, volatile struct motor_desc *motor);

void set_base_velocity(unsigned long vel);

void acc_vel_correction(long correction, volatile struct motor_desc *motor);

void move(char chill_byte, unsigned long percent_vert, char sign_x, unsigned long percent_x, 
            char sign_y, unsigned long percent_y, char sign_yaw, unsigned long percent_yaw);

void calibrate(char direction);

void init_motor(unsigned long sec_seconds, unsigned long max_cycle_time,
                unsigned long max_duty, unsigned long min_duty, volatile struct motor_desc *motor);

void run_motor(volatile struct motor_desc *motor);