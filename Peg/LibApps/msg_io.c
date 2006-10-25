/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "net.h"
#include "diag.h"
#include "msg_pegStructs.h"
#include "msg_pegs.h"
#include "lib_apps.h"
#include "app_tarp_if.h"

extern word 	host_pl;

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

void msg_findTag_out (word state, char** buf_out, lword tag, nid_t peg) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgFindTagType));
	else
		memset (*buf_out, 0, sizeof(msgFindTagType));

	in_header(*buf_out, msg_type) = msg_findTag;
	in_header(*buf_out, rcv) = peg;
	in_findTag(*buf_out, target) = tag;
}

void msg_fwd_in (word state, char * buf, word size) {

	char * out_buf = NULL;
	int tagIndex;
	int w_len;

	if ( in_findTag(buf, target) != 0) {
		tagIndex = find_tag (in_fwd(buf, target));
		//diag ("FWD tag %d peg %d", (word)in_fwd(buf, target), in_header(buf, rcv));
		if (tagIndex >= 0) {
			w_len = size - sizeof(msgFwdType);
			copy_fwd_msg(state, &out_buf, buf + sizeof(msgFwdType), w_len);
		}
	} else {
		// kludge for remote set
		w_len = size - sizeof(msgFwdType);
		copy_fwd_msg(state, &out_buf, buf + sizeof(msgFwdType), w_len);
		if(in_header(out_buf, msg_type) == msg_setPeg) {
			if(in_setPeg(out_buf, new_id) != 0)
				local_host = in_setPeg(out_buf, new_id);
			if (in_setPeg(out_buf, level) != 0) {
				net_opt (PHYSOPT_SETPOWER, &in_setPeg(out_buf, level));
				host_pl = in_setPeg(out_buf, level);
			}
			diag ("PRINT MSG from peg %d %s", in_header(buf, snd), in_setPeg(out_buf, str));
			ufree (out_buf);
			return;
		}
	}
	if (out_buf) {
		//diag ("FWD in %d %d", in_header(out_buf, msg_type), in_header(out_buf, rcv));
		if(msg4tag.buf)
				ufree (msg4tag.buf); // discard old message
		msg4tag.buf = out_buf;
		msg4tag.tstamp = seconds();
	}
}

void msg_fwd_out (word state, char** buf_out, word size, lword tag, nid_t peg, lword pass) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, size);
	else
		memset (*buf_out, 0, size);

	in_header(*buf_out, msg_type) = msg_fwd;
	in_header(*buf_out, rcv) = peg;
	in_fwd(*buf_out, target) = tag;
	in_fwd(*buf_out, passwd) = pass;
}

void msg_getTagAck_in (word state, char * buf, word size) {

	oss_getTag_out (buf, oss_fmt);

	// master stacking came free
	if (master_host != local_host) {
		in_header(buf, rcv) = master_host;
		send_msg (buf, sizeof(msgGetTagAckType));
		in_header(buf, rcv) = local_host; // restore just in case
	}
}

void msg_master_in (char * buf) {
	if (master_host != in_header(buf, snd)) {
		app_diag (D_SERIOUS, "master? %d %d", master_host,
			in_header(buf, snd));
		master_host  = in_header(buf, snd);
		set_master_chg();
	}
	if (is_master_chg) {
		clr_master_chg;
		app_diag (D_INFO, "Set master to %u at %ld", master_host,
			master_delta);
	}
	master_delta = in_master(buf, mtime) - seconds();
}

void msg_master_out (word state, char** buf_out, nid_t peg) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgMasterType));
	else
		memset (*buf_out, 0, sizeof(msgMasterType));

	in_header(*buf_out, msg_type) = msg_master;
	in_header(*buf_out, rcv) = peg;
	in_master(*buf_out, mtime) = seconds();
}

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

void msg_report_in (word state, char * buf) {
	char * out_buf = NULL;

	msg_reportAck_out (state, buf, &out_buf);
	if (out_buf) {
		send_msg (out_buf, sizeof(msgReportAckType));
		ufree (out_buf);
		out_buf = NULL;
	}
	oss_report_out (buf, oss_fmt);

	// master stacking came free
	if (master_host != local_host) {
		in_header(buf, rcv) = master_host;
		send_msg (buf, sizeof(msgReportType));
		in_header(buf, rcv) = local_host; // restore just in case
	}
}

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

void msg_reportAck_out (word state, char * buf, char** out_buf) {

	if (*out_buf == NULL)
		*out_buf = get_mem (state, sizeof(msgReportAckType));
	else
		memset (*out_buf, 0, sizeof(msgReportAckType));

	in_header(*out_buf, msg_type) = msg_reportAck;
	in_header(*out_buf, rcv) = in_header(buf, snd);
	in_reportAck(*out_buf, tagId) = in_report(buf, tagId);
	in_reportAck(*out_buf, state) = in_report(buf, state);
	in_reportAck(*out_buf, count) = in_report(buf, count);
}

void msg_setTagAck_in (word state, char * buf, word size) {

	oss_setTag_out (buf, oss_fmt);

	// master stacking came free
	if (master_host != local_host) {
		in_header(buf, rcv) = master_host;
		send_msg (buf, sizeof(msgSetTagAckType));
		in_header(buf, rcv) = local_host; // restore just in case
	}
}


