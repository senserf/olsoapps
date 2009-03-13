/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* ===================================================================
    This is a placeholder for maintenance and test functionality
    for EcoNet. Implemented:
    - EMS sattelite i/f test

    - next: eprom dumps over RF

    We start with aggregator's code, but likely will branch into an
    independent praxis or blend back into the aggregator.
    =================================================================== */

#include "globals_cus.h"
#include "threadhdrs_cus.h"
#include "flash_stamps.h"
#include "sat_cus.h"

#ifndef __SMURPH__
#include "lhold.h"
#endif

// elsewhere may be a better place for this:
#if CC1000
#define INFO_PHYS_DEV INFO_PHYS_CC1000
#endif

#if CC1100
#define INFO_PHYS_DEV INFO_PHYS_CC1100
#endif

#if DM2200
#define INFO_PHYS_DEV INFO_PHYS_DM2200
#endif

#ifndef INFO_PHYS_DEV
#error "UNDEFINED RADIO"
#endif

#define UI_BUFLEN		UART_INPUT_BUFFER_LENGTH
#define DEF_NID			85
#define DEF_MHOST		10

// Semaphores
#define CMD_READER	(&cmd_line)
#define CMD_WRITER	((&cmd_line)+1)

// =============
// OSS reporting
// =============
/* Custodian that IS_SATGAT is a very starnge beast: it is supposed to
   pass to the PDT only content of msg_satest. Therefore, all other calls to
   oss_out() are voided, with all consequences.
*/
strand (oss_out, char)
       entry (OO_RETRY)
		if (data == NULL)
			app_diag (D_SERIOUS, "NULL in oss_out");
		else
			ser_outb (OO_RETRY, data);
		finish;
endstrand

// sat_in, sat_out should be together and in sat_cus.cc, however,
// because of the crappy 'globals' runstrand() would have to be
// called from app_cus.c anyway... forget it until we have the
// vuee / picos stuff sorted out
__PUBLF (NodeCus, void, sat_in) () {
	int len;
	char * obuf;

	// packetization is not really needed (?);
	if ((len = strlen (cmd_line)) > MAX_SATLEN) {
		diag ("sat_in cut %d %d", len, MAX_SATLEN);
		len = MAX_SATLEN;
		cmd_line[MAX_SATLEN] = '\0';
	}

	// patch the crap back to normal mode
	if (strlen (cmd_line) == 4 && str_cmpn (cmd_line, "s---", 4) == 0) {
		sat_mod = SATMOD_NO;
		stats (NULL);
		return;
	}

	len += sizeof (headerType) +1; // + '\0'
	if ((obuf  = get_mem (WNONE, len)) == NULL)
		return;
	strcpy (&obuf[sizeof(headerType)], cmd_line);
	in_header(obuf, msg_type) = msg_satest;
	in_header(obuf, rcv) = 0; // must be bcast
	in_header(obuf, hco) = 1; // no hopping
	send_msg (obuf, len);
	ufree (obuf);
}

