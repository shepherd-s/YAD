#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/timekeeping.h>
#include <linux/time.h>
#include <linux/delay.h>

#include <linux/dovetail.h>
#include <evl/thread.h>

#include "../common/common.h"
#include "motor_thread.h"

volatile unsigned long mct;
volatile unsigned long maxd;
volatile unsigned long mind;
volatile unsigned long norm;
volatile unsigned long base_velocity;

volatile struct motor_desc fl_motor;
volatile struct motor_desc fr_motor;
volatile struct motor_desc rl_motor;
volatile struct motor_desc rr_motor;

int buf_free = 1;
int t_round = 0;

//function pointer for evl_kthreads running the motors
void evl_init_motor_fnptr(void *evl_args)
{
    unsigned long *chargs;
    volatile struct motor_desc *motor;

    chargs = (unsigned long*) evl_args;

    switch (chargs[4]) {
        case FL_MOTOR_GPIO:
            motor = &fl_motor;
            break;
        case FR_MOTOR_GPIO:
            motor = &fr_motor;
            break;
        case RL_MOTOR_GPIO:
            motor = &rl_motor;
            break;
        case RR_MOTOR_GPIO:
            motor = &rr_motor;
            break;
        default:
            break;
    }

    init_motor(chargs[0], 
                chargs[1] * 1000000,
                chargs[2] * 1000000,
                chargs[3] * 1000000, 
                motor);
}

void set_velocity(unsigned long vel, volatile struct motor_desc *motor)
{
    if (vel <= MAX_VELOCITY) {
        unsigned long v = vel + motor->vel_correction;
        if (v > MAX_VELOCITY && motor->vel_correction < 0)
            v = 0;
        if (v > MAX_VELOCITY && motor->vel_correction > 0)
            v = MAX_VELOCITY;
        motor->velocity = v;
        motor->s_period = ktime_set(0, v * norm + mind);
        motor->l_period = ktime_set(0, mct - (v * norm + mind));
    }
}

void set_base_velocity(unsigned long vel)
{
    unsigned long v = vel;
    if (vel > 100) {
        v = 100;
    }
    base_velocity = v;
}

void acc_vel_correction(long correction, volatile struct motor_desc *motor)
{
    if (motor->vel_correction >= -MAX_CORRECTION && motor->vel_correction <= MAX_CORRECTION)
        motor->vel_correction += correction;
}

void move(char chill_byte, unsigned long percent_vert, char sign_x, unsigned long percent_x, 
            char sign_y, unsigned long percent_y, char sign_yaw, unsigned long percent_yaw)
{
    unsigned long adding_x;
    unsigned long substracting_x;
    unsigned long adding_y;
    unsigned long substracting_y;
    unsigned long chill = base_velocity;
    unsigned long perx = percent_x;
    unsigned long pery = percent_y;
    unsigned long add_limit = MAX_TURNING_THRESHOLD / 2 + MAX_TURNING_THRESHOLD % 2;
    unsigned long substract_limit = MAX_TURNING_THRESHOLD / 2;

    long fl_vel = base_velocity;
    long fr_vel = base_velocity;
    long rl_vel = base_velocity;
    long rr_vel = base_velocity;

    if (chill > CHILL_VELOCITY)
        chill = CHILL_VELOCITY;

    if (perx > MAX_TURNING_THRESHOLD)
        perx = MAX_TURNING_THRESHOLD;
    if (pery > MAX_TURNING_THRESHOLD)
        pery = MAX_TURNING_THRESHOLD;

    adding_x = perx / 2 + (perx % 2);
    adding_x = (adding_x > add_limit) ? add_limit : adding_x;
    substracting_x = perx / 2;
    substracting_x = (substracting_x > substract_limit) ? substract_limit : substracting_x;
    adding_y = pery / 2 + (pery % 2);
    adding_y = (adding_y > add_limit) ? add_limit : adding_y;
    substracting_y = pery / 2;
    substracting_y = (substracting_y > substract_limit) ? substract_limit : substracting_y;

    if (sign_x == '+') {
        fl_vel += adding_x;
        rl_vel += adding_x;
        fr_vel -= substracting_x;
        rr_vel -= substracting_x;
    }
    else {
        fl_vel -= substracting_x;
        rl_vel -= substracting_x;
        fr_vel += adding_x;
        rr_vel += adding_x;
    }

    if (sign_y == '+') {
        fl_vel += adding_y;
        rl_vel -= substracting_y;
        fr_vel += adding_y;
        rr_vel -= substracting_y;
    }
    else {
        fl_vel -= substracting_y;
        rl_vel += adding_y;
        fr_vel -= substracting_y;
        rr_vel += adding_y;
    }

    if (chill_byte != '1') {
        //TODO make cumulative velocity not exceed of max_threshold / 2 + 1
        //Execute vertical movement order
        set_base_velocity(percent_vert);
        set_velocity(percent_vert, &fl_motor);
        set_velocity(percent_vert, &rr_motor);
        set_velocity(percent_vert, &rl_motor);
        set_velocity(percent_vert, &fr_motor);

        //Execute turning order 
        set_velocity(fl_vel, &fl_motor);
        set_velocity(rr_vel, &rr_motor);
        set_velocity(fr_vel, &fr_motor);
        set_velocity(rl_vel, &rl_motor);

        //Execute Yaw order
        //TODO
        /*set_velocity(adding, &rr_motor);
        set_velocity(substracting, &fl_motor);
        set_velocity(adding, &rl_motor);
        set_velocity(substracting, &rr_motor);*/
        /*set_velocity(adding, &rl_motor);
        set_velocity(substracting, &fr_motor);
        set_velocity(adding, &rr_motor);
        set_velocity(substracting, &rl_motor);*/    
    }
    else {
        set_base_velocity(chill);
        set_velocity(chill, &fl_motor);
        set_velocity(chill, &fr_motor);
        set_velocity(chill, &rl_motor);
        set_velocity(chill, &rr_motor);
    }
}

