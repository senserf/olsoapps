#include "sysio.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "cc1100.h"
#include "buttons.h"

#include "taglib.h"
#include "netid.h"

static sint SFD = -1;
static Boolean RS = NO, WACK = NO, FAILURE = NO, SWACK = NO;
static byte BUTT, MADD;
static word ECNT;

void receiver_on () {

	if (!RS) {
		tcv_control (SFD, PHYSOPT_ON, NULL);
		RS = YES;
	}
}

void receiver_off () {

	if (RS) {
		tcv_control (SFD, PHYSOPT_OFF, NULL);
		RS = NO;
	}
}

fsm receiver {

	state WPACKET:

		address pkt;
		word len, cmd;

		pkt = tcv_rnp (WPACKET, SFD);

		if (WACK) {

			len = tcv_left (pkt);

			if (len >= 10 && pkt [1] == HOST_ID &&
			    pkt [2] == PKTYPE_ACK && pkt [3] == ECNT) {
				WACK = NO;
				trigger (&WACK);
			}
		}

		tcv_endp (pkt);
		proceed WPACKET;
}

fsm led_server {

	lword target;

	state INIT:

		target = seconds () + 2;
		proceed FAILURE_ON;

	state LOOP:

		if (FAILURE) {
			// This takes precedence
			FAILURE = NO;
			target = seconds () + FAILURE_BLINK_TIME;
			proceed FAILURE_ON;
		}

		if (WACK) {
			// The LED goes on when waiting for an ACK
			leds (THE_LED, 1);
			if (!SWACK) {
				// At least for 2 sec
				SWACK = YES;
				delay (WACK_ON_TIME, LOOP);
				release;
			}
		} else {
			SWACK = NO;
			leds (THE_LED, 0);
		}

		when (&SWACK, LOOP);
		release;

	state FAILURE_ON:

		leds (THE_LED, 1);
		delay (FAILURE_BLINK_ON, FAILURE_OFF);
		release;

	state FAILURE_OFF:

		leds (THE_LED, 0);
		if (seconds () == target)
			sameas LOOP;
		delay (FAILURE_BLINK_OFF, FAILURE_ON);
}

fsm event_sender {

	word tries, voltage;

	state LOOP:

		if (!BUTT) {
			when (&BUTT, LOOP);
			release;
		}

	state GET_VOLTAGE:

		read_sensor (GET_VOLTAGE, -1, &voltage);
		tries = 0;
		ECNT++;
		WACK = YES;
		SWACK = NO;
		trigger (&SWACK);

	state NEXT_TRY:

		address pkt;

		if (!WACK)
			sameas DONE;

		pkt = tcv_wnp (NEXT_TRY, SFD, 14);

		receiver_on ();

		pkt [1] = HOST_ID;
		pkt [2] = PKTYPE_BUTTON;
		pkt [3] = ECNT;
		pkt [4] = (((word)BUTT) << 8) | MADD;
		pkt [5] = voltage;

		tcv_endp (pkt);

	state WAIT_ACK:

		delay (ACK_WAIT_TIME, NO_ACK);
		when (&WACK, DONE);
		release;

	state NO_ACK:

		tries++;
		if (tries < MAX_TRIES)
			sameas NEXT_TRY;

		FAILURE = WACK;

	state DONE:

		WACK = NO;		// Not needed
		receiver_off ();
		trigger (&SWACK);
		trigger (&MADD);
		BUTT = 0;
		sameas LOOP;
}
			
void start_up () {

	word par;

	powerdown ();
	phys_cc1100 (0, CC1100_MAXPLEN);
	tcv_plug (0, &plug_null);
	if ((SFD = tcv_open (NONE, 0, 0)) < 0)
		syserror (ERESOURCE, "ini");
	par = NETWORK_ID;
	tcv_control (SFD, PHYSOPT_SETSID, &par);
	runfsm receiver;
	runfsm led_server;
	runfsm event_sender;

	diag ("STARTED");
}

address send_button (byte code, byte addr) {

	BUTT = code;
	MADD = addr;

	trigger (&BUTT);

	return (address)(&MADD);
}
