/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// We shall use this one to test XRS, OEP, and the new forthcoming stuff

#include "globals.h"
#include "threadhdrs.h"

// ============================================================================

strand (outlines, const char)

  const char *cc;
  word nc;

  entry (OL_INIT)

	while (*data == '+')
		data++;

	if (*data == '\0')
		finish;

	for (cc = data; *cc != '+' && *cc != '\0'; cc++);
	obuf = (char*) umalloc ((nc = (cc - data)) + 1);
	strncpy (obuf, data, nc);
	obuf [nc] = '\0';
	savedata ((void*)(data + nc));

  entry (OL_NEXT)

	ab_out (OL_NEXT, obuf);
	proceed (OL_INIT);

endstrand
	
// ============================================================================

const char eetest_menu [] = 
		"EEPROM Test+"
		"Commands:+"
		"a adr int    -> store a word+"
		"b adr lint   -> store a lword+"
		"c adr str    -> store a string+"
		"d adr        -> read word+"
		"e adr        -> read lword+"
		"f adr n      -> read string+"
		"g adr n p    -> write n longwords with p starting at adr+"
		"h adr n b t  -> read n blks of b starting at adr t times+"
		"x frm upt    -> erase eeprom from upto+"
		"s            -> sync eeprom+"
		"w fr ln pat  -> erase-write-read test+"
		"k m          -> write a diag message+"
		"m adr w      -> write word to info flash+"
		"n adr        -> read word from info flash+"
		"o adr        -> erase info flash+"
		"q            -> return to main test+";

thread (eetest)

  entry (EP_INIT)

	join (runstrand (outlines, eetest_menu), EP_RCMD);
	release;

  entry (EP_RCMD)

	err = 0;

	if (ibuf) {
		ufree (ibuf);
		ibuf = NULL;
	}

	ibuf = ab_in (EP_RCMD);
	diag ("EETEST GOT CMD");

	switch (ibuf [0]) {
		case 'a': proceed (EP_SWO);
		case 'b': proceed (EP_SLW);
		case 'c': proceed (EP_SST);
		case 'd': proceed (EP_RWO);
		case 'e': proceed (EP_RLW);
		case 'f': proceed (EP_RST);
		case 'g': proceed (EP_WRI);
		case 'h': proceed (EP_REA);
		case 'x': proceed (EP_ERA);
		case 's': proceed (EP_SYN);
		case 'w': proceed (EP_ETS);
		case 'k': proceed (EP_DIA);
		case 'm': proceed (EP_FLW);
		case 'n': proceed (EP_FLR);
		case 'o': proceed (EP_FLE);
		// To be changed
		case 'q': finish;
	}
	
  entry (EP_RCMD1)

	ab_outf (EP_RCMD1, "Illegal command or parameter");
	proceed (EP_INIT);

  entry (EP_SWO)

	scan (ibuf + 1, "%lu %u", &adr, &w);
	err = ee_write (WNONE, adr, (byte*)(&w), 2);

  entry (EP_SWO1)

	ab_outf (EP_SWO1, "[%d] Stored %u at %lu", err, w, adr);
	proceed (EP_RCMD);

  entry (EP_SLW)

	scan (ibuf + 1, "%lu %lu", &adr, &val);
	err = ee_write (WNONE, adr, (byte*)(&val), 4);

  entry (EP_SLW1)

	ab_outf (EP_SLW1, "[%d] Stored %lu at %lu", err, val, adr);
	proceed (EP_RCMD);

  entry (EP_SST)

	scan (ibuf + 1, "%lu %s", &adr, str);
	len = strlen ((const char*)str);
	if (len == 0)
		proceed (EP_RCMD1);

	err = ee_write (WNONE, adr, str, len);

  entry (EP_SST1)

	ab_outf (EP_SST1, "[%d] Stored %s (%u) at %lu", err, str, len,
		adr);
	proceed (EP_RCMD);

  entry (EP_RWO)

	scan (ibuf + 1, "%lu", &adr);
	err = ee_read (adr, (byte*)(&w), 2);

  entry (EP_RWO1)

	ab_outf (EP_RWO1, "[%d] Read %u (%x) from %lu", err, w, w, adr);
	proceed (EP_RCMD);

  entry (EP_RLW)

	scan (ibuf + 1, "%lu", &adr);
	err = ee_read (adr, (byte*)(&val), 4);

  entry (EP_RLW1)

	ab_outf (EP_RLW1, "[%d] Read %lu (%lx) from %lu",
		err, val, val, adr);
	proceed (EP_RCMD);

  entry (EP_RST)

	scan (ibuf + 1, "%lu %u", &adr, &len);
	if (len == 0)
		proceed (EP_RCMD1);

	str [0] = '\0';
	err = ee_read (adr, str, len);
	str [len] = '\0';

  entry (EP_RST1)

	ab_outf (EP_RST+1, "[%d] Read %s (%u) from %lu",
		err, str, len, adr);
	proceed (EP_RCMD);

  entry (EP_WRI)

	len = 0;
	scan (ibuf + 1, "%lu %u %lu", &adr, &len, &val);
	if (len == 0)
		proceed (EP_RCMD1);
	while (len--) {
		err += ee_write (WNONE, adr, (byte*)(&val), 4);
		adr += 4;
	}

  entry (EP_WRI1)

