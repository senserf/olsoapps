/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "lib_app_if.h"
#include "msg_vmesh.h"
#include "tarp.h"
#include "codes.h"
#include "net.h"
#include "nvm.h"

void msg_cmd_in (word state, char * buf) {
	if (cmd_line != NULL) {
		wait (CMD_WRITER, state);
		release;
	}
	if (in_cmd(buf, opcode) == CMD_GET && in_cmd(buf, oplen) == 1)
		cmd_line = get_mem (state, 8);
	else
		cmd_line = get_mem (state, in_cmd(buf, oplen) +1);
	cmd_line[0] = '\0'; 
	memcpy (cmd_line +1, buf + sizeof(msgCmdType), in_cmd(buf, oplen));

	// if !RC_NONE, snd was the target, otherwise it is I
	if ((cmd_ctrl.oprc = in_cmd(buf, oprc)) == RC_NONE)
		cmd_ctrl.t = local_host;
	else
		cmd_ctrl.t = in_header(buf, snd);
	cmd_ctrl.s = in_header(buf, snd); // that's always true
	cmd_ctrl.opcode = in_cmd(buf, opcode); 
	cmd_ctrl.opref = in_cmd(buf, opref); 
	cmd_ctrl.oplen = in_cmd(buf, oplen); 
	trigger (CMD_READER);
}

void msg_cmd_out (word state, char** buf_out) {
	if (*buf_out != NULL)
		ufree (*buf_out); // length may be bad
	*buf_out = get_mem (state, sizeof(msgCmdType) + cmd_ctrl.oplen);
	in_header(*buf_out, msg_type) = msg_cmd;
	// same trick:
	if (cmd_ctrl.oprc == RC_NONE)
		in_header(*buf_out, rcv) = cmd_ctrl.t;
	else
		in_header(*buf_out, rcv) = cmd_ctrl.s;
	in_header(*buf_out, hco) = 0;
	in_cmd(*buf_out, opcode) = cmd_ctrl.opcode;
	in_cmd(*buf_out, opref) = cmd_ctrl.opref;
	in_cmd(*buf_out, oplen) = cmd_ctrl.oplen;
	in_cmd(*buf_out, oprc) = cmd_ctrl.oprc;
	if (cmd_ctrl.oplen)
		 memcpy (*buf_out+sizeof(msgCmdType), cmd_line +1,
			 cmd_ctrl.oplen);
}

void msg_master_in (char * buf) {
	if (master_host != in_header(buf, snd)) {
		dbg_2 (0xBB00 | is_master_chg);
		set_master_chg ();
		master_host  = in_header(buf, snd);
	}
	if (is_master_chg) {
		clr_master_chg;
		nvm_write (NVM_MID, &master_host, 1);
		if (is_cmdmode && !running (st_rep) && br_ctrl.rep_freq >> 1)
			fork (st_rep, NULL);
		if (running (cyc_man)) {
			kill (running (cyc_man));
			cyc_ctrl.st = CYC_ST_ENA;
		}
	}

	// clear missed, set warn, bad and audit_freq
	if (con_miss >= con_warn)
		leds (CON_LED, LED_ON);

	// this is quite regular and frequent, don't trigger con_man
	if (in_master(buf, con) != 0) { // otherwise keep local settings
		connect = in_master(buf, con) << 8;
		freqs = (freqs & 0x00FF) | (in_master(buf, con) & 0xFF00);
	}
	if (running (con_man)) {
		if (audit_freq == 0)
			trigger (CON_TRIG);
	} else if (audit_freq != 0)
		fork (con_man, NULL);

	if (in_master(buf, cyc) == CYC_MSG_FORCE_DIS) { // specialties
		if (running (cyc_man))
			kill (running (cyc_man));
		if (cyc_ctrl.st != CYC_ST_DIS) {
			cyc_ctrl.st = CYC_ST_DIS;
			nvm_write (NVM_CYC_CTRL, (address)&cyc_ctrl, 1);
		}
		return;
	}
	if (in_master(buf, cyc) == CYC_MSG_FORCE_ENA) {
		if (running (cyc_man))
			kill (running (cyc_man));
		if (cyc_ctrl.st != CYC_ST_ENA /*&& cyc_ctrl.st != CYC_ST_DIS*/)
			cyc_ctrl.st = CYC_ST_ENA;
		return;
	}	

	if (in_master(buf, cyc) != 0 && cyc_ctrl.st == CYC_ST_ENA) {
		if (running (cyc_man)) { // bad
			dbg_2 (0xC200 | cyc_ctrl.st);
			kill (running (cyc_man));
		}
		cyc_ctrl.prep = in_master(buf, cyc);
		cyc_ctrl.st = CYC_ST_PREP;
		fork (cyc_man, NULL);
	}
}

