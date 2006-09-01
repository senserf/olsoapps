/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_tagStructs.h"
#include "msg_pegStructs.h"
#include "msg_pegs.h"
#include "lib_apps.h"

extern nid_t local_host;

void oss_setTag_in (word state, lword tag, nid_t peg, lword pass, nid_t nid,
                    word maj, word min, word pl, word span, lword npass) {

	char * out_buf = NULL;
	char * set_buf = NULL;
	int size = sizeof(msgFwdType) + sizeof(msgSetTagType);

	if(peg == 0 || tag == 0)
	   return;
	// alloc and prepare msg fwd
	msg_fwd_out (state, &out_buf, size, tag, peg, pass);
	in_header(out_buf, snd) = local_host;
	// get offset for payload - getTag msg
	set_buf = out_buf + sizeof(msgFwdType);
	in_header(set_buf, msg_type) = msg_setTag;
	in_header(set_buf, rcv) = (nid_t)tag;
	in_header(set_buf, snd) = local_host;
	in_setTag(set_buf, passwd) = pass;
	in_setTag(set_buf, npasswd) = npass;
	in_setTag(set_buf, node_addr) = nid;
	in_setTag(set_buf, pow_levels) = pl;
	in_setTag(set_buf, freq_maj) = maj;
	in_setTag(set_buf, freq_min) = min;
	in_setTag(set_buf, rx_span) = span;
	if (peg == local_host) {
		//	it is local - put it in the wroom
		msg_fwd_in(state, out_buf, size);
	} else {
		send_msg (out_buf,  size);
	}
	ufree (out_buf);
}
