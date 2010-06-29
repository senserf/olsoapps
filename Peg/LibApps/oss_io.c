/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "form.h"
#include "ser.h"
#include "net.h"
#include "tarp.h"
#include "diag.h"
#include "msg_tagStructs.h"
#include "msg_pegStructs.h"
#include "msg_pegs.h"
#include "lib_apps.h"
#include "app.h"

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
			return "noTag\0";
		case newTag:
			return "newTag\0";
		case reportedTag:
			return "reportedTag\0";
		case confirmedTag:
			return "confirmedTag\0";
		case fadingReportedTag:
			return "fadingReportedTag\0";
		case fadingConfirmedTag:
			return "fadingConfirmedTag\0";
		case goneTag:
			return "goneTag\0";
		case sumTag:
			return "sumTag\0";
		default:
			return "unknown?\0";
	}
}
	
void oss_report_out (char * buf, word fmt) {
	char * lbuf = NULL;

	switch (fmt) {
		case OSS_HT:
			lbuf = form (NULL, "Tag %u at Peg %u at %lu: %s(%u)"\
					" with rss:pl (%u:%u)\r\n",
				(word)in_report(buf, tagId), 
				in_header(buf, snd),
				in_report(buf, tStamp),
				stateName (in_report(buf, state)),
				in_report(buf, state),
				(word)(in_report(buf, tagId) >> 24),
				(word)(in_report(buf, tagId) >> 16) & 0x00ff);
			break;

		case OSS_TCL:
			lbuf = form (NULL, "Type%u Peg%u T%u %u %u %lu %u\n",
				in_header(buf, msg_type),
				in_header(buf, snd),
				(word)in_report(buf, tagId),
				(word)(in_report(buf, tagId) >> 16) & 0x00ff,
				(word)(in_report(buf, tagId) >> 24),
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
		case OSS_TCL:
			lbuf = form (NULL, "Tag %u at Peg %u Set ack %u ack seq %u\r\n", 
				in_setTagAck(buf, tag),
				(local_host == master_host ? in_header(buf, snd) : in_header(buf, rcv)),
				in_setTagAck(buf, ack),
				in_setTagAck(buf, ackSeq));
			break;

			// do nothing
			//return;

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
		case OSS_TCL:
			lbuf = form (NULL, "Tag %u at Peg %u Get time %lu host id %x%x"\
					" pow lev %x freq maj %u freq min %u span %u ack seq %u\r\n", 
				in_getTagAck(buf, tag),
				(in_getTagAck(buf, tag) != in_header(buf, snd) ? in_header(buf, snd) : in_header(buf, rcv)),
				in_getTagAck(buf, ltime),
				in_getTagAck(buf, host_id),
				in_getTagAck(buf, pow_levels),
				in_getTagAck(buf, freq_maj),
				in_getTagAck(buf, freq_min),
				in_getTagAck(buf, rx_span),
				in_getTagAck(buf, ackSeq));
			break;

		
			// do nothing
			//return;

		default:
			app_diag (D_SERIOUS, "**Unsupported fmt %u", fmt);
			return;

	}
	if (fork (oss_out, lbuf) == 0 ) {
		app_diag (D_SERIOUS, "oss_out failed");
		ufree (lbuf);
	}
}

void oss_findTag_in (word state, lword tag, nid_t peg) {

	char * out_buf = NULL;
	int tagIndex;

	if (peg == local_host || peg == 0) {
		if (tag == 0) { // summary
			msg_report_out (state, -1, &out_buf);
		} else {
			tagIndex = find_tag (tag);
			msg_report_out (state, tagIndex, &out_buf);
			// as in msg_findTag_in, kludge summary into
			// missing tag:
			if (tagIndex < 0) {
				in_report(out_buf, tagId) = tag;
				in_report(out_buf, state) = noTag;
			}
		}
		in_header(out_buf, snd) = local_host;
		oss_report_out (out_buf, oss_fmt);

	}

	if (peg != local_host) {
		msg_findTag_out (state, &out_buf, tag, peg);
		send_msg (out_buf, sizeof(msgFindTagType));
	}
	ufree (out_buf);
}

void oss_getTag_in (word state, lword tag, nid_t peg, lword pass) {
	
	char * out_buf = NULL;
	char * get_buf = NULL;
	int size = sizeof(msgFwdType) + sizeof(msgGetTagType);
	
	if(peg == 0 || tag == 0)
	   return;
	// alloc and prepare msg fwd 
	msg_fwd_out (state, &out_buf, size, tag, peg, pass);
	in_header(out_buf, snd) = local_host;
	// get offset for payload - getTag msg
	get_buf = out_buf + sizeof(msgFwdType);
	in_header(get_buf, msg_type) = msg_getTag;
	in_header(get_buf, rcv) = (nid_t)tag;
	in_header(get_buf, snd) = local_host;
	in_getTag(get_buf, passwd) = pass;
	if (peg == local_host) {
		//	it is local - put it in the wroom	
		msg_fwd_in(state, out_buf, size);
	} else {	
		send_msg (out_buf,  size);
	}
	ufree (out_buf);
}

void oss_master_in (word state, nid_t peg) {

	char * out_buf = NULL;

	// shortcut to set master_host
	if (local_host == peg) {
		master_host = peg;
		return;
	}

	msg_master_out (state, &out_buf, peg);
	send_msg (out_buf, sizeof(msgMasterType));
	ufree (out_buf);
}

void oss_setTag_in (word state, lword tag, nid_t peg, lword pass, nid_t nid,
                    word maj, word min, word pl, word span, lword npass) {

	char * out_buf = NULL;
	char * set_buf = NULL;
	int size = sizeof(msgFwdType) + sizeof(msgSetTagType);

	if(peg == 0 || tag == 0)
	   return;
	// alloc and prepare msg fwd
	msg_fwd_out (state, &out_buf, size, tag, peg, pass);
	in_header(out_buf, snd) = local_host;
	// get offset for payload - getTag msg
	set_buf = out_buf + sizeof(msgFwdType);
	in_header(set_buf, msg_type) = msg_setTag;
	in_header(set_buf, rcv) = (nid_t)tag;
	in_header(set_buf, snd) = local_host;
	in_setTag(set_buf, passwd) = pass;
	in_setTag(set_buf, npasswd) = npass;
	in_setTag(set_buf, node_addr) = nid;
	in_setTag(set_buf, pow_levels) = pl;
	in_setTag(set_buf, freq_maj) = maj;
	in_setTag(set_buf, freq_min) = min;
	in_setTag(set_buf, rx_span) = span;
	if (peg == local_host) {
		//	it is local - put it in the wroom
		msg_fwd_in(state, out_buf, size);
	} else {
		send_msg (out_buf,  size);
	}
	ufree (out_buf);
}

void oss_setPeg_in (word state, nid_t peg, nid_t nid, word pl, char * str) {

	char * out_buf = NULL;
	char * set_buf = NULL;
	int size = sizeof(msgFwdType) + sizeof(msgSetPegType);

	// alloc and prepare msg fwd
	msg_fwd_out (state, &out_buf, size, 0, peg, 0);
	in_header(out_buf, snd) = local_host;
	// get offset for payload - getTag msg
	set_buf = out_buf + sizeof(msgFwdType);
	in_header(set_buf, msg_type) = msg_setPeg;
	in_header(set_buf, rcv) = 0;
	in_header(set_buf, snd) = local_host;
	in_setPeg(set_buf, level) = pl;
	in_setPeg(set_buf, new_id) = nid;
	memcpy(in_setPeg(set_buf, str), str, 16);
	if (peg != local_host) {
		send_msg (out_buf,  size);
	} else {
		msg_fwd_in(state, out_buf, size);
	}
	ufree (out_buf);
}
