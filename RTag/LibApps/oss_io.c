/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "msg_rtagStructs.h"
#include "lib_app_if.h"
#include "net.h"
#include "ser.h"
#include "tarp.h"
#include "codes.h"
#include "pinopts.h"
#if DM2100
#include "phys_dm2100.h"
#endif

extern tarpCtrlType tarp_ctrl;

// FIXIT: what was that needed for?
#define UART_OUTPUT 1

#if UART_DRIVER && UART_OUTPUT

#define OO_RETRY 00
process (oss_out, char)
	entry (OO_RETRY)
		ser_out (OO_RETRY, data);
		ufree (data);
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
#if 0
		if (val == 0) { // this would be mortal, so validate it
			cmd_ctrl.oprc = RC_EVAL;
			break;
		}
#endif
		// radical, but this is the simplest unbind
		if (val == 0)
			reset();

		local_host = val;
		cmd_ctrl.oprc = RC_OK;
		break;
#if 0
		// illegal if const
	case PAR_HID:
		if (cmd_ctrl.oplen != 1 + sizeof(lword)) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		memcpy (&host_id, cmd_line +2, sizeof(lword));
		cmd_ctrl.oprc = RC_OK;
		break;

		// CMD_MASTER should be the only way
	case PAR_MID:
		if (cmd_ctrl.oplen != 1 + sizeof(nid_t)) {
			cmd_ctrl.oprc = RC_ELEN; 
			break;
		}
		memcpy (&master_host, cmd_line +2, sizeof(nid_t));
		cmd_ctrl.oprc = RC_OK;
		break;
#endif
	case PAR_NID:
		if (cmd_ctrl.oplen != 1 + sizeof(nid_t)) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		memcpy (&val, cmd_line +2, sizeof(nid_t));
		net_opt (PHYSOPT_SETSID, &val);
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_TARP_L:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		tarp_ctrl.param = (tarp_ctrl.param & 0x3F) | (cmd_line[2] << 6);
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_TARP_S:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		tarp_ctrl.param =
			(tarp_ctrl.param & 0xF1) | ((cmd_line[2] & 7) << 1);
		cmd_ctrl.oprc = RC_OK;
		break;
		
	case PAR_TARP_R:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		tarp_ctrl.param =
			(tarp_ctrl.param & 0xCF) | ((cmd_line[2] & 3) << 4);
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_TARP_F:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		tarp_ctrl.param = (tarp_ctrl.param & 0xFE) | (cmd_line[2] & 1);

	case PAR_BEAC:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		beac_freq = cmd_line[2];
		if (beac_freq > 63)
			beac_freq = 63;
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_PIN:
		if (cmd_ctrl.oplen != 3) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		if (cmd_line[2] < 1 || cmd_line[2] > 11) {
			cmd_ctrl.oprc = RC_EVAL;
			break;
		}
		if (pin_write (cmd_line[2], cmd_line[3]) < 0)
			cmd_ctrl.oprc = RC_ERES;
		else
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

	default:
		cmd_ctrl.oprc = RC_EPAR;
  }
}

