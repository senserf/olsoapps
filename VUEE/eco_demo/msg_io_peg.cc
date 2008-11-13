/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifdef __SMURPH__
#include "globals_peg.h"
#include "threadhdrs_peg.h"
#endif

#include "diag.h"
#include "app_peg.h"
#include "msg_peg.h"
#include "oss_fmt.h"

#ifdef	__SMURPH__

#include "node_peg.h"
#include "stdattr.h"

#else	/* PICOS */

#include "net.h"

#endif

#include "attnames_peg.h"

// int tIndex is no good; we would need two zeroes...
// this kludgy crap whould be rewritten anyway.
__PUBLF (NodePeg, void, msg_report_out) (word state, word tIndex,
					char** out_buf, word flags) {
	mclock_t mc;
	word w = sizeof(msgReportType);

	// we'll see about selective reporting...
	if ((flags & REP_FLAG_PLOAD) &&
			tagArray[tIndex].rpload.ppload.ts != 0xFFFFFFFF)
		w += sizeof(reportPloadType);

	if (master_host == 0) { // nobody to report to
		app_diag (D_INFO, "I'm a Ronin");
		return;
	}

	if (*out_buf == NULL)
		*out_buf = get_mem (state, w);
	else
		memset (*out_buf, 0, w);

	in_header(*out_buf, msg_type) = msg_report;
	in_header(*out_buf, rcv) = master_host;
	in_report(*out_buf, flags) = flags;
	if (tIndex & 0x8000) {
		in_report(*out_buf, tagid) = 0;
		in_report(*out_buf, rssi) = 0;
		in_report(*out_buf, pl) = 0;
		mc.sec = 0;
		wall_time (&mc);
		in_report(*out_buf, tStamp) = mc.sec;
		in_report(*out_buf, state) = sumTag;
		in_report(*out_buf, count) = tIndex & 0x7FFF;;
		return;
	}
	in_report(*out_buf, tagid) = tagArray[tIndex].id;
	in_report(*out_buf, rssi) = tagArray[tIndex].rssi;
	in_report(*out_buf, pl) = tagArray[tIndex].pl;
	mc.sec = seconds() - tagArray[tIndex].evTime;
	mc.hms.f = 1;
	wall_time (&mc);
	in_report(*out_buf, tStamp) = mc.sec;
	in_report(*out_buf, state) = tagArray[tIndex].state;
	in_report(*out_buf, count) = ++tagArray[tIndex].count;

	if ((flags & REP_FLAG_PLOAD) &&
			tagArray[tIndex].rpload.ppload.ts != 0xFFFFFFFF) {
		in_report(*out_buf, flags) |= REP_FLAG_PLOAD;
		memcpy (&in_reportPload(*out_buf, ppload),
				&tagArray[tIndex].rpload.ppload,
				sizeof(pongPloadType));
		in_reportPload(*out_buf, ts) = tagArray[tIndex].rpload.ts;
		in_reportPload(*out_buf, eslot) =
			tagArray[tIndex].rpload.eslot; 
	}
}

__PUBLF (NodePeg, void, msg_findTag_in) (word state, char * buf) {

	char * out_buf = NULL;
	int tagIndex;

	if ((word)(in_findTag(buf, target)) == 0) { // summary
		tagIndex = find_tags (in_findTag(buf, target), 1);
		msg_report_out (state, (word)tagIndex | 0x8000, &out_buf,
				REP_FLAG_NOACK);
		if (out_buf == NULL) // ronin
			return;
	} else {
		tagIndex = find_tags (in_findTag(buf, target), 0);

		if (tagIndex < 0) { // none found

			// don't report bulk missing
			if (in_header(buf, rcv) == 0)
				return;
			msg_report_out (state, 0x8000,  &out_buf,
					REP_FLAG_NOACK);
		} else {

			msg_report_out (state, tagIndex, &out_buf,
				       REP_FLAG_NOACK | REP_FLAG_PLOAD);
		}

		if (out_buf == NULL)
			return;

		// kludgy: we have an absent tag reported as sumary; change:
		if (tagIndex < 0) {
			in_report(out_buf, tagid) = in_findTag(buf, target);
			in_report(out_buf, rssi) = 0;
			in_report(out_buf, pl) = 0;
			in_report(out_buf, state) = noTag;
		}
	}
	if (out_buf) {
		if (local_host == master_host) {
			in_header(out_buf, snd) = local_host;
			oss_report_out (out_buf);
		} else
			send_msg (out_buf,
				in_report_pload(out_buf) ?
				sizeof(msgReportType) + sizeof(reportPloadType)
				: sizeof(msgReportType));

		ufree (out_buf);
	}
}

