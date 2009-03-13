/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "globals_tag.h"
#include "threadhdrs_tag.h"
#include "flash_stamps.h"

#ifndef __SMURPH__
#include "lhold.h"
#include "board_pins.h"
#include "sensors.h"
#else
#define SENSOR_LIST alamakota
#define powerdown() 	diag ("pdown");
#define powerup()	diag ("pup");
#endif

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

// Semaphore for command line
#define CMD_READER	(&cmd_line)
#define CMD_WRITER	((&cmd_line)+1)
#define SENS_DONE	(&lh_time)

// rx switch control
#define RX_SW_ON	(&pong_params.rx_span)

// =============
// OSS reporting
// =============
strand (oss_out, char)
    entry (OO_RETRY)
	ser_outb (OO_RETRY, data);
	finish;
endstrand

__PUBLF (NodeTag, void, fatal_err) (word err, word w1, word w2, word w3) {
	//leds (LED_R, LED_BLINK);
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

// will go away when we can see processes from other files
__PUBLF (NodeTag, void, tmpcrap) (word what) {
	switch (what) {
		case 0:
			if (!running(rxsw))
				runthread (rxsw);
			break;
		case 1:
			if (!running(pong))
				runthread (pong);
			break;
		case 2:
			killall (rxsw);
			break;
		default:
			app_diag (D_SERIOUS, "Crap");
	}
}

__PUBLF (NodeTag, void, sens_init) () {
	lword l, u, m; 
	byte b; 

	if (ee_read (EE_SENS_MIN * EE_SENS_SIZE, &b, 1))
		fatal_err (ERR_EER, 0, 0, 1);

	memset (&sens_data, 0, sizeof (sensDataType));
	if (b == 0xFF) { // normal operations (empty eeprom)
		sens_data.eslot = EE_SENS_MIN;
		sens_data.ee.s.f.status = SENS_FF;
		sens_data.ee.s.f.emptym = ee_emptym ? 1 : 0;
		sens_data.ee.s.f.spare = 7;
		return;
	}

	// only efter power down / soft reset there could be anything in eeprom
	l = EE_SENS_MIN; u = EE_SENS_MAX;
	while ((u - l) > 1) {
		m = l + ((u -l) >> 1);
		if (ee_read (m * EE_SENS_SIZE, &b, 1))
			fatal_err (ERR_EER, (word)((m * EE_SENS_SIZE) >> 16),
				(word)(m * EE_SENS_SIZE), 1);

		if (b == 0xFF)
			u = m;
		else
			l = m;
	}

	if (ee_read (u * EE_SENS_SIZE, &b, 1))
		fatal_err (ERR_EER, (word)((u * EE_SENS_SIZE) >> 16),
			(word)(u * EE_SENS_SIZE), 1);

	if (b == 0xFF) {
		if (l < u) {
			if (ee_read (l * EE_SENS_SIZE, &b, 1))
				fatal_err (ERR_EER,
				       (word)((l * EE_SENS_SIZE) >> 16),
				       (word)(l * EE_SENS_SIZE), 1);

			if (b == 0xFF)
				fatal_err (ERR_SLOT, (word)(l >> 16),
					(word)l, 0);

			sens_data.eslot = l;
		}
	} else
		sens_data.eslot = u;

	// collectors start sensing 1st, so don't read and point at 1st empty
	sens_data.eslot++;
	sens_data.ee.s.f.status = SENS_FF;
	sens_data.ee.s.f.emptym = ee_emptym ? 1 : 0;
	sens_data.ee.s.f.spare = 7;
#if 0
	if (ee_read (sens_data.eslot * EE_SENS_SIZE, (byte *)&sens_data.ee,
			EE_SENS_SIZE))
		fatal_err (ERR_EER,
			(word)((sens_data.eslot * EE_SENS_SIZE) >> 16),
			(word)(sens_data.eslot * EE_SENS_SIZE), EE_SENS_SIZE);
#endif
}


__PUBLF (NodeTag, void, init) () {

	sens_init();

// put all the crap here

}

// little helper
static const char * statusName (statu_t s) {
	switch (s.f.status) {
		case SENS_CONFIRMED: 	return "CONFIRMED";
		case SENS_COLLECTED: 	return "COLLECTED";
		case SENS_IN_USE: 	return "IN_USE";
		case SENS_FF: 		return "FFED";
		case SENS_ALL: 		return "ALL";
	}
	app_diag (D_SERIOUS, "unexpected eeprom %x", s);
	return				"really ffed";
}

__PUBLF (NodeTag, word, r_a_d) () {
	char * lbuf = NULL;

	if (sens_dump->dfin) // delayed Finish 
		goto ThatsIt;

	if (ee_read (sens_dump->ind * EE_SENS_SIZE, (byte *)&sens_dump->ee,
				EE_SENS_SIZE)) {
		app_diag (D_SERIOUS, "Failed ee_read");
		goto Finish; 
	}

	if (sens_dump->ee.s.f.status == SENS_FF) {
		if (sens_dump->fr <= sens_dump->to) {
			goto Finish;
		} else {
			goto Continue;
		}
	}

	if (sens_dump->s.f.status == SENS_ALL ||
			sens_dump->s.f.status == sens_dump->ee.s.f.status) {
		lbuf = form (NULL, dump_str,
			statusName (sens_dump->ee.s), sens_dump->ind,
			((mclock_t *)&sens_dump->ee.ts)->hms.f ?
				"time" : "ts",
			((mclock_t *)&sens_dump->ee.ts)->hms.d,
			((mclock_t *)&sens_dump->ee.ts)->hms.h,
			((mclock_t *)&sens_dump->ee.ts)->hms.m,
			((mclock_t *)&sens_dump->ee.ts)->hms.s,
			sens_dump->ee.sval[0],
			sens_dump->ee.sval[1],
			sens_dump->ee.sval[2],
			sens_dump->ee.sval[3],
			sens_dump->ee.sval[4]);

		if (runstrand (oss_out, lbuf) == 0 ) {
			app_diag (D_SERIOUS, "oss_out failed");
			ufree (lbuf);
		}

		sens_dump->cnt++;

		if (sens_dump->upto != 0 && sens_dump->upto <= sens_dump->cnt)
			goto Finish;
	}

Continue:
	if (sens_dump->fr <= sens_dump->to) {
		if (sens_dump->ind >= sens_dump->to)
			goto Finish;
		else
			sens_dump->ind++;
	} else {
		if (sens_dump->ind <= sens_dump->to)
			goto Finish;
		else
			sens_dump->ind--;
	}
	return 1;

Finish:
	// ser_out tends to switch order... delay the output
	sens_dump->dfin = 1;
	return 1;

ThatsIt:
	sens_dump->dfin = 0; // just in case
	lbuf = form (NULL, dumpend_str,
			local_host, sens_dump->fr, sens_dump->to,
			statusName (sens_dump->s), 
			sens_dump->upto, sens_dump->cnt);

	if (runstrand (oss_out, lbuf) == 0 ) {
		app_diag (D_SERIOUS, "oss_out sum failed");
		ufree (lbuf);
	}
	return 0;
}

__PUBLF (NodeTag, void, show_ifla) () {
	char * mbuf = NULL;

	if (if_read (0) == 0xFFFF) {
		diag (OPRE_APP_ACK "No custom sys data");
		return;
	}
	mbuf = form (NULL, ifla_str, if_read (0), if_read (1), if_read (2),
			if_read (3), if_read (4), if_read (5));

	if (runstrand (oss_out, mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out fork");
		ufree (mbuf);
	}
}

__PUBLF (NodeTag, void, read_ifla) () {
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

__PUBLF (NodeTag, void, save_ifla) () {
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
__PUBLF (NodeTag, void, stats) () {
	char * mbuf;
	word mmin;
	word mem = memfree(0, &mmin);

	mbuf = form (NULL, stats_str,
			host_id, local_host, 
			pong_params.freq_maj, pong_params.freq_min,
			pong_params.rx_span, pong_params.pow_levels,
			handle_c_flags (0xFFFF), seconds(),
			sens_data.eslot == EE_SENS_MIN &&
			  sens_data.ee.s.f.status == SENS_FF ?
			0 : sens_data.eslot - EE_SENS_MIN +1,
			mem, mmin);
	if (runstrand (oss_out, mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out fork");
		ufree (mbuf);
	}
}

__PUBLF (NodeTag, void, process_incoming) (word state, char * buf, word size) {

  int    	w_len;
  
  switch (in_header(buf, msg_type)) {

	case msg_setTag:
		msg_setTag_in (buf);
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

	case msg_pongAck:
		msg_pongAck_in (buf);
		return;

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
thread (rcv)

	entry (RC_INIT)

		rcv_packet_size = 0;
		rcv_buf_ptr	= NULL;

	entry (RC_TRY)

		if (rcv_buf_ptr != NULL) {
			ufree (rcv_buf_ptr);
			rcv_buf_ptr = NULL;
			rcv_packet_size = 0;
		}
		rcv_packet_size = net_rx (RC_TRY, &rcv_buf_ptr, NULL, 0);
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

	entry (RC_MSG)

		process_incoming (RC_MSG, rcv_buf_ptr, rcv_packet_size);
		proceed (RC_TRY);
endthread

thread (rxsw)

	entry (RX_OFF)

		net_opt (PHYSOPT_RXOFF, NULL);
		net_diag (D_DEBUG, "Rx off %x", net_opt (PHYSOPT_STATUS, NULL));
		when (RX_SW_ON, RX_ON);
		release;

	entry (RX_ON)

		net_opt (PHYSOPT_RXON, NULL);
		net_diag (D_DEBUG, "Rx on %x", net_opt (PHYSOPT_STATUS, NULL));
		if (pong_params.rx_span == 0) {
			app_diag (D_SERIOUS, "Bad rx span");
			proceed (RX_OFF);;
		}
		delay ( pong_params.rx_span, RX_OFF);
		release;

endthread

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

thread (pong)

	word	level, w;

	entry (PS_INIT)

		in_header(png_frame, msg_type) = msg_pong;
		in_header(png_frame, rcv) = 0;
		// we do not want pong msg multihopping
		in_header(png_frame, hco) = 1;

	entry (PS_NEXT)

		if (local_host == 0 || pong_params.freq_maj == 0) {
			app_diag (D_WARNING, "Pong's suicide");
			finish;
		}
		png_shift = 0;

		// not sure this is a good idea... spread pongs:
		delay (rnd() % 5000, PS_SEND);
		release;

	entry (PS_SEND)

		level = ((pong_params.pow_levels >> png_shift) & 0x000f);
		// pong with this power if data collected (and not confirmed)
		if (level > 0 && sens_data.ee.s.f.status == SENS_COLLECTED) {
			net_opt (PHYSOPT_TXON, NULL);
			net_diag (D_DEBUG, "Tx on %x",
					net_opt (PHYSOPT_STATUS, NULL));
			in_pong (png_frame, level) = level;
			in_pong (png_frame, freq) = pong_params.freq_maj;
			in_pong (png_frame, flags) = 0; // reset flags

			// this got complicated: we're at appropriate plev, and
			// we're toggling rx on/off, or it is permanently ON
			// rx is ON after all tx ot this is the level
			// and either we're toggling or rx is ON permanently
			if ((pong_params.rx_lev == 0 ||
					level == pong_params.rx_lev) &&
			    ((w = running (rxsw)) ||
			     pong_params.rx_span == 1)) {
				in_pong (png_frame, flags) |= PONG_RXON;
				
				if (w) // means running (rxsw)
					trigger (RX_SW_ON);
			
			}

			if (pong_params.rx_span == 1) // always on
				in_pong (png_frame, flags) |= PONG_RXPERM;

			w = map_level (level);
			net_opt (PHYSOPT_SETPOWER, &w);

			if (pong_params.pload_lev == 0 ||
					level == pong_params.pload_lev) {
				in_pong (png_frame, pstatus) =
					sens_data.ee.s.f.status;
				memcpy (in_pongPload (png_frame, sval),
						sens_data.ee.sval,
						NUM_SENS << 1);
				in_pongPload (png_frame, ts) =
					sens_data.ee.ts;
				in_pongPload (png_frame, eslot) =
					sens_data.eslot;
				in_pong (png_frame, flags) |= PONG_PLOAD;
				send_msg (png_frame, sizeof(msgPongType) +
						sizeof(pongPloadType));

			} else
				send_msg (png_frame, sizeof(msgPongType));

			app_diag (D_DEBUG, "Pong on level %u->%u", level, w);
			net_opt (PHYSOPT_TXOFF, NULL);
			net_diag (D_DEBUG, "Tx off %x",
					net_opt (PHYSOPT_STATUS, NULL));

			when (ACK_IN, PS_SEND1);

		} else {
			if (level == 0)
				app_diag (D_DEBUG, "skip level");

			if (sens_data.ee.s.f.status != SENS_CONFIRMED) {
				app_diag (D_DEBUG, "sens not ready %u",
						sens_data.ee.s.f.status);
			} else {
				next_col_time ();
				if (lh_time <= 0)
					proceed (PS_SENS);
				proceed (PS_HOLD);
			}
		}
		//powerdown();
		delay (pong_params.freq_min << 10, PS_SEND1);
		release;

	entry (PS_SEND1)
		//powerup();
		if ((png_shift += 4) < 16)
			proceed (PS_SEND);

		next_col_time ();
		if (lh_time <= 0)
			proceed (PS_SENS);
		//powerdown();

	entry (PS_HOLD)

#if 0
	that was GLACIER:

		while (lh_time != 0) {
			if (lh_time > MAX_UINT) {
				lh_time -= MAX_UINT;
				freeze (MAX_UINT);
			} else {
				diag ("ifreeze %d %d", (word)seconds(),
						(word)lh_time);
				freeze ((word)lh_time);
				diag ("ofreeze %d", (word)seconds());
				lh_time = 0;
			}
		}
#endif
		lhold (PS_HOLD, (lword *)&lh_time);
		//powerup();

		// fall through

	entry (PS_SENS)
		if (running (sens)) {
			app_diag (D_SERIOUS, "Reg. sens delayed");
			delay (SENS_COLL_TIME, PS_COLL);
			when (SENS_DONE, PS_COLL);
			release;
		}

	entry (PS_COLL)
		if (running (sens)) { 
			app_diag (D_SERIOUS, "Mercy sens killing");
			killall (sens);
		}
		runthread (sens);
		delay (SENS_COLL_TIME, PS_NEXT);
		when (SENS_DONE, PS_NEXT);
		release;

endthread

thread (sens)
	mclock_t mc;

	entry (SE_INIT)
		powerup();
		//leds (LED_R, LED_ON);

		switch (sens_data.ee.s.f.status) {
		  case SENS_IN_USE:
			app_diag (D_SERIOUS, "Sens in use");
			break;

		  case SENS_COLLECTED:
			app_diag (D_INFO, "Not confirmed");
			if (is_eew_coll) {
		 	  if (ee_write (WNONE, sens_data.eslot * EE_SENS_SIZE,
				  (byte *)&sens_data.ee, EE_SENS_SIZE)) {
				  app_diag (D_SERIOUS, "ee_write failed %x %x",
						  (word)(sens_data.eslot >> 16),
						  (word)sens_data.eslot);
			  } else {
				  sens_data.eslot++;
			  }
			}
			break;

		  case SENS_CONFIRMED:
			sens_data.eslot++;
		}
                // now we should be at the slot to write to

		// there will be options on eeprom operations... later
		 if (sens_data.eslot >= EE_SENS_MAX) {
			 sens_data.eslot = EE_SENS_MAX -1;
			 app_diag (D_SERIOUS, "EEPROM FULL");
		 }

		sens_data.ee.s.f.status = SENS_IN_USE;
		mc.sec = 0;
		wall_time (&mc);
		sens_data.ee.ts = mc.sec;
		lh_time = seconds();
#ifdef SENSOR_LIST
	entry (SE_0)
		read_sensor (SE_0, 0, &sens_data.ee.sval[0]);

	entry (SE_1)
		read_sensor (SE_1, 1,  &sens_data.ee.sval[1]);

	entry (SE_2)
		read_sensor (SE_2, 2,  &sens_data.ee.sval[2]);

	entry (SE_3)
		read_sensor (SE_3, 3, &sens_data.ee.sval[3]);

	entry (SE_4)
		//read_sensor (SE_4, 4, &sens_data.ee.sval[4]);
		sens_data.ee.sval[4]++;
#else
		app_diag (D_WARNING, "FAKE SENSORS");
		sens_data.ee.sval[0]++;
		sens_data.ee.sval[1]++;
		sens_data.ee.sval[2]++;
		sens_data.ee.sval[3]++;
		sens_data.ee.sval[4]++;

		delay (SENS_COLL_TIME, SE_DONE);
		release;
#endif
	entry (SE_DONE)
		sens_data.ee.s.f.status = SENS_COLLECTED;
		sens_data.ee.s.f.spare = 7;
		sens_data.ee.s.f.emptym = ee_emptym ? 0 : 1;

		//leds (LED_R, LED_BLINK);
		trigger (SENS_DONE);
		powerdown();
		finish;
endthread

/*
   --------------------
   cmd_in process
   CS_ <-> Command State
   --------------------
*/

thread (cmd_in)

	entry (CS_INIT)

		if (ui_ibuf == NULL)
			ui_ibuf = get_mem (CS_INIT, UI_BUFLEN);

	entry (CS_IN)

		// hangs on the uart_a interrupt or polling
		ser_in (CS_IN, ui_ibuf, UI_BUFLEN);
		if (strlen(ui_ibuf) == 0) // CR on empty line does it
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

/*
   --------------------
   Root process
   RS_ <-> Root State
   --------------------
*/

thread (root)
	sint i1, i2, i3, i4, i5;

	entry (RS_INIT)
		ui_obuf = get_mem (RS_INIT, UI_BUFLEN);
		ee_open ();
		form (ui_obuf, ee_str, EE_SENS_MIN, EE_SENS_MAX -1,
			EE_SENS_SIZE);

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
			diag (OPRE_APP_ACK "Init eprom erase");
			if (ee_erase (WNONE, 0, 0))
				app_diag (D_SERIOUS, "erase failed");
			break_flash;
		} else {
			delay (5000, RS_INIT1);
			release;
		}

	entry (RS_INIT1)
		ser_out (RS_INIT1, ui_obuf);
		master_host = local_host;
		read_ifla();
		init();

		if (if_read (IFLASH_SIZE -1) != 0xFFFF) {
			leds (LED_R, LED_OFF);
			diag (OPRE_APP_MENU_C "***Maintenance mode***"
				OMID_CRB "%x %u %u %u",
				if_read (IFLASH_SIZE -1),
				if_read (IFLASH_SIZE -2),
				if_read (IFLASH_SIZE -3),
				if_read (IFLASH_SIZE -4));
			if (!running (cmd_in))
				runthread (cmd_in);
			stats();
			proceed (RS_RCMD);
		}
		leds (LED_G, LED_OFF);

	entry (RS_INIT2)
		ser_out (RS_INIT2, welcome_str);

		net_id = DEF_NID;
		tarp_ctrl.param &= 0xFE; // routing off
		runthread (sens);

		// give sensors time & spread a bit
		delay (SENS_COLL_TIME + rnd() % 444, RS_PAUSE);
		release;

	entry (RS_PAUSE)

		if (net_init (INFO_PHYS_DEV, INFO_PLUG_TARP) < 0) {
			app_diag (D_FATAL, "net_init failed");
			reset();
		}
		net_opt (PHYSOPT_SETSID, &net_id);
		net_opt (PHYSOPT_TXOFF, NULL);

		runthread (rcv);
		runthread (cmd_in);
		switch (pong_params.rx_span) {
			case 0:
			case 2:
				net_opt (PHYSOPT_RXOFF, NULL);
				break;
			case 1:
				break; // ON from net_init
			default:
				runthread (rxsw);
		}

		if (local_host)
			runthread (pong);

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

		if (cmd_line[0] == ' ') // ignore if starts with blank
			proceed (RS_FREE);

                if (cmd_line[0] == 'h') {
			ser_out (RS_DOCMD, welcome_str);
			proceed (RS_FREE);
		}

		if (cmd_line[0] == 'q')
			reset();

		if (cmd_line[0] == 'D') {
			sens_dump = (sensEEDumpType *)
				get_mem (WNONE, sizeof(sensEEDumpType));

			if (sens_dump == NULL)
				proceed (RS_FREE);

			memset (sens_dump, 0, sizeof(sensEEDumpType));
			i1 = 0;
			sens_dump->fr = EE_SENS_MIN;
			sens_dump->to = sens_data.eslot;

			scan (cmd_line+1, "%lu %lu %u %u",
					&sens_dump->fr, &sens_dump->to,
					&i1, &sens_dump->upto);
			switch (i1) {
				case 1:
					sens_dump->s.f.status = SENS_CONFIRMED;
					break;
				case 2:
					sens_dump->s.f.status = SENS_COLLECTED;
					break;
				case 3:
					sens_dump->s.f.status = SENS_IN_USE;
					break;
				default: // including 0
					sens_dump->s.f.status = SENS_ALL;
			}

			if (sens_dump->fr > sens_data.eslot)
				sens_dump->fr = sens_data.eslot;

			if (sens_dump->fr < EE_SENS_MIN)
				sens_dump->fr = EE_SENS_MIN;

			if (sens_dump->to > sens_data.eslot)
				sens_dump->to = sens_data.eslot; // EE_SENS_MAX

			if (sens_dump->to < EE_SENS_MIN)
				sens_dump->to = EE_SENS_MIN;

			sens_dump->ind = sens_dump->fr;

			proceed (RS_DUMP);
		}

		if (cmd_line[0] == 'M') {
			if (if_read (IFLASH_SIZE -1) != 0xFFFF) {
				diag (OPRE_APP_ACK "Already in maintenance");
				reset();
			}
			fatal_err (ERR_MAINT, (word)(seconds() >> 16),
					(word)(seconds()), 0);
			// will reset
		}

		if (cmd_line[0] == 'E') {
			diag (OPRE_APP_ACK "erasing eeprom...");	
			if (ee_erase (WNONE, 0, 0))
				app_diag (D_SERIOUS, "erase failed");
			else
				diag (OPRE_APP_ACK "eeprom erased");
			reset();
		}

		if (cmd_line[0] == 'F') {
			if_erase (IFLASH_SIZE -1);
			break_flash;
			diag (OPRE_APP_ACK "flash p1 erased");
			reset();
		}

		if (cmd_line[0] == 'Q') {
			diag (OPRE_APP_ACK "erasing all...");
			if (ee_erase (WNONE, 0, 0))
				app_diag (D_SERIOUS, "ee_erase failed");
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
					runthread (pong);
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
							runthread (rxsw);
				}
			}

			stats ();
			proceed (RS_FREE);
		}

		if (cmd_line[0] == 'S') {
			if (cmd_line[1] == 'A')
				save_ifla();
			show_ifla();
			proceed (RS_FREE); 
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
			proceed (RS_FREE);
		}

		form (ui_obuf, ill_str, cmd_line);

	entry (RS_UIOUT)

		ser_out (RS_UIOUT, ui_obuf);
		proceed (RS_FREE);

	entry (RS_DUMP)
		if (r_a_d ()) {
			delay (200, RS_DUMP);
			release;
		}
		ufree (sens_dump);
		sens_dump = NULL;
		proceed (RS_FREE);

endthread

praxis_starter (NodeTag);
