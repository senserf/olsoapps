/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "msg_vmeshStructs.h"
#include "lib_app_if.h"
#include "net.h"
#include "tarp.h"
#include "codes.h"
#include "nvm.h"
#include "pinopts.h"
#include "lhold.h"

extern tarpCtrlType tarp_ctrl;

#if UART_DRIVER

#define OO_RETRY 00
process (oss_out, char)
	entry (OO_RETRY)
		app_ser_out (OO_RETRY, data, NO);
		finish;
endprocess (0)
#undef  OO_RETRY

#endif

void oss_set_in () {
  switch (cmd_line[1]) {
	  word val;
	  
	case PAR_LH:
		if (cmd_ctrl.oplen != 1 + sizeof(nid_t)) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		memcpy (&val, cmd_line +2, sizeof(nid_t));
		if (val == 0) {
			cmd_ctrl.oprc = RC_EVAL;
			break;
		}
		cmd_ctrl.oprc = RC_OK;
		if (val == local_host)
			break;

		if (cmd_ctrl.s == local_host) // follow, it has a meaning
			cmd_ctrl.s = val;

		local_host = val;
		nvm_write (NVM_LH, &local_host, 1);
		break;

	case PAR_NID:
		if (cmd_ctrl.oplen != 1 + sizeof(nid_t)) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		memcpy (&val, cmd_line +2, sizeof(nid_t));
		cmd_ctrl.oprc = RC_OK;
		if (val == net_id)
			break;

		if (net_id == 0) { // 1st "bind": kill the beacon with msg_new
			freqs &= 0xFF00;
			trigger (BEAC_TRIG);
		}
		net_id = val;
		nvm_write (NVM_NID, &net_id, 1);
		if (net_id == 0)
			reset();
		net_opt (PHYSOPT_SETSID, &net_id);
		break;

	case PAR_TARP_L:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		tarp_ctrl.param = (tarp_ctrl.param & 0x3F) | (cmd_line[2] << 6);
		cmd_ctrl.oprc = RC_OK;
		nvm_read (NVM_APP, &val, 1);
		val &= 0x00FF;
		val |= (word)tarp_ctrl.param << 8;
		nvm_write (NVM_APP, &val, 1);
		break;

	case PAR_TARP_S:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		tarp_ctrl.param = 
			(tarp_ctrl.param & 0xF1) | ((cmd_line[2] & 7) << 1);
		cmd_ctrl.oprc = RC_OK;
		nvm_read (NVM_APP, &val, 1);
		val &= 0x00FF;
		val |= (word)tarp_ctrl.param << 8;
		nvm_write (NVM_APP, &val, 1);
		break;
		
	case PAR_TARP_R:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		tarp_ctrl.param = 
			(tarp_ctrl.param & 0xCF) | ((cmd_line[2] & 3) << 4);
		cmd_ctrl.oprc = RC_OK;
		nvm_read (NVM_APP, &val, 1);
		val &= 0x00FF;
		val |= (word)tarp_ctrl.param << 8;
		nvm_write (NVM_APP, &val, 1);
		break;

	case PAR_TARP_F:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		tarp_ctrl.param = (tarp_ctrl.param & 0xFE) | (cmd_line[2] & 1);
		cmd_ctrl.oprc = RC_OK;
		nvm_read (NVM_APP, &val, 1);
		val &= 0x00FF;
		val |= (word)tarp_ctrl.param << 8;
		nvm_write (NVM_APP, &val, 1);
		break;

	case PAR_ENCR_M:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		set_encr_mode(cmd_line[2]);
		nvm_read (NVM_APP, &val, 1);
		val &= 0xFFF0;
		val |= encr_data;
		nvm_write (NVM_APP, &val, 1);
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_ENCR_K:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		set_encr_key(cmd_line[2]);
		nvm_read (NVM_APP, &val, 1);
		val &= 0xFFF0;
		val |= encr_data; 
		nvm_write (NVM_APP, &val, 1);
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_ENCR_D:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		val = cmd_line[2];
		set_encr_data(val);
		nvm_read (NVM_APP, &val, 1);
		val &= 0xFFF0;
		val |= encr_data;
		nvm_write (NVM_APP, &val, 1);
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_BEAC:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		val = freqs;
		freqs = (freqs & 0xFF00) | cmd_line[2];
		if (beac_freq > 63)
			freqs =  (freqs & 0xFF00) | 63;
		if (val != freqs)
			trigger (BEAC_TRIG);
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_CON:
		if (cmd_ctrl.oplen != 3) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		val = freqs;
		freqs = (freqs & 0x00FF) | ((word)cmd_line[2] << 8);
		if (audit_freq > 63)
			freqs =  (freqs & 0x00FF) | 0x3FFF; // 63 s
		connect = (connect & 0x00FF) | ((word)cmd_line[3] << 8);
		if (val != freqs)
			trigger (CON_TRIG);
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_RF:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		if (cmd_line[2] > 3) {
			cmd_ctrl.oprc = RC_EVAL;
			break;
		}
		if (cmd_line[2] & 1)
			net_opt (PHYSOPT_RXON, NULL);
		else
			net_opt (PHYSOPT_RXOFF, NULL);
		if (cmd_line[2] & 2)
			net_opt (PHYSOPT_TXON, NULL);
		else
			net_opt (PHYSOPT_TXOFF, NULL);
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_ALRM_DONT:
		if (cmd_ctrl.oplen != 1 + sizeof(lword) + 1) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		memcpy (&br_ctrl.dont_esn, cmd_line +2, sizeof(lword));
		br_ctrl.dont_s = cmd_line[6];
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_STAT_REP:
		if (cmd_ctrl.oplen != 1 + 2 + 1) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		val = br_ctrl.rep_freq >> 1;
		memcpy (&br_ctrl.rep_freq, cmd_line +2, 2);
		br_ctrl.rep_freq <<= 1;
		if (cmd_line[4] & 1)
			br_ctrl.rep_freq++;
		if (running (st_rep) && val != (br_ctrl.rep_freq >> 1))
			trigger (ST_REPTRIG);
		else if (val == 0 && (br_ctrl.rep_freq >> 1) != 0 &&
				master_host != 0 && is_cmdmode)
			fork (st_rep, NULL);
		cmd_ctrl.oprc = RC_OK; 
		break;

	case PAR_RETRY:
		if (cmd_ctrl.oplen != 3) { 
			cmd_ctrl.oprc = RC_ELEN; 
			break; 
		}
		set_retries(cmd_line[2]);
		set_tout(cmd_line[3]);
		cmd_ctrl.oprc = RC_OK;
		break;

	 case PAR_AUTOACK:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		if (cmd_line[2] &1)
			set_autoack;
		else
			clr_autoack;
		cmd_ctrl.oprc = RC_OK; 
		break;

	case PAR_UART:
		if (cmd_ctrl.oplen != 3) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		memcpy (&val, cmd_line +2, 2);
		if (val != 12 && val != 24 && val != 48 && val != 96 &&
				val != 192 && val != 384) {
			cmd_ctrl.oprc = RC_EVAL;
			break;
		}
		ion (UART, CONTROL, (char*)&val, UART_CNTRL_SETRATE);
		cmd_ctrl.oprc = RC_OK;
		break;

	 case PAR_CYC_CTRL:
		if (cmd_ctrl.oplen != 3) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		if (cmd_line[2] > 3 || cmd_line[3] > 3) {
			cmd_ctrl.oprc = RC_EVAL;
			break;
		}
		if (local_host == master_host) {
			if (cmd_line[2] == cyc_ctrl.st ||
				cmd_line[2] == CYC_ST_SLEEP || 
				cmd_line[3] != CYC_MOD_NET) {
				cmd_ctrl.oprc = RC_EMAS;
				break;
			}
			if (cmd_line[2] != CYC_ST_DIS &&
					cyc_ctrl.st != CYC_ST_DIS) {
				cmd_ctrl.oprc = RC_ERES;
				break;
			}
			cyc_ctrl.st = cmd_line[2];
			// don't, it is overloaded:
		       //	cyc_ctrl.mod = cmd_line[2] & 3;
			nvm_write (NVM_CYC_CTRL, (address)&cyc_ctrl, 1);
			cmd_ctrl.oprc = RC_OK;

			// no matter what, kill the old one:
			if ((val = running (cyc_man)) != 0)
				 kill (val);
			if (cyc_ctrl.st == CYC_ST_DIS) {
				if (val == 0) // old cyc_man should be there
					dbg_2 (0xC2F1);
			} else {
				if (val != 0) // should not
					dbg_2 (0xC2F2);
				fork (cyc_man, NULL);
			}
			break;

		}
		// not master
		if (cyc_ctrl.mod != cmd_line[3] &&
				cyc_ctrl.st != CYC_ST_DIS) {
			cmd_ctrl.oprc = RC_ERES;
			break;
		}
		if (cmd_line[2] != cyc_ctrl.st &&
			((cmd_line[2] != CYC_ST_DIS &&
			 cmd_line[2] != CYC_ST_ENA) ||
				(cmd_line[2] == CYC_ST_ENA &&
				cmd_line[3] != CYC_MOD_NET &&
				cmd_line[3] != CYC_MOD_PNET))) {
			cmd_ctrl.oprc = RC_EVAL;
			break;
		}
		if (cyc_ctrl.st == CYC_ST_PREP ||
			(cyc_ctrl.st == CYC_ST_SLEEP && 
				cyc_ctrl.mod != CYC_MOD_PNET)) {
			cmd_ctrl.oprc = RC_ERES;
			break;
		}
		if (running (cyc_man)) {
			dbg_2 (0xC2F3);
			kill (running (cyc_man));
		}
		cyc_ctrl.st = cmd_line[2];
		cyc_ctrl.mod = cmd_line[3];
		nvm_write (NVM_CYC_CTRL, (address)&cyc_ctrl, 1);
		cmd_ctrl.oprc = RC_OK;
		// who knows how we get any consistency here...
		if (cyc_ctrl.mod == CYC_MOD_POFF) {
			net_opt (PHYSOPT_RXOFF, NULL);
			net_opt (PHYSOPT_TXOFF, NULL);
		} else if (cyc_ctrl.mod == CYC_MOD_PON) {
			net_opt (PHYSOPT_RXON, NULL);
			net_opt (PHYSOPT_TXON, NULL);
		}
		// ... and don't touch  NET, PNET (?)
		break;

	 case PAR_CYC_SLEEP:
		if (cmd_ctrl.oplen != 5) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		if (local_host == master_host) {
			cmd_ctrl.oprc = RC_EMAS;
			break;
		}
		if (cyc_ctrl.st != CYC_ST_DIS) {
			cmd_ctrl.oprc = RC_ERES;
			break;
		}
		memcpy (&cyc_sp, cmd_line +2, 4);
		nvm_write (NVM_CYC_SP, (address)&cyc_sp, 2);
		cmd_ctrl.oprc = RC_OK;
		break;

	 case PAR_CYC_M_SYNC:
		if (cmd_ctrl.oplen != 3) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		if (local_host != master_host) {
			cmd_ctrl.oprc = RC_EMAS;
			break;
		}
		memcpy (&val, cmd_line +2, 2);
		if (cyc_ctrl.st == CYC_ST_PREP && val != 0 &&
			val != CYC_MSG_FORCE_DIS && val != CYC_MSG_FORCE_ENA) {
			cmd_ctrl.oprc = RC_ERES;
			break;
		}
		if (val > 0x0FFF && val != CYC_MSG_FORCE_ENA &&
				val != CYC_MSG_FORCE_DIS) {
			cmd_ctrl.oprc = RC_EVAL;
			break;
		}
		cyc_ctrl.prep = val;
		switch (val) {
			case CYC_MSG_FORCE_ENA:
				cyc_ctrl.mod = CYC_MOD_PON;
				break;
			case CYC_MSG_FORCE_DIS:
				cyc_ctrl.mod = CYC_MOD_POFF;
				break;
			default:
				cyc_ctrl.mod = CYC_MOD_NET;
		}
		nvm_write (NVM_CYC_CTRL, (address)&cyc_ctrl, 1);
		cmd_ctrl.oprc = RC_OK;
		break;

	 case PAR_CYC_M_REST:
		if (cmd_ctrl.oplen != 5) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		if (local_host == master_host) {
			cmd_ctrl.oprc = RC_EMAS;
			break;
		}
		memcpy (&cyc_sp, cmd_line +2, 4);
		if (cyc_sp == 0 && (val = running (cyc_man)) != 0)
			kill (val);
		nvm_write (NVM_CYC_SP, (address)&cyc_sp, 2);
		cmd_ctrl.oprc = RC_OK;
		break;

	 case PAR_BINDER:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		if ((cmd_line[2] == 0 && is_binder) ||
			(cmd_line[2] != 0 && !is_binder)) {
			nvm_read (NVM_APP, &val, 1);
			val &= ~16; //b4 in NVM_APP
			if (cmd_line[2] != 0) {
				val |= 16;
				set_binder;
			} else
				clr_binder;
			nvm_write (NVM_APP, &val, 1);
		}
		cmd_ctrl.oprc = RC_OK;
		break;

	 case PAR_CMD_MODE:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		if (cmd_line[2] == 0 && local_host == master_host) {
			cmd_ctrl.oprc = RC_EMAS;
			break;
		}
		if ((cmd_line[2] == 0 && is_cmdmode) ||
			(cmd_line[2] != 0 && !is_cmdmode)) {
			nvm_read (NVM_APP, &val, 1);
			val &= ~32; // b5 in NVM_APP
			if (cmd_line[2] != 0) {
				val |= 32;
				set_cmdmode;
				if (running (dat_in))
					kill (running (dat_in));
				if (!running (cmd_in))
					fork (cmd_in, NULL);
				// if dat_rep's running, it'll fork
				// st_rep at finish
				if (!running (dat_rep))
					fork (st_rep, NULL);
			} else {
				clr_cmdmode;
				if (running (cmd_in))
					kill (running (cmd_in));
				if (!running (dat_in))
					fork (dat_in, NULL);
				if (running (st_rep))
					kill (running (st_rep));
			}
			nvm_write (NVM_APP, &val, 1);
		}
		cmd_ctrl.oprc = RC_OK;
		break;

	default:
		cmd_ctrl.oprc = RC_EPAR;
  }
}

