/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "vuee_ert.h"

#include "app_ert.h"
#include "net.h"
#include "tarp.h"
#include "nvm_ert.h"
#include "msg_ert.h"
#include "codes_ert.h"
#include "pinopts.h"
#include "storage.h"
// #include "trc.h"

#define __dcx_def__
#include "app_ert_data.h"
#undef  __dcx_def__

#if UART_TCV
#include "abb.h"
#include "plug_null.h"
#endif


/*static*/ void process_incoming (word state, char * buf, word size, 
		word rssi) {
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

thread (rcv)

	entry (RS_TRY)
		if (rcv_buf_ptr != NULL) {
			ufree (rcv_buf_ptr);
			rcv_buf_ptr = NULL;
			rcv_packet_size = 0;
		}
		rcv_packet_size = net_rx (RS_TRY, &rcv_buf_ptr,
				&rcv_rssi, encr_data);
		if (rcv_packet_size <= 0) {
			dbg_a (0x01FF); // net_rx failed (%d)...
			dbg_a (0x8000 | rcv_packet_size); // ...packet_size
			proceed (RS_TRY);
		}
#if 0
		diag ("RCV (%d): %x-%u-%u-%u-%u-%u\r\n",
			  rcv_packet_size, in_header(buf_ptr, msg_type),
			  in_header(buf_ptr, seq_no),
			  in_header(buf_ptr, snd),
			  in_header(buf_ptr, rcv),
			  in_header(buf_ptr, hoc),
			  in_header(buf_ptr, hco));
#endif
	entry (RS_MSG)
		process_incoming (RS_MSG, rcv_buf_ptr, rcv_packet_size,
				rcv_rssi);
		proceed (RS_TRY);

endthread

#if UART_DRIVER || UART_TCV

// things are different and messy, to avoid allocating and copying possibly
// long blocks
/*static*/ void logger_in () {
	killall (dat_rep);
	if (dat_ptr) // already allocated
		ufree (dat_ptr);
	if (master_host == 0) {
		dbg_9 (0x4000); // data mode: no master
		return;
	}
	dat_ptr = (byte *)get_mem (WNONE, sizeof(msgDatType) + uart_oset);
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
	runthread (dat_rep);
}

/*static*/ void sensor_in (word state) {
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
		app_ser_out (state, (char *)uart_ibuf, YES);
		return;
	}

	memcpy (&esn, uart_ibuf + 5, 4);
	if (uart_ibuf[9] == br_ctrl.dont_s && esn == br_ctrl.dont_esn)
		return;
	// all through (no aggregation)
	app_ser_out (state, (char *)uart_ibuf, YES);
	(void)msg_alrm_out ((char *)uart_ibuf);
}

#endif

/*
   The UART i/f was designed for io operations that, hopefully, will become
   obsolete. However, for now we maintain them under UART_DRIVER condition,
   and keep related structures also for UART_TCV, even if they are somewhat
   wasteful there.
*/

#if UART_TCV

