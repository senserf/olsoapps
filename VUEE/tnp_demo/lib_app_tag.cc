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
	return (m == msg_master);
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
		app_count.snd++;
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

#ifndef __SMURPH__
int info_in (word, address);
#endif

__PUBLF (NodeTag, void, set_tag) (char * buf) {
	// we may need more scrutiny...
	if (in_setTag(buf, node_addr) != 0)
		local_host = in_setTag(buf, node_addr);
	if (in_setTag(buf, pow_levels) != 0) {
		pong_params.rx_lev = max_pwr(in_setTag(buf, pow_levels));
		pong_params.pow_levels = in_setTag(buf, pow_levels);
	}
	if (in_setTag(buf, freq_maj) != 0)
		pong_params.freq_maj = in_setTag(buf, freq_maj);
	if (in_setTag(buf, freq_min) != 0)
		pong_params.freq_min = in_setTag(buf, freq_min);
	if (in_setTag(buf, rx_span) != 0) {
		pong_params.rx_span = in_setTag(buf, rx_span);
		if (pong_params.rx_span == 2)
			pong_params.rx_span = 0;
	}
	if (in_setTag(buf, npasswd) != 0)
		host_passwd = in_setTag(buf, npasswd);
	if (in_setTag(buf, passwd) != 0 && !running (info_in))
		(void)runstrand (info_in, (word)in_setTag(buf, passwd));
}

__PUBLF (NodeTag, word, check_passwd) (lword p1, lword p2) {
	if (host_passwd == p1)
		return 1;
	if (host_passwd == p2)
		return 2;
	app_diag (D_WARNING, "Failed passwd");
	return 0;
}
