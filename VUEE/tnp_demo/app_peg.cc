/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "globals_peg.h"
#include "threadhdrs_peg.h"

// elsewhere may be a better place for this:
#if CC1000
#define INFO_PHYS_DEV INFO_PHYS_CC1000
#endif

#if CC1100
#define INFO_PHYS_DEV INFO_PHYS_CC1100
#endif

#if DM2200
#define INFO_PHYS_DEV INFO_PHYS_DM2200
#endif

#ifndef INFO_PHYS_DEV
#error "UNDEFINED RADIO"
#endif

#define UI_BUFLEN		UART_INPUT_BUFFER_LENGTH
#define DEF_NID			85
#define DEF_MHOST		10

// Semaphores
#define CMD_READER	(&cmd_line)
#define CMD_WRITER	((&cmd_line)+1)

// =============
// OSS reporting
// =============
strand (oss_out, char)
        entry (OO_RETRY)
		if (data == NULL)
			app_diag (D_SERIOUS, "NULL in oss_out");
		else
			ser_outb (OO_RETRY, data);
                finish;
endstrand

thread (mbeacon)

    entry (MB_START)
	delay (25 * 1024 + rnd() % 10240, MB_SEND); // 30 +/- 5s
	release;

    entry (MB_SEND)
	oss_master_in (MB_SEND, 0);
	proceed (MB_START);

endthread

