/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006.			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "diag.h"
#include "app_peg.h"
#include "msg_peg.h"

#ifdef	__SMURPH__

#include "node_peg.h"
#include "stdattr.h"

#else	/* PICOS */

#include "net.h"
#include "tarp.h"

#endif	/* SMURPH or PICOS */

#include "attnames_peg.h"

/*
   what == 0: find and return the index;
   what == 1: count
*/
__PUBLF (NodePeg, int, find_tags) (lword tag, word what) {
	int i = 0;
	int count = 0;
	lword mask = 0xffffffff;
	
	if (!(tag & 0xff000000))  // rss irrelevant
		mask &= 0x00ffffff;

	if (!(tag & 0x00ff0000)) // power level 
		mask &= 0xff00ffff;

	if (!(tag & 0x0000ffff)) // first or all tags
		mask &= 0xffff0000;

	while (i < tag_lim) {
		if ((word) tag == 0) { // any tag counts
			if (tagArray[i].id != 0 &&
					(tagArray[i].id & mask) == tag) {
				if (what == 0)
					return i;
				else 
					count ++;
			}
		} else {
			if ((tagArray[i].id & mask) == tag) {
				if (what == 0)
					return i;
				else
					count ++;
			}
		}
		i++;
	}
	if (what)
		return count;
	return -1; // found no tag
}

__PUBLF (NodePeg, char*, get_mem) (word state, int len) {
	char * buf = (char *)umalloc (len);
	if (buf == NULL) {
		app_diag (D_WARNING, "No mem %d", len);
		if (state != WNONE) {
			umwait (state);
			release;
		}
	}
	return buf;
}

__PUBLF (NodePeg, void, init_tag) (word i) {
	tagArray[i].id = 0;
	tagArray[i].state = noTag;
	tagArray[i].count = 0;
	tagArray[i].evTime = 0;
	tagArray[i].lastTime = 0;
}

__PUBLF (NodePeg, void, init_tags) () {
	word i = tag_lim;
	while (i-- > 0)
		init_tag (i);
}

__PUBLF (NodePeg, void, set_tagState) (word i, tagStateType state,
							Boolean updEvTime) {
	tagArray[i].state = state;
	tagArray[i].count = 0; // always (?) reset the counter
	tagArray[i].lastTime = seconds();
	if (updEvTime)
		tagArray[i].evTime = tagArray[i].lastTime;
}

__PUBLF (NodePeg, int, insert_tag) (lword tag) {

	int i = 0;

	while (i < tag_lim) {
		if (tagArray[i].id == 0) {
			tagArray[i].id = tag;
			set_tagState (i, newTag, YES);
			app_diag (D_DEBUG, "Inserted tag %lx at %u", tag, i);
			return i;
		}
		i++;
	}
	app_diag (D_SERIOUS, "Failed tag (%lx) insert", tag);
	return -1;
}

// For complex osses, reporting the 2 events (new rssi in, old rssi out) may be
// convenient, but for demonstrations 'old out' is supressed
#define BLOCK_MOVING_GONERS 1
__PUBLF (NodePeg, void, check_tag) (word state, word i, char** buf_out) {

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
			msg_report_out (state, i, buf_out, 0);
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
#if BLOCK_MOVING_GONERS
			if (find_tags (tagArray[i].id & 0x00ffffff, 1) > 1) {
				init_tag (i);
				break;
			}
#endif
			set_tagState (i, goneTag, YES);
			msg_report_out (state, i, buf_out, 0);
			if (local_host == master_host || master_host == 0)
				init_tag (i);
			break;

		default:
			app_diag (D_SERIOUS, "noTag? %lx in %u",
				tagArray[i].id, tagArray[i].state);
	}
}
#undef BLOCK_MOVING_GONERS

__PUBLF (NodePeg, void, copy_fwd_msg) (word state, char** buf_out, char * buf,
								word size) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, size);
	else
		memset (*buf_out, 0, size);

	memcpy (*buf_out, buf, size);
}

__PUBLF (NodePeg, void, send_msg) (char * buf, int size) {
	// it doesn't seem like a good place to filter out
	// local host, but it's convenient, for now...

	// this shouldn't be... WARNING to see why it is needed...
	if (in_header(buf, rcv) == local_host) {
		app_diag (D_WARNING, "Dropped msg(%u) to lh",
			in_header(buf, msg_type));
		return;
	}

	if (net_tx (WNONE, buf, size, 0) == 0) {
		app_count.snd++;
//		app_diag (D_WARNING, "Sent %u to %u",
		app_diag (D_DEBUG, "Sent %u to %u",
			in_header(buf, msg_type),
			in_header(buf, rcv));
	} else
		app_diag (D_SERIOUS, "Tx %u failed",
			in_header(buf, msg_type));
 }

__PUBLF (NodePeg, int, check_msg_size) (char * buf, word size, word repLevel) {
	word expSize;
	
	// for some msgTypes, it'll be less trivial
	switch (in_header(buf, msg_type)) {
		case msg_pong:
			if ((expSize = sizeof(msgPongType)) == size)
				return 0;
			break;

		case msg_master:
			if ((expSize = sizeof(msgMasterType)) == size)
				return 0;
			break;

		case msg_report:
			if ((expSize = sizeof(msgReportType)) == size)
				return 0;
			break;

		case msg_reportAck:
			if ((expSize = sizeof(msgReportAckType)) == size)
				return 0;
			break;

		case msg_getTagAck:
			if ((expSize = sizeof(msgGetTagAckType)) == size)
				 return 0;
			break;

		case msg_setTagAck:
			if ((expSize = sizeof(msgSetTagAckType)) == size)
				return 0;
			break;

		case msg_findTag:
			if ((expSize = sizeof(msgFindTagType)) == size)
				return 0;
			break;

		// if it's needed, can be done... who cares now
		case msg_fwd:
		case msg_rpc:
			return 0;

		default:
			app_diag (repLevel, "Can't check size of %u (%d)",
				in_header(buf, msg_type), size);
			return 0;
	}
	
	// 4N padding might have happened
	if (size > expSize && size >> 2 == expSize >> 2)
		return 0;

	app_diag (repLevel, "Size error for %u: %d of %d",
			in_header(buf, msg_type), size, expSize);
	return (size - expSize);
}

/* if ever this is promoted to a regular functionality,
   we may have a process with a msg buffer waiting for
   trigger (TAG_LISTENS+id) from here, or the msg buffer
   hanging off tagDataType. For now, we keep it as simple as possible:
   check for a msg pending for this tag
*/

__PUBLF (NodePeg, void, check_msg4tag) (nid_t tag) {
//	diag ("check msg");
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
