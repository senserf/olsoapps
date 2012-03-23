/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "msg_col.h"
#include "flash_stamps.h"
#include "hold.h"
#include "app_col_data.h"
#include "sensets.h"
//#include "board_pins.h"

// PiComp
//
// Note: this is still how you declare which program files belong to
// the set; can be here or in an included header

//+++ app_diag_col.cc "lib_app_col.cc" <msg_io_col.cc>

#include "sensors.h"

// PiComp
//
// This is explained in app_peg.cc
//
//const lword	host_id	= (lword) PREINIT (0xBACADEAD, "HID");
//+++ hostid.cc

lword		ref_ts;
lint		ref_date;
lint		lh_time;

sensDatumType	sens_data;

pongParamsType	pong_params = { 30, 5, 0x7531, 2048, 0, 0 };

static char		* cmd_line;

word 			app_flags = DEF_APP_FLAGS;
word			plot_id;

word			buttons;

idiosyncratic int tr_offset (headerType*);

idiosyncratic Boolean msg_isBind (msg_t m), msg_isTrace (msg_t m),
		      msg_isMaster (msg_t m), msg_isNew (msg_t m),
		      msg_isClear (byte o);

idiosyncratic void set_master_chg (void);

idiosyncratic word guide_rtr (headerType * b);

#include "diag.h"
#include "str_col.h"

/*
   On CHRONOS, no UART and related functionlity, added true hw i/f.
   Elsewhere, VUEE uncluded, i/o as was, plus faked lcd & buttons.
*/
#ifndef BOARD_CHRONOS
#error CHRONOS only
#endif

#ifndef __SMURPH__
#define ser_out(a, b)	CNOP
#define ser_outb(a,b)	ufree(b)
#else
#include "ser.h"
#endif

#include "chro_col.h"
#include "form.h"
#include "net.h"

// elsewhere may be a better place for this:
#if CC1000
#define INFO_PHYS_DEV INFO_PHYS_CC1000
#endif

#if CC1100
#define	INFO_PHYS_DEV INFO_PHYS_CC1100
#endif

#if DM2200
#define INFO_PHYS_DEV INFO_PHYS_DM2200
#endif

#ifndef INFO_PHYS_DEV
#error "UNDEFINED RADIO"
#endif

#define DEF_NID	85

// arbitrary
#define UI_BUFLEN		UART_INPUT_BUFFER_LENGTH

#include "dconv.ch"

// =============
// OSS reporting
// =============
fsm oss_out (char*) {
    entry OO_RETRY:
	ser_outb (OO_RETRY, data);
	trigger (OSS_DONE);
	finish;
}

void fatal_err_t (word err, word w1, word w2, word w3) {
	//leds (LED_R, LED_BLINK);
	if_write (IFLASH_SIZE -1, err);
	if_write (IFLASH_SIZE -2, w1);
	if_write (IFLASH_SIZE -3, w2);
	if_write (IFLASH_SIZE -4, w3);
	if (err != ERR_MAINT) {
		app_diag_t (D_FATAL, "HALT %x %u %u %u", err, w1, w2, w3);
		halt();
	}
	reset();
}

static void init () {
	cma_3000_on (0); // for now, we'll see if low sensitivity is enough
	ezlcd_init ();
	ezlcd_on ();
	chro_lo ("ILIAD");
	chro_nn (1, local_host);

	memset (&sens_data, 0, sizeof (sensDatumType));
	sens_data.stat = SENS_FF;
	powerdown();

}

static void show_ifla_t () {

	char * mbuf = NULL;

	if (if_read (0) == 0xFFFF) {
		diag (OPRE_APP_ACK "No custom sys data");
		return;
	}
	mbuf = form (NULL, ifla_str, if_read (0), if_read (1), if_read (2),
			if_read (3), if_read (4), if_read (5));

	if (runfsm oss_out (mbuf) == 0) {
		app_diag_t (D_SERIOUS, "oss_out fork");
		ufree (mbuf);
	}
}

