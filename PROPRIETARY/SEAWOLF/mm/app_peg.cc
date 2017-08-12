/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "app_peg_data.h"
#include "diag.h"
#include "tarp.h"
#include "net.h"
#include "msg_peg.h"
#include "ser.h"
#include "serf.h"
#include "storage.h"

#if CC1100
#define INFO_PHYS_DEV INFO_PHYS_CC1100
#else
#error "UNDEFINED RADIO"
#endif

#define UI_BUFLEN		UART_INPUT_BUFFER_LENGTH
#define DEF_NID			85

// Semaphores
#define CMD_READER	(&cmd_line)
#define CMD_WRITER	((&cmd_line)+1)

static char     *ui_ibuf        = NULL,
                *ui_obuf        = NULL,
                *cmd_line       = NULL;

static word	batter		= 0;
static word	tag_auditFreq	= 4; // in seconds

// PicOS differs from VUEE
#ifdef __SMURPH__
word	host_pl	= 1;
#else
word    host_pl = 2;
#endif
word    app_flags               = DEF_APP_FLAGS;
word    tag_eventGran           = 4;  // in seconds

tagDataType     tagArray [LI_MAX];
tagShortType    ignArray [LI_MAX];
tagShortType    monArray [LI_MAX];
nbuComType      nbuArray [LI_MAX];

ledStateType    led_state       = {0, 0, 0};

profi_t profi_att = 0xFFFF,
	p_inc   = 0xFFFF,
	p_exc   = 0;

char    nick_att  [NI_LEN +1]           = "uninit"; //{'\0'};
char    desc_att  [PEG_STR_LEN +1]      = "uninit desc"; //{'\0'};
char    d_biz  [PEG_STR_LEN +1]         = "uninit biz"; //{'\0'};
char    d_priv  [PEG_STR_LEN +1]        = "uninit priv"; //{'\0'};
char    d_alrm  [PEG_STR_LEN +1]        = {'\0'};


// =============
// OSS reporting
// =============
fsm oss_out (char*) {
        state OO_RETRY:
		if (data == NULL)
			app_diag_S (oss_out_f_str);
		else
			ser_outb (OO_RETRY, data);
                finish;
}

fsm mbeacon {

    state MB_START:
	delay (2048 + rnd() % 2048, MB_SEND); // 2 - 4s
	release;

    state MB_SEND:
	msg_profi_out (0);
	proceed (MB_START);

}

// Display node stats on UI
static void stats (word state) {

	word faults0, faults1;
	word mem0 = memfree(0, &faults0);
	word mem1 = memfree(1, &faults1);

	ser_outf (state, stats_str,
			local_host, seconds(), batter,
			running (mbeacon) ? 1 : 0,
			tag_auditFreq, tag_eventGran, host_pl,
			mem0, mem1, faults0, faults1);
}

static void process_incoming (word state, char * buf, word size, word rssi) {

  if (check_msg_size (buf, size) != 0)
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
		app_diag_S ("Got ? (%u)", in_header(buf, msg_type));

  }
}



// [0, FF] -> [1, FF]
// it can't be 0, as find_tag() will mask the rssi out!
// All RSSI manipulations (or lack of them) thould be done here. The below
// is just a lame example, hopefully useful for demonstrations
static word map_rssi (word r) {
	return r >> 8;
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
	//diag ("rssi %u", r >> 8);

#if 0
	plev 1:
	if ((r >> 8) > 161) return 3;
	if ((r >> 8) > 140) return 2;

	plev 2:
#endif
	if ((r >> 8) > 181) return 3;
	if ((r >> 8) > 150) return 2;
	return 1;
#endif
	
#endif
}

/*
   --------------------
   Receiver process
   RS_ <-> Receiver State
   --------------------
*/

