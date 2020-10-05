/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "cc1100.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "as3932.h"
#include "bma250.h"
#include "buttons.h"

#include "netid.h"
#include "blink.h"

#define	THE_LED		3
#define	THE_OTHER_LED	0
#define	MAX_IDLE_TIME	MAX_WORD

sint	rfc;
lword	last_tap = (lword)(-10);
lword	last_loop = (lword)(-10);
lword	last_but = (lword)(-10);

word	val_volt, val_ivolt, val_cvolt;

// ============================================================================

static void butpress_on (word but) {

	if (but == BUTTON_PANIC) {
		last_but = seconds ();
		blink (THE_OTHER_LED, 1, 1024, 0, 512);
	}
}

static void butpress_off (word but) {

}

static start_devices () {

	address pkt;
	byte i;

	// Radio on
	tcv_control (rfc, PHYSOPT_RXON, NULL);
	bma250_on (BMA250_RANGE_2G, 7, BMA250_STAT_TAP | BMA250_STAT_MOVE);
	bma250_move (30, 1);
	// bma250_tap (0, 10, 4, 0);
	as3932_on ();
	// Button
	buttons_action (butpress_on);
	// Read the AS3932 registers and report them in a special packet
	// (we will be adding other device related stuff as needed)
	if ((pkt = tcv_wnp (WNONE, rfc, 18)) == NULL)
		return;

	pkt [1] = STA_MAGIC;

	for (i = 0; i <= 13; i++)
		((byte*)(pkt + 2)) [i] = as3932_rreg (i);

	tcv_endp (pkt);

}

static stop_devices () {

	as3932_off ();
	bma250_off (0);
	tcv_control (rfc, PHYSOPT_OFF, NULL);
	buttons_action (butpress_off);
}

void send_status_packet () {

	address pkt;
	lword d;

	if ((pkt = tcv_wnp (WNONE, rfc, 18)) == NULL)
		// Ignore
		return;

	pkt [1] = REP_MAGIC;

	d = seconds () - last_tap;

	if (d > MAX_WORD)
		d = MAX_WORD;

	pkt [2] = (word) d;

	d = seconds () - last_loop;

	if (d > MAX_WORD)
		d = MAX_WORD;

	pkt [3] = (word) d;

	d = seconds () - last_but;

	if (d > MAX_WORD)
		d = MAX_WORD;

	pkt [4] = (word) d;

	// Room for extra info
	pkt [5] = val_volt;
	pkt [6] = val_ivolt;
	pkt [7] = val_cvolt;

	tcv_endp (pkt);
}

fsm accel_monitor {

	bma250_data_t c;

	state AM_WAIT:

		wait_sensor (0, AM_TAP);

	state AM_TAP:

		read_sensor (AM_TAP, 0, (address)(&c));
		last_tap = seconds ();
		blink (THE_LED, 3, 64, 64, 512);
		sameas AM_WAIT;
}

fsm loop_monitor {

	as3932_data_t c;

	state LM_WAIT:

		wait_sensor (1, LM_LOOP);

	state LM_LOOP:

		read_sensor (LM_LOOP, 1, (address)(&c));
		last_loop = seconds ();
		blink (THE_LED, 5, 64, 64, 512);
		sameas LM_WAIT;
}

fsm radio_receiver {

	state RS_LOOP:

		address pkt;

		delay (MAX_IDLE_TIME, HALT_ALL);
		pkt = tcv_rnp (RS_LOOP, rfc);

		if (tcv_left (pkt) < 6 || pkt [1] != PING_MAGIC) {
			tcv_endp (pkt);
			sameas RS_LOOP;
		}

		tcv_endp (pkt);

		send_status_packet ();

		sameas RS_LOOP;

	// ====================================================================

	state HALT_ALL:

		// Halts the node to prevent batery drain
		stop_devices ();
		killall (loop_monitor);
		killall (accel_monitor);
		blink (THE_LED, 16, 128, 128, 0);

	state HANG_ME:

		delay (65535, HANG_ME);
}

fsm voltage_monitor {

	state VR_READ:

		read_sensor (VR_READ, -1, &val_volt);

#if N_HIDDEN_SENSORS == 3

	// Two more voltage sensors

	state VR_IREAD:

		read_sensor (VR_IREAD, -3, &val_ivolt);

	state VR_CREAD:

		read_sensor (VR_CREAD, 2, &val_cvolt);
#endif

		delay (4000, VR_READ);
}

fsm root {

	state RS_INIT:

		word sid;

		blink (THE_LED, 4, 128, 128, 256);
		powerdown ();

		phys_cc1100 (0, MAX_PACKET_LENGTH);
		tcv_plug (0, &plug_null);

		rfc = tcv_open (NONE, 0, 0);
		sid = NETID;
		tcv_control (rfc, PHYSOPT_SETSID, &sid);

		start_devices ();

		runfsm radio_receiver;
		runfsm loop_monitor;
		runfsm accel_monitor;
		runfsm voltage_monitor;

		// This thread is gone
		finish;
}
