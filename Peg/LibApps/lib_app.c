/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "net.h"
#include "diag.h"
#include "app.h"
#include "lib_apps.h"
#include "msg_pegs.h"
#include "msg_tagStructs.h"
#include "msg_pegStructs.h"


word	app_flags = 0;
lword	host_passwd = 0;
const lword	host_id = 0xBACA0001;
nid_t	net_id = 85; // 0x55 set network id to any value
nid_t	local_host = 1;
nid_t   master_host = 1;
long   master_delta = 0;
word	host_pl = 9;
word	tag_auditFreq = 10240; // in bin msec
word	tag_eventGran = 10; // in seconds

// if we can get away with it, it's better to have it in IRAM (?)
tagDataType tagArray [tag_lim];


wroomType msg4tag = {NULL, 0};
wroomType msg4ward = {NULL, 0};

appCountType app_count = {0, 0, 0};

int find_tag (lword tag) {
	int i = 0;
	lword mask = 0xffffffff;
	
	if (!(tag & 0xff000000))  // rss irrelevant
		mask &= 0x00ffffff;

	if (!(tag & 0x00ff0000)) // power level 
		mask &= 0xff00ffff;

	while (i < tag_lim) {
		if ((tagArray[i].id & mask) == tag)
			return i;
		i++;
	}
	return -1;
}

char * get_mem (word state, int len) {
	char * buf = (char *)umalloc (len);
	if (buf == NULL) {
		app_diag (D_WARNING, "Waiting for memory");
		umwait (state);
	}
	return buf;
}

void init_tag (word i) {
	tagArray[i].id = 0;
	tagArray[i].state = noTag;
	tagArray[i].count = 0;
	tagArray[i].evTime = 0;
	tagArray[i].lastTime = 0;
}

void init_tags () {
	word i = tag_lim;
	while (i-- > 0)
		init_tag (i);
}

void set_tagState (word i, tagStateType state, bool updEvTime) {
	tagArray[i].state = state;
	tagArray[i].count = 0; // always (?) reset the counter
	tagArray[i].lastTime = seconds();
	if (updEvTime)
		tagArray[i].evTime = tagArray[i].lastTime;
}

int insert_tag (lword tag) {
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

void copy_fwd_msg (word state, char** buf_out, char * buf, word size) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, size);
	else
		memset (*buf_out, 0, size);

	memcpy (*buf_out, buf, size);
}

void send_msg (char * buf, int size) {
	// it doesn't seem like a good place to filter out
	// local host, but it's convenient, for now...

	// this shouldn't be... WARNING to see why it is needed...
	if (in_header(buf, rcv) == local_host) {
		app_diag (D_WARNING, "Dropped msg(%u) to lh",
			in_header(buf, msg_type));
		return;
	}

	if (net_tx (NONE, buf, size, 0) == 0) {
		app_count.snd++;
//		app_diag (D_WARNING, "Sent %u to %u",
		app_diag (D_DEBUG, "Sent %u to %u",
			in_header(buf, msg_type),
			in_header(buf, rcv));
	} else
		app_diag (D_SERIOUS, "Tx %u failed",
			in_header(buf, msg_type));
 }

int check_msg_size (char * buf, word size, word repLevel) {
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

void check_msg4tag (nid_t tag) {
//	diag ("check msg");
	if (msg4tag.buf && in_header(msg4tag.buf, rcv) == tag) {
//		diag ("check msg %d tag %d", in_header(msg4tag.buf, msg_type), tag);
		if (seconds() - msg4tag.tstamp <= 77) // do it
			send_msg (msg4tag.buf,
				in_header(msg4tag.buf, msg_type) == msg_getTag ?
				sizeof(msgGetTagType) : sizeof(msgSetTagType));
		ufree (msg4tag.buf);
		msg4tag.buf = NULL;
		msg4tag.tstamp = 0;
	}
}
