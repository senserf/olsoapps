/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "lib_apps.h"
#include "msg_tagStructs.h"

void msg_setTag_in (word state, char * buf) {
	char * out_buf = NULL;
	word pass = check_passwd (in_setTag(buf, passwd),
			in_setTag(buf, npasswd));
	
	msg_setTagAck_out (state, &out_buf, in_header(buf, snd),
			in_header(buf, seq_no), pass);
    net_opt (PHYSOPT_TXON, NULL);
	send_msg (out_buf, sizeof(msgSetTagAckType));
	net_opt (PHYSOPT_TXOFF, NULL);
	ufree (out_buf);

	if (pass)
		set_tag (buf);
}