__PUBLF (NodePeg, void, msg_findTag_out) (word state, char** buf_out,
							nid_t tag, nid_t peg) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgFindTagType));
	else
		memset (*buf_out, 0, sizeof(msgFindTagType));

	in_header(*buf_out, msg_type) = msg_findTag;
	in_header(*buf_out, rcv) = peg;
	in_findTag(*buf_out, target) = tag;
}

__PUBLF (NodePeg, void, msg_setPeg_in) (char * buf) {
	word mmin, mem;
	char * out_buf = get_mem (WNONE, sizeof(msgStatsPegType));

	if (out_buf == NULL)
		return;

	if (in_setPeg(buf, audi) != 0xffff) {
		tag_auditFreq = in_setPeg(buf, audi);
		tmpcrap (0);
	}

	if (in_setPeg(buf, level) != 0xffff) {
		host_pl = in_setPeg(buf, level);
		net_opt (PHYSOPT_SETPOWER, &host_pl);
	}

	mem = memfree (0, &mmin);
	in_header(out_buf, msg_type) = msg_statsPeg;
	in_header(out_buf, rcv) = in_header(buf, snd);
	in_statsPeg(out_buf, hostid) = host_id;
	in_statsPeg(out_buf, ltime) = seconds();
	in_statsPeg(out_buf, mdelta) = master_delta;

	//in_statsPeg(out_buf, slot) is really # of entries
	if (agg_data.eslot == EE_AGG_MIN && agg_data.ee.s.f.status == AGG_FF)
		in_statsPeg(out_buf, slot) = 0;
	else
		in_statsPeg(out_buf, slot) = agg_data.eslot -
			EE_AGG_MIN +1;

	in_statsPeg(out_buf, audi) = tag_auditFreq;
	in_statsPeg(out_buf, pl) = host_pl;
	in_statsPeg(out_buf, mhost) = master_host;
	in_statsPeg(out_buf, mem) = mem;
	in_statsPeg(out_buf, mmin) = mmin;
	in_statsPeg(out_buf, a_fl) = handle_a_flags (in_setPeg(buf, a_fl));
	send_msg (out_buf, sizeof(msgStatsPegType));
	ufree (out_buf);
}

__PUBLF (NodePeg, void, msg_fwd_in) (word state, char * buf, word size) {

	char * out_buf = NULL;
	int tagIndex;
	int w_len;

	if ((tagIndex = find_tags (in_fwd(buf, target), 0)) < 0)
		return;

	w_len = size - sizeof(msgFwdType);
	if (w_len != sizeof(msgSetTagType))
		app_diag (D_SERIOUS, "Bad fwd len %u", w_len);

	// tag's rx open all time
	if (tagArray[tagIndex].rxperm) {
		send_msg (buf + sizeof(msgFwdType), sizeof(msgSetTagType));
		return;
	}

	copy_fwd_msg (state, &out_buf, buf + sizeof(msgFwdType), w_len);

	if(msg4tag.buf)
		ufree (msg4tag.buf); // discard old message
	msg4tag.buf = out_buf;
	msg4tag.tstamp = seconds();
}

__PUBLF (NodePeg, void, msg_fwd_out) (word state, char** buf_out, word size,
					nid_t tag, nid_t peg) {
	if (*buf_out == NULL)
		*buf_out = get_mem (state, size);
	else
		memset (*buf_out, 0, size);

	in_header(*buf_out, msg_type) = msg_fwd;
	in_header(*buf_out, rcv) = peg;
	in_fwd(*buf_out, target) = tag;
}

