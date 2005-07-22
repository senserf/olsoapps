/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "form.h"
#include "net.h"
#include "tarp.h"
#include "msg_rtag.h"
#include "ser.h"
#include "diag.h"
#include "lib_app_if.h"
// #include "trc.h"

#define TINY_MEM 1

heapmem {70, 30}; // how to find out a good ratio?

// elsewhere may be a better place for this:
#if CHIPCON
#define INFO_PHYS_DEV INFO_PHYS_CHIPCON
#else
#if DM2100
#define INFO_PHYS_DEV INFO_PHYS_DM2100
#else
#define INFO_PHYS_DEV INFO_PHYS_RADIO
#endif
#endif

#if UART_DRIVER
#define ui_out	ser_out
#define ui_in	ser_in
#else
#define ui_out(a, b) 
#define ui_in(a, b, c)  
#endif

#if TINY_MEM
#define UI_BUFLEN		128
#else
#define UI_BUFLEN		256
#endif

static char * ui_ibuf = NULL;
static char * ui_obuf = NULL;

// command line and its semaphores
static char * cmd_line	= NULL;
#define CMD_READER	((word)&cmd_line)
#define CMD_WRITER	((word)((&cmd_line)+1))

// format strings
#if TINY_MEM
static const char welcome_str[] = "\r\nRTag cmds:\r\n"
	"s lh mid pl hid ts tl\r\n"
	"m, r, t, i [rcv]\r\n"
	"o[{r|t| }{+|-}]\r\n"
	"b msec\r\n"
	"h, q\r\n";
#else
static const char welcome_str[] = "\r\nWelcome to RTag Testbed\r\n"
	"Commands:\r\n"
	"\tSet:\ts [ lh [ mid [ pl [ hid [ ts [ tl ]]]]]]\r\n"
	"\tSnd Master:\tm [ rcv ]\r\n"
	"\tSnd RPC:\tr [ rcv ]\r\n"
	"\tSnd Trace:\tt [ rcv ]\r\n"
	"\tSnd Info:\ti [ rcv ]\r\n"
	"\tOpt RF:\t\to[{r|t| }{+|-}]\r\n"
	"\tBeacon:\t\tb [ <msec> ]\r\n"
	"\tHelp:\t\th\r\n"
	"\tq(uit)\r\n";
#endif
static const char o_str[] =     "phys: %x, plug: %x, txrx: %x, pl: %u\r\n";

static const char ill_str[] =	"Illegal command: %s\r\n";

#if TINY_MEM
#if MALLOC_STATS
static const char stats_mem_str[] = " Mem (st, fr, ma, ch, fa) "
					"%u, %u, %u, %u, %u"
#if !MALLOC_SINGLEPOOL
			         "\r\n                         "
				 	"    %u, %u, %u, %u"
#endif
					;
#endif
static const char stats_str[] = "hid:lh(%lx:%lu)\r\n"
	"In(%u:%u) Out(%u:%u) Fwd(%u:%u)\r\n"
	"T(%lu) D(%ld) M(%lu) tarp s:l((%u:%u)"
#else
static const char stats_str[] = "Stats for hId: lHost (%lx:%lu):\r\n"
	" In (%u:%u) Out (%u:%u) Fwd (%u:%u)\r\n"
	" Time (%lu) Delta (%ld) to Master (%lu) PLev %x\r\n"
	" phy %x plug %x txrx %x tarp s:l (%u:%u)\r\n"
#if MALLOC_STATS
	" Mem (st, fr, ma, ch, fa) %u, %u, %u, %u\r\n"
#if !MALLOC_SINGLEPOOL
	" Max free (%u, %u) chunks (%u, %u)\r\n"
#endif
#endif
#endif
	;

extern tarpCountType tarp_count;
extern word tarp_level;
extern word tarp_slack;

