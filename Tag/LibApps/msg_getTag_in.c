/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "lib_apps.h"
#include "msg_tagStructs.h"

void msg_getTag_in (word state, char * buf) {
	char * out_buf = NULL;
	
	msg_getTagAck_out (state, &out_buf, in_header(buf, snd),
			in_header(buf, seq_no));
	send_msg (out_buf, sizeof(msgGetTagAckType));
	ufree (out_buf);
}
