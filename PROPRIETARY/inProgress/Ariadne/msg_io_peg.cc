/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "diag.h"
#include "app_peg.h"
#include "tcvplug.h"
#include "msg_peg.h"
#include "net.h"
#include "tarp.h"
#include "app_peg_data.h"

void msg_master_out () {

	char * buf_out = get_mem (WNONE, sizeof(msgMasterType));

	if (buf_out == NULL)
		return;

	in_header(buf_out, msg_type) = msg_master;
	in_header(buf_out, rcv) = 0;
	in_header(buf_out, hco) = 1;
	in_master(buf_out, attr).w = sattr[2].w;
	if (is_all_out) // overload power level (0..7) with all_out
		in_master(buf_out, attr).w |= 0x0080;
	send_msg (buf_out, sizeof (msgMasterType));
	ufree (buf_out);
}

void msg_master_in (char * buf) {

	if (in_master(buf, attr).w & 0x80) { // all_out kludge
		set_all_out;
		in_master(buf, attr).w &= 0xFF7F;
	} else
		clr_all_out;

	if (sstate == ST_WARM) {
		if (in_master(buf, attr).w == sattr[2].w)
			return;

		app_diag (D_WARNING, "ST_WARMUP: attr change %x %x",
				sattr[2].w, in_master(buf, attr).w);
	}

	net_opt (PHYSOPT_TXOFF, NULL);
	net_qera (TCV_DSP_XMT);
	net_qera (TCV_DSP_RCV); // FIXME now, is it garbage?
	if (master_host != in_header(buf, snd)) { // should be in TARP
		app_diag (D_SERIOUS, "Mayday master %u %u", master_host,
			in_header(buf, snd));
		master_host = in_header(buf, snd);
	}
	clr_master_chg;
	set_state (ST_WARM);
	killall (m_cyc); 
	killall (m_sil);
	sattr[3].w = sattr[2].w = in_master(buf, attr).w;
	set_rf (1);
	stats (NULL);
}

void msg_ping_out (char * buf, word rssi) {

	char * buf_out;

	if (buf && sattr[2].w != in_ping(buf, out_sattr).w) {
		app_diag (D_DEBUG, "Sattr mismatch %x %x", sattr[2].w,
				in_ping(buf, out_sattr).w);
		return;
	}

	if ((buf_out = get_mem (WNONE, sizeof(msgPingType)))  == NULL)
		return;

	in_header(buf_out, msg_type) = msg_ping;
	in_header(buf_out, rcv) = 0;
	in_header(buf_out, hco) = 1;
	in_ping(buf_out, out_sattr).w = sattr[2].w;
	in_ping(buf_out, pings) = ++touts.mc;
	if (buf == NULL) { // seed
		in_ping(buf_out, in_sattr).w = 0;
		in_ping(buf_out, rssi) = 0;
		in_ping(buf_out, id) = 0;
		in_ping(buf_out, seqno) = 0;
	} else {
		in_ping(buf_out, in_sattr).w = in_ping(buf, out_sattr).w;
		in_ping(buf_out, rssi) = rssi;
		in_ping(buf_out, id) = in_header(buf, snd);
		in_ping(buf_out, seqno) = in_header(buf, seq_no);
	}

	send_msg (buf_out, sizeof (msgPingType));
	ufree (buf_out);
}

void msg_ping_in (char * buf, word rssi) {

	if (sstate == ST_WARM) {
		set_state (ST_CYC);
		net_opt (PHYSOPT_TXON, NULL);
	} else
		clr_ping_out;

	if (sstate != ST_CYC) {
		app_diag (D_DEBUG, "Ping dropped in %u", sstate);
		return;
	}

	if (is_uart_out) {
		if (oss_stack <= MAX_OSS_STACK)
			oss_ping_out (buf, rssi);
		else
			cters[2]++;
	}

	if (net_qsize (TCV_DSP_XMT) < DEF_XMT_THOLD) {
		if (is_all_out || local_host < 0x100 ||
			       	in_header(buf, snd) < 0x100) {
			msg_ping_out (buf, rssi);
		}
	} else
		cters[1]++;
}

// just 'i' for now, I'm not sure how useful this can be...
void msg_cmd_out (byte cmd, word dst, word a) {

	char * buf_out = get_mem (WNONE, sizeof(msgCmdType));
	word toff = net_opt (PHYSOPT_STATUS, NULL) & 2 ? 0 : 1;

	if (buf_out == NULL)
		return;

	in_header(buf_out, msg_type) = msg_cmd;
	in_header(buf_out, rcv) = dst;
	in_header(buf_out, hco) = 1;
	in_cmd(buf_out, cmd) = cmd;
	in_cmd(buf_out, a) = a;

	if (toff)
		net_opt (PHYSOPT_TXON, NULL);

	send_msg (buf_out, sizeof (msgCmdType));
	ufree (buf_out);

	if (toff)
		net_opt (PHYSOPT_TXOFF, NULL);
}

void msg_cmd_in (char * buf) {

	if (in_cmd(buf, cmd) != 'i') {
		app_diag (D_WARNING, "Unknown cmd %c", in_cmd(buf, cmd));
		return;
	}

	if (sstate == ST_CYC)
		app_diag (D_WARNING, "Can't update lh in ST_CYC");
	else
		local_host = in_cmd(buf, a);
	msg_stats_out();
}

void msg_sil_out () {

	char * buf_out = get_mem (WNONE, sizeof(msgSilType));

	if (buf_out == NULL)
		return;

	in_header(buf_out, msg_type) = msg_sil;
	in_header(buf_out, rcv) = 0;
	in_header(buf_out, hco) = 1;
	send_msg (buf_out, sizeof (msgSilType));
	ufree (buf_out);
}

void msg_sil_in (char * buf) {

	if (master_host != in_header(buf, snd)) {
		app_diag (D_WARNING, "Ignored silence: not master");
		return;
	}
	set_state (ST_COOL);
	net_opt (PHYSOPT_TXOFF, NULL);
	net_qera (TCV_DSP_XMT);
	net_qera (TCV_DSP_RCV);
}

void msg_stats_out () {

	word mmin, mem;
	word toff = net_opt (PHYSOPT_STATUS, NULL) & 2 ? 0 : 1;
	char * buf_out = get_mem (WNONE, sizeof(msgStatsType));

	if (buf_out == NULL)
		return;

	in_header(buf_out, msg_type) = msg_stats;
	in_header(buf_out, rcv) = 0;
	in_header(buf_out, hco) = 1;
	in_stats(buf_out, hid) = host_id;
	in_stats(buf_out, secs) = seconds();
	in_stats(buf_out, lh) = local_host;
	in_stats(buf_out, mh) = master_host;
	memcpy (&in_stats(buf_out, to), &touts, sizeof(tout_t));
	in_stats(buf_out, satt[0]).w = sattr[0].w;
	in_stats(buf_out, satt[1]).w = sattr[1].w;
	in_stats(buf_out, fl) = app_flags;
	mem = memfree(0, &mmin);
	in_stats(buf_out, mem) = mem;
	in_stats(buf_out, mmin) = mmin;

	if (toff)
		net_opt (PHYSOPT_TXON, NULL);

	send_msg (buf_out, sizeof (msgStatsType));
	ufree (buf_out);

	if (toff)
		net_opt (PHYSOPT_TXOFF, NULL);
}

void msg_stats_in (char * buf) {

	if (!is_uart_out)
		return
	stats (buf);
}
