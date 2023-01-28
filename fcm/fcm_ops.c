#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/timekeeping.h>
#include <linux/delay.h>

#include <evl/thread.h>

#include "../common/common.h"
#include "fcm_ops.h"

static ktime_t normalization;
static ktime_t duty_time;
static unsigned long norm;
static unsigned long velocity;

static int run = 0;

void (*init_motor_fnptr) (void *args)
{
    init_motor((uint8_t) args[1], 
                (unsigned long) args[2] * 1000000,
                (unsigned long) args[3] * 1000000,
                (unsigned long) args[4] * 1000000, 
                (uint8_t) args[5]);
}

void set_velocity(unsigned long vel)
{
    ns_to_ktime((u64)vel * norm);
    velocity = vel;
}

/**
* @brief This procedure controls the motors assuming the are attached
* to an esc by simulating pwm on a gpio pin. 
*/
void run_motor(void *args)
{
    run = 1;
    while (run) {

    }

}

void stop_motor(void) {
    run = 0;
}

/**
* @brief This procedure simulates the esc initialization secuence.
* 
* @param sec_seconds The duration in seconds of each secuence. An usual
* value is 3 seconds.
*
* @param max_cycle_time The time in nanoseconds of the total pwm cycle.
* an usual value is 20000000, 20 ms.
*
* @param max_duty_percent The maximum duty time within the max cycle time in percentage.
* an usual value is 10%. This represents the max velcity.
*
* @nice              2ms(mdp)          20ms(mct)
*                  __|                 |
*            0ms->|  |_________________|<--shape of the wave at max velocity with usual values.
*/
void init_motor(uint8_t sec_seconds, unsigned long max_cycle_time, unsigned long max_duty,
                unsigned long min_duty, uint8_t gpio)
{
    unsigned long mct;
    unsigned long maxd;
    unsigned long mind;

    unsigned long s_wait;
    unsigned long l_wait;

    time64_t init_time;
    time64_t sec_count;

    gpio_request(gpio, "motor_gpio");
    gpio_direction_output(gpio, 0);

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
    normalization = ns_to_ktime((u64)norm);

    set_velocity(100);
    s_wait = velocity * norm + mind;
    l_wait = mct - s_wait;
    init_time = ktime_get_seconds();
    sec_count = 0;
    printk("vel 100 s_wait ---> %lu\n", s_wait);
    printk("vel 100 l_wait ---> %lu\n", l_wait);
    while (sec_count <= sec_seconds) {
        gpio_set_value(gpio, 1);
        ndelay(s_wait);
        gpio_set_value(gpio, 0);
        ndelay(l_wait);
        sec_count = ktime_get_seconds() - init_time;
    }

    set_velocity(0);
    s_wait = velocity * norm + mind;
    l_wait = mct - s_wait;
    init_time = ktime_get_seconds();
    sec_count = 0;
    printk("vel 0 s_wait ---> %lu\n", s_wait);
    printk("vel 0 l_wait ---> %lu\n", l_wait);
    while (sec_count <= sec_seconds) {
        gpio_set_value(gpio, 1);
        ndelay(s_wait);
        gpio_set_value(gpio, 0);
        ndelay(l_wait);
        sec_count = ktime_get_seconds() - init_time;
    }

    mdelay(1000);
    set_velocity(50);
    s_wait = velocity * norm + mind;
    l_wait = mct - s_wait;
    init_time = ktime_get_seconds();
    sec_count = 0;
    printk("vel 0 s_wait ---> %lu\n", s_wait);
    printk("vel 0 l_wait ---> %lu\n", l_wait);
    while (sec_count <= sec_seconds*5) {
        gpio_set_value(gpio, 1);
        ndelay(s_wait);
        gpio_set_value(gpio, 0);
        ndelay(l_wait);
        sec_count = ktime_get_seconds() - init_time;
    }

    mdelay(1000);
}
EXPORT_SYMBOL(init_motor);