void msg_master_out (word state, char** buf_out) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgMasterType));
	else
		memset (*buf_out, 0, sizeof(msgMasterType));

	in_header(*buf_out, msg_type) = msg_master;
	in_header(*buf_out, rcv) = cmd_ctrl.t;
	in_header(*buf_out, hco) = 0;
	//in_master(*buf_out, con) = freqs & 0xFF00 | (connect >> 8);
}

void msg_traceAck_in (word state, char * buf) {
	oss_traceAck_out (state, buf);
}

word msg_traceAck_out (word state, char * buf, char** out_buf) {
	word len;
	if (in_header(buf, msg_type) != msg_traceB)
		len = sizeof(msgTraceAckType) +
			in_header(buf, hoc) * sizeof(nid_t);
	else
		len = sizeof(msgTraceAckType) + sizeof(nid_t);
	if (*out_buf == NULL)
		*out_buf = get_mem (state, len);
	else
		memset (*out_buf, 0, len);
	switch (in_header(buf, msg_type)) {
		case msg_traceF:
			in_header(*out_buf, msg_type) = msg_traceFAck;
			break;
		case msg_traceB:
			in_header(*out_buf, msg_type) = msg_traceBAck;
			break;
		default:
			in_header(*out_buf, msg_type) = msg_traceAck;
	}
	in_header(*out_buf, rcv) = in_header(buf, snd);
	in_header(*out_buf, hco) = 0;
	in_traceAck(*out_buf, fcount) = in_header(buf, hoc);

	// fwd part
	if (in_header(buf, msg_type) != msg_traceB && in_header(buf, hoc) > 1)
		memcpy (*out_buf + sizeof(msgTraceAckType),
			buf + sizeof(msgTraceType),
			sizeof(nid_t) * (in_header(buf, hoc) -1));

	// note that this node is counted in hoc, but is not appended yet
	memcpy (*out_buf + len - sizeof(nid_t),
		(char *)&local_host, sizeof(nid_t));
	return len;
}

void msg_trace_in (word state, char * buf) {
	char * out_buf = NULL;
	word len;

	len = msg_traceAck_out (state, buf, &out_buf);
	if (out_buf) {
		send_msg (out_buf, len);
		ufree (out_buf);
	}
}

void msg_trace_out (word state, char** buf_out) {
	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgTraceType));
	else
		memset (*buf_out, 0, sizeof(msgTraceType));

	switch (cmd_line[2]) {
		case 0:
			in_header(*buf_out, msg_type) = msg_traceB;
			break;
		case 1:
			in_header(*buf_out, msg_type) = msg_traceF;
			break;
		default:
			in_header(*buf_out, msg_type) = msg_trace;
	}
	in_header(*buf_out, rcv) = cmd_ctrl.t;
	in_header(*buf_out, hco) = cmd_line[1];
}