static void read_ifla_t () {
	if (if_read (0) == 0xFFFF) { // usual defaults
		local_host = (word)host_id;
		return;
	}

	local_host = if_read (0);
	pong_params.pow_levels = if_read (1);
	app_flags = if_read (2);
	pong_params.freq_maj = if_read (3);
	pong_params.freq_min = if_read (4);
	pong_params.rx_span = if_read (5);
}

static void save_ifla_t () {

	if (if_read (0) != 0xFFFF) {
		if_erase (0);
		diag (OPRE_APP_ACK "Flash p0 overwritten");
	}
	// there is 'show' after 'save'... don't check if_writes here (?)
	if_write (0, local_host);
	if_write (1, pong_params.pow_levels);
	if_write (2, (app_flags & 0xFFFC)); // off synced, mster chg
	if_write (3, pong_params.freq_maj);
	if_write (4, pong_params.freq_min);
	if_write (5, pong_params.rx_span);
}

// Display node stats on UI
static void stats () {

	char * mbuf;
	word w[2];
	w[0] = memfree (0, &w[1]);

	mbuf = form (NULL, stats_str,
			(word)host_id, local_host, 
			pong_params.freq_maj, pong_params.freq_min,
			pong_params.rx_span, pong_params.pow_levels,
			handle_c_flags (0xFFFF), seconds(),
			w[0], w[1]);
	if (runfsm oss_out (mbuf) == 0) {
		app_diag_t (D_SERIOUS, "oss_out fork");
		ufree (mbuf);
	}
}

void process_incoming (word state, char * buf, word size, word rssi) {

  sint w_len;

  switch (in_header(buf, msg_type)) {

	case msg_setTag:
		msg_setTag_in (buf, rssi);
		return;

	case msg_rpc:
		if (cmd_line != NULL) { // busy with another input
			when (CMD_WRITER, state);
			release;
		}
		w_len = strlen (&buf[sizeof (headerType)]) +1;

		// sanitize
		if (w_len + sizeof (headerType) > size)
			return;

		cmd_line = get_mem_t (state, w_len);
		strcpy (cmd_line, buf + sizeof(headerType));
		trigger (CMD_READER);
		return;

	case msg_pongAck:
		msg_pongAck_in (buf, rssi);

	default:
		return;
  }
}

/*
   --------------------
   Receiver process
   RS_ <-> Receiver State
   --------------------
*/

// In this model, a single rcv is forked once, and runs/sleeps all the time
// Toggling rx happens in the rxsw process, driven from the pong process.
fsm rcv {

	char *buf;
	sint psize;
	word rssi;

	entry RC_TRY:

		if (buf) {
			ufree (buf);
			buf = NULL;
			psize = 0;
		}
		psize = net_rx (RC_TRY, &buf, &rssi, 0);
		if (psize <= 0) {
			app_diag_t (D_SERIOUS, "net_rx failed (%d)", psize);
			proceed RC_TRY;
		}

		app_diag_t (D_DEBUG, "RCV (%d): %x-%u-%u-%u-%u-%u\r\n",
			  psize, in_header(buf, msg_type),
			  in_header(buf, seq_no) & 0xffff,
			  in_header(buf, snd),
			  in_header(buf, rcv),
			  in_header(buf, hoc) & 0xffff,
			  in_header(buf, hco) & 0xffff);

	entry RC_MSG:

		process_incoming (RC_MSG, buf, psize,
				(rssi & 0xff00) ? (rssi >> 8) : 1);
		proceed RC_TRY;
}

fsm lcd_delay {
	state S_INI:
		delay (15000, S_OFF);
		release;
	state S_OFF:
		ezlcd_off();
		finish;
}

fsm rxsw {

	entry RX_OFF:

		// dupa CHRONOS chokes with RXOFF - MUST be investigated
		net_opt (PHYSOPT_RXOFF, NULL);
		net_diag_t (D_DEBUG, "Rx off %x",
				net_opt (PHYSOPT_STATUS, NULL));
		when (RX_SW_ON, RX_ON);
		release;

	entry RX_ON:

		net_opt (PHYSOPT_RXON, NULL);
		net_diag_t (D_DEBUG, "Rx on %x", net_opt(PHYSOPT_STATUS, NULL));
		if (pong_params.rx_span == 0) {
			app_diag_t (D_SERIOUS, "Bad rx span");
			proceed RX_OFF;
		}
		delay ( pong_params.rx_span, RX_OFF);
		release;
}

