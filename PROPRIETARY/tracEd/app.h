#ifndef __app_h__
#define __app_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

#ifdef BOARD_CHRONOS
#define NODE_TYPE	1
#elif defined BOARD_WARSAW
#define NODE_TYPE	2
#elif defined BOARD_WARSAW_BLUE
#define NODE_TYPE	3
#elif defined BOARD_WARSAW
#define NODE_TYPE	4
#else
#define NODE_TYPE	0
#endif

#define DEL_QUANT	(10 << 10)
#define AUD_QUANT	(2* DEL_QUANT)
#define MAS_QUANT	(3* DEL_QUANT)
#define OSS_QUANT	MAX_UINT

#define LED_R   0
#define LED_G   1
#define LED_B   2

#define LED_OFF 0
#define LED_ON  1
#define LED_BLINK 2

// trigger / when ids
#define TRIGGER_BASE_ID	77

#define TRIG_OSSI	(TRIGGER_BASE_ID +0)
#define TRIG_MASTER	(TRIGGER_BASE_ID +1)
#define TRIG_BEAC	(TRIGGER_BASE_ID +2)

// end of trigger / when ids

typedef union {
	word w;
	struct {
		word	b0	:1,
			b1	:1,
			b2	:1,
			b3	:1,

			btyp	:1,
			oss_a  	:1,
			rx    	:1,
			m_chg  	:1,
			hb	:8;
	} f;
} appfl_t; // dupa check sizeof(appfl_t)

#endif
