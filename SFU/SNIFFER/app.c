/* ============================================================= */
/* ============================================================= */
/* ============================================================= */

#include "sysio.h"
#include "tcvphys.h"
#include "tcvplug.h"

#include "ser.h"
#include "serf.h"
#include "form.h"

#include "plug_null.h"

heapmem {100};

#include "phys_cc1100.h"

#define	MAXPLEN		60

int RSFD;

char obuf [3*MAXPLEN + 8];

thread (root)

#define	RS_INIT		0
#define	RS_READ		1
#define	RS_WRITE	2

    word scr;
    address packet;
    int len, k, i;

    entry (RS_INIT)

	phys_cc1100 (0, MAXPLEN);
	tcv_plug (0, &plug_null);

	RSFD = tcv_open (NONE, 0, 0);

	if (RSFD < 0)
		syserror (EHARDWARE, "open");

	scr = 0x0000;

	tcv_control (RSFD, PHYSOPT_SETSID, &scr);
	tcv_control (RSFD, PHYSOPT_RXON, NULL);

    entry (RS_READ)

Again:
	packet = tcv_rnp (RS_READ, RSFD);
	len = tcv_left (packet);

	if (len > MAXPLEN) {
		tcv_endp (packet);
		goto Again;
	}

	k = 0;

	obuf [k++] = '0' + (len / 10);
	obuf [k++] = '0' + (len % 10);
	obuf [k++] = ':';

	for (i = 0; i < len; i++) {
		scr = ((byte*)packet) [i];
		obuf [k++] = ' ';
		obuf [k++] = HEX_TO_ASCII (scr >> 4);
		obuf [k++] = HEX_TO_ASCII (scr     );
	}

	tcv_endp (packet);

	obuf [k++] = '\r';
	obuf [k++] = '\n';
	obuf [k  ] = '\0';

    entry (RS_WRITE)

	ser_out (RS_WRITE, obuf);
	goto Again;

endthread