// This one will be fine with SMURPH as well as PicOS
static word map_level (word l) {
#if CC1000
	switch (l) {
		case 2:
			return 2;
		case 3:
			return 3;
		case 4:
			return 4;
		case 5:
			return 5;
		case 6:
			return 6;
		case 7:
			return 0x0A; // -4 dBm
		case 8:
			return 0x0F; // 0 dBm
		case 9:
			return 0x60; // 4 dBm
		case 10:
			return 0x70;
		case 11:
			return 0x80;
		case 12:
			return 0x90;
		case 13:
			return 0xC0;
		case 14:
			return 0xE0;
		case 15:
			return 0xFF;
		default:
			return 1; // -20 dBm
	}
#else
	return l < 7 ? l : 7;
#endif
}

/*
  --------------------
  Pong process: spontaneous ping a rebours
  PS_ <-> Pong State 
  --------------------

  This model descends from TNP blueprint, but is becoming more and more
  inadequte for sensors (payload here), especially if they are controlled
  remotely (sync, acks, nacks). Likely, the sens process should be in a constant
  loop queuing results and calling utility routines to communicate, write
  to eeprom, etc. EcoNet 2.0, after we have a chance to gather credible
  requirements.

*/

fsm pong {

	word shift;
	char frame [sizeof (msgPongType) + sizeof (pongPloadType)];

	entry PS_INIT:

		in_header(frame, msg_type) = msg_pong;
		in_header(frame, rcv) = 0;
		// we do not want pong msg multihopping
		in_header(frame, hco) = 1;

		in_pong (frame, setsen) = senset;

	entry PS_NEXT:

		if (local_host == 0 || pong_params.freq_maj == 0) {
			app_diag_t (D_WARNING, "Pong's suicide");
			finish;
		}
		shift = 0;

		// not sure this is a good idea... spread pongs:
		delay (rnd() % 100, PS_SEND);
		release;

	entry PS_SEND:

		word   level, w;

		level = ((pong_params.pow_levels >> shift) & 0x000f);
		// pong with this power if data collected (and not confirmed)
		if (level > 0 && sens_data.stat == SENS_COLLECTED) {
			net_opt (PHYSOPT_TXON, NULL);
			net_diag_t (D_DEBUG, "Tx on %x",
					net_opt (PHYSOPT_STATUS, NULL));
			in_pong (frame, level) = level;
			in_pong (frame, freq) = pong_params.freq_maj;
			in_pong (frame, flags) = 0; // reset flags

			// this got complicated: we're at appropriate plev, and
			// we're toggling rx on/off, or it is permanently ON
			// rx is ON after all tx ot this is the level
			// and either we're toggling or rx is ON permanently
			if ((pong_params.rx_lev == 0 ||
					level == pong_params.rx_lev) &&
			    ((w = running (rxsw)) ||
			     pong_params.rx_span == 1)) {
				in_pong (frame, flags) |= PONG_RXON;
				if (w) // means running (rxsw)
					trigger (RX_SW_ON);
			
			}

			if (pong_params.rx_span == 1) // always on
				in_pong (frame, flags) |= PONG_RXPERM;

			w = map_level (level);
			net_opt (PHYSOPT_SETPOWER, &w);

			if (pong_params.pload_lev == 0 ||
					level == pong_params.pload_lev) {
				in_pong (frame, pstatus) =
					sens_data.stat;

				// MAX_SENS may be > NUM_SENS, just
				// zero the crap
				memset (in_pongPload (frame, sval),
						0, MAX_SENS << 1);
				memcpy (in_pongPload (frame, sval),
						sens_data.sval,
						NUM_SENS << 1);
				in_pongPload (frame, ds) =
					sens_data.ds;
				in_pongPload (frame, eslot) =
					0; // sens_data.eslot;
				in_pong (frame, flags) |= PONG_PLOAD;
				send_msg_t (frame, sizeof(msgPongType) +
						sizeof(pongPloadType));

			} else {
				send_msg_t (frame, sizeof(msgPongType));
			}

			app_diag_t (D_DEBUG, "Pong on level %u->%u", level, w);
			net_opt (PHYSOPT_TXOFF, NULL);
			net_diag_t (D_DEBUG, "Tx off %x",
					net_opt (PHYSOPT_STATUS, NULL));

			when (ACK_IN, PS_SEND1);

		} else {
			if (level == 0)
				app_diag_t (D_DEBUG, "skip level");

			if (sens_data.stat != SENS_CONFIRMED) {
				app_diag_t (D_DEBUG, "sens not ready %u",
						sens_data.stat);
			} else {
				leds (LED_B, LED_OFF);
				next_col_time ();
				if (lh_time <= 0 || is_alrms)
					proceed PS_SENS;

				lh_time += seconds();
				proceed PS_HOLD;
			}
		}
		delay (pong_params.freq_min << 10, PS_SEND1);
		release;

	entry PS_SEND1:
		if ((shift += 4) < 16)
			proceed PS_SEND;

		leds (LED_B, LED_BLINK);
		chro_lo ("RONIN");
		next_col_time ();
		if (lh_time <= 0 || is_alrms)
			proceed PS_SENS;

		lh_time += seconds();

	entry PS_HOLD:
		when (ALRMS, PS_SENS);

		// lh_time was patched over patches...
		hold (PS_HOLD, (lword)lh_time);


		// fall through

	entry PS_SENS:
		if (running (sens)) {
			app_diag_t (D_SERIOUS, "Reg. sens delayed");
			delay (SENS_COLL_TIME, PS_COLL);
			when (SENS_DONE, PS_COLL);
			release;
		}

	entry PS_COLL:
		if (running (sens)) { 
			app_diag_t (D_SERIOUS, "Mercy sens killing");
			killall (sens);
		}
		runfsm sens;
		delay (SENS_COLL_TIME, PS_NEXT);
		when (SENS_DONE, PS_NEXT);
		release;

}

