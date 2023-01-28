
#include <stdint.h>

int mpu_write(int fd, char address, char conf_value);

int gpio_write(int fd, char pin, char direction, char value);

int motor_initialization(int fd, char seconds, char max_cycle_ms,
                        char max_duty_ms, char min_duty_ms, char gpio_pin);

int motor_control(int fd, char direction, int percent);

uint16_t mpu_read(int fd, char address);