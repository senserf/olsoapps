/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "form.h"
#include "net.h"
#include "tarp.h"
#include "msg_pegs.h"
#include "ser.h"
#include "diag.h"
#include "app.h"
#include "lib_apps.h"
// #include "trc.h"

heapmem {80, 20}; // how to find or guess the right ratio?

// elsewhere may be a better place for this:
#if CHIPCON
#define INFO_PHYS_DEV INFO_PHYS_CHIPCON
#else
#define INFO_PHYS_DEV INFO_PHYS_RADIO
#endif

// UI is uart_a, including simulated uart_a
#define ui_out	ser_out
#define ui_outf ser_outf
#define ui_in	ser_in
#define ui_inf	ser_inf

// arbitrary
#define UI_BUFLEN		256
#define DEF_LOCAL_HOST		99

static char * ui_ibuf = NULL;
static char * ui_obuf = NULL;

// command line and its semaphores
static char * cmd_line	= NULL;
#define CMD_READER	((word)&cmd_line)
#define CMD_WRITER	((word)((&cmd_line)+1))

// in LibConf
extern lword host_id;
extern lword local_host;
extern lword host_passwd;

// in LibComms
extern int net_init (word, word);

// format strings
static const char welcome_str[] = "\r\nWelcome to Peg Testbed\r\n"
	"Commands:\r\n"
	"\tSet:\ts [ lh [ hid [ mid ] ] ]\r\n"
	"\tSnd Master:\tm [ peg ]\r\n"
	"\tSnd Find:\tf [ tag [ peg [ rss [ pLev ] ] ] ]\r\n"
	"\tOpt RF:\to[{r|t| }{+|-}]"
	"\tHelp:\th\r\n"
	"\tq(uit)\r\n";

static const char o_str[] =	"phys: %x, plug: %x, txrx: %x\r\n";

static const char ill_str[] =	"Illegal command (%s)\r\n";

static const char stats_str[] = "Stats for hostId - localHost (%lx - %lu):\r\n"
	" In (%u:%u), Out (%u:%u), Fwd (%u:%u),\r\n"
	" Freq audit (%u) events (%u),\r\n"
	" Time (%lu), Delta (%ld) to Master (%lu),\r\n"
	" phys: %x, plug: %x, txrx: %x\r\n"
	" Mem free (%u, %u) faults (%u, %u)\r\n";

// Display node stats on UI
static void stats () {
	word faults0, faults1;
	word mem0 = memfree(0, &faults0);
	word mem1 = memfree(1, &faults1);
	
	net_diag (D_UI, stats_str,
			host_id, local_host, 
			app_count.rcv, tarp_count.rcv,
			app_count.snd, tarp_count.snd, 
			app_count.fwd, tarp_count.fwd,
			tag_auditFreq, tag_eventGran,
			seconds(), master_delta, master_host,
			net_opt (PHYSOPT_PHYSINFO, NULL),
			net_opt (PHYSOPT_PLUGINFO, NULL),
			net_opt (PHYSOPT_STATUS, NULL),
			mem0, mem1, faults0, faults1);
}