fsm rcv {

	sint	packet_size = 0;
	word	rssi = 0;
	char   *buf_ptr = NULL;

	state RC_TRY:

		if (buf_ptr != NULL) {
			ufree (buf_ptr);
			buf_ptr = NULL;
			packet_size = 0;
		}
    		packet_size = net_rx (RC_TRY, &buf_ptr, &rssi, 0);
		if (packet_size <= 0) {
			app_diag_S ("net_rx failed (%d)", packet_size);
			proceed RC_TRY;
		}

		app_diag_D ("RCV (%d): %x-%u-%u-%u-%u-%u\r\n",
			packet_size, in_header(buf_ptr, msg_type),
			in_header(buf_ptr, seq_no) & 0xffff,
			in_header(buf_ptr, snd),
			in_header(buf_ptr, rcv),
			in_header(buf_ptr, hoc) & 0xffff,
			in_header(buf_ptr, hco) & 0xffff);

		// that's how we could check which plugin is on
		// if (net_opt (PHYSOPT_PLUGINFO, NULL) != INFO_PLUG_TARP)

	state RC_MSG:
#if 0
	this may be needed for all sorts of calibrations
		if (in_header(buf_ptr, msg_type) == msg_pong)
			app_diag_U ("rss (%d.%d): %d",
				in_header(buf_ptr, snd),
				in_pong(buf_ptr, level), rssi >> 8);
		else
			app_diag_U ("rss %d from %d", rssi >> 8,
					in_header(buf_ptr, snd));
#endif
		process_incoming (RC_MSG, buf_ptr, packet_size,
			map_rssi(rssi));

		proceed RC_TRY;

}

/*
  --------------------
  audit process
  AS_ <-> Audit State
  --------------------
*/

