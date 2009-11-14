/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "net.h"
#include "tarp.h"
#include "nvm.h"
#include "msg_vmesh.h"
#include "lib_app_if.h"
#include "codes.h"
#include "pinopts.h"
#include "storage.h"
// #include "trc.h"

#define TINY_MEM 1

#if UART_DRIVER
#define ui_out	app_ser_out
//#define ui_in	ser_in
#else
#define ui_out(a, b, c) 
//#define ui_in(a, b, c)  
#endif


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

	case msg_stAck:
		msg_stAck_in (buf);
		return;

	case msg_nh:
		msg_nh_in (buf, rssi);
		return;

	case msg_nhAck:
		msg_nhAck_in (buf);
		return;

	case msg_io:
		msg_io_in (buf);
		return;

	case msg_ioAck:
		msg_ioAck_in (buf);
		return;

	case msg_dat:
		msg_dat_in (buf);
		return;

	case msg_datAck:
		msg_datAck_in (buf);
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

#define UI_INLEN        UART_INPUT_BUFFER_LENGTH
#define UI_TOUT         1024
static byte uart_ibuf[UI_INLEN];
static byte uart_oset, uart_len;

// things are different and messy, to avoid allocating and copying possibly
// long blocks
static void logger_in () {
	if (running (dat_rep))
		kill (running (dat_rep));
	if (dat_ptr) // already allocated
		ufree (dat_ptr);
	if (master_host == 0) {
		dbg_9 (0x4000); // data mode: no master
		return;
	}
	dat_ptr = get_mem (NONE, sizeof(msgDatType) + uart_oset);
	if (dat_ptr == NULL) {
		dbg_9 (0x3000 | uart_oset); // data mode: no memory for msg
		return;
	}
	in_header(dat_ptr, msg_type) = msg_dat;
	in_header(dat_ptr, hco) = 0; 
	in_header(dat_ptr, rcv) = master_host;
	in_dat(dat_ptr, ref) = ++dat_seq | 0x80; // ack required
	in_dat(dat_ptr, len) = uart_oset;
	memcpy (dat_ptr + sizeof(msgDatType), uart_ibuf, uart_oset);
	fork (dat_rep, NULL);
}

static void sensor_in (word state) {
	lword esn;
	int i = uart_ibuf[1];
	byte rc = RC_OK;

	if (i < 9 || i > UART_INPUT_BUFFER_LENGTH -2 -1) // uart is crap
		rc = RC_ELEN;
	else if (net_id == 0)
		rc = RC_ENET;
	else if (master_host == 0)
		rc = RC_EMAS;
	else if (master_host == local_host)
		rc = RC_ERES;
	
	if (rc != RC_OK) {
		uart_ibuf[2] = rc;
		app_ser_out (state, uart_ibuf, YES);
		return;
	}

	memcpy (&esn, uart_ibuf + 5, 4);
	if (uart_ibuf[9] == br_ctrl.dont_s && esn == br_ctrl.dont_esn)
		return;
	// all through (no aggregation)
	app_ser_out (state, uart_ibuf, YES);
	(void)msg_alrm_out (uart_ibuf);
}

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
process (cmd_in, void)
	word quant;
	nodata;

	entry (CS_START)
		proceed (CS_IN);

	entry (CS_TOUT)
		dbg_9 (0x2000 | UI_TOUT);

	entry (CS_IN)
		memset (uart_ibuf, 0, UI_INLEN);

	entry (CS_READ_START)
		io (CS_READ_START, UART_A, READ, uart_ibuf, 1);
		if (uart_ibuf[0] != '\0') {
			dbg_9 (0x1000 | uart_ibuf[0]); // no START 0x00
			uart_ibuf[0] = '\0';
			proceed (CS_READ_START);
		}

	entry (CS_READ_LEN)
		delay (UI_TOUT, CS_TOUT);
		io (CS_READ_LEN, UART_A, READ, uart_ibuf + 1, 1);
		if (uart_ibuf[1] < CMD_HEAD_LEN ||
				uart_ibuf[1] > UI_INLEN - 3) {
			dbg_9 (0x3000 | uart_ibuf[1]); // bad length
			uart_ibuf[1] = '\0';
			proceed (CS_READ_START);
		}
		uart_len = uart_ibuf[1] +1; // + 0x04
		uart_oset = 2;

	entry (CS_READ_BODY)
		delay (UI_TOUT, CS_TOUT);
		quant = io (CS_READ_BODY, UART_A, READ, uart_ibuf + uart_oset,
				uart_len);
		uart_len -= quant;
		uart_oset += quant;
		if (uart_len)
			proceed (CS_READ_BODY);
		if (uart_ibuf[uart_oset -1] != 0x04) {
			dbg_9 (0x4000 | uart_ibuf[uart_oset -1]); // no EOT 0x04
			proceed (CS_IN);
		}
		delay (0, CS_DONE_R); // just in case, cancel tout

	entry (CS_DONE_R)
		// handle sensor input w/o command overhead
		if (uart_ibuf [2] == CMD_SENSIN) {
			sensor_in (CS_DONE_R);
			proceed (CS_IN);
		}

	entry (CS_WAIT)
		if (cmd_line != NULL) {
			wait (CMD_WRITER, CS_WAIT);
			release;
		}
		cmd_ctrl.oplen  = uart_ibuf[1] - CMD_HEAD_LEN;
		cmd_ctrl.opcode = uart_ibuf[2];
		cmd_ctrl.opref  = uart_ibuf[3];

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
			memcpy (cmd_line +1, uart_ibuf +CMD_HEAD_LEN +2,
				cmd_ctrl.oplen);
		cmd_ctrl.oprc   = RC_NONE;
		if (cmd_ctrl.opcode == CMD_LOCALE)
			cmd_ctrl.t = local_host;
		else
			memcpy (&cmd_ctrl.t, uart_ibuf + CMD_HEAD_LEN, 2);
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

