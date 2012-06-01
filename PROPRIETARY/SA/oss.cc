#include "oss.h"

fsm oss_in;

static sint SFR, SFU;

static cmdfun_t oss_cmdfun;

byte *oss_outu (word st, sint len) {

	address pkt;

	if ((len & 1))
		len++;

	if ((pkt = tcv_wnp (st, SFU, len)) == NULL)
		return NULL;

	return ((byte*)pkt) + 2;
}

byte *oss_outr (word st, sint len) {

	address pkt;

	if ((len & 1))
		len++;

	if ((pkt = tcv_wnp (st, SFR, len)) == NULL)
		return NULL;

	return ((byte*)pkt) + 2;
}

void oss_send (byte *buf) {

	tcv_endp ((address)(buf - 2));
}

void oss_init (cmdfun_t f) {

	oss_cmdfun = f;

	phys_uart (OSS_PHY, OSS_MAXPLEN, OSS_UART);
	tcv_plug (OSS_PLUG, &plug_boss);
	if ((SFR = tcv_open (WNONE, OSS_PHY, OSS_PLUG, 0)) < 0 ||
	    (SFU = tcv_open (WNONE, OSS_PHY, OSS_PLUG, 1)) < 0  )
		syserror (ENODEVICE, "uart");

	// Start the driver
	runfsm oss_in;
}

void oss_ack (word st, byte cmd) {

	byte *buf;

	if ((buf = oss_outr (st, 1)) == NULL)
		return;

	buf [0] = cmd;

	oss_send (buf);
}

fsm oss_in {

	address pkb;
	word bufl;

	state INIT_0:

		oss_ack (INIT_0, OSS_CMD_NOP);

	state INIT_1:

		oss_ack (INIT_1, OSS_CMD_RSC);

	state LOOP:

		pkb = tcv_rnp (LOOP, SFR);

		if (((byte*)pkb) [2] == OSS_CMD_NOP)
			goto Ignore;

		bufl = tcv_left (pkb);

	state RESPONSE:

		(*oss_cmdfun) (RESPONSE, ((byte*)pkb) + 2, bufl);
Ignore:
		tcv_endp (pkb);
		sameas LOOP;
}