/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "diag.h"
#include "app.h"
#include "msg_pegs.h"
#include "lib_apps.h"

extern lword local_host;

void msg_pong_in (word state, char * buf, word rssi) {
	char * out_buf = NULL; // is static needed / better here? (state)
	int tagIndex;

	lword key = ((lword)rssi << 8) | (lword)in_pong(buf, level);
	key = (key << 16) | in_header(buf, snd);
	// key = rssi(8b) pow_lev(8) tagId(16)
	app_diag (D_DEBUG, "Pong %lx", key);

	if ((tagIndex = find_tag (key)) < 0) { // not found
		insert_tag (key);
		return;
	}
	switch (tagArray[tagIndex].state) {
		case noTag:
			app_diag (D_SERIOUS, "NoTag error");
			return;

		case newTag:
			msg_report_out (state, tagIndex, &out_buf);
			if (master_host == local_host)
				set_tagState (tagIndex, confirmedTag, NO);
			else
				set_tagState (tagIndex, reportedTag, NO);
			break;

		case reportedTag:
			tagArray[tagIndex].lastTime = seconds();
			msg_report_out (state, tagIndex, &out_buf);
			// this shit may happen if the master is changed to
			// be local host:
			if (master_host == local_host)
				set_tagState (tagIndex, confirmedTag, NO);
			break;

		case confirmedTag:
			tagArray[tagIndex].lastTime = seconds();
			app_diag (D_DEBUG, "Ignoring Tag %lx",
				       	tagArray[tagIndex].id);
			break;
			
		case fadingReportedTag:
			set_tagState(tagIndex, reportedTag, NO);
			break;

		case fadingConfirmedTag:
			set_tagState(tagIndex, confirmedTag, NO);
			break;

		case goneTag:
			set_tagState(tagIndex, newTag, YES);
			break;

		default:
			app_diag (D_SERIOUS, "Tag state?(%u) Suicide!",
				tagArray[tagIndex].state);
			reset();
	}

	if (out_buf) {
		if (local_host == master_host) {
			// master with tags - hack the sender
			in_header(out_buf, snd) = local_host;
			oss_report_out (out_buf, oss_fmt);
		} else
			send_msg (out_buf, sizeof(msgReportType));
		ufree (out_buf);
		out_buf = NULL;
	}
}
