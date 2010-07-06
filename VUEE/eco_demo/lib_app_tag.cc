/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.			*/
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

__PUBLF (NodeTag, char*, get_mem) (word state, sint len) {
	char * buf = (char *)umalloc (len);
	if (buf == NULL) {
		app_diag (D_SERIOUS, "No mem reset");
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

__PUBLF (NodeTag, void, send_msg) (char * buf, sint size) {
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


__PUBLF (NodeTag, word, handle_c_flags) (word c_fl) {

	if (c_fl != 0xFFFF) {
		if (c_fl & C_FL_EEW_COLL)
			set_eew_coll;
		else
			clr_eew_coll;

		if (c_fl & C_FL_EEW_CONF)
			set_eew_conf;
		else
			clr_eew_conf;

		if (c_fl & C_FL_EEW_OVER)
			set_eew_over;
		else
			clr_eew_over;
	}

	return (is_eew_over ? C_FL_EEW_OVER : 0) |
	       (is_eew_conf ? C_FL_EEW_CONF : 0) |
	       (is_eew_coll ? C_FL_EEW_COLL : 0);
}

__PUBLF (NodeTag, void, next_col_time) () {

	if (is_synced) { // last coll time doesn't matter
		// ref_date is negative (set time), we need positive:
		lh_time = -wall_date (0);

		lh_time %= pong_params.freq_maj;
		lh_time = pong_params.freq_maj - lh_time;

	} else {
		lh_time = lh_time - seconds() + pong_params.freq_maj;
	}
}

__PUBLF (NodeTag, lint, wall_date) (lint s) {
        lint x = seconds() - ref_ts - s;

        x = ref_date < 0 ? ref_date - x : ref_date + x;
        return x;
}

__PUBLF (NodeTag, void, write_mark) (word what) {
        sensDataType mrk;

        memset (&mrk, 0, sizeof (sensDataType));

	// quit 1 slot before: a sensor write is hanging
        if ((mrk.eslot = sens_data.eslot) + 1 >= EE_SENS_MAX) {
                app_diag (D_SERIOUS, "MARK EEPROM FULL");
                return;
        }

        mrk.ee.s.f.mark = what;
        mrk.ee.s.f.status = SENS_ALL;

        mrk.ee.ds = wall_date (0);
        mrk.ee.sval[0] = plot_id;
        mrk.ee.sval[1] = is_synced ? 1 : 0;
        mrk.ee.sval[2] = pong_params.freq_maj;

        if (ee_write (WNONE, mrk.eslot * EE_SENS_SIZE,
                                (byte *)&mrk.ee, EE_SENS_SIZE)) {
                app_diag (D_SERIOUS, "ee_write mark failed %x %x",
                                (word)(mrk.eslot >> 16), (word)mrk.eslot);

        } else { // all is good
                sens_data.eslot = mrk.eslot +1;
        }
}

