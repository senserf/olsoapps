/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

// Testing phys_uart

#include "tcvphys.h"
#include "phys_uart.h"
#include "plug_null.h"

heapmem {100};

static	int 	sfd, i, len;
static	address msg_in, msg_out;

process (root, void)

	nodata;

    entry (0)

	phys_uart (1, 64, 0);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 1, 0);
	if (sfd < 0)
		syserror (0x1001, "open");
    entry (10)

	// Wait for command
	msg_in = tcv_rnp (10, sfd);
	len = tcv_left (msg_in);

    entry (30)

	msg_out = tcv_wnp (30, sfd, len);
	for (i = 0; i < len; i++)
		((byte*)msg_out) [i] = ((byte*)msg_in) [len - i - 1];
	tcv_endp (msg_in);
	tcv_endp (msg_out);
	proceed (10);


endprocess (1)