void oss_get_in (word state) {
	long l;
	word p, p1;

	if (cmd_ctrl.oplen != 1) {
		cmd_ctrl.oprc = RC_ELEN;
		return;
	}
	cmd_ctrl.oprc = RC_OKRET;

	switch (cmd_line[1]) {
	  case PAR_LH:
		cmd_ctrl.oplen += sizeof(nid_t);
		memcpy (cmd_line +2, &local_host, sizeof(nid_t));
		return;

	  case ATTR_ESN:
		cmd_ctrl.oplen += sizeof(lword);
		memcpy (cmd_line +2, &ESN, sizeof(lword));
		return;

	  case ATTR_MID:
		cmd_ctrl.oplen += sizeof(nid_t);
		memcpy (cmd_line +2, &master_host, sizeof(nid_t));
		return;

	  case PAR_NID:
		cmd_ctrl.oplen += sizeof(nid_t);
		memcpy (cmd_line +2, &net_id, sizeof(nid_t));
		return;
		
	  case PAR_TARP_L:
	  case PAR_TARP_S:
	  case PAR_TARP_R:
	  case PAR_TARP_F:
		cmd_ctrl.oplen++;
		cmd_line[2] = tarp_ctrl.param;
		return;

	  case PAR_ENCR_M:
	  case PAR_ENCR_K:
	  case PAR_ENCR_D:
		cmd_ctrl.oplen++;
		cmd_line[2] = encr_data;
		return;

	  case PAR_BEAC:
		cmd_ctrl.oplen++;
		cmd_line[2] = beac_freq;
		return;

	  case PAR_CON:
		cmd_ctrl.oplen += sizeof(nid_t) +1;
		cmd_line[2] = audit_freq;
		cmd_line[3] = connect >> 8;
		cmd_line[4] = connect;
		return;

	  case PAR_RF:
		cmd_ctrl.oplen++;
		cmd_line[2] = net_opt (PHYSOPT_STATUS, NULL);
		return;

	  case PAR_ALRM_DONT:
		cmd_ctrl.oplen += sizeof(lword) +1;
		memcpy (cmd_line +2, &br_ctrl.dont_esn, sizeof(lword));
		cmd_line[6] = br_ctrl.dont_s;
		return;

	  case PAR_STAT_REP:
		cmd_ctrl.oplen += 2 + 1 + 2;
		p = br_ctrl.rep_freq >> 1;
		memcpy (cmd_line +2, &p, 2);
		cmd_line[4] = br_ctrl.rep_freq & 1;
		if ((p = running(st_rep)) != 0) {
			p = ldleft (p, NULL);
			if (p == MAX_UINT)
				p = 0;
		} else
			p = MAX_UINT; // not running
		memcpy (cmd_line +5, &p, 2);
		return;

	  case PAR_RETRY:
		 cmd_ctrl.oplen += 2;
		 cmd_line[2] = ack_retries;
		 cmd_line[3] = ack_tout;
		 return;

	  case PAR_AUTOACK:
		 cmd_ctrl.oplen++;
		 cmd_line[2] = is_autoack;
		 return;

	  case PAR_UART:
		cmd_ctrl.oplen += 2;
		ion (UART, CONTROL, (char*) &p, UART_CNTRL_GETRATE);
		memcpy (cmd_line +2, &p, 2);
		return;
		 
	  case ATTR_UPTIME:
		 cmd_ctrl.oplen += 4;
		 l = seconds();
		 memcpy (cmd_line +2, &l, 4);
		 return;

	  case ATTR_RSSI:
		cmd_ctrl.oplen += 2;
		memcpy (cmd_line +2, &l_rssi, 2);
		return;

	  case ATTR_MSG_COUNT_T:
		cmd_ctrl.oplen += 6;
		memcpy (cmd_line +2, &tarp_ctrl.rcv, 2);
		memcpy (cmd_line +4, &tarp_ctrl.snd, 2);
		memcpy (cmd_line +6, &tarp_ctrl.fwd, 2);
		return;

	  case ATTR_SYSVER:
		cmd_ctrl.oplen += 2;
		cmd_line[2] = SYSVER_B >> 8;
		cmd_line[3] = (byte)SYSVER_B;
		return;

	  case ATTR_MEM1:
		cmd_ctrl.oplen += 6;
#if MALLOC_STATS
		p = memfree(0, &p1);
		memcpy (cmd_line +2, &p, 2);
		memcpy (cmd_line +4, &p1, 2);
		p = stackfree();
		memcpy (cmd_line +6, &p, 2);
#else
		memset (cmd_line +2, 0, 6);
#endif
		return;

	  case ATTR_MEM2:
		cmd_ctrl.oplen += 6;
#if MALLOC_STATS
		p = maxfree(0, &p1);
		memcpy (cmd_line +2, &p, 2);
		memcpy (cmd_line +4, &p1, 2);
		p = staticsize();
		memcpy (cmd_line +6, &p, 2);
#else
		memset (cmd_line +2, 0, 6);
#endif
		return;

	  case PAR_CYC_CTRL:
		cmd_ctrl.oplen += 2;
		cmd_line[2] = cyc_ctrl.st;
		cmd_line[3] = cyc_ctrl.mod;
		return;

	  case PAR_CYC_SLEEP:
		if (local_host == master_host) {
			cmd_ctrl.oprc = RC_EMAS;
			return;
		}
		cmd_ctrl.oplen += 4;
		memcpy (cmd_line +2, &cyc_sp, 4);
		return;

	  case PAR_CYC_M_REST:
		if (local_host != master_host) {
			cmd_ctrl.oprc = RC_EMAS;
			return;
		}
		cmd_ctrl.oplen += 4;
		memcpy (cmd_line +2, &cyc_sp, 4);
		return;

	  case PAR_CYC_M_SYNC:
		if (local_host != master_host) {
			cmd_ctrl.oprc = RC_EMAS;
			return;
		}
		cmd_ctrl.oplen += 2;
		switch (cyc_ctrl.mod) {
			case CYC_MOD_PON:
				p = CYC_MSG_FORCE_ENA;
				break;
			case CYC_MOD_POFF:
				p = CYC_MSG_FORCE_DIS;
				break;
			default:
				p = cyc_ctrl.prep;
		}
		memcpy (cmd_line +2, &p, 2);
		return;

	  case ATTR_CYC_LEFT:
		cmd_ctrl.oplen += 6;
		if ((p = running (cyc_man)) == 0)
			l = 0;
		else
			l = lhleft (p, &cyc_left);
		memcpy (cmd_line +2, &l, 4);
		cmd_line[6] = cyc_ctrl.st;
		cmd_line[7] = cyc_ctrl.mod;
		return;

	  case ATTR_NHOOD:
		// let's do it as GET... I feel sooner or later
		// we'll have a request to return some local data...

		// don't return by default
		if (msg_nh_out())
			cmd_ctrl.oprc = RC_OK;
		else
			cmd_ctrl.oprc = RC_ERES;
		return;

	  case PAR_BINDER:
		cmd_ctrl.oplen++;
		cmd_line[2] = is_binder ? 1 : 0;
		return;

	  case PAR_CMD_MODE:
		cmd_ctrl.oplen++;
		cmd_line[2] = is_cmdmode ? 1 : 0;
		return;

	  default:
		cmd_ctrl.oprc = RC_EPAR;
	}
}

