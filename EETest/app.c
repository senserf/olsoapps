/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	RS_INIT		0
#define	RS_RCMD		10
#define	RS_SWO		20
#define	RS_SLW		30
#define	RS_SST		40
#define	RS_RWO		50
#define	RS_RLW		60
#define	RS_RST		70

address	a;
word	w, len;
lword	lw;
byte	str [129];
char	ibuf [132];

process (root, int)

  entry (RS_INIT)

  entry (RS_RCMD-2)

	ser_out (RS_RCMD-2,
		"\r\nEEPROM Test\r\n"
		"Commands:\r\n"
		"a adr int    -> store a word\r\n"
		"b adr lint   -> store a lword\r\n"
		"c adr str    -> store a string\r\n"
		"d adr        -> read word\r\n"
		"e adr        -> read lword\r\n"
		"f adr n      -> read string\r\n"
		);

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, 132-1);

	if (ibuf [0] == 'a')
		proceed (RS_SWO);

	if (ibuf [0] == 'b')
		proceed (RS_SLW);

	if (ibuf [0] == 'c')
		proceed (RS_SST);

	if (ibuf [0] == 'd')
		proceed (RS_RWO);

	if (ibuf [0] == 'e')
		proceed (RS_RLW);

	if (ibuf [0] == 'f')
		proceed (RS_RST);

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "?????????\r\n");
	proceed (RS_RCMD-2);

  entry (RS_SWO)

	scan (ibuf + 1, "%u %u", &a, &w);
	ee_writew (a, w);

  entry (RS_SWO+1)

	ser_outf (RS_SWO+1, "Stored %u at %u\r\n", w, a);
	proceed (RS_RCMD);

  entry (RS_SLW)

	scan (ibuf + 1, "%u %lu", &a, &lw);
	ee_writel (a, lw);

  entry (RS_SLW+1)

	ser_outf (RS_SLW+1, "Stored %lu at %u\r\n", lw, a);
	proceed (RS_RCMD);

  entry (RS_SST)

	scan (ibuf + 1, "%u %s", &a, str);
	len = strlen (str);
	if (len == 0)
		proceed (RS_RCMD+1);

	ee_writes (a, str, len);

  entry (RS_SST+1)

	ser_outf (RS_SST+1, "Stored %s (%u) at %u\r\n", str, len, a);
	proceed (RS_RCMD);

  entry (RS_RWO)

	scan (ibuf + 1, "%u", &a);
	w = ee_readw (a);

  entry (RS_RWO+1)

	ser_outf (RS_SST+1, "Read %u from %u\r\n", w, a);
	proceed (RS_RCMD);

  entry (RS_RLW)

	scan (ibuf + 1, "%u", &a);
	lw = ee_readl (a);

  entry (RS_RLW+1)

	ser_outf (RS_SST+1, "Read %lu from %u\r\n", lw, a);
	proceed (RS_RCMD);

  entry (RS_RST)

	scan (ibuf + 1, "%u %u", &a, &len);
	if (len == 0)
		proceed (RS_RCMD+1);

	str [0] = '\0';
	ee_reads (a, str, len);
	str [len] = '\0';

  entry (RS_RST+1)

	ser_outf (RS_SST+1, "Read %s (%u) from %u\r\n", str, len, a);
	proceed (RS_RCMD);

endprocess (1)
