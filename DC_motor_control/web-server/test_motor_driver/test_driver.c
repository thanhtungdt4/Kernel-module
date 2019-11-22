#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "../motor_ioctl.h"

#define MOTOR1		"/dev/motor1"
#define MOTOR2		"/dev/motor2"

void run_motor1(int duty_cycle)
{
	int fd1;

	fd1 = open(MOTOR1, O_RDWR);
	if (fd1 < 0) {
		perror("open motor1 failed\n");
		return;
	}

	ioctl(fd1, GO_STRAIGHT);
	ioctl(fd1, SET_DUTY_CYCLE, &duty_cycle);
	ioctl(fd1, ENABLE_PWM_DEV);

	close(fd1);
}

void stop_motor(char *file_name)
{
	int fd;

	fd = open(file_name, O_RDWR);
	if (fd < 0) {
		printf("open %s failed\n", file_name);
		return;
	}

	ioctl(fd, STOP_MOTOR);

	close(fd);
}
void run_motor2(int duty_cycle)
{
	int fd2;

	fd2 = open(MOTOR2, O_RDWR);
	if (fd2 < 0) {
		perror("open motor2 failed\n");
		return;
	}

	ioctl(fd2, GO_STRAIGHT);
	ioctl(fd2, SET_DUTY_CYCLE, &duty_cycle);
	ioctl(fd2, ENABLE_PWM_DEV);

	close(fd2);
}


int main(int argc, char *argv[])
{
	if (!strcmp(argv[1], "run_motor1")) {
		run_motor1(6000000);
	}

	sleep(60);
	run_motor1(3000000);
	if (!strcmp(argv[1], "stop_motor")) {
		stop_motor(MOTOR1);
	}

	return 0;
}