void oss_ret_out (word state) {
#if UART_DRIVER
	char * buf = get_mem (state, RET_HEAD_LEN + cmd_ctrl.oplen +3);
	word oset = 0;

	buf[oset++] = '\0';
	buf[oset++] = RET_HEAD_LEN + cmd_ctrl.oplen;
	buf[oset++] = cmd_ctrl.opcode;
	buf[oset++] = cmd_ctrl.opref;
	buf[oset++] = cmd_ctrl.oprc;
	memcpy (buf + oset, &cmd_ctrl.t, sizeof(nid_t));
	oset += sizeof(nid_t);
	memcpy (buf + oset, cmd_line +1, cmd_ctrl.oplen);
	oset += cmd_ctrl.oplen;
	buf[oset] = 0x04;
        if (fork (oss_out, buf) == 0 ) {
		dbg_2 (0xC403); // oss_ret_out fork failed
		ufree (buf);
	}
#endif
}

void oss_master_in (word state) {
	char * out_buf = NULL;

	cmd_ctrl.oprc = RC_OK; // ok by default
	// local_host becomes master_host...
	// cmd_ctrl.t == 0 is not good if we want to stack masters,
	// but if we do, or if we want multiple masters within the same
	// net_id, we have to restrict master assignments.
	if (cmd_ctrl.t == 0 || local_host == cmd_ctrl.t) {
		if (master_host != local_host) {
			master_host = local_host;
			nvm_write (NVM_MID, &master_host, 1);
			tarp_ctrl.param &= 0xFE; // routing off
			leds (CON_LED, LED_ON);
			// operator's inervention is likely, just adjust basics:
			cyc_ctrl.st = CYC_ST_DIS;
			cyc_ctrl.mod = CYC_MOD_NET;
			cyc_sp = DEFAULT_CYC_SP + DEFAULT_CYC_M_REST;
			if (running (cyc_man))
				kill (running (cyc_man));
			// to avoid wasting flash: no writes; if there
			// was anything customized there, better cold reboot
		}
		if (local_host == cmd_ctrl.t)
			return;
	}

	msg_master_out (state, &out_buf);
	send_msg (out_buf, sizeof(msgMasterType));
	if (beac_freq == 0 || running(beacon)) {
		ufree (out_buf);
	} else {
		if (fork (beacon, out_buf) == 0) {
			dbg_2 (0xC405); // master beacon fork failed
			ufree (out_buf);
			cmd_ctrl.oprc = RC_EFORK;
		} // beacon frees out_buf at exit
	}
}


