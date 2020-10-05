/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

/* ser -specific ossi (setting praxis-specific cmd_d struct) */

#include "ser.h"
#include "serf.h"
#include "app.h"
#include "app_dcl.h"
#include "oss.h"
#include "oss_dcl.h"
#include "str_ser.h"
#include "msg.h"
#include "msg_dcl.h"
#include "sensors.h"
#include "net.h"
#include "storage.h"

// msg_isTrace tells TARP to morph packets. msg_traceB doesn't morph on the
// way forward, but does so backwards. For our needs here, we shouldn't really
// use msg_isTrace (we could msg_isTrace(m) || m == msg_traceB).
#define msg_seedsTr(m)	((m) == msg_trace || (m) == msg_trace1 || \
	(m) == msg_traceF || (m) == msg_traceB)

void run_oo (out_t * o) {
	if (runfsm ossi_out ((char *)o) == 0) {
		diag ("fork oss");
		if (o->fl_b)
			ufree (o->buf);
		ufree (o);
	}
}

#define out_d	((out_t *)data)
fsm ossi_out (char *) {

	entry OO_INIT:
		if (data == NULL || out_d->buf == NULL)
			goto Fin;

		// if the actual buffer contains empty string, it is interpreted
		// as a binary command... and bad things happen
		if (*(out_d->buf) == '\0') {
			if (out_d->fl_b) {
				ufree (out_d->buf);
			}
			goto Fin;
		}		

	entry OO_START:
		if (out_d->fl_b)
			ser_outb (OO_START, out_d->buf);
		else
			ser_out (OO_START, out_d->buf);

Fin:
		// note that ser_* takes care of .buf
		// operations (sure SEGV or mem leaks if wrong called)
		ufree (out_d);
		finish;
}
#undef out_d

#define msgt (msg_t)(*m)
static char disp_msgt (char * m) {

	if (m == NULL)
		return '0';

	if (msg_seedsTr (msgt))
		return 't';

	if (msgt == msg_odr)
		return 'o';

	if (msgt == msg_disp)
		return 'd';

	return '?';
}
#undef msgt

static word set_disp (char * buf) {
	lword re;
	word w1, w2;
	char s[40];

	if (scan (buf, "%lu %u %u %s", &re, &w1, &w2, s) != 4)
		return 0; // will display values

	if (beac.mpt && in_header(beac.mpt, msg_type) == msg_disp &&
							beac.cur != 0)
		return 1; // locked

	disp.msg.header.msg_type 	= msg_disp;
	disp.msg.header.rcv	 	= w1;
	disp.msg.header.hco 		= 0;
	disp.msg.header.prox		= 0;

	disp.msg.refh = (word)(re >> 16);
	disp.msg.refl = (word)re;
	disp.msg.rack = w2 ? 1 : 0;

	if ((w1 = strlen (s)) > 39) {
		w1 = 40; s[39] = '\0'; //oh shut up
	}
	disp.msg.len = w1;
	strcpy (disp.str, s);
	return 0;
}

static word set_odr (char * buf) {
	lword re;
	word c, ac, w[9];

	if ((c = scan (buf, "%lu %u %u %u %u %u %u %u %u %u %u", &re, &ac,
			&w[0], &w[1], &w[2], &w[3], &w[4], &w[5], &w[6], &w[7],
			&w[8])) < 3)
		return 0;

	if (beac.mpt && in_header(beac.mpt, msg_type) == msg_odr &&
							beac.cur != 0)
		return 1; // locked

	odr.msg.header.msg_type 	= msg_odr;
	odr.msg.header.rcv 		= w[0];
	odr.msg.header.hco 		= 1;
	odr.msg.header.prox		= 1;

	odr.msg.refh = (word)(re >> 16);
	odr.msg.refl = (word)re;
	odr.msg.rack = ac ? 1 : 0;
	odr.msg.ret = 0;
	odr.msg.hok = 0;
	odr.msg.hko = c -2;
	odr.oe[0].id = local_host;

	// assuming 0s: odr.oe[0].frssi = 0; odr.oe[0].brssi = 0;

	for (ac = 1; ac < 10; ac++)
		if (ac < c - 1)
			odr.oe[ac].id = w[ac -1];
		else
			odr.oe[ac].id = 0; // previous req could be longer

	return 0;
}

