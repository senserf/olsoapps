/* ============================================================= */
/* ============================================================= */
/* ============================================================= */

#include "sysio.h"
#include "tcvphys.h"
#include "tcvplug.h"

heapmem {100};

#include "phys_cc1100.h"
#include "phys_uart.h"
#include "plug_null.h"

#define	XMIT_POWER	2

int	RSFD, USFD;

#define	MAXPLEN		60

thread (touart)

#define	TU_NEXT		0

    int len;
    address ipk, opk;

    entry (TU_NEXT)

	ipk = tcv_rnp (TU_NEXT, RSFD);
	len = tcv_left (ipk);
	opk = tcv_wnp (WNONE, USFD, len);

	if (opk == NULL) {
		tcv_endp (ipk);
		proceed (TU_NEXT);
	}

	memcpy (opk, ipk, len);
	tcv_endp (ipk);
	tcv_endp (opk);
	proceed (TU_NEXT);

endthread

thread (root)

#define	RS_INIT		0
#define	RS_READ		1

    word scr;
    int len;
    address ipk, opk;

    entry (RS_INIT)

	phys_cc1100 (0, MAXPLEN);
	phys_uart (1, MAXPLEN, 0);
	tcv_plug (0, &plug_null);

	RSFD = tcv_open (NONE, 0, 0);
	USFD = tcv_open (NONE, 1, 0);

	if (RSFD < 0 || USFD < 0)
		syserror (EHARDWARE, "open");

	scr = 0xffff;

	tcv_control (RSFD, PHYSOPT_SETSID, &scr);
	tcv_control (USFD, PHYSOPT_SETSID, &scr);

	tcv_control (RSFD, PHYSOPT_TXON, NULL);
	tcv_control (RSFD, PHYSOPT_RXON, NULL);
	scr = XMIT_POWER;
	tcv_control (RSFD, PHYSOPT_SETPOWER, &scr);

	tcv_control (USFD, PHYSOPT_TXON, NULL);
	tcv_control (USFD, PHYSOPT_RXON, NULL);

	runthread (touart);

    entry (RS_READ)

	ipk = tcv_rnp (RS_READ, USFD);
	len = tcv_left (ipk);
	opk = tcv_wnp (WNONE, RSFD, len);
	if (opk == NULL) {
		tcv_endp (ipk);
		proceed (RS_READ);
	}
	memcpy (opk, ipk, len);
	tcv_endp (ipk);
	tcv_endp (opk);
	proceed (RS_READ);

endthread
