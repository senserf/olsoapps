/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "lib_app_if.h"
#include "msg_rtag.h"
#include "tarp.h"
#include "codes.h"
#include "net.h"

void msg_cmd_in (word state, char * buf) {
	if (cmd_line != NULL) {
		wait (CMD_WRITER, state);
		release;
	}
	cmd_line = get_mem (state, in_cmd(buf, oplen) +1);
	cmd_line[0] = '\0'; 
	memcpy (cmd_line +1, buf + sizeof(msgCmdType), in_cmd(buf, oplen));

	cmd_ctrl.s_q = ADQ_MSG;
	cmd_ctrl.s = in_cmd(buf, s); 
	cmd_ctrl.p_q = ADQ_MSG;
	cmd_ctrl.p = in_cmd(buf, p); 
	cmd_ctrl.t_q = ADQ_MSG;
	cmd_ctrl.t = in_cmd(buf, t);
	cmd_ctrl.opcode = in_cmd(buf, opcode); 
	cmd_ctrl.opref = in_cmd(buf, opref); 
	cmd_ctrl.oprc = in_cmd(buf, oprc);
	cmd_ctrl.oplen = in_cmd(buf, oplen); 
	// resolve master / local at proxy
	if (cmd_ctrl.oprc == RC_NONE) {
		switch (cmd_ctrl.t) {
			case ADQ_LOCAL << 8:
				cmd_ctrl.t = 0;
				cmd_ctrl.t_q = ADQ_LOCAL;
				break;

			case ADQ_MASTER << 8:
				cmd_ctrl.t = 0;
				cmd_ctrl.t_q = ADQ_MASTER;
				break;

			default:
				cmd_ctrl.t_q = ADQ_MSG;
		}
	} else
		cmd_ctrl.t_q = ADQ_MSG;

	trigger (CMD_READER);
}

void msg_master_in (char * buf) {
	lword md;
	bool  upd = NO;

	if (master_host != in_header(buf, snd)) {
		dbg_2 (0xBB00 | is_master_chg);
		set_master_chg ();
		master_host  = in_header(buf, snd);
	}

	in_master(buf, mtime) = ntowl(in_master(buf, mtime));
	master_delta = in_master(buf, mtime) - seconds();
	if (is_master_chg) {
		clr_master_chg;
		upd = YES;
	}
	if ((md = in_master(buf, mtime) - seconds()) != master_delta) {
		upd = YES;
		master_delta = md;
	}
	if (in_master(buf, cyc_ap) != cyc_ap) {
		upd = YES;
		cyc_ap = in_master(buf, cyc_ap);
	}
	if (in_master(buf, cyc_sp) != cyc_sp) {
		upd = YES;
		cyc_sp = in_master(buf, cyc_sp);
	}
	if (in_master(buf, cyc_ctrl) != (cyc_ctrl & 0x03)) {
		upd = YES;
		if (in_master(buf, cyc_ctrl) & 0x01)
			cyc_ctrl |= 0x01;
		if (in_master(buf, cyc_ctrl) & 0x02)
			cyc_ctrl |= 0x02;
	}
	if (upd)
		killall (cyc_man);
	if (cyc_ap || cyc_sp)
		fork (cyc_man, NULL);
}
#if 0
void msg_master_out (word state, char** buf_out, nid_t rcv) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgMasterType));
	else
		memset (*buf_out, 0, sizeof(msgMasterType));

	in_header(*buf_out, msg_type) = msg_master;
	in_header(*buf_out, rcv) = rcv;
	in_master(*buf_out, mtime) = wtonl(seconds());
}
#endif
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
	memcpy (*out_buf +len - sizeof(nid_t), (char *)&local_host,
			sizeof(nid_t));
	return len;
}

void msg_trace_in (word state, char * buf) {
	char * out_buf = NULL;
	word len;

	len = msg_traceAck_out (state, buf, &out_buf);
	if (out_buf) {
		send_msg (out_buf, len);
		ufree (out_buf);
		out_buf = NULL;
	}
}

void msg_trace_out (word state, char** buf_out, nid_t rcv) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgTraceType));
	else
		memset (*buf_out, 0, sizeof(msgTraceType));

	in_header(*buf_out, msg_type) = msg_trace;
	in_header(*buf_out, rcv) = rcv;
	in_header(*buf_out, hco) = 0;
}

void msg_bind_in (word state, char * buf) {
	char * out_buf;
	nid_t nid = net_opt (PHYSOPT_GETSID, NULL);
	if (nid == in_bind(buf, nid)) {
		diag ("Failed bind %d to %d", nid, in_bind(buf, nid));
		return;
	}
	if (in_bind(buf, msw_hid) != 0 &&
		in_bind(buf, msw_hid) != (word)(host_id >> 16))
		return;
	if ((nid = in_bind(buf, nid)) == 0) // unbind
		reset();

	// get mem 1st
	out_buf =  get_mem (state, sizeof(msgNewType));

	// set what there is to set
	net_opt (PHYSOPT_SETSID, &nid);
	if (in_bind(buf, lh))
		local_host = in_bind(buf, lh);
	else
		local_host = host_id;
	master_host = in_bind(buf, mid);
	master_delta = in_bind(buf, mtime) - seconds();
	cyc_sp = in_bind(buf, cyc_sp);
	cyc_ctrl = in_bind(buf, cyc_ctrl);
	cyc_ap = in_bind(buf, cyc_ap);
	killall (cyc_man);
	if (cyc_ap || cyc_sp)
		fork (cyc_man, NULL);

	// send	msg_new
	if (master_host) // to master
		in_header(out_buf, rcv) = master_host;
	else // to sender
		in_header(out_buf, rcv) = in_header(buf, snd);
	in_header(out_buf, msg_type) = msg_new;
	in_header(out_buf, hco) = 0;
	in_new(out_buf, hid) = host_id;
	in_new(out_buf, nid) = nid;
	in_new(out_buf, mid) = master_host;
	send_msg (out_buf, sizeof (msgNewType));
	ufree (out_buf);
}

void msg_bind_out (word state, char** buf_out) {
	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgBindType));
	else
		memset (*buf_out, 0, sizeof(msgBindType));
	in_header(*buf_out, msg_type) = msg_bind;
	in_header(*buf_out, rcv) = cmd_ctrl.t;
	memcpy (&in_bind(*buf_out, msw_hid), &cmd_line[1], sizeof(nid_t));
	memcpy (&in_bind(*buf_out, lh), &cmd_line[1 + sizeof(nid_t)],
			sizeof(nid_t));
	if (cmd_line[1 + 2*sizeof(nid_t)] == 0) // unbind
		in_bind(*buf_out, nid) = 0;
	else // bind to my nid
		in_bind(*buf_out, nid) = net_opt (PHYSOPT_GETSID, NULL);
	in_header(*buf_out, hco) = cmd_line[1 + 2*sizeof(nid_t) +1]; // range
	in_bind(*buf_out, mtime) = seconds() + master_delta;
	in_bind(*buf_out, mid) = master_host;
	in_bind(*buf_out, cyc_sp) = cyc_sp;
	in_bind(*buf_out, cyc_ctrl) = cyc_ctrl;
	in_bind(*buf_out, cyc_ap) = cyc_ap;
}
