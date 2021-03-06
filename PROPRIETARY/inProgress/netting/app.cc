/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "app.h"
#include "app_dcl.h"
#include "tarp.h"
#include "msg_dcl.h"

// globals init to 0
lword		master_ts;

beac_t		beac;
disp_t		disp;
odr_t		odr;
trac_t		trac;
fim_t		fim_set;

fsm root {

	state RO_INIT:
		init();
		delay (DEL_QUANT, RO_AUDIT);
		release; // let things going for a short while

	// let's keep root running as an auditing process...  good in general?
	state RO_AUDIT:

		// this may not be such a good idea in some situations, e.g.
		// on CHRONOS with RX OFF most of the time:
		if (seconds() - master_ts > 3* (MAS_QUANT >> 10)) {
			master_host = 0;
			master_ts = seconds();
		}
		// what else to audit?
		delay (AUD_QUANT, RO_AUDIT);
		release;
}

