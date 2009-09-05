/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "globals_peg.h"
#include "threadhdrs_peg.h"
#include "flash_stamps.h"
#include "sat_peg.h"

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
strand (oss_out, char)
	char * b;
	sint len;

	// OO_START is the sat patch
	entry (OO_START)
		if (data == NULL) {
			app_diag (D_SERIOUS, "NULL oss_out");
			finish;
		}
		if (IS_SATGAT) {
			len = SATWRAPLEN + 2 + strlen (data);
			b = get_mem (WNONE, len);
			if (b == NULL)
				finish;
			strcpy (b, SATWRAP);
			strcpy (&b[SATWRAPLEN], data);
			b[len -2] = (char)26;
			b[len -1] = '\0';
			// PG: the code will do
			data [4] = '\0';
			sat_rep (data);
			// End PG
			ufree (data);
			data = b;
			savedata (data);
		}

	entry (OO_RETRY)
		ser_outb (OO_RETRY, data);
		finish;
endstrand

strand (satcmd_out, char)
	entry (SCO_TRY)
		ser_out (SCO_TRY, data);
		sat_rep (data);
		finish;
endstrand

thread (mbeacon)

    entry (MB_START)

	// periodically get GPS time, or if not set or sat. i/f uninited
	if (IS_SATGAT && (seconds() - master_ts  > SAT_TIMEADJ || 
			master_date > -SID * 90 || sat_mod == SATMOD_UNINIT))
		(void)runstrand (satcmd_out, SATCMD_STATUS);

	if (IS_SATGAT && sat_mod == SATMOD_UNINIT) // just in case
		(void)runstrand (satcmd_out, SATCMD_PDC);

	delay (25 * 1024 + rnd() % 10240, MB_SEND); // 30 +/- 5s
	release;

    entry (MB_SEND)
	oss_master_in (MB_SEND, 0);
	proceed (MB_START);

endthread

__PUBLF (NodePeg, void, sat_rep) (char * b) {
	sint len;
	char * obuf;

	// oss_out is called also when the net is not inited yet, and
	// it calls in here; ignore - custodian will not hear it
	if (!is_satest || net_opt (PHYSOPT_PHYSINFO, NULL) == -1)
		return;

	if (b == NULL) // came from sat into ui_ibuf
		b = ui_ibuf;

	if ((len = strlen (b)) > MAX_SATLEN) {
		len = MAX_SATLEN;
		b[MAX_SATLEN] = '\0';
	}
	len += sizeof (headerType) +1; // + '\0'

	if ((obuf  = get_mem (WNONE, len)) == NULL)
		return;

	strcpy (&obuf[sizeof(headerType)], b);
	in_header(obuf, msg_type) = msg_satest;
	in_header(obuf, rcv) = 0; // must be bcast
	in_header(obuf, hco) = 1; // no hopping
	send_msg (obuf, len);
	ufree (obuf);
}

__PUBLF (NodePeg, void, sat_out) (char * buf) {
	sint len = strlen (&buf[sizeof (headerType)]) +1; // + '\0'
	char * mbuf;

	if ((mbuf = get_mem (WNONE, len)) == NULL)
		return;

	strcpy (mbuf, &buf[sizeof (headerType)]);

	// note that there could be double-wrapping in sat. crap here
	if (runstrand (oss_out, mbuf) == 0)
		ufree (mbuf);
}

__PUBLF (NodePeg, void, show_ifla) () {
	char * mbuf = NULL;

	if (if_read (0) == 0xFFFF) {
		diag (OPRE_APP_ACK "No custom data");
		return;
	}
	mbuf = form (NULL, ifla_str, if_read (0), if_read (1), if_read (2),
			if_read (3), if_read (4), if_read (5), if_read (6));

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
		sat_mod = IS_DEFSATGAT ? SATMOD_UNINIT : SATMOD_NO;
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
	sat_mod = if_read (6);
	if (master_host == local_host)
		plot_id = local_host;
}


__PUBLF (NodePeg, void, save_ifla) () {
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
	if_write (6, IS_SATGAT ? SATMOD_UNINIT : SATMOD_NO);
}

