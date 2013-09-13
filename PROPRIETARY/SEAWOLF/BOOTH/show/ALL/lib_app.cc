/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013.			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "diag.h"

#include "net.h"
#include "tarp.h"

/*
 * "Virtual" stuff needed by NET & TARP =======================================
 */
idiosyncratic int tr_offset (headerType *h) { return 0; }

idiosyncratic Boolean msg_isBind (msg_t m) { return NO; }

idiosyncratic Boolean msg_isTrace (msg_t m) { return NO; }

idiosyncratic Boolean msg_isMaster (msg_t m) { return NO; }

idiosyncratic Boolean msg_isNew (msg_t m) { return NO; }

idiosyncratic Boolean msg_isClear (byte o) { return YES; }

idiosyncratic void set_master_chg () { CNOP; }

// ============================================================================

char * get_mem (word state, int len) {
	char * buf = (char *)umalloc (len);
	if (buf == NULL) {
		app_diag_W ("No mem %d", len);
		if (state != WNONE) {
			umwait (state);
			release;
		}
	}
	return buf;
}

void send_msg (char * buf, int size) {
	// it doesn't seem like a good place to filter out
	// local host, but it's convenient, for now...

	// this shouldn't be... WARNING to see why it is needed...
	if (in_header(buf, rcv) == local_host) {
		app_diag_W ("Dropped msg(%u) to lh", in_header(buf, msg_type));
		return;
	}

	if (net_tx (WNONE, buf, size, 0) == 0) {
		app_diag_D ("Sent %u to %u", in_header(buf, msg_type),
			in_header(buf, rcv));
	} else
		app_diag_S ("Tx %u failed", in_header(buf, msg_type));
 }
