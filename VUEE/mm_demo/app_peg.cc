/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "globals_peg.h"
#include "threadhdrs_peg.h"

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

// Semaphores
#define CMD_READER	(&cmd_line)
#define CMD_WRITER	((&cmd_line)+1)

// =============
// OSS reporting
// =============
strand (oss_out, char)
        entry (OO_RETRY)
		if (data == NULL)
			app_diag (D_SERIOUS, "NULL in oss_out");
		else
			ser_outb (OO_RETRY, data);
                finish;
endstrand

thread (mbeacon)

    entry (MB_START)
	delay (2048 + rnd() % 2048, MB_SEND); // 2 - 4s
	release;

    entry (MB_SEND)
	msg_profi_out (0);
	proceed (MB_START);

endthread

// Display node stats on UI
__PUBLF (NodePeg, void, stats) (word state) {

	word faults0, faults1;
	word mem0 = memfree(0, &faults0);
	word mem1 = memfree(1, &faults1);

	ser_outf (state, stats_str,
			host_id, local_host,
			tag_auditFreq, tag_eventGran,
			host_pl, seconds(),
			mem0, mem1, faults0, faults1);
}

__PUBLF (NodePeg, void, process_incoming) (word state, char * buf, word size,
								word rssi) {
  int    w_len;

  if (check_msg_size (buf, size, D_SERIOUS) != 0)
	  return;

  switch (in_header(buf, msg_type)) {

	case msg_profi:
		msg_profi_in (buf, rssi);
		return;

	case msg_data:
		msg_data_in (buf);
		return;

	case msg_alrm:
		msg_alrm_in (buf);
		return;

	default:
		app_diag (D_SERIOUS, "Got ? (%u)", in_header(buf, msg_type));

  }
}

