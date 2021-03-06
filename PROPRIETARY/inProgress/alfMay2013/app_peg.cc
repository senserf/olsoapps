/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "app_peg.h"
#include "form.h"
#include "ser.h"
#include "net.h"
#include "tarp.h"
#include "storage.h"
#include "msg_peg.h"
#include "diag.h"
#include "oss_fmt.h"
#include "flash_stamps.h"
#include "hold.h"

#ifdef __SMURPH__
#define	SENSOR_LIST	present
#else
#include "sensors.h"
#endif

#include "app_peg_data.h"

heapmem {80, 20}; // how to find out a good ratio?

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
#define OSS_DONE	((&cmd_line)+2)

// ============================================================================

static char	*ui_ibuf	= NULL,
		*ui_obuf	= NULL,
		*cmd_line	= NULL;

static aggEEDumpType *agg_dump 	= NULL;

static int rcv_packet_size;
static char *rcv_buf_ptr;
static word rcv_rssi;

static lword lh_time;
static char *aud_buf_ptr;
static word aud_ind;

static void oss_master_in (word, nid_t);

word	host_pl		= 7;

lword	master_ts	= 0;
word	app_flags 		= DEF_APP_FLAGS;
word	tag_auditFreq 		= 59;	// in seconds
lint 	master_date		= 0;

// if we can get away with it, it's better to have it in IRAM (?)
tagDataType tagArray [tag_lim];

wroomType msg4tag 		= {NULL, 0};

aggDataType	agg_data;
msgPongAckType	pong_ack	= {{msg_pongAck}};


word	sync_freq		= 0;
word	plot_id			= 0;

#include "tarp_virt.h"

#include "oss_peg_str.h"

// =============
// OSS reporting
// =============
fsm oss_out (char*) {

	state OO_START:

		if (data == NULL) {
			app_diag (D_SERIOUS, "NULL oss_out");
			finish;
		}

	entry OO_RETRY:

		ser_outb (OO_RETRY, data);
		trigger (OSS_DONE);
		finish;
}

fsm mbeacon {

    state MB_SEND:
	oss_master_in (MB_SEND, 0);
    	delay (25 * 1024 + rnd () % 10240, MB_SEND); // 30 +/- 5s
	release;
}

