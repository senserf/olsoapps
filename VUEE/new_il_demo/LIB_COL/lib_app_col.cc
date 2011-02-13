/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011 			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "msg_col.h"

#include "diag.h"
#include "app_col.h"
#include "app_col_data.h"

#include "net.h"

/*
 * "Virtual" stuff needed by NET & TARP =======================================
 */
idiosyncratic int tr_offset (headerType *h) {
	// Unused ??
	return 0;
}

idiosyncratic Boolean msg_isBind (msg_t m) {
	return NO;
}

idiosyncratic Boolean msg_isTrace (msg_t m) {
	return NO;
}

idiosyncratic Boolean msg_isMaster (msg_t m) {
	return NO; //(m == msg_master);
}

idiosyncratic Boolean msg_isNew (msg_t m) {
	return NO;
}

idiosyncratic Boolean msg_isClear (byte o) {
	return YES;
}
idiosyncratic word guide_rtr (headerType * b) {
	return 0; // don't at all (pegs return 1, 2)
}
idiosyncratic void set_master_chg () {
	app_flags |= 2;
}

// ============================================================================

char * get_mem_t (word state, sint len) {
	char * buf = (char *)umalloc (len);
	if (buf == NULL) {
		app_diag_t (D_SERIOUS, "No mem reset");
		reset();
#if 0
		if (state != WNONE) {
			umwait (state);
			release;
		}
#endif
	}
	return buf;
}

void send_msg_t (char * buf, sint size) {
	// it doesn't seem like a good place to filter out
	// local host, but it's convenient, for now...

	// this shouldn't be... WARNING to see why it is needed...
	if (in_header(buf, rcv) == local_host) {
		app_diag_t (D_WARNING, "Dropped msg(%u) to lh",
			in_header(buf, msg_type));
		return;
	}

	if (net_tx (WNONE, buf, size, 0) == 0) {
        	app_diag_t (D_DEBUG, "Sent %u to %u",
			in_header(buf, msg_type),
			in_header(buf, rcv));
	} else
		app_diag_t (D_SERIOUS, "Tx %u failed",
			in_header(buf, msg_type));
}
 
word max_pwr (word p_levs) {
 	word shift = 0;
 	word level = p_levs & 0x000f;
 	while ((shift += 4) < 16) {
	 	if (level < ((p_levs >> shift) & 0x000f)) 
			level = (p_levs >> shift) & 0x000f;
 	}
 	return level;
}


// only alrm0, alrm1 setable, for now
word handle_c_flags (word c_fl) {

	if (c_fl != 0xFFFF) {
		if (c_fl & 256)
			set_alrm0;
		else
			clr_alrm0;

		if (c_fl & 512)
			set_alrm1;
		else
			clr_alrm1;
	}

	return app_flags;
}

void next_col_time () {

	if (is_synced) { // last coll time doesn't matter
		// ref_date is negative (set time), we need positive:
		lh_time = -wall_date_t (0);

		lh_time %= pong_params.freq_maj;
		lh_time = pong_params.freq_maj - lh_time;

	} else {
		lh_time = lh_time - seconds() + pong_params.freq_maj;
	}
}

lint wall_date_t (lint s) {
        lint x = seconds() - ref_ts - s;

        x = ref_date < 0 ? ref_date - x : ref_date + x;
        return x;
}