void oss_info_in (word state) {
	long l;
	char * buf;
	char info;
	word p[3];
	word len;

	if (cmd_ctrl.oplen != 1 && cmd_ctrl.oplen != 3) {
		cmd_ctrl.oprc = RC_ELEN;
		return;
	}
	// this is special case, for ADC params
	if ((info = cmd_line[1]) == INFO_ADC) {
		if (cmd_ctrl.oplen != 3) {
			cmd_ctrl.oprc = RC_ELEN;
			return;
		}
		p[1] = cmd_line[2] & 0x80 ? 1 : 0;
		p[0] = cmd_line[2] & 0x7F;
		if (p[0] < 1 || p[0] > 7 || (p[2] = cmd_line[3]) > 64) {
			cmd_ctrl.oprc = RC_EVAL;
			return;
		}
	}
	switch (info) {
	  case INFO_SYS:
		// host_id, local_host, net_id, seconds(),  master_host,
		// master_delta, beac_freq, cyc_ctrl, cyc_ap, cyc_sp
		len = 1 + 3*sizeof(long) + 3*sizeof(nid_t) +5;
		buf = get_mem (state, len);
		ufree (cmd_line);
		cmd_ctrl.oplen = len -1;
		cmd_line = buf++;
		memcpy (buf, &host_id, sizeof(long));
		buf += sizeof(long);
		memcpy (buf, &local_host, sizeof(nid_t));
		buf += sizeof(nid_t);
		len = net_opt (PHYSOPT_GETSID, NULL); // crapy word reuse
		memcpy (buf, &len, sizeof(nid_t));
		buf += sizeof(nid_t);
		l = seconds();
		memcpy (buf, &l, sizeof(long));
		buf += sizeof(long);
		memcpy (buf, &master_host, sizeof(nid_t));
		buf += sizeof(nid_t);
		memcpy (buf, &master_delta, sizeof(long));
		buf += sizeof(long);
		*buf++ = beac_freq;
		*buf++ = cyc_ctrl;
		*buf++ = cyc_ap;
		memcpy (buf, &cyc_sp, sizeof(word));
		break;

	  case INFO_MSG:
		len = 1 + 1 + 1 + 6*sizeof(word);
		buf = get_mem (state, len);
		ufree (cmd_line);
		cmd_ctrl.oplen = len -1;
		cmd_line = buf++;
		*buf++ = net_opt (PHYSOPT_STATUS, NULL);
		*buf++ = tarp_ctrl.param;
		memcpy (buf, &app_count.rcv, sizeof(word));
		buf += sizeof(word);
		memcpy (buf, &tarp_ctrl.rcv, sizeof(word));
		buf += sizeof(word);
		memcpy (buf, &app_count.snd, sizeof(word));
		buf += sizeof(word);
		memcpy (buf, &tarp_ctrl.snd, sizeof(word));
		buf += sizeof(word);
		memcpy (buf, &app_count.fwd, sizeof(word));
		buf += sizeof(word);
		memcpy (buf, &tarp_ctrl.fwd, sizeof(word));
		break;

	  case INFO_MEM:
		len = 1 + 6;
		buf = get_mem (state, len);
		ufree (cmd_line);
		cmd_ctrl.oplen = len -1;
		cmd_line = buf;
#if MALLOC_STATS
		p[0] = memfree(0, &p[1]);
		buf[1] = p[0];
		buf[2] = p[1];
		p[0] = maxfree(0, &p[1]);
		buf[3] = p[0];
		buf[4] = p[1];
		buf[5] = stackfree();
		buf[6] = staticsize();
#else
		memset (cmd_line, 0, 7);
#endif
		break;

	  case INFO_PIN:
		len = 1 + 2;
		buf = get_mem (state, len);
		ufree (cmd_line);
		cmd_ctrl.oplen = len -1;
		cmd_line = buf;
		p[0] = 0;
		// likely, it'll change...
		for (len = 1; len < 11 /*2*/; len++)
			if (pin_read (len))
				p[0] |= (1 << len);
		memcpy (cmd_line +1, p, 2);
		break;

	  case INFO_ADC:
		// this may not be enough... we'll see
		net_opt (PHYSOPT_RXOFF, NULL);
		cmd_line[1] = pin_read_adc (NONE, p [0], p [1], p[2]);
		net_opt (PHYSOPT_RXON, NULL);
		break;

	  case INFO_UART:
		len = 1 + 4;
		buf = get_mem (state, len);
		cmd_ctrl.oplen = len -1;
		ufree (cmd_line);
		cmd_line = buf;
		buf[1] = UART_RATE/100;
		buf[2] = UART_BITS;
		buf[3] = UART_PARITY;
		buf[4] = 0; // flow if we have it
		break;

	  default:
		cmd_ctrl.oprc = RC_ECMD;
		return;
	}
	cmd_line[0] = '\0';
	cmd_ctrl.oprc = RC_OK;
}

void oss_ret_out (word state) {
#if UART_DRIVER && UART_OUTPUT
	char * buf = get_mem (state, RET_HEAD_LEN + cmd_ctrl.oplen +3);
	word oset = 0;

	buf[oset++] = '\0';
	buf[oset++] = RET_HEAD_LEN + cmd_ctrl.oplen;
	buf[oset++] = cmd_ctrl.s_q;
	memcpy (buf + oset, &cmd_ctrl.s, sizeof(nid_t));
	oset += sizeof(nid_t);
	buf[oset++] = cmd_ctrl.p_q;
	memcpy (buf + oset, &cmd_ctrl.p, sizeof(nid_t));
	oset += sizeof(nid_t);
	buf[oset++] = cmd_ctrl.t_q;
	memcpy (buf + oset, &cmd_ctrl.t, sizeof(nid_t)); 
	oset += sizeof(nid_t);
	buf[oset++] = cmd_ctrl.opcode;
	buf[oset++] = cmd_ctrl.opref;
	buf[oset++] = cmd_ctrl.oprc;
	memcpy (buf + oset, cmd_line +1, cmd_ctrl.oplen);
	oset += cmd_ctrl.oplen;
	buf[oset] = 0x04;
        if (fork (oss_out, buf) == 0 ) {
		diag ("oss_out failed");
		ufree (buf);
	}
#endif
}

