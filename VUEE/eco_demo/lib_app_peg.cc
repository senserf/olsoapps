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

__PUBLF (NodePeg, void, set_tagState) (word i, tagStateType state,
							Boolean updEvTime) {
	tagArray[i].state = state;
	tagArray[i].count = 0; // always (?) reset the counter
	tagArray[i].lastTime = seconds();
	if (updEvTime)
		tagArray[i].evTime = tagArray[i].lastTime;
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
			app_diag (D_DEBUG, "Rep gone %u",
				(word)tagArray[i].id);
			msg_report_out (state, i, buf_out, REP_FLAG_PLOAD);
			break;

		case reportedTag:
#if 0
			app_diag (D_DEBUG, "Set fadingReported %lx",
				tagArray[i].id);
			set_tagState (i, fadingReportedTag, NO);
#endif
			app_diag (D_DEBUG, "Re rep %u",
				(word)tagArray[i].id);
			msg_report_out (state, i, buf_out, REP_FLAG_PLOAD);
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

__PUBLF (NodePeg, void, write_agg) (char * buf) {
	mclock_t mc;

	if (agg_data.ee.status != AGG_FF)
		agg_data.eslot++;

	if (agg_data.eslot >= EE_AGG_MAX) {
		// no
		// fatal_err (ERR_FULL, 0, 0, 0);
		agg_data.eslot--;
		app_diag (D_SERIOUS, "EEPROM FULL");
	}

	agg_data.ee.status = AGG_COLLECTED;
	agg_data.ee.sval[0] = in_pongPload(buf, sval[0]);
	agg_data.ee.sval[1] = in_pongPload(buf, sval[1]);
	agg_data.ee.sval[2] = in_pongPload(buf, sval[2]);
	agg_data.ee.sval[3] = in_pongPload(buf, sval[3]);
	agg_data.ee.sval[4] = in_pongPload(buf, sval[4]);
	mc.sec = 0;
	wall_time (&mc);
	agg_data.ee.ts = mc.sec;
	agg_data.ee.t_ts = in_pongPload(buf, ts);
	agg_data.ee.t_eslot = in_pongPload(buf, eslot);
	agg_data.ee.tag = in_header(buf, snd);

	if (ee_write (WNONE, agg_data.eslot * EE_AGG_SIZE,
		(byte *)&agg_data.ee, EE_AGG_SIZE)) {
		app_diag (D_SERIOUS, "ee_write failed %x %x",
				(word)(agg_data.eslot >> 16),
				(word)agg_data.eslot);
		agg_data.eslot--;
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

	if (in_pong_pload(buf)) {
		pong_ack.header.rcv = in_header(buf, snd);
		pong_ack.header.hco = in_header(buf, hoc);
		pong_ack.ts = in_pongPload(buf, ts);

		if (master_delta == 0) { // do NOT send down your own clock
			pong_ack.reftime = 0;
		} else {
			mc.sec = 0;
			wall_time (&mc);
			pong_ack.reftime = mc.sec;
		}
		
		send_msg ((char *)&pong_ack, sizeof(msgPongAckType));
	}
	if (msg4tag.buf && in_header(msg4tag.buf, rcv) ==
		       in_header(buf, snd)) {
		if (seconds() - msg4tag.tstamp <= 77) // do it
			send_msg (msg4tag.buf, sizeof(msgSetTagType));
		ufree (msg4tag.buf);
		msg4tag.buf = NULL;
		msg4tag.tstamp = 0;
	}
}

#if 0

jebane lokalne globale FIXME

__PUBLF (NodePeg, word, r_a_d) () {
	char * lbuf = NULL;

	if (ee_read (agg_dump->ind * EE_AGG_SIZE, (byte *)&agg_dump->ee,
			EE_AGG_SIZE)) {
		app_diag (D_SERIOUS, "Failed ee_read");
		goto Finish;
	}

	if (ee.status == 0xFF)
		if (agg_dump->fr <= agg_dump.to) {
			goto Finish;
		} else {
			goto Continue;
		}

	if (agg_dump->tag == 0 || agg_dump->ee.tag == agg_dump->tag) {
		lbuf = form (NULL, "\r\nCol %u slot %lu (A: %lu), "
				"ts: %ld (A; %ld)\r\n"
				" PAR: %d, T: %d, H: %d, PD: %d, T2: %d\r\n",
			agg_dump->ee.tag, agg_dump->ee.t_eslot, agg_dump->slot,
		       	agg_dump->ee.t_ts, agg_dump->ee.ts,
			agg_dump->ee.sval[0],
		       	agg_dump->ee.sval[1],
		       	agg_dump->ee.sval[2],
			agg_dump->ee.sval[3],
			agg_dump->ee.sval[4]);
		if (runstrand (oss_out, lbuf) == 0 ) {
			app_diag (D_SERIOUS, "oss_out failed");
			ufree (lbuf);
		}
		agg_dump->cnt++;
		if (agg_dump->upto != 0 && agg_dump->upto <= agg_dump->cnt)
			goto Finish;
	}

Continue:
	if (agg_dump->fr <= agg_dump->to) {
		if (agg_dump->ind >= agg_dump->to)
			goto Finish;
		else
			agg_dump->ind++;
	}
	return 1;

Finish:
	lbuf = form (NULL, "Dump data: Col %u,fr %lu to %lu up to %u #%lu",
			agg_dump->tag, agg_dump->fr, agg_dump->to,
			agg_dump->upto, agg_dump->cnt);
	if (runstrand (oss_out, lbuf) == 0 ) {
		app_diag (D_SERIOUS, "oss_out sum failed");
		ufree (lbuf);
	}
	return 0;
}
#endif

__PUBLF (NodePeg, void, agg_init) () {
	lword l, u, m;
	byte b;

	if (ee_read (0, &b, 1))
		fatal_err (ERR_EER, 0, 0, 1);

	memset (&agg_data, 0, sizeof(aggDataType));

	if (b == 0xFF) { // normal operations
		agg_data.eslot = 0;
		agg_data.ee.status = AGG_FF;
		return;
	}

	// only efter power down / soft reset there could be anything in eeprom
	l = EE_AGG_MIN; u = EE_AGG_MAX;
	while (u - l > 1) {
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
	leds (0, 2);
	if_write (IFLASH_SIZE -1, err);
	if_write (IFLASH_SIZE -2, w1);
	if_write (IFLASH_SIZE -3, w2);
	if_write (IFLASH_SIZE -4, w3);
	app_diag (D_FATAL, "HALT %x %u %u %u", err, w1, w2, w3);
	halt();
}