static word set_trace (char * buf) {
	lword re;
	word c, t, d, h;

	if (msg_seedsTr (trac.msg.header.msg_type) && beac.cur != 0)
		return 1; // locked

	c = scan (buf, "%lu %u %u %u", &re, &t, &d, &h);
	if (c < 4)
		h = 0;
	if (c < 3)
		d = 3;
	if (c < 2)
		t = 0;
	if (c < 1)
		return 0; // show; ref# = 0 must be explicit

	switch (d) { // direction
		case 0:
			trac.msg.header.msg_type = msg_traceB;
			break;
		case 1:
			trac.msg.header.msg_type = msg_trace1;
			break;
		case 2:
			trac.msg.header.msg_type = msg_traceF;
			break;
		default:
			trac.msg.header.msg_type = msg_trace;
	}
	trac.msg.header.rcv = t;
	trac.msg.header.hco = h; // we deliberately do NOT set prox
	trac.msg.ref = re;
	return 0;
}

static word set_beac (char * buf) {
	word f, v;
	char c;

	while (*buf && *buf == ' ') buf++; // shit oss crap

	if (scan (buf, "%c %u %u", &c, &f, &v) < 3 || f == 0 || f > 63
			|| v == 0)
		return 0;

	if (beac.cur != 0)
		return 1; // locked

	switch (c) {
		case 'o':
			beac.mpt = (char *)&odr.msg;
			break;
		case 'd':
			beac.mpt = (char*)&disp.msg;
			break;
		case 't':
			beac.mpt = (char *)&trac.msg;
			break;
		default:
			return 0;
	}

	if (*beac.mpt == '\0')
		return 2; // not set

	beac.freq = f;
	beac.vol  = v;
	return 0;
}

static word set_tarp (char * buf) {
	word co, pl, rx, fw, sl, rr, tl;

	if ((co = scan (buf, "%u %u %u %u %u %u",
		&pl, &rx, &fw, &sl, &rr, &tl)) == 0)
			return 0;

	// careful: check even if we should trust OSS
	if (co > 5 && tl <= 2)
		tarp_ctrl.param = (tarp_ctrl.param & ~(3 << 6)) | (tl << 6);

	if (co > 4 && rr <= 3)
		tarp_ctrl.param = (tarp_ctrl.param & ~(3 << 4)) | (rr << 4);

	if (co > 3 && sl <= 2)
		tarp_ctrl.param = (tarp_ctrl.param & ~(3 << 1)) | (sl << 1);

	if (co > 2 && fw <= 1)
 		tarp_ctrl.param = (tarp_ctrl.param & 0xFE) | fw;

	if (co > 1 && rx <= 1) {
		if (rx)
			net_opt (PHYSOPT_RXON, NULL);
		else
			net_opt (PHYSOPT_RXOFF, NULL);
	}

	if (pl <= 7)
		net_opt (PHYSOPT_SETPOWER, &pl);

	return 0;
}

