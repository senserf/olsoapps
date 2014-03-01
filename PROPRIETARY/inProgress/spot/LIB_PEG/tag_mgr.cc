/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2014        			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "commons.h"
#include "diag.h"
#include "inout.h"

/**************************************************************************
tagList is a simple list of structs with Master-unconfirmed tags. New alarms
from an already present tag overwrite (del, ins) the entry. Events don't
overwrite alarms. Acks from Master remove (del) corresponding entries.
***************************************************************************/
tagListType tagList;

#define _TMGR_DBG	0
void reset_tags () {

	char * mel = tagList.nel;

	while (mel) {
		tagList.nel = in_tdt(mel, nel);
		ufree (mel);
		mel = tagList.nel;
	}
	memset (&tagList, 0, sizeof(tagListType));
}

/***********************************************
0 - deleted event
1 - deleted alarm
2 - wrong ref
3 - couldn't find tag
4 - found alarm, aborted (unforced) deletion
5 - duplicate (aborted deletion)
************************************************/
word del_tag (word id, word ref, word dupeq, Boolean force) {


	char * pl = tagList.nel;
	char * p = NULL;
	word ret;

	while (pl) {
		if (in_tdt(pl, tagid) == id) {

			if (ref == 0 || in_tdt(pl, refTime) == ref) {
				if (ref == 0 && dupeq == in_pdt(pl, dupeq))
					return 5;
				// del pl if allowed
				if (in_pdt(pl, alrm_id) == 0) {
					--tagList.evnts;
					ret = 0;
				} else {
					if (!force)
						return 4;

					--tagList.alrms;
					ret = 1;
				}
				if (p)
					in_tdt(p, nel) = in_tdt(pl, nel);
				else
					tagList.nel = in_tdt(pl, nel);

				ufree (pl);
			} else {
				return 2;
			}
			return ret;
		}
		p = pl; // yeah, I had it with **char - unreadable in 2 weeks
		pl = in_tdt(pl, nel);
	}
	return 3;
}

void report_tag (char * td) {
	char  * mp;
	word	siz = sizeof(msgReportType) + sizeof(pongDataType) +
			in_pdt(td, len);

	mp = get_mem (siz, NO); // continue if no mem

	if (mp == NULL) {
		app_diag_S ("Rep failed");
		return;
	}

	memset (mp, 0, siz);
	in_header(mp,  msg_type) = msg_report;
	in_header(mp,  rcv) = master_host;
	in_report(mp, ref) = in_tdt(td, refTime);
	in_report(mp, tagid) = in_tdt(td, tagid);
	in_report(mp, rssi) = in_tdt(td, rssi);
	in_report(mp, ago) = (word)(seconds() - in_tdt(td, refTime)) > 255 ?
		255 : (byte)(seconds() - in_tdt(td, refTime));

	memcpy (mp + sizeof(msgReportType), td + sizeof(tagDataType),
		siz - sizeof(msgReportType));
	talk (mp, siz, TO_ALL); // will NOT go TO_NET on Master. see talk()
	ufree (mp);
}

// I let it do empty quick things if local_host == master_host
void ins_tag (char * buf, word rssi) { // it is msg_pong in buf

	Boolean force = in_pong(buf, pd).alrm_id == 0 ? NO : YES;
	word ret = del_tag (in_header(buf, snd), 0, in_pong(buf, pd).dupeq, 
			force);
	char * ptr;

	if (ret == 4) {
#if _TMGR_DBG
		app_diag_U ("TMGR Ins set alrm %u", in_header(buf, snd));
#endif
		return;
	}
	if (ret == 5) {
#if _TMGR_DBG
		app_diag_U ("TMGR dupeq %u", in_header(buf, snd));
#endif
		return;
	}

	ptr = get_mem (sizeof(tagDataType) + sizeof(pongDataType) +
		in_pong(buf, pd).len, YES); // failure will reset

	memcpy (ptr + sizeof(tagDataType), buf + sizeof(headerType),
		sizeof(pongDataType) + in_pong(buf, pd).len);

	in_tdt(ptr, refTime) = (word)seconds();
	in_tdt(ptr, tagid) = in_header(buf, snd);
	in_tdt(ptr, rssi) = rssi;
	in_tdt(ptr, marka) = tagList.marka;
	in_tdt(ptr, nel) = tagList.nel;

	report_tag (ptr);

	if (local_host != master_host) { // on Master, just skip insertion...
		tagList.nel = ptr;

		if (force)
			tagList.alrms++;
		else
			tagList.evnts++;
#if _TMGR_DBG
	app_diag_U ("TMGR rep %u %u", tagList.alrms, tagList.evnts);
#endif
	} else 				// ... and free the element
		ufree (ptr);
}
#undef _TMGR_DBG

