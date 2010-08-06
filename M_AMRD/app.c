/* ==================================================================== */
/* Modified (scaled down and split) RTags.                              */
/* SHADOW in DD cache (gbackoff delays).                                */
/* 19200 havoc caused paranoic treatment of input data.                 */
/* Not needed any more, not removed.                                    */
/*                                                                      */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "form.h"
#include "net.h"
#include "tarp.h"
#include "msg_rtag.h"
#include "ser.h"
#include "lib_app_if.h"
// #include "trc.h"

// ignored for SINGLEPOOL
heapmem {70, 30};

#if UART_DRIVER
#define ui_out	ser_out
#define ui_in	ser_in
#else
#define ui_out(a, b) 
#define ui_in(a, b, c)  
#endif

// format strings
static const char welcome_str[] = "\r\nAMR gateway:\r\n"
	"s lh\r\n"
	"m, r, t [rcv]\r\n"
	"b msec\r\n"
	"h, q\r\n";
static const char ill_str[] =	"Illegal cmd: %s";
static const char stats_str[] = "host %u up %u s";

// cmd_ctrl and cmd line semaphores
#define CMD_READER      ((word)&cmd_line)
#define CMD_WRITER      ((word)((&cmd_line)+1))

// In general, this is a dispatching point for incoming msgs.
// (rssi not used in demo)
static void process_incoming (word state, char * buf, word size, word rssi) {
  static int    	w_len;
  static char * mybuf = NULL;
  
  switch (in_header(buf, msg_type)) {

	  // meter readings are hopping in as RPCs
	case msg_rpc:

		if (mybuf == NULL) {
			w_len = strlen (buf + sizeof(msgRpcType));

			// just in impossible case NULL disappeared:
			if (w_len > size - sizeof(msgRpcType))
				w_len = size - sizeof(msgRpcType);
			
			mybuf = get_mem (state, w_len + 3);

			// keep the w_len guard
			strncpy (mybuf, buf + sizeof(msgRpcType), w_len);

			strcpy (mybuf + w_len, "\r\n"); // +2 = NULL
		}
		ser_out (state, mybuf);
		ufree (mybuf);
		mybuf = NULL;
		return;

	case msg_traceAck:
		msg_traceAck_in (buf, size);
		return;

	default:
		return; // ignore everything else (maybe another master)

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
process (rcv, void*)
	static int packet_size	= 0;
	static char * buf_ptr	= NULL;
	static word rssi	= 0;

	nodata;

	entry (RS_TRY)
		packet_size = net_rx (RS_TRY, &buf_ptr, &rssi);
		if (packet_size <= 0) {
			diag ("net_rx failed (%d)", packet_size);
			proceed (RS_TRY);
		}

	entry (RS_MSG)
		process_incoming (RS_MSG, buf_ptr, packet_size, rssi);
		if (buf_ptr != NULL) {
			ufree (buf_ptr);
			buf_ptr = NULL;
			packet_size = 0;
		}
		proceed (RS_TRY);

endprocess

#undef RS_TRY
#undef RS_MSG

/*
   --------------------
   cmd_in process
   CS_ <-> Command State
   --------------------
*/
#define CS_IN   00
#define CS_WAIT 10
#define UI_INLEN 64

process (cmd_in, void*)
	// could as well be malloced
	static unsigned char ui_ibuf[UI_INLEN];
	nodata;

	entry (CS_IN)
		memset (ui_ibuf, 0, UI_INLEN);
		// hangs on the uart_a
		ui_in (CS_IN, ui_ibuf, UI_INLEN);
		if (*ui_ibuf == NULL)
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

endprocess

#undef CS_IN
#undef CS_WAIT
#undef UI_INLEN


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


process (root, void*)

	// input (s command)
	id_t in_lh;
	
	nodata;

	entry (RS_INIT)
		ui_out (RS_INIT, welcome_str);
		if (net_init (INFO_PHYS_DM2100, INFO_PLUG_TARP) < 0) {
			diag ("net_init failed");
			reset();
		}
		// always on for this applet
		net_opt (PHYSOPT_RXON, NULL);
		net_opt (PHYSOPT_TXON, NULL);
		(void) fork (rcv, NULL);
#if UART_DRIVER
		(void) fork (cmd_in, NULL);
#endif
	entry (RS_INIT +1)
		// start master beacon
		oss_master_in (RS_INIT +1, 0);
		proceed (RS_RCMD);

	entry (RS_FREE)
		if (cmd_line) {
			ufree (cmd_line);
			cmd_line = NULL;
		}
		trigger (CMD_WRITER);

	entry (RS_RCMD)
		if (cmd_line == NULL) {
			wait (CMD_READER, RS_RCMD);
			release;
		}

	entry (RS_DOCMD)
		if (cmd_line[0] == '\0') { // bin cmd
			proceed (RS_FREE); // demo
		}
		if (cmd_line[0] == ' ') // ignore if starts with blank
			proceed (RS_FREE);

                if (cmd_line[0] == 'h') {
			ui_out (RS_DOCMD, welcome_str);
			proceed (RS_FREE);
		}

		if (cmd_line[0] == 'q')
			reset();

		if (cmd_line[0] == 'b') {
			scan (cmd_line+1, "%u", &beac_freq);
			if (beac_freq > 63)
				beac_freq = 63;
			diag ("Beacon at %u", beac_freq);
			proceed (RS_FREE);
		}

		if (cmd_line[0] == 'm') {
			in_lh = 0;
			scan (cmd_line+1, "%u", &in_lh);
			oss_master_in (RS_DOCMD, in_lh);
			proceed (RS_FREE);
		}

		if (cmd_line[0] == 'r') {
			oss_rpc_in (RS_DOCMD, cmd_line+1);
			proceed (RS_FREE);
		}

		if (cmd_line[0] == 't') {
			in_lh = 0;
			scan (cmd_line+1, "%u", &in_lh);
			oss_trace_in (RS_DOCMD, in_lh);
			proceed (RS_FREE);
		}

		if (cmd_line[0] == 's') {
			in_lh = 0;
			scan (cmd_line+1, "%u", &in_lh);
			if (in_lh)
				master_host = local_host = in_lh;
			diag (stats_str, local_host, (word)seconds());
			proceed (RS_FREE);
		}
		diag (ill_str, cmd_line);
		proceed (RS_FREE);

endprocess
#undef RS_INIT
#undef RS_FREE
#undef RS_RCMD
#undef RS_DOCMD
