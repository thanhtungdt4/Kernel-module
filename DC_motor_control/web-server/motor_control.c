#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "motor_control.h"

int open_file(int motor_id)
{
	int fd;

	if (motor_id == 1) {
		fd = open(MOTOR1, O_RDWR);
		if (fd < 0) {
			perror("Can not open motor1 file\n");
			return fd;
		}
	} else {
		fd = open(MOTOR2, O_RDWR);
		if (fd < 0) {
			perror("Can not open motor2 file\n");
			return fd;
		}
	}

	return fd;
}

void go_straight(int motor_id)
{
	int fd;

	fd = open_file(motor_id);
	ioctl(fd, GO_STRAIGHT);

	close(fd);
}

void go_back(int motor_id)
{
	int fd;

	fd = open_file(motor_id);
	ioctl(fd, GO_BACK);

	close(fd);
}

void set_duty_cycle(int motor_id, int duty_cycle)
{
	int fd;

	fd = open_file(motor_id);
	ioctl(fd, SET_DUTY_CYCLE, &duty_cycle);

	close(fd);
}

void stop_motor(int motor_id)
{
	int fd;

	fd = open_file(motor_id);
	ioctl(fd, STOP_MOTOR);

	close(fd);
}

void enable_pwm_device(int motor_id)
{
	int fd;

	fd = open_file(motor_id);
	ioctl(fd, ENABLE_PWM_DEV);

	close(fd);
}