void oss_master_in (word state) {

	char * out_buf = NULL;
	word w;
	bool upd = NO;

	cmd_ctrl.oprc = RC_OK; // ok by default
	// local_host becomes master_host...
	// cmd_ctrl.t == 0 is not good if we want to stack masters,
	// but if we do, or if we want multiple masters within the same
	// net_id, we have to restrict master assignments.
	if (cmd_ctrl.t == 0 || local_host == cmd_ctrl.t) {
		if (master_host != local_host) {
			master_host = local_host;
			master_delta = 0;
			upd = YES;
		}
		if ((cyc_ctrl & 0x03) != cmd_line[1]) {
			if (cmd_line[1] & 0x01)
				cyc_ctrl |= 0x01;
			if (cmd_line[1] & 0x02)
				cyc_ctrl |= 0x02;
			upd = YES;
		}
		if (cyc_ap != cmd_line[2]) {
			cyc_ap = cmd_line[2];
			upd = YES;
		}
		memcpy (&w, &cmd_line[3], 2);
		if (cyc_sp != w) {
			cyc_sp = w;
			upd = YES;
		}
		if (upd) {
			// it's not clear how it should be done, maybe
			// on some triggers... some synchronization
			// between uart and duty cycle is needed, too.

			// bring it up if sleeping
			if (cyc_ctrl & CYC_SLEEPING) {
				cyc_ctrl &= ~CYC_SLEEPING;
				clockup();
				powerup();
				net_opt (PHYSOPT_RXON, NULL);
				net_opt (PHYSOPT_TXON, NULL);
			}
			 killall (cyc_man);
		}
		if (cyc_ap || cyc_sp)
			fork (cyc_man, NULL);

		if (local_host == cmd_ctrl.t)
			return;
	}

	// msg_master_out() is obsoleted, as the cycle data can't be
	// easily reconcile with a potentially valuable feature of
	// master stacking
	// msg_master_out (state, &out_buf, cmd_ctrl.t);
	out_buf = get_mem (state, sizeof(msgMasterType));
	in_header(out_buf, msg_type) = msg_master;
	in_header(out_buf, rcv) = cmd_ctrl.t;
	in_header(out_buf, hco) = 0;
	in_master(out_buf, mtime) = wtonl(seconds());
	in_master(out_buf, cyc_ctrl) = cmd_line[1];
	in_master(out_buf, cyc_ap) = cmd_line[2];
	memcpy (&in_master(out_buf, cyc_sp), &cmd_line[3], 2);
	
	send_msg (out_buf, sizeof(msgMasterType));
	if (beac_freq == 0 || running(beacon)) {
		ufree (out_buf);
	} else {
		if (fork (beacon, out_buf) == 0) {
			diag ("Fork info failed");
			ufree (out_buf);
			cmd_ctrl.oprc = RC_EFORK;
		} // beacon frees out_buf at exit
	}
}


// 0x00 <1-len> 0xFF msg_traceAck  <1-fcount> <1-bcount> <node> ... 0x04
void oss_traceAck_out (word state, char * buf) {
#if UART_DRIVER && UART_OUTPUT
	int num = in_header(buf, hoc) + in_traceAck(buf, fcount) -1;
	char * lbuf = get_mem (state, num * sizeof (nid_t) +7);
	lbuf[0] = '\0';
	lbuf[1] = num * sizeof (nid_t) +4;
	lbuf[2] = 0xFF;
	lbuf[3] = msg_traceAck;
	lbuf[4] = in_traceAck(buf, fcount);
	lbuf[5] = in_header(buf, hoc);
	
	memcpy (lbuf +6, buf + sizeof(msgTraceAckType), num * sizeof(nid_t));
	lbuf[num * sizeof (nid_t) +6] = 0x04;

	if (fork (oss_out, lbuf) == 0 ) {
		diag ("oss_out failed");
		ufree (lbuf);
	}
#endif
}

// 0x00 <1-len(10)> 0xFF msg_new  <2-snd> <4-hid> <2-nid> <2-mid> 0x04
void oss_new_out (word state, char * buf) {
#if UART_DRIVER && UART_OUTPUT
	char * lbuf = get_mem (state, 15);
	lbuf[0] = '\0'; 
	lbuf[1] = 12;
	lbuf[2] = 0xFF; 
	lbuf[3] = msg_new;
	memcpy (&lbuf[4], &in_header(buf, snd), 2);
	memcpy (&lbuf[6], &in_new(buf, hid), 4);
	memcpy (&lbuf[10], &in_new(buf, nid), 2);
	memcpy (&lbuf[12], &in_new(buf, mid), 2);
	lbuf[14] = 0x04;
	if (fork (oss_out, lbuf) == 0 ) {
		diag ("oss_out failed");
		ufree (lbuf); 
	}
#endif
}

void oss_trace_in (word state) {

	char * out_buf = NULL;

	cmd_ctrl.oprc = RC_OK;
	msg_trace_out (state, &out_buf, cmd_ctrl.t);
	send_msg (out_buf, sizeof(msgTraceType));
	if (beac_freq == 0 || running(beacon)) {
		ufree (out_buf);
	} else {
		if (fork (beacon, out_buf) == 0) {
			diag ("Fork info failed");
			ufree (out_buf);
			cmd_ctrl.oprc = RC_EFORK;
		} // beacon frees out_buf at exit
	}
}

void oss_bind_in (word state) {
	char * out_buf = NULL;
	
	if (cmd_ctrl.t == local_host) {
		cmd_ctrl.oprc = RC_EADDR;
		return;
	}
	cmd_ctrl.oprc = RC_OK;
	msg_bind_out (state, &out_buf);
	send_msg (out_buf, sizeof(msgBindType));
	ufree (out_buf);
}