#define BATTERY_WARN	2250

fsm sens {

	word w[3];

	entry SE_INIT:
		switch (sens_data.stat) {
		  case SENS_IN_USE:
			app_diag_t (D_SERIOUS, "Sens in use");
			break;

		  case SENS_COLLECTED:
			app_diag_t (D_INFO, "Not confirmed");
		}

		sens_data.stat = SENS_IN_USE;

		sens_data.ds = wall_date_t (0);
		lh_time = seconds();

	entry SE_0:
#ifdef SENSOR_LIST
		read_sensor (SE_0, -2, &sens_data.sval[0]);
	entry SE_1:
		read_sensor (SE_1, -1, &sens_data.sval[1]);
	entry SE_2: // motion
		read_sensor (SE_2, 0,  &sens_data.sval[2]);
	entry SE_3: // temp, pressure
		read_sensor (SE_3, 1, w);
#ifndef __SMURPH__
		sens_data.sval[3] = w[2];
		sens_data.sval[4] = (word)(*((long*)w) >> 3); // 19 to 16 bits
#else
		sens_data.sval[3] = w[0];
	entry SE_4: // fake pressure
		 read_sensor (SE_4, 2, &sens_data.sval[4]);
#endif
#else
		sint i;
		app_diag_t (D_WARNING, "FAKE SENSORS");

		for (i = 0; i < NUM_SENS; i++)
			sens_data.sval[i] += i;

		delay (SENS_COLL_TIME, SE_DONE);
		release;
#endif
	entry SE_DONE:
		sens_data.stat = SENS_COLLECTED;

		// kludge alert: sens[5] - buttons
		sens_data.sval[5] = buttons;
		clr_alrms;

		//leds (LED_R, LED_BLINK);
		trigger (SENS_DONE);
		finish;
}
#undef BATTERY_WARN