// Display node stats on UI
static void stats () {
#if MALLOC_STATS
	word faults0, chunks0;
	word mem0 = memfree(0, &faults0);
	word max0 = maxfree(0, &chunks0);
#if !MALLOC_SINGLEPOOL
	word faults1, chunks1;
	word mem1 = memfree(1, &faults1);
	word max1 = maxfree(1, &chunks1);
#endif
#endif
	
#if TINY_MEM
	app_diag (D_UI, stats_str, host_id, local_host,
		app_count.rcv, tarp_count.rcv,
		app_count.snd, tarp_count.snd,
		app_count.fwd, tarp_count.fwd,
		seconds(), master_delta, master_host,
		tarp_slack, tarp_level);
#if MALLOC_STATS
	app_diag (D_UI, stats_mem_str, stackfree(),
		mem0, max0, chunks0, faults0
#if !MALLOC_SINGLEPOOL
		, mem1, max1, chunks1, faults1
#endif
		);
#endif

#else
	app_diag (D_UI, stats_str,
			host_id, local_host, 
			app_count.rcv, tarp_count.rcv,
			app_count.snd, tarp_count.snd, 
			app_count.fwd, tarp_count.fwd,
			seconds(), master_delta, master_host,
			net_opt (PHYSOPT_PHYSINFO, NULL),
			net_opt (PHYSOPT_PLUGINFO, NULL),
			net_opt (PHYSOPT_STATUS, NULL),
			tarp_slack, tarp_level
#if MALLOC_STATS
			, stackfree(), mem0, max0, chunks0, faults0
#if !MALLOC_SINGLEPOOL
			, mem1, max1, chunks1, faults1
#endif
#endif
			);
#endif
}

