/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

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

void shuffle_hunt (word * a) { // quick hack: asuming 3-el perm. out of 4-el set

	word tmp = rnd() % 4;
	word swp;

	if (tmp < 3) { // pick 3 <-> remove 1
		swp = *(a + tmp); *(a + tmp) = *(a + 3); *(a + 3) = swp;
	}

	if (rnd() & 1) {
		swp = *(a + 1); *(a + 1) = *a; *a = swp;
	}

	if (rnd() & 1) {
		swp = *(a + 2); *(a + 2) = *(a + 1); *(a + 1) = swp;
	}

        if (rnd() & 1) {
		swp = *a; *a = *(a + 2); *(a + 2) = swp;
        }

	diag ("shuffled %u %u %u", *a, *(a +1), *(a + 2));
}

