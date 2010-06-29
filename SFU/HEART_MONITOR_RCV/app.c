/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"

#include "phys_cc1100.h"
#include "plug_null.h"

#define	RF_MAXPLEN	48
#define	RF_MSTYPE_HR	1
#define	RF_MSTYPE_SM	2
#define	RF_MSTYPE_SML	7
#define	RF_MSTYPE_AK	3
#define	RF_FRAME	(2 + 4)		// CRC + netid + type + count (mod 256)
#define	RF_NETID	0xCABA		// To make us distinct
#define	RF_RTIMEOUT	2048
#define	RF_PTIMEOUT	4500
#define	RF_HRATE_INT	1024		// Heart rate send interval

char	ibuf [82];
int	sfd;

word	HRate;

word	RetAck;
byte	PCount, Fin;
Boolean	Receiving = NO;

word	sample [8];

#define	UPDATE_EVENT		((word)&(sample[0]))
#define	MONITOR_INTERVAL	(10*1024)

#define	RC_NEX		0
#define	RC_NAK		1
#define	RC_ACK		2

process (receiver, void)

  address packet;

  entry (RC_NEX)

	if (Receiving)
		delay (RF_PTIMEOUT, RC_NAK);

	packet = tcv_rnp (RC_NEX, sfd);
	unwait (WNONE);

	switch (packet [1] & 0xff00) {

	    case RF_MSTYPE_HR << 8:

		HRate = packet [2];
		tcv_endp (packet);
		trigger (UPDATE_EVENT);
		proceed (RC_NEX);

	    case RF_MSTYPE_SML << 8:

		if (!Receiving)
			proceed (RC_NEX);

		if ((byte)(packet [1]) == PCount) {
			if (Fin) {
End:
				Receiving = NO;
				trigger (UPDATE_EVENT);
				proceed (RC_NEX);
			}
			Fin = YES;
			memcpy (sample, packet + 2, 16);
			trigger (UPDATE_EVENT);
			tcv_endp (packet);
			PCount++;
			proceed (RC_ACK);
		}
		tcv_endp (packet);
		proceed (RC_NAK);

	    case RF_MSTYPE_SM << 8:

		if (!Receiving)
			proceed (RC_NEX);

		if ((byte)(packet [1]) == PCount) {
			memcpy (sample, packet + 2, 16);
			trigger (UPDATE_EVENT);
			tcv_endp (packet);
			PCount++;
			proceed (RC_ACK);
		}
		tcv_endp (packet);
		proceed (RC_NAK);

	    default:
		
		tcv_endp (packet);
		proceed (RC_NEX);
	}

  entry (RC_NAK)

	if (Fin == 1 && RetAck >= 4)
		// Force close
		goto End;

	packet = tcv_wnp (RC_NAK, sfd, RF_FRAME);
	RetAck++;
	packet [0] = RF_NETID;
	packet [1] = (RF_MSTYPE_AK << 8) | (byte) (PCount - 1);
	tcv_endp (packet);
	proceed (RC_NEX);

  entry (RC_ACK)

	RetAck = 0;
	packet = tcv_wnp (RC_ACK, sfd, RF_FRAME);
	packet [0] = RF_NETID;
	packet [1] = (RF_MSTYPE_AK << 8) | (byte) (PCount - 1);
	tcv_endp (packet);
	proceed (RC_NEX);

endprocess (1);

#define	MO_UPD		0

process (monitor, void)

  entry (MO_UPD)

	ser_outf (MO_UPD, "[%d] HR: %x, SMP: %x %x %x %x %x %x %x %x\r\n",
		Receiving,
		HRate,
		sample [0],
		sample [1],
		sample [2],
		sample [3],
		sample [4],
		sample [5],
		sample [6],
		sample [7]);

	delay (MONITOR_INTERVAL, MO_UPD);
	wait (UPDATE_EVENT, MO_UPD);

endprocess (1);


#define	RS_INIT		0
#define	RS_RCMF		1
#define	RS_RCMD		2
#define	RS_RCME		3
#define	RS_STA		4
#define	RS_DON		5

process (root, int)

  entry (RS_INIT)

	phys_cc1100 (0, RF_MAXPLEN);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);

	if (sfd < 0) {
		diag ("RF open failed");
		halt ();
	}

	tcv_control (sfd, PHYSOPT_TXON, NULL);
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	RetAck = RF_NETID;
	tcv_control (sfd, PHYSOPT_SETSID, &RetAck);

	fork (monitor, NULL);
	fork (receiver, NULL);

  entry (RS_RCMF)

	ser_out (RS_RCMF,
		"\r\nADCS Receiver Test\r\n"
		"Commands:\r\n"
		"s            -> start receiving\r\n"
	);

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, 132-1);

	switch (ibuf [0]) {
		case 's': proceed (RS_STA);
	}
	
  entry (RS_STA)

	Fin = 0;
	PCount = 0x00;
	RetAck = 0;
	Receiving = YES;

  entry (RS_DON)

	ser_outf (RS_DON, "Done\r\n");
	proceed (RS_RCMD);

endprocess (1);
