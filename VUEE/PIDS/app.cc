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

#define	MAX_PACKET_LENGTH	60
#define	IBUF_LENGTH		82

#define	vtoi(a)		((int)(a))
#define	itov(a)		((void*)(a))

#ifdef	__SMURPH__

strandhdr (spinner, Node) {

	void *data;

	states { SP_START };

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

	states { AJ_START };

	perform;

};

threadhdr (root, Node) {

	states { RS_INIT, RS_RCMD, RS_ECHO };

	perform;
};

#define	ibuf	_dan (Node, ibuf)
#define	SFD	_dan (Node, SFD)
#define	IBL	_dan (Node, IBL)

#else	/* PICOS */

#include "app_data.h"

#define	SP_START	0

#define	SJ_START	0
#define	SJ_JOINED	1

#define	AJ_START	0

#define	RS_INIT		0
#define	RS_RCMD		1
#define	RS_ECHO		2

#endif	/* VUEE or PICOS */

// ============================================================================

strand (spinner, void)

  entry (SP_START)

	if (vtoi (data) == 0) {
		diag ("spinner %d terminating", getcpid ());
		finish;
	}
	
	diag ("spinner %d %d", getcpid (), vtoi (data));
	savedata (itov (vtoi (data) - 1));
	delay (2048, SP_START);

endstrand

strand (sjoiner, void) 

  int k;

  entry (SJ_START)

	k = join (vtoi (data), SJ_JOINED);
	diag ("sjoiner %d <- %d [%d], r = %d", getcpid (), vtoi (data), k,
		running (spinner));
	if (k == 0) {
		diag ("Nothing to join, terminating");
		finish;
	}
	release;

  entry (SJ_JOINED)

	diag ("sjoiner %d joined!", getcpid ());
	finish;

endstrand

thread (ajoiner)

  entry (AJ_START)

	if (running (spinner)) {
		diag ("ajoiner still waiting");
		joinall (spinner, AJ_START);
		release;
	}

	diag ("ajoiner exits");
	finish;

endthread

// ============================================================================

thread (root)

  int p; word w; byte b;

  entry (RS_INIT)

	phys_uart (0, 96, 0);
	tcv_plug (0, &plug_null);
	SFD = tcv_open (WNONE, 0, 0);
	if (SFD < 0)
		syserror (ENODEVICE, "uart");
	w = 0xffff;
	tcv_control (SFD, PHYSOPT_SETSID, &w);
	tcv_control (SFD, PHYSOPT_TXON, NULL);
	tcv_control (SFD, PHYSOPT_RXON, NULL);
	ab_init (SFD);
	ab_mode (AB_MODE_PASSIVE);

  entry (RS_RCMD)

	if ((ibuf = (byte*)abb_in (RS_RCMD, &IBL)) == NULL)
		proceed (RS_RCMD);

	if (IBL != 2) {
		// Echo the buffer reversed if not our command, we only
		// expect 2 bytes
		assert (IBL, "Received zero length buffer");
		for (w = 0; w < IBL/2; w++) {
			b = ibuf [w];
			ibuf [w] = ibuf [IBL - 1 - w];
			ibuf [IBL - 1 - w] = b;
		}
		proceed (RS_ECHO);
	}

	p = ibuf [1];
			
	switch (ibuf [0]) {

		case 1:	// Start a spinner

			p = ibuf [1];
			runstrand (spinner, itov (p));
			ibuf [0] = 7;
			proceed (RS_RCMD);

		case 2: // Start a joiner

			runstrand (sjoiner, itov (p));
			ibuf [0] = 7;
			proceed (RS_RCMD);

		case 3: // Start a global joiner

			runthread (ajoiner);
			ibuf [0] = 7;
			proceed (RS_RCMD);

		default:

			ibuf [0] = 0xfe;

	}

  entry (RS_ECHO)

	if (abb_out (RS_ECHO, (address) ibuf, IBL))
		diag ("ABB_OUT ERR");

	proceed (RS_RCMD);

endthread
		
praxis_starter (Node);
