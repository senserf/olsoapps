/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_pegStructs.h"
#include "lib_apps.h"

extern lword local_host;

void oss_findTag_in (word state, lword tag, lword peg) {

	char * out_buf = NULL;
	int tagIndex;

	if (peg == local_host) {
		if (tag == 0) { // summary
			msg_report_out (state, -1, &out_buf);
		} else {
			tagIndex = find_tag (tag);
			msg_report_out (state, tagIndex, &out_buf);
			// as in msg_findTag_in, kludge summary into
			// missing tag:
			if (tagIndex < 0) {
				in_report(out_buf, tagId) = tag;
				in_report(out_buf, state) = noTag;
			}
		}
		in_header(out_buf, snd) = local_host;
		oss_report_out (out_buf, oss_fmt);

	} else {

		msg_findTag_out (state, &out_buf, tag, peg);
		send_msg (out_buf, sizeof(msgFindTagType));
	}
	ufree (out_buf);
}
