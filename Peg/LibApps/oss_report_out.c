/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_pegStructs.h"
#include "msg_tagStructs.h"
#include "app.h"
#include "ser.h"
#include "diag.h"
#include "form.h"

#define OO_RETRY 00
process (oss_out, char)

	entry (OO_RETRY)
		ser_out (OO_RETRY, data);
		ufree (data);
		finish;
endprocess (0)
#undef  OO_RETRY

static char * stateName (unsigned state) {
	switch ((tagStateType)state) {
		case noTag:
			return "noTag";
		case newTag:
			return "newTag";
		case reportedTag:
			return "reportedTag";
		case confirmedTag:
			return "confirmedTag";
		case fadingReportedTag:
			return "fadingReportedTag";
		case fadingConfirmedTag:
			return "fadingConfirmedTag";
		case goneTag:
			return "goneTag";
		case sumTag:
			return "sumTag";
		default:
			return "unknown?";
	}
}
	
void oss_report_out (char * buf, word fmt) {
	char * lbuf = NULL;

	switch (fmt) {
		case OSS_HT:
			lbuf = form (NULL, "Tag %u at Peg %u at %lu: %s(%u)"\
					" with rss:pl (%u:%u)\r\n",
				*((word*)&in_report(buf, tagId) +1), 
				in_header(buf, snd),
				in_report(buf, tStamp),
				stateName (in_report(buf, state)),
				in_report(buf, state),
				(*(word*)&in_report(buf, tagId)) >>8,
				(*(word*)&in_report(buf, tagId)) & 0x00ff);
			break;

		case OSS_TCL:
			lbuf = form (NULL, "Type%u Peg%u T%u %u %u %lu %u\n",
				in_header(buf, msg_type),
				in_header(buf, snd),
				*((word*)&in_report(buf, tagId) +1),
				(*(word*)&in_report(buf, tagId)) & 0x00ff,
				(*(word*)&in_report(buf, tagId)) >>8,
				in_report(buf, tStamp),
				in_report(buf, state) == goneTag ? 0 : 1);
			break;

		default:
			app_diag (D_SERIOUS, "**Unsupported fmt %u", fmt);
			return;

	}
	if (fork (oss_out, lbuf) == 0 ) {
		app_diag (D_SERIOUS, "oss_out failed");
		ufree (lbuf);
	}
}

void oss_setTag_out (char * buf, word fmt) {
	char * lbuf = NULL;

	switch (fmt) {
		case OSS_HT:
			lbuf = form (NULL, "Tag %u at Peg %u Set ack %u ack seq %u\r\n", 
				in_header(buf, snd),
				in_header(buf, rcv),
				in_setTagAck(buf, ack),
				in_setTagAck(buf, ackSeq));
			break;

		case OSS_TCL:
			// do nothing
			return;

		default:
			app_diag (D_SERIOUS, "**Unsupported fmt %u", fmt);
			return;

	}
	if (fork (oss_out, lbuf) == 0 ) {
		app_diag (D_SERIOUS, "oss_out failed");
		ufree (lbuf);
	}
}

void oss_getTag_out (char * buf, word fmt) {
	char * lbuf = NULL;

	switch (fmt) {
		case OSS_HT:
			lbuf = form (NULL, "Tag %u at Peg %u Get time %lu host id %lu"\
					" pow lev %u freq maj %u freq min %u span %u ack seq %u\r\n", 
				in_header(buf, snd),
				in_header(buf, rcv),
				in_getTagAck(buf, ltime),
				in_getTagAck(buf, host_id),
				in_getTagAck(buf, pow_levels),
				in_getTagAck(buf, freq_maj),
				in_getTagAck(buf, freq_min),
				in_getTagAck(buf, rx_span),
				in_getTagAck(buf, ackSeq));
			break;

		case OSS_TCL:
			// do nothing
			return;

		default:
			app_diag (D_SERIOUS, "**Unsupported fmt %u", fmt);
			return;

	}
	if (fork (oss_out, lbuf) == 0 ) {
		app_diag (D_SERIOUS, "oss_out failed");
		ufree (lbuf);
	}
}
