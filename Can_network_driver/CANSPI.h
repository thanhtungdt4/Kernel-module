#ifndef __CANSPI_
#define __CANSPI_

typedef unsigned char 	uint8_t;

typedef union {
	struct {
		uint8_t idType;
		uint32_t id;
		uint8_t dlc;
		uint8_t data0;
		uint8_t data1;
		uint8_t data2;
		uint8_t data3;
		uint8_t data4;
		uint8_t data5;
		uint8_t data6;
		uint8_t data7;		
	} frame;
	uint8_t array[14];
} uCAN_MSG;

#endif
