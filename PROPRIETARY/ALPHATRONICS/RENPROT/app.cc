/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "sysio.h"
#include "netid.h"
#include "tcvphys.h"
#include "phys_uart.h"
#include "plug_null.h"

// This is a simple test of the RENESAS exchange protocol: any message received
// is transformed by incrementing all bytes modulo 256 and echoed back; no
// radio

sint	sfu = -1;

fsm root {

	address pkti, pkto;
	word len;

	state RS_INIT:

		phys_uart (0, MAX_RENESAS_MESSAGE_LENGTH, 0);
		tcv_plug (0, &plug_null);
		sfu = tcv_open (WNONE, 0, 0);
		if (sfu < 0)
			syserror (ENODEVICE, "UA!");

		leds (0, 2);
		leds (1, 2);
		leds (2, 2);

	state FIRST_MSG:

		address pkt;

		// Send some dummy message to check if it works at all
		pkt = tcv_wnp (FIRST_MSG, sfu, 2);
		pkt [0] = 0x5555;
		tcv_endp (pkt);

		delay (2048, GO_AHEAD);
		release;

	state GO_AHEAD:

		leds (0, 0);
		leds (1, 0);
		leds (2, 0);
		diag ("RUNNING");

	state WPACKET:

		pkti = tcv_rnp (WPACKET, sfu);
		len = tcv_left (pkti);

	state SPACKET:

		word i;

		pkto = tcv_wnp (SPACKET, sfu, len);

		for (i = 0; i < len; i++)
			((byte*)pkto) [i] = ((byte*)pkti) [i] + 1;

		tcv_endp (pkti);
		tcv_endp (pkto);

		proceed WPACKET;
}