// 0x00 <1-len> 0xFF msg_traceAck  <1-fcount> <1-bcount> <node> ... 0x04
void oss_traceAck_out (word state, char * buf) {
#if UART_DRIVER
	int num = 0;
	if (in_header(buf, msg_type) != msg_traceBAck)
		num = in_traceAck(buf, fcount);
	if (in_header(buf, msg_type) != msg_traceFAck)
		num += in_header(buf, hoc);
	if (in_header(buf, msg_type) == msg_traceAck)
		num--; // dst counted twice
	char * lbuf = get_mem (state, num * sizeof (nid_t) +7);
	lbuf[0] = '\0';
	lbuf[1] = num * sizeof (nid_t) +4;
	lbuf[2] = CMD_MSGOUT;
	lbuf[3] = in_header(buf, msg_type);
	lbuf[4] = in_traceAck(buf, fcount);
	lbuf[5] = in_header(buf, hoc);
	
	memcpy (lbuf +6, buf + sizeof(msgTraceAckType), num * sizeof(nid_t));
	lbuf[num * sizeof (nid_t) +6] = 0x04;

	if (fork (oss_out, lbuf) == 0 ) {
		dbg_2 (0xC406); // oss_traceAck_out fork failed
		ufree (lbuf);
	}
#endif
}

