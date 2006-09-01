/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_pegStructs.h"
#include "msg_tagStructs.h"
#include "msg_pegs.h"
#include "lib_apps.h"

extern nid_t local_host;

void oss_getTag_in (word state, lword tag, nid_t peg, lword pass) {
	
	char * out_buf = NULL;
	char * get_buf = NULL;
	int size = sizeof(msgFwdType) + sizeof(msgGetTagType);
	
	if(peg == 0 || tag == 0)
	   return;
	// alloc and prepare msg fwd 
	msg_fwd_out (state, &out_buf, size, tag, peg, pass);
	in_header(out_buf, snd) = local_host;
	// get offset for payload - getTag msg
	get_buf = out_buf + sizeof(msgFwdType);
	in_header(get_buf, msg_type) = msg_getTag;
	in_header(get_buf, rcv) = (nid_t)tag;
	in_header(get_buf, snd) = local_host;
	in_getTag(get_buf, passwd) = pass;
	if (peg == local_host) {
		//	it is local - put it in the wroom	
		msg_fwd_in(state, out_buf, size);
	} else {	
		send_msg (out_buf,  size);
	}
	ufree (out_buf);
}
