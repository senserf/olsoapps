/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// We shall use this one to test XRS, OEP, and the new forthcoming stuff

#include "globals.h"
#include "threadhdrs.h"

thread (root)

    word i;

    entry (RS_INIT)

	phys_uart (0, 84, 0);
	tcv_plug (0, &plug_null);
	if ((SFD = tcv_open (WNONE, 0, 0)) < 0)
		syserror (ENODEVICE, "uart");
	i = 0xffff;
	tcv_control (SFD, PHYSOPT_SETSID, &i);
	tcv_control (SFD, PHYSOPT_TXON, NULL);
	tcv_control (SFD, PHYSOPT_RXON, NULL);
	ab_init (SFD);
	// ab_mode (AB_MODE_ACTIVE);

    entry (RS_RCMD)

	diag ("READING");
	ibuf = ab_in (RS_RCMD);

    entry (RS_ECHO)

	diag ("WRITING");
	ab_outf (RS_ECHO, "GOT: %s", ibuf);
	ufree (ibuf);
	proceed (RS_RCMD);

endthread

praxis_starter (Node);
