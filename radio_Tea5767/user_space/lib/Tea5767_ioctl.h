#ifndef __TEA5767_IOCTL_
#define __TEA5767_IOCTL_

#include <sys/ioctl.h>

typedef unsigned int 		uint32_t;

#define MAGIC_NO		150
#define SET_FREQUENCY		_IOWR(MAGIC_NO, 0, uint32_t)
#define FIND_STATIONS		_IOW(MAGIC_NO, 1, uint32_t)
#define SET_MUTED		_IOW(MAGIC_NO, 2, bool)
#define SET_STANDBY		_IOW(MAGIC_NO, 3, bool)
#define SET_STEREO_NC		_IOW(MAGIC_NO, 4, bool)
#define GET_FREQUENCY		_IOR(MAGIC_NO, 5, uint32_t)
#define GET_READY		_IOR(MAGIC_NO, 6, uint32_t)
#define IS_MUTED_GET		_IOR(MAGIC_NO, 7, bool)
#define GET_STATIONS		_IOR(MAGIC_NO, 8, uint32_t)
#define IS_STEREO		_IOR(MAGIC_NO, 9, bool)
#define NEXT_STATIONS		_IO(MAGIC_NO, 10)

#endif
