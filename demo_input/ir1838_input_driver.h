#ifndef __IR1838_H_
#define __IR1838_H_
#include <linux/types.h>

#define DECODE_SUCCESS	1
#define DECODE_FAILED	0

#define USECPERTICK	50
#define TOLERANCE	25

#define _GAP	5000
#define GAP_TICKS (_GAP/USECPERTICK)

#define TICKS_LOW(us) (((us) * 75 / 100) / USECPERTICK)
#define TICKS_HIGH(us) (((us) * 125 / 100) / USECPERTICK + 1)

#define MARK_EXCESS	100
#define RAWBUF		64
#define STATE_IDLE	2
#define STATE_MARK	3
#define STATE_SPACE	4
#define STATE_STOP	5

#define NEC_HDR_MARK    9000
#define NEC_BIT_MARK    560
#define NEC_HDR_SPACE   4500
#define NEC_ONE_SPACE   1600
#define NEC_ZERO_SPACE  560
#define NEC_RPT_SPACE   2250

#define NEC_BITS 32
#define MARK  0
#define SPACE 1
#define REPEAT 0xffffffff

struct irparams {
	uint8_t rcvstate;
	unsigned int rawbuf[RAWBUF];
	uint32_t timer;
	uint8_t rawlen;
};

struct decode_result {
	unsigned int *rawbuf;
	uint8_t rawlen;
	int bits;
	uint32_t value;
};

#endif
