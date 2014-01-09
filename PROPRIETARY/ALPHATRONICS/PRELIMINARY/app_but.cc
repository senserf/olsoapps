/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"

#include "phys_cc1100.h"
#include "plug_null.h"
#include "cc1100.h"

#include "buttons.h"
#include "switches.h"
#include "netid.h"

sint	sfd = -1;

word	repeat_count;		// Countdown for resending wake packets

word	nblinks;

word	buttons = 0, buttout;

switches_t	sw;

#define	RETRY_COUNT	4
#define	RETRY_TIME	1024
#define	CONT_BUTTON	0x20

static word get_host_id () {

	read_switches (&sw);
	return sw.S0 + sw.S1 * 10;
}

#define	HOST_ID		get_host_id ()

#include "ackrcv.h"

// ============================================================================

static void butpress (word but) {

	buttons |= (1 << but);
	trigger (&buttons);
}

// ============================================================================

fsm root {

	state INIT_ALL:

		word par;

		powerdown ();
		phys_cc1100 (0, CC1100_MAXPLEN);
		tcv_plug (0, &plug_null);
		if ((sfd = tcv_open (NONE, 0, 0)) < 0)
			syserror (ERESOURCE, "ini");
		par = NETWORK_ID;
		tcv_control (sfd, PHYSOPT_SETSID, &par);
		runfsm receiver;
		buttons_action (butpress);

		leds (0, 2);
		leds (1, 2);
		leds (2, 2);
		leds (3, 2);

		delay (3072, GO_AHEAD);
		release;

	state GO_AHEAD:

		leds (0, 0);
		leds (1, 0);
		leds (2, 0);
		leds (3, 0);

	state WAIT_BUTTON:

		if (buttons == 0) {
			when (&buttons, WAIT_BUTTON);
			release;
		}

		event_count++;
		leds (3, 1);

		buttout = buttons;
		buttons = 0;
		repeat_count = RETRY_COUNT;
		WACK = YES;

	state SEND_BUTTONS:

		address pkt;

		if ((buttout & CONT_BUTTON)) {
			// Special (keep going) button
			if (buttons & (CONT_BUTTON - 1))
				// Any other button cancels
				goto Term;
			goto Skip;
		}

		if ((buttout & CONT_BUTTON) == 0 && WACK == NO) {
			receiver_off ();
			nblinks = 4;
			proceed BLINKS;
		}

		if (repeat_count == 0) {
Term:
			receiver_off ();
			WACK = NO;
			nblinks = 2;
			proceed BLINKS;
		}
Skip:

		pkt = tcv_wnp (SEND_BUTTONS, sfd, 14);

		// Host ID
		pkt [1] = HOST_ID;
		pkt [2] = PKTYPE_BUTTON;
		pkt [3] = event_count;
		pkt [4] = buttout;
		pkt [5] = (((word)(sw.S2)) << 8) | sw.S3;

		tcv_endp (pkt);

		if (repeat_count == RETRY_COUNT)
			receiver_on ();

		if ((buttout & CONT_BUTTON) == 0)
			repeat_count --;

		delay (RETRY_TIME, SEND_BUTTONS);
		when (&WACK, SEND_BUTTONS);
		release;

	state BLINKS:

		leds (3, 0);
		if (nblinks == 0)
			proceed WAIT_BUTTON;
		nblinks--;
		delay (128, BLINK1);
		release;

	state BLINK1:

		leds (3, 1);
		delay (128, BLINKS);
		release;
}