#define DS_START	0
#define DS_TOUT		10
#define DS_IN		20
#define DS_READ_START	30
#define DS_READ		40
process (dat_in, void)
	nodata;

	entry (DS_START)
		proceed (DS_IN);

	entry (DS_TOUT)
		dbg_9 (0x2000 | uart_oset);
		logger_in ();


	entry (DS_IN)
		memset (uart_ibuf, 0, UI_INLEN);

	entry (DS_READ_START)
		io (DS_READ_START, UART_A, READ, uart_ibuf, 1);
		if (is_crmode && uart_ibuf[0] == 0x0D) { // discard empty CRs
			uart_ibuf[0] = '\0';
			proceed (DS_READ_START);
		}
		uart_oset = 1;

	entry (DS_READ)
		delay (UI_TOUT, DS_TOUT);
		io (DS_READ, UART_A, READ, uart_ibuf + uart_oset, 1);
		if (uart_oset >= 64 ||
			       (is_crmode && uart_ibuf [uart_oset] == 0x0D)) {
			// CR is wanted as part of the string...
			if (uart_oset < 64)
				uart_oset++;
			logger_in ();
			proceed (DS_IN);
		}
		uart_oset++;
		proceed (DS_READ);
endprocess (1)
#undef DS_START
#undef DS_TOUT
#undef DS_IN
#undef DS_READ_START
#undef DS_READ

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

		case CMD_IO:
			oss_io_in();
			return;

		case CMD_IOACK:
			oss_ioack_in();
			return;

		case CMD_RESET:
			oss_reset_in();
			return;

		case CMD_LOCALE:
			oss_locale_in ();
			return;

		case CMD_DAT:
			oss_dat_in ();
			return;

		case CMD_DATACK:
			oss_datack_in ();
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


static void def_pin (word pin) {
	switch (pin) {
		case 1:
		case 2:
		case 3:
		case 8:
		case 9:
		case 10:
			pin_write (pin, 2);
			return;
		case 4:
		case 5:
			pin_write (pin, 1);
			return;
		case 6:
		case 7:
			pin_write (pin, 4);
		default:
			return;
	}
}

extern tarpCtrlType tarp_ctrl;

