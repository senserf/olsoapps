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
#include "ab.h"

#define	MAX_PACKET_LENGTH	96

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

	states { SJ_START, SJ_JOINED };

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

#define	buff	_dan (Node, buff)
#define	SFD	_dan (Node, SFD)

#else	/* PICOS */

#include "app_data.h"

#define	SP_START	0
#define	SP_LOOP		1

#define	SJ_START	0
#define	SJ_JOINED	1

#define	AJ_START	0
#define	AJ_WAIT 	1
#define	AJ_JOINED 	2

#define	RS_INIT		0
#define	RS_RCMD		1
#define	RS_ECHO		2
#define	RS_OK 		3

#endif	/* VUEE or PICOS */

// ============================================================================

void pidmess (int st, const char *m) {
//
// Send a three-byte message with process ID
//
	ab_outf (st, "%s: %d", m, getcpid ());
}

// ============================================================================

strand (spinner, void*)

  entry (SP_START)

	// Send the initial message
	pidmess (SP_START, "Started spinner");


  entry (SP_LOOP)

	// Heart beat message
	pidmess (SP_LOOP, "Spinner heart beat");
	delay (vtoi (data), SP_LOOP);

endstrand

strand (sjoiner, void*) 

  entry (SJ_START)

	// Send the initial message
	pidmess (SJ_START, "Started joiner");

	join (vtoi (data), SJ_JOINED);
	release;

  entry (SJ_JOINED)

	pidmess (SJ_JOINED, "Joined");
	finish;

endstrand

thread (ajoiner)

  entry (AJ_START)

	pidmess (AJ_START, "Started global joiner");

  entry (AJ_WAIT)

	if (running (spinner)) {
		pidmess (AJ_WAIT, "Global joiner still running");
		joinall (spinner, AJ_WAIT);
		release;
	}

  entry (AJ_JOINED)

	pidmess (AJ_JOINED, "Global joiner joined");
	finish;

endthread

// ============================================================================

thread (root)

  sint p;
  word w;

  entry (RS_INIT)

	phys_uart (0, MAX_PACKET_LENGTH, 0);
	tcv_plug (0, &plug_null);
	SFD = tcv_open (WNONE, 0, 0);
	if (SFD < 0)
		syserror (ENODEVICE, "uart");
	w = 0xffff;
	tcv_control (SFD, PHYSOPT_SETSID, &w);
	// tcv_control (SFD, PHYSOPT_TXON, NULL);
	// tcv_control (SFD, PHYSOPT_RXON, NULL);
	ab_init (SFD);

  entry (RS_RCMD)

	buff = ab_in (RS_RCMD);

	switch (buff [0]) {

		case 's':	// Start a spinner

			if (scan (buff + 1, "%u", &w) != 1)
				proceed (RS_ECHO);

			runstrand (spinner, itov (w));
			proceed (RS_OK);

		case 'j':	// Start a joiner

			if (scan (buff + 1, "%d", &p) != 1)
				proceed (RS_ECHO);

			runstrand (sjoiner, itov (p));
			proceed (RS_OK);

		case 'a':	// Start a global joiner

			// No arguments
			runthread (ajoiner);
			proceed (RS_OK);

		case 'k':	// Kill a process

			if (scan (buff + 1, "%d", &p) != 1)
				proceed (RS_ECHO);

			kill (p);
			proceed (RS_OK);
	}

  entry (RS_ECHO)

	ab_outf (RS_ECHO, "Illegal command: %s", buff);

Ret:	ufree (buff);
	proceed (RS_RCMD);
	
  entry (RS_OK)

	ab_outf (RS_OK, "OK");
	goto Ret;

endthread
		
praxis_starter (Node);
