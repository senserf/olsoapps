/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "vuee_ert.h"

#if UART_TCV
#include "abb.h"
#endif

#if UART_DRIVER || UART_TCV

#if UART_DRIVER
strand (app_outserial, char*)

	sint quant;

  entry (OM_INIT)
	if (*data) { // raw output
		outs_ptr = data;
		outs_len = outs_ptr[1];
		outs_ptr += 2;	// skip BOT, len
	} else {
		outs_ptr = data;
		outs_len = outs_ptr[1] +3; // 3: 0x00, len, 0x04
	}

  entry (OM_WRITE)
	quant = io (OM_WRITE, UART_A, WRITE, (char*)outs_ptr, outs_len);
	outs_ptr += quant;
	outs_len -= quant;
	if (outs_len == 0) {
		/* This is always a fresh buffer allocated dynamically */
		ufree (data);
		finish;
	}
	proceed (OM_WRITE);
endstrand

#else

// UART_TCV
strand (app_outserial, char*)

	entry (OM_INIT)
		// +3: add 0x00 <len> ... 0x04
		abb_out (OM_INIT, (byte *)data, data[1] +3);
		finish;
endstrand

#endif

#define UI_OUTLEN	UART_INPUT_BUFFER_LENGTH
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
		if (st == WNONE) {
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
			if (st == WNONE)
				return WNONE;
			umwait (st);
			release;
		}
		memcpy (buf, m, prcs);
		if (runstrand (app_outserial, buf) == 0) {
			ufree (buf);
			dbg_2 (0xC400); // outserial fork failed
		}

	} else {
		if (runstrand (app_outserial, m) == 0) {
			ufree (m);
			dbg_2 (0xC401); // outserial fork failed no cpy
		}
	}
	return 0;
}
#undef UI_OUTLEN

#else

int app_ser_out (word st, char *m, bool cpy) {
	return 0;
}
#endif