static void show_ifla () {

	char * mbuf = NULL;

	if (if_read (0) == 0xFFFF) {
		diag (OPRE_APP_ACK "No custom data");
		return;
	}
	mbuf = form (NULL, ifla_str, if_read (0), if_read (1), if_read (2),
			if_read (3), if_read (4), if_read (5), if_read (6));

	if (runfsm oss_out (mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out fork");
		ufree (mbuf);
	}
}

static void read_ifla () {

	if (if_read (0) == 0xFFFF) { // usual defaults

		local_host = (word)host_id;
#ifndef __SMURPH__
		master_host = DEF_MHOST;
#endif
		if (master_host == local_host)
			plot_id = local_host;
		return;
	}

	local_host = if_read (0);
	host_pl = if_read (1);
	app_flags = if_read (2);
	tag_auditFreq = if_read (3);
	master_host = if_read (4);
	sync_freq = if_read (5);
	if (sync_freq > 0) {
		master_ts = seconds();
		master_date = -1;
		diag (impl_date_str);
	}
	if (master_host == local_host)
		plot_id = local_host;
}

static void save_ifla () {

	if (if_read (0) != 0xFFFF) {
		if_erase (0);
		diag (OPRE_APP_ACK "p0 owritten");
	}
	// there is 'show' after 'save'... don't check if_writes here (?)
	if_write (0, local_host);
	if_write (1, host_pl);
	if_write (2, (app_flags & 0xFFFE)); // off master chg
	if_write (3, tag_auditFreq);
	if_write (4, master_host);
	if_write (5, sync_freq);
}

// Display node stats on UI
static void stats (char * buf) {

	char * mbuf = NULL;
	word mmin, mem;

	if (buf == NULL) {
		mem = memfree(0, &mmin);
		mbuf = form (NULL, stats_str,
			host_id, local_host, tag_auditFreq,
			host_pl, handle_a_flags (0xFFFF), seconds(),
		       	master_ts, master_host,
			agg_data.eslot == EE_AGG_MIN &&
			  IS_AGG_EMPTY (agg_data.ee.s.f.status) ?
			0 : agg_data.eslot - EE_AGG_MIN +1,
			mem, mmin);
	} else {
	  switch (in_header (buf, msg_type)) {
	    case msg_statsPeg:
		mbuf = form (NULL, stats_str,
			in_statsPeg(buf, hostid), in_header(buf, snd),
			in_statsPeg(buf, audi), in_statsPeg(buf, pl),
			in_statsPeg(buf, a_fl),
			in_statsPeg(buf, ltime), in_statsPeg(buf, mts),
			in_statsPeg(buf, mhost), in_statsPeg(buf, slot),
			in_statsPeg(buf, mem), in_statsPeg(buf, mmin));
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

	if (runfsm oss_out (mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out fork");
		ufree (mbuf);
	}
}

static void process_incoming (word state, char * buf, word size, word rssi) {

  sint    w_len;

  if (check_msg_size (buf, size, D_SERIOUS) != 0)
	  return;

  switch (in_header(buf, msg_type)) {

	case msg_pong:
#if 0
this is dangerous is forgotten
		if (in_header(buf, snd) / 1000 != local_host / 1000)
			return;

#endif
		if (in_pong_rxon(buf)) 
			check_msg4tag (buf);

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
		if (master_host != local_host) {
			in_header(buf, rcv) = master_host;
			in_header(buf, hco) = 0; // was 1
			send_msg (buf, sizeof(msgStatsTagType));
		} else {
			//in_header(buf, snd) = local_host;
			stats (buf);
		}

		return;

	case msg_rpc:
		if (cmd_line != NULL) { // busy with another input
			when (CMD_WRITER, state);
#if 0
smells a bit...
			when (CMD_WRITER, state);
			release;
#endif
			return;
		}

		w_len = strlen (&buf[sizeof (headerType)]) +1;

		// sanitize
		if (w_len + sizeof (headerType) > size)
			return;

		cmd_line = get_mem (state, w_len);
		strcpy (cmd_line, buf + sizeof(headerType));

		trigger (CMD_READER);
		return;

	default:
		app_diag (D_SERIOUS, "Got ?(%u)", in_header(buf, msg_type));

  }
}

#include "dconv.ch"

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
	return r; // eco demo: don't overwhelm
}

/*
   --------------------
   Receiver process
   RS_ <-> Receiver State
   --------------------
*/

// In this model, a single rcv is forked once, and runs / sleeps all the time
fsm rcv {

	entry RC_INIT:

		rcv_packet_size = 0;
		rcv_buf_ptr = NULL;
		rcv_rssi = 0;

	entry RC_TRY:

		if (rcv_buf_ptr != NULL) {
			ufree (rcv_buf_ptr);
			rcv_buf_ptr = NULL;
			rcv_packet_size = 0;
		}
    		rcv_packet_size = net_rx (RC_TRY, &rcv_buf_ptr, &rcv_rssi, 0);
		if (rcv_packet_size <= 0) {
			app_diag (D_SERIOUS, "net_rx failed (%d)",
				rcv_packet_size);
			proceed RC_TRY;
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

	entry RC_MSG:
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
			map_rssi(rcv_rssi >> 8));
		proceed RC_TRY;
}

/*
  --------------------
  audit process
  AS_ <-> Audit State
  --------------------
*/
#define POW_FREQ_SHIFT 6

fsm audit {

	entry AS_INIT:

		aud_buf_ptr = NULL;

	entry AS_START:

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


	entry AS_TAGLOOP:

		if (aud_ind-- == 0) {
			app_diag (D_DEBUG, "Audit ends");
			lh_time = tag_auditFreq + seconds ();
			proceed AS_HOLD;
		}

	entry AS_TAGLOOP1:

		check_tag (AS_TAGLOOP1, aud_ind, &aud_buf_ptr);

		if (aud_buf_ptr) {
			if (local_host == master_host) {
				in_header(aud_buf_ptr, snd) = local_host;
				oss_report_out (aud_buf_ptr);
			} else
				send_msg (aud_buf_ptr,
					in_report_pload(aud_buf_ptr) ?
				sizeof(msgReportType) + sizeof(reportPloadType)
				: sizeof(msgReportType));

			ufree (aud_buf_ptr);
			aud_buf_ptr = NULL;
		}
		proceed AS_TAGLOOP;

	entry AS_HOLD:

		hold (AS_HOLD, lh_time);
		proceed AS_START;
}

#undef POW_FREQ_SHIFT

idiosyncratic void tmpcrap (word what) {

	switch (what) {

		case 0:
			if (tag_auditFreq != 0 && !running (audit))
				runfsm audit;
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
fsm cmd_in {

	entry CS_INIT:

		if (ui_ibuf == NULL)
			ui_ibuf = get_mem (CS_INIT, UI_BUFLEN);

	entry CS_IN:

		// hangs on the uart_a interrupt or polling
		ser_in (CS_IN, ui_ibuf, UI_BUFLEN);
		if (strlen(ui_ibuf) == 0)
			// CR on empty line would do it
			proceed CS_IN;

	entry CS_WAIT:

		if (cmd_line != NULL) {
			when (CMD_WRITER, CS_WAIT);
			release;
		}

		cmd_line = get_mem (CS_WAIT, strlen(ui_ibuf) +1);
		strcpy (cmd_line, ui_ibuf);
		trigger (CMD_READER);
		proceed CS_IN;
}

void oss_report_out (char * buf) {

  char * lbuf = NULL;
  mdate_t md, md2;

  if (in_report_pload(buf)) {

	md.secs = in_reportPload(buf, ds);
	s2d (&md);
	md2.secs = in_reportPload(buf, ppload.ds);
	s2d (&md2);

	// Another kludge, possibly affecting general sensor data:
	// how to format variable sensor strings, include additional data, etc?
	// Here we load plev and rssi as CHRONOS sensors [6], [7].
	// SENSET_CHRO == 4; don't include  senset.h/cc here:
	if (in_report(buf, setsen) == 4 ||
			in_report(buf, setsen) == 3) {
		in_reportPload(buf, ppload.sval[6]) = in_report(buf, pl);
		in_reportPload(buf, ppload.sval[7]) = in_report(buf, rssi);
	}

	lbuf = form (NULL, rep_str, in_reportPload(buf, ppload.ds),

		in_header(buf, snd), local_host,

		in_report(buf, tagid),

		in_report(buf, state) == goneTag ?
			" ***gone***" : " ",
#if 0
		in_reportPload(buf, ppload.sval[0]),
#endif
		in_reportPload(buf, ppload.sval[1]),
#if 0
		in_reportPload(buf, ppload.sval[2]),
#endif
		in_reportPload(buf, ppload.sval[3]),
		in_reportPload(buf, ppload.sval[4]),
		in_reportPload(buf, ppload.sval[5]),
		in_reportPload(buf, ppload.sval[6]),
		in_reportPload(buf, ppload.sval[7]),
		in_report(buf, setsen));

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

    if (runfsm oss_out (lbuf) == 0 ) {
	app_diag (D_SERIOUS, "oss_out failed");
	ufree (lbuf);
    }
}

static const char * markName (statu_t s) {
        switch (s.f.mark) {
		case 0:
		case MARK_EMPTY:   return "NONE";
                case MARK_BOOT: return "BOOT";
                case MARK_PLOT: return "PLOT";
                case MARK_SYNC: return "SYNC";
                case MARK_MCHG: return "MCHG";
                case MARK_DATE: return "DATE";
        }
        app_diag (D_SERIOUS, "? eeprom %x", s);
        return "????";
}       

static word r_a_d () {

	char * lbuf = NULL;
	mdate_t md, md2;

	if (agg_dump->dfin) // delayed Finish
		goto ThatsIt;

	if (ee_read (agg_dump->ind * EE_AGG_SIZE, (byte *)&agg_dump->ee,
				EE_AGG_SIZE)) {
		app_diag (D_SERIOUS, "Failed ee_read");
		goto Finish;
	}

	if (IS_AGG_EMPTY (agg_dump->ee.s.f.status)) {
		if (agg_dump->fr <= agg_dump->to) {
			goto Finish;
		} else {
			goto Continue;
		}
	}

	if (agg_dump->tag == 0 || agg_dump->ee.tag == agg_dump->tag ||
			agg_dump->ee.s.f.status == AGG_ALL) {

	    if (agg_dump->ee.s.f.status == AGG_ALL) { // mark

		md.secs = agg_dump->ee.ds;
		s2d (&md);

		lbuf = form (NULL, dumpmark_str, markName (agg_dump->ee.s),
			agg_dump->ind,

			md.dat.f ?  2009 + md.dat.yy : 1001 + md.dat.yy,
			md.dat.mm, md.dat.dd, md.dat.h, md.dat.m, md.dat.s,

			agg_dump->ee.sval[0],
			agg_dump->ee.sval[1],
			agg_dump->ee.sval[2]);

	    } else { // sens reading

		md.secs = agg_dump->ee.t_ds;
		s2d (&md);
		md2.secs = agg_dump->ee.ds;
		s2d (&md2);

		lbuf = form (NULL, dump_str,

			agg_dump->ee.tag, agg_dump->ee.t_eslot, agg_dump->ind,

			md.dat.f ?  2009 + md.dat.yy : 1001 + md.dat.yy,
			md.dat.mm, md.dat.dd, md.dat.h, md.dat.m, md.dat.s,

			md2.dat.f ?  2009 + md2.dat.yy : 1001 + md2.dat.yy,
			md2.dat.mm, md2.dat.dd, md2.dat.h, md2.dat.m, md2.dat.s,

			agg_dump->ee.sval[0],
                        agg_dump->ee.sval[1],
                        agg_dump->ee.sval[2],
                        agg_dump->ee.sval[3],
                        agg_dump->ee.sval[4], 
			agg_dump->ee.sval[5],
                        agg_dump->ee.sval[6],
                        agg_dump->ee.sval[7],
			agg_dump->ee.s.f.setsen);
	    }

	    if (runfsm oss_out (lbuf) == 0 ) {
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

	if (runfsm oss_out (lbuf) == 0 ) {
		app_diag (D_SERIOUS, "oss_out sum failed");
		ufree (lbuf);
	}

	return 0;
}

static void oss_findTag_in (word state, nid_t tag, nid_t peg) {

	char * out_buf = NULL;
	sint tagIndex;

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

static void oss_master_in (word state, nid_t peg) {

	char * out_buf = NULL;

	if (local_host != master_host && (local_host == peg || peg == 0)) {
		if (!running (mbeacon))
			runfsm mbeacon;
		master_host = local_host;
		master_ts  = 0;
		master_date = 0;
		tarp_ctrl.param &= 0xFE;
		if (local_host == peg)
			return;
	}
	msg_master_out (state, &out_buf, peg);
	send_msg (out_buf, sizeof(msgMasterType));
	ufree (out_buf);
}

static void oss_setTag_in (word state, word tag,
	       	nid_t peg, word maj, word min, 
		word span, word pl, word c_fl) {

	char * out_buf = NULL;
	char * set_buf = NULL;
	sint size = sizeof(msgFwdType) + sizeof(msgSetTagType);

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

static void oss_setPeg_in (word state, nid_t peg, 
				word audi, word pl, word a_fl) {

	char * out_buf = get_mem (state, sizeof(msgSetPegType));
	memset (out_buf, 0, sizeof(msgSetPegType));

	in_header(out_buf, msg_type) = msg_setPeg; // hco == 0
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

fsm root {

	entry RS_INIT:

		ui_obuf = get_mem (RS_INIT, UI_BUFLEN);
#ifdef BOARD_WARSAW_BLUE
		// Use UART 2 via Bluetooth
		ser_select (1);
#endif

	entry RS_INIEE:

		if (ee_open ()) {
			leds (LED_B, LED_ON);
			leds (LED_R, LED_ON);

#if STORAGE_SDCARD
// loop: likely SD is missing
			delay (3000, RS_INIEE);
			release;
#else
			//fatal_err (ERR_EER, 0, 1, 1);
			app_diag (D_FATAL," ee_open failed");
			halt();
#endif
		}

		leds (LED_B, LED_OFF);
		leds (LED_R, LED_OFF);
		form (ui_obuf, ee_str, EE_AGG_MIN, EE_AGG_MAX -1, EE_AGG_SIZE);

		if (if_read (IFLASH_SIZE -1) != 0xFFFF) {

			if (if_read (IFLASH_SIZE -1) == ERR_EER) {
				if_erase (IFLASH_SIZE -1);
				break_flash;
				diag (OPRE_APP_ACK "p1 erased");
				reset();
			}

			leds (LED_R, LED_BLINK);
		} else
			leds (LED_G, LED_BLINK);

		// is_flash_new is set const (a branch compiled out)
		if (is_flash_new) {
			diag (OPRE_APP_ACK "Init ee erase");
			ee_erase (WNONE, 0, 0);
			break_flash;
			read_ifla();
		} else {
			read_ifla();
			delay (5000, RS_INIT1);
			release;
		}

	entry RS_INIT1:

		ser_out (RS_INIT1, ui_obuf);
		agg_init();

		if (if_read (IFLASH_SIZE -1) != 0xFFFF) {
			leds (LED_R, LED_OFF);
			diag (OPRE_APP_MENU_A "*Maint mode*"
				OMID_CRB "%x %u %u %u",
				if_read (IFLASH_SIZE -1),
				if_read (IFLASH_SIZE -2),
				if_read (IFLASH_SIZE -3),
				if_read (IFLASH_SIZE -4));
			if (!running (cmd_in))
				runfsm cmd_in;
			stats (NULL);
			proceed RS_RCMD;
		}
		leds (LED_G, LED_OFF);

	entry RS_INIT2:

		ser_out (RS_INIT2, welcome_str);

#ifndef __SMURPH__
		net_id = DEF_NID;
#endif
		tarp_ctrl.param = 0xB1; // level 2, rec 3, slack 0, fwd on

		init_tags();

		// spread a bit in case of a sync reset
		delay (rnd() % 1000, RS_PAUSE);
		release;

	entry RS_PAUSE:

		if (net_init (INFO_PHYS_DEV, INFO_PLUG_TARP) < 0) {
			app_diag (D_FATAL, "net_init failed");
			reset();
		}
		net_opt (PHYSOPT_SETSID, &net_id);
		net_opt (PHYSOPT_SETPOWER, &host_pl);
		runfsm rcv;
		runfsm cmd_in;
		runfsm audit;
		if (master_host == local_host) {
			runfsm mbeacon;
			tarp_ctrl.param &= 0xFE;
		}
		write_mark (MARK_BOOT);

		proceed RS_RCMD;

	entry RS_FREE:

		ufree (cmd_line);
		cmd_line = NULL;
		trigger (CMD_WRITER);

	entry RS_RCMD:

		if (cmd_line == NULL) {
			when (CMD_READER, RS_RCMD);
			release;
		}

	entry RS_DOCMD:

		mdate_t md;
		sint	i1, i2, i3, i4, i5, i6, i7;

		if (master_host != local_host &&
				(cmd_line[0] == 'T' || cmd_line[0] == 'c' ||
				 cmd_line[0] == 'f')) {
			strcpy (ui_obuf, only_master_str);
			proceed RS_UIOUT;
		}

		if (if_read (IFLASH_SIZE -1) != 0xFFFF &&
				(cmd_line[0] == 'm' || cmd_line[0] == 'c' ||
				cmd_line[0] == 'f')) {
			strcpy (ui_obuf, not_in_maint_str);
			proceed RS_UIOUT;
		}

	  switch (cmd_line[0]) {

		case ' ': proceed RS_FREE; // ignore if starts with blank

                case 'h':
			ser_out (RS_DOCMD, welcome_str);
			proceed RS_FREE;

		case 'T':

			i1 = i2 = i3 = i4 = i5 = i6 = 0;

			if ((i7 = scan (cmd_line+1, "%u-%u-%u %u:%u:%u", 
					&i1, &i2, &i3, &i4, &i5, &i6)) != 6 &&
					i7 != 0) {
				form (ui_obuf, bad_str, cmd_line, i7);
				proceed RS_UIOUT;
			}

			if (i7 == 6) {
				if (i1 < 2009  || i1 > 2039 ||
						i2 < 1 || i2 > 12 ||
						i3 < 1 || i3 > 31 ||
						i4 > 23 ||
						i5 > 59 ||
						i6 > 59) {
					form (ui_obuf, bad_str, cmd_line, -1);
					proceed RS_UIOUT;
				}
				md.dat.f = 1;
				md.dat.yy = i1 - 2008;
				md.dat.mm = i2;
				md.dat.dd = i3;
				md.dat.h = i4;
				md.dat.m = i5;
				md.dat.s = i6;
				d2s (&md);
				master_date = md.secs;
				master_ts = seconds();
				write_mark (MARK_DATE);
			}

			md.secs = wall_date (0);
			s2d (&md);
			form (ui_obuf, clock_str,
				md.dat.f ? 2009 + md.dat.yy : 1001 + md.dat.yy,
				md.dat.mm, md.dat.dd,
				md.dat.h, md.dat.m, md.dat.s, seconds());
			proceed RS_UIOUT;

		case 'P':
			i1 = -1;
			scan (cmd_line+1, "%d", &i1);
			if (i1 > 0 && plot_id != i1) {
				if (local_host != master_host) {
					strcpy (ui_obuf, only_master_str);
					proceed RS_UIOUT;
				}
				plot_id = i1;
				write_mark (MARK_PLOT);
			}
			form (ui_obuf, plot_str, plot_id);
			proceed RS_UIOUT;

		case 'q': reset();

		case 'D':
			agg_dump = (aggEEDumpType *)
				get_mem (WNONE, sizeof(aggEEDumpType));
			
			if (agg_dump == NULL )
				proceed RS_FREE;

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
			proceed RS_DUMP;

		case 'M':
			if (if_read (IFLASH_SIZE -1) != 0xFFFF) {
				diag (OPRE_APP_ACK "Already in maint");
				reset();
			}
			fatal_err (ERR_MAINT, (word)(seconds() >> 16),
					(word)(seconds()), 0);
			// will reset

		case 'E':
			diag (OPRE_APP_ACK "erasing ee...");
			ee_erase (WNONE, 0, 0);
			diag (OPRE_APP_ACK "ee erased");
			reset();

		case 'F':
			if_erase (IFLASH_SIZE -1);
			break_flash;
			diag (OPRE_APP_ACK "p2 erased");
			reset();

		case 'Q':
			diag (OPRE_APP_ACK "erasing all...");
			ee_erase (WNONE, 0, 0);
			if_erase (-1);
			break_flash;
			diag (OPRE_APP_ACK "all erased");
			reset();

		case 'm':
			 i1 = 0;
			 scan (cmd_line+1, "%u", &i1);
			 
			 if (i1 != local_host && (local_host == master_host ||
					 i1 != 0))
				 diag (OPRE_APP_ACK "Sent Master beacon");
			 else if (local_host != master_host)
				 diag (OPRE_APP_ACK "Became Master");

			 oss_master_in (RS_DOCMD, i1);
			 proceed RS_FREE;

		case 'f':
			i1 = i2 = 0;
			scan (cmd_line+1, "%u %u", &i1, &i2);
			oss_findTag_in (RS_DOCMD, i1, i2);
			proceed RS_FREE;

		case 'c':
			i1 = i2 = i3 = i4 = i5 = i6 = i7 = -1;
			
			// tag peg fM fm span pl
			scan (cmd_line+1, "%d %d %d %d %d %x %x",
				&i1, &i2, &i3, &i4, &i5, &i6, &i7);
			
			if (i1 <= 0 || i3 < -1 || i4 < -1 || i5 < -1) {
				form (ui_obuf, bad_str, cmd_line, 0);
				proceed RS_UIOUT;
			}

			if (i2 <= 0)
				i2 = local_host;

			oss_setTag_in (RS_DOCMD, i1, i2, i3, i4, i5, i6, i7);
			proceed RS_FREE;
		
		case 'a':
			i1 = i2 = i3 = i4 = -1;
			if (scan (cmd_line+1, "%d %d %d %x", &i1, &i2, &i3, &i4)
					== 0 || i1 < 0)
				i1 = local_host;

			if (if_read (IFLASH_SIZE -1) != 0xFFFF &&
				i1 !=local_host) {
				strcpy (ui_obuf, not_in_maint_str);
				proceed RS_UIOUT;
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
						runfsm audit;
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

			proceed RS_FREE;

		case 'S':
			if (cmd_line[1] == 'A')
				save_ifla();
			show_ifla();
			proceed RS_FREE;

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
			proceed RS_FREE;

		case 'Y':
			i1 = -1;
			scan (cmd_line +1, "%u", &i1);
			if ((i1 == 0 || (i1 > 59 && SID % i1 == 0)) 
					&& sync_freq != i1) {
				sync_freq = i1;
				if (sync_freq > 0 && master_date >= 0) {
					master_ts = seconds();
					master_date = -1;
					diag (impl_date_str); 
				}
				write_mark (MARK_SYNC);
			}
			form (ui_obuf, sync_str, sync_freq);
			proceed RS_UIOUT;

		default:
			form (ui_obuf, ill_str, cmd_line);
	  }

	entry RS_UIOUT:

		ser_out (RS_UIOUT, ui_obuf);
		proceed RS_FREE;

	entry RS_DUMP:
		if (running (oss_out)) {
			delay (50, RS_DUMP);
			when (OSS_DONE, RS_DUMP);
			release;
		}

		if (r_a_d ())
			proceed RS_DUMP;

		ufree (agg_dump);
		agg_dump = NULL;
		proceed RS_FREE;
}
