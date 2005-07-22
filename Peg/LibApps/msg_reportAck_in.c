/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_pegStructs.h"
#include "lib_apps.h"
#include "diag.h"
#include "lib_apps.h"

void msg_reportAck_in (char * buf) {
	int tagIndex;

	if ((tagIndex = find_tag (in_reportAck(buf, tagId))) < 0) { // not found
		app_diag (D_INFO, "Ack for a goner %lx",
			       in_reportAck(buf, tagId));
		return;
	}

	app_diag (D_DEBUG, "Ack (in %u) for %lx in %u",
		in_reportAck(buf, state),
		tagArray[tagIndex].id, tagArray[tagIndex].state);

	switch (tagArray[tagIndex].state) {
		case reportedTag:
			set_tagState(tagIndex, confirmedTag, NO);
			break;

		case fadingReportedTag:
			set_tagState(tagIndex, fadingConfirmedTag, NO);
			break;

		case goneTag:
			init_tag (tagIndex);
			break;

		default:
			app_diag (D_INFO, "Ignoring Ack for %lx on %u",
				tagArray[tagIndex].id,
				tagArray[tagIndex].state);
	}
}
