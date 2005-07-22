/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_pegStructs.h"
#include "lib_apps.h"

extern lword local_host;

void msg_findTag_in (word state, char * buf) {

	char * out_buf = NULL;
	int tagIndex;
	
	if ( in_findTag(buf, target) == 0) { // summary
		msg_report_out (state, -1, &out_buf);
	} else {
		tagIndex = find_tag (in_findTag(buf, target));
		msg_report_out (state, tagIndex, &out_buf);
		// kludgy: we have an absent tag reported as sumary; change:
		if (tagIndex < 0) {
			in_report(out_buf, tagId) = in_findTag(buf, target);
			in_report(out_buf, state) = noTag;
		}
	}
	if (out_buf) {
		if (local_host == master_host) {
			in_header(out_buf, snd) = local_host;
			oss_report_out (out_buf, oss_fmt);
		} else
			send_msg (out_buf, sizeof(msgReportType));
		ufree (out_buf);
	}
}
