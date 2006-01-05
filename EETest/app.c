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
#define	RS_WRI		80
#define	RS_REA		90
#define	RS_LED		100
#define	RS_BLI		110
#define	RS_DIA		120
#define	RS_DUM		130
#define	RS_FLR		140
#define	RS_FLW		150
#define	RS_FLE		160
#define	RS_SYS		170
#define	RS_MAL		180

word	a, w, len, bs, nt, sl, ss, dcnt;
lword	lw;
byte	str [129], *blk;
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
		"g adr n p    -> write n longwords with p starting at adr\r\n"
		"h adr n b t  -> read n blks of b starting at adr t times\r\n"
		"i led w      -> led status [w = 0, 1, 2]\r\n"
		"j w          -> blinkrate 0-low, 1-high\r\n"
		"k m          -> write a diag message\r\n"
#if DIAG_MESSAGES > 2
		"l            -> dump diag\r\n"
#endif
		"m adr w      -> write word to info flash\r\n"
		"n adr        -> read word from info flash\r\n"
		"o            -> erase info flash\r\n"
		"p            -> trigger syserror\r\n"
		"q            -> malloc reset test\r\n"
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

	if (ibuf [0] == 'g')
		proceed (RS_WRI);

	if (ibuf [0] == 'h')
		proceed (RS_REA);

	if (ibuf [0] == 'i')
		proceed (RS_LED);

	if (ibuf [0] == 'j')
		proceed (RS_BLI);

	if (ibuf [0] == 'k')
		proceed (RS_DIA);

#if DIAG_MESSAGES > 2
	if (ibuf [0] == 'l')
		proceed (RS_DUM);
#endif

	if (ibuf [0] == 'm')
		proceed (RS_FLW);

	if (ibuf [0] == 'n')
		proceed (RS_FLR);

	if (ibuf [0] == 'o')
		proceed (RS_FLE);

	if (ibuf [0] == 'p')
		proceed (RS_SYS);

	if (ibuf [0] == 'q')
		proceed (RS_MAL);

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "?????????\r\n");
	proceed (RS_RCMD-2);

  entry (RS_SWO)

	scan (ibuf + 1, "%u %u", &a, &w);
	ee_write (a, (byte*)(&w), 2);

  entry (RS_SWO+1)

	ser_outf (RS_SWO+1, "Stored %u at %u\r\n", w, a);
	proceed (RS_RCMD);

  entry (RS_SLW)

	scan (ibuf + 1, "%u %lu", &a, &lw);
	ee_write (a, (byte*)(&lw), 4);

  entry (RS_SLW+1)

	ser_outf (RS_SLW+1, "Stored %lu at %u\r\n", lw, a);
	proceed (RS_RCMD);

  entry (RS_SST)

	scan (ibuf + 1, "%u %s", &a, str);
	len = strlen (str);
	if (len == 0)
		proceed (RS_RCMD+1);

	ee_write (a, str, len);

  entry (RS_SST+1)

	ser_outf (RS_SST+1, "Stored %s (%u) at %u\r\n", str, len, a);
	proceed (RS_RCMD);

  entry (RS_RWO)

	scan (ibuf + 1, "%u", &a);
	ee_read (a, (byte*)(&w), 2);

  entry (RS_RWO+1)

	ser_outf (RS_SST+1, "Read %u (%x) from %u\r\n", w, w, a);
	proceed (RS_RCMD);

  entry (RS_RLW)

	scan (ibuf + 1, "%u", &a);
	ee_read (a, (byte*)(&lw), 4);

  entry (RS_RLW+1)

	ser_outf (RS_SST+1, "Read %lu (%lx) from %u\r\n", lw, lw, a);
	proceed (RS_RCMD);

  entry (RS_RST)

	scan (ibuf + 1, "%u %u", &a, &len);
	if (len == 0)
		proceed (RS_RCMD+1);

	str [0] = '\0';
	ee_read (a, str, len);
	str [len] = '\0';

  entry (RS_RST+1)

	ser_outf (RS_SST+1, "Read %s (%u) from %u\r\n", str, len, a);
	proceed (RS_RCMD);

  entry (RS_WRI)

	len = 0;
	scan (ibuf + 1, "%u %u %lu", &a, &len, &lw);
	if (len == 0)
		proceed (RS_RCMD+1);
	while (len--) {
		ee_write (a, (byte*)(&lw), 4);
		a += 4;
	}

  entry(RS_WRI+1)

Done:
	ser_out (RS_WRI+1, "Done\r\n");
	proceed (RS_RCMD);

  entry (RS_REA)

	len = 0;
	bs = 0;
	nt = 0;
	scan (ibuf + 1, "%u %u %u %u", &a, &len, &bs, &nt);
	if (len == 0)
		proceed (RS_RCMD+1);
	if (bs == 0)
		bs = 4;

	if (nt == 0)
		nt = 1;

	blk = (byte*) umalloc (bs);

	while (nt--) {

		sl = len;
		ss = a;
		while (sl--) {
			ee_read (ss, blk, bs);
			ss += bs;
		}

	}

	goto Done;

  entry (RS_LED)

	scan (ibuf + 1, "%u %u", &bs, &nt);
	leds (bs, nt);
	proceed (RS_RCMD);

  entry (RS_BLI)

	scan (ibuf + 1, "%u", &bs);
	fastblink (bs);
	proceed (RS_RCMD);

  entry (RS_DIA)

	diag ("MSG %d (%x) %u: %s", dcnt, dcnt, dcnt, ibuf+1);
	dcnt++;
	proceed (RS_RCMD);

#if DIAG_MESSAGES > 2

  entry (RS_DUM)

	diag_dump ();
	proceed (RS_RCMD);
#endif

  entry (RS_FLR)

	scan (ibuf + 1, "%u", &a);
	if (a >= IFLASH_SIZE)
		proceed (RS_RCMD+1);
	diag ("IF [%u] = %x", a, IFLASH [a]);
	proceed (RS_RCMD);

  entry (RS_FLW)

	scan (ibuf + 1, "%u %u", &a, &bs);
	if (a >= IFLASH_SIZE)
		proceed (RS_RCMD+1);
	if_write (a, bs);
	goto Done;

  entry (RS_FLE)

	if_erase ();
	goto Done;

  entry (RS_SYS)

	syserror (111, "error");

  entry (RS_MAL)

	umalloc (16);
	delay (100, RS_MAL);

endprocess (1)