// See above: we're keeping uart_ibuf and the framing, even if not needed here.
thread (cmd_in)
	byte *ib;

	entry (CS_START)
		memset (uart_ibuf, 0, UI_INLEN);

	entry (CS_IN)
		ib = abb_in (CS_IN, (address)&uart_len);
                if (uart_len < CMD_HEAD_LEN || uart_len > UI_INLEN - 3) {
			dbg_9 (0x3000 | uart_len); // bad length
			ufree (ib);
			proceed (CS_IN);
		}
		uart_ibuf [0] = '\0';
		uart_ibuf [1] = uart_len;
		uart_ibuf [uart_len +2] = 0x04;
		memcpy (&uart_ibuf[2], ib, uart_len);
		ufree (ib);
		proceed (CS_DONE_R);

	entry (CS_DONE_R)
                // handle sensor input w/o command overhead
                if (uart_ibuf [2] == CMD_SENSIN) {
                        sensor_in (CS_DONE_R);
                        proceed (CS_START);
                }

        entry (CS_WAIT)
                if (cmd_line != NULL) {
                        when (CMD_WRITER, CS_WAIT);
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
                        cmd_line = (byte *)get_mem (CS_WAIT, 8);
                else
                        cmd_line = (byte *)get_mem (CS_WAIT, cmd_ctrl.oplen +1);

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
                proceed (CS_START);

endthread

thread (dat_in)
        byte *ib;

        entry (DS_START)
                memset (uart_ibuf, 0, UI_INLEN);

        entry (DS_IN)
                ib = abb_in (DS_IN, (address)&uart_oset);

		// in UART_TCV we break (or move to OSS) crap with crmode
		// and keep just the 64 size limit
		if (uart_oset > 64)
			uart_oset = 64;
		memcpy (uart_ibuf, ib, uart_oset);
		ufree (ib);
		logger_in ();
		proceed (DS_IN);
endthread

// UART_TCV
#endif

#if UART_DRIVER

#define UI_TOUT         1024


thread (cmd_in)
	word quant;

	entry (CS_START)
		proceed (CS_IN);

	entry (CS_TOUT)
		dbg_9 (0x2000 | UI_TOUT);

	entry (CS_IN)
		memset (uart_ibuf, 0, UI_INLEN);

	entry (CS_READ_START)
		io (CS_READ_START, UART_A, READ, (char *)uart_ibuf, 1);
		if (uart_ibuf[0] != '\0') {
			dbg_9 (0x1000 | uart_ibuf[0]); // no START 0x00
			uart_ibuf[0] = '\0';
			proceed (CS_READ_START);
		}

	entry (CS_READ_LEN)
		delay (UI_TOUT, CS_TOUT);
		io (CS_READ_LEN, UART_A, READ, (char *)(uart_ibuf + 1), 1);
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
		quant = io (CS_READ_BODY, UART_A, READ,
				(char *)(uart_ibuf + uart_oset),
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
			when (CMD_WRITER, CS_WAIT);
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
			cmd_line = (byte *)get_mem (CS_WAIT, 8);
		else
			cmd_line = (byte *)get_mem (CS_WAIT, cmd_ctrl.oplen +1);

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

endthread

thread (dat_in)
	nodata;

	entry (DS_START)
		proceed (DS_IN);

	entry (DS_TOUT)
		dbg_9 (0x2000 | uart_oset);
		logger_in ();


	entry (DS_IN)
		memset (uart_ibuf, 0, UI_INLEN);

	entry (DS_READ_START)
		io (DS_READ_START, UART_A, READ, (char *)uart_ibuf, 1);
		if (is_crmode && uart_ibuf[0] == 0x0D) { // discard empty CRs
			uart_ibuf[0] = '\0';
			proceed (DS_READ_START);
		}
		uart_oset = 1;

	entry (DS_READ)
		delay (UI_TOUT, DS_TOUT);
		io (DS_READ, UART_A, READ, (char *)(uart_ibuf + uart_oset), 1);
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
endthread

#endif

/*static*/ void cmd_exec (word state) {

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
			
/*static*/ void cmd_out (word state) {
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


/*static*/ void def_pin (word pin) {
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

// see app.h for defaults
/*static*/ void read_eprom_and_init() {
	lword lw;
	word w[NVM_BOOT_LEN];

	nvm_read (NVM_NID, w, NVM_BOOT_LEN);
	if (w[NVM_UART] == 0xFFFF || (w[NVM_UART] != 12 && w[NVM_UART] != 24 &&
				w[NVM_UART] != 48 && w[NVM_UART] != 96 &&
				w[NVM_UART] != 192 && w[NVM_UART] != 384))
	       w[NVM_UART] = UART_RATE / 100;

#if UART_DRIVER
	ion (UART, CONTROL, (char *)&w[NVM_UART], UART_CNTRL_SETRATE);
#endif

#if UART_TCV
	tcv_control (OSFD, PHYSOPT_SETRATE, &w[NVM_UART]);
#endif

	if (w[NVM_NID] == 0xFFFF)
		net_id = 0;
	else
		net_id = w[NVM_NID];
	if (w[NVM_LH] == 0 || w[NVM_LH] == 0xFFFF)
		local_host = (nid_t)ESN;
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
				runthread (cyc_man);
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

	if (if_read (NVM_IO_STATE) != 0xFFFF) {
		// find last io state (with the counter):
		w[0] = NVM_IO_STATE +2;
		while (w[0] < (NVM_PAGE_SIZE <<1) -1 && if_read (w[0]) != 0xFFFF)
			w[0] += 2;
		w[1] = if_read (w[0] -2);
		w[2] = if_read (w[0] -1);

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
			runthread (io_rep);
		io_pload = 0xFFFFFFFF; // restore
	}
	if (io_creg >> 26 != 0)
		runthread (io_back);

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
				runthread (st_rep);
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
/*static*/ bool valid_input () {

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

#if DM2200
#define INFO_PHYS_	INFO_PHYS_DM2200
#else
// assumed CC1100.. DM2100 isa bad (?)
#define INFO_PHYS_	INFO_PHYS_CC1100
#endif

thread (root)
#if UART_TCV
	word i;
#endif
	entry (RS_INIT)
		if (net_init (INFO_PHYS_, INFO_PLUG_TARP) < 0) {
			dbg_0 (0x1000); // net_init failed, reset
			halt(); //reset();
		}
#if UART_TCV
		phys_uart (1, UART_INPUT_BUFFER_LENGTH, 0);
		tcv_plug (1, &plug_null);
		//oep_setphy (1);
		if ((OSFD =  tcv_open (WNONE, 1, 1)) < 0) {
			 dbg_0 (0x1001); // OSSI open failed, reset
			 reset();
		}

		// Initialize the PHY and AB
		i = 0xffff;
		tcv_control (OSFD, PHYSOPT_SETSID, &i);
		tcv_control (OSFD, PHYSOPT_TXON, NULL);
		tcv_control (OSFD, PHYSOPT_RXON, NULL);
		ab_init (OSFD);
		//ab_mode (AB_MODE_ACTIVE);
#endif
		read_eprom_and_init();
		(void) runthread (rcv);
#if UART_DRIVER || UART_TCV
		if (is_cmdmode)
			(void) runthread (cmd_in);
		else
			(void) runthread (dat_in);
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
			when (CMD_READER, RS_RCMD);
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

endthread

praxis_starter (NodeErt);

