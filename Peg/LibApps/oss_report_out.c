/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_pegStructs.h"
#include "app.h"
#include "ser.h"
#include "diag.h"

#define OO_RETRY 00
process (oss_out, char)

	entry (OO_RETRY)
		ser_out (OO_RETRY, data);
		ufree (data);
		finish;
endprocess (0)
#undef  OO_RETRY

void oss_report_out (char * buf, word fmt) {
	char * lbuf;

	switch (fmt) {
		case OSS_HT:
			lbuf = form (NULL, "**Tag %u at Peg %lu at %lu in %u"\
					" with rss:pl (%u:%u)\r\n",
				*((word*)&in_report(buf, tagId) +1), 
				in_header(buf, snd),
				in_report(buf, tStamp),
				in_report(buf, state),
				(*(word*)&in_report(buf, tagId)) >>8,
				(*(word*)&in_report(buf, tagId)) & 0x00ff);
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