Done:
	ab_outf (EP_WRI+1, "Done %d", err);
	proceed (EP_RCMD);

  entry (EP_REA)

	len = 0;
	bs = 0;
	nt = 0;
	scan (ibuf + 1, "%lu %u %u %u", &adr, &len, &bs, &nt);
	if (len == 0)
		proceed (EP_RCMD1);
	if (bs == 0)
		bs = 4;

	if (nt == 0)
		nt = 1;

	blk = (byte*) umalloc (bs);

	while (nt--) {

		sl = len;
		ss = adr;
		while (sl--) {
			err += ee_read (ss, blk, bs);
			ss += bs;
		}

	}

	ufree (blk);

	goto Done;

  entry (EP_ERA)

	adr = 0;
	u = 0;
	scan (ibuf + 1, "%lu %lu", &adr, &u);
	err = ee_erase (WNONE, adr, u);
	goto Done;

  entry (EP_SYN)

	err = ee_sync (WNONE);
	goto Done;

  entry (EP_DIA)

	diag ("MSG %d (%x) %u: %s", dcnt, dcnt, dcnt, ibuf+1);
	dcnt++;
	proceed (EP_RCMD);

  entry (EP_FLR)

	scan (ibuf + 1, "%u", &w);
	if (w >= IFLASH_SIZE)
		proceed (EP_RCMD1);
	diag ("IF [%u] = %x", w, if_read (w));
	proceed (EP_RCMD);

  entry (EP_FLW)

	scan (ibuf + 1, "%u %u", &w, &bs);
	if (w >= IFLASH_SIZE)
		proceed (EP_RCMD1);
	if (if_write (w, bs))
		diag ("FAILED");
	else
		diag ("OK");
	goto Done;

  entry (EP_FLE)

	b = -1;
	scan (ibuf + 1, "%d", &b);
	if_erase (b);
	goto Done;

  entry (EP_ETS)

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

  entry (EP_ETS_O)

	if (u == 0) {
		ab_outf (EP_ETS_O, "ERASING ALL FLASH");
	} else {
		ab_outf (EP_ETS_O, "ERASING from %lu (%lx) to %lu (%lx)",
			s, s, u - 4, u - 4);
	}

  entry (EP_ETS_E)

	//w = ee_erase (WNONE, s, u);
	w = ee_erase (EP_ETS_E, s, u);

  entry (EP_ETS_F)

	ab_outf (EP_ETS_F, "ERASE COMPLETE, %u ERRORS", w);

	adr = s;
	w = 0;

  entry (EP_ETS_G)

	if (pat == LWNONE)
		val = adr;
	else
		val = pat;

	//w += ee_write (WNONE, adr, (byte*)(&adr), 4);
	w += ee_write (EP_ETS_G, adr, (byte*)(&val), 4);

	if ((adr & 0xFFF) == 0)
		proceed (EP_ETS_K);

  entry (EP_ETS_M)

	adr += 4;
	if (adr < u)
		proceed (EP_ETS_G);

  entry (EP_ETS_H)

	//w += ee_sync (WNONE);
	w += ee_sync (EP_ETS_H);
	ab_outf (EP_ETS_H, "WRITE COMPLETE, %u ERRORS", w);

	// Start reading
	adr = s;
	w = 0;
	err = 0;

  entry (EP_ETS_I)

	w += ee_read (adr, (byte*)(&val), 4);

	if (pat != LWNONE) {
		if (val != pat) {
			diag ("MISREAD (PATTERN): %x %x => %x %x",
				(word)(adr >> 16), (word) adr,
				(word)(val >> 16), (word) val);
			err++;
		}
	} else {
		if (val != adr) {
			diag ("MISREAD (ADDRESS): %x %x != %x %x",
				(word)(val >> 16), (word) val,
				(word)(adr  >> 16), (word) adr );
			err++;
		}
	}
	if ((adr & 0xFFF) == 0)
		proceed (EP_ETS_L);

  entry (EP_ETS_N)

	adr += 4;
	if (adr < u)
		proceed (EP_ETS_I);

  entry (EP_ETS_J)

	ab_outf (EP_ETS_J, "READ COMPLETE, %u ERRORS, %u MISREADS",
		w, err);

	proceed (EP_RCMD);

  entry (EP_ETS_K)

	ab_outf (EP_ETS_K, "WRITTEN %lu (%lx)", adr, adr);
	proceed (EP_ETS_M);

  entry (EP_ETS_L)

	ab_outf (EP_ETS_L, "READ %lu (%lx)", adr, adr);
	proceed (EP_ETS_N);

endthread

// ============================================================================

const char root_menu [] = 
	"TEST: XRS, EEPROM, ...+"
	"Selection:+"
	"E -> EEPROM test+"
	"Q -> reset board+";

thread (root)

  entry (RS_INIT)

	phys_uart (0, 84, 0);
	tcv_plug (0, &plug_null);
	if ((SFD = tcv_open (WNONE, 0, 0)) < 0)
		syserror (ENODEVICE, "uart");
	w = 0xffff;
	tcv_control (SFD, PHYSOPT_SETSID, &w);
	tcv_control (SFD, PHYSOPT_TXON, NULL);
	tcv_control (SFD, PHYSOPT_RXON, NULL);
	ab_init (SFD);
	// ab_mode (AB_MODE_ACTIVE);
	ee_open ();

	ibuf = NULL;

  entry (RS_RESTART)

	join (runstrand (outlines, root_menu), RS_RCMD);
	release;

  entry (RS_RCMD)

	if (ibuf) {
		ufree (ibuf);
		ibuf = NULL;
	}

	ibuf = ab_in (RS_RCMD);
	diag ("ROOT GOT CMD");

	switch (ibuf [0]) {

		case 'E': {
				join (runthread (eetest), RS_RESTART);
				release;
		}

		case 'Q': {
				reset ();
		}

	}
	
  entry (RS_RCMD1)

	ab_outf (RS_RCMD1, "Illegal command or parameter");
	proceed (RS_RESTART);

endthread


praxis_starter (Node);