fsm ossi_in {
	char ibuf[UART_INPUT_BUFFER_LENGTH];
	word w;
	char * cp;

	state OI_INIT:
		ser_out (OI_INIT, welcome_str);

	state OI_CLR:
		ibuf[0] = '\0'; // clear ibuf

	state OI_IN:
		ser_in (OI_IN, ibuf, UART_INPUT_BUFFER_LENGTH);
		if ((w = strlen(ibuf)) == 0)
			// CR on empty line would do it
			proceed OI_IN;

	state OI_CMD:
		switch (ibuf[0]) {
			case 'q': reset();
			case 'h': proceed OI_INIT;
			case 's': proceed OI_SETS;
			case 'b': proceed OI_BEAC;
			case 'x': proceed OI_XMIT;
			case 'F': proceed OI_FIM;
			case 'm': proceed OI_MAS;
			case 't': proceed OI_TRANS;
			case 'f': proceed OI_OFMT;
		}
		proceed OI_ILL;

	state OI_MAS:
		if (scan (ibuf+1, "%u", &w) > 0) {
			if (w) {
				master_host = local_host;
				if (!running (mbeacon))
					runfsm mbeacon;
				else
					trigger (TRIG_MASTER);
			 } else {
				 master_host = 0;
				 killall (mbeacon);
				 master_ts= seconds();
				 highlight_clear();
			 }
		}
		proceed OI_STATS;

	state OI_TRANS:

		if (scan (ibuf+1, "%u", &w) > 0)
			fim_set.f.stran = w ? 1 : 0;
		proceed OI_STATS;

	state OI_OFMT:

		if (scan (ibuf+1, "%u", &w) > 0)
			fim_set.f.ofmt = w ? 1 : 0;
		proceed OI_STATS;

	state OI_SETS:
		if (w == 1) proceed OI_STATS;
		switch (ibuf[1]) {
			case 'h':
				if (scan (ibuf+2, "%u", &w) > 0 && w != 0)
					local_host = w;
				proceed OI_STATS;
			case 'c':
				if (scan (ibuf+2, "%u", &w) > 0 && w < 256)
					net_opt (PHYSOPT_SETCHANNEL, &w);
				proceed OI_STATS;
			case 'd':
				if (set_disp (ibuf+2))
					proceed OI_LCK;
				proceed OI_SHOW_DISP;
			case 'o':
				if (set_odr (ibuf+2))
					proceed OI_LCK;
				proceed OI_SHOW_ODR;
			case 't':
				if (set_trace (ibuf+2))
					proceed OI_LCK;
				proceed OI_SHOW_TRAC;
			case 'b':
				switch (set_beac (ibuf+2)) {
					case 2: proceed OI_NOTSET;
					case 1: proceed OI_LCK;
				}
				proceed OI_SHOW_BEAC;
			case 'T':
				if (set_tarp (ibuf+2))
					proceed OI_LCK;
				proceed OI_SHOW_TARP;
			default:
				proceed OI_STATS;
		}

	state OI_BEAC:

		switch (ibuf[1]) {

			case 'a':
				if (running (msgbeac))
					proceed OI_ALRUN;

				if (beac.mpt == NULL || *beac.mpt == '\0' ||
					beac.freq == 0 ||
					beac.vol  == 0)
						proceed OI_NOTSET;

				beac.cur = 1; // lock it here
				runfsm msgbeac;
				proceed OI_SHOW_BEAC;

			case 'd':
				if (!running (msgbeac))
					proceed OI_NOTRUN;

				trigger (TRIG_BEAC_DOWN);
				delay (1024, OI_SHOW_BEAC); // give it a chance
				release;

		}
		proceed OI_SHOW_BEAC;

	state OI_XMIT:
	    switch (ibuf[1]) {
		case 'd':
			if (running (msgbeac) &&
			    in_header(beac.mpt, msg_type) == msg_disp) {
				trigger (TRIG_BEAC_PUSH);
				delay (1024, OI_SHOW_BEAC);
				release;
			}

			send_msg ((char *)&disp.msg, sizeof(msgDispType) +
					disp.msg.len +1); // with trailing '\0'
			proceed OI_SHOW_DISP;

		case 'o':
			if (running (msgbeac) &&
			    in_header(beac.mpt, msg_type) == msg_odr) {
				trigger (TRIG_BEAC_PUSH);
				delay (1024, OI_SHOW_BEAC);
				release;
			}

			send_msg ((char *)&odr.msg, sizeof(msgOdrType) +
					(odr.msg.hko +1) * sizeof(odre_t));
			proceed OI_SHOW_ODR;

		case 't':
			if (running (msgbeac) &&
			    msg_seedsTr(in_header(beac.mpt, msg_type))) {
				trigger (TRIG_BEAC_PUSH);
				delay (1024, OI_SHOW_BEAC);
				release;
			}

			send_msg ((char *)&trac.msg, sizeof(msgTraceType));
			proceed OI_SHOW_TRAC;
		}
		proceed OI_ILL;

	state OI_FIM:
		switch (ibuf[1]) {
		    case 'w':
			fim_set.f.tparam = tarp_ctrl.param;
			fim_set.f.polev = net_opt (PHYSOPT_GETPOWER, NULL);
			fim_set.f.rx = net_opt (PHYSOPT_STATUS, NULL) & 1;
			w = fim_write();
			break;

		    case 'e':
			fim_erase ();
			// through to read
		    default:
			w = fim_read();
		}

	state OI_FIMEND:
		ser_outf (OI_FIMEND, fim_str, w, fim_set.f.polev,
			fim_set.f.rx, fim_set.f.tparam & 1,
			(fim_set.f.tparam >> 1) & 3,
			(fim_set.f.tparam >> 4) & 3,
			(fim_set.f.tparam >> 6) & 3);

		proceed OI_CLR;

// stats
	state OI_STATS:
		read_sensor (OI_STATS, SENSOR_BATTERY, &w);

	state OI_STAT1:
		word m[2];
		m[0] =  memfree (0, &m[1]);

		cp = form (NULL, stats_str, local_host,
				net_opt (PHYSOPT_GETCHANNEL, NULL),
			       	seconds(), master_host, master_ts, m[0], m[1],
			       	stackfree(), w, fim_set.f.stran,
				fim_set.f.ofmt);

	state OI_STATEND:
		ser_out (OI_STATEND, cp);
		ufree (cp);
		proceed OI_CLR;
// end of stats

	state OI_SHOW_TARP:
		ser_outf (OI_SHOW_TARP, tarp_str, 
			net_opt (PHYSOPT_GETPOWER, NULL),
			net_opt (PHYSOPT_STATUS, NULL) & 1, // TX 'always on'
			tarp_fwd_on, tarp_slack, tarp_rte_rec, tarp_level);
	 	proceed OI_CLR;

	state OI_SHOW_BEAC:
		ser_outf (OI_SHOW_BEAC, beac_str, disp_msgt (beac.mpt),
				beac.mpt ? *beac.mpt : 0,
				beac.freq, beac.cur, beac.vol);
	 	proceed OI_CLR;

	state OI_SHOW_DISP:
		ser_outf (OI_SHOW_DISP, disp_str, ((lword)disp.msg.refh << 16) +
							disp.msg.refl,
			disp.msg.header.rcv, disp.msg.rack,
			disp.msg.len, disp.str);
	 	proceed OI_CLR;

	state OI_SHOW_TRAC:
	    switch (trac.msg.header.msg_type) { // get direction as entered
		case msg_traceB:
			w = 0;
			break;
		case msg_trace1:
			w = 1;
			break;
		case msg_traceF:
			w = 2;
			break;
		case msg_trace:
			w = 3;
			break;
		default:
			w = 77; // ??
	    }
	state OI_SHOW_TRACEND:
		ser_outf (OI_SHOW_TRACEND, trac_str, trac.msg.ref,
			trac.msg.header.rcv, w, trac.msg.header.hco);
		proceed OI_CLR;

	state OI_SHOW_ODR:
		ser_outf (OI_SHOW_ODR, odr_str, odr.msg.hko,
			((lword)odr.msg.refh << 16) + odr.msg.refl,
			odr.msg.rack, odr.oe[1].id,
			odr.oe[2].id, odr.oe[3].id, odr.oe[4].id, odr.oe[5].id,
			odr.oe[6].id, odr.oe[7].id, odr.oe[8].id, odr.oe[9].id);
		proceed OI_CLR;

	state OI_ILL:
		ser_outf (OI_ILL, ill_str, ibuf);
	 	proceed OI_CLR;

	state OI_LCK:
		ser_out (OI_LCK, lck_str);
		proceed OI_CLR;

	state OI_NOTRUN:
		ser_out (OI_NOTRUN, notrun_str);
		proceed OI_CLR;

	state OI_ALRUN:
		ser_out (OI_ALRUN, alrun_str);
		proceed OI_CLR;

	state OI_NOTSET:
		ser_out (OI_NOTSET, notset_str);
		proceed OI_CLR;
}

