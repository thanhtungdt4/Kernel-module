#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

#define FILE_PATH    "/dev/bt_device"

int main()
{
	int fd;
	int act;
	char buffer[64];
	struct pollfd fds[1];

	fd = open(FILE_PATH, O_RDONLY);
	if (fd < 0) {
		perror("Open failed\n");
		return fd;
	}

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	while (1) {
		printf("Start to read bluetooth data..\n");
		act = poll(fds, 1, -1);
		if (act == -1) {
			perror("poll failed\n");
			return act;
		}

		if (fds[0].revents & POLLIN) {
            memset(buffer, 0, sizeof(buffer));
			if (read(fd, buffer, sizeof(buffer)) > 0) {
				printf("message: %s\n", buffer);
			} else {
				perror("Cannot read message\n");
			}
		}
	}

    close(fd);
    return 0;
}