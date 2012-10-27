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

//msg-related global 
mb_t beac;

// statics here
static void msg_master_in (char * buf);
static void process_incoming (char * buf, word size, word rssi);
static word map_rssi (word r);
static void msg_trace_in (char * buf, word rssi);
static void msg_any_in (char * b, word siz);
static void send_msg (char * buf, sint size);

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

// more or less universal beacon: note that it could have its private data
// and many beacons active... not such a good idea in general, I think.
fsm msgbeac {
	state MSGB_LOOP:
		if (beac.f == 0) {
			ufree (beac.b);
			beac.b = NULL;
		}
		if (beac.b == NULL)
			finish;

		send_msg (beac.b, beac.s);

		when (TRIG_BEAC, MSGB_LOOP);
		delay (beac.f << 10, MSGB_LOOP);
		release;
}		

// Receiver fsm and its static helpers
static void process_incoming (char * buf, word size, word rssi) {

	switch (in_header(buf, msg_type)) {


		case msg_master:
			msg_master_in (buf);
			return;

		case msg_any:
			msg_any_in (buf, size);
			return;

		case msg_stats:
			ossi_stats_out (buf);
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

// dupa does it hurt in any way if rcv stays on RXOFF?
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
		highlight_set (0, local_host == 12 ? 0.0 : 1.5,
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

// Send uses net_tx, way too complex for current real needs...
static void send_msg (char * buf, sint size) {

	if (in_header(buf, rcv) == local_host) {
		diag ("%u to lh", in_header(buf, msg_type));
		return;
	}

	if (net_tx (WNONE, buf, size, 0) != 0)
		diag ("Tx %u", in_header(buf, msg_type));
}

static void msg_master_in (char * buf) {
	if (app_flags.f.m_chg) {
		app_flags.f.m_chg = 0;
		if (running (mbeacon)) { // I was It
			killall (mbeacon);
			highlight_clear();
		}
	}

	master_ts = seconds();
	leds (LED_G, LED_ON);

	// there may be races between local 'm' and msgs... just in case:
	if (master_host != in_header(buf, snd)) {
		master_host = in_header(buf, snd);
		ossi_stats_out(NULL);
	}
}

// Note that the ,sg is constructed once forthe beacon's lifetime, i.e. no
// updates, e.g. for status or timestamps. It is trivial to make data current,
// of course.
static void beac_or_send (char * b, word s) {
	if (beac.f != 0 && beac.b == NULL) {
		beac.b = b;
		beac.s = s;
		runfsm msgbeac;
	} else {
		send_msg (b, s);
		ufree (b);
	}
}

sint msg_stats_out (nid_t t) {
	char * b = get_mem (WNONE, sizeof(msgStatsType));
	word w;

	if (b == NULL)
		return 1;

	in_header(b, msg_type) = msg_stats;

	if (t == 0xFFFF) { // kludgy bcast
		in_header(b, rcv) = 0;
		in_header(b, hco) = 1; // only the neighbours
	} else
		in_header(b, rcv) = t;

	in_stats(b, ltime) = seconds();
	in_stats(b, mhost) = master_host;
	in_stats(b, fl) = app_flags;
	in_stats(b, mem) = memfree(0, &w);
	in_stats(b, mmin) = w;
	in_stats(b, stack) = stackfree();
	in_stats(b, batter) = bat;

	beac_or_send (b, sizeof(msgStatsType));
	return 0;
}

sint msg_trace_out (nid_t t, word dir, word hlim) {
	char * b = get_mem (WNONE, sizeof(msgTraceType));

	if (b == NULL)
		return 1;

	switch (dir) {
		case 0:
			in_header(b, msg_type) = msg_traceB;
			break;
		case 1:
			in_header(b, msg_type) = msg_trace1;
			break;
		case 2:
			in_header(b, msg_type) = msg_traceF;
			break;
		default:
			in_header(b, msg_type) = msg_trace;
	}
	in_header(b, rcv) = t;
	in_header(b, hco) = hlim;

	beac_or_send (b, sizeof(msgTraceType));
	return 0;
}

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

static void msg_any_in (char * b, word siz) {
	req_t * req;
	cmd_t * cmd;
	char  * ptr;

	siz -= sizeof(msgAnyType);
	if ((req = (req_t *)get_mem (WNONE, sizeof(req_t))) == NULL ||
			(cmd = (cmd_t *)get_mem (WNONE, siz)) == NULL) {
		ufree (req); // ufree (NULL) is OK
		return;
	}

	memcpy ((char *)cmd, b + sizeof(msgAnyType), siz);

	// 0 or 1 are ok - even bytes in the msg
	if ((word)(siz - cmd->size - sizeof(cmd_t)) > 1) {
		diag ("any_in %d %d", cmd->size, siz);
		ufree (req); ufree (cmd);
		return;
	}

/* it is tempting to set buf just sizeof(char *) ahead, unfortunately the string
   is passed to oss_out, which runs independently from this (rcv) fsm.
   Although oss_out can be instructed to keep the buffer, but it is meant for
   const strings; here, we could end up releasing the whole 'a' command with
   serialized buf before actually oss_out'ing the buf with undefined outcome
*/
	if (cmd->size != 0) {
		if ((ptr = get_mem (WNONE, cmd->size)) == NULL) {
			ufree (req); ufree (cmd);
			return;
		}
		memcpy (ptr, (char *)&(cmd->buf) + sizeof(char *), cmd->size);
		cmd->buf = ptr;
	}
	req->src = in_header(b, snd);
	req->cmd = cmd;
	if (req_in (req)) // req is ufreed in req_in(), get data from b:
		diag ("rpc %c fr %u", ((cmd_t *)(b + sizeof(msgAnyType)))->code,
			in_header(b, snd));
}

/* under r->cmd, we have the 'a' cmd_t struct followed by the embedded cmd_t
   struct followed by its buf
*/
sint msg_any_out (req_t * r) {
	char * b = get_mem (WNONE, sizeof(msgAnyType) + r->cmd->size);

	if (b == NULL)
		return 1;

	in_header(b, msg_type) = msg_any;
	in_header(b, rcv) = r->cmd->argv_w[0];
	in_header(b, hco) = r->cmd->argv_w[1];
	memcpy (b +sizeof(msgAnyType), (char *)r->cmd + sizeof(cmd_t), 
		r->cmd->size);

	beac_or_send (b, sizeof(msgAnyType) + r->cmd->size);
	return 0;
}