fsm audit {

	word ind;

	state AS_START:

		if (tag_auditFreq == 0) {
			app_diag_W ("Audit stops");
			finish;
		}
		read_sensor (AS_START, -1, &batter);
		ind = LI_MAX;

		// leds should be better... when we know what's needed
		if (led_state.state != LED_OFF &&
				led_state.state != LED_ON) {
			led_state.state = LED_ON;
			leds (led_state.color, LED_ON);
		}

		app_diag_D ("Audit starts");

	state AS_TAGLOOP:

		if (ind-- == 0) {
			app_diag_D ("Audit ends");
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

		check_tag (ind);
		proceed AS_TAGLOOP;
}

/*
   --------------------
   cmd_in process
   CS_ <-> Command State
   --------------------
*/
fsm cmd_in {

	state CS_INIT:

		if (ui_ibuf == NULL)
			ui_ibuf = get_mem (CS_INIT, UI_BUFLEN);

	state CS_IN:

		// hangs on the uart_a interrupt or polling
		ser_in (CS_IN, ui_ibuf, UI_BUFLEN);
		if (strlen(ui_ibuf) == 0)
			// CR on empty line would do it
			proceed CS_IN;

	state CS_WAIT:

		if (cmd_line != NULL) {
			when (CMD_WRITER, CS_WAIT);
			release;
		}

		cmd_line = get_mem (CS_WAIT, strlen(ui_ibuf) +1);
		strcpy (cmd_line, ui_ibuf);
		trigger (CMD_READER);
		proceed CS_IN;

}

static const char * stateName (word state) {
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
#if 0
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
}
#endif
static const char * descName (word info) {
	if (info & INFO_PRIV) return "private";
	if (info & INFO_BIZ) return "business";
	if (info & INFO_DESC) return "intro";
	// noombuzz is independent...
	return "noDesc";
}

static const char * descMark (word info, word list) {

	if (list)
		return "LT";

	if (info & INFO_PRIV) return "PRIV";
	if (info & INFO_BIZ) return "BIZ";
	if (info & INFO_DESC) return "DESC";
	// noombuzz is independent...
	return "HELLO";
}

// arg. list indicates calls from 'lt'; kludgy wrap for 'LT' for Android stuff
void oss_profi_out (word ind, word list) {
	char * lbuf = NULL;

    switch (oss_fmt) {
	case OSS_ASCII_DEF:
		lbuf = form (NULL, profi_ascii_def,
#if ANDROIDEMO
                        descMark(tagArray[ind].info, list),
#endif
			//locatName (tagArray[ind].rssi, tagArray[ind].pl),
			 tagArray[ind].pl, tagArray[ind].rssi,
			tagArray[ind].nick, tagArray[ind].id,
			seconds() - tagArray[ind].evTime,
#if ANDROIDEMO == 0
			tagArray[ind].intim == 0 ? " " : " *intim* ",
#endif
			stateName (tagArray[ind].state),
			tagArray[ind].profi,
			descName (tagArray[ind].info),
#if ANDROIDEMO == 0
			(tagArray[ind].info & INFO_NBUZZ) ? " noombuzz)" : ")",
#endif
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
		app_diag_S ("**Unsupported fmt %u", oss_fmt);
		return;

    }

    if (runfsm oss_out (lbuf) == 0 ) {
	app_diag_S (oss_out_f_str);
	ufree (lbuf);
    }
}

void oss_data_out (word ind) {
	// for now
	oss_profi_out (ind, 0);
}

void oss_nvm_out (nvmDataType * buf, word slot) {
	char * lbuf = NULL;

	// no matter what format
	if (slot == 0)
		lbuf = form (NULL, nvm_local_ascii_def, slot, buf->id,
			buf->profi, buf->local_inc, buf->local_exc,
			buf->nick, buf->desc, buf->dpriv, buf->dbiz);
	else
		lbuf = form (NULL, nvm_ascii_def, slot, buf->id, buf->profi,
			buf->nick, buf->desc, buf->dpriv, buf->dbiz);

	if (runfsm oss_out (lbuf) == 0 ) {
		app_diag_S (oss_out_f_str);
		ufree (lbuf);
	}
}

void oss_alrm_out (char * buf) {
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
		app_diag_S ("**Unsupported fmt %u", oss_fmt);
		return;
    }

    if (runfsm oss_out (lbuf) == 0 ) {
	    app_diag_S (oss_out_f_str);
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

fsm root {

	sint	i1;
	word	w1, w2, w3, w4, rt_ind, rt_id;
	nvmDataType nvm_dat;
	char	s1 [18]; // unless NI_LEN is greater

	state RS_INIT:
	//w1 = memfree(0, &w2);
	//diag ("mem check %u %u", w1, w2 );

#ifdef BOARD_WARSAW_BLUE
                ser_select (1);
#endif

		ser_out (RS_INIT, welcome_str);
		ee_open ();
#ifndef __SMURPH__
		net_id = DEF_NID;
#endif
		//tarp_ctrl.param = 0xB1; // level 2, rec 3, slack 0, fwd on
		tarp_ctrl.param = 0xA3; // level 2, rec 2, slack 1, fwd on

		init_tags();
		ui_obuf = get_mem (RS_INIT, UI_BUFLEN);
#if 0
		// spread in case of a sync reset
		delay (rnd() % (tag_auditFreq << 10), RS_PAUSE);
		release;
#endif
		
	state RS_PAUSE:

		if (net_init (INFO_PHYS_DEV, INFO_PLUG_TARP) < 0) {
			app_diag_F ("net_init failed");
			reset();
		}
		net_opt (PHYSOPT_SETSID, &net_id);
		net_opt (PHYSOPT_SETPOWER, &host_pl);
		runfsm rcv;
		runfsm cmd_in;
		runfsm audit;
		led_state.color = LED_G;
		led_state.state = LED_ON;
		leds (LED_G, LED_ON);

		if (ee_read (NVM_OSET, (byte *)&nvm_dat, NVM_SLOT_SIZE)) {
			app_diag_S ("Can't read nvm");
		} else {
	 	  if (nvm_dat.id != 0xFFFF) {
#if 0
			if (nvm_dat.id != local_host)
				app_diag_W ("Bad nvm data");
			else {
#endif
				local_host = nvm_dat.id;
				strncpy (nick_att, nvm_dat.nick, NI_LEN);
				strncpy (desc_att, nvm_dat.desc, PEG_STR_LEN);
				strncpy (d_biz, nvm_dat.dbiz, PEG_STR_LEN);
				strncpy (d_priv, nvm_dat.dpriv, PEG_STR_LEN);
				profi_att = nvm_dat.profi;
				p_inc = nvm_dat.local_inc;
				p_exc = nvm_dat.local_exc;
				oss_nvm_out (&nvm_dat, 0);
//			}
		  }

		  else {
			  local_host = (word)host_id;
#ifdef __SMURPH__
			  // model doesn't hold eprom at start, but has preinits
			  memset (&nvm_dat, 0xFF, NVM_SLOT_SIZE);
			  nvm_dat.id = local_host;
			  nvm_dat.profi = profi_att;
			  nvm_dat.local_inc = p_inc;
			  nvm_dat.local_exc = p_exc;
			  strncpy (nvm_dat.nick, nick_att, NI_LEN);
			  strncpy (nvm_dat.desc, desc_att, PEG_STR_LEN);
			  strncpy (nvm_dat.dpriv, d_priv, PEG_STR_LEN);
			  strncpy (nvm_dat.dbiz, d_biz, PEG_STR_LEN);
			  if (ee_write (WNONE, NVM_OSET, (byte *)&nvm_dat,
						  NVM_SLOT_SIZE))
				  app_diag_S ("ee_write failed");
#endif
		  }
		}

		if (local_host != 0xDEAD)
			runfsm mbeacon;

		proceed RS_RCMD;

	state RS_FREE:

		ufree (cmd_line);
		cmd_line = NULL;
		trigger (CMD_WRITER);

	state RS_RCMD:

		if (cmd_line == NULL) {
			when (CMD_READER, RS_RCMD);
			release;
		}

	state RS_DOCMD:

		switch (cmd_line[0]) {
			case ' ': proceed RS_FREE; // ignore blank
			case 'q': reset();
			case 'Q': ee_erase (WNONE, 0, 0); reset();
			case 's': proceed RS_SETS;
			case 'p': proceed RS_PROFILES;
			case '?':
			case 'h': proceed RS_HELPS;
			case 'L': proceed RS_LISTS;
			case 'M': proceed RS_MLIST;
			case 'Z': proceed RS_ZLIST;
			case 'Y': proceed RS_Y;
			case 'N': proceed RS_N;
			case 'T': proceed RS_TARGET;
			case 'B': proceed RS_BIZ;
			case 'P': proceed RS_PRIV;
			case 'A': proceed RS_ALRM;
			case 'S': proceed RS_STOR;
			case 'R': proceed RS_RETR;
			case 'E': proceed RS_ERA;
			case 'X': proceed RS_BEAC;
			case 'U': proceed RS_AUTO;
			default:
				form (ui_obuf, ill_str, cmd_line);
				proceed RS_UIOUT;
		}

	state RS_SETS:

		w1 = strlen (cmd_line);
		if (w1 < 2) {
			form (ui_obuf, ill_str, cmd_line);
			proceed RS_UIOUT;
		}
		if (w1 > 2)
			w1 -= 3;
		else
			w1 -= 2;

		switch (cmd_line[1]) {

		  case 'i':
			if (w1 > 0 && scan (cmd_line +2, "%u", &w1) > 0) {
				if (w1 == 0xDEAD && local_host != 0xDEAD) {
					ee_erase (WNONE, 0, 0); 
					reset();
				}
				if (w1 != 0xDEAD && w1 != 0 &&
						local_host == 0xDEAD) {
					local_host = rt_id = w1;
					if (!running (mbeacon)) // shouldn't be
							runfsm mbeacon;
					goto StoreAll;
				}
			}
			form (ui_obuf, "Id: %u\r\n", local_host);
			proceed RS_UIOUT;

		  case 'n':
			if (w1 > 0)
				strncpy (nick_att, cmd_line +3,
					w1 > NI_LEN ? NI_LEN : w1);
			form (ui_obuf, "Nick: %s\r\n", nick_att);
			proceed RS_UIOUT;

		  case 'd':
			if (w1 > 0)
				strncpy (desc_att, cmd_line +3,
					w1 > PEG_STR_LEN ?
						PEG_STR_LEN : w1);
			form (ui_obuf, "Desc: %s\r\n", desc_att);
			proceed RS_UIOUT;

		  case 'b':
			if (w1 > 0)
				strncpy (d_biz, cmd_line +3,
					w1 > PEG_STR_LEN ?
						PEG_STR_LEN : w1);
			form (ui_obuf, "Biz: %s\r\n", d_biz);
			proceed RS_UIOUT;

		  case 'p':
			if (w1 > 0)
				strncpy (d_priv, cmd_line +3,
					w1 > PEG_STR_LEN ?
						PEG_STR_LEN : w1);
			form (ui_obuf, "Priv: %s\r\n", d_priv);
			proceed RS_UIOUT;

		  case 'a':
			if (w1 > 0)
				strncpy (d_alrm, cmd_line +3,
					w1 > PEG_STR_LEN ?
						PEG_STR_LEN : w1);
			form (ui_obuf, "Alrm: %s\r\n", d_alrm);
			proceed RS_UIOUT;

		  default:
			form (ui_obuf, ill_str, cmd_line);
			proceed RS_UIOUT;
		}

	state RS_PROFILES:

		w1 = strlen (cmd_line);
		if (w1 < 2) {
			form (ui_obuf, ill_str, cmd_line);
			proceed RS_UIOUT;
		}

		switch (cmd_line[1]) {

		  case 'p':
			if (scan (cmd_line +2, "%x", &w1) > 0)
				profi_att = w1;
			form (ui_obuf, "Profile: %x\r\n", profi_att);
			proceed RS_UIOUT;

		  case 'e':
			if (scan (cmd_line +2, "%x", &w1) > 0)
				p_exc = w1;
			form (ui_obuf, "Exclude: %x\r\n", p_exc);
			proceed RS_UIOUT;

		  case 'i':
			if (scan (cmd_line +2, "%x", &w1) > 0)
				p_inc = w1;
			form (ui_obuf, "Include: %x\r\n", p_inc);
			proceed RS_UIOUT;

		  default:
			form (ui_obuf, ill_str, cmd_line);
			proceed RS_UIOUT;
		}

	state RS_HELPS:

		w1 = strlen (cmd_line);
		if (w1 < 2 || cmd_line[1] == ' ') {
			ser_out (RS_HELPS, welcome_str);
			proceed RS_FREE;
		}

		switch (cmd_line[1]) {

		  case 's':
			ser_outf (RS_HELPS, hs_str, nick_att, desc_att, d_biz,
					d_priv, d_alrm,
					profi_att, p_exc, p_inc);
			proceed RS_FREE;
			
		  case 'p':
			stats (RS_HELPS);
			proceed RS_FREE;

		  case 'z':
			w2 = w3 = 0xFFFF;
			if (w1 > 2)
				scan (cmd_line +2, "%x %x", &w2, &w3);
			rt_id = w2 & w3;
			rt_ind = 7;
			proceed RS_Z_HELP;

		  case 'e': // I know, but keep 'e' and 'z' separate
			w2 = w3 = 0xFFFF;
			if (w1 > 2)
				scan (cmd_line +2, "%x %x", &w2, &w3);
			rt_id = w2 & w3;
			rt_ind = 15;
			proceed RS_E_HELP;

		  default:
			form (ui_obuf, ill_str, cmd_line);
			proceed RS_UIOUT;
		}

	state RS_Y:
		if (scan (cmd_line +1, "%u", &w1) == 0 || w1 == 0) {
			form (ui_obuf, bad_str, cmd_line);
			proceed RS_UIOUT;
		}

		if ((i1 = find_tag (w1)) < 0) {
			if ((i1 = find_ign (w1)) < 0) {
				app_diag_W ("No tag %u", w1);
				proceed RS_FREE;
			}
			init_ign (i1);
			form (ui_obuf, "ign: removed %u\r\n", w1);
			proceed RS_UIOUT;
		}

		if (tagArray[i1].state == reportedTag)
			set_tagState (i1, confirmedTag, NO);
		else if (tagArray[i1].state == confirmedTag &&
				tagArray[i1].info & INFO_IN)
			set_tagState (i1, matchedTag, YES);

		msg_data_out (w1, INFO_DESC);
		proceed RS_FREE;

	// now all this is the same, but won't be...
	state RS_BIZ:
		if (scan (cmd_line +1, "%u", &w1) == 0 || w1 == 0) {
			form (ui_obuf, bad_str, cmd_line);
			proceed RS_UIOUT;
		}

		if ((i1 = find_tag (w1)) < 0) {
			app_diag_W ("No tag %u", w1);
			proceed RS_FREE;
		}

		if (tagArray[i1].state == reportedTag)
			set_tagState (i1, confirmedTag, NO);
		else if (tagArray[i1].state == confirmedTag &&
				tagArray[i1].info & INFO_IN)
			set_tagState (i1, matchedTag, YES);

		msg_data_out (w1, INFO_BIZ);
		proceed RS_FREE;

	state RS_PRIV:
		if (scan (cmd_line +1, "%u", &w1) == 0 || w1 == 0) {
			form (ui_obuf, bad_str, cmd_line);
			proceed RS_UIOUT;
		}

		if ((i1 = find_tag (w1)) < 0) {
			app_diag_W ("No tag %u", w1);
			proceed RS_FREE;
		}

		if (tagArray[i1].state == reportedTag)
			set_tagState (i1, confirmedTag, NO);
		else if (tagArray[i1].state == confirmedTag &&
				tagArray[i1].info & INFO_IN)
			set_tagState (i1, matchedTag, YES);

		msg_data_out (w1, INFO_PRIV);
		proceed RS_FREE;

	state RS_TARGET:
		if (scan (cmd_line +1, "%u", &w1) == 0 || w1 == 0) {
			form (ui_obuf, bad_str, cmd_line);
			proceed RS_UIOUT;
		}

		msg_profi_out (w1);
		proceed RS_FREE;

	state RS_ALRM:
		w1 = w2 = 0;
		scan (cmd_line +1, "%u %u", &w1, &w2);
#if 0
		if (w1 == 0) {
			form (ui_obuf, bad_str, cmd_line);
			proceed (RS_UIOUT);
		}
#endif
		msg_alrm_out (w1, w2, NULL);
		proceed (RS_FREE);

	state RS_N:
		if (scan (cmd_line +1, "%u", &w1) == 0 || w1 == 0) {
			form (ui_obuf, bad_str, cmd_line);
			proceed RS_UIOUT;
		}

		if ((i1 = find_tag (w1)) < 0) {
			app_diag_W ("No tag %u", w1);
			proceed RS_FREE;
		}

		insert_ign (tagArray[i1].id, tagArray[i1].nick);
		proceed RS_FREE;

	state RS_ZLIST:
		if (strlen(cmd_line) < 3) {
			form (ui_obuf, bad_str, cmd_line);
			proceed RS_UIOUT;
		}

		switch (cmd_line[1]) {
		  case '+':
			if (scan (cmd_line +2, "%u %u %x %u %s",
			    &w1, &w2, &w3, &w4, s1) != 5 || w1 == 0) {
				form (ui_obuf, bad_str, cmd_line);
				proceed RS_UIOUT;
			}
			if ((i1 = find_nbu (w1)) < 0) {
				insert_nbu (w1, w2, w3, w4, s1);
				form (ui_obuf, "NBuzz add %u\r\n", w1);
			} else {
				nbuArray[i1].what = w2;
				nbuArray[i1].vect = w3;
				nbuArray[i1].dhook = w4;
				strncpy (nbuArray[i1].memo, s1, NI_LEN);
				form (ui_obuf, "NBuzz upd %u\r\n", w1);
			}
			proceed RS_UIOUT;

		  case '-':
			if (scan (cmd_line +2, "%u", &w1) != 1 || w1 == 0) {
				form (ui_obuf, bad_str, cmd_line);
				proceed RS_UIOUT;
			}
			if ((i1 = find_nbu (w1)) < 0) {
				form (ui_obuf, "NBuzz no %u\r\n", w1);
			} else {
				init_nbu (i1);
				form (ui_obuf, "NBuzz del %u\r\n", w1);
			}
			proceed RS_UIOUT;

		  default:
			form (ui_obuf, bad_str, cmd_line);
			proceed RS_UIOUT;
		}

	state RS_MLIST:
		if (strlen(cmd_line) < 3) {
			form (ui_obuf, bad_str, cmd_line);
			proceed RS_UIOUT;
		}

		switch (cmd_line[1]) {
		  case '+':
			if (scan (cmd_line +2, "%u %s", &w1, s1) != 2 ||
					w1 == 0) {
				form (ui_obuf, bad_str, cmd_line);
				proceed RS_UIOUT;
			}
			if ((i1 = find_mon (w1)) < 0) {
				insert_mon (w1, s1);
				form (ui_obuf, "Mon add %u\r\n", w1);
			} else {
				strncpy (monArray[i1].nick, s1, NI_LEN);
				form (ui_obuf, "Mon upd %u\r\n", w1);
			}
			proceed RS_UIOUT;

		  case '-':
			if (scan (cmd_line +2, "%u", &w1) != 1 || w1 == 0) {
				form (ui_obuf, bad_str, cmd_line);
				proceed RS_UIOUT; 
			}
			if ((i1 = find_mon (w1)) < 0) {
				form (ui_obuf, "Mon no %u\r\n", w1);
			} else {
				init_mon (i1);
				form (ui_obuf, "Mon del %u\r\n", w1);
			}
			proceed RS_UIOUT;

		  default:
			form (ui_obuf, bad_str, cmd_line);
			proceed RS_UIOUT; 
		}

	state RS_ERA:
		rt_id = 0;
		scan (cmd_line +1, "%u", &rt_id);
		if (rt_id == 0) {
			form (ui_obuf, bad_str, cmd_line);
			proceed RS_UIOUT;
		}
		rt_ind = 0;
		proceed RS_E_NVM;

	state RS_STOR:
		rt_id = 0;
		rt_ind = 0;
		scan (cmd_line +1, "%u", &rt_id);

		if (rt_id == 0)
			proceed RS_L_NVM;
StoreAll:
		if (rt_id == local_host) {
			memset (&nvm_dat, 0xFF, NVM_SLOT_SIZE);
			nvm_dat.id = rt_id;
			nvm_dat.profi = profi_att;
			nvm_dat.local_inc = p_inc;
			nvm_dat.local_exc = p_exc;
			strncpy (nvm_dat.nick, nick_att, NI_LEN);
			strncpy (nvm_dat.desc, desc_att, PEG_STR_LEN);
			strncpy (nvm_dat.dpriv, d_priv, PEG_STR_LEN);
			strncpy (nvm_dat.dbiz, d_biz, PEG_STR_LEN);

			if (ee_write (WNONE, NVM_OSET, (byte *)&nvm_dat,
						NVM_SLOT_SIZE))
				app_diag_S ("ee_write failed");

			proceed RS_FREE;
		}

		if ((i1 = find_tag (rt_id)) < 0) {
			form (ui_obuf, "No curr tag %u\r\n", rt_id);
			proceed RS_UIOUT;
		}
		rt_id = i1;
		proceed RS_S_NVM;

	state RS_RETR:
		rt_id = 0;
		rt_ind = 0;
		scan (cmd_line +1, "%u", &rt_id);
		proceed RS_L_NVM;

	state RS_E_NVM:
		if (++rt_ind >= NVM_SLOT_NUM) { // starting at 1
			app_diag_W ("No nvm entry %u", rt_id);
			proceed RS_FREE;
		}
		if (ee_read (NVM_OSET + NVM_SLOT_SIZE * rt_ind,
					(byte *)&nvm_dat, NVM_SLOT_SIZE)) {
			app_diag_S ("NMV read failed");
			proceed RS_FREE;
		}
		if (nvm_dat.id == rt_id) {
			memset (&nvm_dat, 0xFFFF, NVM_SLOT_SIZE);
			if (ee_write (WNONE, NVM_OSET + rt_ind * NVM_SLOT_SIZE,
					(byte *)&nvm_dat, NVM_SLOT_SIZE)) {
				app_diag_S ("ee_write failed");
				proceed RS_FREE; 
			}
			rt_id = 0;
			rt_ind = 0;
			proceed RS_L_NVM;
		}
		proceed RS_E_NVM;

	state RS_S_NVM:
		if (++rt_ind >= NVM_SLOT_NUM) { // starting at 1
			app_diag_W ("NVM full");
			proceed RS_FREE;
		}

		if (ee_read (NVM_OSET + NVM_SLOT_SIZE * rt_ind,
					(byte *)&nvm_dat, NVM_SLOT_SIZE)) {
			app_diag_S ("NMV read failed");
			proceed RS_FREE;
		}

		if (nvm_dat.id == 0xFFFF || nvm_dat.id == tagArray[rt_id].id) {

			if (nvm_dat.id == 0xFFFF) {
				nvm_dat.dpriv[0] = '\0';
				nvm_dat.dbiz [0] = '\0';
				nvm_dat.desc [0] = '\0';
			}

			nvm_dat.id = tagArray[rt_id].id;
			nvm_dat.profi = tagArray[rt_id].profi;
			strncpy (nvm_dat.nick, tagArray[rt_id].nick, NI_LEN);

			if (tagArray[rt_id].info & INFO_PRIV) {
				strncpy (nvm_dat.dpriv, tagArray[rt_id].desc,
						PEG_STR_LEN);
			} else if (tagArray[rt_id].info & INFO_BIZ) {
				strncpy (nvm_dat.dbiz, tagArray[rt_id].desc,
						PEG_STR_LEN);
			} else {
				strncpy (nvm_dat.desc, tagArray[rt_id].desc,
						PEG_STR_LEN);
			}

			if (ee_write (WNONE, NVM_OSET + rt_ind * NVM_SLOT_SIZE,
					(byte *)&nvm_dat, NVM_SLOT_SIZE))
				app_diag_S ("ee_write failed");

			proceed RS_FREE;
		}
		proceed RS_S_NVM;

	state RS_LISTS:
		rt_ind = 0;
		switch (cmd_line[1]) {
		  case 'z':
			ser_out (RS_LISTS, nb_imp_str);
			proceed RS_L_NBU;
		  case 't':
			ser_out (RS_LISTS, cur_tag_str);
			proceed RS_L_TAG;
		  case 'i':
			ser_out (RS_LISTS, be_ign_str);
			proceed RS_L_IGN;
		  case 'm':
			ser_out (RS_LISTS, be_mon_str);
			proceed RS_L_MON;
		  default:
			form (ui_obuf, bad_str, cmd_line);
			proceed RS_UIOUT;
		}

	state RS_L_NVM:
		if (ee_read (NVM_OSET + NVM_SLOT_SIZE * rt_ind,
					(byte *)&nvm_dat, NVM_SLOT_SIZE)) {
			app_diag_S ("NVM read failed");
			proceed RS_FREE;
		}

		if (nvm_dat.id != 0xFFFF && (rt_id == 0 || nvm_dat.id == rt_id))
			oss_nvm_out (&nvm_dat, rt_ind);

		if (++rt_ind < NVM_SLOT_NUM)
			proceed RS_L_NVM;

		proceed RS_FREE;

	state RS_L_TAG:
		if (tagArray[rt_ind].id != 0)
			oss_profi_out (rt_ind, 1);

		if (++rt_ind < LI_MAX)
			proceed RS_L_TAG;

		proceed RS_FREE;

	state RS_L_IGN:
		if (ignArray[rt_ind].id != 0)
			ser_outf (RS_L_IGN, be_ign_el_str, ignArray[rt_ind].id,
				ignArray[rt_ind].nick);

		if (++rt_ind < LI_MAX)
			proceed (RS_L_IGN);

		proceed RS_FREE;

	state RS_L_NBU:
		if (nbuArray[rt_ind].id != 0) {
			nbuVec (s1, nbuArray[rt_ind].vect);
			s1[8] = '\0';
			ser_outf (RS_L_NBU, nb_imp_el_str,
				nbuArray[rt_ind].what ? '+' : '-',
				nbuArray[rt_ind].id,
				nbuArray[rt_ind].dhook, s1,
				nbuArray[rt_ind].memo);
		}

		if (++rt_ind < LI_MAX)
			proceed RS_L_NBU;

		proceed RS_FREE;

	state RS_L_MON:
		if (monArray[rt_ind].id != 0)
			ser_outf (RS_L_MON, be_mon_el_str, monArray[rt_ind].id,
				monArray[rt_ind].nick);

		if (++rt_ind < LI_MAX)
			proceed RS_L_MON;

		proceed RS_FREE;

	state RS_Z_HELP:
		if (rt_id & (1 << rt_ind))
			ser_outf (RS_Z_HELP, hz_str, rt_ind, d_nbu[rt_ind]);
		if (rt_ind == 0)
			proceed RS_FREE;
		rt_ind--;
		proceed RS_Z_HELP;

	state RS_E_HELP:
		if (rt_id & (1 << rt_ind))
			ser_outf (RS_E_HELP, he_str, rt_ind, d_event[rt_ind]);
		if (rt_ind == 0)
			proceed RS_FREE;
		rt_ind--;
		proceed RS_E_HELP;

	state RS_BEAC:
		if (scan (cmd_line +1, "%u", &w1) > 0) {
			if (w1) {
				if (!running (mbeacon)) {
					runfsm mbeacon;
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
		proceed RS_UIOUT;

	state RS_AUTO:
		if (scan (cmd_line +1, "%u", &w1) > 0) {
			if (w1)
				set_autoack;
			else
				clr_autoack;
		}
		form (ui_obuf, "Automatch is %s\r\n",
				is_autoack ? "on" : "off");
		proceed RS_UIOUT;

	state RS_UIOUT:
		ser_out (RS_UIOUT, ui_obuf);
		proceed RS_FREE;

}

