/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "app.h"
#include "diag.h"
#include "lib_apps.h"

extern nid_t local_host;

void check_tag (word state, word i, char** buf_out) {
	if (i >= tag_lim) {
		app_diag (D_FATAL, "tagAr bound %u", i);
		return;
	}
	
	if (tagArray[i].id == 0 ||
		seconds() - tagArray[i].lastTime < tag_eventGran)
		return;	

	switch (tagArray[i].state) {
		case newTag:
			app_diag (D_DEBUG, "Delete %lx", tagArray[i].id);
			init_tag (i);
			break;

		case goneTag:
			app_diag (D_DEBUG, "Report gone %lx", tagArray[i].id);
			msg_report_out (state, i, buf_out);
			break;

		case reportedTag:
			app_diag (D_DEBUG, "Set fadingReported %lx",
				tagArray[i].id);
			set_tagState (i, fadingReportedTag, NO);
			break;

		case confirmedTag:
			app_diag (D_DEBUG, "Set fadingConfirmed %lx",
				tagArray[i].id);
			set_tagState (i, fadingConfirmedTag, NO);
			break;

		case fadingReportedTag:
		case fadingConfirmedTag:
			app_diag (D_DEBUG, "Report going %lx",
				tagArray[i].id);
			set_tagState (i, goneTag, YES);
			msg_report_out (state, i, buf_out);
			if (local_host == master_host || master_host == 0)
				init_tag (i); // problematic kludge...
			break;

		default:
			app_diag (D_SERIOUS, "noTag? %lx in %u",
				tagArray[i].id, tagArray[i].state);
	}
}

