/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "lib_apps.h"
#include "msg_tagStructs.h"

extern nid_t local_host;

void msg_getTagAck_in (word state, char * buf, word size) {
	
	oss_getTag_out (buf, oss_fmt);

	// master stacking came free
	if (master_host != local_host) {
		in_header(buf, rcv) = master_host;
		send_msg (buf, sizeof(msgGetTagAckType));
		in_header(buf, rcv) = local_host; // restore just in case
	}
}
