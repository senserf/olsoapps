/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "app.h"
#include "msg_gene.h"
#include "net.h"

brCtrlType br_ctrl;

const	lword	ESN = 0xBACA0001;

nid_t	net_id;
word	app_flags, freqs, connect, l_rssi;

char * cmd_line  = NULL;
cmdCtrlType cmd_ctrl = {0, 0x00, 0x00, 0x00, 0x00};

int beacon (word, address);

char * get_mem (word state, int len);
void send_msg (char * buf, int size);

// let's not #include any _if, but list all externs explicitly:
extern nid_t local_host;
extern nid_t master_host;
extern bool msg_br_out();
extern int msg_st_out();
extern lword esns[];
extern word  svec[];

#define SRS_ITER	00
#define SRS_BR		10
#define SRS_CONT	20
#define SRS_ST		30
#define SRS_NEXT	40
#define SRS_FIN		50
process (st_rep, void)

	static int left; 

	entry (SRS_ITER)
		if (br_ctrl.rep_freq  >> 1 == 0 ||
			local_host == master_host ||
			master_host == 0) // neg / pos is bit 0
			kill (0);
		left = ack_retries + 1; // +1 makes "tries" from "retries"

	entry (SRS_BR)
		if (left-- <= 0)
			proceed (SRS_CONT);
		clr_brSTACK;
		clr_brSTNACK;
		if (msg_br_out()) // sent out
			wait (ST_ACKS, SRS_CONT);
		delay (ack_tout << 10, SRS_BR);
		release;

	entry (SRS_CONT)
		if (is_brSTNACK)
			proceed (SRS_FIN);
		left = ack_retries + 1;

	entry (SRS_ST)
		if (left-- <= 0)
			proceed (SRS_NEXT);

		clr_brSTACK;
		clr_brSTNACK;
		switch (msg_st_out()) {
			case -1: // nothing to report
				proceed (SRS_FIN);
			case YES:
				wait (ST_ACKS, SRS_NEXT);
				// fall through
			default: // NO, bad things: retry
				delay (ack_tout << 10, SRS_ST);
		}
		release;

	 entry (SRS_NEXT)
		if (is_brSTNACK || esns[1] == 0xFFFFFFFF)
			proceed (SRS_FIN);
		left = ack_retries + 1;
		esns[0] = esns[1];
		proceed (SRS_ST);

	entry (SRS_FIN)
		esns[0] = esns[1] = 0;
		memset (svec, 0, SVEC_SIZE << 1);
		// ldelay on 0 takes PicOS down,  double check:
		if (br_ctrl.rep_freq  >> 1 == 0)
			kill (0);
		wait (ST_REPTRIG, SRS_ITER);
		ldelay (br_ctrl.rep_freq >> 1, SRS_ITER);
		release;
endprocess (1)
#undef SRS_ITER
#undef SRS_BR
#undef SRS_CONT
#undef SRS_ST
#undef SRS_NEXT
#undef SRS_FIN

#define BS_ITER 00
#define BS_ACT  10
process (beacon, char)
	entry (BS_ITER)
		wait (BEAC_TRIG, BS_ACT);
		delay (beac_freq << 10, BS_ACT);
		release;

	entry (BS_ACT)
		if (beac_freq == 0) {
			ufree (data);
			kill (0);
		}
		switch (in_header(data, msg_type)) {

		case msg_master:
			if (master_host != local_host) {
				ufree (data);
				kill (0);
			}
			in_master(data, con) = freqs & 0xFF00 | (connect >> 8);
			send_msg (data, sizeof(msgMasterType));
			break;
// not needed, out:
#if 0
		case msg_trace:
			send_msg (data, sizeof(msgTraceType));
			break;
#endif
		case msg_new:
			if (net_id) {
				ufree (data);
				kill (0);
			}
			send_msg (data, sizeof(msgNewType));
			break;

		default:
			dbg_a (0x03FA); // beacon failed
			freqs &= 0xFF00;
		}
		proceed (BS_ITER);
endprocess (1)
#undef  BS_ITER 
#undef  BS_ACT 

#define CS_INIT	00
#define CS_ITER	10
#define CS_ACT	20
process (con_man, void)

	entry (CS_INIT)
		fastblink (0);
		leds (CON_LED, LED_ON);
	
	entry (CS_ITER)
		wait (CON_TRIG, CS_ACT);
		delay (audit_freq << 10, CS_ACT);
		release;

	entry (CS_ACT)
		if (master_host == local_host) {
			leds (CON_LED, LED_ON);
			kill (0);
		}
		if (audit_freq == 0) {
			fastblink (1);
			leds (CON_LED, LED_BLINK);
			kill (0);
		}
		if (con_miss != 0xFF)
			connect++;
		if (con_miss >= con_bad + con_warn)
			leds (CON_LED, LED_OFF);
		else if (con_miss >= con_warn)
			leds (CON_LED, LED_BLINK);
		proceed (CS_ITER);
endprocess (1)
#undef  CS_INIT
#undef	CS_ITER
#undef	CS_ACT

char * get_mem (word state, int len) {
	char * buf = (char *)umalloc (len);
	if (buf == NULL) {
		dbg_e (0x1000 | len); // get_mem() failed
		if (state != NONE) {
			umwait (state);
			release;
		}
	}
	return buf;
}

void send_msg (char * buf, int size) {

	// this shouldn't be... but saves time in debugging
	if (in_header(buf, rcv) == local_host) {
		dbg_a (0x0400 | in_header(buf, msg_type)); // Dropped to lh
		return;
	}

	if (net_tx (NONE, buf, size, encr_data) != 0) {
#if 0
		// dupa: remove with the new driver
		net_opt (PHYSOPT_RXOFF, NULL);
		net_opt (PHYSOPT_TXON, NULL);
		net_opt (PHYSOPT_RXON, NULL);
#endif
		dbg_a (0x0500 | in_header(buf, msg_type)); // Tx failed
	}
}

