#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

#define FILE_PATH    "/dev/light_sensor"

int main()
{
	int fd;
	int act;
	int sensor_val;
	struct pollfd fds[1];

	fd = open(FILE_PATH, O_RDONLY);
	if (fd < 0) {
		perror("Open failed\n");
		return fd;
	}

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	while (1) {
		printf("Start to read light sensor...\n");
		act = poll(fds, 1, -1);
		if (act == -1) {
			perror("poll failed\n");
			return act;
		}

		if (fds[0].revents & POLLIN) {
			if (read(fd, &sensor_val, sizeof(sensor_val)) > 0) {
				printf("Light Value: %d\n", sensor_val);
			} else {
				perror("Cannot read sensor value\n");
			}
		}
	}
}
