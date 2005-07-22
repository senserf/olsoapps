/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_pegs.h"
#include "lib_apps.h"
#include "diag.h"

static word countTags() {
	word i = tag_lim;
	word j = 0;

	while (i-- > 0) {
		if (tagArray[i].id)
			j++;
	}
	return j;
}

void msg_report_out (word state, int tIndex, char** out_buf) {

	if (master_host == 0) { // nobody to report to
		app_diag (D_INFO, "I'm a Ronin");
		return;
	}
	
	if (*out_buf == NULL)
		*out_buf = get_mem (state, sizeof(msgReportType));
	else
		memset (*out_buf, 0, sizeof(msgReportType));

	in_header(*out_buf, msg_type) = msg_report;
	in_header(*out_buf, rcv) = master_host;
	if (tIndex < 0) {
		in_report(*out_buf, tagId) = 0;
		in_report(*out_buf, tStamp) = master_delta;
		in_report(*out_buf, state) = sumTag;
		in_report(*out_buf, count) = countTags();
		return;
	}
	in_report(*out_buf, tagId) = tagArray[tIndex].id;
	in_report(*out_buf, tStamp) = tagArray[tIndex].evTime + master_delta;
	in_report(*out_buf, state) = tagArray[tIndex].state;
	in_report(*out_buf, count) = ++tagArray[tIndex].count;
}