void ossi_trace_out (char * buf, word rssi) {
        sint i, num = 0, cnt;
        char * ptr = buf;
	char **lines;
	out_t * out;

	if ((out = (out_t*)get_mem (WNONE, sizeof (out_t))) == NULL)
		return;

	ptr += (in_header(buf, msg_type) == msg_trace1) ?
		(cnt = sizeof (msgTraceType)) :
			(cnt = sizeof (msgTraceAckType));

        if (in_header(buf, msg_type) != msg_traceBAck &&
			in_header(buf, msg_type) != msg_trace1)
                num = in_traceAck(buf, fcount);

        if (in_header(buf, msg_type) != msg_traceFAck)
                num += in_header(buf, hoc);

        if (in_header(buf, msg_type) == msg_traceAck ||
		in_header(buf, msg_type) == msg_trace1)
                num--; // dst counted twice

	if (num > (cnt = NET_MAXPLEN - cnt) / 2)
		// PG: don't exceed the packet size
		num = cnt;
	cnt = 0;

	// num + 1st, last in words
	if ((lines =  (char **)get_mem (WNONE, sizeof(char *)*(num +2))) ==
			 NULL)
		goto Cleanup;

	if ((lines[cnt++] = form (NULL, "%lu: tr(%u) #%lu %u %u %u:\r\n",
			seconds(),
			in_header(buf, snd),
			(in_header(buf, msg_type) == msg_trace1) ?
				in_trace(buf, ref) :
				((lword)in_traceAck(buf, refh) << 16) +
						in_traceAck(buf, refl),
			in_header(buf, msg_type),
                        (in_header(buf, msg_type) == msg_trace1) ?
				in_header(buf, seq_no) : // this is handy
				in_traceAck(buf, fcount),
                        in_header(buf, hoc))) == NULL)
		goto Cleanup;

        while (num--) {
		if ((lines[cnt++] = form (NULL, " %u %u%c\r\n",
				*(byte *)ptr, *(byte *)(ptr +1),
			cnt == 1 && in_header(buf, msg_type) == msg_traceBAck ?
				'*' : ' ')) == NULL)
			goto Cleanup;
                // careful with (... *ptr++, *ptr++) instead
                ptr += 2;
	}

	if ((lines[cnt++] = form (NULL, "%c%u %u\r\n",
			in_header(buf, msg_type) == msg_traceFAck ? '*' : ' ',
			local_host, rssi)) == NULL)
		goto Cleanup;

	// so, we have the lines[] loaded up to cnt-1. out->buf will hold
	// formatted output:
	num = 0;
	for (i = 0; i < cnt; i++)
		num += strlen (lines[i]);

	if ((out->buf = get_mem (WNONE, num +1)) == NULL) // +1 for '\0'
		goto Cleanup;

	for (i = 0; i < cnt; i++) {
		strcat (out->buf, lines[i]);
		ufree (lines[i]);
	}
	ufree (lines);

	out->fl_b = 1;
	run_oo (out);
	return;

Cleanup:
	while (cnt--)
		ufree (lines[cnt]);
	ufree (lines);
	ufree (out);
}

