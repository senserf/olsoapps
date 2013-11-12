/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011 			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "diag.h"
#include "app_peg.h"
#include "msg_peg.h"

#include "net.h"
#include "tarp.h"
#include "form.h"

#include "app_peg_data.h"

/*
 * "Virtual" stuff needed by NET & TARP =======================================
 */
idiosyncratic word  guide_rtr (headerType *  b) {
	return (b->rcv == 0 || b->msg_type  == msg_pong ||
			b->msg_type == msg_pongAck) ? 1 : 2;
}

idiosyncratic int tr_offset (headerType *h) {
	// Unused ??
	return 0;
}

idiosyncratic Boolean msg_isBind (msg_t m) {
	return NO;
}

idiosyncratic Boolean msg_isTrace (msg_t m) {
	return NO;
}

idiosyncratic Boolean msg_isMaster (msg_t m) {
	return (m == msg_master);
}

idiosyncratic Boolean msg_isNew (msg_t m) {
	return NO;
}

idiosyncratic Boolean msg_isClear (byte o) {
	return YES;
}

idiosyncratic void set_master_chg () {
	app_flags |= 2;
}

/*
   what == 0: find and return the index;
   what == 1: count
*/
idiosyncratic sint find_tags (word tag, word what) {

	sint i = 0;
	sint count = 0;

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

idiosyncratic char *get_mem (word state, sint len) {

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

idiosyncratic void init_tag (word i) {

	tagArray[i].id = 0;
	tagArray[i].rssi = 0;
	tagArray[i].pl = 0;
	tagArray[i].rxperm = 0;
	tagArray[i].state = noTag;
	tagArray[i].freq = 0;
	tagArray[i].count = 0;
	tagArray[i].evTime = 0;
	tagArray[i].lastTime = 0;
	tagArray[i].rpload.ppload.ds = 0x80000000;
}

idiosyncratic void init_tags () {

	word i = tag_lim;
	while (i-- > 0)
		init_tag (i);
}

idiosyncratic void set_tagState (word i, tagStateType state,
							Boolean updEvTime) {
	tagArray[i].state = state;
	tagArray[i].count = 0; // always (?) reset the counter
	tagArray[i].lastTime = seconds();
	if (updEvTime)
		tagArray[i].evTime = tagArray[i].lastTime;

}

idiosyncratic sint insert_tag (word tag) {

	sint i = 0;

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
idiosyncratic void check_tag (word state, word i, char** buf_out) {

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

idiosyncratic void copy_fwd_msg (word state, char** buf_out, char * buf,
								word size) {
	if (*buf_out == NULL)
		*buf_out = get_mem (state, size);
	else
		memset (*buf_out, 0, size);

	memcpy (*buf_out, buf, size);
}

idiosyncratic void send_msg (char * buf, sint size) {

	// it doesn't seem like a good place to filter out
	// local host, but it's convenient, for now...

	// this shouldn't be... WARNING to see why it is needed...
	if (in_header(buf, rcv) == local_host) {
		app_diag (D_WARNING, "Dropped msg(%u) to lh",
			in_header(buf, msg_type));
		return;
	}
	if (net_tx (WNONE, buf, size, 0) == 0) {
		app_diag (D_DEBUG, "Sent(%d) %u to %u", size,
			in_header(buf, msg_type),
			in_header(buf, rcv));
	} else
		app_diag (D_SERIOUS, "Tx %u failed",
			in_header(buf, msg_type));
}

idiosyncratic sint check_msg_size (char * buf, word size, word repLevel) {

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

		default:
			// this may be useful, but we're running out of space:
			//app_diag (repLevel, "Can't check size of %u (%d)",
			//	in_header(buf, msg_type), size);
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

idiosyncratic void check_msg4tag (char * buf) {

	// do NOT send down your own date unless you're the Master
	lint md = master_ts != 0 || local_host == master_host ?
		wall_date (0) : 0;
#if 0
	let's see how spot handles it
	if (msg4tag.buf && in_header(msg4tag.buf, rcv) ==
		       in_header(buf, snd)) { // msg waiting

		if (in_pong_pload(buf)) { // add ack data
			in_setTag(msg4tag.buf, ds) = in_pongPload(buf, ds);
			in_setTag(msg4tag.buf, refdate) = md;
			in_setTag(msg4tag.buf, syfreq) = sync_freq;
		} else {
			in_setTag(msg4tag.buf, refdate) = 0;
			in_setTag(msg4tag.buf, ds) = 0; 
			in_setTag(msg4tag.buf, syfreq) = 0;
		}

		send_msg (msg4tag.buf, sizeof(msgSetTagType));
		ufree (msg4tag.buf);
		msg4tag.buf = NULL;
		msg4tag.tstamp = 0;

	} else { // no msg waiting; send ack
#endif
		if (in_pong_pload(buf)) {
			pong_ack.header.rcv = in_header(buf, snd);
			pong_ack.header.hco = in_header(buf, hoc);
			pong_ack.ds = in_pongPload(buf, ds);
			pong_ack.refdate = md;
			pong_ack.syfreq = sync_freq;
			send_msg ((char *)&pong_ack, sizeof(msgPongAckType));
		}
//	}
}

idiosyncratic void agg_init () {

	lword l, u, m;
	byte b;

	memset (&agg_data, 0, sizeof(aggDataType));

	agg_data.eslot = 0;
	agg_data.ee.s.f.mark = 0;
	agg_data.ee.s.f.status = 0;
}

idiosyncratic void fatal_err (word err, word w1, word w2, word w3) {

	//leds (LED_R, LED_BLINK);
	if_write (IFLASH_SIZE -1, err);
	if_write (IFLASH_SIZE -2, w1);
	if_write (IFLASH_SIZE -3, w2);
	if_write (IFLASH_SIZE -4, w3);
	if (err != ERR_MAINT) {
		leds (LED_R, LED_ON);
		app_diag (D_FATAL, "HALT %x %u %u %u", err, w1, w2, w3);
		halt();
	}
	reset();
}

idiosyncratic word handle_a_flags (word a_fl) {

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

idiosyncratic sint str_cmpn (const char * s1, const char * s2, sint n) {

	while (n-- && (*s1 != '\0') && (*s2 != '\0'))
		if (*s1++ != *s2++)
			return 1;
	return (n == -1 ? 0 : -1);
}

idiosyncratic lint wall_date (lint s) {

        lint x = seconds() - master_ts - s;

        x = master_date < 0 ? master_date - x : master_date + x;
        return x;
}