static void process_incoming (word state, char * buf, word size, word rssi) {
  int    	w_len;
  
  switch (in_header(buf, msg_type)) {

	case msg_master:
		msg_master_in (buf);
		stats();
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

	case msg_trace:
		msg_trace_in (state, buf);
		return;

	case msg_info:
		in_header(buf, msg_type) = rssi; // hack alert!
		msg_info_in (buf);
		return;

	case msg_traceAck:
		msg_traceAck_in (buf, size);
		return;

	default:
		app_diag (D_SERIOUS, "Got ? (%u)", in_header(buf, msg_type));

  }
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

	entry (RS_MSG)
		process_incoming (RS_MSG, buf_ptr, packet_size, rssi);
		app_count.rcv++;
		proceed (RS_TRY);

endprocess (1)

#undef RS_TRY
#undef RS_MSG

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
		// hangs on the uart_a interrupt or polling
		ui_in (CS_IN, ui_ibuf, UI_BUFLEN);
		if (strlen(ui_ibuf) == 0) // CR on empty line does it
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
#define RS_UIOUT	50


process (root, void)

	// input (s command)
	word in_pl;
	id_t in_lh, in_hid, in_mh;
	
	nodata;

	entry (RS_INIT)
		ui_out (RS_INIT, welcome_str);
		if (net_init (INFO_PHYS_DEV, INFO_PLUG_TARP) < 0) {
			app_diag (D_FATAL, "net_init failed");
			reset();
		}
		net_opt (PHYSOPT_RXON, NULL);
		net_opt (PHYSOPT_TXON, NULL);
#if CHIPCON
		net_opt (PHYSOPT_SETPOWER, &pow_level);
#endif
		(void) fork (rcv, NULL);
#if UART_DRIVER
		(void) fork (cmd_in, NULL);
#endif
		proceed (RS_RCMD);

	entry (RS_FREE)
		ufree (cmd_line);
		cmd_line = NULL;
		ufree (ui_obuf);
		ui_obuf = NULL;
		trigger (CMD_WRITER);

	entry (RS_RCMD)
		if (cmd_line == NULL) {
			wait (CMD_READER, RS_RCMD);
			release;
		}
		if (ui_obuf == NULL) 
			ui_obuf = get_mem (RS_RCMD, UI_BUFLEN);

	entry (RS_DOCMD)
		if (cmd_line[0] == ' ') // ignore if starts with blank
			proceed (RS_FREE);

                if (cmd_line[0] == 'h') {
			strcpy (ui_obuf, welcome_str);
			proceed (RS_UIOUT);
		}

		if (cmd_line[0] == 'q')
			reset();

		if (cmd_line[0] == 'b') {
			scan (cmd_line+1, "%u", &beac_freq);
			app_diag (D_UI, "Beacon at %u", beac_freq);
			proceed (RS_FREE);
		}

		if (cmd_line[0] == 'm') {
			in_lh = 0;
			scan (cmd_line+1, "%lu", &in_lh);
			oss_master_in (RS_DOCMD, in_lh);
			proceed (RS_FREE);
		}

		if (cmd_line[0] == 'r') {
			oss_rpc_in (RS_DOCMD, cmd_line+1);
			proceed (RS_FREE);
		}

		if (cmd_line[0] == 't') {
			in_lh = 0;
			scan (cmd_line+1, "%lu", &in_lh);
			oss_trace_in (RS_DOCMD, in_lh);
			proceed (RS_FREE);
		}

		if (cmd_line[0] == 'i') {
			in_lh = 0;
			scan (cmd_line+1, "%lu", &in_lh);
			oss_info_in (RS_DOCMD, in_lh);
			proceed (RS_FREE);
		}

		if (cmd_line[0] == 'o') {
			in_pl = 0;
			if (strlen (cmd_line) > 2) {
				switch (cmd_line[1]) {
					case 'r':
						in_pl = 1;
						break;
					case 't':
						in_pl = 2;
						break;
					case ' ':
						in_pl = 3;
						break;
					case 'p':
					  scan (cmd_line+2, "%u", &in_pl);
					  if (in_pl && in_pl != pow_level) {
#if CHIPCON
					    pow_level = in_pl;
					    net_opt (PHYSOPT_SETPOWER,
							&pow_level);
#else
					    app_diag (D_WARNING, "No var pLev");
#endif
					    }
					    form (ui_obuf, o_str,
					      net_opt (PHYSOPT_PHYSINFO, NULL),
					      net_opt (PHYSOPT_PLUGINFO, NULL),
					      net_opt (PHYSOPT_STATUS, NULL),
					      pow_level);
					    proceed (RS_UIOUT);
					default:
					  form (ui_obuf, ill_str, cmd_line);
					  proceed (RS_UIOUT);
				}
				switch (cmd_line[2]) {
					case '+':
					  if (in_pl & 1)
						net_opt (PHYSOPT_RXON, NULL);
					  if (in_pl & 2)
						net_opt (PHYSOPT_TXON, NULL);
					  break;
					case '-':
					  if (in_pl & 1)
						net_opt (PHYSOPT_RXOFF, NULL);
					  if (in_pl & 2)
						net_opt (PHYSOPT_TXOFF, NULL);
					  break;
					default:
					  form (ui_obuf, ill_str, cmd_line);
					  proceed (RS_UIOUT);
				}
			}
			form (ui_obuf, o_str, net_opt (PHYSOPT_PHYSINFO, NULL),
				net_opt (PHYSOPT_PLUGINFO, NULL),
				net_opt (PHYSOPT_STATUS, NULL), pow_level);
			proceed (RS_UIOUT);
		}

		if (cmd_line[0] == 's') {
			in_lh = in_hid = in_mh = 0;
			scan (cmd_line+1, "%lu %lu %lx %u %u",
				&in_lh, &in_mh,
				&in_hid, &tarp_slack, &tarp_level);
			if (in_lh)
				local_host = in_lh;
			if (in_mh)
				master_host = in_mh;
			if (in_hid)
				host_id = in_hid;
			stats();
			proceed (RS_FREE);
		}

		form (ui_obuf, ill_str, cmd_line);

	entry (RS_UIOUT)
		ui_out (RS_UIOUT, ui_obuf);
		proceed (RS_FREE);

endprocess (0) // ganz egal? is (1) somehow more efficient? 
#undef RS_INIT
#undef RS_FREE
#undef RS_RCMD
#undef RS_DOCMD
#undef RS_UIOUT
