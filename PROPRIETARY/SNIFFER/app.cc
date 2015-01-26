#include "sysio.h"
#include "phys_cc1100.h"
#include "phys_uart.h"
#include "plug_null.h"

#include "ossi.h"

#ifndef __SMURPH__
#include "cc1100.h"
#endif

#define	MAX_PACKET_LENGTH 	CC1100_MAXPLEN

// ============================================================================

sint sd_rf, sd_uart;
Boolean power_up = 1;

oss_hdr_t	*CMD;
address		PMT;
word		PML;

// ============================================================================

#define	msghdr	((oss_hdr_t*)msg)
#define msgblk	((message_packet_t*)(msg + 1))

void handle_command ();

// ============================================================================

static void pktmsg (address pay, word plen) {
//
// Issue a "packet" message
//
	address msg;

	if ((msg = tcv_wnp (WNONE, sd_uart, sizeof (oss_hdr_t) +
	    sizeof (message_packet_t) + plen + 2)) == NULL)
		return;

	msghdr->code = message_packet_code;
	msghdr->ref = 0;
	msgblk->payload.size = plen;
	memcpy (msgblk->payload.content, pay, plen);
	tcv_endp (msg);
}

#undef	msgblk

fsm radio_receiver {

	address pkt;


	state RCV_WAIT:

		word len;

		pkt = tcv_rnp (RCV_WAIT, sd_rf);
		len = tcv_left (pkt);

		if (len >= 4)
			pktmsg (pkt, len);

		tcv_endp (pkt);
		proceed RCV_WAIT;
}

// ============================================================================

fsm root {

	state RS_INIT:

		word si = 0xffff;

		phys_cc1100 (0, MAX_PACKET_LENGTH);
		phys_uart (1, OSS_PACKET_LENGTH, 0);
		tcv_plug (0, &plug_null);

		sd_rf = tcv_open (WNONE, 0, 0);		// CC1100
		sd_uart = tcv_open (WNONE, 1, 0);	// UART

		tcv_control (sd_uart, PHYSOPT_SETSID, &si);
		tcv_control (sd_rf, PHYSOPT_RXON, NULL);

		if (sd_rf < 0 || sd_uart < 0)
			syserror (ERESOURCE, "ini");

		runfsm radio_receiver;

	state RS_CMD:

		// Process commands (none for now)
		CMD = (oss_hdr_t*)tcv_rnp (RS_CMD, sd_uart);
		PML = tcv_left ((address)CMD);
		if (PML >= sizeof (oss_hdr_t)) {
			PML -= sizeof (oss_hdr_t);
			PMT = (address)(CMD + 1);
			handle_command ();
		}
		tcv_endp ((address)CMD);
		proceed RS_CMD;
}

// ============================================================================

void oss_ack (word status) {

	address msg;

	if ((msg = tcv_wnp (WNONE, sd_uart, sizeof (oss_hdr_t) + 2)) != NULL) {
		((oss_hdr_t*)msg)->code = 0;
		((oss_hdr_t*)msg)->ref = CMD->ref;
		msg [1] = status;
		tcv_endp (msg);
	}
}

// ============================================================================

void handle_command () {

	address msg;

	switch (CMD->code) {

		case 0:
			// Heartbeat, autoconnect
			if (*((lword*)PMT) == OSS_PRAXIS_ID && (msg = 
			    tcv_wnp (WNONE, sd_uart, sizeof (oss_hdr_t) + 4))
				!= NULL) {

				msg [0] = 0;
				*(msg + 1) = (word)(OSS_PRAXIS_ID ^ 
					(OSS_PRAXIS_ID >> 16));
				tcv_endp (msg);
			}
			return;
	}
}

