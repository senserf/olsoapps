/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "app_sui_data.h"
#include "diag.h"
#include "tarp.h"
#include "net.h"
#include "msg.h"
#include "oss_sui.h"
#include "ser.h"
#include "serf.h"
#include "storage.h"

#if CC1100
#define INFO_PHYS_DEV INFO_PHYS_CC1100
#else
#error "UNDEFINED RADIO"
#endif

#define UI_BUFLEN		UART_INPUT_BUFFER_LENGTH

// Semaphores
#define CMD_READER	(&cmd_line)
#define CMD_WRITER	((&cmd_line)+1)

static char     *ui_ibuf        = NULL,
                *ui_obuf        = NULL,
                *cmd_line       = NULL;

static word	batter		= 0;
static word	tag_auditFreq	= 4; // in seconds

// Hunt part should be common for SUI and LCD and will be... when needed.
// For now, let's hack it into alrm processing, with statics as in LCD
static lword hts, hstart;
static word hunt[4] = {1000, 1001, 1002, 1003};
static word hunt_ind = 0;
#define WIN_RSSI	200
#define HUNT_END	3

word	host_pl	= 7;
word    app_flags               = DEF_APP_FLAGS;
word    tag_eventGran           = 4;  // in seconds

tagDataType     tagArray [LI_MAX];
tagShortType    ignArray [LI_MAX];
tagShortType    monArray [LI_MAX];
nbuComType      nbuArray [LI_MAX];

ledStateType    led_state       = {0, 0, 0};

profi_t profi_att = (profi_t) PREINIT (PROF_SUI, "PROFI");
profi_t p_inc = (profi_t) PREINIT (0xFFFF, "PINC");
profi_t p_exc = (profi_t) PREINIT (0, "PEXC");

#ifdef __SMURPH__
// a bit costly, do it for vuee only, see strcpy at root's init
const char * _nick_att_ini = (char *) PREINIT ("uninit", "NICK");
const char * _desc_att_ini = (char *) PREINIT ("uninit desc", "DESC");
const char * _d_biz_ini = (char *) PREINIT ("uninit biz", "DBIZ");
const char * _d_priv_ini = (char *) PREINIT ("uninit priv", "DPRIV");
#endif

char    nick_att  [NI_LEN +1]          = "nick";
char    desc_att  [PEG_STR_LEN +1]      = "desc";
char    d_biz  [PEG_STR_LEN +1]         = "biz";
char    d_priv  [PEG_STR_LEN +1]        = "priv";
char    d_alrm  [PEG_STR_LEN +1]        = {'\0'};

fsm mbeacon {

    state MB_START:
		delay (2048 + rnd() % 2048, MB_SEND); // 2 - 4s
		release;

    initial state MB_SEND:
		msg_profi_out (0);
		proceed (MB_START);

}


