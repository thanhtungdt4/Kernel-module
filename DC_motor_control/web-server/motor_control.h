#ifndef __MOTOR_CTRL_
#define __MOTOR_CTRL_

#include "motor_ioctl.h"

#define MOTOR1		"/dev/motor1"
#define MOTOR2		"/dev/motor2"

void go_straight(int motor_id);
void go_back(int motor_id);
void set_duty_cycle(int motor_id, int duty_cycle);
void stop_motor(int motor_id);
void enable_pwm_device(int motor_id);

#endif
