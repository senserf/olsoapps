/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "net.h"
#include "tarp.h"
#include "msg_rtag.h"
#include "ser.h"
#include "lib_app_if.h"
#include "codes.h"
// #include "trc.h"

#define TINY_MEM 1

#if UART_DRIVER
#define ui_out	ser_out
#define ui_in	ser_in
#else
#define ui_out(a, b) 
#define ui_in(a, b, c)  
#endif

extern tarpCountType tarp_count;
extern tarpParamType tarp_param;

static void process_incoming (word state, char * buf, word size, word rssi) {
  
  switch (in_header(buf, msg_type)) {

	case msg_master:
		msg_master_in (buf);
		return;

	case msg_cmd:
		msg_cmd_in (state, buf);
		return;

	case msg_trace:
		msg_trace_in (state, buf);
		return;

	case msg_traceAck:
		oss_traceAck_out (state, buf);
		return;

	default:
		diag ("Got ? (%u)", in_header(buf, msg_type));

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
			diag ("net_rx failed (%d)", packet_size);
			proceed (RS_TRY);
		}
#if 0
		diag ("RCV (%d): %x-%u-%u-%u-%u-%u\r\n",
			  packet_size, in_header(buf_ptr, msg_type),
			  in_header(buf_ptr, seq_no),
			  in_header(buf_ptr, snd),
			  in_header(buf_ptr, rcv),
			  in_header(buf_ptr, hoc),
			  in_header(buf_ptr, hco));
#endif
	entry (RS_MSG)
		process_incoming (RS_MSG, buf_ptr, packet_size, rssi);
		app_count.rcv++;
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
	nodata;

	entry (CS_IN)
		memset (ui_ibuf, 0, UI_INLEN);
		// hangs on the uart_a
		ui_in (CS_IN, ui_ibuf, UI_INLEN);
		// verify bin cmd at source:
		if (*ui_ibuf == NULL &&
			(ui_ibuf[1] < CMD_HEAD_LEN ||
			 ui_ibuf[ui_ibuf[1] +2] != 0x04)) {
			diag ("Bad bin cmd");
			proceed (CS_IN);
		}

	entry (CS_WAIT)
		if (cmd_line != NULL) {
			wait (CMD_WRITER, CS_WAIT);
			release;
		}
		if (*ui_ibuf) { // string
			cmd_line = get_mem (CS_WAIT, strlen(ui_ibuf) +1);
			strcpy (cmd_line, ui_ibuf);
			trigger (CMD_READER);
			proceed (CS_IN);
		}

		// bin cmd
		cmd_ctrl.p_q = ui_ibuf[2];
		memcpy (&cmd_ctrl.p, &ui_ibuf[3], 2);
		cmd_ctrl.t_q = ui_ibuf[5];
		memcpy (&cmd_ctrl.t, &ui_ibuf[6], 2);
		cmd_ctrl.opcode = ui_ibuf[8];
		cmd_ctrl.opref  = ui_ibuf[9];
		cmd_ctrl.s_q = ADQ_OSS;
		cmd_ctrl.s = local_host;
		cmd_ctrl.oprc   = RC_NONE;
		cmd_ctrl.oplen  = ui_ibuf[1] - CMD_HEAD_LEN;

	entry (CS_WAIT +1)
		cmd_line = get_mem (CS_WAIT +1, cmd_ctrl.oplen +1);
		cmd_line[0] = '\0';
		if (cmd_ctrl.oplen)
			memcpy (cmd_line +1, ui_ibuf +CMD_HEAD_LEN +2,
				cmd_ctrl.oplen);
		trigger (CMD_READER);
		proceed (CS_IN);

endprocess

#undef CS_IN
#undef CS_WAIT
#undef UI_INLEN

static void cmd_exec (word state) {
	if (cmd_ctrl.t_q == ADQ_MASTER) {
		if (master_host == 0) {
			cmd_ctrl.oprc = RC_EMAS;
			return;
		}
		cmd_ctrl.t = master_host;
	}
	if (cmd_ctrl.t_q == ADQ_LOCAL)
		cmd_ctrl.t = local_host;

	switch (cmd_ctrl.opcode) {
		case CMD_MASTER:
			oss_master_in (state, cmd_ctrl.t);
			cmd_ctrl.oprc = RC_OK;
			return;

		case CMD_DISP:
			cmd_ctrl.oprc = RC_OK;
			return;

		case CMD_TRACE:
			oss_trace_in (state, cmd_ctrl.t);
			cmd_ctrl.oprc = RC_OK;
			return;

		case CMD_SET:
			oss_set_in();
			return;

		case CMD_INFO:
			oss_info_in(state);
			return;

	}
	cmd_ctrl.oprc = RC_ECMD;
}
			
static void cmd_out (word state, id_t rcv) {
	char * out_buf;
	if (net_id == 0) {
		cmd_ctrl.oprc = RC_ENET;
		return;
	}

	out_buf = get_mem (state, sizeof(msgCmdType) + cmd_ctrl.oplen);
	in_header(out_buf, msg_type) = msg_cmd;
	in_header(out_buf, rcv) = rcv;
	in_cmd(out_buf, s) = cmd_ctrl.s;
	in_cmd(out_buf, p) = cmd_ctrl.p;
	in_cmd(out_buf, t) = cmd_ctrl.t;
	in_cmd(out_buf, opcode) = cmd_ctrl.opcode;
	in_cmd(out_buf, opref) = cmd_ctrl.opref;
	in_cmd(out_buf, oplen) = cmd_ctrl.oplen;
	in_cmd(out_buf, oprc) = cmd_ctrl.oprc;
	if (cmd_ctrl.oplen)
		 memcpy (out_buf+sizeof(msgCmdType), cmd_line +1,
			 cmd_ctrl.oplen);
	send_msg (out_buf, sizeof(msgCmdType) + cmd_ctrl.oplen);
	// this may be called after cmd exec, and called for ret sending
	// to target: don't "positively" override the ret code
	if (cmd_ctrl.oprc == RC_NONE)
		cmd_ctrl.oprc = RC_OK;
	return;
}

// in more restricted applets, much more either error settings or
// corrections should be here
static void validate() {
	if (cmd_ctrl.p_q == ADQ_MASTER) {
		if (master_host == 0) {
			cmd_ctrl.oprc = RC_EMAS;
			return;
		}
		cmd_ctrl.p = master_host;
	}
	if (cmd_ctrl.p_q == ADQ_LOCAL)
			 cmd_ctrl.p = local_host;
}
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
#define RS_TXTCMD	40
#define RS_RETOUT	60

process (root, void*)

	nodata;

	entry (RS_INIT)
		if (net_init (INFO_PHYS_DM2100, INFO_PLUG_TARP) < 0) {
			diag ("net_init failed");
			reset();
		}
		net_opt (PHYSOPT_RXON, NULL);
		net_opt (PHYSOPT_TXON, NULL);
		(void) fork (rcv, NULL);
#if UART_DRIVER
		(void) fork (cmd_in, NULL);
#endif
		proceed (RS_RCMD);

	entry (RS_FREE)
		ufree (cmd_line);
		cmd_line = NULL;
		memset (&cmd_ctrl, 0, sizeof(cmd_ctrl));
		trigger (CMD_WRITER);

	entry (RS_RCMD)
		if (cmd_line == NULL) {
			wait (CMD_READER, RS_RCMD);
			release;
		}

	entry (RS_DOCMD)
		if (cmd_line[0] != '\0') // txt cmd
			proceed (RS_TXTCMD);

		validate();
		if (cmd_ctrl.oprc != RC_NONE)
			proceed (RS_RETOUT);

	 entry (RS_DOCMD +1)
		if (cmd_ctrl.p_q != ADQ_MSG && cmd_ctrl.p != local_host) {
			cmd_out (RS_DOCMD +1, cmd_ctrl.p);
			proceed (RS_RETOUT);
		}

	entry (RS_DOCMD +2)
		cmd_exec (RS_DOCMD +2);

	entry (RS_DOCMD +3)
		if (cmd_ctrl.t != local_host) {
			// in some cases, we do NOT want ret going to
			// the target(s)
			if (cmd_ctrl.opcode != CMD_MASTER &&
			    cmd_ctrl.opcode != CMD_TRACE)
				cmd_out (RS_DOCMD +3, cmd_ctrl.t);
		}
		// in some other (freaking blueprint) cases, the
		// local ret is excessive, too:
		if (cmd_ctrl.opcode == CMD_TRACE)
			proceed (RS_FREE);
		proceed (RS_RETOUT);

	entry (RS_TXTCMD)
		if (cmd_line[0] == ' ') // ignore if starts with blank
			proceed (RS_FREE);

		if (cmd_line[0] == 'q')
			reset();

		diag ("Bad txt cmd: %s", cmd_line);
		proceed (RS_FREE);

	entry (RS_RETOUT)
		oss_ret_out (RS_RETOUT);
		proceed (RS_FREE);

endprocess
#undef RS_INIT
#undef RS_FREE
#undef RS_RCMD
#undef RS_DOCMD
#undef RS_TXTCMD
#undef RS_RETOUT