__PUBLF (NodeCus, void, sat_out) (char * buf) {
	int len = strlen (&buf[sizeof (headerType)]) +1; // + '\0'
	char * mbuf;

	if (!IS_SATGAT)
		len += 2; // LF, CR

	if ((mbuf = get_mem (WNONE, len)) == NULL)
		return;

	strcpy (mbuf, &buf[sizeof (headerType)]);

	if (!IS_SATGAT) {
		mbuf[len -1] = '\0';
		mbuf[len -2] = (char)10; // LF
		mbuf[len -3] = (char)13; // CR
	}

        if (runstrand (oss_out, mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out fork");
		ufree (mbuf);
	}
}

__PUBLF (NodeCus, void, show_ifla) () {
	char * mbuf = NULL;

	if (IS_SATGAT)
		return;

	if (if_read (0) == 0xFFFF) {
		diag (OPRE_APP_ACK "No custom sys data");
		return;
	}
	mbuf = form (NULL, ifla_str, if_read (0), if_read (1), if_read (2),
			if_read (3), if_read (4), if_read (5), if_read(6));

	if (runstrand (oss_out, mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out fork");
		ufree (mbuf);
	}
}

__PUBLF (NodeCus, void, read_ifla) () {

	if (if_read (0) == 0xFFFF) { // usual defaults
		local_host = (word)host_id;
#ifndef __SMURPH__
		master_host = DEF_MHOST;
#endif
		sat_mod = IS_DEFSATGAT ? SATMOD_YES : SATMOD_NO;
		return;
	}

	local_host = if_read (0);
	host_pl = if_read (1);
	app_flags = if_read (2);
	tag_auditFreq = if_read (3);
	master_host = if_read (4);
	sync_freq = if_read (5);
	if (sync_freq > 0) {
		master_delta = seconds();
		master_clock.hms.d = 0;
		master_clock.hms.h = 0;
		master_clock.hms.m = 0;
		master_clock.hms.s = 0;
		master_clock.hms.f = 1;
		diag (OPRE_APP_ACK "Implicit T 0:0:0");
	}
	sat_mod = if_read (6);
}


__PUBLF (NodeCus, void, save_ifla) () {
	if (if_read (0) != 0xFFFF) {
		if_erase (0);
		diag (OPRE_APP_ACK "Flash p0 overwritten");
	}
	// there is 'show' after 'save'... don't check if_writes here (?)
	if_write (0, local_host);
	if_write (1, host_pl);
	if_write (2, (app_flags & 0xFFFC)); // off spare, master chg
	if_write (3, tag_auditFreq);
	if_write (4, master_host);
	if_write (5, sync_freq);
	if_write (6, IS_SATGAT ? SATMOD_YES : SATMOD_NO);
}

// Display node stats on UI
__PUBLF (NodeCus, void, stats) (char * buf) {
	char * mbuf = NULL;
	word mmin, mem;

	if (IS_SATGAT)
		return;

	if (buf == NULL) {
		mem = memfree(0, &mmin);
		mbuf = form (NULL, stats_str,
			host_id, local_host, tag_auditFreq,
			host_pl, handle_a_flags (0xFFFF), seconds(),
		       	master_delta, master_host,
			agg_data.eslot == EE_AGG_MIN &&
			  agg_data.ee.s.f.status == AGG_FF ?
			0 : agg_data.eslot - EE_AGG_MIN +1,
			mem, mmin, sat_mod);
	} else {
	  switch (in_header (buf, msg_type)) {
	    case msg_statsPeg:
		mbuf = form (NULL, stats_str,
			in_statsPeg(buf, hostid), in_header(buf, snd),
			in_statsPeg(buf, audi), in_statsPeg(buf, pl),
			in_statsPeg(buf, a_fl),
			in_statsPeg(buf, ltime), in_statsPeg(buf, mdelta),
			in_statsPeg(buf, mhost), in_statsPeg(buf, slot),
			in_statsPeg(buf, mem), in_statsPeg(buf, mmin), 12345);
		break;

	    case msg_statsTag:
		mbuf = form (NULL, statsCol_str,
			in_statsTag(buf, hostid),
			(word)in_statsTag(buf, hostid), in_header(buf, snd),
			in_statsTag(buf, maj), in_statsTag(buf, min),
			in_statsTag(buf, span), in_statsTag(buf, pl),
			in_statsTag(buf, c_fl),
			in_statsTag(buf, ltime), in_statsTag(buf, slot),
			in_statsTag(buf, mem), in_statsTag(buf, mmin));
		break;

	    default:
		app_diag (D_SERIOUS, "Bad stats type %u", 
			in_header (buf, msg_type));
	  }
	}

	if (runstrand (oss_out, mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out fork");
		ufree (mbuf);
	}
}

__PUBLF (NodeCus, void, process_incoming) (word state, char * buf, word size,
								word rssi) {
  int    w_len;

  if (check_msg_size (buf, size, D_SERIOUS) != 0)
	  return;

  switch (in_header(buf, msg_type)) {

	case msg_pong:
#if 0
		if (in_header(buf, snd) / 1000 != local_host / 1000)
			return;

		if (in_pong_rxon(buf)) 
			check_msg4tag (buf);
#endif
		msg_pong_in (state, buf, rssi);
		return;

	case msg_report:
		msg_report_in (state, buf);
		return;

	case msg_reportAck:
		msg_reportAck_in (buf);
		return;

	case msg_master:
		msg_master_in (buf);
		return;

	case msg_findTag:
		msg_findTag_in (state, buf);
		return;

	case msg_fwd:
		msg_fwd_in (state, buf, size);
		return;

	case msg_setPeg:
		msg_setPeg_in (buf);
		return;

	case msg_statsPeg:
		stats (buf);
		return;

	case msg_statsTag:
		stats (buf);
		return;

	case msg_rpc:
		if (cmd_line != NULL) { // busy with another input
			when (CMD_WRITER, state);
			release;
		}
		w_len = size - sizeof(msgRpcType);
		cmd_line = get_mem (state, w_len);
		memcpy (cmd_line, buf + sizeof(msgRpcType), w_len);
		trigger (CMD_READER);
		return;

	case msg_satest:
		sat_out (buf);
		return;

	default:
		app_diag (D_SERIOUS, "Got ? (%u)", in_header(buf, msg_type));

  }
}

// [0, FF] -> [1, F]
// it can't be 0, as find_tags() will mask the rssi out!
static word map_rssi (word r) {
#if 0
#ifdef __SMURPH__
/* temporary rough estimates
 =======================================================
 RP(d)/XP [dB] = -10 x 5.1 x log(d/1.0m) + X(1.0) - 33.5
 =======================================================
 151, 118

*/
	if ((r >> 8) > 151) return 3;
	if ((r >> 8) > 118) return 2;
	return 1;
#else
	if ((r >> 8) > 161) return 3;
	if ((r >> 8) > 140) return 2;
	return 1;
#endif
#endif
	return 1; // eco demo: don't overwhelm
}

/*
   --------------------
   Receiver process
   RS_ <-> Receiver State
   --------------------
*/

// In this model, a single rcv is forked once, and runs / sleeps all the time
thread (rcv)

	entry (RC_INIT)

		rcv_packet_size = 0;
		rcv_buf_ptr = NULL;
		rcv_rssi = 0;

	entry (RC_TRY)

		if (rcv_buf_ptr != NULL) {
			ufree (rcv_buf_ptr);
			rcv_buf_ptr = NULL;
			rcv_packet_size = 0;
		}
    		rcv_packet_size = net_rx (RC_TRY, &rcv_buf_ptr, &rcv_rssi, 0);
		if (rcv_packet_size <= 0) {
			app_diag (D_SERIOUS, "net_rx failed (%d)",
				rcv_packet_size);
			proceed (RC_TRY);
		}

		app_diag (D_DEBUG, "RCV (%d): %x-%u-%u-%u-%u-%u\r\n",			  
		rcv_packet_size, in_header(rcv_buf_ptr, msg_type),
			  in_header(rcv_buf_ptr, seq_no) & 0xffff,
			  in_header(rcv_buf_ptr, snd),
			  in_header(rcv_buf_ptr, rcv),
			  in_header(rcv_buf_ptr, hoc) & 0xffff,
			  in_header(rcv_buf_ptr, hco) & 0xffff);

		// that's how we could check which plugin is on
		// if (net_opt (PHYSOPT_PLUGINFO, NULL) != INFO_PLUG_TARP)

	entry (RC_MSG)
#if 0
	will be needed for all sorts of calibrations
//#endif
		if (in_header(rcv_buf_ptr, msg_type) == msg_pong)
			app_diag (D_UI, "rss (%d.%d): %d",
				in_header(rcv_buf_ptr, snd),
				in_pong(rcv_buf_ptr, level), rcv_rssi >> 8);
		else
			app_diag (D_UI, "rss %d from %d", rcv_rssi >> 8,
					in_header(rcv_buf_ptr, snd));
#endif
		process_incoming (RC_MSG, rcv_buf_ptr, rcv_packet_size,
			map_rssi(rcv_rssi));
		proceed (RC_TRY);

endthread

/*
  --------------------
  audit process
  AS_ <-> Audit State
  --------------------
*/

thread (audit)

	nodata;

	entry (AS_INIT)

		aud_buf_ptr = NULL;

	entry (AS_START)

		if (aud_buf_ptr != NULL) {
			ufree (aud_buf_ptr);
			aud_buf_ptr = NULL;
		}
		if (tag_auditFreq == 0) {
			app_diag (D_WARNING, "Audit stops");
			finish;
		}
		aud_ind = tag_lim;
		app_diag (D_DEBUG, "Audit starts");

	entry (AS_TAGLOOP)

		if (aud_ind-- == 0) {
			app_diag (D_DEBUG, "Audit ends");
			lh_time = tag_auditFreq;
			proceed (AS_HOLD);
		}

	entry (AS_TAGLOOP1)

		check_tag (AS_TAGLOOP1, aud_ind, &aud_buf_ptr);

		if (aud_buf_ptr) {
			in_header(aud_buf_ptr, snd) = local_host;
			oss_report_out (aud_buf_ptr);

			ufree (aud_buf_ptr);
			aud_buf_ptr = NULL;
		}
		proceed (AS_TAGLOOP);

	entry (AS_HOLD)
		lhold (AS_HOLD, &lh_time);
		proceed (AS_START);
endthread

__PUBLF (NodeCus, void, tmpcrap) (word what) {
	switch (what) {
		case 0:
			if (tag_auditFreq != 0 && !running (audit))
				runthread (audit);
			return;

		default:
			app_diag (D_SERIOUS, "Crap");
	}
}

/*
   --------------------
   cmd_in process
   CS_ <-> Command State
   --------------------
*/
thread (cmd_in)

	nodata;

	entry (CS_INIT)

		if (ui_ibuf == NULL)
			ui_ibuf = get_mem (CS_INIT, UI_BUFLEN);

	entry (CS_IN)

		// hangs on the uart_a interrupt or polling
		ser_in (CS_IN, ui_ibuf, UI_BUFLEN);
		if (strlen(ui_ibuf) == 0)
			// CR on empty line would do it
			proceed (CS_IN);

	entry (CS_WAIT)

		if (cmd_line != NULL) {
			when (CMD_WRITER, CS_WAIT);
			release;
		}

		cmd_line = get_mem (CS_WAIT, strlen(ui_ibuf) +1);
		strcpy (cmd_line, ui_ibuf);
		trigger (CMD_READER);
		proceed (CS_IN);

endthread

#if 0
static char * stateName (unsigned state) {
	switch ((tagStateType)state) {
		case noTag:
			return "noTag";
		case newTag:
			return "newTag";
		case reportedTag:
			return "reportedTag";
		case confirmedTag:
			return "confirmedTag";
		case fadingReportedTag:
			return "fadingReportedTag";
		case fadingConfirmedTag:
			return "fadingConfirmedTag";
		case goneTag:
			return "goneTag";
		case sumTag:
			return "sumTag";
		default:
			return "unknown?";
	}
}

static char * locatName (word id, word rssi) {
	if (id == 0)
		return "total";

	switch (rssi) {
		case 3:
			return "proxy";
		case 2:
			return "near";
		case 1:
			return "far";
		case 0:
			return "no";
	}
	return "rssi?";
	// should be more... likely a number with  distance?
}
#endif

__PUBLF (NodeCus, void, oss_report_out) (char * buf) {

  char * lbuf = NULL;

  if (IS_SATGAT)
	  return;

  if (in_report_pload(buf)) {

	lbuf = form (NULL, rep_str,

		in_header(buf, snd),
		in_reportPload(buf, eslot),

		((mclock_t *)&in_reportPload(buf, ts))->hms.f ?
			"time" : "ts",
		((mclock_t *)&in_reportPload(buf, ts))->hms.d,
		((mclock_t *)&in_reportPload(buf, ts))->hms.h,
		((mclock_t *)&in_reportPload(buf, ts))->hms.m,
		((mclock_t *)&in_reportPload(buf, ts))->hms.s,

		in_report(buf, tagid),
		in_reportPload(buf, ppload.eslot),

		((mclock_t *)&in_reportPload(buf, ppload.ts))->hms.f ?
			"time" : "ts",
		((mclock_t *)&in_reportPload(buf, ppload.ts))->hms.d,
		((mclock_t *)&in_reportPload(buf, ppload.ts))->hms.h,
		((mclock_t *)&in_reportPload(buf, ppload.ts))->hms.m,
		((mclock_t *)&in_reportPload(buf, ppload.ts))->hms.s,

		in_report(buf, state) == goneTag ?
			" ***gone***" : " ",

		in_reportPload(buf, ppload.sval[0]),
		in_reportPload(buf, ppload.sval[1]),
		in_reportPload(buf, ppload.sval[2]),
		in_reportPload(buf, ppload.sval[3]),
		in_reportPload(buf, ppload.sval[4])); // eoform

    } else if (in_report(buf, state) == sumTag) {
		lbuf = form (NULL, repSum_str,
			in_header(buf, snd), in_report(buf, count));

    } else if (in_report(buf, state) == noTag) {
		lbuf = form (NULL, repNo_str,
			in_report(buf, tagid), in_header(buf, snd));

    } else {
		app_diag (D_WARNING, "%sReport? %u %u %u", OPRE_DIAG,
			in_header(buf, snd), in_report(buf, tagid),
			in_report(buf, state));
    }

    if (runstrand (oss_out, lbuf) == 0 ) {
	app_diag (D_SERIOUS, "oss_out failed");
	ufree (lbuf);
    }
}

__PUBLF (NodeCus, word, r_a_d) () {
	char * lbuf = NULL;

	if (IS_SATGAT)
		return 0;

	if (agg_dump->dfin) // delayed Finish
		goto ThatsIt;

	if (ee_read (agg_dump->ind * EE_AGG_SIZE, (byte *)&agg_dump->ee,
				EE_AGG_SIZE)) {
		app_diag (D_SERIOUS, "Failed ee_read");
		goto Finish;
	}

	if (agg_dump->ee.s.f.status == AGG_FF) {
		if (agg_dump->fr <= agg_dump->to) {
			goto Finish;
		} else {
			goto Continue;
		}
	}

	if (agg_dump->tag == 0 || agg_dump->ee.tag == agg_dump->tag) {
		lbuf = form (NULL, dump_str,

			agg_dump->ee.tag, agg_dump->ee.t_eslot, agg_dump->ind,

			((mclock_t *)&agg_dump->ee.t_ts)->hms.f ? "time" : "ts",
			((mclock_t *)&agg_dump->ee.t_ts)->hms.d,
			((mclock_t *)&agg_dump->ee.t_ts)->hms.h,
			((mclock_t *)&agg_dump->ee.t_ts)->hms.m,
			((mclock_t *)&agg_dump->ee.t_ts)->hms.s,

			((mclock_t *)&agg_dump->ee.ts)->hms.f ? "time" : "ts",
			((mclock_t *)&agg_dump->ee.ts)->hms.d,
			((mclock_t *)&agg_dump->ee.ts)->hms.h,
			((mclock_t *)&agg_dump->ee.ts)->hms.m,
			((mclock_t *)&agg_dump->ee.ts)->hms.s,

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
	} else {
		if (agg_dump->ind <= agg_dump->to)
			goto Finish;
		else
			agg_dump->ind--;
	}
	return 1;

Finish:
	// ser_out tends to switch order... delay the output
	agg_dump->dfin = 1;
	return 1;

ThatsIt:
	agg_dump->dfin = 0; // just in case
	lbuf = form (NULL, dumpend_str,
			agg_dump->tag, agg_dump->fr, agg_dump->to,
			agg_dump->upto, agg_dump->cnt);

	if (runstrand (oss_out, lbuf) == 0 ) {
		app_diag (D_SERIOUS, "oss_out sum failed");
		ufree (lbuf);
	}
	return 0;
}

__PUBLF (NodeCus, void, oss_findTag_in) (word state, nid_t tag, nid_t peg) {

	char * out_buf = NULL;
	int tagIndex;

	if (peg == local_host || peg == 0) {
		if (tag == 0) { // summary
			tagIndex = find_tags (tag, 1);
			msg_report_out (state, tagIndex | 0x8000, &out_buf,
					REP_FLAG_NOACK);
			if (out_buf == NULL)
				return;
		} else {
			tagIndex = find_tags (tag, 0);
			msg_report_out (state, tagIndex, &out_buf,
				tagIndex < 0 ? REP_FLAG_NOACK :
					REP_FLAG_NOACK | REP_FLAG_PLOAD);
			if (out_buf == NULL)
				return;

			// as in msg_findTag_in, kludge summary into
			// missing tag:
			if (tagIndex < 0) {
				in_report(out_buf, tagid) = tag;
				in_report(out_buf, state) = noTag;
			}
		}

		in_header(out_buf, snd) = local_host;
		// don't report bulk missing (but do summary)
		if ((word)tag == 0 || peg != 0 || tagIndex >= 0)
			oss_report_out (out_buf);

	}

	if (peg != local_host) {
		msg_findTag_out (state, &out_buf, tag, peg);
		send_msg (out_buf, sizeof(msgFindTagType));
	}
	ufree (out_buf);
}

__PUBLF (NodeCus, void, oss_setTag_in) (word state, word tag,
	       	nid_t peg, word maj, word min, 
		word span, word pl, word c_fl) {

	char * out_buf = NULL;
	char * set_buf = NULL;
	int size = sizeof(msgFwdType) + sizeof(msgSetTagType);

#if 0
       	already checked
	if (peg == 0 || tag == 0) {
		app_diag (D_WARNING, "set: no zeroes");
		return;
	}
#endif
	// alloc and prepare msg fwd
	msg_fwd_out (state, &out_buf, size, tag, peg);
	// get offset for payload - setTag msg
	set_buf = out_buf + sizeof(msgFwdType);
	in_header(set_buf, msg_type) = msg_setTag;
	in_header(set_buf, rcv) = tag;
	in_header(set_buf, snd) = local_host;
	in_header(set_buf, hco) = 1; // encapsulated is a proxy msg
	in_setTag(set_buf, pow_levels) = pl;
	in_setTag(set_buf, freq_maj) = maj;
	in_setTag(set_buf, freq_min) = min;
	in_setTag(set_buf, rx_span) = span;
	in_setTag(set_buf, c_fl) = c_fl;
	if (peg == local_host || peg == 0)
		// put it in the wroom
		msg_fwd_in(state, out_buf, size);
	if (peg != local_host)
		send_msg (out_buf,  size);
	ufree (out_buf);
}

__PUBLF (NodeCus, void, oss_setPeg_in) (word state, nid_t peg, 
				word audi, word pl, word a_fl) {

	char * out_buf = get_mem (state, sizeof(msgSetPegType));

	in_header(out_buf, msg_type) = msg_setPeg;
	in_header(out_buf, rcv) = peg;
	in_setPeg(out_buf, level) = pl;
	in_setPeg(out_buf, audi) = audi;
	in_setPeg(out_buf, a_fl) = a_fl;
	send_msg (out_buf,  sizeof(msgSetPegType));
	ufree (out_buf);
}

// ==========================================================================

/*
   --------------------
   Root process
   RS_ <-> Root State
   --------------------
*/

thread (root)

	mclock_t mc;
	sint	i1, i2, i3, i4, i5, i6, i7;

	entry (RS_INIT)
		ui_obuf = get_mem (RS_INIT, UI_BUFLEN);
		ee_open ();
		form (ui_obuf, ee_str, EE_AGG_MIN, EE_AGG_MAX -1, EE_AGG_SIZE);

		if (if_read (IFLASH_SIZE -1) != 0xFFFF)
			leds (LED_R, LED_BLINK);
		else
			leds (LED_G, LED_BLINK);

		if (is_flash_new) {
			diag (OPRE_APP_ACK "Init eprom erase");
			if (ee_erase (WNONE, 0, 0))
				app_diag (D_SERIOUS, "erase failed");
			break_flash;
			read_ifla();
		} else {
			read_ifla();
			delay (5000, RS_INIT1);
			release;
		}

	entry (RS_INIT1)
		if (!IS_SATGAT)
			ser_out (RS_INIT1, ui_obuf);
		agg_init();

		if (if_read (IFLASH_SIZE -1) != 0xFFFF) {
			leds (LED_R, LED_OFF);
			diag (OPRE_APP_MENU_A "***Maintenance mode***"
				OMID_CRB "%x %u %u %u",
				if_read (IFLASH_SIZE -1),
				if_read (IFLASH_SIZE -2),
				if_read (IFLASH_SIZE -3),
				if_read (IFLASH_SIZE -4));
			if (!running (cmd_in))
				runthread (cmd_in);
			stats(NULL);
			proceed (RS_RCMD);
		}
		leds (LED_G, LED_OFF);

	entry (RS_INIT2)
		if (!IS_SATGAT)
			ser_out (RS_INIT2, welcome_str);

#ifndef __SMURPH__
		net_id = DEF_NID;
#endif
		tarp_ctrl.param = 0xB0; // level 2, rec 3, slack 0, fwd off

		init_tags();

		// spread a bit in case of a sync reset
		delay (rnd() % 1000, RS_PAUSE);
		release;

	entry (RS_PAUSE)
		if (net_init (INFO_PHYS_DEV, INFO_PLUG_TARP) < 0) {
			app_diag (D_FATAL, "net_init failed");
			reset();
		}
		net_opt (PHYSOPT_SETSID, &net_id);
		net_opt (PHYSOPT_SETPOWER, &host_pl);
		runthread (rcv);
		runthread (cmd_in);
		runthread (audit);
		proceed (RS_RCMD);

	entry (RS_FREE)

		ufree (cmd_line);
		cmd_line = NULL;
		trigger (CMD_WRITER);

	entry (RS_RCMD)

		if (cmd_line == NULL) {
			when (CMD_READER, RS_RCMD);
			release;
		}

	entry (RS_DOCMD)
		if (IS_SATGAT) {
			sat_in();
			proceed (RS_FREE);
		}

		if (master_host != local_host &&
				(cmd_line[0] == 'T' || cmd_line[0] == 'c' ||
				 cmd_line[0] == 'f')) {
			strcpy (ui_obuf, only_master_str);
			proceed (RS_UIOUT);
		}

		if (if_read (IFLASH_SIZE -1) != 0xFFFF &&
				(cmd_line[0] == 'c' ||
				cmd_line[0] == 'f')) {
			strcpy (ui_obuf, not_in_maint_str);
			proceed (RS_UIOUT);
		}

	  switch (cmd_line[0]) {

		case ' ': proceed (RS_FREE); // ignore if starts with blank

                case 'h':
			ser_out (RS_DOCMD, welcome_str);
			proceed (RS_FREE);

		case 'T':
			i1 = i2 = i3 = 0;

			if ((i4 = scan (cmd_line+1, "%u:%u:%u", &i1,
							&i2, &i3)) != 3 &&
					i4 != 0) {
				form (ui_obuf, bad_str, cmd_line);
				proceed (RS_UIOUT);
			}

			if (i4 == 3) {
				if (i1 > 23 || i2 > 59 || i3 > 59) {
					form (ui_obuf, bad_str, cmd_line);
					proceed (RS_UIOUT);
				}
				master_delta = seconds();
				master_clock.hms.d = 0;
				master_clock.hms.h = i1;
				master_clock.hms.m = i2;
				master_clock.hms.s = i3;
				master_clock.hms.f = 1;
			}

			mc.sec = 0;
			wall_time (&mc);
			form (ui_obuf, clock_str, mc.hms.f ? "time" : "ts",
					mc.hms.d, mc.hms.h, mc.hms.m,
					mc.hms.s, seconds());
			proceed (RS_UIOUT);

		case 'q': reset();

		case 'D':
			agg_dump = (aggEEDumpType *)
				get_mem (WNONE, sizeof(aggEEDumpType));
			
			if (agg_dump == NULL )
				proceed (RS_FREE);

			memset (agg_dump, 0, sizeof(aggEEDumpType));
			i1 = 0;
			agg_dump->fr = EE_AGG_MIN;
			agg_dump->to = agg_data.eslot;

			scan (cmd_line+1, "%lu %lu %u %u",
					&agg_dump->fr, &agg_dump->to,
					&agg_dump->tag, &i1);
			agg_dump->upto = i1; // :15, that's why

			if (agg_dump->fr > agg_data.eslot)
				agg_dump->fr = agg_data.eslot;

			if (agg_dump->fr < EE_AGG_MIN)
				agg_dump->fr = EE_AGG_MIN;

			if (agg_dump->to > agg_data.eslot)
				agg_dump->to = agg_data.eslot; //EE_AGG_MAX;

			if (agg_dump->to < EE_AGG_MIN)
				agg_dump->to = EE_AGG_MIN;

			agg_dump->ind = agg_dump->fr;
			proceed (RS_DUMP);

		case 'M':
			if (if_read (IFLASH_SIZE -1) != 0xFFFF) {
				diag (OPRE_APP_ACK "Already in maintenance");
				reset();
			}
			fatal_err (ERR_MAINT, (word)(seconds() >> 16),
					(word)(seconds()), 0);
			// will reset

		case 'E':
			diag (OPRE_APP_ACK "erasing eeprom...");
			if (ee_erase (WNONE, 0, 0))
				app_diag (D_SERIOUS, "erase failed");
			else
				diag (OPRE_APP_ACK "eeprom erased");
			reset();

		case 'F':
			if_erase (IFLASH_SIZE -1);
			break_flash;
			diag (OPRE_APP_ACK "flash p1 erased");
			reset();

		case 'Q':
			diag (OPRE_APP_ACK "erasing all...");
			if (ee_erase (WNONE, 0, 0))
				app_diag (D_SERIOUS, "ee_erase failed");
			if_erase (-1);
			break_flash;
			diag (OPRE_APP_ACK "all erased");
			reset();

		case 's':
			if (strlen (cmd_line) == 4 &&
				str_cmpn (&cmd_line[1], "+++", 3) == 0) {

				sat_mod = SATMOD_YES;
				proceed (RS_FREE);
			}
			if ((i1 = msg_satest_out (cmd_line)) < 0) {
				diag (OPRE_APP_ACK "Failed %d", i1);
				form (ui_obuf, bad_str, cmd_line);
				proceed (RS_UIOUT);
			}
			proceed (RS_FREE);

		case 'f':
			i1 = i2 = 0;
			scan (cmd_line+1, "%u %u", &i1, &i2);
			oss_findTag_in (RS_DOCMD, i1, i2);
			proceed (RS_FREE);

		case 'c':
			i1 = i2 = i3 = i4 = i5 = i6 = i7 = -1;
			
			// tag peg fM fm span pl
			scan (cmd_line+1, "%d %d %d %d %d %x %x",
				&i1, &i2, &i3, &i4, &i5, &i6, &i7);
			
			if (i1 <= 0 || i3 < -1 || i4 < -1 || i5 < -1) {
				form (ui_obuf, bad_str, cmd_line);
				proceed (RS_UIOUT);
			}

			if (i2 <= 0)
				i2 = local_host;

			oss_setTag_in (RS_DOCMD, i1, i2, i3, i4, i5, i6, i7);
			proceed (RS_FREE);
		
		case 'a':
			i1 = i2 = i3 = i4 = -1;
			if (scan (cmd_line+1, "%d %d %d %x", &i1, &i2, &i3, &i4)
					== 0 || i1 < 0)
				i1 = local_host;

			if (if_read (IFLASH_SIZE -1) != 0xFFFF &&
				i1 !=local_host) {
				strcpy (ui_obuf, not_in_maint_str);
				proceed (RS_UIOUT);
			}

			if (i2 < -1)
				i2 = -1;
			if (i3 < -1)
				i3 = -1;
			if (i1 == local_host || i1 == 0) {
				if (i2 >= 0) {
					tag_auditFreq = i2;
					if (tag_auditFreq != 0 &&
							!running (audit))
						runthread (audit);
				}
				if (i3 != -1) {
					host_pl = i3 > 7 ? 7 : i3;
					net_opt (PHYSOPT_SETPOWER, &host_pl);
				}
				(void)handle_a_flags ((word)i4);
				stats (NULL);

			}
			if (i1 != local_host) {
				oss_setPeg_in (RS_DOCMD, i1, i2, i3, i4);
			}

			proceed (RS_FREE);

		case 'S':
			if (cmd_line[1] == 'A')
				save_ifla();
			show_ifla();
			proceed (RS_FREE);

		case 'I':
			if (cmd_line[1] == 'D' || cmd_line[1] == 'M') {
				i1 = -1;
				scan (cmd_line +2, "%d", &i1);
				if (i1 > 0) {
				       if (cmd_line[1] == 'D') {
					       local_host = i1;
				       } else {
					       master_host = i1;
				       }
				}
			}
			stats(NULL);
			proceed (RS_FREE);

		case 'Y':
			i1 = -1;
			scan (cmd_line +1, "%u", &i1);
			if (i1 >= 0 && sync_freq != i1) {
				sync_freq = i1;
				if (sync_freq > 0 && master_clock.hms.f == 0) {
					master_delta = seconds();
					master_clock.hms.d = 0;
					master_clock.hms.h = 0;
					master_clock.hms.m = 0;
					master_clock.hms.s = 0;
					master_clock.hms.f = 1;
					diag (OPRE_APP_ACK "Implicit T 0:0:0");
				}
			}
			diag (OPRE_APP_ACK "Synced to %u", sync_freq);
			proceed (RS_FREE);

		default:
			form (ui_obuf, ill_str, cmd_line);
	  }

	entry (RS_UIOUT)
		ser_out (RS_UIOUT, ui_obuf);
		proceed (RS_FREE);

	entry (RS_DUMP)
		if (r_a_d ()) {
			// delay is needed in VUEE, not a bad idea in PicOS
			delay (200, RS_DUMP);
			release;
		}
		ufree (agg_dump);
		agg_dump = NULL;
		proceed (RS_FREE);

endthread

praxis_starter (NodeCus);
