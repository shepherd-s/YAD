#define BUS_NUM 0
#define UBUFFER_SIZE 32
#define ACCEL16_NORM 2048

///////////////////////////////////////////////////////////////////////////////command flags
#define MPU_CONFIG_FLAG 0x00
#define ESC_CONTROL_FLAG 0x01
#define GPIO_CONFIG_FLAG 0x02
#define MOTOR_CONFIG_FLAG 0x03

///////////////////////////////////////////////////////////////////////////////word values for MPU9250
#define UC_DATA_RESET 0x41
#define PM_1_RESET 0x80
#define PM_2_ON 0x00
#define FIFO_ENABLE 0xFF
#define CONFIG_EN 0x00
#define GYRO_CONFIG_SET 0x18
#define ACCEL_CONFIG_SET 0x18
#define ACCEL_CONFIG_2_SET 0x00

//////////////////////////////////////////////////////////////////////////////adresses for MPU9250
#define WHO_AM_I 0x75
#define SIGNAL_PATH_RESET 0x68
#define USR_CTRL 0x6A
#define PWR_MGMT_1 0x6B
#define PWR_MGMT_2 0x6C
#define FIFO_EN 0x23
#define CONFIG 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define ACCEL_CONFIG_2 0x1D
#define INT_ENABLE 0x38
#define SMPLRT_DIV 0x19
#define INT_PIN_CFG 0x37

#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40
#define GYRO_XOUT_H 0x43
#define GYRO_XOUT_L 0x44
#define GYRO_YOUT_H 0x45
#define GYRO_YOUT_L 0x46
#define GYRO_ZOUT_H 0x47
#define GYRO_ZOUT_L 0x48

#define MPU_READ 0x80
#define MPU_WRITE 0x0

//////////////////////////////////////////////////////////////////////////////gpio configs
#define IN 0x00
#define OUT 0X01
#define GPIO_6 0x06 //motor front left
#define HIGH 0x01
#define LOW 0x00

//////////////////////////////////////////////////////////////////////////////motor control
#define ARM_SEQUENCE 0x00
#define FORWARD 0x01
#define REVERSE 0x02
#define VERTICAL 0x03
#define LEFT 0x04
#define RIGHT 0x05