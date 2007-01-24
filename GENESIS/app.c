/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "net.h"
#include "tarp.h"
#include "msg_gene.h"
#include "lib_app_if.h"
#include "codes.h"
// #include "trc.h"

#define TINY_MEM 1

#if UART_DRIVER
#define ui_out	app_ser_out
//#define ui_in	ser_in
#else
#define ui_out(a, b, c) 
//#define ui_in(a, b, c)  
#endif

static void sensrx_in (char * buf) {
	word v;
	if (buf[1] != 3) // bad length
		return;
	memcpy (&v, buf + 3, 2);
	if (v != sensrx_ver) {
		sensrx_ver = v;
		ee_write (WNONE, EE_SENSRX_VER, (byte *)&sensrx_ver, 2);
	}
}

static void sensor_in (word state, char * buf) {
	lword esn;
	int	i;
	byte rc = RC_OK;
	memcpy (&esn, buf + 5, 4);
	i = buf[1];

	if (i < 9 || i > UART_INPUT_BUFFER_LENGTH -2 -1) // uart is crap
		rc = RC_ELEN;
	else if (net_id == 0)
		rc = RC_ENET;
	else if (master_host == 0)
		rc = RC_EMAS;
	else if (master_host == local_host)
		rc = RC_ERES;
	
	if (rc != RC_OK) {
		buf[2] = rc;
		app_ser_out (state, buf, YES);
		return;
	}

	if (esn_count == 0) { // all through
		app_ser_out (state, buf, YES);
		(void)msg_alrm_out (buf);
		return;
	}
	
	// nonempty ESN table, we do status aggregation:

	// immediate forward
	if (buf[9] & 0x25) { // b0, b2, b5
		app_ser_out (state, buf, YES);
		if (buf[9] != br_ctrl.dont_s || esn != br_ctrl.dont_esn)
			(void)msg_alrm_out (buf);
		return;
	}

	if ((i = lookup_esn (&esn)) < 0)
		buf[2] = RC_EADDR;
	else
		set_svec (i, YES);
	app_ser_out (state, buf, YES);

	// not a heart beat, is in the ESN table, and
	// not blocked by "don't tell"
	if (buf[9] != 0 && buf[2] != RC_EADDR &&
			(buf[9] != br_ctrl.dont_s || esn != br_ctrl.dont_esn))
		(void)msg_alrm_out (buf);
}

