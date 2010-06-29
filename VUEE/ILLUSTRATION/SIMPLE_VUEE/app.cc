/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "node.h"

#include "sysio.h"
#include "ser.h"
#include "serf.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "plug_null.h"

#define	MAX_PACKET_LENGTH	60
#define	IBUF_LENGTH		82

#ifdef	__SMURPH__

threadhdr (receiver, Node) {

	states { RC_TRY, RC_SHOW };

	perform;

};

threadhdr (sender, Node) {

	char *data;

	states { SN_SEND };

	perform;

	void setup (char *bf) { data = bf; };
};

threadhdr (root, Node) {

	states { RS_INIT, RS_RCMD_M, RS_RCMD, RS_RCMD_E, RS_XMIT };

	perform;
};

#define	sfd	_dan (Node, sfd)
#define	Count	_dan (Node, Count)
#define	rpkt	_dan (Node, rpkt)
#define	spkt	_dan (Node, spkt)
#define	ibuf	_dan (Node, ibuf)

#else	/* PICOS */

#include "app_data.h"

#define	RC_TRY		0
#define	RC_SHOW		1

#define	SN_SEND		0

#define	RS_INIT		0
#define	RS_RCMD_M	1
#define	RS_RCMD		2
#define	RS_RCMD_E	3
#define	RS_XMIT		4

#endif	/* VUEE or PICOS */

// ============================================================================

void show (word st, address pkt) {

	ser_outf (st, "RCV: %d [%s] pow = %d qua = %d\r\n",
		Count++,
		(char*)(pkt + 1),
		((byte*)pkt) [tcv_left (pkt) - 1],
		((byte*)pkt) [tcv_left (pkt) - 2]
	);
}

thread (receiver)

  entry (RC_TRY)

	rpkt = tcv_rnp (RC_TRY, sfd);

  entry (RC_SHOW)

	show (RC_SHOW, rpkt);
	tcv_endp (rpkt);
	proceed (RC_TRY);

endthread

// ============================================================================

word plen (char *str) {

	word k;
	if ((k = strlen (str)) > MAX_PACKET_LENGTH - 5) {
		str [MAX_PACKET_LENGTH - 4] = '\0';
		k = MAX_PACKET_LENGTH - 5;
	}

	return (k + 6) & 0xfe;
}

strand (sender, char)

  entry (SN_SEND)

	spkt = tcv_wnp (SN_SEND, sfd, plen (data));
	spkt [0] = 0;
	strcpy ((char*)(spkt + 1), data);
	tcv_endp (spkt);
	finish;

endstrand

// ============================================================================

thread (root)

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUF_LENGTH);
	phys_cc1100 (0, MAX_PACKET_LENGTH);
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
