/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"


#define	OM_INIT		00
#define	OM_WRITE	10
process (app_outserial, char)

	static char *ptr;
	static int len;
	int quant;

  entry (OM_INIT)
	ptr = data;
	len = ptr[1] +3; // 3: 0x00, len, 0x04

  entry (OM_WRITE)
	quant = io (OM_WRITE, UART_A, WRITE, (char*)ptr, len);
	ptr += quant;
	len -= quant;
	if (len == 0) {
		/* This is always a fresh buffer allocated dynamically */
		ufree (data);
		finish;
	}
	proceed (OM_WRITE);

endprocess (1)
#undef 	OM_INIT
#undef	OM_WRITE

#define UI_OUTLEN	64
int app_ser_out (word st, char *m, bool cpy) {

	int prcs;
	char *buf;

	if ( m[1] + 3 > UI_OUTLEN) {
		dbg_2 (0xC100 | m[1]); // app_ser_out: > UI_OUTLEN
		if (!cpy)
			ufree (m);
		return 0;
	}
	if ((prcs = running (app_outserial)) != 0) {
		dbg_f (0xC200 | m[1]); // app_ser_out: wait for previous
		if (st == NONE) {
			if (!cpy)
				ufree (m);
			return prcs;
		}
		join (prcs, st);
		release;
	}

	if (cpy) {
		prcs =  m[1] + 3;
		if ((buf = (char *)umalloc (prcs)) == NULL) {
			dbg_f (0xC300 | m[1]); // app_ser_out: wait for mem	
			if (st == NONE)
				return NONE;
			umwait (st);
			release;
		}
		memcpy (buf, m, prcs);
		if (fork (app_outserial, buf) <= 0) {
			ufree (buf);
			dbg_2 (0xC400); // outserial fork failed
		}

	} else {
		if (fork (app_outserial, m) <= 0) {
			ufree (m);
			dbg_2 (0xC401); // outserial fork failed no cpy
		}
	}
	return 0;
}
#undef UI_OUTLEN