extern tarpCtrlType tarp_ctrl;
void msg_bind_in (char * buf) {
	word w[4];
	
	// nid from the chip is either 0 or the same as the message's
	if (net_id == in_bind(buf, nid))
		return;

	if ((in_bind(buf, esn_h) != 0 || in_bind(buf, esn_l) != 0) &&
		ESN != (((lword)in_bind(buf, esn_h) << 16) |
			in_bind(buf, esn_l)))
		return; // not mine, not a bulk binding
	if ((net_id = in_bind(buf, nid)) == 0) // just in case, should NOT be
		reset();
	net_opt (PHYSOPT_SETSID, &net_id);
	if (in_bind(buf, lh))
		local_host = in_bind(buf, lh);
	// else leave it as is

	// write to nvm
	w[NVM_NID] = net_id;
	w[NVM_LH] = local_host;
	w[NVM_MID] = master_host = in_bind(buf, mid);
	set_encr_data(in_bind(buf, encr));
	w[NVM_APP] = encr_data;
	if (is_binder)
		w[NVM_APP] |= 1 << 4;
	if (is_cmdmode)
		w[NVM_APP] |= 1 << 5;
	w[NVM_APP] |= tarp_ctrl.param << 8;
	nvm_write (NVM_NID, w, 4);

	connect = in_bind(buf, con) << 8;
	freqs = in_bind(buf, con) & 0xFF00; // also kills unbound beacon:
	trigger (BEAC_TRIG);

	// as requested: start st_rep on all binds
	if (is_cmdmode && !running(st_rep) && (br_ctrl.rep_freq >> 1) != 0 &&
			master_host != 0)
		fork (st_rep, NULL);
}

void msg_bind_out (word state, char** buf_out) {
	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgBindType));
	else
		memset (*buf_out, 0, sizeof(msgBindType));
	in_header(*buf_out, msg_type) = msg_bind;
	in_header(*buf_out, rcv) = 0;
	memcpy (&in_bind(*buf_out, esn_l), &cmd_line[1], sizeof(lword));
	memcpy (&in_bind(*buf_out, lh), &cmd_line[5], sizeof(nid_t));
	in_bind(*buf_out, nid) = net_id;
	in_header(*buf_out, hco) = cmd_line[7]; // range
	in_bind(*buf_out, mid) = master_host;
	in_bind(*buf_out, con) = (freqs & 0xFF00) | (connect >> 8);
	in_bind(*buf_out, encr) = encr_data;
}

void msg_bindReq_in (char * buf) { 
	oss_bindReq_out (buf);
}

bool msg_bindReq_out (char * buf, char** buf_out) {
	if (*buf_out == NULL) {
		*buf_out = get_mem (NONE, sizeof(msgBindReqType));
		if (*buf_out == NULL)
			return NO;
	} else
		 memset (*buf_out, 0, sizeof(msgBindReqType));
	in_header(*buf_out, msg_type) = msg_bindReq; 
	in_header(*buf_out, rcv) = master_host; 
	in_header(*buf_out, hco) = 0; 
	in_bindReq(*buf_out, esn_l) = in_new(buf, esn_l);
	in_bindReq(*buf_out, esn_h) = in_new(buf, esn_h); 
	in_bindReq(*buf_out, lh) = in_header(buf, snd); 
	return YES;
}

void msg_nhAck_in (char * buf) {
	oss_nhAck_out (buf);
}

bool msg_nhAck_out (char * buf, char** buf_out) {
	if (*buf_out == NULL) {
		*buf_out = get_mem (NONE, sizeof(msgNhAckType));
		if (*buf_out == NULL)
			return NO;
	} else
		memset (*buf_out, 0, sizeof(msgNhAckType));
	in_header(*buf_out, msg_type) = msg_nhAck;
	in_header(*buf_out, rcv) = in_nh(buf, host);
	in_header(*buf_out, hco) = 0;
	in_nhAck(*buf_out, host) = in_header(buf, snd);
	in_nhAck(*buf_out, esn_l) = ESN;
	in_nhAck(*buf_out, esn_h) = ESN >> 16;
	return YES;
}

void msg_new_in (char * buf) {
	char * out_buf = NULL;
	if (master_host == local_host) {
		oss_bindReq_out (buf);
		return;
	}
	if (!is_binder)
		return;
	if (msg_bindReq_out (buf, &out_buf)) {
		send_msg (out_buf, sizeof(msgBindReqType));
		ufree (out_buf);
	}
}

