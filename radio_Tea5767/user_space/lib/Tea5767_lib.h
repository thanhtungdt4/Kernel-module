#ifndef _TEA5767_LIB_
#define _TEA5767_LIB_

#include <stdbool.h>
#include <unistd.h>

#include "Tea5767_ioctl.h"

#define FILENAME                "/dev/Tea5767-i2c"

uint32_t set_frequency(double freq);
void find_stations(uint32_t minlvl);
void set_muted(bool muted);
void set_standby(bool stby);
void set_stereoNC(bool stereo);
uint32_t get_frequency(void);
uint32_t get_ready(void);
bool is_muted(void);
uint32_t get_stations(void);
bool is_stereo(void);
void next_stations(void);

#endif
