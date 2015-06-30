/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2015                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "params.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "cc1100.h"

// Generates dummy event REPORT packets for version 1.0

typedef struct {

	word min, max;

} interval_t;

static const interval_t intervals [] = {
	{     1,     1 },
	{    32,    64 },
	{   256,   512 },
	{   512,  2048 },
	{  1024,  4096 },
	{  4096,  8192 },
	{  8192, 32768 },
	{ 65535, 65535 } };

#define	NINTERVALS	(sizeof (intervals) / sizeof (interval_t))

static sint	sfd = -1;

static byte	seq = 0;	// Packet sequence number
static byte	inx = 0;	// Interval index
static word	ref = 0;	// Event reference number

static word genword (word min, word max) {

	return (rnd () % (max - min + 1)) + min;
}

// ============================================================================

static void fill_event_packet (address p) {

	word w;

	((byte*)p) [2] = PKTYPE_REPO;
	((byte*)p) [3] = seq++;

	// Sender Peg
	p [2] = genword (PEG_MIN, PEG_MAX);
	// Recipient == the master
	p [3] = 1;
	// Hop counts
	p [4] = 0x0200;
	// Reference
	p [5] = ref++;
	// Tag
	p [6] = genword (TAG_MIN, TAG_MAX);
	// RSSI (fixed) + ago (0)
	p [7] = 0x0061;
	// Power
	((byte*)p) [16] = 0x73;
	// Button
	((byte*)p) [17] = seq;
	// The rest
	p  [9] = 0x0204;
	p [10] = 0x09bd;
	p [11] = 0x0100;
}

fsm receiver {

	state RECEIVE:

		address pkt;

		pkt = tcv_rnp (RECEIVE, sfd);
		tcv_endp (pkt);

	sameas RECEIVE;
}

fsm root {

	byte led = 0;

	state INIT_ALL:

		word par;

		phys_cc1100 (0, CC1100_MAXPLEN);
		tcv_plug (0, &plug_null);
		if ((sfd = tcv_open (NONE, 0, 0)) < 0)
			syserror (ERESOURCE, "ini");

		par = NETID;
		tcv_control (sfd, PHYSOPT_SETSID, &par);
		tcv_control (sfd, PHYSOPT_ON, NULL);
		runfsm receiver;

		// Determine the interval
		if ((inx = (byte)(host_id & 0xf) - 1) >= NINTERVALS)
			inx = 3;

		leds (0, 0);
		leds (1, 0);
		leds (2, 0);
		
	state SENDER:

		led = 1 - led;
		leds (0, led);

		delay (genword (intervals [inx] . min, intervals [inx] . max),
			NEXTEVENT);
		release;

	state NEXTEVENT:

		address pkt;

		pkt = tcv_wnp (NEXTEVENT, sfd, 26);
		fill_event_packet (pkt);
		tcv_endp (pkt);
		proceed SENDER;
}