static void process_incoming (word state, char * buf, word size, word rssi) {
	// let's leave the rssi as a formal param, even if it's
	// redundant with l_rssi, which could be set in net / tarp
	if (in_header(buf, snd) == master_host)
		l_rssi = rssi;

  switch (in_header(buf, msg_type)) {

	case msg_master:
		msg_master_in (buf);
		return;

	case msg_trace:
	case msg_traceF:
	case msg_traceB:
		msg_trace_in (state, buf);
		return;

	case msg_traceAck:
	case msg_traceFAck:
	case msg_traceBAck:
		msg_traceAck_in (state, buf);
		return;

	case msg_cmd:
		msg_cmd_in (state, buf);
		return;

	case msg_new:
		if (master_host == 0)
			return;
		msg_new_in (buf);
		return;

	case msg_bind:
		msg_bind_in (buf);
		return;

	case msg_bindReq:
		msg_bindReq_in (buf);
		return;

	case msg_br:
		msg_br_in (buf);
		return;
		
	case msg_alrm:
		msg_alrm_in (buf);
		return;

	case msg_st:
		msg_st_in (buf);
		return;

	case msg_stAck:
		msg_stAck_in (buf);
		return;

	case msg_stNack:
		msg_stNack_in (buf);
		return;

	case msg_nh:
		msg_nh_in (buf, rssi);
		return;

	case msg_nhAck:
		msg_nhAck_in (buf);
		return;

	default:
		dbg_a (0x0200 | in_header(buf, msg_type)); // Got ?

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
process (rcv, void)
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
		packet_size = net_rx (RS_TRY, &buf_ptr, &rssi, encr_data);
		if (packet_size <= 0) {
			dbg_a (0x01FF); // net_rx failed (%d)...
			dbg_a (0x8000 | packet_size); // ...packet_size
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
		proceed (RS_TRY);

endprocess (1)

#undef RS_TRY
#undef RS_MSG


#if UART_DRIVER

/*
   --------------------
   cmd_in process
   CS_ <-> Command State
   --------------------
*/
#define CS_START	00
#define CS_TOUT		10
#define CS_IN		20
#define CS_READ_START	30
#define CS_READ_LEN	40
#define CS_READ_BODY	50
#define CS_DONE_R	60
#define CS_WAIT 	70

#define UI_INLEN 	64
#define UI_TOUT		1024
process (cmd_in, void)
	static byte ui_ibuf[UI_INLEN];
	static byte ptr, len;
	word quant;

	nodata;

	entry (CS_START)
		proceed (CS_IN);

	entry (CS_TOUT)
		dbg_9 (0x2000 | UI_TOUT);

	entry (CS_IN)
		memset (ui_ibuf, 0, UI_INLEN);

	entry (CS_READ_START)
		io (CS_READ_START, UART_A, READ, ui_ibuf, 1);
		if (ui_ibuf[0] != '\0') {
			dbg_9 (0x1000 | ui_ibuf[0]); // no START 0x00
			ui_ibuf[0] = '\0';
			proceed (CS_READ_START);
		}

	entry (CS_READ_LEN)
		delay (UI_TOUT, CS_TOUT);
		io (CS_READ_LEN, UART_A, READ, ui_ibuf + 1, 1);
		// 3 is for CMD_SENSRX, 1 less than CMD_HEAD_LEN
		if (ui_ibuf[1] < 3 || ui_ibuf[1] > UI_INLEN - 3) {
			dbg_9 (0x3000 | ui_ibuf[1]); // bad length
			ui_ibuf[1] = '\0';
			proceed (CS_READ_START);
		}
		len = ui_ibuf[1] +1; // + 0x04
		ptr = 2;

	entry (CS_READ_BODY)
		delay (UI_TOUT, CS_TOUT);
		quant = io (CS_READ_BODY, UART_A, READ, ui_ibuf + ptr, len);
		len -= quant;
		ptr += quant;
		if (len)
			proceed (CS_READ_BODY);
		if (ui_ibuf[ptr -1] != 0x04) {
			dbg_9 (0x4000 | ui_ibuf[ptr -1]); // no EOT 0x04
			proceed (CS_IN);
		}
		delay (0, CS_DONE_R); // just in case, cancel tout

	entry (CS_DONE_R)
		// handle sensor input w/o command overhead
		if (ui_ibuf [2] == CMD_SENSIN) {
			sensor_in (CS_DONE_R, ui_ibuf);
			proceed (CS_IN);
		}
		if (ui_ibuf [2] == CMD_SENSRX) {
			sensrx_in (ui_ibuf);
			proceed (CS_IN);
		}

	entry (CS_WAIT)
		if (cmd_line != NULL) {
			wait (CMD_WRITER, CS_WAIT);
			release;
		}
		cmd_ctrl.oplen  = ui_ibuf[1] - CMD_HEAD_LEN;
		cmd_ctrl.opcode = ui_ibuf[2];
		cmd_ctrl.opref  = ui_ibuf[3];

		// do this ugly thing for CMD_GET, so we don't have
		// to resize cmd_line later on. 6 is the longest value,
		// all together it'll be 8 bytes.
		// Similarly for CMD_LOCALE.
		if (cmd_ctrl.opcode == CMD_GET || cmd_ctrl.opcode == CMD_LOCALE)
			cmd_line = get_mem (CS_WAIT, 8);
		else
			cmd_line = get_mem (CS_WAIT, cmd_ctrl.oplen +1);

		cmd_line[0] = '\0';
		if (cmd_ctrl.oplen)
			memcpy (cmd_line +1, ui_ibuf +CMD_HEAD_LEN +2,
				cmd_ctrl.oplen);
		cmd_ctrl.oprc   = RC_NONE;
		if (cmd_ctrl.opcode == CMD_LOCALE)
			cmd_ctrl.t = local_host;
		else
			memcpy (&cmd_ctrl.t, ui_ibuf + CMD_HEAD_LEN, 2);
		cmd_ctrl.s = local_host;
		trigger (CMD_READER);
		proceed (CS_IN);

endprocess (1)
#undef CS_START
#undef CS_TOUT
#undef CS_IN
#undef CS_READ_START
#undef CS_READ_LEN
#undef CS_READ_BODY
#undef CS_DONE_R
#undef CS_WAIT
#undef UI_INLEN
#undef UI_TOUT

#endif

static void cmd_exec (word state) {

	switch (cmd_ctrl.opcode) {
		case CMD_MASTER:
			oss_master_in (state);
			return;

		case CMD_TRACE:
			oss_trace_in (state);
			return;

		case CMD_SET:
			oss_set_in();
			return;

		case CMD_GET:
			oss_get_in(state);
			return;

		case CMD_BIND:
			oss_bind_in (state);
			return;

		case CMD_SACK:
			oss_sack_in ();
			return;

		case CMD_SNACK:
			oss_snack_in();
			return;

		case CMD_SENS:
			oss_sens_in();
			return;

		case CMD_RESET:
			oss_reset_in();
			return;

		case CMD_LOCALE:
			oss_locale_in ();
			return;
	}
	cmd_ctrl.oprc = RC_ECMD;
}
			
static void cmd_out (word state) {
	char * out_buf = NULL;
	if (net_id == 0) {
		cmd_ctrl.oprc = RC_ENET;
		return;
	}
	msg_cmd_out (state, &out_buf);
	send_msg (out_buf, sizeof(msgCmdType) + cmd_ctrl.oplen);
	ufree (out_buf);
	return;
}

extern tarpCtrlType tarp_ctrl;
static void read_eprom_and_init() {
	word w[NVM_BOOT_LEN >> 1];

	ee_read (EE_NID, (byte *)w, NVM_BOOT_LEN);
	if (w[EE_NID >> 1] == 0xFFFF)
		net_id = 0;
	else
		net_id = w[EE_NID >> 1];
	if (w[EE_LH >> 1] == 0 || w[EE_LH >> 1] == 0xFFFF)
		local_host = ESN;
	else
		local_host = w[EE_LH >> 1];
	if (w[EE_MID >> 1] == 0xFFFF)
		master_host = 0;
	else
		master_host = w[EE_MID >> 1];

	app_flags = DEFAULT_APP_FLAGS;
	if ((w[EE_APP >> 1] & 0x0F) != 0x0F)
		set_encr_data (w[EE_APP >> 1] & 0x0F);
	if (!(w[EE_APP >> 1] & 0x10))
		clr_binder;
	if ((w[EE_APP >> 1] & 0xFF00) != 0xFF00)
		tarp_ctrl.param = w[EE_APP >> 1] >> 8;

	sensrx_ver = w[EE_SENSRX_VER >> 1];

	// 3 - warn, +4 - bad, (missed 0x00)
	connect = 0x3400;
	l_rssi = 0;
	esn_count = count_esn();

	net_opt (PHYSOPT_SETSID, &net_id);
	memset (&br_ctrl, 0, sizeof(brCtrlType));
	br_ctrl.rep_freq = 450 << 1; // 8h = 450*64s, 0: negative reporting
	if (net_id == 0) {
		freqs = 31; // beacon on 31s RONIN_FREQ
		msg_new_out();
		fastblink (1);
		leds (CON_LED, LED_BLINK);
	} else {
		if (master_host != local_host) {
			freqs = 0;
			set_powup;
			fork (st_rep, NULL);
			leds (CON_LED, LED_OFF);
			
		} else {
			freqs = 0x1D1D; // 29s freq, 29s beacon
			leds (CON_LED, LED_ON);
			tarp_ctrl.param &= 0xFE; // routing off
		}
	}
}

// uart proved to be malicious, check what there is to check:
static bool valid_input () {

	if (cmd_ctrl.opcode == 0 || 
		cmd_ctrl.opcode > CMD_LOCALE && cmd_ctrl.opcode != CMD_SENSIN &&
			cmd_ctrl.opcode != CMD_SENSRX) {
		dbg_9 (0x5000 | cmd_ctrl.opcode);
		return NO;
	}
	if (cmd_ctrl.oprc > RC_EMEM && cmd_ctrl.oprc != RC_NONE) {
		dbg_9 (0x6000 | cmd_ctrl.oprc);
		return NO;
	}
	return YES;
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
#define RS_CMDOUT	50
#define RS_RETOUT	60


#if DM2100
#define INFO_PHYS_	INFO_PHYS_DM2100
#else
// assumed CC1100
#define INFO_PHYS_	INFO_PHYS_CC1100
#endif

process (root, void)

	nodata;

	entry (RS_INIT)

		if (net_init (INFO_PHYS_, INFO_PLUG_TARP) < 0) {
			dbg_0 (0x1000); // net_init failed, reset
			reset();
		}
		read_eprom_and_init();
		(void) fork (rcv, NULL);
#if UART_DRIVER
		(void) fork (cmd_in, NULL);
#endif
		//leds (CON_LED, LED_BLINK); // TXON sets it on on dm2100
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

		if (!valid_input())
			proceed (RS_FREE);

		if (cmd_ctrl.oprc != RC_NONE) // response
			proceed (RS_RETOUT);

	entry (RS_DOCMD)
		if (cmd_ctrl.t == local_host) {
			cmd_exec (RS_DOCMD);
			if (cmd_ctrl.oprc == RC_ECMD || 
				cmd_ctrl.opcode == CMD_LOCALE)
				proceed (RS_RETOUT);
			if (cmd_ctrl.oprc != RC_OK || cmd_ctrl.opref & 0x80)
				if (cmd_ctrl.s == local_host)
					proceed (RS_RETOUT);
				else
					proceed (RS_CMDOUT); // send ret
			proceed (RS_FREE);
		} else if (net_id == 0) { // no point
			cmd_ctrl.oprc = RC_ENET;
			proceed (RS_RETOUT);
		}
		// cmds turned into msgs to target:
		if (cmd_ctrl.opcode == CMD_TRACE ||
			cmd_ctrl.opcode == CMD_MASTER ||
			cmd_ctrl.opcode == CMD_SACK ||
			cmd_ctrl.opcode == CMD_SNACK) {
			cmd_exec (RS_DOCMD);
			// requested: master should confirm
			if (cmd_ctrl.oprc != RC_OK
				|| cmd_ctrl.opcode == CMD_MASTER)
				proceed (RS_RETOUT);
			proceed (RS_FREE);
		}
		proceed (RS_CMDOUT);

	entry (RS_CMDOUT)
		cmd_out (RS_CMDOUT);
		proceed (RS_FREE);

	entry (RS_RETOUT)
		oss_ret_out (RS_RETOUT);
		proceed (RS_FREE);

endprocess (0)
#undef RS_INIT
#undef RS_FREE
#undef RS_RCMD
#undef RS_DOCMD
#undef RS_CMDOUT
#undef RS_RETOUT

praxis_starter (Node);