// b < 5 <-> button, else b is event
void do_butt (word b) {
	// button to event translation: don't allow button #0 with acc-meter
	// in 'full events' sent from the master
	word ev = b < 5 ? (1 << b) : b & 0xfffe;

	// sleep / reset button #4
	if (ev & (1 << 4)) {
		chro_xx (1, ev);
		if (buttons & (1 << 4)) {
			ezlcd_on();
			chro_lo ("RESET");
			reset();
		}
		chro_lo ("SLEEP");
		cma_3000_off ();
		runfsm lcd_delay;
		net_opt (PHYSOPT_RXOFF, NULL);
		net_opt (PHYSOPT_TXOFF, NULL);
		buttons = ev;

		killall (rxsw);
		killall (pong);
		killall (sens);
		killall (pong);
		killall (rcv); // keep last: this fsm can call do_butt()!!

		return;
	}

	if (ev & 1) {
		if (buttons & 1)
			cma_3000_on (0);
		else
			cma_3000_on (1);
	}

	if (ev > (1<<4)) { // external event: note that we always trigger alrm
		if (buttons & 0x1F & ev) {
			chro_lo ("IGNOR");
		} else {
			buttons = ev;
		}

	} else { // button (internal or external)
		//flip
		buttons = buttons & ev ? 
			buttons & ~ev : buttons | ev;
	}

	if (!(buttons & 0x1F)) // all buttons 0-4 cleared
		buttons = 0; // clear events
	trigger (ALRMS);
	set_alrms;
	chro_xx (1, buttons);
	chro_xx (0, ev);
}

#ifdef __SMURPH__
/*
   --------------------
   cmd_in process
   CS_ <-> Command State
   --------------------
*/

fsm cmd_in {

	char *ibuf;

	entry CS_INIT:

		if (ibuf == NULL)
			ibuf = get_mem_t (CS_INIT, UI_BUFLEN);

		memset (ibuf, 0, UI_BUFLEN);

	entry CS_IN:
		// hangs on the uart_a interrupt or polling
		ser_in (CS_IN, ibuf, UI_BUFLEN);
		if (strlen(ibuf) == 0) // CR on empty line does it
			proceed CS_IN;

	entry CS_WAIT:

		if (cmd_line != NULL) {
			when (CMD_WRITER, CS_WAIT);
			release;
		}

		cmd_line = get_mem_t (CS_WAIT, strlen(ibuf) +1);
		strcpy (cmd_line, ibuf);
		trigger (CMD_READER);
		proceed CS_IN;
}
#endif

/*
   --------------------
   Root process
   RS_ <-> Root State
   --------------------
*/

