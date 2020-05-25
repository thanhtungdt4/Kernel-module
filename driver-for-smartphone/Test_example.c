#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define MAGIC_NO                100
#define SET_REBOOT_CMD          _IOW(MAGIC_NO, 0, char *)

typedef struct pwm_config {
        int enable;
        int duty_cycle;
} pwm_config_t;
#define CMD_LED_ON              _IOW(MAGIC_NO, 1, pwm_config_t)
#define CMD_LED_OFF             _IOW(MAGIC_NO, 2, pwm_config_t)

#define DEV_FILE		"/dev/example"

void help(void)
{
	printf("Test_example reboot --> reboot device\n"
	       "Test_example led_on <duty value> --> turn on LED\n"
	       "Test_example led_off --> Turn off LED\n");
}

int main(int argc, char **argv)
{
	int fd;
	pwm_config_t led_off;
	pwm_config_t led_on;
	int val = 0;

	fd = open(DEV_FILE, O_RDWR);
	if (fd < 0) {
		printf("Can not open file\n");
		return fd;
	}

	if (argc == 2) {
		if (!strcmp(argv[1], "reboot")) {
			printf("Reboot device...\n");
			ioctl(fd, SET_REBOOT_CMD, "reboot");
		} else if (!strcmp(argv[1], "led_off")) {
			printf("Turn off LED\n");
			led_off.enable = 0;
			led_off.duty_cycle = 500000;
			ioctl(fd, CMD_LED_OFF, &led_off);
		} else
			help();
	} else if (argc == 3) {
		if (!strcmp(argv[1], "led_on")) {
			printf("Turn on LED\n");
			val = atoi(argv[2]);
			printf("val = %d\n", val);
			led_on.enable = 1;
			led_on.duty_cycle = val;
			ioctl(fd, CMD_LED_ON, &led_on);
		} else
			help();
	} else
		help();

	return 0;
}