void calibrate(char direction)
{
    switch (direction) {
        case CBACKWARD:
            if (t_round) {
                acc_vel_correction(-1, &rl_motor);
                acc_vel_correction(-1, &rr_motor);
                t_round = 0;
            }
            else {
                acc_vel_correction(1, &fr_motor);
                acc_vel_correction(1, &fl_motor);
                t_round = 1;
            }
            break;

        case CRIGHT:
            if (t_round) {
                acc_vel_correction(-1, &fr_motor);
                acc_vel_correction(-1, &rr_motor);
                t_round = 0;
            }
            else {
                acc_vel_correction(1, &rl_motor);
                acc_vel_correction(1, &fl_motor);
                t_round = 1;
            }
            break;

        case CLEFT:
            if (t_round) {
                acc_vel_correction(-1, &fl_motor);
                acc_vel_correction(-1, &rl_motor);
                t_round = 0;
            }
            else {
                acc_vel_correction(1, &rr_motor);
                acc_vel_correction(1, &fr_motor);
                t_round = 1;
            }
            break;

        case CYAWR:
            if (t_round) {
                acc_vel_correction(-1, &fl_motor);
                acc_vel_correction(-1, &rr_motor);
                t_round = 0;
            }
            else {
                acc_vel_correction(1, &fr_motor);
                acc_vel_correction(1, &rl_motor);
                t_round = 1;
            }
            break;

        case CYAWL:
            if (t_round) {
                acc_vel_correction(-1, &fr_motor);
                acc_vel_correction(-1, &rl_motor);
                t_round = 0;
            }
            else {
                acc_vel_correction(1, &fl_motor);
                acc_vel_correction(1, &rr_motor);
                t_round = 1;
            }
            break;

        default:
            break;
    }
}

/**
* @brief This procedure controls the motors assuming the are attached
* to an esc by simulating pwm on a gpio pin. 
*/
void run_motor(volatile struct motor_desc *motor)
{
    while (!evl_kthread_should_stop()) {
        gpiod_set_value(motor->gpio, 1);
        evl_sleep(motor->s_period);
        gpiod_set_value(motor->gpio, 0);
        evl_sleep(motor->l_period);
    }
}

/**
* @brief This procedure simulates the esc initialization secuence.
* 
* @param sec_seconds The duration in seconds of each secuence. An usual
* value is 3 seconds.
*
* @param max_cycle_time The time in nanoseconds of the total pwm cycle.
* an usual value is 20000000, 20ms.
*
* @param max_duty The maximum duty time in ns, usual value 2000000, 2ms.
*
* @param min_duty The minimum duty time in ns, usual value 1000000, 1ms.
*
* @nice              2ms(mdp)          20ms(mct)
*                  __|                 |
*            0ms->|  |_________________|   Shape of the wave at max velocity with usual values.
*
* @nice             1ms(mdp)           20ms(mct)
*                  _|                  |
*            0ms->| |__________________|   Shape of the wave at min velocity with usual values.
*/
void init_motor(unsigned long sec_seconds, unsigned long max_cycle_time,
                unsigned long max_duty, unsigned long min_duty, volatile struct motor_desc *motor)
{
    time64_t init_time = 0;
    time64_t sec_count = 0;
    buf_free = 1;

    mct = max_cycle_time;
    maxd = max_duty;
    mind = min_duty;
 
    if (max_duty > max_cycle_time) {
        maxd = max_cycle_time;
    }
    if (min_duty > max_duty) {
        mind = max_duty;
    }
    norm = (maxd - mind) / 100;

    motor->vel_correction = 0;

    init_time = ktime_get_seconds();
    set_velocity(MAX_VELOCITY, motor);
    while (sec_count < sec_seconds) {
        sec_count = ktime_get_seconds() - init_time;

        gpiod_set_value(motor->gpio, 1);
        evl_sleep(motor->s_period);

        gpiod_set_value(motor->gpio, 0);
        evl_sleep(motor->l_period);
    }

    init_time = ktime_get_seconds();
    sec_count = 0;
    set_velocity(0, motor);
    while (sec_count < sec_seconds) {
        sec_count = ktime_get_seconds() - init_time;

        gpiod_set_value(motor->gpio, 1);
        evl_sleep(motor->s_period);

        gpiod_set_value(motor->gpio, 0);
        evl_sleep(motor->l_period);
    }

    set_velocity(0, motor);
    run_motor(motor);
}
