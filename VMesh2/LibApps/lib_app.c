/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "app.h"
#include "msg_vmesh.h"
#include "net.h"
#include "nvm.h"
#include "pinopts.h"
#include "tcvplug.h"
#include "lhold.h"

const	lword	ESN = 0xBACA0001;
lword	cyc_sp;
lword	cyc_left = 0;
lword	io_creg; // LSB: nvm_freq :6, cmp oper :2; creg :24
lword	io_pload = 0xFFFFFFFF;

nid_t	net_id;
word	app_flags, freqs, connect, l_rssi;
byte * cmd_line  = NULL;

cmdCtrlType cmd_ctrl = {0, 0x00, 0x00, 0x00, 0x00};
brCtrlType br_ctrl;
cycCtrlType cyc_ctrl;

int cyc_man (word, address);
int con_man (word, address);
int beacon (word, address);
int del_man (word, address);

char * get_mem (word state, int len);
void send_msg (char * buf, int size);

// let's not #include any _if, but list all externs explicitly:
extern nid_t local_host;
extern nid_t master_host;
extern bool msg_br_out();
extern int msg_st_out();
extern bool msg_io_out();
extern lword esns[];
extern word  svec[];
extern void oss_io_out (char * buf, bool acked);

#define SRS_INIT	00
#define SRS_DEL		10
#define SRS_ITER	20
#define SRS_BR		30
#define SRS_CONT	40
#define SRS_ST		50
#define SRS_NEXT	60
#define SRS_FIN		70
#define ST_REP_BOOT_DELAY	100
process (st_rep, word)

	static int left; 

	entry (SRS_INIT)
		if (data)
			left = rnd() % ST_REP_BOOT_DELAY;
		else
			left = 0;

	entry (SRS_DEL)
		if (left > 63) {
			left -= 63;
			delay (63 << 10, SRS_DEL);
			release;
		}
		if (left > 0) {
			delay (left << 10, SRS_ITER);
			release;
		}

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
#undef SRS_INIT
#undef SRS_DEL
#undef SRS_ITER
#undef SRS_BR
#undef SRS_CONT
#undef SRS_ST
#undef SRS_NEXT
#undef SRS_FIN

#define IRS_ITER	0
#define IRS_IRUPT	10
#define IRS_REP		20
#define IRS_NVM		30
#define IRS_FIN		40
process (io_rep, void)
	static int left;
	lword lw;
	nodata;

	entry (IRS_ITER)
		wait (PMON_CMPEVENT, IRS_IRUPT);
		wait (PMON_NOTEVENT, IRS_IRUPT);
		if (io_creg >> 26) // nvm writes
			delay ((word)(io_creg >> 26) << 10, IRS_NVM);
		if ((lw = pmon_get_state()) & PMON_STATE_CMP_PENDING ||
				lw & PMON_STATE_NOT_PENDING)
			proceed (IRS_IRUPT);
		release;

	entry (IRS_IRUPT)
		io_pload = pmon_get_cnt() << 8 | pmon_get_state();
		if (io_pload & PMON_STATE_CMP_PENDING) {
			switch (io_creg & 3) {
				case 1:
					pmon_add_cmp (io_creg >> 8);
					break;
				case 2:
					pmon_sub_cnt (io_creg >> 8);
					break;
				case 3:
					pmon_dec_cnt ();
			} // 0 is nop
			pmon_pending_cmp();
		}
		if (io_pload & PMON_STATE_NOT_PENDING)
			pmon_pending_not();
		left = ack_retries + 1; // +1 makes "tries" from "retries"

	entry (IRS_REP)
		if (left-- <= 0)
			proceed (IRS_FIN);

		// this is different from br_rep: report locally as well
		if (master_host == 0 || master_host == local_host) {
			oss_io_out(NULL, YES);
			proceed (IRS_FIN);
		}

		clr_ioACK;
		if (msg_io_out())
			wait (IO_ACK_TRIG, IRS_FIN);
		delay (ack_tout << 10, IRS_REP);
		wait (PMON_CMPEVENT, IRS_IRUPT);
		wait (PMON_NOTEVENT, IRS_IRUPT);
		release;

	entry (IRS_NVM)
		nvm_io_backup();
		proceed (IRS_ITER);

	entry (IRS_FIN)
		io_pload = 0xFFFFFFFF;
		proceed (IRS_ITER);

endprocess (1)
#undef IRS_ITER
#undef IRS_IRUPT
#undef IRS_REP
#undef IRS_NVM
#undef IRS_FIN


