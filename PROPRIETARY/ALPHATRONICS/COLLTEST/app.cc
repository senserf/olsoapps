#include "sysio.h"
#include "phys_uart.h"
#include "plug_null.h"
#include "noss.h"

sint	sd_uart = -1;		// UART file descriptor
byte	next_ref = 1;		// Next reference number for outgoing message
word	sernum;

static void update_ref () {

	if (++next_ref == 0)
		next_ref = 2;
}

#define	msghdr	((oss_hdr_t*)msg)

static void smpmsg () {
//
// Issue a 'sample' message to the OSS
//
	address msg;
	sint i;

	if ((msg = tcv_wnp (WNONE, sd_uart, sizeof (oss_hdr_t) +
	    sizeof (msg_sample_t) + 2)) == NULL)
		return;

#define	msgcnt	((msg_sample_t*)(msg + 1))

	msghdr->code = OPC_SAMPLE;
	msghdr->ref = next_ref;
	update_ref ();

	// Some random content
	msgcnt->peg = rnd ();
	msgcnt->tag = rnd ();
	msgcnt->ser = sernum++;
	for (i = 0; i < 32; i++)
		msgcnt->rss [i] = (byte) rnd ();

	tcv_endp (msg);
}

#undef msgcnt

fsm sender {

	state SN_SLEEP:

		delay ((rnd () & 0x0fff) + 4096, SN_SEND);
		release;

	state SN_SEND:

		smpmsg ();
		proceed SN_SLEEP;
}

static void handle_command (oss_hdr_t *cmd, word length) {
//
// For now, we only handle the polling for heartbeat/signature, which is
// needed for autoconnect to work
//
	address msg;

	if (length >= 6 && cmd->code == OPC_HEARTBEAT &&
	    *((lword*)(cmd + 1)) == OSS_PRAXIS_ID &&
	    (msg = tcv_wnp (WNONE, sd_uart, sizeof (oss_hdr_t) + 4)) != NULL) {
		// The heartbeat message has opcode = 0 and ref = 0
		msghdr->code = OPC_HEARTBEAT;
		msghdr->ref = 0;
		// The payload is one word consisting of the two xor'red halves
		// of the praxis Id
		msg [1] = (word)(OSS_PRAXIS_ID ^ (OSS_PRAXIS_ID >> 16));
		tcv_endp (msg);
	}
}

fsm root {

	state RS_INIT:

		word si = WNONE;

		phys_uart (0, OSS_PACKET_LENGTH, 0);
		tcv_plug (0, &plug_null);
		sd_uart = tcv_open (WNONE, 0, 0);

		if (sd_uart < 0)
			syserror (ERESOURCE, "ini");

		// This is needed to mark the NetId field in UART packets
		// as unused (i.e., used as part of the payload)
		tcv_control (sd_uart, PHYSOPT_SETSID, &si);

		runfsm sender;

		leds (0, 2);

	state RS_CMD:

		oss_hdr_t *cmd;

		// Process incoming commands
		cmd = (oss_hdr_t*) tcv_rnp (RS_CMD, sd_uart);
		handle_command (cmd, tcv_left ((address)cmd));
		tcv_endp ((address)cmd);
		proceed RS_CMD;
}
