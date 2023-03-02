
#include <stdint.h>

int mpu_write(int fd, char address, char conf_value);

int motor_initialization(int fd, unsigned long seconds, unsigned long max_cycle_ms,
                        unsigned long max_duty_ms, unsigned long min_duty_ms, unsigned long gpio_pin);

int oob_motor_control(int fd, unsigned long *command);

uint16_t mpu_read(int fd, char address);