#define CS_INIT         00
#define CS_ACT          10
#define CS_HOLD		20
process (cyc_man, void)
	word a;
	nodata;

	entry (CS_INIT)
		if (cyc_ctrl.st == CYC_ST_DIS || cyc_ctrl.mod == CYC_MOD_PON ||
			cyc_ctrl.mod == CYC_MOD_POFF || cyc_ctrl.prep == 0 ||
			cyc_sp == 0) {
			cyc_left = 0;
			kill (0);
		}

		if (cyc_ctrl.st == CYC_ST_SLEEP || cyc_ctrl.st == CYC_ST_ENA) {
			cyc_left = cyc_sp;
			if (local_host != master_host) {
				// double check
				if (cyc_ctrl.st == CYC_ST_ENA) {
					dbg_2 (0xC2F6);
					cyc_left = 0;
					cyc_ctrl.st = CYC_ST_ENA;
					kill (0);
				}
				net_opt (PHYSOPT_RXOFF, NULL);
				leds (CON_LED, LED_OFF);
				if (cyc_ctrl.mod == CYC_MOD_NET)
					net_opt (PHYSOPT_TXOFF, NULL);
				else // CYC_MOD_PNET
					net_opt (PHYSOPT_TXHOLD, NULL);
				net_qera (TCV_DSP_XMTU);
				net_qera (TCV_DSP_RCVU);
				if ((a = running (con_man)))
					kill (a);
			}
		} else
			cyc_left = cyc_ctrl.prep;

	entry (CS_ACT)
		if (cyc_ctrl.st == CYC_ST_DIS || cyc_ctrl.mod == CYC_MOD_PON ||
                        cyc_ctrl.mod == CYC_MOD_POFF || cyc_ctrl.prep == 0 ||
			cyc_sp == 0) {
			cyc_left = 0;
                        kill (0);
		}
		if (cyc_left == 0) {
			switch (cyc_ctrl.st) {
				case CYC_ST_ENA:
					cyc_ctrl.st = CYC_ST_PREP;
					break;
				case CYC_ST_PREP:
					if (master_host == local_host)
						cyc_ctrl.st = CYC_ST_ENA;
					else
						cyc_ctrl.st = CYC_ST_SLEEP;
					break;
				default:  // also CYC_ST_SLEEP
					if (local_host == master_host)
						dbg_2 (0xC2F5);
					cyc_ctrl.st = CYC_ST_ENA;
					net_opt (PHYSOPT_RXON, NULL);
					net_opt (PHYSOPT_TXON, NULL);
					leds (CON_LED, LED_BLINK);
					kill (0);
			}
			proceed (CS_INIT);
		}
		if (cyc_ctrl.st == CYC_ST_SLEEP &&
				cyc_ctrl.mod == CYC_MOD_NET) {
			while (cyc_left != 0) {
				if (cyc_left > MAX_UINT) {
					cyc_left -= MAX_UINT;
					freeze (MAX_UINT);
				} else {
					freeze ((word)cyc_left);
					cyc_left = 0;
				}
			}
			proceed (CS_ACT);
		}
	entry (CS_HOLD)
		lhold (CS_HOLD, &cyc_left);
		if (cyc_left) {
			dbg_2 (0xC2F7);
			cyc_left = 0;
		}
		proceed (CS_ACT);
endprocess (1)
#undef CS_INIT
#undef CS_ACT
#undef CS_HOLD

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
			//in_master(data, con) = freqs & 0xFF00 | (connect >> 8);
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

static word get_prep () {
	word w;
	if (cyc_ctrl.mod != CYC_MOD_NET)
		return (cyc_ctrl.mod  == CYC_MOD_PON ?
			CYC_MSG_FORCE_ENA : CYC_MSG_FORCE_DIS);

	if (cyc_ctrl.st != CYC_ST_PREP || cyc_ctrl.prep == 0 ||
			(w = running (cyc_man)) == 0)
		return 0;
	w = (word)lhleft (w, &cyc_left);
	if (w > cyc_ctrl.prep) {
		dbg_2 (0xC2F4);
		return 0;
	}
	return w;
}

void send_msg (char * buf, int size) {

	// this shouldn't be... but saves time in debugging
	if (in_header(buf, rcv) == local_host) {
		dbg_a (0x0400 | in_header(buf, msg_type)); // Dropped to lh
		return;
	}

	// master inserts are convenient here, as the beacon is prominent...
	if (in_header(buf, msg_type) == msg_master) {
		in_master(buf, con) = (freqs & 0xFF00) | (connect >> 8);
		in_master(buf, cyc) = get_prep();
	}
	if (cyc_ctrl.mod == CYC_MOD_POFF) {
		net_opt (PHYSOPT_TXON, NULL);
		if (net_tx (NONE, buf, size, encr_data) != 0) {
			dbg_a (0x0500 | in_header(buf, msg_type)); // Tx failed
		}
		net_opt (PHYSOPT_TXOFF, NULL);
	} else {
		if (net_tx (NONE, buf, size, encr_data) != 0) {
			dbg_a (0x0500 | in_header(buf, msg_type));
		}
	}
}

