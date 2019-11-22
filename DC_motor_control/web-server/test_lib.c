#include <stdio.h>
#include <unistd.h>
#include "motor_control.h"

int main()
{
	printf("go straight\n");
	go_straight(1);
	set_duty_cycle(1, 5000000);
	enable_pwm_device(1);
	sleep(20);
	
	printf("decrease speed\n");
	set_duty_cycle(1, 1500000);
	sleep(20);

	printf("go back\n");
	go_back(1);
	sleep(20);

	printf("stop\n");
	stop_motor(1);

	return 0;
}
