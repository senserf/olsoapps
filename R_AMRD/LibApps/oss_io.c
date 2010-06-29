/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "msg_rtagStructs.h"
#include "lib_app_if.h"
#include "form.h"
#include "tarp.h"

extern lword last_read;
extern word count_read;
extern char * buf_read;
extern bool from_remote;

void oss_rpc_in (word state, char * in_buf) {
	int len;
	char * out_buf;
	if (in_buf == NULL)
		return;
	if (from_remote) {
		if (in_buf[0]  == 'r')
			reset();
		len = sizeof(msgRpcType) + 16; 
		out_buf = get_mem (state, len);
		form (out_buf +sizeof(msgRpcType), "@%ld:%u",
			seconds() - last_read, count_read);
		
	} else { // meter reading in
		len = sizeof(msgRpcType) + strlen (in_buf) +3;
		out_buf = get_mem (state, len);
		out_buf [sizeof(msgRpcType)] = '!';
		out_buf [sizeof(msgRpcType) +1] = ':';
		strcpy (out_buf+sizeof(msgRpcType) +2, in_buf);
	}
	in_header(out_buf, msg_type) = msg_rpc;
	in_header(out_buf, rcv) = master_host;
	send_msg (out_buf, len);
	ufree (out_buf);
}