fsm abeacon (word target) {

    state START:
	if (local_host == PINDA_ID) 
		delay (60000 + (rnd() % 2048), SEND);
    	else
		delay (2048 + rnd() % 2048, SEND); // 2 - 4s
	release;

    initial state SEND:
		msg_alrm_out (target, local_host == PINDA_ID ? 9 : 0, NULL);
		proceed (START);

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
	word hunting = 0;

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
		// for now, a hack...
		if (in_alrm(buf, profi) & PROF_TAG) {
		
			if (!(profi_att & PROF_HUNT) || hunt_ind >= HUNT_END ||
					in_header(buf, snd) != hunt[hunt_ind])
				return;
				
			// current hunt
			hts = seconds();
			if (in_header (buf, hoc) <= 1) {
			
				if (rssi >= WIN_RSSI) {

					rssi = WIN_RSSI;

					if (++hunt_ind >= HUNT_END) {
						hunting = 2;
					} else {
						hunting = 1;
						rssi = 0; // a new one counts
					}
				} 
			}
			form (in_alrm(buf, desc), "!TT!,%u,%u,%u,%u",
				in_header (buf, hoc), 
				WIN_RSSI - rssi, HUNT_END - hunt_ind, hunting);
			form (d_alrm, "!TH!&%u&%u&%u&%u&%u", 
				(word)(hts - hstart),
				in_header (buf, hoc), WIN_RSSI - rssi, 
				HUNT_END - hunt_ind, hunting);	

			if (hunting >=2 ) {
				if (!running (abeacon)) {
					runfsm abeacon (PINDA_ID);
				}
			} else {
				msg_alrm_out (PINDA_ID, 9, NULL);
			}
		}
			
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

		// hunt check
		if ((profi_att & PROF_HUNT) && hunt_ind < HUNT_END &&
				seconds() - hts > 10) {
			form (d_alrm, "!TH!&%u&%u&%u&%u&%u",
					(word)(seconds() - hstart),
					0, 0, HUNT_END - hunt_ind, 0);
			msg_alrm_out (PINDA_ID, 0, NULL);
			oss_alrm_out (NULL);
		}
		
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
		
#ifdef __SMURPH__
	strncpy (nick_att, _nick_att_ini, NI_LEN);
	strncpy (desc_att, _desc_att_ini, PEG_STR_LEN);
	strncpy (d_biz, _d_biz_ini, PEG_STR_LEN);
	strncpy (d_priv, _d_priv_ini, PEG_STR_LEN);
#endif
		net_id = DEF_NID;

		//tarp_ctrl.param = 0xB1; // level 2, rec 3, slack 0, fwd on
		tarp_ctrl.param = 0xA2; // level 2, rec 2, slack 1, fwd off(!?)
		//we're departing from all forwarders beacause of flooding
		//alrms from treasure tags (only they forward)

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

		if (local_host != 0xDEAD &&
				(profi_att & PROF_KIOSK) == 0 &&
				(profi_att & PROF_TAG) == 0)
			runfsm mbeacon;

		if (profi_att & PROF_TAG) {
			highlight_set (0, 0.0, NULL);
			strcpy (d_alrm, "!TT!");
			runfsm abeacon (0);
			tarp_ctrl.param |= 1; // fwd ON
		} else {
			if (profi_att & PROF_KIOSK) {
				strcpy (d_alrm, "!KI!&Combat R&D Booth");
				runfsm abeacon (0);
			}
		}

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
			case 'a': proceed RS_ABEAC;
			case 'H': proceed RS_HUNT;
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

		if (w1 < 2 || w2 > 2 && cmd_line[2] != ' ' &&
				cmd_line[2] != '&' && cmd_line[2] != '|') {
			form (ui_obuf, ill_str, cmd_line);
			proceed RS_UIOUT;
		}

		switch (cmd_line[1]) {

		  case 'p':
			if (scan (cmd_line +3, "%x", &w1) > 0) {
				if (cmd_line[2] == ' ') {
					profi_att = w1;
				} else if (cmd_line[2] == '|') {
					profi_att |= w1;
				} else {
					profi_att &= ~w1;
				}
			}
			form (ui_obuf, "Profile: %x\r\n", profi_att);
			proceed RS_UIOUT;

		  case 'e':
			if (scan (cmd_line +3, "%x", &w1) > 0) {
				if (cmd_line[2] == ' ') {
					p_exc = w1;
				} else if (cmd_line[2] == '|') {
					p_exc |= w1;
				} else {
					p_exc &= ~w1;
				}
			}
			form (ui_obuf, "Exclude: %x\r\n", p_exc);
			proceed RS_UIOUT;

		  case 'i':
			if (scan (cmd_line +3, "%x", &w1) > 0) {
				if (cmd_line[2] == ' ') {
					p_inc = w1;
				} else if (cmd_line[2] == '|') {
					p_inc |= w1;
				} else {
					p_inc &= ~w1;
				}
			}
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

	state RS_HUNT:
		if (scan (cmd_line +1, "%u", &w1) > 0) {
			if (w1) {
				profi_att |= PROF_HUNT;
				p_exc &= ~PROF_TAG;
				hunt_ind = 0;
				shuffle_hunt (&hunt[0]);
				hstart = seconds();
			} else {
				profi_att &= ~PROF_HUNT;
				p_exc |= PROF_TAG;
				killall (abeacon); // no mercy for others
			}
		}
		form (ui_obuf, "HUNT O%s\r\n", (profi_att & PROF_HUNT) ?
				"N" : "FF");
		proceed RS_UIOUT;

	state RS_ABEAC:
		w2 = 0;
		if (scan (cmd_line +1, "%u %u", &w1, &w2) > 0) {
			if (w1) {
				if (!running (abeacon)) {
					runfsm abeacon(w2);
				}
						
			} else {
				if (running (abeacon)) {
					killall (abeacon);
				}
			}
		}
		form (ui_obuf, "Alarm beacon(%u)  O%s\r\n", w2,
			       running (abeacon) ?  "N" : "FF");
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

