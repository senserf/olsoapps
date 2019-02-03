/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

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

#if 0
trace ("%d %d %d %d %d %d %d",
	sizeof (headerType),
	sizeof (msgMasterType),
	sizeof (msgDispType),
	sizeof (msgOdrType),
	sizeof (msgTraceType),
	sizeof (msgTraceAckType),
	sizeof (out_t));
#endif
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