void ossi_odr_out (char * b) {
        sint i, num = 0, cnt = 0;
        char * ptr = b;
	char **lines;
	out_t * out;

	if ((out = (out_t*)get_mem (WNONE, sizeof (out_t))) == NULL)
		return;

	ptr += sizeof (msgOdrType);
	num = 1 + in_odr(b, hko);

	// num + 1st
	if ((lines =  (char **)get_mem (WNONE, sizeof(char *)*(num +1))) ==
			 NULL)
		goto Cleanup;

	if (fim_set.f.ofmt) {
	    if ((lines[cnt++] = form (NULL, "%lu: odr %lu ",
			seconds(),
			((lword)in_odr(b, refh) << 16) + in_odr(b, refl)
			)) == NULL)
		goto Cleanup;

	} else {
	    if ((lines[cnt++] = form (NULL, "%lu: odr #%lu [%u.%u.%u]:\r\n",
			seconds(),
			((lword)in_odr(b, refh) << 16) + in_odr(b, refl),
			in_odr(b, ret), in_odr(b, hok),
			in_odr(b, hko))) == NULL)
		goto Cleanup;
	}

        while (num--) {
		if (in_odr(b, hok) == cnt -1) {
			i = 1;
			if (in_odr(b, ret) && in_odr(b, hok) != in_odr(b, hko))
				i++;
		} else
			i = 0;

		if (fim_set.f.ofmt) {

		    if ((lines[cnt++] = form (NULL, "%u%s",
				((odre_t *)ptr)->id,
				num == 0 ? "\r\n" : " ")) == NULL)
			goto Cleanup;

		} else {

		    if ((lines[cnt++] = form (NULL, " %u: %c(%u %u %u)%c\r\n",
				cnt -1, i == 1 ? '>' : ' ',
				((odre_t *)ptr)->id,
				((odre_t *)ptr)->frssi,
				((odre_t *)ptr)->brssi,
				i == 2 ? '<' : ' ')) == NULL)
			goto Cleanup;
		}

                ptr += sizeof(odre_t);
	}

	// so, we have the lines[] loaded up to cnt-1. out->buf will hold
	// formatted output:
	num = 0;
	for (i = 0; i < cnt; i++)
		num += strlen (lines[i]);

	if ((out->buf = get_mem (WNONE, num +1)) == NULL) // +1 for '\0'
		goto Cleanup;

	for (i = 0; i < cnt; i++) {
		strcat (out->buf, lines[i]);
		ufree (lines[i]);
	}
	ufree (lines);

	out->fl_b = 1;
	run_oo (out);
	return;

Cleanup:
	while (cnt--)
		ufree (lines[cnt]);
	ufree (lines);
	ufree (out);
}

void ossi_disp_out (char * b) {
	out_t * out;

	if ((out = (out_t*)get_mem (WNONE, sizeof (out_t))) == NULL)
		return;

	if ((out->buf = form (NULL,
			"%lu: disp #%lu fr %u.%u ack %u %u<%s>\r\n",
		seconds(),
		((lword)in_disp(b, refh) << 16) + in_disp(b, refl),
		in_header(b, snd), in_header(b, hoc), in_disp(b, rack),
		in_disp(b, len), b + sizeof(msgDispType))) == NULL) {
			ufree (out);
			return;
	}
	out->fl_b = 1;
	run_oo (out);
}
#undef msg_seedsTr

