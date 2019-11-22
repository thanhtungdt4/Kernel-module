#ifndef __MOTOR_IOCTL
#define __MOTOR_IOCTL

#define MAGIC_NO        100

#define SET_DUTY_CYCLE          _IOW(MAGIC_NO, 0, int)
#define GO_STRAIGHT             _IO(MAGIC_NO, 1)
#define GO_BACK                 _IO(MAGIC_NO, 2)
#define STOP_MOTOR              _IO(MAGIC_NO, 3)
#define ENABLE_PWM_DEV          _IO(MAGIC_NO, 4)

#endif
