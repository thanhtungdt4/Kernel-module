#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAGIC_NO                150
#define DEMO_CMD                _IOWR(MAGIC_NO, 0, int)

#define FILE_NAME		"/dev/demo"

int main()
{
	int fd;
	int to_kernel = 5;

	fd = open(FILE_NAME, O_RDWR);
	if (fd < 0) {
		perror("Can not open file\n");
		return fd;
	}

	ioctl(fd, DEMO_CMD, &to_kernel);

	printf("get from kernel: %d\n", to_kernel);

	return 0;
}
