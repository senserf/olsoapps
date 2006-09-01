/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "lib_apps.h"
#include "msg_tagStructs.h"

void msg_getTag_in (word state, char * buf) {
	char * out_buf = NULL;
	word pass;
	diag("get tag in %d pas %d", in_header(buf, snd), in_getTag(buf, passwd));
	pass = check_passwd (in_getTag(buf, passwd), 0);
	
	msg_getTagAck_out (state, &out_buf, in_header(buf, snd),
			in_header(buf, seq_no), pass);
	net_opt (PHYSOPT_TXON, NULL);
	send_msg (out_buf, sizeof(msgGetTagAckType));
	net_opt (PHYSOPT_TXOFF, NULL);
	ufree (out_buf);
}