// see app.h for defaults
static void read_eprom_and_init() {
	lword lw;
	word w[NVM_BOOT_LEN];

	nvm_read (NVM_NID, w, NVM_BOOT_LEN);
	if (w[NVM_UART] == 0xFFFF || (w[NVM_UART] != 12 && w[NVM_UART] != 24 &&
				w[NVM_UART] != 48 && w[NVM_UART] != 96 &&
				w[NVM_UART] != 192 && w[NVM_UART] != 384))
	       w[NVM_UART] = UART_RATE / 100;
	ion (UART, CONTROL, (char *)&w[NVM_UART], UART_CNTRL_SETRATE);

	if (w[NVM_NID] == 0xFFFF)
		net_id = 0;
	else
		net_id = w[NVM_NID];
	if (w[NVM_LH] == 0 || w[NVM_LH] == 0xFFFF)
		local_host = ESN;
	else
		local_host = w[NVM_LH];
	if (w[NVM_MID] == 0xFFFF)
		master_host = 0;
	else
		master_host = w[NVM_MID];

	app_flags = DEFAULT_APP_FLAGS;
	if ((w[NVM_APP] & 0x0F) != 0x0F)
		set_encr_data (w[NVM_APP] & 0x0F);
	if (!(w[NVM_APP] & 0x40))
		clr_crmode;
	if (!(w[NVM_APP] & 0x20))
		clr_cmdmode;
	if (!(w[NVM_APP] & 0x10))
		clr_binder;
	if ((w[NVM_APP] & 0xFF00) != 0xFF00)
		tarp_ctrl.param = w[NVM_APP] >> 8;

	if (w[NVM_CYC_CTRL] != 0xFFFF) {
		memcpy (&cyc_ctrl, w + NVM_CYC_CTRL, 2);
		if (local_host == master_host) {
			if (cyc_ctrl.st == CYC_ST_PREP) // break is bad
				cyc_ctrl.st = CYC_ST_ENA;
			if (cyc_ctrl.st == CYC_ST_ENA)
				fork (cyc_man, NULL);
		} else {
			if (cyc_ctrl.st != CYC_ST_DIS)
				cyc_ctrl.st = CYC_ST_ENA;
		}
	} else {
		cyc_ctrl.mod = CYC_MOD_NET;
		cyc_ctrl.prep = DEFAULT_CYC_M_SYNC;
		if (local_host == master_host)
			cyc_ctrl.st = CYC_ST_DIS;
		else
			cyc_ctrl.st = CYC_ST_ENA;
	}

	if (w[NVM_CYC_SP] != 0xFFFF || w[NVM_CYC_SP +1] != 0xFFFF)
		memcpy (&cyc_sp, w + NVM_CYC_SP, 4);
	else {
		cyc_sp = DEFAULT_CYC_SP;
		if (local_host == master_host)
			cyc_sp += DEFAULT_CYC_M_REST;
	}

	// deal with IO pins before pmon
	memcpy (&lw, w + NVM_IO_PINS, 4);
	for (w[0] = 1; w[0] <= 10; w[0]++) {
#if CON_ON_PINS
		if (w[0] == LED_PIN0 || w[0] == LED_PIN1)
			continue;
#endif
		if ((w[1] = (lw >> ((w[0] -1) * 3)) & 7) == 7)
			def_pin (w[0]);
		else
			pin_write (w[0], w[1]);
	}

	// using io_pload as scratch:
	if (w[NVM_IO_CMP] != 0xFFFF || w[NVM_IO_CMP] != 0xFFFF) {
		memcpy (&io_pload, w + NVM_IO_CMP, 4);
		pmon_set_cmp (io_pload);
		pmon_set_cmp (-1); // a bit inconsistent pmon i/f
		pmon_pending_cmp(); // jic, clear it
	}

	if (w[NVM_IO_CREG] != 0xFFFF || w[NVM_IO_CREG + 1] != 0xFFFF)
		memcpy (&io_creg, w + NVM_IO_CREG, 4);
	else
		io_creg = 0;

	if (IFLASH[NVM_IO_STATE] != 0xFFFF) {
		// find last io state (with the counter):
		w[0] = NVM_IO_STATE +2;
		while (w[0] < (NVM_PAGE_SIZE <<1) -1 && IFLASH[w[0]] != 0xFFFF)
			w[0] += 2;
		w[1] = IFLASH[w[0] -2];
		w[2] = IFLASH[w[0] -1];

		// use io_pload as scratch
		memcpy (&io_pload, w +1, 4);
		if (w[1] & PMON_STATE_CNT_ON)
			pmon_start_cnt (io_pload >> 8,
				w[1] & PMON_STATE_CNT_RISING);
		if (w[1] & PMON_STATE_NOT_ON)
			pmon_start_not (w[1] & PMON_STATE_NOT_RISING);
		if (w[1] & PMON_STATE_CMP_ON) {
			io_pload = pmon_get_cmp();
			if (((io_creg >> 24) & 3) == 1) { // try to recover cmp
				while (io_pload <= pmon_get_cnt())
					io_pload += io_creg & 0x00FFFFFFL;
				if (io_pload & 0xFF000000)
					io_pload >>= 24;
			}
			pmon_set_cmp (io_pload);
		}

		if ((w[0] = pmon_get_state()) & PMON_STATE_NOT_ON ||
				w[0] & PMON_STATE_CMP_ON)
			fork (io_rep, NULL);
		io_pload = 0xFFFFFFFF; // restore
	}
	if (io_creg >> 26 != 0)
		fork (io_back, NULL);

	if (w[NVM_RSSI_THOLD] != 0xFFFF)
		tarp_ctrl.rssi_th = w[NVM_RSSI_THOLD];

	// 3 - warn, +4 - bad, (missed 0x00)
	connect = 0x3400;
	l_rssi = 0;

	net_opt (PHYSOPT_SETSID, &net_id);
	memset (&br_ctrl, 0, sizeof(brCtrlType));
	br_ctrl.rep_freq = 450 << 1; // 8h = 450*64s, 0: negative reporting
	if (net_id == 0) {
		freqs = 31; // beacon on 31s RONIN_FREQ
		msg_new_out();
		fastblink (1);
		app_leds (LED_BLINK);
	} else {
		if (master_host != local_host) {
			freqs = 0;
			if (is_cmdmode) {
				shared_left = -1;
				fork (st_rep, NULL);
			}
			app_leds (LED_BLINK);
			
		} else {
			freqs = 0x1D1D; // 29s freq, 29s beacon
			app_leds (LED_ON);
			tarp_ctrl.param &= 0xFE; // routing off
		}
	}
}

