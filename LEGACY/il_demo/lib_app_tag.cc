/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010 			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "msg_tag.h"
#include "vuee_tag.h"

#ifdef __SMURPH__
#endif

#include "diag.h"
#include "app_tag.h"

#ifdef	__SMURPH__

#include "stdattr.h"

#else	/* PICOS */

#include "net.h"

#endif	/* SMURPH or PICOS */

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

void write_mark_t (word what) {
        sensDataType mrk;

        memset (&mrk, 0, sizeof (sensDataType));

	// quit 1 slot before: a sensor write is hanging
        if ((mrk.eslot = sens_data.eslot) + 1 >= EE_SENS_MAX) {
                app_diag_t (D_SERIOUS, "MARK EEPROM FULL");
                return;
        }

        mrk.ee.s.f.mark = what;
        mrk.ee.s.f.status = SENS_ALL;

        mrk.ee.ds = wall_date_t (0);
        mrk.ee.sval[0] = plot_id;
        mrk.ee.sval[1] = is_synced ? 1 : 0;
        mrk.ee.sval[2] = pong_params.freq_maj;

        if (ee_write (WNONE, mrk.eslot * EE_SENS_SIZE,
                                (byte *)&mrk.ee, EE_SENS_SIZE)) {
                app_diag_t (D_SERIOUS, "ee_write mark failed %x %x",
                                (word)(mrk.eslot >> 16), (word)mrk.eslot);

        } else { // all is good
                sens_data.eslot = mrk.eslot +1;
        }
}

