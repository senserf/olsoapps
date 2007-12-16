/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "globals_tag.h"
#include "threadhdrs_tag.h"

#ifndef __SMURPH__
#include "lhold.h"
#include "board_pins.h"
#include "sensors.h"
#else
#define	SENSOR_PAR	0
#define SENSOR_TEMP	1
#define SENSOR_HUMID	2
#define SENSOR_LIST dupajas
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
	leds (0, 2);
	if_write (IFLASH_SIZE -1, err);
	if_write (IFLASH_SIZE -2, w1);
	if_write (IFLASH_SIZE -3, w2);
	if_write (IFLASH_SIZE -4, w3);
	halt();
}

__PUBLF (NodeTag, void, sens_init) () {
	lword l, u, m; 
	byte b; 

	if (ee_read (0, &b, 1))
		fatal_err (ERR_EER, 0, 0, 1);

	memset (&sens_data, 0, sizeof (sensDataType));
	if (b == 0xFF) { // normal operations (empty eeprom)
		sens_data.eslot = 0;
		sens_data.ee.status = SENS_FF;
		return;
	}

	// only efter power down / soft reset there could be anything in eeprom
	l = EE_SENS_MIN; u = EE_SENS_MAX;
	while (u - l > 1) {
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
	if (ee_read (sens_data.eslot * EE_SENS_SIZE, (byte *)&sens_data.ee,
			EE_SENS_SIZE))
		fatal_err (ERR_EER,
			(word)((sens_data.eslot * EE_SENS_SIZE) >> 16),
			(word)(sens_data.eslot * EE_SENS_SIZE), EE_SENS_SIZE);
}


__PUBLF (NodeTag, void, init) () {

	sens_init();

// put all the crap here

}

// little helper
static const char * statusName (word s) {
	switch (s) {
		case SENS_CONFIRMED: 	return "CONFIRMED";
		case SENS_COLLECTED: 	return "COLLECTED";
		case SENS_IN_USE: 	return "IN_USE";
		case SENS_FF: 		return "FFED";
		case 0: 		return "ALL";
	}
	app_diag (D_SERIOUS, "ffed %x", s);
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

	if (sens_dump->ee.status == 0xFF) {
		if (sens_dump->fr <= sens_dump->to) {
			goto Finish;
		} else {
			goto Continue;
		}
	}

	if (sens_dump->status == 0 ||
			sens_dump->status == sens_dump->ee.status) {
		lbuf = form (NULL, "\r\n%s slot %lu ts %ld: "
					"PAR: %d, T: %d, H: %d\r\n",
			statusName (sens_dump->ee.status), sens_dump->ind, 
			sens_dump->ee.ts,
			sens_dump->ee.sval[0],
			sens_dump->ee.sval[1],
			sens_dump->ee.sval[2]);

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
	lbuf = form (NULL, "Collector %u direct dump: slots %lu -> %lu "
			"status %s upto %u #%lu\r\n",
			local_host, sens_dump->fr, sens_dump->to,
			statusName (sens_dump->status), 
			sens_dump->upto, sens_dump->cnt);

	if (runstrand (oss_out, lbuf) == 0 ) {
		app_diag (D_SERIOUS, "oss_out sum failed");
		ufree (lbuf);
	}
	return 0;
}

// Display node stats on UI
__PUBLF (NodeTag, void, stats) () {

	word faults0, faults1;
	word mem0 = memfree (0, &faults0);
	word mem1 = memfree (1, &faults1);

#if TINY_MEM
	char * mbuf;
	mbuf = form (NULL, stats_str1, host_id, local_host);
	if (runstrand (oss_out, mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out failed");
		ufree (mbuf);
		return;
	}

	mbuf = form (NULL, stats_str2, tarp_ctrl.rcv,
			tarp_ctrl.snd, tarp_ctrl.fwd);
	if (runstrand (oss_out, mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out failed");
		ufree (mbuf);
		return;
	}

	mbuf = form (NULL, stats_str3, seconds(), pong_params.freq_maj,
			pong_params.freq_min, pong_params.pow_levels);
	if (runstrand (oss_out, mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out failed");
		ufree (mbuf);
		return;
	}

	mbuf = form (NULL, stats_str4, net_opt (PHYSOPT_PHYSINFO, NULL),
			net_opt (PHYSOPT_PLUGINFO, NULL),
			net_opt (PHYSOPT_STATUS, NULL));
	if (runstrand (oss_out, mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out failed");
		ufree (mbuf);
		return;
	}

	mbuf = form (NULL, stats_str5, mem0, mem1, faults0, faults1);
	if (runstrand (oss_out, mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out failed");
		ufree (mbuf);
		return;
	}
#else
	app_diag (D_UI, stats_str,
			host_id, local_host, 
			tarp_ctrl.rcv, tarp_ctrl.snd, tarp_ctrl.fwd,
			seconds(), pong_params.freq_maj, pong_params.freq_min, 
			pong_params.pow_levels,
			net_opt (PHYSOPT_PHYSINFO, NULL),
			net_opt (PHYSOPT_PLUGINFO, NULL),
			net_opt (PHYSOPT_STATUS, NULL),
			mem0, mem1, faults0, faults1);
#endif
}

__PUBLF (NodeTag, void, process_incoming) (word state, char * buf, word size) {

  int    	w_len;
  
  switch (in_header(buf, msg_type)) {

	case msg_getTag:
		msg_getTag_in (state, buf);
		return;

	case msg_setTag:
		msg_setTag_in (state, buf);
		stats ();
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

	case msg_getTagAck:
	case msg_setTagAck:
	case msg_pong:
	case msg_master:

		return;

	default:
		app_diag (D_INFO, "Got ? (%u)", in_header(buf, msg_type));

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
*/

thread (pong)

	word	level, w;

	entry (PS_INIT)

		in_header(png_frame, msg_type) = msg_pong;
		in_header(png_frame, rcv) = 0;
		// we do not want pong msg be forwarded by 
		in_header(png_frame, hco) = 1;

	entry (PS_NEXT)

		// let's say 1ms is bad -- helps with input, and
		// doesn't make any sense anyway
		if (local_host == 0 || pong_params.freq_maj < 2) {
			app_diag (D_WARNING, "Pong's suicide");
			finish;
		}
		png_shift = 0;

	entry (PS_SEND)

		level = ((pong_params.pow_levels >> png_shift) & 0x000f);
		// pong with this power if data collected (and not confirmed)
		if (level > 0 && sens_data.ee.status == SENS_COLLECTED) {
			net_opt (PHYSOPT_TXON, NULL);
			net_diag (D_DEBUG, "Tx on %x",
					net_opt (PHYSOPT_STATUS, NULL));
			in_pong (png_frame, level) = level;
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

			w = map_level (level);
			net_opt (PHYSOPT_SETPOWER, &w);

			if (pong_params.pload_lev == 0 ||
					level == pong_params.pload_lev) {
				in_pong (png_frame, status) =
					sens_data.ee.status;
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
		} else {
			if (level == 0)
				app_diag (D_DEBUG, "skip level");
			else if (sens_data.ee.status != SENS_CONFIRMED)
				app_diag (D_DEBUG, "sens not ready %u",
						sens_data.ee.status);
		}
		delay (pong_params.freq_min << 10, PS_SEND1);
		release;

	entry (PS_SEND1)

		if ((png_shift += 4) < 16)
			proceed (PS_SEND);

		// << 2 is for 4 levels
		lh_time = pong_params.freq_maj - (pong_params.freq_min << 2) -
			(SENS_COLL_TIME >> 10);

		if (lh_time <= 0)
			proceed (PS_SENS);

	entry (PS_HOLD)
		lhold (PS_HOLD, (lword *)&lh_time);

	entry (PS_SENS)
		if (running (sens)) {
			app_diag (D_SERIOUS, "Reg. sens delayed");
			delay (SENS_COLL_TIME, PS_COLL);
			release;
		}

	entry (PS_COLL)
		if (running (sens)) { 
			app_diag (D_SERIOUS, "Mercy sens killing");
			killall (sens);
		}
		runthread (sens);
		delay (SENS_COLL_TIME, PS_NEXT);
		release;

endthread

thread (sens)

	entry (SE_INIT)
		leds (0, 1);
		if (sens_data.ee.status != SENS_FF)
			sens_data.eslot++;

		if (sens_data.eslot >= EE_SENS_MAX) 
			fatal_err (ERR_FULL, 0, 0, 0);

		switch (sens_data.ee.status) {
			case SENS_IN_USE:
				app_diag (D_SERIOUS, "Sens in use");
				break;
			case SENS_COLLECTED:
				app_diag (D_INFO, "Not confirmed");
				// fall through
			default:
				sens_data.ee.status = SENS_IN_USE;
		}

		sens_data.ee.ts = seconds() + ref_time;
#ifdef SENSOR_LIST
	entry (SE_PAR)
		read_sensor (SE_PAR, SENSOR_PAR, &sens_data.ee.sval[0]);

	entry (SE_TEMP)
		read_sensor (SE_TEMP, SENSOR_TEMP,  &sens_data.ee.sval[1]);

	entry (SE_HUMID)
		read_sensor (SE_HUMID, SENSOR_HUMID,  &sens_data.ee.sval[2]);
#else
		app_diag (D_WARNING, "FAKE SENSORS");
		// sensor 0 PAR
		sens_data.ee.sval[0]++;

		// sensor 1 T
		sens_data.ee.sval[1]++;

		// sensor 2 H
		sens_data.ee.sval[2]++;

		delay (SENS_COLL_TIME, SE_DONE);
		release;
#endif
	entry (SE_DONE)
		sens_data.ee.status = SENS_COLLECTED;
#if 1 
		if (ee_write (WNONE, sens_data.eslot * EE_SENS_SIZE,
				(byte *)&sens_data.ee, EE_SENS_SIZE)) {
			app_diag (D_SERIOUS, "ee_write failed %x %x",
					(word)(sens_data.eslot >> 16),
					(word)sens_data.eslot);
			sens_data.eslot--;
		}
#endif
		leds (0, 2);
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

	// input (s command)
	word in_lh, in_pl, in_maj, in_min, in_span;

	entry (RS_INIT)
		if (if_read (IFLASH_SIZE -1) != 0xFFFF) {
			app_diag (D_UI, "Error mode (D, E, F, Q)\r\n"
				"%x %u %u %u",
				if_read (IFLASH_SIZE -1),
				if_read (IFLASH_SIZE -2),
				if_read (IFLASH_SIZE -3),
				if_read (IFLASH_SIZE -4));
			ui_obuf = get_mem (RS_INIT, UI_BUFLEN);
			if (!running (cmd_in))
				runthread (cmd_in);
			proceed (RS_RCMD);
		}

		init();
		local_host = (nid_t) host_id;
		net_id = DEF_NID;
		master_host = local_host;
		tarp_ctrl.param &= 0xFE; // routing off
		ser_out (RS_INIT, welcome_str);
		ui_obuf = get_mem (RS_INIT, UI_BUFLEN);
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
			strcpy (ui_obuf, welcome_str);
			proceed (RS_UIOUT);
		}

		if (cmd_line[0] == 'q')
			reset();

		if (cmd_line[0] == 'D') {
			sens_dump = (sensEEDumpType *)
				get_mem (WNONE, sizeof(sensEEDumpType));

			if (sens_dump == NULL)
				proceed (RS_FREE);

			memset (sens_dump, 0, sizeof(sensEEDumpType));
			in_lh = 0;
			scan (cmd_line+1, "%lu %lu %u %u",
					&sens_dump->fr, &sens_dump->to,
					&in_lh, &sens_dump->upto);
			sens_dump->status = (byte)in_lh;
			if (sens_dump->fr > EE_SENS_MAX)
				sens_dump->fr = 0;

			if (sens_dump->to > EE_SENS_MAX)
				sens_dump->to = EE_SENS_MAX;

			sens_dump->ind = sens_dump->fr;

			proceed (RS_DUMP);
		}

		if (cmd_line[0] == 'E') { 
			if (ee_erase (WNONE, 0, 0))
				app_diag (D_UI, "ee_erase failed");
			else
				app_diag (D_UI, "eprom erased");
			reset();
		}

		if (cmd_line[0] == 'F') {
			if_erase (-1);
			app_diag (D_UI, "flash erased");
			reset();
		}

		if (cmd_line[0] == 'Q') {
			if (ee_erase (WNONE, 0, 0))
				app_diag (D_UI, "ee_erase failed");
			if_erase (-1);
			reset();
		}

		if (cmd_line[0] == 's') {
			in_lh = in_pl = in_maj = in_min = in_span = 0;
			scan (cmd_line+1, "%u %u %u %x %u",
				&in_lh, &in_maj, &in_min,  &in_pl, &in_span);
			
			if (in_lh) {
				if (local_host == 0) {
					local_host = in_lh;
					runthread (pong);
				} else {
					local_host = in_lh;
				}
			}

			// we follow max power, but likely rx i pload levels
			// should be independent
			if (in_pl) {
				if (pong_params.rx_lev != 0) // 'all' stays
					pong_params.rx_lev = max_pwr(in_pl);
				
				if (pong_params.pload_lev != 0)
					pong_params.pload_lev =
						pong_params.rx_lev;
				
				pong_params.pow_levels = in_pl;
			}

			if (in_maj) {
				
				if (pong_params.freq_maj < 2) {
					pong_params.freq_maj = in_maj;
					runthread (pong);
				} else {
					pong_params.freq_maj = in_maj;
				}
			}

			if (in_min)
				pong_params.freq_min = in_min;

			if (in_span && pong_params.rx_span != in_span) {
				pong_params.rx_span = in_span;
				
				switch (in_span) {
					case 2:
						pong_params.rx_span = 0;
					// (zero can't be entered) case 0:
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
