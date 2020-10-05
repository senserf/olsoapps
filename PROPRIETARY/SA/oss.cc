/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "oss.h"
#include "tcvplug.h"

fsm oss_in;

static sint SFD;

static cmdfun_t oss_cmdfun;

byte *oss_out (word st, sint len) {

	address pkt;

	if ((len & 1))
		len++;

	if ((pkt = tcv_wnp (st, SFD, len + 2)) == NULL)
		return NULL;

	return (byte*)pkt;
}

void oss_snd (byte *buf) {

	tcv_endp ((address)buf);
}

void oss_ini (cmdfun_t f) {

	word st;

	oss_cmdfun = f;

	phys_uart (OSS_PHY, OSS_MAXPLEN, OSS_UART);
	tcv_plug (OSS_PLUG, &plug_null);
	if ((SFD = tcv_open (WNONE, OSS_PHY, OSS_PLUG, 0)) < 0)
		syserror (ENODEVICE, "uart");

	st = 0xffff;
	tcv_control (SFD, PHYSOPT_SETSID, &st);

	// Start the driver
	runfsm oss_in;
}

void oss_ers () {

	tcv_erase (SFD, TCV_DSP_XMT);
}

void oss_sig (word st) {

	byte *buf;

	if ((buf = oss_out (st, 4)) == NULL)
		// Cannot happen
		return;

	buf [0] = OSS_CMD_U_POL;
	buf [1] = OSS_MAG0;
	buf [2] = OSS_MAG1;
	buf [3] = OSS_MAG2;

	oss_snd (buf);
}

void oss_ack (word st, byte cmd, byte ser) {

	byte *buf;

	if ((buf = oss_out (st, 2)) == NULL)
		return;

	buf [0] = cmd;
	buf [1] = ser;

	oss_snd (buf);
}

fsm oss_in {

	address pkb;
	word bufl;

	state INIT_0:

		oss_ack (INIT_0, OSS_CMD_RSC, 0);

	state INIT_1:

		oss_sig (INIT_1);

	state LOOP:

		pkb = tcv_rnp (LOOP, SFD);

		if (*((byte*)pkb) == OSS_CMD_NOP)
			goto Ignore;

		bufl = tcv_left (pkb);

	state RESPONSE:

		(*oss_cmdfun) (RESPONSE, (byte*)pkb, bufl);
Ignore:
		tcv_endp (pkb);
		sameas LOOP;
}
