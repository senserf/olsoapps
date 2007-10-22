/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define	IBUFLEN		132

#include "globals.h"
#include "threadhdrs.h"

thread (root)

    entry (RS_INIT)

    entry (RS_RCMD_M)

	ser_out (RS_RCMD_M,
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
		"m adr w      -> write word to info flash\r\n"
		"n adr        -> read word from info flash\r\n"
		"o adr        -> erase info flash\r\n"
		"l n m        -> led n m: 0-off, 1-on, 2-blink\r\n"
		"r eset\r\n"
	);

    entry (RS_RCMD)

	err = 0;
	ser_in (RS_RCMD, ibuf, IBUFLEN-1);

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
		case 'm': proceed (RS_FLW);
		case 'n': proceed (RS_FLR);
		case 'o': proceed (RS_FLE);
		case 'l':
			  scan (ibuf + 1, "%u %u", &w, &bs);
			  if (w > 3 || bs > 2) // LED3?
				  proceed (RS_RCMD_E);
			  leds (w, bs);
			  ser_out (RS_RCMD, "Done with leds");
			  proceed (RS_RCMD);
		case 'r': reset();
	}

    entry (RS_RCMD_E)

	ser_out (RS_RCMD_E, "?????????\r\n");
	proceed (RS_RCMD_M);
	
    entry (RS_SWO)

	scan (ibuf + 1, "%lu %u", &a, &w);
	err = ee_write (WNONE, a, (byte*)(&w), 2);

    entry (RS_SWO_A)

	ser_outf (RS_SWO_A, "[%d] Stored %u at %lu\r\n", err, w, a);
	proceed (RS_RCMD);

    entry (RS_SLW)

	scan (ibuf + 1, "%lu %lu", &a, &lw);
	err = ee_write (WNONE, a, (byte*)(&lw), 4);

    entry (RS_SLW_A)

	ser_outf (RS_SLW_A, "[%d] Stored %lu at %lu\r\n", err, lw, a);
	proceed (RS_RCMD);

    entry (RS_SST)

	scan (ibuf + 1, "%lu %s", &a, str);
	len = strlen ((char*)str);
	if (len == 0)
		proceed (RS_RCMD_E);

	err = ee_write (WNONE, a, str, len);

  entry (RS_SST_A)

	ser_outf (RS_SST_A, "[%d] Stored %s (%u) at %lu\r\n", err, str, len, a);
	proceed (RS_RCMD);

  entry (RS_RWO)

	w = 0;
	scan (ibuf + 1, "%lu", &a);
	err = ee_read (a, (byte*)(&w), 2);

  entry (RS_RWO_A)

	ser_outf (RS_RWO_A, "[%d] Read %u (%x) from %lu\r\n", err, w, w, a);
	proceed (RS_RCMD);

  entry (RS_RLW)

	scan (ibuf + 1, "%u", &a);
	err = ee_read (a, (byte*)(&lw), 4);

  entry (RS_RLW_A)

	ser_outf (RS_RLW_A, "[%d] Read %lu (%lx) from %lu\r\n", err, lw, lw, a);
	proceed (RS_RCMD);

  entry (RS_RST)

	scan (ibuf + 1, "%lu %u", &a, &len);
	if (len == 0)
		proceed (RS_RCMD_E);

	str [0] = '\0';
	err = ee_read (a, str, len);
	str [len] = '\0';

  entry (RS_RST_A)

	ser_outf (RS_RST_A, "[%d] Read %s (%u) from %lu\r\n", err, str, len, a);
	proceed (RS_RCMD);

  entry (RS_WRI)

	len = 0;
	scan (ibuf + 1, "%lu %u %lu", &a, &len, &lw);
	if (len == 0)
		proceed (RS_RCMD_E);
	while (len--) {
		err += ee_write (WNONE, a, (byte*)(&lw), 4);
		a += 4;
	}

  entry (RS_WRI_A)

Done:
	ser_outf (RS_WRI_A, "Done %d\r\n", err);
	proceed (RS_RCMD);

  entry (RS_REA)

	len = 0;
	bs = 0;
	nt = 0;
	scan (ibuf + 1, "%lu %u %u %u", &a, &len, &bs, &nt);
	if (len == 0)
		proceed (RS_RCMD_E);
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

  entry (RS_FLR)

	scan (ibuf + 1, "%u", &a);
	if (a >= IFLASH_SIZE)
		proceed (RS_RCMD_E);

  entry (RS_FLR_A)

	ser_outf (RS_FLR_A, "IF [%u] = %x\r\n", a, if_read (a));
	proceed (RS_RCMD);

  entry (RS_FLW)

	scan (ibuf + 1, "%u %u", &a, &bs);

	if (a >= IFLASH_SIZE)
		proceed (RS_RCMD_E);

	if_write (a, bs);
	goto Done;

  entry (RS_FLE)

	b = -1;
	scan (ibuf + 1, "%d", &b);
	if_erase (b);
	goto Done;

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

endthread

praxis_starter (Node);
