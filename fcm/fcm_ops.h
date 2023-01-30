#pragma once

int init_motor_fnptr(void *args);

void set_velocity(unsigned long vel);
void init_motor(uint8_t sec_seconds, unsigned long max_cycle_time,
                unsigned long max_duty, unsigned long min_duty, uint8_t gpio);
void run_motor(void *args);