void oss_bindReq_out (char * buf) {
#if UART_DRIVER
	char * lbuf = get_mem (NONE, 4 + 8 + 1);
	if (lbuf == NULL)
		return;
	lbuf[0] = '\0';
	lbuf[1] = 2 + 8;
	lbuf[2] = CMD_MSGOUT;
	lbuf[3] = msg_bindReq;
	switch (in_header(buf, msg_type)) {
	  case msg_new:
		memcpy (lbuf +4, &in_new(buf, esn_l), 4);
		memcpy (lbuf +8, &in_header(buf, snd), 2);
		memcpy (lbuf +10, &local_host, 2);
		break;
	  case msg_bindReq:
		memcpy (lbuf +4, &in_bindReq(buf, esn_l), 4); 
		memcpy (lbuf +8, &in_bindReq(buf, lh), 2);
		memcpy (lbuf +10, &in_header(buf, snd), 2); 
		break;
	  default:
		dbg_2 (0xBA00 | in_header(buf, msg_type)); // bad bindReq
		ufree (lbuf);
		return;
	}
	lbuf[12] = 0x04;
	if (fork (oss_out, lbuf) == 0 ) {
		dbg_2 (0xC404); // oss_bindReq_out fork failed
		ufree (lbuf);
	}
#endif
}

void oss_nhAck_out (char * buf) {
#if UART_DRIVER
	char * lbuf = get_mem (NONE, 4 + 9 + 1);
	if (lbuf == NULL)
		return;
	lbuf[0] = '\0';
	lbuf[1] = 2 + 9;
	lbuf[2] = CMD_MSGOUT;
	lbuf[3] = msg_nhAck;
	switch (in_header(buf, msg_type)) {
	  case msg_nh:
		memcpy (lbuf +4, &in_header(buf, snd), 2);
		memcpy (lbuf +6, &ESN, 4);
		memcpy (lbuf +10, &local_host, 2);
		lbuf[12] = 0;
		break;
	  case msg_nhAck:
	  	memcpy (lbuf +4, &in_nhAck(buf, host), 2);
		memcpy (lbuf +6, &in_nhAck(buf, esn_l), 4);
		memcpy (lbuf +10, &in_header(buf, snd), 2);
		lbuf[12] = in_header(buf, hoc);
		break;
	  default:
		dbg_2 (0xBA00 | in_header(buf, msg_type)); // bad nhAck
		ufree (lbuf);
		return;
	}
	lbuf[13] = 0x04;
	if (fork (oss_out, lbuf) == 0 ) {
		dbg_2 (0xC404); // oss_nhAck_out fork failed ????
		ufree (lbuf);
	}
#endif
}

