/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "globals.h"
#include "threadhdrs.h"

#define	MAX_PACKET_LENGTH	60
#define	IBUF_LENGTH		82

/*
 *	This is a simple sender-receiver praxis to be used for illustration of
 *	PicOS -> VUEE conversion.
 */

__PUBLF (Node, void, show) (word st, address pkt) {

	ser_outf (st, "RCV: %d [%s] pow = %d qua = %d\r\n",
		Count++,
		(char*)(pkt + 1),
		((byte*)pkt) [tcv_left (pkt) - 1],
		((byte*)pkt) [tcv_left (pkt) - 2]
	);
}

thread (receiver)

  entry (RC_TRY)

	r_packet = tcv_rnp (RC_TRY, sfd);

  entry (RC_SHOW)

	show (RC_SHOW, r_packet);
	tcv_endp (r_packet);
	proceed (RC_TRY);

endthread

__PUBLF (Node, word, plen) (char *str) {

	word k;
	if ((k = strlen (str)) > MAX_PACKET_LENGTH - 5) {
		str [MAX_PACKET_LENGTH - 4] = '\0';
		k = MAX_PACKET_LENGTH - 5;
	}

	return (k + 6) & 0xfe;
}

strand (sender, char)

  address packet;	// This one is OK as a truly automatic variable

  entry (SN_SEND)

	packet = tcv_wnp (SN_SEND, sfd, plen (data));
	packet [0] = 0;
	strcpy ((char*)(packet + 1), data);
	tcv_endp (packet);
	finish;

endstrand

thread (root)

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUF_LENGTH);
	phys_dm2200 (0, MAX_PACKET_LENGTH);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (WNONE, 0, 0);
	tcv_control (sfd, PHYSOPT_TXON, NULL);
	tcv_control (sfd, PHYSOPT_RXON, NULL);

	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

	runthread (receiver);

  entry (RS_RCMD_M)

	ser_out (RS_RCMD_M,
		"\r\nRF S-R example\r\n"
		"Command:\r\n"
		"s string  -> send the string in a packet\r\n"
	);

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, IBUF_LENGTH-1);

	if (ibuf [0] == 's')
		proceed (RS_XMIT);

  entry (RS_RCMD_E)

	ser_out (RS_RCMD_E, "Illegal command\r\n");
	proceed (RS_RCMD_M);

  entry (RS_XMIT)

	runstrand (sender, ibuf + 1);
	proceed (RS_RCMD);

endthread

praxis_starter (Node);
