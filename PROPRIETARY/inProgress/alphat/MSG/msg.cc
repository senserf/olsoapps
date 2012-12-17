/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012        			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "app.h"
#include "msg.h"
#include "oss.h"
#include "app_dcl.h"
#include "msg_dcl.h"
#include "oss_dcl.h"

#include "net.h"
#include "tarp.h"

// statics here
static void msg_master_in (char * buf);
static void process_incoming (char * buf, word size, word rssi);
static word map_rssi (word r);
static void msg_trace_in (char * buf, word rssi);
static void msg_odr_in (char * buf, word rssi, word siz);
static void msg_disp_in (char * buf, word rssi, word siz);

// Master beacon, killable with no need to cleanup.
// Note that in many praxes it is more practical to go for a suicide
// on a flag.
fsm mbeacon {
	state MB_SEND:
		char * b = get_mem (MB_SEND,  sizeof(msgMasterType));

	in_header(b, msg_type) = msg_master; // rcv, hco == 0
	send_msg (b, sizeof(msgMasterType));
	ufree (b);
	master_ts = seconds();
	highlight_set (1, 0.0, NULL);

	when (TRIG_MASTER, MB_SEND);
	delay (MAS_QUANT + rnd() % 1024, MB_SEND);
	release;
}

// more or less universal beacon with semi-private data settable by separate
// operations
fsm msgbeac {
	byte oref;
	byte siz;

	// assumed set: mpt, freq, vol set, cur == 1
	state MSGB_INI:
		switch (in_header(beac.mpt, msg_type)) {
			case msg_odr:
				oref = odr.msg.ref;
				siz = sizeof(msgOdrType) +
					(odr.msg.hko +1) * sizeof(odre_t);
				break;
			case msg_disp:
				oref = disp.msg.ref;
				siz = sizeof(msgDispType) + disp.msg.len +1;
				break;
			default:
				oref = 0;
				siz = sizeof(msgTraceType);
		}

	state MSGB_LOOP:
		switch (in_header(beac.mpt, msg_type)) {
			case msg_odr:
				if (oref == 0)
					odr.msg.ref = (word)seconds();
				else
					odr.msg.ref = oref + beac.cur;
				break;
			case msg_disp:
				if (oref == 0)
					disp.msg.ref = (word)seconds();
				else
					disp.msg.ref = oref + beac.cur;
		}

		send_msg (beac.mpt, siz);
		if (++beac.cur > beac.vol)
			proceed MSGB_DOWN;

		when (TRIG_BEAC_PUSH, MSGB_LOOP);
		when (TRIG_BEAC_DOWN, MSGB_DOWN);
		delay (beac.freq << 10, MSGB_LOOP);
		release;

	state MSGB_DOWN:
		switch (in_header(beac.mpt, msg_type)) {
			case msg_odr:
				odr.msg.ref = oref;
				break;
			case msg_disp:
				disp.msg.ref = oref;
		}
		beac.cur = 0;
		finish;
}		

// Receiver fsm and its static helpers
static void process_incoming (char * buf, word size, word rssi) {

	switch (in_header(buf, msg_type)) {


		case msg_master:
			msg_master_in (buf);
			return;

		case msg_trace:
		case msg_traceF:
		case msg_traceB:
			msg_trace_in (buf, rssi);
			return;

		case msg_traceAck:
		case msg_traceFAck:
		case msg_traceBAck:
		case msg_trace1:
			ossi_trace_out (buf, rssi);
			return;

		case msg_odr:
			msg_odr_in (buf, rssi, size);
			return;

		case msg_disp:
			msg_disp_in (buf, rssi, size);
			return;

		default:
			diag ("Got ?(%u)", in_header(buf, msg_type));
	}
}

// [0, FF] -> [1, F]
// even this is not practical; there should be max. 128 values
static word map_rssi (word r) {
#if 0
#ifdef __SMURPH__
/* rough estimates
 =======================================================
 RP(d)/XP [dB] = -10 x 5.1 x log(d/1.0m) + X(1.0) - 33.5
 =======================================================
 151, 118
*/

        if ((r >> 8) > 151) return 3;
        if ((r >> 8) > 118) return 2;
        return 1;
#else
        if ((r >> 8) > 161) return 3;
        if ((r >> 8) > 140) return 2;
        return 1;
#endif
#endif
        return (r & 0xff00) ? (r >> 8) : 1;
}