void oss_alrm_out (char * buf) {
#if UART_DRIVER
	char * lbuf;
	word len = buf[sizeof(msgAlrmType)];
	if (len > UART_INPUT_BUFFER_LENGTH - 12)
		return;
	if ((lbuf = get_mem (NONE, 4 + 10 + 1 +len)) == NULL)
		return;
	lbuf[0] = '\0';
	lbuf[1] = 2 + 10 + len;
	lbuf[2] = CMD_MSGOUT;
	lbuf[3] = msg_alrm;
	memcpy (lbuf +4, &in_header(buf, snd), 2);
	lbuf[6] = in_alrm(buf, typ);
	lbuf[7] = in_alrm(buf, rev);
	memcpy (lbuf +8, &in_alrm(buf, esn_l), 4); //cheating
	lbuf[12] = in_alrm(buf, s);
	if (len)
		memcpy (lbuf +13, buf + sizeof(msgAlrmType) +1, len);
	lbuf[13 + len] = in_alrm(buf, rssi);
	lbuf[14 + len] = 0x04;
	if (fork (oss_out, lbuf) == 0 ) {
		dbg_2 (0xC407); // oss_alrm_out fork failed
		ufree (lbuf);
	}
#endif
}

void oss_br_out (char * buf, bool acked) {
#if UART_DRIVER
	char * lbuf = get_mem (NONE, 4 + 5 + 1);
	if (lbuf == NULL)
		return;
	lbuf[0] = '\0';
	lbuf[1] = 2 + 5;
	lbuf[2] = CMD_MSGOUT;
	lbuf[3] = msg_br;
	memcpy (lbuf +4, &in_header(buf, snd), 2);
	memcpy (lbuf +6, &in_br(buf, con), 2);
	lbuf[8] = acked;
	lbuf[9] = 0x04;
	if (fork (oss_out, lbuf) == 0 ) {
		dbg_2 (0xC408); // oss_br_out fork failed
		ufree (lbuf);
	}
#endif
}

void oss_datack_out (char * buf) {
#if UART_DRIVER
	char * lbuf = get_mem (NONE, 4 + sizeof(nid_t) +1 +1);
	if (lbuf == NULL)
		return;
	lbuf[0] = '\0';
	lbuf[1] = 2 + sizeof(nid_t) +1;
	lbuf[2] = CMD_MSGOUT; 
	lbuf[3] = msg_datAck;
	memcpy (lbuf +4, &in_header(buf, snd), 2);
	lbuf[6] = in_dat(buf, ref);
	lbuf[7] = 0x04;
	if (fork (oss_out, lbuf) == 0 ) {
		dbg_2 (0xC40C); // oss_datAck_out fork falied
		ufree (lbuf);
	}
#endif
}

void oss_dat_out (char * buf, bool acked) {
#if UART_DRIVER
	char * lbuf;
	word w;
	if (is_cmdmode) {
		w = 4 + sizeof(nid_t) +1 + in_dat(buf, len) +1 +1;
		lbuf = get_mem (NONE, w);
		if (lbuf == NULL) 
			return; 
		lbuf[0] = '\0'; 
		lbuf[1] = w - 2 -1;
		lbuf[2] = CMD_MSGOUT; 
		lbuf[3] = msg_dat;
		memcpy (lbuf +4, &in_header(buf, snd), 2);
		lbuf[6] = in_dat(buf, ref);
		memcpy (lbuf +7, buf + sizeof (msgDatType), in_dat(buf, len));
		w = 7 + in_dat(buf, len);
		lbuf[w++] = acked;
		lbuf[w] = 0x04;
	} else {
		lbuf = get_mem (NONE, 2 + in_dat(buf, len));
		if (lbuf == NULL)  
			return;  
		lbuf[0] = 1; // !zero <-> raw output
		lbuf[1] = in_dat(buf, len);
		memcpy (lbuf +2, buf + sizeof (msgDatType), in_dat(buf, len));
	}
	if (fork (oss_out, lbuf) == 0 ) {
		dbg_2 (0xC40B); // oss_dat_out fork failed
		ufree (lbuf);
	}
#endif 
}

void oss_io_out (char * buf, bool acked) {
#if UART_DRIVER
	char * lbuf = get_mem (NONE, 4 + 7 +1);
	if (lbuf == NULL)
		return;
	lbuf[0] = '\0';
	lbuf[1] = 2 + 7;
	lbuf[2] = CMD_MSGOUT;
	lbuf[3] = msg_io;
	if (buf) {
		memcpy (lbuf +4, &in_header(buf, snd), 2);
		memcpy (lbuf +6, &in_io(buf, pload), 4);
	} else { // local
		lbuf[4] = local_host;
		memcpy (lbuf +6, &io_pload, 4);
	}
	lbuf[10] = acked;
	lbuf[11] = 0x04;
	if (fork (oss_out, lbuf) == 0 ) {
		dbg_2 (0xC40A); // oss_io_out fork failed
		ufree (lbuf);
	}
#endif
}

void oss_trace_in (word state) {
	char * out_buf = NULL;
	if (cmd_ctrl.oplen != 2) {
		cmd_ctrl.oprc = RC_ELEN;
		return;
	}
	cmd_ctrl.oprc = RC_OK;
	msg_trace_out (state, &out_buf);
	send_msg (out_buf, sizeof(msgTraceType));
	ufree (out_buf);
}

