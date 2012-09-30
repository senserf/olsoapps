/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "params.h"
#include "plug_null.h"
#include "hold.h"
#ifdef	__SMURPH__
#include "phys_cc1100.h"
#else
#include "cc1100.h"
#endif


const word NID = NETWORK_ID;

word sen [4];

fsm root {

	lword next_run;
	sint SFD, Tries;

	state INIT:

		phys_cc1100 (0, CC1100_MAXPLEN);
		tcv_plug (0, &plug_null);
		SFD = tcv_open (WNONE, 0, 0);
		tcv_control (SFD, PHYSOPT_SETSID, (address) &NID);
		powerdown ();
		delay (1024, RUN);
		release;

	state RUN:

		Tries = 1;

	state SEN0:

		read_sensor (SEN0, 0, sen + 0);

	state SEN1:

		read_sensor (SEN1, 1, sen + 1);

	state SEN2:

		read_sensor (SEN2, 2, sen + 2);

	state SEN3:

		read_sensor (SEN3, 3, sen + 3);

		tcv_control (SFD, PHYSOPT_ON, NULL);

	state SEND:

		address packet;

		packet = tcv_wnp (SEND, SFD, PACKET_LENGTH);

		packet [1] = MAGIC;
		memcpy (packet + 2, sen, 8);
		tcv_endp (packet);

		if (Tries < REPEAT_COUNT) {
			Tries++;
			delay (REPEAT_DELAY, SEND);
			release;
		}

		delay (TRANSMIT_MARGIN, ROFF);
		release;

	state ROFF:

		tcv_control (SFD, PHYSOPT_OFF, NULL);

		next_run = seconds () + CYCLE_INTERVAL;

	state HIBERNATE:

		hold (HIBERNATE, next_run);
		sameas RUN;

}
