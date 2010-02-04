/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009 			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "vuee_ert.h"

#include "app_ert.h"
#include "msg_ert.h"
#include "net.h"
#include "nvm_ert.h"
#include "pinopts.h"
#include "tcvplug.h"
#ifndef __SMURPH__
#include "lhold.h"
#endif

char * get_mem (word state, int len);
void send_msg (char * buf, int size);

thread (st_rep)

	entry (SRS_INIT)
		if (shared_left == -1)
			shared_left = rnd() % ST_REP_BOOT_DELAY;
		else
			shared_left = 0;

	entry (SRS_DEL)
		if (shared_left > 63) {
			shared_left -= 63;
			delay (63 << 10, SRS_DEL);
			release;
		}
		if (shared_left > 0) {
			delay (shared_left << 10, SRS_ITER);
			release;
		}

	entry (SRS_ITER)
		if (br_ctrl.rep_freq  >> 1 == 0 ||
			local_host == master_host ||
			master_host == 0) // neg / pos is bit 0
			finish;
		shared_left = ack_retries +1; //+1 makes "tries" from "retries"

	entry (SRS_BR)
		if (shared_left-- <= 0)
			proceed (SRS_FIN);
		clr_brSTACK;
		if (msg_br_out()) // sent out
			when (ST_ACKS, SRS_FIN);
		delay (ack_tout << 10, SRS_BR);
		release;

	entry (SRS_FIN)
		// ldelay on 0 takes PicOS down,  double check:
		if (br_ctrl.rep_freq  >> 1 == 0)
			finish;
		when (ST_REPTRIG, SRS_ITER);
		ldelay (br_ctrl.rep_freq >> 1, SRS_ITER);
		release;
endthread

thread (dat_rep)
	nodata;

	entry (DRS_INIT)
		shared_left = ack_retries +1; //+1 makes "tries" from "retries"

	entry (DRS_REP)
		if (shared_left-- <= 0 || master_host == 0 ||
				local_host == master_host)
			proceed (DRS_FIN);
		in_header(dat_ptr, rcv) = master_host;
		in_header(dat_ptr, hco) = 0;
		clr_datACK;
		send_msg ((char *)dat_ptr,
				sizeof(msgDatType) + in_dat(dat_ptr, len));
		when (DAT_ACK_TRIG, DRS_FIN);
		delay (ack_tout << 10, DRS_REP);
		release;

	entry (DRS_FIN)
		if (dat_ptr) {
			ufree (dat_ptr);
			dat_ptr = NULL;
		}
		// if changed in meantime... see oss_io.c
		if (is_cmdmode)
			runthread (st_rep);
		finish;
endthread

thread (io_back)

	entry (IBS_ITER)
		if ((iob_htime = (io_creg >> 26) * 900) == 0)
			finish;
		nvm_io_backup();

	entry (IBS_HOLD)
		lhold (IBS_HOLD, &iob_htime);
		proceed (IBS_ITER);
endthread

thread (io_rep)
	lword lw;
	nodata;

	entry (IRS_INIT)
		// Debatable, but seems a good idea at powerup if
		// we try to recover comparator. So, to be consistent,
		// clear the flags at any start:
		pmon_pending_cmp();
		pmon_pending_not();

	entry (IRS_ITER)
		when (PMON_CMPEVENT, IRS_IRUPT);
		when (PMON_NOTEVENT, IRS_IRUPT);
		if ((lw = pmon_get_state()) & PMON_STATE_CMP_PENDING ||
				lw & PMON_STATE_NOT_PENDING)
			proceed (IRS_IRUPT);
		release;

	entry (IRS_IRUPT)
		io_pload = pmon_get_cnt() << 8 | pmon_get_state();
		if (io_pload & PMON_STATE_CMP_PENDING) {
			switch ((io_creg >> 24) & 3) {
				case 1:
					pmon_add_cmp (io_creg & 0x00FFFFFFL);
					break;
				case 2:
					pmon_sub_cnt (io_creg & 0x00FFFFFFL);
					break;
				case 3:
					pmon_dec_cnt ();
			} // 0 is nop
			pmon_pending_cmp();
		}
		if (io_pload & PMON_STATE_NOT_PENDING)
			pmon_pending_not();
		ior_left = ack_retries + 1; // +1 makes "tries" from "retries"

	entry (IRS_REP)
		if (ior_left-- <= 0)
			proceed (IRS_FIN);

		// this is different from st_rep: report locally as well
		if (master_host == 0 || master_host == local_host) {
			oss_io_out(NULL, YES);
			proceed (IRS_FIN);
		}

		clr_ioACK;
		if (msg_io_out())
			when (IO_ACK_TRIG, IRS_FIN);
		delay (ack_tout << 10, IRS_REP);
		when (PMON_CMPEVENT, IRS_IRUPT);
		when (PMON_NOTEVENT, IRS_IRUPT);
		release;

	entry (IRS_FIN)
		io_pload = 0xFFFFFFFF;
		proceed (IRS_ITER);

endthread

#if DM2200

extern void hstat (word); // kludge from phys_dm2200.c
#define	HSTAT_OFF	hstat (0)

#else

#define	HSTAT_OFF	CNOP

#endif
	
