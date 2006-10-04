/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "net.h"
#include "lib_apps.h"
#include "msg_tagStructs.h"
#include "msg_tags.h"

void msg_getTag_in (word state, char * buf) {
	char * out_buf = NULL;
	word pass;
	pass = check_passwd (in_getTag(buf, passwd), 0);

	msg_getTagAck_out (state, &out_buf, in_header(buf, snd),
			in_header(buf, seq_no), pass);
	//diag("get from %d seq %d", in_header(buf, snd), in_header(buf, seq_no));

	net_opt (PHYSOPT_TXON, NULL);
	send_msg (out_buf, sizeof(msgGetTagAckType));
	net_opt (PHYSOPT_TXOFF, NULL);
	ufree (out_buf);
}
void msg_getTagAck_out (word state, char** out_buf, nid_t rcv, seq_t seq, word pass) {

	if (*out_buf == NULL)
		*out_buf = get_mem (state, sizeof(msgGetTagAckType));
	else
		memset (*out_buf, 0, sizeof(msgGetTagAckType));

	in_header(*out_buf, msg_type) = msg_getTagAck;
	in_header(*out_buf, rcv) = rcv;

	in_getTagAck(*out_buf, ackSeq) = seq;
	in_getTagAck(*out_buf, ltime) = seconds();
	in_getTagAck(*out_buf, host_id) = host_id;
	in_getTagAck(*out_buf, tag) = local_host;
//	diag("get ack in %x %x pas %x %x", in_getTagAck(*out_buf, ltime), in_getTagAck(*out_buf, host_id));
	if(pass) {
		in_getTagAck(*out_buf, pow_levels) = pong_params.pow_levels;
		in_getTagAck(*out_buf, freq_maj) = pong_params.freq_maj;
		in_getTagAck(*out_buf, freq_min) = pong_params.freq_min;
		in_getTagAck(*out_buf, rx_span) = pong_params.rx_span;
		in_getTagAck(*out_buf, spare) = 0xff;
	}
}

void msg_setTag_in (word state, char * buf) {
	char * out_buf = NULL;
	word pass = check_passwd (in_setTag(buf, passwd),
			in_setTag(buf, npasswd));
	//diag("set from %d seq %d", in_header(buf, snd), in_header(buf, seq_no));

	msg_setTagAck_out (state, &out_buf, in_header(buf, snd),
			in_header(buf, seq_no), pass);
    net_opt (PHYSOPT_TXON, NULL);
	send_msg (out_buf, sizeof(msgSetTagAckType));
	net_opt (PHYSOPT_TXOFF, NULL);
	ufree (out_buf);

	if (pass)
		set_tag (buf);
}

void msg_setTagAck_out (word state, char** out_buf, nid_t rcv, seq_t seq,
			word pass) {

	if (*out_buf == NULL)
		*out_buf = get_mem (state, sizeof(msgSetTagAckType));
	else
		memset (*out_buf, 0, sizeof(msgSetTagAckType));

	in_header(*out_buf, msg_type) = msg_setTagAck;
	in_header(*out_buf, rcv) = rcv;
	in_setTagAck(*out_buf, tag) = local_host;
	in_setTagAck(*out_buf, ackSeq) = seq;
	in_setTagAck(*out_buf, ack) = (byte)pass;
}
