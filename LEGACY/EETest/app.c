/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "storage.h"

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
#define	RS_ERA		95
#define	RS_SYN		97
#define	RS_LED		100
#define	RS_BLI		110
#define	RS_DIA		120
#define	RS_DUM		130
#define	RS_FLR		140
#define	RS_FLW		150
#define	RS_FLE		160
#define	RS_SYS		170
#define	RS_MAL		180

#define	RS_ETS		190
#define	RS_ETS_E	191
#define	RS_ETS_F	192
#define	RS_ETS_G	193
#define	RS_ETS_H	194
#define	RS_ETS_I	195
#define	RS_ETS_J	196
#define	RS_ETS_K	197
#define	RS_ETS_L	198
#define	RS_ETS_M	199
#define	RS_ETS_N	200
#define	RS_ETS_O	201

word	w, err, len, bs, nt, sl, ss, dcnt;
int	b;
lword	s, a, u, lw, pat;
byte	str [129], *blk;
char	ibuf [132];

process (root, int)

  entry (RS_INIT)

	ee_open ();

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
		"x frm upt    -> erase eeprom from upto\r\n"
		"s            -> sync eeprom\r\n"
		"w fr ln pat  -> erase-write-read test\r\n"
		"i led w      -> led status [w = 0, 1, 2]\r\n"
		"j w          -> blinkrate 0-low, 1-high\r\n"
		"k m          -> write a diag message\r\n"
#if DIAG_MESSAGES > 2
		"l            -> dump diag\r\n"
#endif
		"m adr w      -> write word to info flash\r\n"
		"n adr        -> read word from info flash\r\n"
		"o adr        -> erase info flash\r\n"
		"p            -> trigger syserror\r\n"
		"q            -> malloc reset test\r\n"
		);

  entry (RS_RCMD)

	err = 0;
	ser_in (RS_RCMD, ibuf, 132-1);

	switch (ibuf [0]) {
		case 'a': proceed (RS_SWO);
		case 'b': proceed (RS_SLW);
		case 'c': proceed (RS_SST);
		case 'd': proceed (RS_RWO);
		case 'e': proceed (RS_RLW);
		case 'f': proceed (RS_RST);
		case 'g': proceed (RS_WRI);
		case 'h': proceed (RS_REA);
		case 'x': proceed (RS_ERA);
		case 's': proceed (RS_SYN);
		case 'w': proceed (RS_ETS);
		case 'i': proceed (RS_LED);
		case 'j': proceed (RS_BLI);
		case 'k': proceed (RS_DIA);
#if DIAG_MESSAGES > 2
		case 'l': proceed (RS_DUM);
#endif
		case 'm': proceed (RS_FLW);
		case 'n': proceed (RS_FLR);
		case 'o': proceed (RS_FLE);
		case 'p': proceed (RS_SYS);
		case 'q': proceed (RS_MAL);
	}
	
  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "?????????\r\n");
	proceed (RS_RCMD-2);

  entry (RS_SWO)

	scan (ibuf + 1, "%lu %u", &a, &w);
	err = ee_write (WNONE, a, (byte*)(&w), 2);

  entry (RS_SWO+1)

	ser_outf (RS_SWO+1, "[%d] Stored %u at %lu\r\n", err, w, a);
	proceed (RS_RCMD);

  entry (RS_SLW)

	scan (ibuf + 1, "%lu %lu", &a, &lw);
	err = ee_write (WNONE, a, (byte*)(&lw), 4);

  entry (RS_SLW+1)

	ser_outf (RS_SLW+1, "[%d] Stored %lu at %lu\r\n", err, lw, a);
	proceed (RS_RCMD);

  entry (RS_SST)

	scan (ibuf + 1, "%lu %s", &a, str);
	len = strlen (str);
	if (len == 0)
		proceed (RS_RCMD+1);

	err = ee_write (WNONE, a, str, len);

  entry (RS_SST+1)

	ser_outf (RS_SST+1, "[%d] Stored %s (%u) at %lu\r\n", err, str, len, a);
	proceed (RS_RCMD);

  entry (RS_RWO)

	scan (ibuf + 1, "%lu", &a);
	err = ee_read (a, (byte*)(&w), 2);

  entry (RS_RWO+1)

	ser_outf (RS_RWO+1, "[%d] Read %u (%x) from %lu\r\n", err, w, w, a);
	proceed (RS_RCMD);

  entry (RS_RLW)

	scan (ibuf + 1, "%u", &a);
	err = ee_read (a, (byte*)(&lw), 4);

  entry (RS_RLW+1)

	ser_outf (RS_RLW+1, "[%d] Read %lu (%lx) from %lu\r\n", err, lw, lw, a);
	proceed (RS_RCMD);

  entry (RS_RST)

	scan (ibuf + 1, "%lu %u", &a, &len);
	if (len == 0)
		proceed (RS_RCMD+1);

	str [0] = '\0';
	err = ee_read (a, str, len);
	str [len] = '\0';

  entry (RS_RST+1)

	ser_outf (RS_RST+1, "[%d] Read %s (%u) from %lu\r\n", err, str, len, a);
	proceed (RS_RCMD);

  entry (RS_WRI)

	len = 0;
	scan (ibuf + 1, "%lu %u %lu", &a, &len, &lw);
	if (len == 0)
		proceed (RS_RCMD+1);
	while (len--) {
		err += ee_write (WNONE, a, (byte*)(&lw), 4);
		a += 4;
	}

  entry(RS_WRI+1)