fsm root {

	char *obuf;

	entry RS_INIT:
		obuf = get_mem_t (RS_INIT, UI_BUFLEN);

		if (if_read (IFLASH_SIZE -1) != 0xFFFF)
			leds (LED_R, LED_BLINK);
		else
			leds (LED_G, LED_BLINK);
#ifndef __SMURPH__
#if CRYSTAL2_RATE
#error CRYSTAL2 RATE MUST BE 0, UART_RATE 9600
#endif
#endif
		if (is_flash_new) {
			diag (OPRE_APP_ACK "Init erase");
			break_flash;
		} else {
			delay (5000, RS_INIT1);
			release;
		}

	entry RS_INIT1:
		ser_out (RS_INIT1, obuf);
		master_host = local_host;
		read_ifla_t();
		init();

		if (if_read (IFLASH_SIZE -1) != 0xFFFF) {
			leds (LED_R, LED_OFF);
			diag (OPRE_APP_MENU_C "***Maintenance mode***"
				OMID_CRB "%x %u %u %u",
				if_read (IFLASH_SIZE -1),
				if_read (IFLASH_SIZE -2),
				if_read (IFLASH_SIZE -3),
				if_read (IFLASH_SIZE -4));
#ifdef __SMURPH__
			if (!running (cmd_in))
				runfsm cmd_in;
#endif
			stats();
			proceed RS_RCMD;
		}
		leds (LED_G, LED_OFF);

	entry RS_INIT2:
		ser_out (RS_INIT2, welcome_str);

		net_id = DEF_NID;
		tarp_ctrl.param &= 0xFE; // routing off
		runfsm sens;

		// give sensors time & spread a bit
		delay (SENS_COLL_TIME + rnd() % 444, RS_PAUSE);
		release;

	entry RS_PAUSE:

		if (net_init (INFO_PHYS_DEV, INFO_PLUG_TARP) < 0) {
			app_diag_t (D_FATAL, "net_init failed");
			reset();
		}
		net_opt (PHYSOPT_SETSID, &net_id);
		net_opt (PHYSOPT_TXOFF, NULL);

		runfsm rcv;
		switch (pong_params.rx_span) {
			case 0:
			case 2:
				net_opt (PHYSOPT_RXOFF, NULL);
				break;
			case 1:
				break; // ON from net_init
			default:
				runfsm rxsw;
		}

		if (local_host)
			runfsm pong;
#ifndef __SMURPH__
		buttons_action (do_butt);
		finish;
#else
		runfsm cmd_in;
#endif
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

		sint i1, i2, i3, i4, i5;

		if (cmd_line[0] == ' ') // ignore if starts with blank
			proceed RS_FREE;

                if (cmd_line[0] == 'h') {
			ser_out (RS_DOCMD, welcome_str);
			proceed RS_FREE;
		}

		if (cmd_line[0] == 'q')
			reset();

		if (cmd_line[0] == 'M') {
			if (if_read (IFLASH_SIZE -1) != 0xFFFF) {
				diag (OPRE_APP_ACK "Already in maintenance");
				reset();
			}
			fatal_err_t (ERR_MAINT, (word)(seconds() >> 16),
					(word)(seconds()), 0);
			// will reset
		}

		if (cmd_line[0] == 'F') {
			if_erase (IFLASH_SIZE -1);
			break_flash;
			diag (OPRE_APP_ACK "flash p1 erased");
			reset();
		}

		if (cmd_line[0] == 'Q') {
			if_erase (-1);
			break_flash;
			diag (OPRE_APP_ACK "all erased");
			reset();
		}

		if (cmd_line[0] == 's') {

			i1 = i2 = i3 = i4 = i5 = -1;

			// Maj, min, rx_span, pow_level
			scan (cmd_line+1, "%d %d %d %x %x",
				       	&i1, &i2, &i3, &i4, &i5);

			// we follow max power, but likely rx and pload levels
			// should be independent
			if (i4 >= 0) {
#if 0
				if (i4 > 7)
					i4 = 7;
				i4 |= (i4 << 4) | (i4 << 8) | (i4 << 12);
#endif
				if (pong_params.rx_lev != 0) // 'all' stays
					pong_params.rx_lev = max_pwr(i4);
				
				if (pong_params.pload_lev != 0)
					pong_params.pload_lev =
						pong_params.rx_lev;
				
				pong_params.pow_levels = i4;
			}

			(void)handle_c_flags ((word)i5);

			if (i1 >= 0) {
				pong_params.freq_maj = i1;
				if (pong_params.freq_maj != 0 &&
						!running (pong))
					runfsm pong;
			}

			if (i2 >= 0)
				pong_params.freq_min = i2;

			if (i3 >= 0 && pong_params.rx_span != i3) {
				pong_params.rx_span = i3;
				
				switch (i3) {
					case 0:
						killall (rxsw);
						net_opt (PHYSOPT_RXOFF, NULL);
						break;
					case 1:
						killall (rxsw);
						net_opt (PHYSOPT_RXON, NULL);
						break;
					default:
						if (!running (rxsw))
							runfsm rxsw;
				}
			}

			stats ();
			proceed RS_FREE;
		}

		if (cmd_line[0] == 'S') {
			if (cmd_line[1] == 'A')
				save_ifla_t();
			show_ifla_t();
			proceed RS_FREE; 
		}

		if (cmd_line[0] == 'I') {
			if (cmd_line[1] == 'D') {
				i1 = -1;
				scan (cmd_line +2, "%d", &i1);
				if (i1 > 0) {
					local_host = i1;
				}
			}
			stats ();
			proceed RS_FREE;
		}

		if (cmd_line[0] == 'B') {
			i1 = -1;
			scan (cmd_line +1, "%d", &i1);
			if (i1 >= 0)
				do_butt (i1);
			else
				diag ("BUTTONS: %x", buttons);
		}
		proceed RS_FREE;

		form (obuf, ill_str, cmd_line);

	entry RS_UIOUT:

		ser_out (RS_UIOUT, obuf);
		proceed RS_FREE;

}