#ifndef __SMURPH__
int mbeacon (word, address);
#endif

__PUBLF (NodePeg, void, msg_master_in) (char * buf) {
	if (master_host != in_header(buf, snd)) {
		app_diag (D_SERIOUS, "master? %d %d", master_host,
			in_header(buf, snd));
		master_host  = in_header(buf, snd);
		set_master_chg();
	}

	// I'm not sure... let's assume the master is unconditional...
	//if (in_master(buf, mtime) != 0) {
		master_clock.sec = in_master(buf, mtime);
		master_delta = seconds();
	//}
	sync_freq = in_master(buf, syfreq);

	if (is_master_chg) {
		clr_master_chg;
		if (running (mbeacon)) { // I was the Master
			killall (mbeacon);
			tarp_ctrl.param |= 0x01; // routing ON
			diag (OPRE_APP_ACK "Abdicating for %u", master_host);
		} else {
		diag (OPRE_APP_ACK "Set master to %u", master_host);
		}
	}
}

__PUBLF (NodePeg, void, msg_master_out) (word state, char** buf_out,
								nid_t peg) {
	mclock_t mc;

	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgMasterType));
	else
		memset (*buf_out, 0, sizeof(msgMasterType));

	in_header(*buf_out, msg_type) = msg_master;
	in_header(*buf_out, rcv) = peg;

	mc.sec = 0;
	wall_time (&mc);
	in_master(*buf_out, mtime) = mc.sec;
	in_master(*buf_out, syfreq) = sync_freq;
}

__PUBLF (NodePeg, void, msg_pong_in) (word state, char * buf, word rssi) {
	char * out_buf = NULL; // is static needed / better here? (state)
	sint tagIndex;
	mclock_t mc;

	app_diag (D_DEBUG, "Pong %u", in_header(buf, snd));
	mc.sec = 0;
	wall_time (&mc);

	if ((tagIndex = find_tags (in_header(buf, snd), 0)) < 0) { // not found

		if ((tagIndex = insert_tag (in_header(buf, snd))) < 0)
		       return;

		tagArray[tagIndex].freq = in_pong(buf, freq);
		tagArray[tagIndex].rssi = rssi;
		tagArray[tagIndex].pl = in_pong(buf, level);
		if (in_pong_rxperm(buf))
			tagArray[tagIndex].rxperm = 1;
		else
			tagArray[tagIndex].rxperm = 0;

		if (!in_pong_pload(buf))
			return;

		memcpy (&tagArray[tagIndex].rpload.ppload, 
				buf + sizeof (msgPongType),
				sizeof (pongPloadType));
		tagArray[tagIndex].rpload.ts = mc.sec;
		tagArray[tagIndex].rpload.eslot = agg_data.eslot;
		if (agg_data.ee.s.f.status != AGG_FF)
			tagArray[tagIndex].rpload.eslot++;

	} else {
		if (in_pong_rxperm(buf))
			tagArray[tagIndex].rxperm = 1;
		else
			tagArray[tagIndex].rxperm = 0;

		tagArray[tagIndex].freq = in_pong(buf, freq);
		tagArray[tagIndex].rssi = rssi;
		tagArray[tagIndex].pl = in_pong(buf, level);

		if (in_pong_pload(buf) && tagArray[tagIndex].rpload.ppload.ts !=
				in_pongPload(buf, ts)) {

			if (is_eew_coll &&
				       	tagArray[tagIndex].state == reportedTag)
				write_agg ((word)tagIndex);

			memcpy (&tagArray[tagIndex].rpload.ppload,
				buf + sizeof (msgPongType),
				sizeof (pongPloadType));
			tagArray[tagIndex].rpload.ts = mc.sec;
			tagArray[tagIndex].rpload.eslot = agg_data.eslot;
			if (agg_data.ee.s.f.status != AGG_FF)
				tagArray[tagIndex].rpload.eslot++;
			set_tagState (tagIndex, newTag, YES);
		}
	}

	switch (tagArray[tagIndex].state) {
		case noTag:
			app_diag (D_SERIOUS, "NoTag error");
			return;

		case newTag:
			msg_report_out (state, tagIndex, &out_buf,
					REP_FLAG_PLOAD);
			if (master_host == local_host)
				set_tagState (tagIndex, confirmedTag, NO);
			else
				set_tagState (tagIndex, reportedTag, NO);
			break;

		case reportedTag:
			tagArray[tagIndex].lastTime = seconds();
			msg_report_out (state, tagIndex, &out_buf,
					REP_FLAG_PLOAD);
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
#if 0
		case fadingReportedTag:
			set_tagState(tagIndex, reportedTag, NO);
			break;

		case fadingConfirmedTag:
			set_tagState(tagIndex, confirmedTag, NO);
			break;
#endif
		case goneTag:
			set_tagState(tagIndex, newTag, YES);
			break;

		default:
			app_diag (D_SERIOUS, "Tag state?(%u) Suicide!",
				tagArray[tagIndex].state);
			reset ();
	}

	if (out_buf) { // won't be if I'm a Ronin
		if (local_host == master_host) {
			// master with tags - hack the sender
			in_header(out_buf, snd) = local_host;
			oss_report_out (out_buf);
		} else
			send_msg (out_buf,
			in_report_pload(out_buf) ?
				sizeof(msgReportType) + sizeof(reportPloadType)
				: sizeof(msgReportType));

		ufree (out_buf);
		out_buf = NULL;
	}
}

