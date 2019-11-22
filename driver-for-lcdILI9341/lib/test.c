#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../lcd_ioctl.h"

#define FILENAME	"/dev/lcdILI9341"
int main()
{
	int fd;
	int ret;
	rect_t rect;
	pixel_t pixel;

	fd = open(FILENAME, O_RDWR);
	if (fd < 0) {
		perror("open: Failed\n");
		return fd;
	}

	/*rect.x = 20;
	rect.y = 30;
	rect.w = 40;
	rect.h = 50;
	rect.color = 100;*/
	pixel.x = 50;
	pixel.y = 50;
	pixel.color = 120;

	ret = ioctl(fd, DRAW_PIXEL, &pixel);
	if (ret < 0) {
		perror("ioctl: Failed\n");	
		return ret;
	}
	return 0;
}
