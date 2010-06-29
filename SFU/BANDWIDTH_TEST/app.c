/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"

#include "ser.h"
#include "serf.h"
#include "form.h"
#include "phys_cc1100.h"
#include "plug_null.h"

#define	IBUFLEN		32

int SFD, PacketLength;
int TheThread = 0;
lword Counter;
lword Timer;

const word xpower = 7;

#define	RC_GETIT	0
#define	RC_SHOW		1

thread (receiver)

    address packet;

    entry (RC_GETIT)

	packet = tcv_rnp (RC_GETIT, SFD);
	Counter++;
	tcv_endp (packet);
	if ((Counter & 0xff) != 0)
		proceed (RC_GETIT);

    entry (RC_SHOW)

	ser_outf (RC_SHOW, "RCV: %lu %lu pps\r\n", Counter,
		Counter / (seconds () - Timer));
	proceed (RC_GETIT);

endthread

#define	SN_SEND		0

thread (sender)

    address packet;
    int i;

    entry (SN_SEND)

	packet = tcv_wnp (SN_SEND, SFD, PacketLength);

	packet [1] = (word) (Counter >> 16);
	for (i = 2; i < (PacketLength >> 1); i++)
		packet [i] = (word) Counter;

	Counter++;

	tcv_endp (packet);

	delay (1, SN_SEND);

endthread

/* ================= */
/* End packet sender */
/* ================= */

#define	RS_INIT		0
#define	RS_MENU		1
#define	RS_RCMD		2
#define	RS_ERROR	3
#define	RS_SND		4
#define	RS_RCV		5
#define	RS_QUIT		6

thread (root)

    static char *ibuf;

    entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);
	phys_cc1100 (0, 0);
	tcv_plug (0, &plug_null);
	SFD = tcv_open (NONE, 0, 0);
	if (SFD < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

    entry (RS_MENU)

	ser_out (RS_MENU,
		"\r\nRF Bandwidth Test\r\n"
		"Commands:\r\n"
		"s pl     -> start sending back-to-back packets of pl bytes\r\n"
		"r        -> start receiving\r\n"
		"q        -> stop\r\n"
		);

    entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	if (ibuf [0] == 's')
		proceed (RS_SND);
	if (ibuf [0] == 'r')
		proceed (RS_RCV);
	if (ibuf [0] == 'q')
		proceed (RS_QUIT);

    entry (RS_ERROR)

	ser_out (RS_ERROR, "Illegal command or parameter\r\n");
	proceed (RS_MENU);

    entry (RS_SND)

	if (TheThread != 0)
		proceed (RS_ERROR);

	Counter = 0;
	PacketLength = 0;
	scan (ibuf + 1, "%d", &PacketLength);
	if (PacketLength < 4 || PacketLength > 60 - 4)
		proceed (RS_ERROR);

	PacketLength = (PacketLength + 1) & 0xfffe;

	tcv_control (SFD, PHYSOPT_TXON, NULL);
	tcv_control (SFD, PHYSOPT_SETPOWER, &xpower);
	TheThread = runthread (sender);
	proceed (RS_RCMD);

    entry (RS_RCV)

	if (TheThread != 0)
		proceed (RS_ERROR);

	tcv_control (SFD, PHYSOPT_RXON, NULL);
	Timer = seconds ();
	Counter = 0;
	TheThread = runthread (receiver);
	proceed (RS_RCMD);

    entry (RS_QUIT)

	if (TheThread == 0)
		proceed (RS_ERROR);

	tcv_control (SFD, PHYSOPT_RXOFF, NULL);
	tcv_control (SFD, PHYSOPT_TXOFF, NULL);

	kill (TheThread);
	TheThread = 0;
	proceed (RS_RCMD);

endthread
