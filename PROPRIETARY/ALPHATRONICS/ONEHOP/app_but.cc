/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2014                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "taglib.h"

#ifdef __SMURPH__

#define	get_address()	0

#else

#include "switches.h"

switches_t	sw;

static byte get_address () {

	read_switches (&sw);
	return (byte)(sw.S0 + sw.S1 * 10);
}

#endif

// ============================================================================

static byte current, ebuf [2];

// ============================================================================

static void butpress (word but) {

	// Make sure they are numbered from 1
	but++;

	if (current) {
		// Sending already, circular buffer for two pending events
		if (ebuf [0] == 0) {
			ebuf [0] = but;
			ebuf [1] = 0;
			return;
		}
		if (ebuf [1] != 0)
			ebuf [0] = ebuf [1];
		ebuf [1] = but;
		return;
	}
	current = but;
	trigger (&current);
}

// ============================================================================

fsm root {

	state INIT_ALL:

		start_up ();
		buttons_action (butpress);

	state LOOP:

		if (current) {
			when (send_button (current, get_address ()), DONE);
		} else {
			when (&current, LOOP);
		}

		release;

	state DONE:

		if (ebuf [0]) {
			current = ebuf [0];
			ebuf [0] = ebuf [1];
			ebuf [1] = 0;
		} else {
			current = 0;
		}

		sameas LOOP;
}
