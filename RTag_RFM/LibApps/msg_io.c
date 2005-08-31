/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "lib_app_if.h"
#include "msg_rtag.h"
#include "tarp.h"
#include "codes.h"
#if 0
void msg_info_in (char * buf) {
	in_info(buf, ltime) = ntowl(in_info(buf, ltime));
	in_info(buf, m_delta) = ntowl(in_info(buf, m_delta));
	oss_info_out (buf, oss_fmt);
}

void msg_disp_in (word state, char * buf) {
#if UART_DRIVER
	if (cmd_line != NULL) { // busy with another input
		wait (CMD_WRITER, state);
		release;
	}
	cmd_line = get_mem (state, in_disp(buf, len) +1);
	cmd_line[0] = '\0';
	memcpy (cmd_line +1, buf + sizeof(msgDispType), in_disp(buf, len));

	cmd_ctrl.s_q = OPQUAL_MSG;	
	cmd_ctrl.s = in_header(buf, snd);
	cmd_ctrl.p_q = OPQUAL_OSS;
	cmd_ctrl.p = local_host;
	cmd_ctrl.t_q = OPQUAL_OSS;
	cmd_ctrl.t = in_header(buf, rcv); // can be bcast, show it
	cmd_ctrl.opcode = CMD_DISP;
	cmd_ctrl.oprc = RC_NONE;
	cmd_ctrl.oplen = in_disp(buf, len);

	trigger (CMD_READER);
#endif
}
#endif
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

#if 0
void msg_info_out (word state, char** buf_out, id_t rcv) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgInfoType));
	else
		memset (*buf_out, 0, sizeof(msgInfoType));

	in_header(*buf_out, msg_type) = msg_info;
	in_header(*buf_out, rcv) = rcv;
	in_info(*buf_out, ltime) = wtonl(seconds());
	in_info(*buf_out, host_id) = host_id;
	in_info(*buf_out, m_host) = master_host;
	in_info(*buf_out, m_delta) = wtonl(master_delta);
	in_info(*buf_out, pl) = pow_level;
}
#endif
void msg_master_in (char * buf) {
	in_master(buf, mtime) = ntowl(in_master(buf, mtime));
	master_delta = in_master(buf, mtime) - seconds();
	master_host  = in_header(buf, snd); // blindly, for now
}

void msg_master_out (word state, char** buf_out, id_t rcv) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgMasterType));
	else
		memset (*buf_out, 0, sizeof(msgMasterType));

	in_header(*buf_out, msg_type) = msg_master;
	in_header(*buf_out, rcv) = rcv;
	in_master(*buf_out, mtime) = wtonl(seconds());
}

word msg_traceAck_out (word state, char * buf, char** out_buf) {
	word len = sizeof(msgTraceAckType) +
		in_header(buf, hoc) * sizeof(id_t);
	if (*out_buf == NULL)
		*out_buf = get_mem (state, len);
	else
		memset (*out_buf, 0, len);


	in_header(*out_buf, msg_type) = msg_traceAck;
	in_header(*out_buf, rcv) = in_header(buf, snd);
	in_traceAck(*out_buf, fcount) = in_header(buf, hoc);

	// fwd part
	memcpy (*out_buf + sizeof(msgTraceAckType), buf + sizeof(msgTraceType),
		sizeof(id_t) * (in_header(buf, hoc) -1));

	// note that this node is counted in hoc, but is not appended yet
	memcpy (*out_buf +len - sizeof(id_t), (char *)&local_host, sizeof(id_t));
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

void msg_trace_out (word state, char** buf_out, id_t rcv) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgTraceType));
	else
		memset (*buf_out, 0, sizeof(msgTraceType));

	in_header(*buf_out, msg_type) = msg_trace;
	in_header(*buf_out, rcv) = rcv;
}