// dupa does it hurt in any way if rcv stays while RXOFF?
fsm rcv {

        char *buf;
        sint psize;
        word rssi;

        entry RC_TRY:

                if (buf) {
                        ufree (buf);
                        buf = NULL;
                        psize = 0;
                }
                psize = net_rx (RC_TRY, &buf, &rssi, 0);

                if (psize <= 0) {
                        diag ("net_rx (%d)", psize);
                        proceed RC_TRY;
                }

		// say, we want to study rcv on a particular node
		highlight_set (0, 1.5,
			"RCV (%u): %u %u %u",
			psize, in_header(buf, msg_type),
			in_header(buf, snd),
	  		in_header(buf, hoc));

#ifdef D_DEBUG
                diag ("RCV (%d): %x-%u-%u-%u-%u-%u\r\n",
                          psize, in_header(buf, msg_type),
                          in_header(buf, seq_no) & 0xffff,
                          in_header(buf, snd),
                          in_header(buf, rcv),
                          in_header(buf, hoc) & 0xffff,
                          in_header(buf, hco) & 0xffff);
#endif

                // that's how we could check which plugin is on
                // if (net_opt (PHYSOPT_PLUGINFO, NULL) != INFO_PLUG_TARP)

        entry RC_MSG:

                process_incoming (buf, psize, map_rssi(rssi));
                proceed RC_TRY;
}

void send_msg (char * buf, sint size) {

	if (in_header(buf, rcv) == local_host) {
		diag ("%u to lh", in_header(buf, msg_type));
		return;
	}

	if (net_tx (WNONE, buf, size, 0) != 0)
		diag ("Tx %u %u", in_header(buf, msg_type), size);
}

static void msg_master_in (char * buf) {
	if (running (mbeacon)) { // I was It
		killall (mbeacon);
		highlight_clear();
	}

	master_ts = seconds();

	// there may be races between local 'm' and msgs... just in case:
	if (master_host != in_header(buf, snd))
		master_host = in_header(buf, snd);
}

static void msg_disp_in (char * buf, word rssi, word siz) {

	ossi_disp_out (buf);

	if (in_disp(buf, ret)) { // write back
		in_disp(buf, ret) = 0;
		in_header(buf, rcv) = in_header(buf, snd);
		in_header(buf, hco) = 0;
		send_msg (buf, siz);
	}
}

#define oep	((odre_t *)(buf + sizeof(msgOdrType)))
static void msg_odr_in (char * buf, word rssi, word siz) {

	// first, update this hop:
	if (in_odr (buf, ret)) {
		in_odr (buf, hok)--;
		(oep + in_odr (buf, hok))->brssi = rssi;
	} else {
		in_odr (buf, hok)++;
		(oep + in_odr (buf, hok))->frssi = rssi;
	}

	if (in_odr (buf, hok) == in_odr (buf, hko))
		in_odr (buf, ret) = 1;

	if (in_odr (buf, hok) != 0) { // forward

		in_header(buf, rcv) = in_odr (buf, ret) ?
			(oep + in_odr (buf, hok) -1)->id :
			(oep + in_odr (buf, hok) +1)->id;

		send_msg (buf, siz); // I'm not entirely sure if this is ok...
	}

	ossi_odr_out (buf);
}
#undef oep

static void msg_trace_in (char * buf, word rssi) {
        char * b;
        word len;

        if (in_header(buf, msg_type) != msg_traceB)
                len = sizeof(msgTraceAckType) +
                        (in_header(buf, hoc) & 0x7F) * 2;
        else
                len = sizeof(msgTraceAckType) + 2;

        b = get_mem (WNONE, len);
        memset (b, 0, len);

        switch (in_header(buf, msg_type)) {
                case msg_traceF:
                        in_header(b, msg_type) = msg_traceFAck;
                        break;
                case msg_traceB:
                        in_header(b, msg_type) = msg_traceBAck;
                        break;
                default:
                        in_header(b, msg_type) = msg_traceAck;
        }

        in_header (b, rcv) = in_header (buf, snd);
        // hco is 0
        in_traceAck(b, fcount) = in_header(buf, hoc) & 0x7F;

        // fwd part
        if (in_header(buf, msg_type) != msg_traceB &&
                        (in_header(buf, hoc) & 0x7F) > 1)
                memcpy (b + sizeof(msgTraceAckType),
                                buf + sizeof(msgTraceType),
                                2 * ((in_header(buf, hoc) & 0x7F) -1));

        // note that this node is counted in hoc, but is not appended yet, so:
        *((byte *)b + len - 2) = (byte)local_host;
        *((byte *)b + len - 1) = (byte)rssi;
        send_msg (b, len);
        ufree (b);
}

