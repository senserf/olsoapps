/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
// This one tests some operations on processes as well as binary XRS
// (aka ABB)
//

#include "node.h"
#include "sysio.h"
#include "plug_null.h"
#include "phys_uart.h"
#include "abb.h"

#define	MAX_PACKET_LENGTH	96

// Commands
#define	ORD_C_SP		0x01	// Start a process
#define	ORD_C_JO		0x02	// Join to a PID
#define	ORD_C_JA		0x03	// Join to all
#define	ORD_C_KI		0x04	// Kill a process

#define	vtoi(a)		((sint)(int)(a))
#define	itov(a)		((void*)(int)(a))

#ifdef	__SMURPH__

strandhdr (spinner, Node) {

	void *data;

	states { SP_START, SP_LOOP };

	void savedata (void *d) { data = d; };
	void setup (void *d) { savedata (d); };

	perform;
};

strandhdr (sjoiner, Node) {

	void *data;

	states { SJ_START, SJ_JOINED, SJ_NONE };

	void savedata (void *d) { data = d; };
	void setup (void *d) { savedata (d); };

	perform;
};

threadhdr (ajoiner, Node) {

	states { AJ_START, AJ_WAIT, AJ_JOINED };

	perform;

};

threadhdr (root, Node) {

	states { RS_INIT, RS_RCMD, RS_ECHO, RS_OK };

	perform;
};

#define	ipacket	_dan (Node, ipacket)
#define	SFD	_dan (Node, SFD)
#define	ibl	_dan (Node, ibl)
#define	cord	_dan (Node, cord)

#else	/* PICOS */

#include "app_data.h"

#define	SP_START	0
#define	SP_LOOP		1

#define	SJ_START	0
#define	SJ_JOINED	1
#define	SJ_NONE		2

#define	AJ_START	0
#define	AJ_WAIT 	1
#define	AJ_JOINED 	2

#define	RS_INIT		0
#define	RS_RCMD		1
#define	RS_ECHO		2
#define	RS_OK 		3

#endif	/* VUEE or PICOS */

static char rep [] = { 
			0x70,	// OK
			0x71,	// Created PID
			0x72,	// Joined
			0x73,	// Joined all
			0x74,	// Terminated
			0x77	// Heart beat
		     };
#define	ORD_R_OK		0
#define	ORD_R_PI		1
#define	ORD_R_JO		2
#define	ORD_R_JA		3
#define	ORD_R_TE		4
#define	ORD_R_HB		5

// ============================================================================

void pidmess (int st, int ord) {
//
// Send a three-byte message with process ID
//
	byte *packet;
	sint p;

	packet = abb_outf (st, 3);
	packet [0] = rep [ord];
	p = getcpid ();
	memcpy (packet + 1, (char*)(&p), 2);
}

// ============================================================================

strand (spinner, void)

  entry (SP_START)

	// Send the initial message
	pidmess (SP_START, ORD_R_PI);

  entry (SP_LOOP)

	// Heart beat message
	pidmess (SP_LOOP, ORD_R_HB);
	delay (vtoi (data), SP_LOOP);

endstrand

strand (sjoiner, void) 

  entry (SJ_START)

	// Send the initial message
	pidmess (SJ_START, ORD_R_PI);

	if (join (vtoi (data), SJ_JOINED) == 0)
		// Nothing to join to
		proceed (SJ_NONE);

	release;

  entry (SJ_JOINED)

	pidmess (SJ_JOINED, ORD_R_JO);
	finish;

  entry (SJ_NONE)

	pidmess (SJ_NONE, ORD_R_TE);
	finish;

endstrand

thread (ajoiner)

  entry (AJ_START)

	pidmess (AJ_START, ORD_R_PI);

  entry (AJ_WAIT)

	if (running (spinner)) {
		pidmess (AJ_WAIT, ORD_R_HB);
		joinall (spinner, AJ_WAIT);
		release;
	}

  entry (AJ_JOINED)

	pidmess (AJ_JOINED, ORD_R_JA);
	finish;

endthread

// ============================================================================

thread (root)

  sint p;
  byte *opacket;

  entry (RS_INIT)

	phys_uart (0, MAX_PACKET_LENGTH, 0);
	tcv_plug (0, &plug_null);
	SFD = tcv_open (WNONE, 0, 0);
	if (SFD < 0)
		syserror (ENODEVICE, "uart");
	ibl = 0xffff;
	tcv_control (SFD, PHYSOPT_SETSID, &ibl);
	// tcv_control (SFD, PHYSOPT_ON, NULL);
	// Active mode
	// tcv_control (SFD, PHYSOPT_TXON, NULL);
	ab_init (SFD);

  entry (RS_RCMD)

	ipacket = abb_in (RS_RCMD, &ibl);

	if (ibl < 1) {
		diag ("Zero length packet");
UF:
		ufree (ipacket);
		proceed (RS_RCMD);
	}

	// The command
	cord = ipacket [0];
		
	switch (cord) {

		case ORD_C_SP:	// Start a spinner

			if (ibl != 3) {
BL:
				diag ("Bad message length");
				goto UF;
			}

			memcpy (&p, ipacket + 1, 2);
			runstrand (spinner, itov (p));
			proceed (RS_OK);

		case ORD_C_JO:	// Start a joiner

			if (ibl != 3)
				goto BL;

			memcpy (&p, ipacket + 1, 2);
			runstrand (sjoiner, itov (p));
			proceed (RS_OK);

		case ORD_C_JA:	// Start a global joiner

			// No arguments
			runthread (ajoiner);
			proceed (RS_OK);

		case ORD_C_KI:	// Kill a process

			if (ibl != 3)
				goto BL;

			memcpy (&p, ipacket + 1, 2);
			kill (p);
			proceed (RS_OK);
	}

	// We get here on default

  entry (RS_ECHO)

	opacket = abb_outf (RS_ECHO, ibl);
	memcpy (opacket, ipacket, ibl);
	goto UF;

  entry (RS_OK)

	opacket = abb_outf (RS_OK, 1);
	opacket [0] = rep [ORD_R_OK];
	goto UF;

endthread
		
praxis_starter (Node);
