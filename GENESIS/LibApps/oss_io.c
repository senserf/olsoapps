/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "msg_geneStructs.h"
#include "lib_app_if.h"
#include "net.h"
#include "tarp.h"
#include "msg_gene.h"
#include "codes.h"
#if DM2100
#include "phys_dm2100.h"
#endif
#include "storage.h"

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
		ee_write (WNONE, EE_LH, (byte *)&local_host, 2);
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
		ee_write (WNONE, EE_NID, (byte *)&net_id, 2);
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
		ee_read (EE_APP, (byte *)&val, 2);
		val &= 0x00FF;
		val |= (word)tarp_ctrl.param << 8;
		ee_write (WNONE, EE_APP, (byte *)&val, 2);
		break;

	case PAR_TARP_S:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		tarp_ctrl.param = 
			(tarp_ctrl.param & 0xF1) | ((cmd_line[2] & 7) << 1);
		cmd_ctrl.oprc = RC_OK;
		ee_read (EE_APP, (byte *)&val, 2);
		val &= 0x00FF;
		val |= (word)tarp_ctrl.param << 8;
		ee_write (WNONE, EE_APP, (byte *)&val, 2);
		break;
		
	case PAR_TARP_R:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		tarp_ctrl.param = 
			(tarp_ctrl.param & 0xCF) | ((cmd_line[2] & 3) << 4);
		cmd_ctrl.oprc = RC_OK;
		ee_read (EE_APP, (byte *)&val, 2);
		val &= 0x00FF;
		val |= (word)tarp_ctrl.param << 8;
		ee_write (WNONE, EE_APP, (byte *)&val, 2);
		break;

	case PAR_TARP_F:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		tarp_ctrl.param = (tarp_ctrl.param & 0xFE) | (cmd_line[2] & 1);
		cmd_ctrl.oprc = RC_OK;
		ee_read (EE_APP, (byte *)&val, 2);
		val &= 0x00FF;
		val |= (word)tarp_ctrl.param << 8;
		ee_write (WNONE, EE_APP, (byte *)&val, 2);
		break;

	case PAR_ENCR_M:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		set_encr_mode(cmd_line[2]);
		ee_read (EE_APP, (byte *)&val, 2);
		val &= 0xFFF0;
		val |= encr_data;
		ee_write (WNONE, EE_APP, (byte *)&val, 2);
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_ENCR_K:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		set_encr_key(cmd_line[2]);
		ee_read (EE_APP, (byte *)&val, 2);
		val &= 0xFFF0;
		val |= encr_data; 
		ee_write (WNONE, EE_APP, (byte *)&val, 2);
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_ENCR_D:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		val = cmd_line[2];
		set_encr_data(val);
		ee_read (EE_APP, (byte *)&val, 2);
		val &= 0xFFF0;
		val |= encr_data;
		ee_write (WNONE, EE_APP, (byte *)&val, 2);
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_BEAC:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		val = freqs;
		freqs = freqs & 0xFF00 | cmd_line[2];
		if (beac_freq > 63)
			freqs =  freqs & 0xFF00 | 63;
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
		freqs = freqs & 0x00FF | ((word)cmd_line[2] << 8);
		if (audit_freq > 63)
			freqs =  freqs & 0x00FF | 0x3FFF; // 63 s
		connect = connect & 0x00FF | ((word)cmd_line[3] << 8);
		if (val != freqs)
			trigger (CON_TRIG);
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_RF:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		if (cmd_line[2] < 0 || cmd_line[2] > 3) {
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

	case PAR_RFPWR:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		if ((val = cmd_line[2]) < 0 || val > 7) {
			cmd_ctrl.oprc = RC_EVAL;
			break;
		}
		net_opt (PHYSOPT_SETPOWER, &val);
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
		if (running (st_rep) && val != (br_ctrl.rep_freq >> 1)) {
			clr_powup;
			trigger (ST_REPTRIG);
		}
		else if (val == 0 && (br_ctrl.rep_freq >> 1) != 0 &&
				master_host != 0)
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

	 case PAR_BINDER:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		if (cmd_line[2] == 0 && is_binder ||
				cmd_line[2] != 0 && !is_binder) {
			ee_read (EE_APP, (byte *)&val, 2);
			val &= ~16; //b4 in NVM_APP
			if (cmd_line[2] != 0) {
				val |= 16;
				set_binder;
			} else
				clr_binder;
			ee_write (WNONE, EE_APP, (byte *)&val, 2);
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

	  case ATTR_UPTIME:
		 cmd_ctrl.oplen += 4;
		 l = seconds();
		 memcpy (cmd_line +2, &l, 4);
		 return;

	  case ATTR_BRIDGE:
		 cmd_ctrl.oplen += 4;
		 p = esn_count;
		 memcpy (cmd_line +2, &p, 2);
		 p = s_count();
		 memcpy (cmd_line +4, &p, 2);
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

	  case PAR_RFPWR:
		cmd_ctrl.oplen++;
		net_opt (PHYSOPT_GETPOWER, &p);
		cmd_line[2] = p;
		return;

	  case ATTR_SYSVER:
		cmd_ctrl.oplen += 4;
		cmd_line[2] = SYSVER_U;
		cmd_line[3] = SYSVER_L;
		memcpy (cmd_line +4, &sensrx_ver, 2);
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

	  case ATTR_NHOOD:
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
			ee_write (WNONE, EE_MID, (byte *)&master_host, 2);
			tarp_ctrl.param &= 0xFE; // routing off
			leds (CON_LED, LED_ON);
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
			dbg_2 (0xC405); // master beacon fork filed
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

void oss_nhAck_out (char * buf, word rssi) {
#if UART_DRIVER
	char * lbuf = get_mem (NONE, 4 + 11 + 1);
	if (lbuf == NULL)
		return;
	lbuf[0] = '\0';
	lbuf[1] = 2 + 11;
	lbuf[2] = CMD_MSGOUT;
	lbuf[3] = msg_nhAck;
	switch (in_header(buf, msg_type)) {
		case msg_nh:
			memcpy (lbuf +4, &in_header(buf, snd), 2);
			memcpy (lbuf +6, &ESN, 4);
			memcpy (lbuf +10, &local_host, 2);
			memcpy (lbuf +12, &rssi, 2);
			lbuf[14] = 0;
			break;
		case msg_nhAck:
			memcpy (lbuf +4, &in_nhAck(buf, host), 2);
			memcpy (lbuf +6, &in_nhAck(buf, esn_l), 4);
			memcpy (lbuf +10, &in_header(buf, snd), 2);
			memcpy (lbuf +12, &in_nhAck(buf, rssi), 2);
			lbuf[14] = in_header(buf, hoc);
			break;
		default:
			dbg_2 (0xBA00 | in_header(buf, msg_type)); // bad nhAck
			ufree (lbuf);
			return;
	}
	lbuf[15] = 0x04;
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
	char * lbuf = get_mem (NONE, 4 + 9 + 1);
	if (lbuf == NULL)
		return;
	lbuf[0] = '\0';
	lbuf[1] = 2 + 9;
	lbuf[2] = CMD_MSGOUT;
	lbuf[3] = msg_br;
	memcpy (lbuf +4, &in_header(buf, snd), 2);
	memcpy (lbuf +6, &in_br(buf, con), 2);
	memcpy (lbuf +8, &in_br(buf, esn_no), 2);
	memcpy (lbuf +10, &in_br(buf, s_no), 2);
	lbuf[12] = acked;
	lbuf[13] = 0x04;
	if (fork (oss_out, lbuf) == 0 ) {
		dbg_2 (0xC408); // oss_br_out fork failed
		ufree (lbuf);
	}
#endif
}

void oss_st_out (char * buf, bool acked) {
#if UART_DRIVER
	char * lbuf = get_mem (NONE, 4 + 5 + (in_st(buf, count) << 2) +1);
	if (lbuf == NULL)
		return;
	lbuf[0] = '\0';
	lbuf[1] = 2 + 5 + (in_st(buf, count) << 2);
	lbuf[2] = CMD_MSGOUT;
	lbuf[3] = msg_st;
	memcpy (lbuf +4, &in_header(buf, snd), 2);
	lbuf[6] = in_st(buf, con); // only missed beats (msg_br has it all)
	lbuf[7] = in_st(buf, count);
	lbuf[8] = acked;
	memcpy (lbuf +9, buf + sizeof(msgStType), in_st(buf, count) << 2);
	lbuf[9 + (in_st(buf, count) << 2)] =  0x04; 
	if (fork (oss_out, lbuf) == 0 ) {
		dbg_2 (0xC409); // oss_st_out fork failed
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

void oss_sack_in () {
	cmd_ctrl.oprc = msg_stAck_out() ? RC_OK : RC_EMEM;
}

void oss_snack_in() {
	cmd_ctrl.oprc = msg_stNack_out (cmd_ctrl.t) ? RC_OK : RC_EMEM;
}


// 1st shot: guessing what may be useful...
#define SENS_FIND 0
#define SENS_READ 1
#define SENS_NEXT_ANY 2
#define SENS_NEXT_0 3
#define SENS_NEXT_1 4
#define SENS_WRITE 5
#define SENS_ERASE 6
void oss_sens_in() {
	lword e;
	int st;

	if (cmd_ctrl.oplen != 5) {
		cmd_ctrl.oprc = RC_ELEN;
		return;
	}
	memcpy (&e, cmd_line +1, 4);
	switch (cmd_line[5]) {
	  case SENS_FIND:
		if ((st = lookup_esn_st (&e)) < 0)
			cmd_ctrl.oprc = RC_OK;
		else {
			cmd_ctrl.oprc = RC_OKRET;
			cmd_line[5] |= (st << 4);
		}
		return;

	  case SENS_READ:
		if ((st = lookup_esn_st (&e)) < 0)
			 cmd_ctrl.oprc = RC_EVAL;
		else {
			cmd_ctrl.oprc = RC_OKRET;
			cmd_line[5] |= (st << 4);
		}
		return;

	  case SENS_NEXT_ANY:
		if ((st = get_next (&e, 2)) < 0)
			cmd_ctrl.oprc = RC_EVAL;
		else {
			cmd_ctrl.oprc = RC_OKRET;
			cmd_line[5] |= (st << 4);
			memcpy (cmd_line +1, &e, 4);
		}
		return;

	  case SENS_NEXT_0:
		if ((st = get_next (&e, 0)) < 0)
			cmd_ctrl.oprc = RC_EVAL; 
		else {
			cmd_ctrl.oprc = RC_OKRET;
			cmd_line[5] |= (st << 4);
			memcpy (cmd_line +1, &e, 4);
		}
		return;

	  case SENS_NEXT_1:
		if ((st = get_next (&e, 1)) < 0)
			cmd_ctrl.oprc = RC_EVAL;
		else {
			cmd_ctrl.oprc = RC_OKRET;
			cmd_line[5] |= (st << 4);
			memcpy (cmd_line +1, &e, 4);
		}
		return;

	  case SENS_WRITE:
		cmd_ctrl.oprc = add_esn (&e, &st);
		return;

	  case SENS_ERASE:
		cmd_ctrl.oprc = era_esn (&e) < 0 ? RC_EVAL : RC_OK;
		 return;

	  default:
		cmd_ctrl.oprc = RC_EPAR;
	}
}
#undef SENS_FIND
#undef SENS_READ
#undef SENS_NEXT_ANY
#undef SENS_NEXT_0
#undef SENS_NEXT_1
#undef SENS_WRITE
#undef SENS_ERASE

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

