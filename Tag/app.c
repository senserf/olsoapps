/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "form.h"
#include "ser.h"
#include "net.h"
#include "tarp.h"
#include "diag.h"
#include "msg_tags.h"
#include "lib_apps.h"
// #include "trc.h"

heapmem {80, 20}; // how to find out a good ratio?

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

// UI is uart_a, including simulated uart_a
#define ui_out	ser_out
#define ui_outf ser_outf
#define ui_in	ser_in
#define ui_inf	ser_inf

// arbitrary
#define UI_BUFLEN		UART_INPUT_BUFFER_LENGTH

static char * ui_ibuf = NULL;
static char * ui_obuf = NULL;

// command line and its semaphores
static char * cmd_line	= NULL;
#define CMD_READER	((word)&cmd_line)
#define CMD_WRITER	((word)((&cmd_line)+1))

// rx switch control
#define RX_SW_ON	((word)&pong_params.rx_span)

// in LibConf
extern lword	host_id;
extern nid_t	local_host;
extern lword	host_passwd;
extern nid_t	net_id;

extern tarpCtrlType tarp_ctrl;

#define TINY_MEM 1
// format strings
#if TINY_MEM
static const char welcome_str[] = "\r\nTag: s (lh M m pl span hid) h q\r\n";
#else
static const char welcome_str[] = "\r\nWelcome to Tag Testbed\r\n"
	"Commands:\r\n"
	"\tSet:\ts[lh[ maj[ min[ pl[ span[ hid]]]]]]\r\n"
	"\tHelp:\th\r\n"
	"\tq(uit)\r\n";
#endif

static const char ill_str[] =	"Illegal command (%s)\r\n";

#if TINY_MEM
static const char stats_str1[] = "Stats for hostId: localHost (%lx: %u):\r\n";
static const char stats_str2[] = " In (%u:%u), Out (%u:%u), Fwd (%u:%u),\r\n";
static const char stats_str3[] = " Time (%lu), Freqs (%u, %u), PLev: %x\r\n";
static const char stats_str4[] = "phys: %x, plug: %x, txrx: %x\r\n";
static const char stats_str5[] = " Mem free (%u, %u) faults (%u, %u)\r\n";
#else
static const char stats_str[] = "Stats for hostId: localHost (%lx: %u):\r\n"
	" In (%u:%u), Out (%u:%u), Fwd (%u:%u),\r\n"
	" Time (%lu), Freqs (%u, %u), PLev: %x\r\n"
	"phys: %x, plug: %x, txrx: %x\r\n"
	" Mem free (%u, %u) faults (%u, %u)\r\n";
#endif

