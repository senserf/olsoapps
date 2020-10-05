/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "sysio.h"
#include "netid.h"
#include "rtc_cc430.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "cc1100.h"
#include "plug_null.h"
#include "sensors.h"

#ifndef	AP320_BROKEN
#include "buttons.h"
#endif

// ============================================================================
// 
// This one is UART-free. This is how it works:
//
// When you press the panic button, the board lights the LED and sends the
// WAKE packet up to 4 times, or until it gets an ACK. In the latter case,
// following the ACK, the board enters the motion reporting mode for 10
// seconds during which it will be reporting motion events. While in the
// motion detection mode, the LED blinks.
//
// If there's no ACK after 4 retries, the LED blinks twice and goes off.
//
// ============================================================================

#define	BUTTON_POLL_INTERVAL	1024
#define	RETRY_TIME		1024
#define	RETRY_COUNT		4
#define	MD_TIME			(1024 * 10)

//#define	MOTION_SENSOR_PRESENT	1
#define	MOTION_SENSOR_PRESENT	0

#define	HOST_ID			((word)host_id)

// ============================================================================
sint	sfd = -1;

word	repeat_count;		// Countdown for resending wake packets

word	nblinks;

#if MOTION_SENSOR_PRESENT

fsm motion_detector {

	bma250_data_t c;

	state MOTION_WAIT:

		wait_sensor (SENSOR_MOTION, MOTION_EVENT);
		release;

	state MOTION_EVENT:

		read_sensor (MOTION_EVENT, SENSOR_MOTION, (address)(&c));

	state MOTION_SEND:

		address pkt = tcv_wnp (MOTION_SEND, sfd, 18);

		pkt [1] = HOST_ID;
		pkt [2] = PKTYPE_MOTION;
		pkt [3] = c.stat;
		pkt [4] = c.temp;
		pkt [5] = c.x;
		pkt [6] = c.y;
		pkt [7] = c.z;

		tcv_endp (pkt);

		proceed MOTION_WAIT;
}

static void start_motion () {

	bma250_on (BMA250_RANGE_2G, 7, BMA250_STAT_MOVE);
	bma250_move (2, 40);
	runfsm motion_detector;
}

static void stop_motion () {

	killall (motion_detector);
	bma250_off (0);
}

#endif

#include "ackrcv.h"

// ============================================================================

#ifndef AP320_BROKEN

static word buttons = 0;

static void butpress (word but) {

	buttons |= (1 << but);
	trigger (&buttons);
}

#endif

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

#ifndef	AP320_BROKEN
		buttons_action (butpress);
#endif

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

	state BUTTON_MONITOR:

#ifdef AP320_BROKEN
		// Due to an error, we do 1-sec polling; this will
		// be fixed in the next version (I hope)
		if (!button_pressed) {
			delay (BUTTON_POLL_INTERVAL, BUTTON_MONITOR);
			release;
		}
#else
		if (buttons == 0) {
			when (&buttons, BUTTON_MONITOR);
			release;
		}
		buttons = 0;
#endif
		event_count++;
		// LED goes on
		leds (3, 1);

#ifdef AP320_BROKEN
	state WAIT_BUTTON_DOWN:

		// Wait until the button goes down
		if (button_pressed) {
			delay (10, WAIT_BUTTON_DOWN);
			release;
		}
#endif
		repeat_count = RETRY_COUNT;
		WACK = YES;

	state SEND_WAKE:

		address pkt;

		if (WACK == NO) {
			receiver_off ();
#if MOTION_SENSOR_PRESENT
			proceed ACKED;
#else
			nblinks = 4;
			proceed BLINKS;
#endif
		}

		if (repeat_count == 0) {
			// No ACK
			receiver_off ();
			WACK = NO;
			nblinks = 2;
			proceed BLINKS;
		}

		pkt = tcv_wnp (SEND_WAKE, sfd, 10);
		pkt [1] = HOST_ID;
		pkt [2] = PKTYPE_WAKE;
		pkt [3] = event_count;
		tcv_endp (pkt);

		if (repeat_count == RETRY_COUNT) {
			// First time around
			receiver_on ();
		}

		repeat_count--;

		delay (RETRY_TIME, SEND_WAKE);
		when (&WACK, SEND_WAKE);
		release;

	state BLINKS:

		leds (3, 0);
		if (nblinks == 0)
			proceed BUTTON_MONITOR;
		nblinks--;
		delay (128, BLINK1);
		release;

	state BLINK1:

		leds (3, 1);
		delay (128, BLINKS);
		release;

#if MOTION_SENSOR_PRESENT

	state ACKED:

		leds (3, 2);
		start_motion ();
		delay (MD_TIME, STOP_MD);
		release;

	state STOP_MD:

		stop_motion ();
		leds (3, 0);
		proceed BUTTON_MONITOR;
#endif
}