Done:
	ser_outf (RS_WRI+1, "Done %d\r\n", err);
	proceed (RS_RCMD);

  entry (RS_REA)

	len = 0;
	bs = 0;
	nt = 0;
	scan (ibuf + 1, "%lu %u %u %u", &a, &len, &bs, &nt);
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
			err += ee_read (ss, blk, bs);
			ss += bs;
		}

	}

	ufree (blk);

	goto Done;

  entry (RS_ERA)

	a = 0;
	u = 0;
	scan (ibuf + 1, "%lu %lu", &a, &u);
	err = ee_erase (WNONE, a, u);
	goto Done;

  entry (RS_SYN)

	err = ee_sync (WNONE);
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
	diag ("IF [%u] = %x", a, if_read (a));
	proceed (RS_RCMD);

  entry (RS_FLW)

	scan (ibuf + 1, "%u %u", &a, &bs);
	if (a >= IFLASH_SIZE)
		proceed (RS_RCMD+1);
	if (if_write (a, bs))
		diag ("FAILED");
	else
		diag ("OK");
	goto Done;

  entry (RS_FLE)

	b = -1;
	scan (ibuf + 1, "%d", &b);
	if_erase (b);
	goto Done;

  entry (RS_SYS)

	syserror (111, "error");

  entry (RS_MAL)

	umalloc (16);
	delay (100, RS_MAL);

  entry (RS_ETS)

	// ERASE-WRITE-READ

	s = 1;
	u = 0;
	pat = LWNONE;
	scan (ibuf + 1, "%lu %lu %lx", &s, &u, &pat);

	// Truncate to longword boundaries
	s = s & 0xfffffffc;
	u = u & 0xfffffffc;
	if (u != 0)
		u += s;
	else
		// Everything
		s = 0;

  entry (RS_ETS_O)

	if (u == 0) {
		ser_out (RS_ETS_O, "ERASING ALL FLASH\r\n");
	} else {
		ser_outf (RS_ETS_O, "ERASING from %lu (%lx) to %lu (%lx)\r\n",
			s, s, u - 4, u - 4);
	}

  entry (RS_ETS_E)

	//w = ee_erase (WNONE, s, u);
	w = ee_erase (RS_ETS_E, s, u);

  entry (RS_ETS_F)

	ser_outf (RS_ETS_F, "ERASE COMPLETE, %u ERRORS\r\n", w);

	a = s;
	w = 0;

  entry (RS_ETS_G)

	if (pat == LWNONE)
		lw = a;
	else
		lw = pat;

	//w += ee_write (WNONE, a, (byte*)(&a), 4);
	w += ee_write (RS_ETS_G, a, (byte*)(&lw), 4);

	if ((a & 0xFFF) == 0)
		proceed (RS_ETS_K);

  entry (RS_ETS_M)

	a += 4;
	if (a < u)
		proceed (RS_ETS_G);

  entry (RS_ETS_H)

	//w += ee_sync (WNONE);
	w += ee_sync (RS_ETS_H);
	ser_outf (RS_ETS_H, "WRITE COMPLETE, %u ERRORS\r\n", w);

	// Start reading
	a = s;
	w = 0;
	err = 0;

  entry (RS_ETS_I)

	w += ee_read (a, (byte*)(&lw), 4);

	if (pat != LWNONE) {
		if (lw != pat) {
			diag ("MISREAD (PATTERN): %x %x",
				(word)(lw >> 16), (word) lw);
			err++;
		}
	} else {
		if (lw != a) {
			diag ("MISREAD (ADDRESS): %x %x != %x %x",
				(word)(lw >> 16), (word) lw,
				(word)(a  >> 16), (word) a );
			err++;
		}
	}
	if ((a & 0xFFF) == 0)
		proceed (RS_ETS_L);

  entry (RS_ETS_N)

	a += 4;
	if (a < u)
		proceed (RS_ETS_I);

  entry (RS_ETS_J)

	ser_outf (RS_ETS_J, "READ COMPLETE, %u ERRORS, %u MISREADS\r\n",
		w, err);

	proceed (RS_RCMD);

  entry (RS_ETS_K)

	ser_outf (RS_ETS_K, "WRITTEN %lu (%lx)\r\n", a, a);
	proceed (RS_ETS_M);

  entry (RS_ETS_L)

	ser_outf (RS_ETS_L, "READ %lu (%lx)\r\n", a, a);
	proceed (RS_ETS_N);

endprocess