bool msg_new_out () {
	char * buf_out = get_mem (NONE, sizeof(msgNewType));
	if (buf_out == NULL)
		return NO;
	in_header(buf_out, msg_type) = msg_new;
	in_header(buf_out, rcv) = 0;
	in_header(buf_out, hco) = 1;
	in_new(buf_out, esn_l) = ESN;
	in_new(buf_out, esn_h) = ESN >> 16;
	send_msg (buf_out, sizeof(msgNewType));
	if (beac_freq == 0 || running(beacon)) {
		ufree (buf_out);
	} else {
		if (fork (beacon, buf_out) == 0) {
			dbg_2 (0xC402); // beacon fork failed
			ufree (buf_out);
			return NO;
		}
	}
	return YES;
}

void msg_nh_in (char * buf) {
	char * out_buf = NULL;
	if (in_nh(buf, host) == local_host) {
		oss_nhAck_out (buf);
		return;
	}
	if (msg_nhAck_out (buf, &out_buf)) {
		send_msg (out_buf, sizeof(msgNhAckType));
		ufree (out_buf);
	}
}

bool msg_nh_out () {
	char * buf_out = get_mem (NONE, sizeof(msgNhType));
	if (buf_out == NULL)
		return NO;
	in_header(buf_out, msg_type) = msg_nh;
	in_header(buf_out, rcv) = 0;
	in_header(buf_out, hco) = 1;
	in_nh(buf_out, host) = cmd_ctrl.s;
	send_msg (buf_out, sizeof(msgNhType));
	ufree (buf_out);
	return YES;
}

void msg_alrm_in (char * buf) {
	oss_alrm_out (buf);
}

bool msg_alrm_out (char * buf) {
	char * buf_out = get_mem (NONE, sizeof(msgAlrmType) + buf[1] - 9 + 1);
	if (buf_out == NULL)
		return NO;
	in_header(buf_out, msg_type) = msg_alrm;
	in_header(buf_out, rcv) = master_host;
	in_header(buf_out, hco) = 0;
	in_alrm(buf_out, typ) = buf[3];
	in_alrm(buf_out, rev) = buf[4];
	memcpy (&in_alrm(buf_out, esn_l), buf +5, 2);
	memcpy (&in_alrm(buf_out, esn_h), buf +7, 2);
	in_alrm(buf_out, s) = buf[9];
	in_alrm(buf_out, rssi) = buf[10 + buf[1] - 9];
	// the length is not in the msgAlrm struct, to avoid 1 byte loss
	buf_out[sizeof(msgAlrmType)] = buf[1] - 9;
	memcpy (buf_out + sizeof(msgAlrmType) + 1, buf + 10, buf[1] - 9);
	net_opt (PHYSOPT_CAV, NULL);
	send_msg (buf_out, sizeof(msgAlrmType) + buf[1] - 9 + 1);
	ufree (buf_out);
	return YES;
}

void msg_io_in (char * buf) {
	if (is_autoack && msg_ioAck_out (buf))
		oss_io_out (buf, YES);
	else
		oss_io_out (buf, NO);
}

bool msg_io_out () {
	char * buf_out = get_mem (NONE, sizeof(msgIoType));
	if (buf_out == NULL)
		return NO;
	in_header(buf_out, msg_type) = msg_io;
	in_header(buf_out, rcv) = master_host;
	in_header(buf_out, hco) = 0;
	in_io(buf_out, pload) = io_pload;
	send_msg (buf_out, sizeof(msgIoType));
	ufree (buf_out);
	return YES;
}

void msg_br_in (char * buf) {
	if (is_autoack && msg_stAck_out (buf))
		oss_br_out (buf, YES);
	else
		oss_br_out (buf, NO);
}

bool msg_br_out() {
	char * buf_out = get_mem (NONE, sizeof(msgBrType));
	if (buf_out == NULL)
		 return NO;
	in_header(buf_out, msg_type) = msg_br;
	in_header(buf_out, rcv) = master_host;
	in_header(buf_out, hco) = 0;
	in_br(buf_out, con) = connect;
	send_msg (buf_out, sizeof(msgBrType));
	ufree (buf_out);
	return YES;
}

