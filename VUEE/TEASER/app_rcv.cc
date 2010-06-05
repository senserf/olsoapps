/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "ser.h"
#include "serf.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "plug_null.h"

#define	MAX_PACKET_LENGTH	60

word	Count;
int	sfd = -1;

// ============================================================================

void show (word st, address pkt) {

	ser_outf (st, "RCV: %d [%s] pow = %d qua = %d\r\n",
		Count++,
		(char*)(pkt + 1),
		((byte*)pkt) [tcv_left (pkt) - 1],
		((byte*)pkt) [tcv_left (pkt) - 2]
	);
}

fsm receiver {

  shared address rpkt;

  entry RC_TRY:

	rpkt = tcv_rnp (RC_TRY, sfd);

  entry RC_SHOW:

	show (RC_SHOW, rpkt);
	tcv_endp (rpkt);
	proceed RC_TRY;
}

// ============================================================================

fsm root {

  entry RS_INIT:

	phys_cc1100 (0, MAX_PACKET_LENGTH);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (WNONE, 0, 0);
	tcv_control (sfd, PHYSOPT_RXON, NULL);

	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

	runfsm receiver;
	finish;
}
