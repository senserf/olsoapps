/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * This is a test for CC1100. What it does is pretty much obvious from the
 * command menu presented after startup.
 */

#include "sysio.h"
#include "tcvphys.h"

heapmem {50, 50};

#include "ser.h"
#include "serf.h"
#include "form.h"

#include "phys_cc1100.h"
#include "plug_null.h"

#define	IBUFLEN		64
#define	MINPLEN	8
#define	MAXPLEN	128
#define	MAGIC	0xDEAF

char	ibuf [IBUFLEN];
int	sfd;
lword	packsen, packser = 0xffffffff, packrec, packlos;
word	intvl = 2048;	// Send interval
word	tkillflag, rkillflag;
word	minpl = 32, maxpl = 32;

static word gen_packet_length (void) {

	static	word rndseed = 12345;

	if (minpl >= maxpl)
		return minpl;

	rndseed = (entropy + 1 + rndseed) * 6751;
	return ((rndseed % (maxpl - minpl + 1)) + minpl);
}

#define	SN_SEND		00
#define	SN_WAIT		10
#define	SN_OUTT		20

process (sender, void)

	static address packet;
	static word plen;
	int i, pl;

	nodata;

  entry (SN_SEND)

	if (tkillflag) {
		tkillflag = 0;
		finish;
	}

	plen = gen_packet_length ();

  entry (SN_WAIT)

	if (tkillflag) {
		tkillflag = 0;
		finish;
	}
	wait ((word) &tkillflag, SN_SEND);

	packet = tcv_wnp (SN_WAIT, sfd, plen);

	packet [0] = 0;
	packet [1] = MAGIC;
	packet [2] = (word)(packsen & 0x0000ffff);
	packet [3] = (word)((packsen >> 16) & 0x0000ffff);

	pl = plen / 2;
	for (i = 4; i < pl; i++)
		packet [i] = (word) entropy;

	tcv_endp (packet);

  entry (SN_OUTT)

	ser_outf (SN_OUTT, "SND %lu, len = %u\r\n", packsen, plen);
	packsen++;
	delay (intvl, SN_SEND);

endprocess (1)

#define	RC_GET		00
#define	RC_OUT		10

process (receiver, void)

	static address packet;
	static word plen;
	lword rsn;

	nodata;

  entry (RC_GET)

	if (rkillflag) {
		rkillflag = 0;
		finish;
	}

	wait ((word) &rkillflag, RC_GET);
	packet = tcv_rnp (RC_GET, sfd);

	if (packet [1] != MAGIC)
		// Ignore
		proceed (RC_GET);

	rsn = (lword) (packet [2]) | ((lword) (packet [3]) << 16);
	if (packser == 0xffffffff || rsn < packser) {
		packrec = 0;
		packlos = 0;
	} else {
		packlos += (rsn - packser - 1);
	}
	packser = rsn;
	packrec++;

	// Info length
	plen = tcv_left (packet) - 2;

  entry (RC_OUT)

	ser_outf (RC_OUT,
	    "RCV: %lu, len = %u, RSSI = %d, LQ = %d [R=%lu,L=%lu]\r\n",
		packser, tcv_left (packet) - 2, 
		    ((byte*)packet) [plen + 1],
		    ((byte*)packet) [plen    ],
			packrec, packlos);

	tcv_endp (packet);
	proceed (RC_GET);

endprocess (1)

static void snd_start () {

	tkillflag = 0;
	tcv_control (sfd, PHYSOPT_TXON, NULL);
	if (!running (sender)) 
		fork (sender, NULL);
}

static void rcv_start () {

	rkillflag = 0;
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	if (!running (receiver))
		fork (receiver, NULL);
}

static void quit () {

	if (running (sender)) {
		tkillflag = 1;
		tcv_control (sfd, PHYSOPT_TXOFF, NULL);
		trigger ((word) &tkillflag);
	}
	
	if (running (receiver)) {
		tcv_control (sfd, PHYSOPT_RXOFF, NULL);
		rkillflag = 1;
		trigger ((word) &rkillflag);
	}
}

#define RS_INIT		 00
#define RS_ERROR	 10
#define RS_RCMD		 20
#define RS_SND		 30
#define RS_DONE		 40
#define RS_RCV		 50
#define RS_QUI		 60
#define RS_INT		 70
#define RS_REL		 80
#define RS_RES		 90
#define RS_POW		100
#define RS_PLE		110
#define RS_STA		120

process (root, int)

  word k, l;

  entry (RS_INIT)

	phys_cc1100 (0, MAXPLEN);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);

	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

	proceed (RS_RCMD-1);

  entry (RS_ERROR)

	ser_out (RS_ERROR, "Illegal command or argument\r\n");
	delay (1024, RS_RCMD-1);
	release;

  entry (RS_RCMD-1)

	ser_out (RS_RCMD-1,
		"\r\nCC1100 Test\r\n"
		"Commands:\r\n"
		"s        -> start sending\r\n"
		"r        -> start receiving\r\n"
		"q        -> quit (sending/receiving)\r\n"
		"i intvl  -> (re)set sending interval (def == 2048 msec)\r\n"
		"x        -> reset lost packet count\r\n"
		"c        -> reset sent packet count to zero\r\n"
		"p pow    -> set transmit power (0-7) (def == 3)\r\n"
		"l frm to -> set output packet length (def == 32-32 bytes)\r\n"
		"z        -> restart the program\r\n"
	);

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	if (ibuf [0] == 's')
		proceed (RS_SND);

	if (ibuf [0] == 'r')
		proceed (RS_RCV);

	if (ibuf [0] == 'q')
		proceed (RS_QUI);

	if (ibuf [0] == 'i')
		proceed (RS_INT);

	if (ibuf [0] == 'x')
		proceed (RS_REL);

	if (ibuf [0] == 'c')
		proceed (RS_RES);

	if (ibuf [0] == 'p')
		proceed (RS_POW);

	if (ibuf [0] == 'l')
		proceed (RS_PLE);

	if (ibuf [0] == 'z')
		proceed (RS_STA);

	proceed (RS_ERROR);

  entry (RS_SND)

	snd_start ();

  entry (RS_DONE)

	ser_outf (RS_DONE, "Done\r\n");
	proceed (RS_RCMD);

  entry (RS_RCV)

	rcv_start ();
	proceed (RS_DONE);

  entry (RS_QUI)

	quit ();
	proceed (RS_DONE);

  entry (RS_INT)

	k = 0;
	scan (ibuf + 1, "%u", &k);
	if (k == 0)
		proceed (RS_ERROR);
	if (k < 64)
		k = 64;
	intvl = k;
	proceed (RS_DONE);

  entry (RS_REL)

	packrec = packlos = 0;
	packser = 0xffffffff;
	proceed (RS_DONE);

  entry (RS_RES)

	packsen = 0;
	proceed (RS_DONE);

  entry (RS_POW)

	k = 8;
	scan (ibuf + 1, "%u", &k);
	if (k > 7)
		proceed (RS_ERROR);
	tcv_control (sfd, PHYSOPT_SETPOWER, &k);
	proceed (RS_DONE);

  entry (RS_PLE)

	k = l = 0;
	scan (ibuf + 1, "%u %u", &k, &l);
	if (k < MINPLEN || k > MAXPLEN)
		proceed (RS_ERROR);
	if (l == 0)
		l = k;
	else if (l > MAXPLEN || l < k)
		proceed (RS_ERROR);
	minpl = k;
	maxpl = l;
	proceed (RS_DONE);

  entry (RS_STA)

	reset ();
	// No return
	proceed (RS_STA);

	nodata;

endprocess (1)
