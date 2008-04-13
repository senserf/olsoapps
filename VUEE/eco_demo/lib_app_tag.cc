/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007.			*/
/* All rights reserved.							*/
/* ==================================================================== */

#ifdef __SMURPH__
#include "globals_tag.h"
#include "threadhdrs_tag.h"
#endif

#include "diag.h"
#include "app_tag.h"
#include "msg_tag.h"

#ifdef	__SMURPH__

#include "node_tag.h"
#include "stdattr.h"

#else	/* PICOS */

#include "net.h"

#endif	/* SMURPH or PICOS */

#include "attnames_tag.h"

/*
 * "Virtual" stuff needed by NET & TARP =======================================
 */
__PUBLF (NodeTag, int, tr_offset) (headerType *h) {
	// Unused ??
	return 0;
}

__PUBLF (NodeTag, Boolean, msg_isBind) (msg_t m) {
	return NO;
}

__PUBLF (NodeTag, Boolean, msg_isTrace) (msg_t m) {
	return NO;
}

__PUBLF (NodeTag, Boolean, msg_isMaster) (msg_t m) {
	return NO; //(m == msg_master);
}

__PUBLF (NodeTag, Boolean, msg_isNew) (msg_t m) {
	return NO;
}

__PUBLF (NodeTag, Boolean, msg_isClear) (byte o) {
	return YES;
}

__PUBLF (NodeTag, void, set_master_chg) () {
	app_flags |= 2;
}

// ============================================================================

__PUBLF (NodeTag, char*, get_mem) (word state, int len) {
	char * buf = (char *)umalloc (len);
	if (buf == NULL) {
		app_diag (D_WARNING, "No mem %d", len);
		if (state != WNONE) {
			umwait (state);
			release;
		}
	}
	return buf;
}

__PUBLF (NodeTag, void, send_msg) (char * buf, int size) {
	// it doesn't seem like a good place to filter out
	// local host, but it's convenient, for now...

	// this shouldn't be... WARNING to see why it is needed...
	if (in_header(buf, rcv) == local_host) {
		app_diag (D_WARNING, "Dropped msg(%u) to lh",
			in_header(buf, msg_type));
		return;
	}

	if (net_tx (WNONE, buf, size, 0) == 0) {
        	app_diag (D_DEBUG, "Sent %u to %u",
			in_header(buf, msg_type),
			in_header(buf, rcv));
	} else
		app_diag (D_SERIOUS, "Tx %u failed",
			in_header(buf, msg_type));
}
 
__PUBLF (NodeTag, word, max_pwr) (word p_levs) {
 	word shift = 0;
 	word level = p_levs & 0x000f;
 	while ((shift += 4) < 16) {
	 	if (level < ((p_levs >> shift) & 0x000f)) 
			level = (p_levs >> shift) & 0x000f;
 	}
 	return level;
}

// ============================================================================

// IN: mc->sec: # of s. from NOW, can't go back before m_ref time
//     mc->hms.f == 1 <=> go back in time mc->sec seconds
// OUT: *mc: wall time with the input offset (usually 0)

// Different than in Pegs, just for fun and lack of past events
__PUBLF (NodeTag, void, wall_time) (mclock_t *mc) {
	lword lw = seconds() - ref_time;
	word w1, w2, w3, w4;

	if (mc->hms.f &&  (mc->sec && 0x7FFFFFFF) > lw) {
		app_diag (D_SERIOUS, "Ignoring bad offset");
		mc->sec = 0;
	}

	lw += mc->sec;
	mc->sec = 0;

	w1 = lw / (24L * 3600) + ref_clock.hms.d;
	lw %= 24L * 3600;
	w2 = lw / 3600 + ref_clock.hms.h;
	lw %= 3600;
	w3 = lw / 60 + ref_clock.hms.m;
	w4 = lw % 60 + ref_clock.hms.s;

	if (w4 >= 60) {
		w4 -= 60;
		w3++;
	}
	if (w3 >= 60) {
		w3 -= 60;
		w2++;
	}
	if (w2 >= 24) {
		w2 -= 24;
		w1++;
	}
	mc->hms.d = w1; mc->hms.h = w2; mc->hms.m = w3; mc->hms.s = w4;
	mc->hms.f = ref_clock.hms.f;
}

