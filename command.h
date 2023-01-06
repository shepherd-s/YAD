#include <stdint.h>

int mpu_write(int fd, char address, char conf_value);

int gpio_write(int fd, char pin, char direction, char value);

int motor_control(int fd, char direction, int percent);

uint16_t mpu_read(int fd, char address);