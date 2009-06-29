/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "ser.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "plug_null.h"

#define	MAX_PACKET_LENGTH	60
#define	IBUF_LENGTH		82

int 	sfd = -1;

address spkt;

word plen (char *str) {

	word k;
	if ((k = strlen (str)) > MAX_PACKET_LENGTH - 5) {
		str [MAX_PACKET_LENGTH - 4] = '\0';
		k = MAX_PACKET_LENGTH - 5;
	}

	return (k + 6) & 0xfe;
}

#define	SN_SEND		0

strand (sender, char)

  entry (SN_SEND)

	spkt = tcv_wnp (SN_SEND, sfd, plen (data));
	spkt [0] = 0;
	strcpy ((char*)(spkt + 1), data);
	tcv_endp (spkt);
	finish;

endstrand

// ============================================================================

char	*ibuf;

#define	RS_INIT		0
#define	RS_RCMD_M	1
#define	RS_RCMD		2
#define	RS_RCMD_E	3
#define	RS_XMIT		4

thread (root)

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUF_LENGTH);
	phys_cc1100 (0, MAX_PACKET_LENGTH);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (WNONE, 0, 0);
	tcv_control (sfd, PHYSOPT_TXON, NULL);

	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

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
