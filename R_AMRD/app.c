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
#include "net.h"
#include "tarp.h"
#include "msg_rtag.h"
#include "ser.h"
#include "lib_app_if.h"
// #include "trc.h"

#define ui_in	ser_in

char * cmd_line  = NULL;
#define CMD_READER      ((word)&cmd_line)
#define CMD_WRITER      ((word)((&cmd_line)+1))

extern tarpCountType tarp_count;
extern tarpParamType tarp_param;

lint  master_delta = 0;
lword last_read = 0;
word  count_read = 0;
bool  from_remote = NO;

static void process_incoming (word state, char * buf, word size, word rssi) {
  
  switch (in_header(buf, msg_type)) {

	case msg_master:
		msg_master_in (buf);
		return;

	case msg_rpc:
		if (cmd_line != NULL) { // busy with another input
			wait (CMD_WRITER, state);
			release;
		}
		if (size < sizeof(msgRpcType) +2)
			return;
		cmd_line = get_mem (state, 2);
		cmd_line[0] = buf[sizeof(msgRpcType)];
		cmd_line[1] = '\0';
		from_remote = YES;
		trigger (CMD_READER);
		return;

	case msg_trace:
		msg_trace_in (state, buf);
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
#define RS_TRY	00
#define RS_MSG	10

// In this model, a single rcv is forked once, and runs / sleeps all the time
process (rcv, void*)
	static int packet_size	= 0;
	static char * buf_ptr	= NULL;
	static word rssi	= 0;

	nodata;

	entry (RS_TRY)
		if (buf_ptr != NULL) {
			ufree (buf_ptr);
			buf_ptr = NULL;
			packet_size = 0;
		}
		packet_size = net_rx (RS_TRY, &buf_ptr, &rssi);
		if (packet_size <= 0) {
			proceed (RS_TRY);
		}

	entry (RS_MSG)
		process_incoming (RS_MSG, buf_ptr, packet_size, rssi);
		// free before transition...
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
	static unsigned char ui_ibuf[UI_INLEN];
	static int len =0;
	nodata;

	entry (CS_IN)
		memset (ui_ibuf, 0, UI_INLEN);
		// hangs on the uart_a
		ui_in (CS_IN, ui_ibuf, UI_INLEN);
		if (*ui_ibuf == NULL || (len = strlen(ui_ibuf)) <= 0) {
			proceed (CS_IN);
		}
		// 19200 paranoia
		if (len >= UI_INLEN) {
			ui_ibuf[UI_INLEN-1] = '\0';
			len = UI_INLEN-1;
		}

	entry (CS_WAIT)
		if (cmd_line != NULL) {
			wait (CMD_WRITER, CS_WAIT);
			release;
		}
		cmd_line = get_mem (CS_WAIT, len +1);
		strcpy (cmd_line, ui_ibuf);
		last_read = seconds();
		count_read++;
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
#define RS_RCMD		20
#define RS_DOCMD	30

process (root, void*)

	nodata;

	entry (RS_INIT)
		if (net_init (INFO_PHYS_DM2100, INFO_PLUG_TARP) < 0) {
			reset();
		}
		net_opt (PHYSOPT_RXON, NULL);
		net_opt (PHYSOPT_TXON, NULL);
		(void) fork (rcv, NULL);
		(void) fork (cmd_in, NULL);
		// in case flash was screwed
		if (local_host == 0)
			local_host = 666;
		proceed (RS_RCMD);

	entry (RS_RCMD)
		if (cmd_line == NULL) {
			wait (CMD_READER, RS_RCMD);
			release;
		}

	entry (RS_DOCMD)
		// demo
		if (*cmd_line != NULL && master_host != 0)
			oss_rpc_in (RS_DOCMD, cmd_line);
		if (from_remote)
			from_remote = NO;
		if (cmd_line)
			ufree (cmd_line);
		cmd_line = NULL;
		trigger (CMD_WRITER);
		proceed (RS_RCMD);
endprocess
#undef RS_INIT
#undef RS_RCMD
#undef RS_DOCMD
