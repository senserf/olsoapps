/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "net.h"
#include "diag.h"
#include "app.h"
#include "msg_tagStructs.h"

pongParamsType pong_params = {	5120, 	// freq_maj
				1024, 	// freq_min
				0x0987, // 7-8-9 (3 levels)
				1024,	// rx_span
				7	// rx_lev
};

word	app_flags = 0;
lword	host_passwd = 0;
const lword	host_id = 0xBACA0061;
nid_t	net_id = 85; // 0x55 set network id to any value
nid_t	local_host = 97;
nid_t   master_host = 1;

appCountType app_count = {0, 0, 0};

char * get_mem (word state, int len) {
	char * buf = (char *)umalloc (len);
	if (buf == NULL) {
		app_diag (D_WARNING, "Waiting for memory");
		umwait (state);
	}
	return buf;
}
void send_msg (char * buf, int size) {
	// it doesn't seem like a good place to filter out
	// local host, but it's convenient, for now...

	// this shouldn't be... WARNING to see why it is needed...
	if (in_header(buf, rcv) == local_host) {
		app_diag (D_WARNING, "Dropped msg(%u) to lh",
			in_header(buf, msg_type));
		return;
	}

	if (net_tx (NONE, buf, size, 0) == 0) {
		app_count.snd++;
//		app_diag (D_WARNING, "Sent msg %u to %u",
        app_diag (D_DEBUG, "Sent %u to %u",
			in_header(buf, msg_type),
			in_header(buf, rcv));
	} else
		app_diag (D_SERIOUS, "Tx %u failed",
			in_header(buf, msg_type));
 }
 
 word max_pwr (word p_levs) {
 	word shift = 0;
 	word level = p_levs & 0x000f;
 	while ((shift += 4) < 16) {
	 	if (level < (p_levs >> shift) & 0x000f) 
			level = (p_levs >> shift) & 0x000f;
 	}
 	return level;
 }
 
 void set_tag (char * buf) {
	// we may need more scrutiny...
	if (in_setTag(buf, node_addr) != 0)
		local_host = in_setTag(buf, node_addr);
	if (in_setTag(buf, pow_levels) != 0)
	    pong_params.rx_lev = max_pwr(in_setTag(buf, pow_levels));
		pong_params.pow_levels = in_setTag(buf, pow_levels);
	if (in_setTag(buf, freq_maj) != 0)
		pong_params.freq_maj = in_setTag(buf, freq_maj);
	if (in_setTag(buf, freq_min) != 0)
		pong_params.freq_min = in_setTag(buf, freq_min);
	if (in_setTag(buf, rx_span) != 0)
		pong_params.rx_span = in_setTag(buf, rx_span);
	if (in_setTag(buf, npasswd) != 0)
		host_passwd = in_setTag(buf, npasswd);
}

word check_passwd (lword p1, lword p2) {
	if (host_passwd == p1)
		return 1;
	if (host_passwd == p2)
		return 2;
	app_diag (D_WARNING, "Failed passwd");
	return 0;
}