void oss_bind_in (word state) {
	char * out_buf = NULL;
	cmd_ctrl.oprc = RC_OK;
	msg_bind_out (state, &out_buf);
	send_msg (out_buf, sizeof(msgBindType));
	ufree (out_buf);
}

void oss_dat_in () {
	if (cmd_ctrl.t == local_host) {
		cmd_ctrl.oprc = RC_ERES;
		return;
	}
#if 0
	if (cmd_ctrl.oplen == 0) {
		cmd_ctrl.oprc = RC_ELEN;
		return;
	}
#endif
	if (cmd_ctrl.oplen > 64) {
		cmd_ctrl.oprc = RC_ELEN;
		return;
	}
	cmd_ctrl.oprc = msg_dat_out ();
}

void oss_datack_in () {
	if (cmd_ctrl.t == local_host) {
		cmd_ctrl.oprc = RC_ERES;
		return;
	}
	cmd_ctrl.oprc = msg_datAck_out(NULL) ? RC_OK : RC_EMEM;
}

void oss_sack_in () {
	cmd_ctrl.oprc = msg_stAck_out(NULL) ? RC_OK : RC_EMEM;
}

void oss_ioack_in () {
	cmd_ctrl.oprc = msg_ioAck_out(NULL) ? RC_OK : RC_EMEM;
}

static void pin_backup (word pin, word val) {
	lword l;
	word def, nvm_val;
	
	switch (pin) {
		case 1:
		case 2:
		case 3:
		case 8:
		case 9:
		case 10:
			def = 2;
		case 4:
		case 5:
			def = 1;
		case 6:
		case 7:
			def = 4;
		default:
			def = 0xEE;
	}
	nvm_read (NVM_IO_PINS, (address)&l, 2);
	nvm_val = (l >> ((pin -1) * 3)) & 7;
	if (val != nvm_val && (val != def || nvm_val != 7)) {
		l &= ~(7L << ((pin -1) * 3));
		l |= (lword)val << ((pin -1) * 3);
		nvm_write (NVM_IO_PINS, (address)&l, 2);
	}
}

#define IO_READ		0
#define IO_WRITE 	1
#define IO_ADC		2
#define IO_CNT_START	3
#define IO_CMP_START    4
#define IO_NOT_START    5
#define IO_STOP		6
#define IO_CNT_GET	7
#define IO_CMP_GET	8
#define IO_STATE	9
#define IO_CREG_GET	10
#define IO_CREG_SET	11
#define IO_FLAGS_GET	12
#define IO_FLAGS_SET	13

