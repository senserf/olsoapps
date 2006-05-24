/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "plug_null.h"
#include "phys_dm2200.h"

static int fd = -1;

process (receiver, void)
	static address packet;
	nodata;

  entry (0)
	diag ("rcv ready");
	packet = tcv_rnp (0, fd);
	diag ("rcv ?");
	tcv_endp (packet);
	delay (1007, 0);
endprocess (1)

process (sender, void)
	static address packet;
	nodata;

  entry (0)
	diag ("tx");
	packet = tcv_wnp (0, fd, 40);
	packet[0] = 0;
	tcv_endp (packet);
	delay (2011, 0);
endprocess (1)
	
process (root, void)
	nodata;

	entry (0)
		phys_dm2200 (0, 60);
		tcv_plug (0, &plug_null);
		if ((fd = tcv_open (NONE, 0, 0)) < 0) {
			diag ("???");
			delay (10000, 0);
			release;
		}
		tcv_control (fd, PHYSOPT_TXON, NULL);
		tcv_control (fd, PHYSOPT_RXON, NULL);
		leds (0, 2);
		diag ("init");
		fork (receiver, NULL);
		fork (sender, NULL);
		delay (5000, 1);
		release;

	entry (1)
		tcv_control (fd, PHYSOPT_RXOFF, NULL);
		tcv_control (fd, PHYSOPT_TXOFF, NULL);
		leds (0, 0);
		diag ("freeze %x", P4OUT);
		freeze ((rnd () & 3) + 1);
		tcv_control (fd, PHYSOPT_RXON, NULL);
		tcv_control (fd, PHYSOPT_TXON, NULL);
		diag ("...... UP %x", P4OUT);
		leds (0, 2);
		delay (2000, 1);
		release;
endprocess (0)
#undef RS_INIT
#undef RS_FREE
#undef RS_RCMD
#undef RS_DOCMD
#undef RS_CMDOUT
#undef RS_RETOUT
