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
#if DM2100
#include "phys_dm2100.h"
#endif

extern tarpParamType tarp_param;
extern tarpCountType tarp_count;

#if UART_DRIVER && UART_OUTPUT

#define OO_RETRY 00
process (oss_out, char*)
	entry (OO_RETRY)
		ser_out (OO_RETRY, data);
		ufree (data);
		finish;
endprocess
#undef  OO_RETRY

#endif
#if 0
void oss_info_in (word state, id_t rcv) {

	char * out_buf = NULL;
	msg_info_out (state, &out_buf, rcv);

	if (local_host == rcv) {
		in_header(out_buf, snd) = local_host;
		in_info(out_buf, ltime) = seconds();
		in_info(out_buf, host_id) = host_id;
		in_info(out_buf, m_host) = master_host;
		in_info(out_buf, m_delta) = master_delta;
		oss_info_out (out_buf, oss_fmt);
		ufree (out_buf);
		return;
	}
	send_msg (out_buf, sizeof(msgInfoType));
	if (beac_freq == 0 || running(beacon)) {
		ufree (out_buf);
	} else {
		if (fork (beacon, out_buf) == 0) {
			diag ("Fork info failed");
			ufree (out_buf);
		} // beacon frees out_buf at exit
	}
}
#endif

void oss_set_in () {
  switch (cmd_line[1]) {
	  word val;
	  
	case PAR_LH:
		if (cmd_ctrl.oplen != 1 + sizeof(id_t)) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		memcpy (&val, cmd_line +2, sizeof(id_t));
		if (val == 0) { // this would be mortal, so validate it
			cmd_ctrl.oprc = RC_EVAL;
			break;
		}
		local_host = val;
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_HID:
		if (cmd_ctrl.oplen != 1 + sizeof(lword)) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		memcpy (&host_id, cmd_line +2, sizeof(lword));
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_MID:
		if (cmd_ctrl.oplen != 1 + sizeof(id_t)) {
			cmd_ctrl.oprc = RC_ELEN; 
			break;
		}
		memcpy (&master_host, cmd_line +2, sizeof(id_t));
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_NID:
		if (cmd_ctrl.oplen != 1 + sizeof(id_t)) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		memcpy (&net_id, cmd_line +2, sizeof(id_t));
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_TARP_L:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		tarp_param.level = cmd_line[2];
		cmd_ctrl.oprc = RC_OK;
		break;

	case PAR_TARP_S:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		tarp_param.slack = cmd_line[2];
		cmd_ctrl.oprc = RC_OK;
		break;
		
	case PAR_TARP_N:
		if (cmd_ctrl.oplen != 2) {
			cmd_ctrl.oprc = RC_ELEN;
			break;
		}
		tarp_param.nhood = cmd_line[2];
		cmd_ctrl.oprc = RC_OK;
		break;

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
		pin_set (cmd_line[2], cmd_line[3]);
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
	lint l;
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
		// master_delta, beac_freq
		len = 1 + 3*sizeof(lint) + 3*sizeof(id_t) +1;
		buf = get_mem (state, len);
		ufree (cmd_line);
		cmd_ctrl.oplen = len -1;
		cmd_line = buf++;
		memcpy (buf, &host_id, sizeof(lint));
		buf += sizeof(lint);
		memcpy (buf, &local_host, sizeof(id_t));
		buf += sizeof(id_t);
		memcpy (buf, &net_id, sizeof(id_t));
		buf += sizeof(id_t);
		l = seconds();
		memcpy (buf, &l, sizeof(lint));
		buf += sizeof(lint);
		memcpy (buf, &master_host, sizeof(id_t));
		buf += sizeof(id_t);
		memcpy (buf, &master_delta, sizeof(lint));
		buf += sizeof(lint);
		*buf = beac_freq;
		break;

	  case INFO_MSG:
		len = 1 + 1 + 3 + 6*sizeof(word);
		buf = get_mem (state, len);
		ufree (cmd_line);
		cmd_ctrl.oplen = len -1;
		cmd_line = buf++;
		*buf++ = net_opt (PHYSOPT_STATUS, NULL);
		*buf++ = tarp_param.level;
		*buf++ = tarp_param.slack;
		*buf++ = tarp_param.nhood;
		memcpy (buf, &app_count.rcv, sizeof(word));
		buf += sizeof(word);
		memcpy (buf, &tarp_count.rcv, sizeof(word));
		buf += sizeof(word);
		memcpy (buf, &app_count.snd, sizeof(word));
		buf += sizeof(word);
		memcpy (buf, &tarp_count.snd, sizeof(word));
		buf += sizeof(word);
		memcpy (buf, &app_count.fwd, sizeof(word));
		buf += sizeof(word);
		memcpy (buf, &tarp_count.fwd, sizeof(word));
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
		for (len = 1; len < 12; len++)
			if (pin_get (len))
				p[0] |= (1 << len);
		memcpy (cmd_line +1, p, 2);
		break;

	  case INFO_ADC:
		// this may not be enough... we'll see
		net_opt (PHYSOPT_RXOFF, NULL);
		cmd_line[1] = pin_get_adc (p [0], p [1], p[2]);
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
	memcpy (buf + oset, &cmd_ctrl.s, sizeof(id_t));
	oset += sizeof(id_t);
	buf[oset++] = cmd_ctrl.p_q;
	memcpy (buf + oset, &cmd_ctrl.p, sizeof(id_t));
	oset += sizeof(id_t);
	buf[oset++] = cmd_ctrl.t_q;
	memcpy (buf + oset, &cmd_ctrl.t, sizeof(id_t)); 
	oset += sizeof(id_t);
	buf[oset++] = cmd_ctrl.opcode;
	buf[oset++] = cmd_ctrl.opref;
	buf[oset++] = cmd_ctrl.oprc;
	memcpy (buf + oset, cmd_line +1, cmd_ctrl.oplen);
	oset += cmd_ctrl.oplen;
	buf[oset] = 0x04;
        if (fork (oss_out, buf) == 0 )
		diag ("oss_out failed");
#endif
}
#if 0
void oss_info_out (char * buf, word fmt) {
#if UART_DRIVER
	char * lbuf;

	switch (fmt) {
		case OSS_HT:
			lbuf = form (NULL, "Info from (%lx:%u) at %lu:\r\n"
				"m %u td %ld, pl:rssi %u:%u\r\n",
				in_info(buf, host_id),
				in_header(buf, snd),
				in_info(buf, ltime),
				in_info(buf, m_host),
				in_info(buf, m_delta),
				in_info(buf, pl),
				in_header(buf, msg_type)); // hacked in rssi
			break;

		case OSS_TCL:
			// not yet
		default:
			diag ("**Unsupported fmt %u", fmt);
			return;

	}
	if (fork (oss_out, lbuf) == 0 ) {
		diag ("oss_out failed");
		ufree (lbuf);
	}
#endif
}
#endif
void oss_master_in (word state, id_t rcv) {

	char * out_buf = NULL;

	// shortcut to set master_host
	if (local_host == rcv) {
		master_host = rcv;
		master_delta = 0;
		return;
	}

	msg_master_out (state, &out_buf, rcv);
	send_msg (out_buf, sizeof(msgMasterType));
	if (beac_freq == 0 || running(beacon)) {
		ufree (out_buf);
	} else {
		if (fork (beacon, out_buf) == 0) {
			diag ("Fork info failed");
			ufree (out_buf);
		} // beacon frees out_buf at exit
	}
}


// 0x00 <1-len> 0xFF 0x04 <1-fcount> <1-bcount> <node> ... 0x04
void oss_traceAck_out (word state, char * buf) {
#if UART_DRIVER && UART_OUTPUT
	int num = in_header(buf, hoc) + in_traceAck(buf, fcount) -1;
	char * lbuf = get_mem (state, num * sizeof (id_t) +7);
	lbuf[0] = '\0';
	lbuf[1] = num * sizeof (id_t) +4;
	lbuf[2] = 0xFF;
	lbuf[3] = msg_traceAck;
	lbuf[4] = in_traceAck(buf, fcount);
	lbuf[5] = in_header(buf, hoc);
	
	memcpy (lbuf +6, buf + sizeof(msgTraceAckType), num);
	lbuf[num * sizeof (id_t) +6] = 0x04;

	if (fork (oss_out, lbuf) == 0 ) {
		diag ("oss_out failed");
		ufree (lbuf);
	}
#endif
}

void oss_trace_in (word state, id_t dst) {

	char * out_buf = NULL;

	msg_trace_out (state, &out_buf, dst);
	send_msg (out_buf, sizeof(msgTraceType));
	if (beac_freq == 0 || running(beacon)) {
		ufree (out_buf);
	} else {
		if (fork (beacon, out_buf) == 0) {
			diag ("Fork info failed");
			ufree (out_buf);
		} // beacon frees out_buf at exit
	}
}
