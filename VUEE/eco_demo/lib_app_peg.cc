/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.			*/
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
#include "form.h"

#endif	/* SMURPH or PICOS */

#include "attnames_peg.h"

/*
 * "Virtual" stuff needed by NET & TARP =======================================
 */
__PUBLF (NodePeg, int, tr_offset) (headerType *h) {
	// Unused ??
	return 0;
}

__PUBLF (NodePeg, Boolean, msg_isBind) (msg_t m) {
	return NO;
}

__PUBLF (NodePeg, Boolean, msg_isTrace) (msg_t m) {
	return NO;
}

__PUBLF (NodePeg, Boolean, msg_isMaster) (msg_t m) {
	return (m == msg_master);
}

__PUBLF (NodePeg, Boolean, msg_isNew) (msg_t m) {
	return NO;
}

__PUBLF (NodePeg, Boolean, msg_isClear) (byte o) {
	return YES;
}

__PUBLF (NodePeg, void, set_master_chg) () {
	app_flags |= 2;
}

// ============================================================================

// IN: mc->sec: # of s. from NOW, can't go back before T == 0.0:0:0
//     mc->hms.f == 1 <=> go back in time mc->sec seconds
// OUT: *mc: wall time with the input offset (usually 0)

__PUBLF (NodePeg, void, wall_time) (mclock_t *mc) {
	word w1, w2, w3, w4;

	lword lw = seconds() - master_delta +
		24L * 3600 * master_clock.hms.d +
		3600L * master_clock.hms.h +
		60 * master_clock.hms.m + master_clock.hms.s;

	if (mc->hms.f &&  (mc->sec & 0x7FFFFFFF) > lw) {
		app_diag (D_SERIOUS, "Ignoring bad offset %u",
				(word)mc->sec);
		mc->sec = 0;
	}

	if (mc->hms.f)
		lw -= mc->sec;
	else
		lw += mc->sec;
	mc->sec = 0;

	w1 = lw / (24L * 3600);
	lw %= 24L * 3600;
	w2 = lw / 3600;
	lw %= 3600;
	w3 = lw / 60;
	w4 = lw % 60;

	if (w4 >= 60) {
		w4 -= 60;
		w3++;
	}
	if (w3 >= 60) {
		w3 -= 60;
		w2++;
	}
	if (w2 >= 24) {
		 w2 -= 24;
		 w1++;
	}
	mc->hms.d = w1; mc->hms.h = w2; mc->hms.m = w3; mc->hms.s = w4;
	mc->hms.f = master_clock.hms.f;
}