void oss_io_in() {
	lword l;
	word w, w1;

	if (cmd_ctrl.oplen != 4) {
		cmd_ctrl.oprc = RC_ELEN;
		return;
	}
	switch (cmd_line[1] & 0x0F) {
		case IO_READ:
			if (cmd_line[2] == 0xFF) { // bulk
				l = 0;
				for (w1 = 1; w1 < 11; w1++) {
					if ((w = pin_read (w1)) & 8)
						w = w & 1 ? 7 : 6;
					l |= (lword)w << 3 * (w1 -1);
				}
				memcpy (cmd_line +1, &l, 4);
				cmd_ctrl.oprc = RC_OKRET;
				return;
			}
			if (cmd_line[2] < 1 || cmd_line[2] > 10) {
				cmd_ctrl.oprc = RC_EVAL;
				return;
			}
			if ((w = pin_read (cmd_line[2])) & 8)
				w = w & 1 ? 7 : 6;
			cmd_line[3] = w;
			cmd_ctrl.oprc = RC_OKRET;
			return;

		case IO_WRITE:
			// analog in on pin > 7 is illegal
			if (cmd_line[2] < 1 || cmd_line[2] > 10 ||
				(cmd_line[2] > 7 && (cmd_line[3] & 4)) ||
				cmd_line[3] == 3 || cmd_line[3] > 4) {
				cmd_ctrl.oprc = RC_EVAL;
				return;
			}
			if (pin_write (cmd_line[2], cmd_line[3]) < 0) {
				cmd_ctrl.oprc = RC_ERES;
			} else {
				cmd_ctrl.oprc = RC_OKRET;
				pin_backup (cmd_line[2], cmd_line[3]);
			}
			return;

		case IO_ADC:
			if ((w = cmd_line[2] & 0x0F) < 1 || w > 7 ||
					(w1 = cmd_line[3]) > 64 ||
					cmd_line[2] >> 4 > 2) {
				cmd_ctrl.oprc = RC_EVAL;
				return;
			}
			net_opt (PHYSOPT_RXOFF, NULL);
			w = pin_read_adc (NONE, w, cmd_line[2] >> 4, w1);
			net_opt (PHYSOPT_RXON, NULL);
			if (w == -1)
				cmd_ctrl.oprc = RC_ERES;
			else {
				cmd_ctrl.oprc = RC_OKRET;
				memcpy (cmd_line +3, &w, 2);
			}
			return;
#if PULSE_MONITOR
		case IO_CNT_START:
			if (pmon_get_state() & PMON_STATE_CNT_ON) {
				cmd_ctrl.oprc = RC_ERES;
				return;
			}
			if (cmd_line[1] & 0x20)
				pmon_start_cnt (-1, cmd_line[1] & 0x10 ? 1 : 0);
			else {
				l = 0;
				memcpy (&l, cmd_line +2, 3);
				pmon_start_cnt (l, cmd_line[1] & 0x10 ? 1 : 0);
			}
			cmd_ctrl.oprc = RC_OKRET;
			nvm_io_backup();
			return;

		case IO_CMP_START:
			if (pmon_get_state() & PMON_STATE_CMP_ON) {
				cmd_ctrl.oprc = RC_ERES;
				return;
			}
			l = 0;
			memcpy (&l, cmd_line +2, 3);
			if (!running (io_rep))
				fork (io_rep, NULL);
			pmon_set_cmp (l);
			cmd_ctrl.oprc = RC_OKRET;
			nvm_write (NVM_IO_CMP, (address)&l, 2);
			nvm_io_backup();
			return;

		case IO_NOT_START:
			if (pmon_get_state() & PMON_STATE_NOT_ON)
				cmd_ctrl.oprc = RC_ERES;
			else {
				if (!running (io_rep))
					fork (io_rep, NULL);
				cmd_ctrl.oprc = RC_OKRET;
				pmon_start_not (cmd_line[2]);
			}
			nvm_io_backup();
			return;

		case IO_STOP:
			cmd_ctrl.oprc = RC_OKRET;
			w = pmon_get_state();
			cmd_line[2] &= 0x0F; // jic; clear high bits
			if (cmd_line[2]) { // CNT
				if (w & PMON_STATE_CNT_ON)
					pmon_stop_cnt();
				else {
					cmd_ctrl.oprc = RC_ERES;
					cmd_line[2] |= 0xF0;
				}
			}
			if (cmd_line[3]) { // CMP
				if (w & PMON_STATE_CMP_ON)
					pmon_set_cmp(-1); // stop
				else {
					cmd_ctrl.oprc = RC_ERES;
					cmd_line[3] |= 0xF0;
				}
			}
			if (cmd_line[4]) { // NOT
				if (w & PMON_STATE_NOT_ON)
					pmon_stop_not();
				else {
					cmd_ctrl.oprc = RC_ERES;
					cmd_line[4] |= 0xF0;
				}
			}
			if ((w1 = running (io_rep)) != 0) {
				w = pmon_get_state();
				if (!(w & PMON_STATE_CMP_ON ||
					w & PMON_STATE_NOT_ON))
					kill (w1);
			}
			if ((cmd_line[2] & 0xF0) == 0 ||
					(cmd_line[3] & 0xF0) == 0 ||
					(cmd_line[4] & 0xF0) == 0)
				nvm_io_backup();
			return;

		case IO_CNT_GET:
			if (pmon_get_state() & PMON_STATE_CNT_ON)
				cmd_line[1] |= 0x10;
			l = pmon_get_cnt();
			memcpy (cmd_line +2, &l, 3);
			cmd_ctrl.oprc = RC_OKRET;
			return;

		case IO_CMP_GET:
			if (pmon_get_state() & PMON_STATE_CMP_ON)
				cmd_line[1] |= 0x10;
			l = pmon_get_cmp();
			memcpy (cmd_line +2, &l, 3);
			cmd_ctrl.oprc = RC_OKRET;
			return;

		case IO_STATE:
			cmd_line[2] = pmon_get_state();
			cmd_ctrl.oprc = RC_OKRET;
			return;

		case IO_CREG_GET:
			memcpy (cmd_line +2, &io_creg, 3);
			cmd_ctrl.oprc = RC_OKRET;
			return; 

		case IO_CREG_SET:
			memcpy (&io_creg, cmd_line +2, 3);
			nvm_write (NVM_IO_CREG, (address)&io_creg, 2);
			cmd_ctrl.oprc = RC_OKRET;
			return;

		case IO_FLAGS_GET:
			cmd_line[2] = io_creg >> 24;
			cmd_line[3] = cmd_line[2] & 3;
			cmd_line[2] >>= 2;
			cmd_ctrl.oprc = RC_OKRET;
			return;

		case IO_FLAGS_SET:
			if (cmd_line[2] > 63 || cmd_line[3] > 3) {
				cmd_ctrl.oprc = RC_EVAL;
				return;
			}
			w = cmd_line[2] << 2 | cmd_line[3];
			*((byte *)(&io_creg) +3) = (byte)w;
			nvm_write (NVM_IO_CREG +1, (address)&io_creg +1, 1);
			if (cmd_line[2] != 0 && !running (io_back))
				fork (io_back, NULL);
			else if (cmd_line[2] == 0 &&
					(w = running (io_back)) != 0)
				kill (w);
			cmd_ctrl.oprc = RC_OKRET;
			return;
#endif
		default:
			cmd_ctrl.oprc = RC_EPAR;
	}
}
#undef IO_READ
#undef IO_WRITE
#undef IO_ADC
#undef IO_CNT_START
#undef IO_CMP_START
#undef IO_NOT_START
#undef IO_STOP
#undef IO_CNT_GET
#undef IO_CMP_GET
#undef IO_STATE
#undef IO_CREG_GET
#undef IO_CREG_SET
#undef IO_FLAGS_GET
#undef IO_FLAGS_SET

void oss_reset_in() {
	if (cmd_ctrl.oplen != 1) {
		cmd_ctrl.oprc = RC_ELEN;
		return;
	}
	cmd_ctrl.oprc = RC_OK;
	app_reset (cmd_line[1]);
}

void oss_locale_in () {
	if (cmd_ctrl.oplen != 0) {
		cmd_ctrl.oprc = RC_ELEN;
		return;
	}
	cmd_ctrl.oplen = 6;
	cmd_ctrl.oprc = RC_OK;
	memcpy (cmd_line +1, &ESN, 4);
	memcpy (cmd_line +5, &master_host, 2);
	return;
}

