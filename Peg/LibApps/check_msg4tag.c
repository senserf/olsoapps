/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "msg_pegs.h"
#include "lib_apps.h"

/* if ever this is promoted to a regular functionality,
   we may have a process with a msg buffer waiting for
   trigger (TAG_LISTENS+id) from here, or the msg buffer
   hanging off tagDataType. For now, we keep it as simple as possible:
   check for a msg pending for this tag
*/

void check_msg4tag (lword tag) {
	if (msg4tag.buf && in_header(msg4tag.buf, rcv) == tag) {
		if (seconds() - msg4tag.tstamp <= 77) // do it
			send_msg (msg4tag.buf,
				in_header(msg4tag.buf, msg_type) == msg_getTag ?
				sizeof(msgGetTagType) : sizeof(msgSetTagType));
		ufree (msg4tag.buf);
		msg4tag.buf = NULL;
		msg4tag.tstamp = 0;
	}
}