// Display node stats on UI
__PUBLF (NodePeg, void, stats) () {

	word faults0, faults1;
	word mem0 = memfree(0, &faults0);
	word mem1 = memfree(1, &faults1);
#if TINY_MEM
	char * mbuf;
	mbuf = form (NULL, stats_str1, host_id, local_host);
	if (runstrand (oss_out, mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out fork");
		ufree (mbuf);
		return;
	}

	mbuf = form (NULL, stats_str2, app_count.rcv, tarp_ctrl.rcv,
			app_count.snd, tarp_ctrl.snd,
			app_count.fwd, tarp_ctrl.fwd);
	if (runstrand (oss_out, mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out fork");
		ufree (mbuf);
		return;
	}

	mbuf = form (NULL, stats_str3, tag_auditFreq, tag_eventGran);
	if (runstrand (oss_out, mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out fork");
		ufree (mbuf);
		return;
	}

	mbuf = form (NULL, stats_str4, host_pl, seconds(),
			master_delta, master_host);
	if (runstrand (oss_out, mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out fork");
		ufree (mbuf);
		return;
	}
	
	mbuf = form (NULL, stats_str5, net_opt (PHYSOPT_PHYSINFO, NULL),
			net_opt (PHYSOPT_PLUGINFO, NULL),
			net_opt (PHYSOPT_STATUS, NULL));
	if (runstrand (oss_out, mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out fork");
		ufree (mbuf);
		return;
	}
	
	mbuf = form (NULL, stats_str6, mem0, mem1, faults0, faults1);
	if (runstrand (oss_out, mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out fork");
		ufree (mbuf);
		return;
	}
#else
	net_diag (D_UI, stats_str,
			host_id, local_host,
			app_count.rcv, tarp_ctrl.rcv,
			app_count.snd, tarp_ctrl.snd,
			app_count.fwd, tarp_ctrl.fwd,
			tag_auditFreq, tag_eventGran,
			host_pl, seconds(), master_delta, master_host,
			net_opt (PHYSOPT_PHYSINFO, NULL),
			net_opt (PHYSOPT_PLUGINFO, NULL),
			net_opt (PHYSOPT_STATUS, NULL),
			mem0, mem1, faults0, faults1);
#endif

}

__PUBLF (NodePeg, void, process_incoming) (word state, char * buf, word size,
								word rssi) {
  int    w_len;

  if (check_msg_size (buf, size, D_SERIOUS) != 0)
	  return;

  switch (in_header(buf, msg_type)) {

	case msg_pong:
		if (in_pong_rxon(buf)) 
			check_msg4tag (in_header(buf, snd));

		msg_pong_in (state, buf, rssi);
		return;

	case msg_report:
		msg_report_in (state, buf);
		return;

	case msg_reportAck:
		msg_reportAck_in (buf);
		return;

	case msg_master:
		msg_master_in (buf);
		return;

	case msg_findTag:
		msg_findTag_in (state, buf);
		return;

	case msg_fwd:
		msg_fwd_in (state, buf, size);
		return;

	case msg_getTagAck:
		msg_getTagAck_in (state, buf, size);
		return;

	case msg_setTagAck:
		msg_setTagAck_in (state, buf, size);
		return;

//    case msg_setPeg:
//		msg_setPeg_in (state, buf, size);
//		return;

	case msg_rpc:
		if (cmd_line != NULL) { // busy with another input
			when (CMD_WRITER, state);
			release;
		}
		w_len = size - sizeof(msgRpcType);
		cmd_line = get_mem (state, w_len);
		memcpy (cmd_line, buf + sizeof(msgRpcType), w_len);
		trigger (CMD_READER);
		return;

	// more types, not implemented yet
	default:
		app_diag (D_SERIOUS, "Got ? (%u)", in_header(buf, msg_type));

  }
}

// [0, FF] -> [1, F]
// it can't be 0, as find_tags() will mask the rssi out!
static word map_rssi (word r) {
#ifdef __SMURPH__
/* temporary rough estimates
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
}

/*
   --------------------
   Receiver process
   RS_ <-> Receiver State
   --------------------
*/

// In this model, a single rcv is forked once, and runs / sleeps all the time
thread (rcv)

	entry (RC_INIT)

		rcv_packet_size = 0;
		rcv_buf_ptr = NULL;
		rcv_rssi = 0;

	entry (RC_TRY)

		if (rcv_buf_ptr != NULL) {
			ufree (rcv_buf_ptr);
			rcv_buf_ptr = NULL;
			rcv_packet_size = 0;
		}
    		rcv_packet_size = net_rx (RC_TRY, &rcv_buf_ptr, &rcv_rssi, 0);
		if (rcv_packet_size <= 0) {
			app_diag (D_SERIOUS, "net_rx failed (%d)",
				rcv_packet_size);
			proceed (RC_TRY);
		}

		app_diag (D_DEBUG, "RCV (%d): %x-%u-%u-%u-%u-%u\r\n",			  
		rcv_packet_size, in_header(rcv_buf_ptr, msg_type),
			  in_header(rcv_buf_ptr, seq_no) & 0xffff,
			  in_header(rcv_buf_ptr, snd),
			  in_header(rcv_buf_ptr, rcv),
			  in_header(rcv_buf_ptr, hoc) & 0xffff,
			  in_header(rcv_buf_ptr, hco) & 0xffff);

		// that's how we could check which plugin is on
		// if (net_opt (PHYSOPT_PLUGINFO, NULL) != INFO_PLUG_TARP)

	entry (RC_MSG)
#if 0
	dupa: will be needed for all sorts of calibrations
//#endif
		if (in_header(rcv_buf_ptr, msg_type) == msg_pong)
			app_diag (D_UI, "rss (%d.%d): %d",
				in_header(rcv_buf_ptr, snd),
				in_pong(rcv_buf_ptr, level), rcv_rssi >> 8);
		else
			app_diag (D_UI, "rss %d from %d", rcv_rssi >> 8,
					in_header(rcv_buf_ptr, snd));
#endif
		process_incoming (RC_MSG, rcv_buf_ptr, rcv_packet_size,
			map_rssi(rcv_rssi));
		app_count.rcv++;
		proceed (RC_TRY);

endthread

/*
  --------------------
  audit process
  AS_ <-> Audit State
  --------------------
*/

thread (audit)

	nodata;

	entry (AS_INIT)

		aud_buf_ptr = NULL;

	entry (AS_START)

		if (aud_buf_ptr != NULL) {
			ufree (aud_buf_ptr);
			aud_buf_ptr = NULL;
		}
		if (tag_auditFreq == 0) {
			app_diag (D_WARNING, "Audit stops");
			finish;
		}
		aud_ind = tag_lim;
		app_diag (D_DEBUG, "Audit starts");

	entry (AS_TAGLOOP)

		if (aud_ind-- == 0) {
			app_diag (D_DEBUG, "Audit ends");
			delay (tag_auditFreq, AS_START);
			release;
		}

	entry (AS_TAGLOOP1)

		check_tag (AS_TAGLOOP1, aud_ind, &aud_buf_ptr);

		if (aud_buf_ptr) {
			if (local_host == master_host) {
				in_header(aud_buf_ptr, snd) = local_host;
				oss_report_out (aud_buf_ptr, oss_fmt);
			} else
				send_msg (aud_buf_ptr, sizeof(msgReportType));
			ufree (aud_buf_ptr);
			aud_buf_ptr = NULL;
		}
		proceed (AS_TAGLOOP);
endthread

/*
   --------------------
   cmd_in process
   CS_ <-> Command State
   --------------------
*/
thread (cmd_in)

	nodata;

	entry (CS_INIT)

		if (ui_ibuf == NULL)
			ui_ibuf = get_mem (CS_INIT, UI_BUFLEN);

	entry (CS_IN)

		// hangs on the uart_a interrupt or polling
		ser_in (CS_IN, ui_ibuf, UI_BUFLEN);
		if (strlen(ui_ibuf) == 0)
			// CR on empty line would do it
			proceed (CS_IN);

	entry (CS_WAIT)

		if (cmd_line != NULL) {
			when (CMD_WRITER, CS_WAIT);
			release;
		}

		cmd_line = get_mem (CS_WAIT, strlen(ui_ibuf) +1);
		strcpy (cmd_line, ui_ibuf);
		trigger (CMD_READER);
		proceed (CS_IN);

endthread

static char * stateName (unsigned state) {
	switch ((tagStateType)state) {
		case noTag:
			return "noTag";
		case newTag:
			return "newTag";
		case reportedTag:
			return "reportedTag";
		case confirmedTag:
			return "confirmedTag";
		case fadingReportedTag:
			return "fadingReportedTag";
		case fadingConfirmedTag:
			return "fadingConfirmedTag";
		case goneTag:
			return "goneTag";
		case sumTag:
			return "sumTag";
		default:
			return "unknown?";
	}
}

static char * locatName (lword id) {
	if ((word)id == 0)
		return "total";
	if (((id >> 16) & 0xFF) == 0)
		return "no";
	switch (id >> 24) {
		case 3:
			return "proxy";
		case 2:
			return "near";
		case 1:
			return "far";
		case 0:
			return "no";
	}
	return "rssi?";
	// should be more... likely a number with  distance?
}

__PUBLF (NodePeg, void, oss_report_out) (char * buf, word fmt) {

	char * lbuf = NULL;

	switch (fmt) {
		case OSS_HT:
			lbuf = form (NULL, "T%u at P%u at %ld: %s(%u)"\
					" rss:pl.c (%u:%u.%u)\r\n",
				(word)in_report(buf, tagId), 
				in_header(buf, snd),
				in_report(buf, tStamp),
				stateName (in_report(buf, state)),
				in_report(buf, state),
				(word)(in_report(buf, tagId) >> 24),
				(word)(in_report(buf, tagId) >> 16) & 0x00ff,
				in_report(buf, count));
			break;

		case OSS_TCL:
			lbuf = form (NULL, "mt%u P%u T%u %u %u %lu %u %u\n",
				in_header(buf, msg_type),
				in_header(buf, snd),
				(word)in_report(buf, tagId),
				(word)(in_report(buf, tagId) >> 16) & 0x00ff,
				(word)(in_report(buf, tagId) >> 24),
				in_report(buf, tStamp),
				in_report(buf, state) == goneTag ? 0 : 1,
			        in_report(buf, count));
			break;

		case OSS_LOCDEMO:
			lbuf = form (NULL, "T%u(%x) at P%u at %ld:%s%s #%u\r\n",
				(word)in_report(buf, tagId),
				(word)(in_report(buf, tagId) >> 16),
				in_header(buf, snd),
				in_report(buf, tStamp),
				in_report(buf, state) == goneTag ?
					" gone " : " ",
				locatName (in_report(buf, tagId)),
				in_report(buf, count));
			break;

		default:
			app_diag (D_SERIOUS, "**Unsupported fmt %u", fmt);
			return;

	}
	if (runstrand (oss_out, lbuf) == 0 ) {
		app_diag (D_SERIOUS, "oss_out failed");
		ufree (lbuf);
	}
}

__PUBLF (NodePeg, void, oss_setTag_out) (char * buf, word fmt) {

	char * lbuf = NULL;

	switch (fmt) {
		case OSS_HT:
		case OSS_TCL:
		case OSS_LOCDEMO:
			lbuf = form (NULL,
				"T %u at P %u Set ack %u ack seq %u\r\n", 
				in_setTagAck(buf, tag),
				(local_host == master_host ?
				    in_header(buf, snd) : in_header(buf, rcv)),
				in_setTagAck(buf, ack),
				in_setTagAck(buf, ackSeq));
			break;

		default:
			app_diag (D_SERIOUS, "**Unsupported fmt %u", fmt);
			return;

	}
	if (runstrand (oss_out, lbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out failed");
		ufree (lbuf);
	}
}

__PUBLF (NodePeg, void, oss_getTag_out) (char * buf, word fmt) {
	char * lbuf = NULL;

	switch (fmt) {
		case OSS_HT:
		case OSS_TCL:
		case OSS_LOCDEMO:
			lbuf = form (NULL,
				"T %u at P %u Get time %lu host id %lx"
				" pow lev %x freq maj %u freq min %u span %u" 
				" ack seq %u\r\n", 
				in_getTagAck(buf, tag),
				(in_getTagAck(buf, tag) != in_header(buf, snd) ?
				    in_header(buf, snd) : in_header(buf, rcv)),
				in_getTagAck(buf, ltime),
				in_getTagAck(buf, host_ident),
				in_getTagAck(buf, pow_levels),
				in_getTagAck(buf, freq_maj),
				in_getTagAck(buf, freq_min),
				in_getTagAck(buf, rx_span),
				in_getTagAck(buf, ackSeq));
			break;

		default:
			app_diag (D_SERIOUS, "**Unsupported fmt %u", fmt);
			return;

	}
	if (runstrand (oss_out, lbuf) == 0 ) {
		app_diag (D_SERIOUS, "oss_out failed");
		ufree (lbuf);
	}
}

__PUBLF (NodePeg, void, oss_findTag_in) (word state, lword tag, nid_t peg) {

	char * out_buf = NULL;
	int tagIndex;

	if (peg == local_host || peg == 0) {
		if ((word)tag == 0) { // summary
			tagIndex = find_tags (tag, 1);
			msg_report_out (state, tagIndex | 0x8000, &out_buf, 1);
			if (out_buf == NULL)
				return;
		} else {
			tagIndex = find_tags (tag, 0);
			msg_report_out (state, tagIndex, &out_buf, 1);
			if (out_buf == NULL)
				return;

			// as in msg_findTag_in, kludge summary into
			// missing tag:
			if (tagIndex < 0) {
				in_report(out_buf, tagId) = tag;
				in_report(out_buf, state) = noTag;
			}
		}

		in_header(out_buf, snd) = local_host;
		// don't report bulk missing (but do summary)
		if ((word)tag == 0 || peg != 0 || tagIndex >= 0)
			oss_report_out (out_buf, oss_fmt);

	}

	if (peg != local_host) {
		msg_findTag_out (state, &out_buf, tag, peg);
		send_msg (out_buf, sizeof(msgFindTagType));
	}
	ufree (out_buf);
}

__PUBLF (NodePeg, void, oss_getTag_in) (word state, lword tag, nid_t peg,
								lword pass) {
	
	char * out_buf = NULL;
	char * get_buf = NULL;
	int size = sizeof(msgFwdType) + sizeof(msgGetTagType);
	
	if (peg == 0 || tag == 0) {
		app_diag (D_WARNING, "get: no zeroes");
		return;
	}
	// alloc and prepare msg fwd 
	msg_fwd_out (state, &out_buf, size, tag, peg, pass);
	in_header(out_buf, snd) = local_host;
	// get offset for payload - getTag msg
	get_buf = out_buf + sizeof(msgFwdType);
	in_header(get_buf, msg_type) = msg_getTag;
	in_header(get_buf, rcv) = (nid_t)tag;
	in_header(get_buf, snd) = local_host;
	in_getTag(get_buf, passwd) = pass;
	if (peg == local_host) {
		//	it is local - put it in the wroom	
		msg_fwd_in(state, out_buf, size);
	} else {	
		send_msg (out_buf,  size);
	}
	ufree (out_buf);
}

__PUBLF (NodePeg, void, oss_master_in) (word state, nid_t peg) {

	char * out_buf = NULL;

	// shortcut to set master_host
	if (local_host == peg || peg == 0) {
		if (!running (mbeacon))
				runthread (mbeacon);
		master_host = local_host;
		if (local_host == peg)
			return;
	}
	msg_master_out (state, &out_buf, peg);
	send_msg (out_buf, sizeof(msgMasterType));
	ufree (out_buf);
}

// FIXME: doesn't make any sense... why global, why params, why not cmd_line
__PUBLF (NodePeg, void, oss_setTag_in) (word state, lword tag,
	       	nid_t peg, lword pass, nid_t nid, word maj, word min, 
		word pl, word span, lword npass) {

	char * out_buf = NULL;
	char * set_buf = NULL;
	int size = sizeof(msgFwdType) + sizeof(msgSetTagType);

	if (peg == 0 || tag == 0) {
		app_diag (D_WARNING, "set: no zeroes");
		return;
	}

	// alloc and prepare msg fwd
	msg_fwd_out (state, &out_buf, size, tag, peg, pass);
	in_header(out_buf, snd) = local_host;
	// get offset for payload - getTag msg
	set_buf = out_buf + sizeof(msgFwdType);
	in_header(set_buf, msg_type) = msg_setTag;
	in_header(set_buf, rcv) = (nid_t)tag;
	in_header(set_buf, snd) = local_host;
	in_setTag(set_buf, passwd) = pass;
	in_setTag(set_buf, npasswd) = npass;
	in_setTag(set_buf, node_addr) = nid;
	in_setTag(set_buf, pow_levels) = pl;
	in_setTag(set_buf, freq_maj) = maj;
	in_setTag(set_buf, freq_min) = min;
	in_setTag(set_buf, rx_span) = span;
	if (peg == local_host || peg == 0)
		// put it in the wroom
		msg_fwd_in(state, out_buf, size);
	if (peg != local_host)
		send_msg (out_buf,  size);
	ufree (out_buf);
}

__PUBLF (NodePeg, void, oss_setPeg_in) (word state, nid_t peg, nid_t nid,
							word pl, char * str) {
	char * out_buf = NULL;
	char * set_buf = NULL;
	int size = sizeof(msgFwdType) + sizeof(msgSetPegType);

	// alloc and prepare msg fwd
	msg_fwd_out (state, &out_buf, size, 0, peg, 0);
	in_header(out_buf, snd) = local_host;
	// get offset for payload - getTag msg
	set_buf = out_buf + sizeof(msgFwdType);
	in_header(set_buf, msg_type) = msg_setPeg;
	in_header(set_buf, rcv) = 0;
	in_header(set_buf, snd) = local_host;
	in_setPeg(set_buf, level) = pl;
	in_setPeg(set_buf, new_id) = nid;
	memcpy(in_setPeg(set_buf, str), str, 16);
	if (peg != local_host) {
		send_msg (out_buf,  size);
	} else {
		msg_fwd_in(state, out_buf, size);
	}
	ufree (out_buf);
}

// ==========================================================================

/*
   --------------------
   Root process
   RS_ <-> Root State
   --------------------
*/

thread (root)

	nid_t 		rtin_lh, rtin_mh;
	word		rtin_hpl; 		// peg power level
	int		opt_sel;
	char 		str [PEG_STR_LEN];
	nodata;

	entry (RS_INIT)

		local_host = (word)host_id;
#ifndef __SMURPH__
		net_id = DEF_NID;
		master_host = DEF_MHOST;
#endif
		tarp_ctrl.param = 0xB1; // level 2, rec 3, slack 0, fwd on
		// these survive for repeated commands;
		rtin_tag = rtin_pass = rtin_npass = 0;
		rtin_peg = rtin_rssi = rtin_pl = 0;

		init_tags();
		ser_out (RS_INIT, welcome_str);
		ui_obuf = get_mem (RS_INIT, UI_BUFLEN);

		// spread in case of a sync reset
		delay (rnd() % tag_auditFreq, RS_PAUSE);
		release;

	entry (RS_PAUSE)

		if (net_init (INFO_PHYS_DEV, INFO_PLUG_TARP) < 0) {
			app_diag (D_FATAL, "net_init failed");
			reset();
		}
		net_opt (PHYSOPT_SETSID, &net_id);
		net_opt (PHYSOPT_SETPOWER, &host_pl);
		runthread (rcv);
		runthread (cmd_in);
		runthread (audit);
		if (master_host == local_host)
			runthread (mbeacon);
		proceed (RS_RCMD);

	entry (RS_FREE)

		ufree (cmd_line);
		cmd_line = NULL;
		trigger (CMD_WRITER);

	entry (RS_RCMD)

		if (cmd_line == NULL) {
			when (CMD_READER, RS_RCMD);
			release;
		}

	entry (RS_DOCMD)

//	     	app_trace("rs_docmd");
		if (cmd_line[0] == ' ') // ignore if starts with blank
			proceed (RS_FREE);

                if (cmd_line[0] == 'h') {
//                	app_trace("help");
			strcpy (ui_obuf, welcome_str);
//			app_trace("help_end");
			proceed (RS_UIOUT);
		}

		if (cmd_line[0] == 'q')
			reset();

		if (cmd_line[0] == 'm') {
			 scan (cmd_line+1, "%u", &rtin_peg);
			 oss_master_in (RS_DOCMD, rtin_peg);
			 proceed (RS_FREE);
		}

		if (cmd_line[0] == 'f') {
//			app_trace("fwd");
			rtin_tag = 0;
		    	rtin_peg = rtin_rssi = rtin_pl = 0;
			scan (cmd_line+1, "%lu %u %u %u",
				&rtin_tag, &rtin_peg, &rtin_rssi, &rtin_pl);
			rtin_tag |=  ((lword)rtin_rssi << 24) |
				     ((lword)rtin_pl   << 16);
			oss_findTag_in (RS_DOCMD, rtin_tag, rtin_peg);
//			app_trace("fwd-end");
			proceed (RS_FREE);
		}

		if (cmd_line[0] == 'g') {
//			app_trace("get");
			scan (cmd_line+1, "%lu %u %lu",
				&rtin_tag, &rtin_peg, &rtin_pass);
			oss_getTag_in (RS_DOCMD, rtin_tag, rtin_peg, rtin_pass);
//			app_trace("get_end");
			proceed (RS_FREE);
		}

        	if (cmd_line[0] == 't') {
			rtin_lh = rtin_pl = rtin_maj = rtin_min = rtin_span = 0;
			rtin_npass = 0;
			scan (cmd_line+1, "%lu %u %lu %u %u %u %x %u %lu",
				&rtin_tag, &rtin_peg, &rtin_pass, &rtin_lh,
				&rtin_maj, &rtin_min, &rtin_pl, &rtin_span,
				&rtin_npass);
			oss_setTag_in (RS_DOCMD, rtin_tag, rtin_peg, rtin_pass,
					rtin_lh, rtin_maj, rtin_min, rtin_pl,
					rtin_span, rtin_npass);
			proceed (RS_FREE);
		}
		
		if (cmd_line[0] == 'r') {
			rtin_lh = rtin_peg = rtin_hpl = 0;
			*str = '\0';
			scan (cmd_line+1, "%u %u %u %s",
				&rtin_peg, &rtin_lh, &rtin_hpl, str);
			oss_setPeg_in (RS_DOCMD, rtin_peg, rtin_lh, rtin_hpl,
				str);
			proceed (RS_FREE);
		}

		if (cmd_line[0] == 's') {
			rtin_lh = rtin_hpl = rtin_mh = 0;
			scan (cmd_line+1, "%u %u %u",
				&rtin_lh, &rtin_mh, &rtin_hpl);
			if (rtin_lh)
				local_host = rtin_lh;
			if (rtin_mh)
				master_host = rtin_mh;
			if (rtin_hpl) {
				net_opt (PHYSOPT_SETPOWER, &rtin_hpl);
				host_pl = rtin_hpl;
			}
			stats ();
			proceed (RS_FREE);
		}

		if (cmd_line[0] == 'o') {

			if (strlen (cmd_line) > 2) {
				opt_sel = 0;
				if (cmd_line[1] == 'r')
					opt_sel++;
				else if (cmd_line[1] == 't')
					opt_sel += 2;
				else if (cmd_line[1] == ' ')
					opt_sel +=3;
				else {
					form (ui_obuf, ill_str, cmd_line);
					proceed (RS_UIOUT);
				}

				if (cmd_line[2] == '+') {
					if (opt_sel & 1)
					net_opt (PHYSOPT_RXON, NULL);
					if (opt_sel & 2)
					net_opt (PHYSOPT_TXON, NULL);

				} else if (cmd_line[2] == '-') {
					if (opt_sel & 1)
						net_opt (PHYSOPT_RXOFF, NULL);
					if (opt_sel & 2)
						net_opt (PHYSOPT_TXOFF, NULL);
				}
			}
			form (ui_obuf, o_str, net_opt (PHYSOPT_PHYSINFO, NULL),
				net_opt (PHYSOPT_PLUGINFO, NULL),
			       	net_opt (PHYSOPT_STATUS, NULL));
			proceed (RS_UIOUT);
		}

		form (ui_obuf, ill_str, cmd_line);

	entry (RS_UIOUT)
		ser_out (RS_UIOUT, ui_obuf);
		proceed (RS_FREE);

endthread

praxis_starter (NodePeg);
