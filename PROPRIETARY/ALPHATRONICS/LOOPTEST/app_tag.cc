#include "sysio.h"
#include "cc1100.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "as3932.h"

#include "rf.h"
#include "netid.h"
#include "ossi.h"

// ============================================================================
// ============================================================================

byte	RadioOn; 	// Just 0 or 1, RadioDelay indicates the RX open time
as3932_data_t SVal;

static sint RFC;
static word RadioDelay = RADIO_WOR_IDLE_TIMEOUT;
static byte LastRef;

#define SENSOR_EVENT	(&(SVal))

// ============================================================================

static address new_msg (byte code, word len) {
//
// Tries to acquire a packet for outgoing RF message; len == parameter length
//
	address msg;

	if ((msg = tcv_wnp (WNONE, RFC, len + RFPFRAME + sizeof (oss_hdr_t))) !=
	    NULL) {
		msg [1] = NODE_ID;
		osshdr (msg) -> code = code;
		osshdr (msg) -> ref = LastRef;
	}
	return msg;
}

static void oss_ack (word status) {

	address msg;

	if ((msg = new_msg (0, sizeof (status))) != NULL) {
		osspar (msg) [0] = status;
		tcv_endp (msg);
	}
}

void switch_radio (byte on) {

	word par [2];

	if (RadioOn != on) {
		par [0] = RadioOn = on;
		tcv_control (RFC, PHYSOPT_OFF, par);
	}

	if (RadioOn) {
		par [0] = RadioDelay;
		par [1] = 2048;		// This is just in case
		tcv_control (RFC, PHYSOPT_SETPARAMS, par);
	}
}

// ============================================================================

fsm sensor_monitor {

	address msg;

	state AR_LOOP:

		when (SENSOR_EVENT, AR_EVENT);
		wait_sensor (SENSOR_AS3932, AR_EVENT);
		release;

	state AR_EVENT:

		read_sensor (AR_EVENT, SENSOR_AS3932, (address)(&SVal));

		if ((msg = new_msg (message_status_code,
			    sizeof (message_status_t))) == NULL) {
			delay (256, AR_EVENT);
			release;
		}	

		((lword*)(osspar (msg))) [0] = seconds ();
		SVal.fwake = (SVal.fwake & 0x7f);
		if (as3932_int)
			SVal.fwake |= 0x80;
		((lword*)(osspar (msg))) [1] = *((lword*)(&SVal));
		tcv_endp (msg);
		sameas AR_LOOP;
}

static void handle_rf_command (byte code, address par, word pml) {

	address msg;
	word u;

	switch (code) {

		case command_ping_code:

			trigger (SENSOR_EVENT);
OK:
			oss_ack (ACK_OK);
			return;
#undef	pmt
		case command_turn_code:

			if (pml < sizeof (command_turn_t)) {
BadLength:
				oss_ack (ACK_LENGTH);
				return;
			}

#define	pmt ((command_turn_t*)par)

			if (pmt->conf == 0 && pmt->mode == 255) {
				// Turn off
				as3932_off ();
			} else {
				as3932_on (pmt->conf, pmt->mode, pmt->patt);
			}
				
			goto OK;
#undef	pmt
		case command_rreg_code:

			if (pml < sizeof (command_rreg_t))
				goto BadLength;

#define	pmt ((command_rreg_t*)par)
			if (pmt->reg > 13)
				u = P1IN;
			else
				u = as3932_rreg (pmt->reg);
			if ((msg = new_msg (message_regval_code,
			    sizeof (message_regval_t))) != NULL) {
				((byte*)(osspar (msg))) [0] = pmt->reg;
				((byte*)(osspar (msg))) [1] = (byte) u;
				tcv_endp (msg);
			}
#undef	pmt
			return;

		case command_wreg_code:

			if (pml < sizeof (command_wreg_t))
				goto BadLength;

#define	pmt ((command_wreg_t*)par)

			as3932_wreg (pmt->reg, pmt->val);
#undef	pmt
			goto OK;

		case command_wcmd_code:

			if (pml < sizeof (command_wcmd_t))
				goto BadLength;

#define	pmt ((command_wcmd_t*)par)

			as3932_wcmd (pmt->what);
#undef	pmt
			goto OK;



		case command_radio_code:

			if (pml < sizeof (command_radio_t))
				goto BadLength;

#define	pmt ((command_radio_t*)par)

			if (pmt->delay == 0) {
				switch_radio (0);
			} else {
				RadioDelay = pmt->delay;
				switch_radio (1);
			}
			goto OK;
	}

	oss_ack (ACK_COMMAND);
}

fsm radio_receiver {

	state RS_LOOP:

		address pkt;
		oss_hdr_t *osh;

		pkt = tcv_rnp (RS_LOOP, RFC);

		if (tcv_left (pkt) >= OSSMINPL && pkt [1] == NODE_ID &&
		    (osh = osshdr (pkt)) -> ref != LastRef) {

			LastRef = osh -> ref;

			handle_rf_command (
				osh->code,
				osspar (pkt),
				tcv_left (pkt) - OSSFRAME
			);
		}

		tcv_endp (pkt);
		sameas RS_LOOP;
}

// ============================================================================

fsm root {

	state RS_INIT:

		word i;

		for (i = 0; i < 4; i++) {
			leds (3, 1);
			mdelay (512);
			leds (3, 0);
			mdelay (512);
		}

	state RS_GO:

		word sid;

		powerdown ();

		phys_cc1100 (0, MAX_PACKET_LENGTH);
		tcv_plug (0, &plug_null);

		RFC = tcv_open (NONE, 0, 0);
		sid = NETID;
		tcv_control (RFC, PHYSOPT_SETSID, &sid);

		// Start in default mode
		as3932_on (0, 0, 0);

		runfsm radio_receiver;
		runfsm sensor_monitor;

#ifdef RADIO_INITIALLY_ON
		switch_radio (1);
#endif
		finish;
}

#ifdef DEBUGGING

void rms (word code, word c, word v) {

	address pkt;

	if ((pkt = tcv_wnp (WNONE, RFC, 12)) == NULL)
		return;

	pkt [1] = NODE_ID;
	pkt [2] = 0xFF | (code << 8);
	pkt [3] = c;
	pkt [4] = v;

	tcv_endp (pkt);
}

#endif

// ============================================================================
// ============================================================================
