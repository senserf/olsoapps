/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "sysio.h"
#include "taglib.h"

// ============================================================================

static byte current;

// ============================================================================

static void butpress (word but) {

	if (current) {
		// Sending already
		return;
	}
	// Button = 1 + global
	current = 0x81;
	trigger (&current);
}

// ============================================================================

fsm root {

	state INIT_ALL:

		leds (0, 1);
		leds (1, 1);
		leds (2, 1);

		delay (1024, STARTED);
		release;

	state STARTED:

		leds (0, 0);
		leds (1, 0);
		leds (2, 0);

		start_up ();
		buttons_action (butpress);

	state LOOP:

		if (current) {
			when (send_button (current, 0), DONE);
		} else {
			when (&current, LOOP);
		}

		release;

	state DONE:

		current = 0;
		sameas LOOP;
}
