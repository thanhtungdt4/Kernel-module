#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Tea5767_lib.h"

int open_file()
{
	int fd;

	fd = open(FILENAME, O_RDWR);
	if (fd < 0) {
		perror("open: Failed\n");
		exit(-1);
	}

	return fd;
}

uint32_t set_frequency(double freq)
{
	int fd;
	int freq_int;
	int ret;

	fd = open_file();
	freq_int = freq * 1000000;

	printf("frequency: %d\n", freq_int);

	if (ioctl(fd, SET_FREQUENCY, &freq_int) < 0) {
		perror("ioctl: Failed\n");
		close(fd);
		return -1;
	}
	ret = freq_int;

	close(fd);

	return ret;
}

void find_stations(uint32_t minlvl)
{
	int fd;

	fd = open_file();

	if (ioctl(fd, FIND_STATIONS, &minlvl) < 0) {
		perror("ioctl: Failed\n");
		close(fd);
		return;
	}

	close(fd);
}

void set_muted(bool muted)
{
	int fd;

	fd = open_file();
	if (ioctl(fd, SET_MUTED, &muted) < 0) {
		perror("ioctl: Failed\n");
		close(fd);
		return;
	}

	close(fd);
}

void set_standby(bool stby)
{
	int fd;

	fd = open_file();
	if (ioctl(fd, SET_STANDBY, &stby) < 0) {
		perror("ioctl: Failed\n");
		close(fd);
		return;
	}

	close(fd);
}

void set_stereoNC(bool stereo)
{
	int fd;

	fd = open_file();
	if (ioctl(fd, SET_STEREO_NC, &stereo) < 0) {
		perror("ioctl: Failed\n");
		close(fd);
		return;
	}

	close(fd);
}

uint32_t get_frequency(void)
{
	int fd;
	uint32_t freq;

	fd = open_file();
	if (ioctl(fd, GET_FREQUENCY, &freq) < 0) {
		perror("ioctl: Failed\n");
		close(fd);
		return -1;
	}

	close(fd);

	return freq;
}

uint32_t get_ready(void)
{
	int fd;
	uint32_t rdy;

	fd = open_file();
	if (ioctl(fd, GET_READY, &rdy) < 0) {
		perror("ioctl: Failed\n");
		close(fd);
		return -1;
	}

	close(fd);

	return rdy;
}

bool is_muted(void)
{
	int fd;
	bool muted;

	fd = open_file();
	if (ioctl(fd, IS_MUTED_GET, &muted) < 0) {
		perror("ioctl: Failed\n");
		close(fd);
		return false;
	}

	close(fd);

	return muted;
}

uint32_t get_stations(void)
{
	int fd;
	uint32_t stations;

	fd = open_file();
	if (ioctl(fd, GET_STATIONS, &stations) < 0) {
		perror("ioctl: Failed\n");
		close(fd);
		return -1;
	}

	close(fd);

	return stations;
}

bool is_stereo(void)
{
	int fd;
	bool stereo;

	fd = open_file();
	if (ioctl(fd, IS_STEREO, &stereo) < 0) {
		perror("ioctl: Failed\n");
		close(fd);
		return false;
	}

	close(fd);

	return stereo;
}

void next_stations(void)
{
	int fd;

	fd = open_file();
	if (ioctl(fd, NEXT_STATIONS) < 0) {
		perror("ioctl: Failed\n");
		close(fd);
		return;
	}

	close(fd);
}