__PUBLF (NodePeg, void, msg_report_in) (word state, char * buf) {
	char * out_buf = NULL;

	if (!in_report_noack(buf)) { // don't ack
		msg_reportAck_out (state, buf, &out_buf);
		if (out_buf) {
			send_msg (out_buf, sizeof(msgReportAckType));
			ufree (out_buf);
			out_buf = NULL;
		} else {
			app_diag (D_SERIOUS, "ack not sent");
		}
	}
	oss_report_out (buf);

	// master stacking came free
	if (master_host != local_host) {
		in_header(buf, rcv) = master_host;
		send_msg (out_buf,
			in_report_pload(out_buf) ?
			sizeof(msgReportType) + sizeof(reportPloadType)
			: sizeof(msgReportType));
		in_header(buf, rcv) = local_host; // restore just in case
	}
}

__PUBLF (NodePeg, void, msg_reportAck_in) (char * buf) {
	int tagIndex;

	if ((tagIndex = find_tags (in_reportAck(buf, tagid), 0)) < 0) {
		app_diag (D_INFO, "Ack for a goner %u",
			       in_reportAck(buf, tagid));
		return;
	}

	app_diag (D_DEBUG, "Ack (in %u) for %lx in %u",
		in_reportAck(buf, state),
		tagArray[tagIndex].id, tagArray[tagIndex].state);

	switch (tagArray[tagIndex].state) {
		case reportedTag:
			set_tagState(tagIndex, confirmedTag, NO);
			// it is tempting to update eeprom, but don't, as it
			// may cause unnecessary page erasures
			break;
#if 0
		case fadingReportedTag:
			set_tagState(tagIndex, fadingConfirmedTag, NO);
			break;
#endif
		case goneTag:
			init_tag (tagIndex);
			break;

		default:
			app_diag (D_INFO, "Ignoring Ack for %lx on %u",
				tagArray[tagIndex].id,
				tagArray[tagIndex].state);
	}
}

__PUBLF (NodePeg, void, msg_reportAck_out) (word state, char * buf,
							char** out_buf) {
	if (*out_buf == NULL)
		*out_buf = get_mem (state, sizeof(msgReportAckType));
	else
		memset (*out_buf, 0, sizeof(msgReportAckType));

	in_header(*out_buf, msg_type) = msg_reportAck;
	in_header(*out_buf, rcv) = in_header(buf, snd);
	in_reportAck(*out_buf, tagid) = in_report(buf, tagid);
	in_reportAck(*out_buf, state) = in_report(buf, state);
}