// Display node stats on UI
__PUBLF (NodePeg, void, stats) (char * buf) {
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
			mem, mmin, sat_mod, pow_sup);
	} else {
	  switch (in_header (buf, msg_type)) {
	    case msg_statsPeg:
		mbuf = form (NULL, stats_str,
			in_statsPeg(buf, hostid), in_header(buf, snd),
			in_statsPeg(buf, audi), in_statsPeg(buf, pl),
			in_statsPeg(buf, a_fl),
			in_statsPeg(buf, ltime), in_statsPeg(buf, mts),
			in_statsPeg(buf, mhost), in_statsPeg(buf, slot),
			in_statsPeg(buf, mem), in_statsPeg(buf, mmin), 0, 0);
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

__PUBLF (NodePeg, void, process_incoming) (word state, char * buf, word size,
								word rssi) {
  sint    w_len;

  if (check_msg_size (buf, size, D_SERIOUS) != 0)
	  return;

  switch (in_header(buf, msg_type)) {

	case msg_pong:

		if (in_header(buf, snd) / 1000 != local_host / 1000)
			return;

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

		// don't compete with sat. multilines
		if (sat_mod == SATMOD_WAITRX)
			return;

		w_len = strlen (&buf[sizeof (headerType)]) +1;

		// sanitize
		if (w_len + sizeof (headerType) > size)
			return;

		cmd_line = get_mem (state, w_len);
		strcpy (cmd_line, buf + sizeof(headerType));

		trigger (CMD_READER);
		return;

	case msg_satest:
#if 0
FIXME: if that works, clean off sat_out()
		sat_out (buf);
#endif
		ser_out (state, &buf[sizeof(headerType)]);
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
#define POW_FREQ_SHIFT 6
thread (audit)

	nodata;

	entry (AS_INIT)

		aud_buf_ptr = NULL;

	entry (AS_START)
		if (local_host == master_host && (pow_ts == 0 ||
	(word)(seconds() - pow_ts) >= (tag_auditFreq << POW_FREQ_SHIFT))) {
			read_sensor (AS_START, 0, &pow_sup);
			pow_ts = seconds();
			stats (NULL);
		}

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

		if (IS_SATGAT)
			(void)runstrand (satcmd_out, SATCMD_QUEUE);

	entry (AS_TAGLOOP)

		if (aud_ind-- == 0) {
			app_diag (D_DEBUG, "Audit ends");
			lh_time = tag_auditFreq;
			proceed (AS_HOLD);
		}

	entry (AS_TAGLOOP1)

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
		proceed (AS_TAGLOOP);

	entry (AS_HOLD)

		lhold (AS_HOLD, &lh_time);
		proceed (AS_START);
endthread
#undef POW_FREQ_SHIFT

__PUBLF (NodePeg, void, tmpcrap) (word what) {
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

	mdate_t md;
	word u[6];

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

		if (!IS_SATGAT || sat_mod == SATMOD_WAITRX) {
			cmd_line = get_mem (CS_WAIT, strlen(ui_ibuf) +1);
			strcpy (cmd_line, ui_ibuf);
			if (sat_mod == SATMOD_WAITRX)
				sat_mod = SATMOD_YES;
			trigger (CMD_READER);

			if (IS_SATGAT)
				goto SatRep;
			else
				proceed (CS_IN);
		}
		// there may be more SATWAITs

		if (str_cmpn (ui_ibuf, SATATT_MSG, SAT_MSGLEN) == 0) {
			sat_mod = SATMOD_WAITRX;
			goto SatRep;
		}

		if (str_cmpn (ui_ibuf, SATIN_RNG, SAT_RNGLEN) == 0) {
			if (sat_mod == SATMOD_UNINIT)
				sat_mod = SATMOD_YES;
			(void)runstrand (satcmd_out, SATCMD_GET);
			goto SatRep;
		}

		if (str_cmpn (ui_ibuf, SATHEX_MSG, SAT_MSGLEN) == 0) {
			(void)runstrand (satcmd_out, SATCMD_H2T);
			goto SatRep;
		}

		if (str_cmpn (ui_ibuf, SATIN_SBBB, SAT_SSETLEN) == 0) {
			set_satest;
			goto SatRep;
		}

		if (str_cmpn (ui_ibuf, SATIN_SPPP, SAT_SSETLEN) == 0) {
			clr_satest;
			goto SatRep;
		}

		if (str_cmpn (ui_ibuf, SATIN_SMMM, SAT_SSETLEN) == 0) {
			clr_satest;
			sat_mod = SATMOD_NO;
			goto SatRep;
		}

		u[0] = strlen (ui_ibuf);

		if (str_cmpn (ui_ibuf, SATIN_QUE, SAT_QUELEN) == 0) {
			if (u[0] < SAT_QUEMINLEN || scan (ui_ibuf +SAT_QUEOSET,
					SAT_QUEFORM, &u[0], &u[1]) != 2) {

				ui_ibuf[0] = '!';

			} else {

				if (u[0] < SAT_QUEFULL) {
					if (sat_mod != SATMOD_YES) {
						ui_ibuf[1] = '+';
						sat_mod = SATMOD_YES;
					}
				} else {
#if 0
					ui_ibuf[0] = 'F';
					if (sat_mod != SATMOD_FULL) {
						ui_ibuf[1] = '+';
						sat_mod = SATMOD_FULL;
					}
#endif
					// let's try this:
					runstrand (satcmd_out, SATCMD_BOOT);
					sat_mod = SATMOD_UNINIT;
					strcpy (ui_ibuf, "PDT Reboot");
				}
			}

			goto SatRep;
		}

		if (str_cmpn (ui_ibuf, SATIN_STA, SAT_STALEN) == 0) {

			if (sat_mod == SATMOD_UNINIT)
				sat_mod = SATMOD_YES;

			if (u[0] < SAT_STAMINLEN || scan (ui_ibuf +SAT_STAOSET,
					SAT_STAFORM, &u[0], &u[1], &u[2],
						&u[3], &u[4], &u[5]) != 6 ||
					u[2] < 2009) { // not from GPS
				ui_ibuf[0] = '?';
				goto SatRep;
			}

			if (master_date > -SID * 90)
				ui_ibuf[0] = 's';

			md.dat.f = 1;
			md.dat.yy = u[2] - 2008;
			md.dat.mm = u[0];
			md.dat.dd = u[1];
			md.dat.h = u[3];
			md.dat.m = u[4];
			md.dat.s = u[5];
			d2s (&md);

			if (ui_ibuf[0] != 's') {
			       if (md.secs !=  wall_date (0)) {
				       ui_ibuf[0] = 'U';
			       } else {
				       ui_ibuf[0] = 'u';
			       }
			}
			master_date = md.secs;
			master_ts = seconds();
			goto SatRep;

#if 0

			// no time yet
			if (master_date > -SID * 90) {
				master_ts = seconds();
				md.dat.f = 1;
				md.dat.yy = u[2] - 2008;
				md.dat.mm = u[0];
				md.dat.dd = u[1];
				md.dat.h = u[3];
				md.dat.m = u[4];
				md.dat.s = u[5];
				d2s (&md);
				master_date = md.secs;
				ui_ibuf[0] = 's';
				goto SatRep;
			}

			// time to adjust time?
			if (seconds() - master_ts  > SAT_TIMEADJ) {
                                md.dat.f = 1;
                                md.dat.yy = u[2] - 2008;
                                md.dat.mm = u[0];
                                md.dat.dd = u[1];
                                md.dat.h = u[3];
                                md.dat.m = u[4];
                                md.dat.s = u[5];
                                d2s (&md);
				if (md.secs !=  master_date) {
					master_date = md.secs;
					master_ts = seconds();
					ui_ibuf[0] = 'u';
				}
			}
#endif

		}

		// report (via RF)  everything (to a custodian)
SatRep:		sat_rep (NULL);
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

__PUBLF (NodePeg, void, oss_report_out) (char * buf) {

  char * lbuf = NULL;
  mdate_t md, md2;

  if (in_report_pload(buf)) {

	  md.secs = in_reportPload(buf, ds);
	  s2d (&md);
	  md2.secs = in_reportPload(buf, ppload.ds);
	  s2d (&md2);

      if (IS_SATGAT) {
        lbuf = form (NULL, satrep_str,
		in_reportPload(buf, eslot), 
		in_report(buf, tagid), in_reportPload(buf, ppload.eslot),
		md2.dat.f ?  2009 + md2.dat.yy : 2001 + md2.dat.yy,
		md2.dat.mm, md2.dat.dd, md2.dat.h, md2.dat.m, md2.dat.s,

		in_report(buf, state) == goneTag ?
			" ***gone***" : " ",
		in_reportPload(buf, ppload.sval[0]),
		in_reportPload(buf, ppload.sval[1]),
		in_reportPload(buf, ppload.sval[2]),
		in_reportPload(buf, ppload.sval[3]),
		in_reportPload(buf, ppload.sval[4]),
		in_reportPload(buf, ppload.sval[5]));
      } else {
	lbuf = form (NULL, rep_str,

		in_header(buf, snd),
		in_reportPload(buf, eslot),

		md.dat.f ?  2009 + md.dat.yy : 1001 + md.dat.yy,
		md.dat.mm, md.dat.dd, md.dat.h, md.dat.m, md.dat.s,

		in_report(buf, tagid),
		in_reportPload(buf, ppload.eslot),

		md2.dat.f ?  2009 + md2.dat.yy : 1001 + md2.dat.yy,
		md2.dat.mm, md2.dat.dd, md2.dat.h, md2.dat.m, md2.dat.s,

		in_report(buf, state) == goneTag ?
			" ***gone***" : " ",

		in_reportPload(buf, ppload.sval[0]),
		in_reportPload(buf, ppload.sval[1]),
		in_reportPload(buf, ppload.sval[2]),
		in_reportPload(buf, ppload.sval[3]),
		in_reportPload(buf, ppload.sval[4]),
		in_reportPload(buf, ppload.sval[5])); // eoform
      }

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

__PUBLF (NodePeg, word, r_a_d) () {
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
			agg_dump->ee.sval[4], agg_dump->ee.sval[5]);
	    }

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

__PUBLF (NodePeg, void, oss_findTag_in) (word state, nid_t tag, nid_t peg) {

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

__PUBLF (NodePeg, void, oss_master_in) (word state, nid_t peg) {

	char * out_buf = NULL;

	if (local_host != master_host && (local_host == peg || peg == 0)) {
		if (!running (mbeacon))
			runthread (mbeacon);
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

__PUBLF (NodePeg, void, oss_setTag_in) (word state, word tag,
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

__PUBLF (NodePeg, void, oss_setPeg_in) (word state, nid_t peg, 
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

	mdate_t md;
	char * tmpb; // FIXME this crap should be clean up soon

	sint	i1, i2, i3, i4, i5, i6, i7;

	entry (RS_INIT)
		ui_obuf = get_mem (RS_INIT, UI_BUFLEN);

	entry (RS_INIEE)
		if (ee_open ()) {
			leds (LED_B, LED_ON);
			leds (LED_R, LED_ON);
#if 0
don't even try it: I don't know what I am, as it is in NVM, 
plus we dont want to flood the sat link
			if (!IS_SATGAT) { 
				ser_out (RS_INIEE, noee_str);
			} else {
				tmpb = form (NULL, satframe_str, noee_str);
				if (runstrand (oss_out, tmpb) == 0) {
					ufree (tmpb);
				}
			}
#endif

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

	entry (RS_INIT1)

#ifdef BCAST_SATEST_MSG
#if BCAST_SATEST_MSG
// FIXME clean it into a consistent oss / sat i/f
		set_satest;
#endif
#endif

		if (!IS_SATGAT) {
			ser_out (RS_INIT1, ui_obuf);
		} else { 
			tmpb = form (NULL, satframe_str, ui_obuf);
			if (runstrand (oss_out, tmpb) == 0) {
				ufree (tmpb);
			}
		}
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
				runthread (cmd_in);
			stats(NULL);
			proceed (RS_RCMD);
		}
		leds (LED_G, LED_OFF);

	entry (RS_INIT2)
		if (!IS_SATGAT) {
			ser_out (RS_INIT2, welcome_str);
		} else {
			tmpb = form (NULL, satframe_str, satstart_str);
			if (runstrand (oss_out, tmpb) == 0) {
				ufree (tmpb);
			}
		}

#ifndef __SMURPH__
		net_id = DEF_NID;
#endif
		tarp_ctrl.param = 0xB1; // level 2, rec 3, slack 0, fwd on

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
		if (master_host == local_host) {
			runthread (mbeacon);
			tarp_ctrl.param &= 0xFE;
		}
		write_mark (MARK_BOOT);

		// PG: just the OPRE code (all I need)
		if (IS_SATGAT) {
			ui_obuf [4] = '\0';
			sat_rep (OPRE_APP_MENU_A);
			// We do it three times for reliability; it would be
			// stupid to reset the node multiple times if packets
			// are lost.
			sat_rep (OPRE_APP_MENU_A);
			sat_rep (OPRE_APP_MENU_A);
		}
		// End PG

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

		if (master_host != local_host &&
				(cmd_line[0] == 'T' || cmd_line[0] == 'c' ||
				 cmd_line[0] == 'f')) {
			strcpy (ui_obuf, only_master_str);
			proceed (RS_UIOUT);
		}

		if (if_read (IFLASH_SIZE -1) != 0xFFFF &&
				(cmd_line[0] == 'm' || cmd_line[0] == 'c' ||
				cmd_line[0] == 'f')) {
			strcpy (ui_obuf, not_in_maint_str);
			proceed (RS_UIOUT);
		}

	  switch (cmd_line[0]) {

		case ' ': proceed (RS_FREE); // ignore if starts with blank

                case 'h':
			if (!IS_SATGAT) {
				ser_out (RS_DOCMD, welcome_str);
			} else {
				tmpb = form (NULL, satframe_str, satstart_str);
				if (runstrand (oss_out, tmpb) == 0) {
					ufree (tmpb);
				}
			}
			proceed (RS_FREE);

		case 'T':
			i1 = i2 = i3 = i4 = i5 = i6 = 0;

			if ((i7 = scan (cmd_line+1, "%u-%u-%u %u:%u:%u", 
					&i1, &i2, &i3, &i4, &i5, &i6)) != 6 &&
					i7 != 0) {
				form (ui_obuf, bad_str, cmd_line, i7);
				proceed (RS_UIOUT);
			}

			if (i7 == 6) {
				if (i1 < 2009  || i1 > 2039 ||
						i2 < 1 || i2 > 12 ||
						i3 < 1 || i3 > 31 ||
						i4 > 23 ||
						i5 > 59 ||
						i6 > 59) {
					form (ui_obuf, bad_str, cmd_line, -1);
					proceed (RS_UIOUT);
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
			proceed (RS_UIOUT);

		case 'P':
			i1 = -1;
			scan (cmd_line+1, "%d", &i1);
			if (i1 > 0 && plot_id != i1) {
				if (local_host != master_host) {
					strcpy (ui_obuf, only_master_str);
					proceed (RS_UIOUT);
				}
				plot_id = i1;
				write_mark (MARK_PLOT);
			}
			form (ui_obuf, plot_str, plot_id);
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

		case 's':
			switch (cmd_line[1]) {
				case '+':
					if (sat_mod == SATMOD_NO)
						sat_mod = SATMOD_UNINIT;
					clr_satest;
					break;
				case '-':
					sat_mod = SATMOD_NO;
					clr_satest;
					break;
				case '!':
					if (sat_mod == SATMOD_NO)
						sat_mod = SATMOD_UNINIT;
					set_satest;
					break;
			}

			stats (NULL);
			proceed (RS_FREE);

		case 'm':
			 i1 = 0;
			 scan (cmd_line+1, "%u", &i1);
			 
			 if (i1 != local_host && (local_host == master_host ||
					 i1 != 0))
				 diag (OPRE_APP_ACK "Sent Master beacon");
			 else if (local_host != master_host)
				 diag (OPRE_APP_ACK "Became Master");

			 oss_master_in (RS_DOCMD, i1);
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
				form (ui_obuf, bad_str, cmd_line, 0);
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
			proceed (RS_UIOUT);

		default:
			form (ui_obuf, ill_str, cmd_line);
	  }

	entry (RS_UIOUT)
               if (!IS_SATGAT) { 
                        ser_out (RS_UIOUT, ui_obuf);
                } else { 
                        tmpb = form (NULL, satframe_str, ui_obuf);
                        if (runstrand (oss_out, tmpb) == 0) {
                                ufree (tmpb);
                        }
                }
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

praxis_starter (NodePeg);