void msg_dat_in (char * buf) {
	if ((in_dat(buf, ref) & 0x80) && is_autoack && msg_datAck_out (buf))
		oss_dat_out (buf, YES);
	else
		oss_dat_out (buf, NO);
}

// this is called only for CMD_DAT. Data mode goes out directly from
// lib_app.c::dat_rep().
word msg_dat_out () {
	char * buf_out;
	buf_out = get_mem (NONE, sizeof(msgDatType) + cmd_ctrl.oplen);
	if (buf_out == NULL)
		return RC_EMEM;
	in_header(buf_out, msg_type) = msg_dat;
	in_header(buf_out, rcv) = cmd_ctrl.t;
	in_header(buf_out, hco) = 0;
	in_dat(buf_out, ref) = cmd_ctrl.opref;
	in_dat(buf_out, len) = cmd_ctrl.oplen;
	memcpy (buf_out + sizeof(msgDatType), cmd_line +1, cmd_ctrl.oplen);
	send_msg (buf_out, sizeof(msgDatType) + cmd_ctrl.oplen);
	ufree (buf_out);
	return RC_OK;
}

void msg_stAck_in (char * buf) {
	if (is_brSTACK)
		return; // not waited for
	set_brSTACK;
	trigger (ST_ACKS);
}

bool msg_stAck_out (char * buf) {
	char * buf_out = get_mem (NONE, sizeof(msgStAckType));
	if (buf_out == NULL)
		return NO;
	in_header(buf_out, msg_type) = msg_stAck;
	if (buf)
		in_header(buf_out, rcv) = in_header(buf, snd);
	else
		in_header(buf_out, rcv) = cmd_ctrl.t;
	in_header(buf_out, hco) = 0;
	send_msg (buf_out, sizeof(msgStAckType));
	ufree (buf_out);
	return YES;
}

void msg_ioAck_in (char * buf) {
	if (is_ioACK || io_pload != in_ioAck(buf, pload))
		return;
	set_ioACK;
	trigger (IO_ACK_TRIG);
}

bool msg_ioAck_out (char * buf) {
	char * buf_out = get_mem (NONE, sizeof(msgIoAckType));
	if (buf_out == NULL)
		return NO;
	in_header(buf_out, msg_type) = msg_ioAck;
	in_header(buf_out, hco) = 0;
	if (buf == NULL) { // cmd line
		in_header(buf_out, rcv) = cmd_ctrl.t;
		memcpy (&in_ioAck(buf_out, pload), cmd_line +1, 4);
	} else { // msg_io in
		in_header(buf_out, rcv) = in_header(buf, snd);
		in_ioAck(buf_out, pload) = in_io(buf, pload);
	}
	send_msg (buf_out, sizeof(msgIoAckType));
	ufree (buf_out);
	return YES;
}

void msg_datAck_in (char * buf) {
	if (is_cmdmode) {
		oss_datack_out (buf);
		return;
	}
	// in dat mode; clear retries:
	if (is_datACK || (dat_seq | 0x80) != in_datAck(buf, ref))
		return;
	set_datACK;
	trigger (DAT_ACK_TRIG);
}

bool msg_datAck_out (char * buf) {
	char * buf_out = get_mem (NONE, sizeof(msgDatAckType));
	if (buf_out == NULL)
		return NO;
	in_header(buf_out, msg_type) = msg_datAck;
	in_header(buf_out, hco) = 0;
	if (buf == NULL) { // cmd line
		in_header(buf_out, rcv) = cmd_ctrl.t;
		in_datAck(buf_out, ref) = cmd_line[1];
	} else { //msg_dat in
		in_header(buf_out, rcv) = in_header(buf, snd);
		in_datAck(buf_out, ref) = in_dat(buf, ref);
	}
	send_msg (buf_out, sizeof(msgDatAckType));
	ufree (buf_out);
	return YES;
}

