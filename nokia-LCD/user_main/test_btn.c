#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lcd_lib.h"

#define FILENAME		"/dev/buttons"

Btn_State_t btn_state;

int main()
{
	int fd;

	fd = open(FILENAME, O_RDONLY);
	if (fd < 0) {
		perror("open: Failed\n");
		return -1;
	}

	while (1) {
		if (read(fd, &btn_state, sizeof(btn_state)) < 0) {
			perror("read: Failed\n");
			return -1;
		}
		if (btn_state == UP_STATE)
			printf("Up button pressed\n");
		else if (btn_state == DOWN_STATE)
			printf("Down button pressed\n");
		else if (btn_state == LEFT_STATE)
			printf("left button pressed\n");
		else
			printf("right button pressed\n");
		usleep(100);
	}

	return 0;
}