// uart proved to be malicious, check what there is to check:
static bool valid_input () {

	if (cmd_ctrl.opcode == 0 || cmd_ctrl.opcode == 7 ||
			cmd_ctrl.opcode == 8 ||
		(cmd_ctrl.opcode > CMD_DATACK &&
		 cmd_ctrl.opcode != CMD_SENSIN)) {
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


#if DM2200
#define INFO_PHYS_	INFO_PHYS_DM2200
#else
// assumed CC1100.. DM2100 isa bad (?)
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
		if (is_cmdmode)
			(void) fork (cmd_in, NULL);
		else
			(void) fork (dat_in, NULL);
#endif
		// Show TR8100's CFG1 and CON_ON_PINS:
		if (CON_ON_PINS)
			dbg_1 (DM2200_CFG1 | 0x0100);
		else
			dbg_1 (DM2200_CFG1);

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
			if (cmd_ctrl.oprc != RC_OK || cmd_ctrl.opref & 0x80) {
				if (cmd_ctrl.s == local_host)
					proceed (RS_RETOUT);
				else
					proceed (RS_CMDOUT); // send ret
			}
			proceed (RS_FREE);
		} else if (net_id == 0) { // no point
			cmd_ctrl.oprc = RC_ENET;
			proceed (RS_RETOUT);
		}
		// cmds turned into msgs to target:
		if (cmd_ctrl.opcode == CMD_TRACE ||
			cmd_ctrl.opcode == CMD_MASTER ||
			cmd_ctrl.opcode == CMD_SACK ||
			cmd_ctrl.opcode == CMD_IOACK ||
			cmd_ctrl.opcode == CMD_DAT ||
			cmd_ctrl.opcode == CMD_DATACK) {
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