static void process_incoming (word state, char * buf, word size, word rssi) {
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

	case msg_rpc:
		if (cmd_line != NULL) { // busy with another input
			wait (CMD_WRITER, state);
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

// [0, FF] -> [0, F], say:
static word map_rssi (word r) {
	return (r >> 4);
}

/*
   --------------------
   Receiver process
   RS_ <-> Receiver State
   --------------------
*/
#define RS_TRY	00
#define RS_MSG	10

// In this model, a single rcv is forked once, and runs / sleeps all the time
process (rcv, void)
	static int packet_size	= 0;
	static char * buf_ptr	= NULL;
	static word rssi	= 0;
	nodata;

	entry (RS_TRY)
		if (buf_ptr != NULL) {
			ufree (buf_ptr);
			buf_ptr = NULL;
			packet_size = 0;
		}
		packet_size = net_rx (RS_TRY, &buf_ptr, &rssi);
		if (packet_size <= 0) {
			app_diag (D_SERIOUS, "net_rx failed (%d)", packet_size);
			proceed (RS_TRY);
		}

		app_diag (D_DEBUG, "RCV (%d): %x-%u-%lu-%lu-%u-%u\r\n",
			  packet_size, in_header(buf_ptr, msg_type),
			  in_header(buf_ptr, seq_no),
			  in_header(buf_ptr, snd),
			  in_header(buf_ptr, rcv),
			  in_header(buf_ptr, hoc),
			  in_header(buf_ptr, hco));

		// that's how we could check which plugin is on
		// if (net_opt (PHYSOPT_PLUGINFO, NULL) != INFO_PLUG_TARP)

	entry (RS_MSG)
		process_incoming (RS_MSG, buf_ptr, packet_size, map_rssi(rssi));
		app_count.rcv++;
		proceed (RS_TRY);

endprocess (1)

#undef RS_TRY
#undef RS_MSG


/*
  --------------------
  audit process
  AS_ <-> Audit State
  --------------------
*/
#define AS_START	00
#define AS_TAGLOOP	10
process  (audit, void)
	static char * buf_ptr = NULL;
	static word ind;
	nodata;

	entry (AS_START)
		if (buf_ptr != NULL) {
			ufree (buf_ptr);
			buf_ptr = NULL;
		}
		if (tag_auditFreq == 0) {
			app_diag (D_WARNING, "Audit stops");
			kill (0);
		}
		ind = tag_lim;
		app_diag (D_DEBUG, "Audit starts");

	entry (AS_TAGLOOP)
		if (ind-- == 0) {
			app_diag (D_DEBUG, "Audit ends");
			delay (tag_auditFreq, AS_START);
			release;
		}

	entry (AS_TAGLOOP +1)
		check_tag (AS_TAGLOOP+1, ind, &buf_ptr);

		if (buf_ptr) {
			if (local_host == master_host) {
				in_header(buf_ptr, snd) = local_host;
				oss_report_out (buf_ptr, oss_fmt);
			} else
				send_msg (buf_ptr, sizeof(msgReportType));
			ufree (buf_ptr);
			buf_ptr = NULL;
		}
		proceed (AS_TAGLOOP);
		
endprocess (1)
#undef AS_START
#undef AS_TAGLOOP

/*
   --------------------
   cmd_in process
   CS_ <-> Command State
   --------------------
*/
#define CS_INIT 00
#define CS_IN	10
#define CS_WAIT 20

process (cmd_in, void)
	nodata;

	entry (CS_INIT)
		if (ui_ibuf == NULL)
			ui_ibuf = get_mem (CS_INIT, UI_BUFLEN);

	entry (CS_IN)
		ser_in (CS_IN, ui_ibuf, UI_BUFLEN); // hangs on the uart_a interrupt or polling
		if (strlen(ui_ibuf) == 0) // CR on empty line would do it
			proceed (CS_IN);

	entry (CS_WAIT)
		if (cmd_line != NULL) {
			wait (CMD_WRITER, CS_WAIT);
			release;
		}

		cmd_line = get_mem (CS_WAIT, strlen(ui_ibuf) +1);
		strcpy (cmd_line, ui_ibuf);
		trigger (CMD_READER);
		proceed (CS_IN);
endprocess (1)

#undef CS_INIT
#undef CS_IN
#undef CS_WAIT


/*
   --------------------
   Root process
   RS_ <-> Root State
   --------------------
*/
#define RS_INIT		00
#define RS_FREE		10
#define RS_RCMD		20
#define RS_DOCMD	30
#define RS_MEM		40
#define RS_UIOUT	50


process (root, void)
	static lword	in_tag, in_peg;
	static word	in_rssi, in_pl;
	lword 		in_lh, in_hid, in_mh;
	int		opt_sel;

	nodata;

	entry (RS_INIT)
		local_host = DEF_LOCAL_HOST;
		host_id = -local_host;

		// these survive for repeated commands;
		in_tag = in_peg = 0;
		in_rssi = in_pl = 0;

		init_tags();
		ui_out (RS_INIT, welcome_str);
		ui_obuf = get_mem (RS_INIT, UI_BUFLEN);

		if (net_init (INFO_PHYS_DEV, INFO_PLUG_TARP) < 0) {
			app_diag (D_FATAL, "net_init failed");
			reset();
		}

		(void) fork (rcv, NULL);
		(void) fork (cmd_in, NULL);
		(void) fork (audit, NULL);
		proceed (RS_RCMD);

	entry (RS_FREE)
		ufree (cmd_line);
		cmd_line = NULL;
		trigger (CMD_WRITER);

	entry (RS_RCMD)
		if (cmd_line == NULL) {
			wait (CMD_READER, RS_RCMD);
			release;
		}

	entry (RS_DOCMD)
		if (cmd_line[0] == ' ') // ignore if starts with blank
			proceed (RS_FREE);

                if (cmd_line[0] == 'h') {
			strcpy (ui_obuf, welcome_str);
			proceed (RS_UIOUT);
		}

		if (cmd_line[0] == 'q')
			reset();
			
		if (cmd_line[0] == 'm') {
			 scan (cmd_line+1, "%lu", &in_peg);
			 oss_master_in (RS_DOCMD, in_peg);
			 proceed (RS_FREE);
		}

		if (cmd_line[0] == 'f') {
			scan (cmd_line+1, "%lu, %lu, %u, %u",
				&in_tag, &in_peg, &in_rssi, &in_pl);
			*(word*)&in_tag = (in_rssi << 8 ) | in_pl;
			oss_findTag_in (RS_DOCMD, in_tag, in_peg);
			proceed (RS_FREE);
		}

		if (cmd_line[0] == 's') {
			in_lh = in_hid = in_mh = 0;
			scan (cmd_line+1, "%lu, %lu, %lu",
				&in_lh, &in_hid, &in_mh);
			if (in_lh)
				local_host = in_lh;
			if (in_hid)
				host_id = in_hid;
			if (in_mh)
				master_host = in_mh;
			stats();
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
		ui_out (RS_UIOUT, ui_obuf);
		proceed (RS_FREE);

endprocess (0) /* ganz egal? is (1) somehow better? */
#undef RS_INIT
#undef RS_FREE
#undef RS_MEM
#undef RS_RCMD
#undef RS_DOCMD
#undef RS_UIOUT