// [0, FF] -> [1, FF]
// it can't be 0, as find_tag() will mask the rssi out!
static word map_rssi (word r) {
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
	dupa: will be needed for all sorts of calibrations
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
	word count = 0;

	entry (AS_START)

		if (tag_auditFreq == 0) {
			app_diag (D_WARNING, "Audit stops");
			finish;
		}
		aud_ind = LI_MAX;
		app_diag (D_DEBUG, "Audit starts");

	entry (AS_TAGLOOP)

		if (aud_ind-- == 0) {
			app_diag (D_DEBUG, "Audit ends");
			if (led_state.color == LED_R) {
				if (led_state.dura > 1) {
					if (running (mbeacon))
						led_state.color = LED_G;
					else
						led_state.color = LED_B;
					led_state.state = LED_ON;
					leds (LED_R, LED_OFF);
					leds (led_state.color, LED_ON);
				} else {
					led_state.dura++;
				}

			}
			delay (tag_auditFreq << 10, AS_START);
			release;
		}

		check_tag (aud_ind);
		proceed (AS_TAGLOOP);
endthread

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

static char * stateName (word state) {
	switch ((tagStateType)state) {
		case noTag:
			return "noTag";
		case newTag:
			return "new";
		case reportedTag:
			return "reported";
		case confirmedTag:
			return "confirmed";
		case fadingReportedTag:
			return "fadingReported";
		case fadingConfirmedTag:
			return "fadingConfirmed";
		case goneTag:
			return "gone";
		case sumTag:
			return "sum";
		case fadingMatchedTag:
			return "fadingMatched";
		case matchedTag:
			return "matched";
		default:
			return "unknown?";
	}
}

static char * locatName (word rssi, word pl) { // ignoring pl 
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

static char * descName (word info) {
	if (info & INFO_PRIV) return "private";
	if (info & INFO_BIZ) return "business";
	if (info & INFO_DESC) return "intro";
	return "no desc";
}

__PUBLF (NodePeg, void, oss_profi_out) (word ind) {
	char * lbuf = NULL;

    switch (oss_fmt) {
	case OSS_ASCII_DEF:
		lbuf = form (NULL, profi_ascii_def,
			locatName (tagArray[ind].rssi, tagArray[ind].pl),
			tagArray[ind].nick, tagArray[ind].id,
			seconds() - tagArray[ind].evTime,
			tagArray[ind].intim == 0 ? " " : " *intim* ",
			stateName (tagArray[ind].state),
			tagArray[ind].profi,
			descName (tagArray[ind].info),
			tagArray[ind].desc);
			break;

	case OSS_ASCII_RAW:
		lbuf = form (NULL, profi_ascii_raw,
			tagArray[ind].nick, tagArray[ind].id,
			tagArray[ind].evTime, tagArray[ind].lastTime,
			tagArray[ind].intim, tagArray[ind].state,
			tagArray[ind].profi, tagArray[ind].desc,
			tagArray[ind].info,
			tagArray[ind].pl, tagArray[ind].rssi);
			break;
	default:
		app_diag (D_SERIOUS, "**Unsupported fmt %u", oss_fmt);
		return;

    }

    if (runstrand (oss_out, lbuf) == 0 ) {
	app_diag (D_SERIOUS, "oss_out failed");
	ufree (lbuf);
    }
}

__PUBLF (NodePeg, void, oss_data_out) (word ind) {
	// for now
	oss_profi_out (ind);
}

__PUBLF (NodePeg, void, oss_alrm_out) (char * buf) {
	char * lbuf = NULL;

    switch (oss_fmt) {
	case OSS_ASCII_DEF:
		lbuf = form (NULL, alrm_ascii_def,
			in_alrm(buf, nick), in_header(buf, snd),
			in_alrm(buf, profi), in_alrm(buf, level),
			in_header(buf, hoc), in_header(buf, rcv),
			in_alrm(buf, desc));
		break;

	case OSS_ASCII_RAW:
		lbuf = form (NULL, alrm_ascii_raw,
			in_alrm(buf, nick), in_header(buf, snd),
			in_alrm(buf, profi), in_alrm(buf, level),
			in_header(buf, hoc), in_header(buf, rcv),
			in_alrm(buf, desc));
		break;
	default:
		app_diag (D_SERIOUS, "**Unsupported fmt %u", oss_fmt);
		return;
    }

    if (runstrand (oss_out, lbuf) == 0 ) {
	    app_diag (D_SERIOUS, "oss_out failed");
	    ufree (lbuf);
    }
}

// ==========================================================================

/*
   --------------------
   Root process
   RS_ <-> Root State
   --------------------
*/

thread (root)

	sint	i1;
	word	w1, w2;
	char	s1[NI_LEN];

	entry (RS_INIT)
		ser_out (RS_INIT, welcome_str);
		local_host = (word)host_id;
#ifndef __SMURPH__
		net_id = DEF_NID;
#endif
		tarp_ctrl.param = 0xB1; // level 2, rec 3, slack 0, fwd on

		init_tags();
		ui_obuf = get_mem (RS_INIT, UI_BUFLEN);
#if 0
		// spread in case of a sync reset
		delay (rnd() % (tag_auditFreq << 10), RS_PAUSE);
		release;
#endif
		
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
		runthread (mbeacon);
		led_state.color = LED_G;
		led_state.state = LED_ON;
		leds (LED_G, LED_ON);
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

		switch (cmd_line[0]) {
			case ' ': proceed (RS_FREE); // ignore blank
			case 'q': reset();
			case 'Q': ee_erase (WNONE, 0, 0); reset();
			case 's': proceed (RS_SETS);
			case 'p': proceed (RS_PROFILES);
			case 'h': proceed (RS_HELPS);
			case 'L': proceed (RS_LISTS);
			case 'M': proceed (RS_MLIST);
			case 'Y': proceed (RS_Y);
			case 'N': proceed (RS_N);
			case 'T': proceed (RS_TARGET);
			case 'B': proceed (RS_BIZ);
			case 'P': proceed (RS_PRIV);
			case 'A': proceed (RS_ALRM);
			case 'S': proceed (RS_STOR);
			case 'R': proceed (RS_RETR);
			case 'X': proceed (RS_BEAC);
			default:
				form (ui_obuf, ill_str, cmd_line);
				proceed (RS_UIOUT);
		}

	entry (RS_SETS)

		w1 = strlen (cmd_line);
		if (w1 < 2) {
			form (ui_obuf, ill_str, cmd_line);
			proceed (RS_UIOUT);
		}
		if (w1 > 2)
			w1 -= 3;
		else
			w1 -= 2;

		switch (cmd_line[1]) {
		  case 'n':
			if (w1 > 0)
				strncpy (nick_att, cmd_line +3,
					w1 > NI_LEN ? NI_LEN : w1);
			form (ui_obuf, "Nick: %s", nick_att);
			proceed (RS_UIOUT);

		  case 'd':
			if (w1 > 0)
				strncpy (desc_att, cmd_line +3,
					w1 > PEG_STR_LEN ?
						PEG_STR_LEN : w1);
			form (ui_obuf, "Desc: %s", desc_att);
			proceed (RS_UIOUT);

		  case 'b':
			if (w1 > 0)
				strncpy (d_biz, cmd_line +3,
					w1 > PEG_STR_LEN ?
						PEG_STR_LEN : w1);
			form (ui_obuf, "Biz: %s", d_biz);
			proceed (RS_UIOUT);

		  case 'p':
			if (w1 > 0)
				strncpy (d_priv, cmd_line +3,
					w1 > PEG_STR_LEN ?
						PEG_STR_LEN : w1);
			form (ui_obuf, "Priv: %s", d_priv);
			proceed (RS_UIOUT);

		  case 'a':
			if (w1 > 0)
				strncpy (d_alrm, cmd_line +3,
					w1 > PEG_STR_LEN ?
						PEG_STR_LEN : w1);
			form (ui_obuf, "Alrm: %s", d_alrm);
			proceed (RS_UIOUT);

		  default:
			form (ui_obuf, ill_str, cmd_line);
			proceed (RS_UIOUT);
		}

	entry (RS_PROFILES)

		w1 = strlen (cmd_line);
		if (w1 < 2) {
			form (ui_obuf, ill_str, cmd_line);
			proceed (RS_UIOUT);
		}

		switch (cmd_line[1]) {

		  case 'p':
			if (scan (cmd_line +2, "%x", &w1) > 0)
				profi_att = w1;
			form (ui_obuf, "Profile: %x", profi_att);
			proceed (RS_UIOUT);

		  case 'e':
			if (scan (cmd_line +2, "%x", &w1) > 0)
				p_exc = w1;
			form (ui_obuf, "Exclude: %x", p_exc);
			proceed (RS_UIOUT);

		  case 'i':
			if (scan (cmd_line +2, "%x", &w1) > 0)
				p_inc = w1;
			form (ui_obuf, "Include: %x", p_inc);
			proceed (RS_UIOUT);

		  default:
			form (ui_obuf, ill_str, cmd_line);
			proceed (RS_UIOUT);
		}

	entry (RS_HELPS)

		w1 = strlen (cmd_line);
		if (w1 < 2 || cmd_line[2] == ' ') {
			ser_out (RS_HELPS, welcome_str);
			proceed (RS_FREE);
		}

		switch (cmd_line[1]) {

		  case 's':
			ser_outf (RS_HELPS, hs_str, nick_att, desc_att, d_biz,
					d_priv, d_alrm,
					profi_att, p_exc, p_inc);
			proceed (RS_FREE);
			
		  case 'p':
			stats (RS_HELPS);
			proceed (RS_FREE);

		  default:
			form (ui_obuf, ill_str, cmd_line);
			proceed (RS_UIOUT);
		}

	entry (RS_Y)
		if (scan (cmd_line +1, "%u", &w1) == 0 || w1 == 0) {
			form (ui_obuf, bad_str, cmd_line);
			proceed (RS_UIOUT);
		}

		if ((i1 = find_tag (w1)) < 0) {
			if ((i1 = find_ign (w1)) < 0) {
				app_diag (D_WARNING, "No tag %u", w1);
				proceed (RS_FREE);
			}
			init_ign (i1);
			form (ui_obuf, "ign: removed %u", w1);
			proceed (RS_UIOUT);
		}

		if (tagArray[i1].state == reportedTag)
			set_tagState (i1, confirmedTag, NO);
		else if (tagArray[i1].state == confirmedTag)
			set_tagState (i1, matchedTag, YES);

		msg_data_out (w1, INFO_DESC);
		proceed (RS_FREE);

	// now all this is the same, but won't be...
	entry (RS_BIZ)
		if (scan (cmd_line +1, "%u", &w1) == 0 || w1 == 0) {
			form (ui_obuf, bad_str, cmd_line);
			proceed (RS_UIOUT);
		}

		if ((i1 = find_tag (w1)) < 0) {
			app_diag (D_WARNING, "No tag %u", w1);
			proceed (RS_FREE);
		}

		if (tagArray[i1].state == reportedTag)
			set_tagState (i1, confirmedTag, NO);
		else if (tagArray[i1].state == confirmedTag)
			set_tagState (i1, matchedTag, YES);

		msg_data_out (w1, INFO_BIZ);
		proceed (RS_FREE);

	entry (RS_PRIV)
		if (scan (cmd_line +1, "%u", &w1) == 0 || w1 == 0) {
			form (ui_obuf, bad_str, cmd_line);
			proceed (RS_UIOUT);
		}

		if ((i1 = find_tag (w1)) < 0) {
			app_diag (D_WARNING, "No tag %u", w1);
			proceed (RS_FREE);
		}

		if (tagArray[i1].state == reportedTag)
			set_tagState (i1, confirmedTag, NO);
		else if (tagArray[i1].state == confirmedTag)
			set_tagState (i1, matchedTag, YES);

		msg_data_out (w1, INFO_PRIV);
		proceed (RS_FREE);

	entry (RS_TARGET)
		if (scan (cmd_line +1, "%u", &w1) == 0 || w1 == 0) {
			form (ui_obuf, bad_str, cmd_line);
			proceed (RS_UIOUT);
		}

		msg_profi_out (w1);
		proceed (RS_FREE);

	entry (RS_ALRM)
		w1 = w2 = 0;
		scan (cmd_line +1, "%u %u", &w1, &w2);
#if 0
		if (w1 == 0) {
			form (ui_obuf, bad_str, cmd_line);
			proceed (RS_UIOUT);
		}
#endif
		msg_alrm_out (w1, w2);
		proceed (RS_FREE);

	entry (RS_N)
		if (scan (cmd_line +1, "%u", &w1) == 0 || w1 == 0) {
			form (ui_obuf, bad_str, cmd_line);
			proceed (RS_UIOUT);
		}

		if ((i1 = find_tag (w1)) < 0) {
			app_diag (D_WARNING, "No tag %u", w1);
			proceed (RS_FREE);
		}

		insert_ign (tagArray[i1].id, tagArray[i1].nick);
		proceed (RS_FREE);

	entry (RS_MLIST)
		if (strlen(cmd_line) < 3) {
			form (ui_obuf, bad_str, cmd_line);
			proceed (RS_UIOUT);
		}

		switch (cmd_line[1]) {
		  case '+':
			if (scan (cmd_line +2, "%u %s", &w1, s1) != 2) {
				form (ui_obuf, bad_str, cmd_line);
				proceed (RS_UIOUT);
			}
			if ((i1 = find_mon (w1)) < 0) {
				insert_mon (w1, s1);
				form (ui_obuf, "Mon add %u", w1);
			} else {
				strncpy (monArray[i1].nick, s1, NI_LEN);
				form (ui_obuf, "Mon upd %u", w1);
			}
			proceed (RS_UIOUT);

		  case '-':
			if (scan (cmd_line +2, "%u", &w1) != 1) {
				form (ui_obuf, bad_str, cmd_line);
				proceed (RS_UIOUT); 
			}
			if ((i1 = find_mon (w1)) < 0) {
				form (ui_obuf, "Mon no %u", w1);
			} else {
				init_mon (i1);
				form (ui_obuf, "Mon del %u", w1);
			}
			proceed (RS_UIOUT);

		  default:
			form (ui_obuf, bad_str, cmd_line);
			proceed (RS_UIOUT); 
		}

	entry (RS_LISTS)
		rt_ind = 0;
		switch (cmd_line[1]) {
		  case 't':
			ser_out (RS_LISTS, "Current tags:\r\n");
			proceed (RS_L_TAG);
		  case 'i':
			ser_out (RS_LISTS, "Being ignored:\r\n");
			proceed (RS_L_IGN);
		  case 'm':
			ser_out (RS_LISTS, "Being monitored:\r\n");
			proceed (RS_L_MON);
		  default:
			form (ui_obuf, bad_str, cmd_line);
			proceed (RS_UIOUT);
		}

	entry (RS_L_TAG)
		if (tagArray[rt_ind].id != 0)
			oss_profi_out (rt_ind);

		if (++rt_ind < LI_MAX)
			proceed (RS_L_TAG);

		proceed (RS_FREE);

	entry (RS_L_IGN)
		if (ignArray[rt_ind].id != 0)
			ser_outf (RS_L_IGN, " %u %s\r\n", ignArray[rt_ind].id,
				ignArray[rt_ind].nick);

		if (++rt_ind < LI_MAX)
			proceed (RS_L_IGN);

		proceed (RS_FREE);

	entry (RS_L_MON)
		if (monArray[rt_ind].id != 0)
			ser_outf (RS_L_IGN, " %u %s\r\n", monArray[rt_ind].id,
				monArray[rt_ind].nick);

		if (++rt_ind < LI_MAX)
			proceed (RS_L_IGN);

		proceed (RS_FREE);

	entry (RS_BEAC)
		if (scan (cmd_line +1, "%u", &w1) > 0) {
			if (w1) {
				if (!running (mbeacon)) {
					runthread (mbeacon);
					if (led_state.color == LED_B) {
						led_state.color = LED_G;
						leds (LED_B, LED_OFF);
						leds (LED_G, led_state.state);
					}
				}
						
			} else {
				if (running (mbeacon)) {
					killall (mbeacon);
					if (led_state.color == LED_G) {
						led_state.color = LED_B;
						leds (LED_G, LED_OFF);
						leds (LED_B, led_state.state);
					}
				}
			}
		}
		form (ui_obuf, "Beacon is%stransmitting\r\n",
			       running (mbeacon) ?  " " : " not ");
		proceed (RS_UIOUT);
			
	entry (RS_UIOUT)
		ser_out (RS_UIOUT, ui_obuf);
		proceed (RS_FREE);

endthread

praxis_starter (NodePeg);