/*
   what == 0: find and return the index;
   what == 1: count
*/
__PUBLF (NodePeg, int, find_tags) (word tag, word what) {
	int i = 0;
	int count = 0;

	while (i < tag_lim) {
		if (tag == 0) { // any tag counts
			if (tagArray[i].id != 0) {
				if (what == 0)
					return i;
				else 
					count ++;
			}
		} else {
			if (tagArray[i].id == tag) {
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
		app_diag (D_SERIOUS, "No mem reset");
		reset();
#if 0
		if (state != WNONE) {
			umwait (state);
			release;
		}
#endif
	}
	return buf;
}

__PUBLF (NodePeg, void, init_tag) (word i) {
	tagArray[i].id = 0;
	tagArray[i].rssi = 0;
	tagArray[i].pl = 0;
	tagArray[i].rxperm = 0;
	tagArray[i].state = noTag;
	tagArray[i].freq = 0;
	tagArray[i].count = 0;
	tagArray[i].evTime = 0;
	tagArray[i].lastTime = 0;
	tagArray[i].rpload.ppload.ts = 0xFFFFFFFF;
}

__PUBLF (NodePeg, void, init_tags) () {
	word i = tag_lim;
	while (i-- > 0)
		init_tag (i);
}

#if 0
would have to be in global processes crap... is in app_peg.cc
__PUBLF (NodePeg, void, show_ifla) () {
	char * mbuf = NULL;

	if (if_read (0) == 0xFFFF) {
		diag (OPRE_APP_ACK "No custom sys data");
		return;
	}
	mbuf = form (NULL, ifla_str, if_read (0), if_read (1), if_read (2),
			if_read (3), if_read (4));

	if (runstrand (oss_out, mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out fork");
		ufree (mbuf);
	}
}

__PUBLF (NodePeg, void, read_ifla) () {
	if (if_read (0) == 0xFFFF) { // usual defaults
		local_host = (word)host_id;
#ifndef __SMURPH__
		master_host = DEF_MHOST;
#endif
		return;
	}

	local_host = if_read (0);
	host_pl = if_read (1);
	app_flags = if_read (2);
	tag_auditFreq = if_read (3);
	master_host = if_read (4);
}

__PUBLF (NodePeg, void, save_ifla) () {
	if (if_read (0) != 0xFFFF) {
		if_erase (0);
		diag (OPRE_APP_ACK "Flash p0 overwritten");
	}

	// there is 'show' after 'save'... don't check if_writes here (?)
	if_write (0, local_host);
	if_write (1, host_pl);
	if_write (2, app_flags);
	if_write (3, tag_auditFreq);
	if_write (4, master_host);
}
#endif

__PUBLF (NodePeg, void, set_tagState) (word i, tagStateType state,
							Boolean updEvTime) {
	tagArray[i].state = state;
	tagArray[i].count = 0; // always (?) reset the counter
	tagArray[i].lastTime = seconds();
	if (updEvTime)
		tagArray[i].evTime = tagArray[i].lastTime;

	if (is_eew_conf && state == confirmedTag)
		write_agg (i);
}

__PUBLF (NodePeg, int, insert_tag) (word tag) {

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
		seconds() - tagArray[i].lastTime < tag_auditFreq)
		return;	

	switch (tagArray[i].state) {
		case newTag:
#if 0
this is for mobile tags, to cut off flickering ones
			app_diag (D_DEBUG, "Delete %lx", tagArray[i].id);
			init_tag (i);
#endif
			app_diag (D_DEBUG, "Rep new %u",
				(word)tagArray[i].id);
			set_tagState (i, reportedTag, NO);
			break;

		case goneTag:
			// stop after 4 beats
			if (seconds() - tagArray[i].lastTime >
					((lword)tagArray[i].freq << 2)) {
				app_diag (D_WARNING,
					"Stopped reporting gone %u",
					(word)tagArray[i].id);
				init_tag (i);

			} else {
				app_diag (D_DEBUG, "Rep gone %u",
					(word)tagArray[i].id);
				msg_report_out (state, i, buf_out, 
						REP_FLAG_PLOAD);
				// if in meantime I becane the Master:
				if (local_host == master_host || 
						master_host == 0)
					init_tag (i);
			}
			break;

		case reportedTag:
#if 0
			app_diag (D_DEBUG, "Set fadingReported %lx",
				tagArray[i].id);
			set_tagState (i, fadingReportedTag, NO);
#endif
			// mark 'gone' for unconfirmed as well
			if (seconds() - tagArray[i].lastTime >
					((lword)tagArray[i].freq << 1)) {
				app_diag (D_DEBUG, "Rep going %u",
						(word)tagArray[i].id);
				set_tagState (i, goneTag, YES);
				msg_report_out (state, i, buf_out,
						REP_FLAG_PLOAD);

				if (local_host == master_host ||
						master_host == 0)
					init_tag (i);

			} else {
				app_diag (D_DEBUG, "Re rep %u",
					(word)tagArray[i].id);
				msg_report_out (state, i, buf_out, 
						REP_FLAG_PLOAD);
				// if I become the Master, this is needed:
				if (local_host == master_host || 
						master_host == 0)
					set_tagState (i, confirmedTag, NO);
			}
			break;

		case confirmedTag:
			// missed 2 beats
			if (seconds() - tagArray[i].lastTime >
					((lword)tagArray[i].freq << 1)) {

				app_diag (D_DEBUG, "Rep going %u",
					(word)tagArray[i].id);
				set_tagState (i, goneTag, YES);
				msg_report_out (state, i, buf_out, 
						REP_FLAG_PLOAD);

				if (local_host == master_host ||
						master_host == 0)
					init_tag (i);
			} // else do nothing
			break;
#if 0
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
#endif
		default:
			app_diag (D_SERIOUS, "Tag? State? %u in %u",
				(word)tagArray[i].id, tagArray[i].state);
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
			if (in_pong_pload(buf))
				expSize = sizeof(msgPongType) +
					sizeof(pongPloadType);
			else
				expSize = sizeof(msgPongType);

			if (expSize == size)
				return 0;
			break;

		case msg_master:
			if ((expSize = sizeof(msgMasterType)) == size)
				return 0;
			break;

		case msg_report:
			if (in_report_pload(buf))
				expSize = sizeof(msgReportType) +
					sizeof(reportPloadType);
			else
				expSize = sizeof(msgReportType);

			if (expSize == size)
				return 0;
			break;

		case msg_reportAck:
			if ((expSize = sizeof(msgReportAckType)) == size)
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

		case msg_setPeg:
			if ((expSize = sizeof(msgSetPegType)) == size)
				return 0;
			break;

		case msg_statsPeg:
			if ((expSize = sizeof(msgStatsPegType)) == size)
				return 0;
			break;

		 case msg_statsTag:
			if ((expSize = sizeof(msgStatsTagType)) == size)
				return 0;
			break;

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

__PUBLF (NodePeg, void, write_agg) (word ti) {

	if (agg_data.ee.s.f.status != AGG_FF)
		agg_data.eslot++;

	agg_data.ee.s.f.status = tagArray[ti].state == confirmedTag ?
		AGG_CONFIRMED : AGG_COLLECTED;
	agg_data.ee.s.f.emptym = ee_emptym ? 0 : 1;
	agg_data.ee.s.f.spare = 7;
	agg_data.ee.sval[0] = tagArray[ti].rpload.ppload.sval[0];
	agg_data.ee.sval[1] = tagArray[ti].rpload.ppload.sval[1];
	agg_data.ee.sval[2] = tagArray[ti].rpload.ppload.sval[2];
	agg_data.ee.sval[3] = tagArray[ti].rpload.ppload.sval[3];
	agg_data.ee.sval[4] = tagArray[ti].rpload.ppload.sval[4];

	agg_data.ee.ts = tagArray[ti].rpload.ts;
	agg_data.ee.t_ts = tagArray[ti].rpload.ppload.ts;
	agg_data.ee.t_eslot = tagArray[ti].rpload.ppload.eslot;
	agg_data.ee.tag = tagArray[ti].id;

	if (agg_data.eslot >= EE_AGG_MAX) {
		agg_data.eslot = EE_AGG_MAX -1;
		app_diag (D_SERIOUS, "EEPROM FULL");

	} else {

		if (ee_write (WNONE, agg_data.eslot * EE_AGG_SIZE,
			(byte *)&agg_data.ee, EE_AGG_SIZE)) {
			app_diag (D_SERIOUS, "ee_write failed %x %x",
				(word)(agg_data.eslot >> 16),
				(word)agg_data.eslot);
			agg_data.ee.s.f.status = AGG_FF;
		}
	}
}

/* if ever this is promoted to a regular functionality,
   we may have a process with a msg buffer waiting for
   trigger (TAG_LISTENS+id) from here, or the msg buffer
   hanging off tagDataType. For now, we keep it as simple as possible:
   check for a msg pending for this tag
*/

__PUBLF (NodePeg, void, check_msg4tag) (char * buf) {
	mclock_t mc;

	mc.sec = 0;
	if (master_delta != 0) // do NOT send down your own clock
		wall_time (&mc);

	if (msg4tag.buf && in_header(msg4tag.buf, rcv) ==
		       in_header(buf, snd)) { // msg waiting

		if (in_pong_pload(buf)) { // add ack data
			in_setTag(msg4tag.buf, ts) = in_pongPload(buf, ts);
			in_setTag(msg4tag.buf, reftime) = mc.sec;
			in_setTag(msg4tag.buf, syfreq) = sync_freq;
			in_setTag(msg4tag.buf, ackflags) =
				agg_data.eslot >= EE_AGG_MAX -1 ? 1 : 0;
		} else {
			in_setTag(msg4tag.buf, reftime) = 0;
			in_setTag(msg4tag.buf, ts) = 0; 
			in_setTag(msg4tag.buf, syfreq) = 0;
			in_setTag(msg4tag.buf, ackflags) = 0;
		}

		send_msg (msg4tag.buf, sizeof(msgSetTagType));
		ufree (msg4tag.buf);
		msg4tag.buf = NULL;
		msg4tag.tstamp = 0;

	} else { // no msg waiting; send ack
		if (in_pong_pload(buf)) {
			pong_ack.header.rcv = in_header(buf, snd);
			pong_ack.header.hco = in_header(buf, hoc);
			pong_ack.ts = in_pongPload(buf, ts);
			pong_ack.reftime = mc.sec;
			pong_ack.syfreq = sync_freq;
			pong_ack.ackflags = 
				agg_data.eslot >= EE_AGG_MAX -1 ? 1 : 0;
			send_msg ((char *)&pong_ack, sizeof(msgPongAckType));
		}
	}
}

__PUBLF (NodePeg, void, agg_init) () {
	lword l, u, m;
	byte b;

	if (ee_read ((lword)EE_AGG_MIN * EE_AGG_SIZE, &b, 1))
		fatal_err (ERR_EER, 0, 0, 1);

	memset (&agg_data, 0, sizeof(aggDataType));

	if (b == 0xFF) { // normal operations
		agg_data.eslot = EE_AGG_MIN;
		agg_data.ee.s.f.status = AGG_FF;
		agg_data.ee.s.f.emptym = is_eem_empty ? 1 : 0;
		agg_data.ee.s.f.spare = 7;
		return;
	}

	// only efter power down / soft reset there could be anything in eeprom
	l = EE_AGG_MIN; u = EE_AGG_MAX;
	while ((u - l) > 1) {
		m = l + ((u -l) >> 1);

		if (ee_read (m * EE_AGG_SIZE, &b, 1))
			fatal_err (ERR_EER, (word)((m * EE_AGG_SIZE) >> 16),
					(word)(m * EE_AGG_SIZE), 1);

		if (b == 0xFF)
			u = m;
		else
			l = m;
	}

	if (ee_read (u * EE_AGG_SIZE, &b, 1))
		fatal_err (ERR_EER, (word)((u * EE_AGG_SIZE) >> 16),
				(word)(u * EE_AGG_SIZE), 1);

	if (b == 0xFF) {
		if (l < u) {
			if (ee_read (l * EE_AGG_SIZE, &b, 1))
				fatal_err (ERR_EER,
					(word)((l * EE_AGG_SIZE) >> 16),
					(word)(l * EE_AGG_SIZE), 1);

			if (b == 0xFF)
				fatal_err (ERR_SLOT, (word)(l >> 16),
						(word)l, 0);

			agg_data.eslot = l;
		}
	} else
		agg_data.eslot = u;

	if (ee_read (agg_data.eslot * EE_AGG_SIZE, (byte *)&agg_data.ee,
				EE_AGG_SIZE))
		fatal_err (ERR_EER,
			(word)((agg_data.eslot * EE_AGG_SIZE) >> 16),
			(word)(agg_data.eslot * EE_AGG_SIZE), EE_AGG_SIZE);
}

__PUBLF (NodePeg, void, fatal_err) (word err, word w1, word w2, word w3) {
	leds (LED_R, LED_BLINK);
	if_write (IFLASH_SIZE -1, err);
	if_write (IFLASH_SIZE -2, w1);
	if_write (IFLASH_SIZE -3, w2);
	if_write (IFLASH_SIZE -4, w3);
	if (err != ERR_MAINT) {
		app_diag (D_FATAL, "HALT %x %u %u %u", err, w1, w2, w3);
		halt();
	}
	reset();
}

__PUBLF (NodePeg, word, handle_a_flags) (word a_fl) {
	if (a_fl != 0xFFFF) {
		if (a_fl & A_FL_EEW_COLL) 
			set_eew_coll;
		else
			clr_eew_coll;

		if (a_fl & A_FL_EEW_CONF)
			set_eew_conf;
		else
			clr_eew_conf;

		if (a_fl & A_FL_EEW_OVER)
			set_eew_over;
		else
			clr_eew_over;
	}

	return (is_eew_over ? A_FL_EEW_OVER : 0) |
	       (is_eew_conf ? A_FL_EEW_CONF : 0) |
	       (is_eew_coll ? A_FL_EEW_COLL : 0);
}

