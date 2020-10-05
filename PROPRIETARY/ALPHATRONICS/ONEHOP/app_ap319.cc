/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "sysio.h"
#include "taglib.h"

// ============================================================================

static byte current, saddress, locals, ebuf [2];

// ============================================================================
// ============================================================================

#ifdef	__SMURPH__

static word NFLAGS = (word) preinit ("switches");

static void get_node_params () {

	saddress = (byte)((NFLAGS & 0xF) + ((NFLAGS >> 4) & 0xF) * 10);
	locals   = (byte)(NFLAGS >> 8);
}

#else

#include "switches.h"

static void get_node_params () {

	switches_t sw;

	read_switches (&sw);
	saddress = (byte)(sw.S0 + sw.S1 * 10);
	locals   = sw.S2;
}

#endif

// ============================================================================
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
			// It probably makes little sense to do it so often
			get_node_params ();
			if ((locals >> ((current & 0x0F) - 1)) & 1)
				// Set the global flag
				current |= 0x80;
			when (send_button (current, saddress), DONE);
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
