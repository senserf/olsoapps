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

#define	DBBSIZE		32

static byte dbb [DBBSIZE];

#define SENSOR_EVENT	(&(SVal))

// ============================================================================
void debg (byte v) {

	word i;

	for (i = 1; i < DBBSIZE; i++)
		dbb [i-1] = dbb [i];

	dbb [DBBSIZE-1] = v;
}

void debh (byte n) {

	byte c;

	while (1) {
		for (c = 0; c < n; c++) {
			leds (3,1);
			mdelay (100);
			leds (3, 0);
			mdelay (100);
		}
		mdelay (500);
	}
}

// ============================================================================
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

static void handle_dump (word addr, byte size) {

	address msg;

	if ((msg = new_msg (message_dump_code, 2 + size + (size & 1))) !=
	    NULL) {
		osspar (msg) [0] = size;
		memcpy (osspar (msg) + 1, (address) addr, size);
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

static word blink_count = 0;

fsm blinker {

	state BL_START:

		if (blink_count == 0)
			finish;

		blink_count--;

		leds (3, 1);

		delay (128, BL_A1);
		release;

	state BL_A1:

		leds (3, 0);
		delay (128, BL_A2);

	state BL_A2:

		leds (3, 1);
		delay (128, BL_OFF);

	state BL_OFF:

		leds (3, 0);
		delay (360, BL_START);
}

static void blink () {

	if (blink_count < 8)
		blink_count++;

	if (!running (blinker))
		runfsm blinker;
}

fsm loop_monitor {

	address msg;

	state AR_START:

		when (SENSOR_EVENT, AR_ON);
		delay (1024, AR_ON);
		release;

	state AR_ON:

		as3932_on ();

	state AR_LOOP:

		when (SENSOR_EVENT, AR_EVENT);
		wait_sensor (SENSOR_AS3932, AR_EVENT);

	state AR_EVENT:

#define	LSV (*((lword*)(&SVal)))

		read_sensor (AR_EVENT, SENSOR_AS3932, (address)(&SVal));

		if (LSV != 0)
			blink ();

		if ((msg = new_msg (message_status_code,
			    sizeof (message_status_t))) == NULL) {
			delay (256, AR_EVENT);
			release;
		}	

		((lword*)(osspar (msg))) [0] = seconds ();
		((lword*)(osspar (msg))) [1] = LSV;

		tcv_endp (msg);
#undef LSV
		as3932_off ();

		sameas AR_START;
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

			if (pmt->what == 0)
				// Turn off
				as3932_off ();
			else
				as3932_on ();
				
			goto OK;
#undef	pmt

		case command_dump_code:

			if (pml < sizeof (command_dump_t))
				goto BadLength;

#define	pmt ((command_dump_t*)par)

			if (pmt->addr == WNONE) {
				pmt->addr = (word)dbb;
				pmt->size = DBBSIZE;
			} else if (pmt->size < 1) {
				pmt->size = 1;
			} else if (pmt->size > 32) {
				pmt->size = 32;
			}

			handle_dump (pmt->addr, pmt->size);

			goto OK;
#undef	pmt

		case command_rreg_code:

			if (pml < sizeof (command_rreg_t))
				goto BadLength;

#define	pmt ((command_rreg_t*)par)

			if ((msg = new_msg (message_rreg_code, 2)) != NULL) {
				((message_rreg_t*)(osspar (msg)))->reg =
					pmt->reg;
				((message_rreg_t*)(osspar (msg)))->val =
					(pmt->reg & 0x80) ?
						bma250_rreg (pmt->reg & 0x7F) :
						as3932_rreg (pmt->reg);
				tcv_endp (msg);
			}

			goto OK;
#undef	pmt

		case command_wreg_code:

			if (pml < sizeof (command_wreg_t))
				goto BadLength;

#define	pmt ((command_wreg_t*)par)

			if (pmt->reg & 0x80)
				bma250_wreg (pmt->reg & 0x7F, pmt->val);
			else
				as3932_wreg (pmt->reg, pmt->val);
			goto OK;
#undef	pmt

		case command_wcmd_code:

			if (pml < sizeof (command_wcmd_t))
				goto BadLength;

#define	pmt ((command_wcmd_t*)par)

			as3932_wcmd (pmt->cmd);
			goto OK;
#undef	pmt

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

		// bma250_on (BMA250_RANGE_2G, 7, BMA250_STAT_MOVE);
		powerdown ();

		phys_cc1100 (0, MAX_PACKET_LENGTH);
		tcv_plug (0, &plug_null);

		RFC = tcv_open (NONE, 0, 0);
		sid = NETID;
		tcv_control (RFC, PHYSOPT_SETSID, &sid);

		runfsm radio_receiver;
		runfsm loop_monitor;

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

