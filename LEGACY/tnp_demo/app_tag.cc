/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "globals_tag.h"
#include "threadhdrs_tag.h"

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
strand (oss_out, char*)
    entry (OO_RETRY)
	ser_outb (OO_RETRY, data);
	finish;
endstrand

strand (info_in, address)
	word dela;

    entry (II_INIT)
	dela = (word)data;
	if (dela > 20)
		dela = 10;

	if (dela > 10) {
		dela -= 10;
		leds (0, 1);
	} else {
		leds (0, 2);
	}
	delay (dela << 10, II_DONE);
	release;

    entry (II_DONE)
	leds (0, 0);
	finish;
endstrand

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

	mbuf = form (NULL, stats_str2, app_count.rcv, tarp_ctrl.rcv,
			app_count.snd, tarp_ctrl.snd,
			app_count.fwd, tarp_ctrl.fwd);
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
			app_count.rcv, tarp_ctrl.rcv,
			app_count.snd, tarp_ctrl.snd, 
			app_count.fwd, tarp_ctrl.fwd,
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

	case msg_getTagAck:
	case msg_setTagAck:
	case msg_pong:

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
		app_count.rcv++;
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

		net_opt (PHYSOPT_TXON, NULL);
		net_diag (D_DEBUG, "Tx on %x", net_opt (PHYSOPT_STATUS, NULL));

		level = ((pong_params.pow_levels >> png_shift) & 0x000f);
		if (level > 0 ) { // pong with this power
			in_pong (png_frame, level) = level;

			// this got complicated: we're at appropriate plev, and
			// we're toggling rx on/off, or it is permanently ON
			if (level == pong_params.rx_lev &&
			    ((w = running (rxsw)) ||
			     pong_params.rx_span == 1)) {
				in_pong (png_frame, flags) |= PONG_RXON;
				if (w) // means running (rxsw)
					trigger (RX_SW_ON);
			} else
				in_pong (png_frame, flags) = 0;

			level = map_level (level);
			net_opt (PHYSOPT_SETPOWER, &level);
			send_msg (png_frame, sizeof(msgPongType));
			app_diag (D_DEBUG, "It was pong on level %u",
				in_pong (png_frame, level));
		}
		net_opt (PHYSOPT_TXOFF, NULL);
		net_diag (D_DEBUG, "Tx off %x", net_opt (PHYSOPT_STATUS, NULL));
		delay (pong_params.freq_min, PS_SEND1);
		release;

	entry (PS_SEND1)

		if ((png_shift += 4) < 16)
			proceed (PS_SEND);

		if (pong_params.freq_maj > pong_params.freq_min << 2) {
		// << 2 is for 4 levels
			delay (pong_params.freq_maj - 
					(pong_params.freq_min << 2),
				PS_NEXT);
			release;
		}
		proceed (PS_NEXT);

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

		local_host = (nid_t) host_id;
		net_id = DEF_NID;
		master_host = local_host;
#if 0
		leds (0, 0); // for the demo: on WARSAW, they're on
		leds (1, 0); leds (2, 0);
#endif
		tarp_ctrl.param &= 0xFE; // routing off
		ser_out (RS_INIT, welcome_str);
		ui_obuf = get_mem (RS_INIT, UI_BUFLEN);

		// spread within freq_min (especially in VUEE)
		delay (rnd() % pong_params.freq_min, RS_PAUSE);
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
			if (in_pl) {
				pong_params.rx_lev = max_pwr(in_pl);				
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

endthread

praxis_starter (NodeTag);