thread (cyc_man)

	entry (CYS_INIT)
		if (cyc_ctrl.st == CYC_ST_DIS || cyc_ctrl.mod == CYC_MOD_PON ||
			cyc_ctrl.mod == CYC_MOD_POFF || cyc_ctrl.prep == 0 ||
			cyc_sp == 0) {
			cyc_left = 0;
			if (local_host == master_host) 
				cyc_ctrl.st = CYC_ST_DIS;
			else if (cyc_ctrl.st != CYC_ST_DIS)
				cyc_ctrl.st = CYC_ST_ENA;
			finish;
		}

		if (cyc_ctrl.st == CYC_ST_SLEEP || cyc_ctrl.st == CYC_ST_ENA) {
			cyc_left = cyc_sp;
			if (local_host != master_host) {
				// double check
				if (cyc_ctrl.st == CYC_ST_ENA) {
					dbg_2 (0xC2F6);
					cyc_left = 0;
					cyc_ctrl.st = CYC_ST_ENA;
					finish;
				}
				net_opt (PHYSOPT_RXOFF, NULL);
				app_leds (LED_OFF);
				if (cyc_ctrl.mod == CYC_MOD_NET)
					net_opt (PHYSOPT_TXOFF, NULL);
				else // CYC_MOD_PNET
					net_opt (PHYSOPT_TXHOLD, NULL);
				net_qera (TCV_DSP_XMTU);
				net_qera (TCV_DSP_RCVU);
				killall (con_man);
			}
		} else
			cyc_left = cyc_ctrl.prep;

	entry (CYS_ACT)
		if (cyc_ctrl.st == CYC_ST_DIS || cyc_ctrl.mod == CYC_MOD_PON ||
                        cyc_ctrl.mod == CYC_MOD_POFF || cyc_ctrl.prep == 0 ||
			cyc_sp == 0) {
			cyc_left = 0;
			if (local_host == master_host)
				cyc_ctrl.st = CYC_ST_DIS; 
			else if (cyc_ctrl.st != CYC_ST_DIS)
				cyc_ctrl.st = CYC_ST_ENA;
                        finish;
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
					app_leds (LED_BLINK);
					finish;
			}
			proceed (CYS_INIT);
		}
		if (cyc_ctrl.st == CYC_ST_SLEEP &&
				cyc_ctrl.mod == CYC_MOD_NET) {
#if 0
			while (cyc_left != 0) {
				if (cyc_left > MAX_UINT) {
					cyc_left -= MAX_UINT;
					HSTAT_OFF;
					freeze (MAX_UINT);
				} else {
					HSTAT_OFF;
					freeze ((word)cyc_left);
					cyc_left = 0;
				}
			}
			proceed (CYS_ACT);
#endif
			// let's keep it as it was, but I believe we can be
			// in powerdown() all time
			if (cyc_left != 0) {
				HSTAT_OFF;
				powerdown();
			}
			// fall through
		}
	entry (CYS_HOLD)
		lhold (CYS_HOLD, &cyc_left);
		powerup(); // see above
		if (cyc_left) {
			dbg_2 (0xC2F7);
			cyc_left = 0;
		}
		proceed (CYS_ACT);
endthread

strand (beacon, char)
	entry (BS_ITER)
		when (BEAC_TRIG, BS_ACT);
		delay (beac_freq << 10, BS_ACT);
		release;

	entry (BS_ACT)
		if (beac_freq == 0) {
			ufree (data);
			finish;
		}
		switch (in_header(data, msg_type)) {

		case msg_master:
			if (master_host != local_host) {
				ufree (data);
				finish;
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
				finish;
			}
			send_msg (data, sizeof(msgNewType));
			break;

		default:
			dbg_a (0x03FA); // beacon failed
			freqs &= 0xFF00;
		}
		proceed (BS_ITER);
endstrand

thread (con_man)

	entry (COS_INIT)
		fastblink (0);
		app_leds (LED_ON);
	
	entry (COS_ITER)
		when (CON_TRIG, COS_ACT);
		delay (audit_freq << 10, COS_ACT);
		release;

	entry (COS_ACT)
		if (master_host == local_host) {
			app_leds (LED_ON);
			finish;
		}
		if (audit_freq == 0) {
			fastblink (1);
			app_leds (LED_BLINK);
			finish;
		}
		if (con_miss != 0xFF)
			connect++;
		if (con_miss >= con_bad + con_warn)
			app_leds (LED_OFF);
		else if (con_miss >= con_warn)
			app_leds (LED_BLINK);
		proceed (COS_ITER);
endthread

char * get_mem (word state, int len) {
	char * buf = (char *)umalloc (len);
	if (buf == NULL) {
		dbg_e (0x1000 | len); // get_mem() failed
		if (state != WNONE) {
			umwait (state);
			release;
		}
	}
	return buf;
}

/*static*/ word get_prep () {
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
	if (local_host != master_host && cyc_ctrl.mod == CYC_MOD_POFF) {
		net_opt (PHYSOPT_TXON, NULL);
		if (net_tx (WNONE, buf, size, encr_data) != 0) {
			dbg_a (0x0500 | in_header(buf, msg_type)); // Tx failed
		}
		net_opt (PHYSOPT_TXOFF, NULL);
	} else {
		if (net_tx (WNONE, buf, size, encr_data) != 0) {
			dbg_a (0x0500 | in_header(buf, msg_type));
		}
	}
}

void app_leds (const word act) {
	leds (CON_LED, act);
#if CON_ON_PINS
	switch (act) {
		case LED_BLINK:
			pin_write (LED_PIN0, 0);
			if (is_fastblink)
				pin_write (LED_PIN1, 0);
			else
				pin_write (LED_PIN1, 1);
			break;
		case LED_ON:
			pin_write (LED_PIN0, 1);
			pin_write (LED_PIN1, 1);
			break;
		default: // OFF
			pin_write (LED_PIN0, 1);
			pin_write (LED_PIN1, 0);
	}
#endif
}

