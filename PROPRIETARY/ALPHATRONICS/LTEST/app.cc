/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2015                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "params.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "cc1100.h"
#include "buttons.h"

#define	NETID	0x004d
#define	HOSTID	2

static sint	sfd = -1;
static byte	button_pressed = 0;
static byte	pwr, try;
static word	sernum;

//
// When the button is pressed, quickly send a bunch of packets, back-to-back
// on all power levels
//

static void butpress (word but) {

	if (button_pressed) {
		// Sending already
		return;
	}
	button_pressed = 1;
	trigger (&button_pressed);
}

fsm root {

	state INIT_ALL:

		word par;

		powerdown ();
		phys_cc1100 (0, CC1100_MAXPLEN);
		tcv_plug (0, &plug_null);
		if ((sfd = tcv_open (NONE, 0, 0)) < 0)
			syserror (ERESOURCE, "ini");

		par = NETID;
		tcv_control (sfd, PHYSOPT_SETSID, &par);
		buttons_action (butpress);

	state RESUME:

		leds (3, 0);
		button_pressed = 0;

	state EVENT_LOOP:

		if (button_pressed)
			sameas (HANDLE_BUTTON);

		tcv_control (sfd, PHYSOPT_OFF, NULL);

		when (&button_pressed, EVENT_LOOP);
		release;

	state HANDLE_BUTTON:

		leds (3, 1);
		delay (512, LED_OFF_A);
		release;

	state LED_OFF_A:

		leds (3, 0);

		pwr = 0;

	state NEXT_POWER:

		try = 0;

	state SEND_PACKET:

		address packet;

		// Make the packet look like a measurement packet for RFTEST
		// with RSSI reportable at node 1, i.e., the collector

		packet = tcv_wnp (SEND_PACKET, sfd, (POFF_ACT+2)*2);
		packet [POFF_DRI] = 0xAAAA;
		packet [POFF_RCV] = 0;
		packet [POFF_SND] = HOSTID;
		packet [POFF_SER] = sernum++;
		packet [POFF_FLG] = pwr;
		packet [POFF_ACT] = 1;
		packet [POFF_ACT+1] = (pwr << 12);

		tcv_endp (packet);

		if (INTER_PACKET_DELAY) {
			delay (INTER_PACKET_DELAY, SEND_DONE);
			release;
		}

	state SEND_DONE:

		if (++try < PACKETS_PER_LEVEL)
			proceed SEND_PACKET;

		if (++pwr <= 7)
			proceed NEXT_POWER;

		// Done all, two blinks
		leds (3, 1);
		delay (512, LED_OFF_B);
		release;

	state LED_OFF_B:

		leds (3, 0);
		delay (256, LED_ON_A);
		release;

	state LED_ON_A:

		leds (3, 1);
		button_pressed = 0;	
		delay (256, RESUME);
}
