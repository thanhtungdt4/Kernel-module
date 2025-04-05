#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

#define SERIAL_FILE		"/dev/ttyAMA0"
#define BAURATE			B9600

int main()
{
	int tty_fd;
	struct termios newtio;
	char buf[20];

	memset(buf, 0, 20);
	tty_fd = open(SERIAL_FILE, O_RDWR | O_NONBLOCK);
	if (tty_fd < 0) {
		perror("open: Failed\n");
		return tty_fd;
	}

	newtio.c_iflag = 0;
	newtio.c_oflag = 0;
	newtio.c_cflag = CS8 | CREAD | CLOCAL;
	newtio.c_lflag = 0;
	newtio.c_cc[VMIN] = 1;
	newtio.c_cc[VTIME] = 5;
	
	cfsetispeed(&newtio, BAURATE);
	cfsetospeed(&newtio, BAURATE);
	tcsetattr(tty_fd, TCSANOW, &newtio);
	tcflush(tty_fd, TCIFLUSH);

	while (read(tty_fd, buf, 20)) {
		printf("%s\n", buf);
		tcflush(tty_fd, TCIFLUSH);
	}

	return 0;
}