// Display node stats on UI
static void stats () {
	word faults0, faults1;
	word mem0 = memfree(0, &faults0);
	word mem1 = memfree(1, &faults1);
#if TINY_MEM
	app_diag (D_UI, stats_str1, host_id, local_host);
	app_diag (D_UI, stats_str2, app_count.rcv, tarp_ctrl.rcv,
			app_count.snd, tarp_ctrl.snd,
			app_count.fwd, tarp_ctrl.fwd);
	app_diag (D_UI, stats_str3, seconds(), pong_params.freq_maj,
			pong_params.freq_min, pong_params.pow_levels);
	app_diag (D_UI, stats_str4, net_opt (PHYSOPT_PHYSINFO, NULL),
			net_opt (PHYSOPT_PLUGINFO, NULL),
			net_opt (PHYSOPT_STATUS, NULL));
	app_diag (D_UI, stats_str5, mem0, mem1, faults0, faults1);
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

static void process_incoming (word state, char * buf, word size) {
  int    	w_len;
  
  switch (in_header(buf, msg_type)) {

	case msg_getTag:
		msg_getTag_in (state, buf);
		return;

	case msg_setTag:
		msg_setTag_in (state, buf);
		stats();
		return;

	case msg_rpc:
		if (cmd_line != NULL) { // busy with another input
			wait (CMD_WRITER, state);
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
		app_diag (D_SERIOUS, "Got ? (%u)", in_header(buf, msg_type));

  }
}

/*
   --------------------
   Receiver process
   RS_ <-> Receiver State
   --------------------
*/
#define RS_TRY	00
#define RS_MSG	10

// In this model, a single rcv is forked once, and runs / sleeps all the time
// Toggling rx happens in the rxsw process, driven from the pong process.
process (rcv, void)
	static int packet_size	= 0;
	static char * buf_ptr	= NULL;

	nodata;

	entry (RS_TRY)
		if (buf_ptr != NULL) {
			ufree (buf_ptr);
			buf_ptr = NULL;
			packet_size = 0;
		}
		packet_size = net_rx (RS_TRY, &buf_ptr, NULL, 0);
		if (packet_size <= 0) {
			app_diag (D_SERIOUS, "net_rx failed (%d)", packet_size);
			proceed (RS_TRY);
		}

//		app_diag (D_WARNING, "RCV (%d): %x-%u-%u-%u-%u-%u\r\n",
		app_diag (D_DEBUG, "RCV (%d): %x-%u-%u-%u-%u-%u\r\n",
			  packet_size, in_header(buf_ptr, msg_type),
			  in_header(buf_ptr, seq_no) & 0xffff,
			  in_header(buf_ptr, snd),
			  in_header(buf_ptr, rcv),
			  in_header(buf_ptr, hoc) & 0xffff,
			  in_header(buf_ptr, hco) & 0xffff);

	entry (RS_MSG)
		process_incoming (RS_MSG, buf_ptr, packet_size);
		app_count.rcv++;
		proceed (RS_TRY);

endprocess (1)

#undef RS_TRY
#undef RS_MSG

/*
  -------------------
  Rxsw process
  ------------------
*/
#define RS_OFF	00
#define RS_ON	10
	
process (rxsw, void)
	nodata;

	entry (RS_OFF)
		net_opt (PHYSOPT_RXOFF, NULL);
		net_diag (D_DEBUG, "Rx off %x", net_opt (PHYSOPT_STATUS, NULL));
		wait (RX_SW_ON, RS_ON);
		release;

	entry (RS_ON)
		net_opt (PHYSOPT_RXON, NULL);
		net_diag (D_DEBUG, "Rx on %x", net_opt (PHYSOPT_STATUS, NULL));
		delay ( pong_params.rx_span, RS_OFF);
		release;

endprocess (1)

#undef RS_OFF
#undef RS_ON

// [0, F] -> [0, FF] (high power levels may not make sense anyway)
static word map_level (word l) {
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
}

/*
  --------------------
  Pong process: spontaneous ping a rebours
  PS_ <-> Pong State 
  --------------------
*/

#define PS_INIT         00
#define PS_NEXT         10
#define PS_SEND		20
process (pong, void)

	// have it static -- the only regular activity: send it
	static char	 	frame[sizeof(msgPongType)];
	static word 		shift;
	word			level;
	nodata;

	entry (PS_INIT)
		in_header(frame, msg_type) = msg_pong;
		in_header(frame, rcv) = 0;
		in_header(frame, hco) = 1; // we do not want pong msg be forwarded by 

	entry (PS_NEXT)
		// let's say 1ms is bad -- helps with input, and
		// doesn't make any sense anyway
		if (local_host == 0 || pong_params.freq_maj < 2) {
			app_diag (D_WARNING, "Pong's suicide");
			kill (0);
		}
		shift = 0;

	entry (PS_SEND)
		net_opt (PHYSOPT_TXON, NULL);
		net_diag (D_DEBUG, "Tx on %x", net_opt (PHYSOPT_STATUS, NULL));

	entry (PS_SEND +1)
		level = ((pong_params.pow_levels >> shift) & 0x000f);
		if (level > 0 ) { // pong with this power
			in_pong (frame, level) = level;

			if (level == pong_params.rx_lev) {
				in_pong (frame, flags) |= PONG_RXON;
				trigger (RX_SW_ON);
			} else
				in_pong (frame, flags) = 0;

			level = map_level (level);
			net_opt (PHYSOPT_SETPOWER, &level);
			send_msg (frame, sizeof(msgPongType));
			app_diag (D_DEBUG, "It was pong on level %u",
				in_pong (frame, level));
		}
		net_opt (PHYSOPT_TXOFF, NULL);
		net_diag (D_DEBUG, "Tx off %x", net_opt (PHYSOPT_STATUS, NULL));
		delay (pong_params.freq_min, PS_SEND +2);
		release;

	entry (PS_SEND +2)
		if ((shift += 4) < 16)
			proceed (PS_SEND);

		if (pong_params.freq_maj > pong_params.freq_min << 2) {
		// << 2 is for 4 levels
			delay (pong_params.freq_maj - 
					(pong_params.freq_min << 2),
				PS_NEXT);
			release;
		}
		proceed (PS_NEXT);

endprocess (1)

#undef PS_INIT
#undef PS_NEXT
#undef PS_SEND	

/*
   --------------------
   cmd_in process
   CS_ <-> Command State
   --------------------
*/
#define CS_INIT 00
#define CS_IN	10
#define CS_WAIT 20

process (cmd_in, void)
	nodata;

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
			wait (CMD_WRITER, CS_WAIT);
			release;
		}

		cmd_line = get_mem (CS_WAIT, strlen(ui_ibuf) +1);
		strcpy (cmd_line, ui_ibuf);
		trigger (CMD_READER);
		proceed (CS_IN);
endprocess (1)

#undef CS_INIT
#undef CS_IN
#undef CS_WAIT


/*
   --------------------
   Root process
   RS_ <-> Root State
   --------------------
*/
#define RS_INIT		00
#define RS_FREE		10
#define RS_RCMD		20
#define RS_DOCMD	30
#define RS_UIOUT	50


process (root, void)

	// input (s command)
	word in_lh, in_pl, in_maj, in_min, in_span;
	lword in_hid;
	
	nodata;

	entry (RS_INIT)
		local_host = (nid_t)host_id;
		tarp_ctrl.param &= 0xFE; // routing off
		ui_out (RS_INIT, welcome_str);
		ui_obuf = get_mem (RS_INIT, UI_BUFLEN);
		if (net_init (INFO_PHYS_DEV, INFO_PLUG_TARP) < 0) {
			app_diag (D_FATAL, "net_init failed");
			reset();
		}
		net_opt (PHYSOPT_SETSID, &net_id);
		net_opt (PHYSOPT_RXOFF, NULL);
		net_opt (PHYSOPT_TXOFF, NULL);

		(void) fork (rcv, NULL);
		(void) fork (cmd_in, NULL);
		(void) fork (rxsw, NULL);
		if (local_host)
			(void) fork (pong, NULL);
		proceed (RS_RCMD);

	entry (RS_FREE)
		ufree (cmd_line);
		cmd_line = NULL;
		trigger (CMD_WRITER);

	entry (RS_RCMD)
		if (cmd_line == NULL) {
			wait (CMD_READER, RS_RCMD);
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
			in_hid = 0;
			scan (cmd_line+1, "%u %u %u %x %u %lx",
				&in_lh, &in_maj, &in_min,  &in_pl, &in_span,
				&in_hid);
			if (in_lh) {
				if (local_host == 0) {
					local_host = in_lh;
					(void) fork (pong, NULL);
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
					(void) fork (pong, NULL);
				} else {
					pong_params.freq_maj = in_maj;
				}
			}
			if (in_min)
				pong_params.freq_min = in_min;
			if (in_span)
				pong_params.rx_span = in_span;
			if (in_hid)
				host_id = in_hid;
			stats();
			proceed (RS_FREE);
		}

		form (ui_obuf, ill_str, cmd_line);

	entry (RS_UIOUT)
		ui_out (RS_UIOUT, ui_obuf);
		proceed (RS_FREE);

endprocess (0) // ganz egal? is (1) somehow more efficient? 
#undef RS_INIT
#undef RS_FREE
#undef RS_RCMD
#undef RS_DOCMD
#undef RS_UIOUT
