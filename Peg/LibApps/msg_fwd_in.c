/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_pegStructs.h"
#include "lib_apps.h"

extern nid_t local_host;

void msg_fwd_in (word state, char * buf, word size) {

	char * out_buf = NULL;
	int tagIndex;
	int w_len;

	if ( in_findTag(buf, target) != 0) {
		tagIndex = find_tag (in_fwd(buf, target));
		diag ("FWD tag %d peg %d", (word)in_fwd(buf, target), in_header(buf, rcv));
		if (tagIndex >= 0) {
			w_len = size - sizeof(msgFwdType);
			copy_fwd_msg(state, &out_buf, buf + sizeof(msgFwdType), w_len);	   
		}
	}
	if (out_buf) {
		diag ("FWD in %d %d", in_header(out_buf, msg_type), in_header(out_buf, rcv));
		if(msg4tag.buf)
				ufree (msg4tag.buf); // discard old message
		msg4tag.buf = out_buf;
		msg4tag.tstamp = seconds();
	}
}
