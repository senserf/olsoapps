/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "lib_app_if.h"
#include "msg_gene.h"
#include "tarp.h"
#include "codes.h"
#include "net.h"

void msg_cmd_in (word state, char * buf) {
	if (cmd_line != NULL) {
		wait (CMD_WRITER, state);
		release;
	}
	// dupa: butt ugly, clean this crap!
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
		set_master_chg;
		master_host  = in_header(buf, snd);
	}
	if (is_master_chg) {
		clr_master_chg;
		ee_write (EE_MID, (byte *)&master_host, 2);
		if (!running (st_rep) && br_ctrl.rep_freq >> 1)
			fork (st_rep, NULL);
	}

	// clear missed, set warn, bad and audit_freq
	if (con_miss >= con_warn)
		leds (CON_LED, LED_ON);

	// this is quite regular and frequent, don't trigger con_man
	if (in_master(buf, con) != 0) { // otherwise keep local settings
		connect = in_master(buf, con) << 8;
		freqs = freqs & 0x00FF | in_master(buf, con) & 0xFF00;
	}
	if (running (con_man)) {
		if (audit_freq == 0)
			trigger (CON_TRIG);
	} else if (audit_freq != 0)
		fork (con_man, NULL);
}

void msg_master_out (word state, char** buf_out) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgMasterType));
	else
		memset (*buf_out, 0, sizeof(msgMasterType));

	in_header(*buf_out, msg_type) = msg_master;
	in_header(*buf_out, rcv) = cmd_ctrl.t;
	in_header(*buf_out, hco) = 0;
	in_master(*buf_out, con) = freqs & 0xFF00 | (connect >> 8);
}

void msg_traceAck_in (word state, char * buf) {
	oss_traceAck_out (state, buf);
}

word msg_traceAck_out (word state, char * buf, char** out_buf) {
	word len = sizeof(msgTraceAckType) +
		in_header(buf, hoc) * sizeof(nid_t);
	if (*out_buf == NULL)
		*out_buf = get_mem (state, len);
	else
		memset (*out_buf, 0, len);


	in_header(*out_buf, msg_type) = msg_traceAck;
	in_header(*out_buf, rcv) = in_header(buf, snd);
	in_header(*out_buf, hco) = 0;
	in_traceAck(*out_buf, fcount) = in_header(buf, hoc);

	// fwd part
	memcpy (*out_buf + sizeof(msgTraceAckType), buf + sizeof(msgTraceType),
		sizeof(nid_t) * (in_header(buf, hoc) -1));

	// note that this node is counted in hoc, but is not appended yet
	memcpy (*out_buf +len - sizeof(nid_t), (char *)&local_host, sizeof(nid_t));
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

	in_header(*buf_out, msg_type) = msg_trace;
	in_header(*buf_out, rcv) = cmd_ctrl.t;
	in_header(*buf_out, hco) = cmd_line[1];
}

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

	// write to eeprom
	w[0] = net_id;
	w[1] = local_host;
	w[2] = master_host = in_bind(buf, mid);
	w[3] = in_bind(buf, encr);
	set_encr_data(in_bind(buf, encr));
	ee_write (EE_NID, (byte *)w, 8);

	connect = in_bind(buf, con) << 8;
	freqs = in_bind(buf, con) & 0xFF00; // also kills unbound beacon:
	trigger (BEAC_TRIG);

	// as requested: start st_rep on all binds
	if (!running(st_rep) && (br_ctrl.rep_freq >> 1) != 0 &&
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
	in_bind(*buf_out, con) = freqs & 0xFF00 | (connect >> 8);
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

void msg_new_in (char * buf) {
	char * out_buf = NULL;
	if (master_host == local_host) {
		oss_bindReq_out (buf);
		return;
	}
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

void msg_st_in (char * buf) {
	if (is_autoack && msg_stAck_aout (buf))
		oss_st_out (buf, YES);
	else
		oss_st_out (buf, NO);
}

int msg_st_out () {
	char * buf_out = get_mem (NONE, sizeof(msgStType) + (ESN_PACK << 2));
	if (buf_out == NULL)
		return NO;
	if (!load_esns (buf_out)) {
		ufree (buf_out);
		return -1;
	}
	in_header(buf_out, msg_type) = msg_st;
	in_header(buf_out, rcv) = master_host;
	in_header(buf_out, hco) = 0;
	in_st(buf_out, con) = connect;
	send_msg (buf_out, sizeof(msgStType) +(in_st(buf_out, count) << 2));
	ufree (buf_out);
	return YES;
}

void msg_br_in (char * buf) {
	if (is_autoack && msg_stAck_aout (buf))
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
	in_br(buf_out, esn_no) = esn_count;
	in_br(buf_out, s_no) = s_count();
	send_msg (buf_out, sizeof(msgBrType));
	ufree (buf_out);
	return YES;
}

void msg_stAck_in (char * buf) {
	if (is_brSTACK || (esns[1] >> 16) != in_stAck(buf, esn_h) ||
		(esns[1] & 0x0000FFFF) != in_stAck(buf, esn_l))
		return; // not waited for or not the one
	set_brSTACK;
	trigger (ST_ACKS);
}

bool msg_stAck_out () {
	char * buf_out = get_mem (NONE, sizeof(msgStAckType));
	if (buf_out == NULL)
		return NO;
	in_header(buf_out, msg_type) = msg_stAck;
	in_header(buf_out, rcv) = cmd_ctrl.t;
	in_header(buf_out, hco) = 0;
	memcpy (&in_stAck(buf_out, esn_l), &cmd_line[1], 2);
	memcpy (&in_stAck(buf_out, esn_h), &cmd_line[3], 2);
	send_msg (buf_out, sizeof(msgStAckType));
	ufree (buf_out);
	return YES;
}

bool msg_stAck_aout (char * buf) {
	char * buf_out = get_mem (NONE, sizeof(msgStAckType));
	if (buf_out == NULL)
		return NO;
	in_header(buf_out, msg_type) = msg_stAck;
	in_header(buf_out, rcv) = in_header(buf, snd);
	in_header(buf_out, hco) = 0;
	if (in_header(buf, msg_type) == msg_br) {
		in_stAck(buf_out, esn_h) =
		  in_stAck(buf_out, esn_l) = 0;
	} else { // msg_st
		if (in_st(buf, count) == 0) {
			in_stAck(buf_out, esn_h) =
			  in_stAck(buf_out, esn_l) = 0xFFFF;
		} else {
			memcpy (&in_stAck(buf_out, esn_l),
			  buf + sizeof(msgStType) +
			  ((in_st(buf, count) -1) <<2), 2);	
			memcpy (&in_stAck(buf_out, esn_h),
			  buf + sizeof(msgStType) +
			  ((in_st(buf, count) -1) <<2) +2, 2);
		}
	}
	send_msg (buf_out, sizeof(msgStAckType));
	ufree (buf_out);
	return YES;
}

void msg_stNack_in () {
	if (is_brSTNACK)
		return;
	set_brSTNACK;
	trigger (ST_ACKS);
}

bool msg_stNack_out (nid_t dest) {
	char * buf_out = get_mem (NONE, sizeof(msgStNackType));
	if (buf_out == NULL)
		return NO;
	in_header(buf_out, msg_type) = msg_stNack;
	in_header(buf_out, rcv) = dest;
	in_header(buf_out, hco) = 0;
	send_msg (buf_out, sizeof(msgStNackType));
	ufree (buf_out);
	return YES;
}
	

