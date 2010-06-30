/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "app_peg.h"
#include "app_peg_data.h"
#include "form.h"
#include "serf.h"
#include "ser.h"
#include "net.h"
#include "tarp.h"
#include "diag.h"
#include "msg_peg.h"
#include "storage.h"

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

// ============================================================================

// from, to, current, and for set_rf (sort of previous, on the Master only)
sattr_t sattr[4];

tout_t touts = { 0, 0, {(DEF_I_SIL << 12) | (DEF_I_WARM << 8) | DEF_I_CYC} };

static char *cmd_line = NULL;

word	app_flags 	= DEF_APP_FLAGS;
word	sstate		= ST_OFF; // waste of 12 bits
word	oss_stack	= 0;
word	cters[3]	= {0, 0, 0};

// ============================================================================

static trueconst char welcome_str[] = "***Site Survey 0.2***\r\n"
	"Set / show attribs:\r\n"
	"\tfrom:\t\tsf [pl [ra [ch]]]\r\n"
	"\tto:\t\tst [pl [ra [ch]]]\r\n"
	"\tid:\t\tsi x y <new x> <new y>\r\n"
	"\tuart:\t\tsu [0 | 1]\r\n"
	"\tfmt:\t\tsy [0 | 1]\r\n"
	"\tall out:\tsa [0 | 1]\r\n"
	"\tstats:\t\ts [x y]\r\n\r\n"
	"Timeouts / intervals:\r\n"
	"\tt [<silence> [<warmup> [<cycle>]]]\r\n\r\n"
	"Execute Operation:\r\n"
	"\tsilence:\tos\r\n"
	"\tgo:\t\tog\r\n"
	"\tmaster:\t\tom\r\n"
	"\tauto:\t\toa\r\n"
	"\terase:\t\toe\r\n\r\n"
	"Help: \th\r\n";

static trueconst char ill_str[] = "Illegal command (%s)\r\n";
static trueconst char bad_str[] = "Bad or incomplete command (%s)\r\n";
static trueconst char bad_state_str[] = "Not allowed in state (%u)\r\n";

static trueconst char stats_str[] = 
	"Stats for %s hid - lh %lx - %u (%u, %u):\r\n"
	" Attr pl %u-%u (%u), rate %u-%u (%u), ch %u-%u (%u)\r\n"
	" Counts m(%u) r(%u) t(%u) u(%u), touts: sil %u, warm %u, cyc %u\r\n"
	" Time %lu (sstart %lu), Flags %x, Mem: free %u, min %u\r\n"
	" State %u RF %u\r\n";

static trueconst char ping_ascii_def[] = "Ping %lu\r\n"
	"[%u %u %u] %u (%u) (%u, %u)->(%u, %u)\r\n"
	"[%u %u %u] %u (%u) (%u, %u)->(%u, %u)\r\n";

static trueconst char ping_ascii_raw[] = " %lu %u %u %u %u %u %u %u %u %u"
	" %u %u %u %u %u %u %u %u %u\r\n";

// ============================================================================

idiosyncratic int tr_offset (headerType*);

idiosyncratic Boolean msg_isBind (msg_t m), msg_isTrace (msg_t m),
		      msg_isMaster (msg_t m), msg_isNew (msg_t m),
		      msg_isClear (byte o);

idiosyncratic void set_master_chg (void);

// ============================================================================

static int next_cyc () {

	if (is_uart_out)
		stats (NULL);

	sattr[3].w = sattr[2].w;
	if (sattr[2].b.pl == sattr[1].b.pl) {
		sattr[2].b.pl = sattr[0].b.pl;
		if (sattr[2].b.ra == sattr[1].b.ra) {
			sattr[2].b.ra = sattr[0].b.ra;
			if (sattr[2].b.ch == sattr[1].b.ch) {
				app_diag (D_UI, "Done");
				runfsm m_sil;
				return 1;
			} else {
				if (sattr[0].b.ch > sattr[1].b.ch) {
					sattr[2].b.ch--;
				} else {
					sattr[2].b.ch++;
				}
			}
		} else {
			if (sattr[0].b.ra > sattr[1].b.ra) {
				sattr[2].b.ra--;
			} else {
				sattr[2].b.ra++;
			}
		}
	} else {
		if (sattr[0].b.pl > sattr[1].b.pl) {
			sattr[2].b.pl--;
		} else {
			sattr[2].b.pl++;
		}
	}

	return 0;
}

static void process_incoming (word state, char * buf, word size, word rssi) {

  if (check_msg_size (buf, size, D_SERIOUS) != 0)
	  return;

  switch (in_header(buf, msg_type)) {

	case msg_master:
		msg_master_in (buf);
		return;

	case msg_ping:
		msg_ping_in (buf, rssi);
		return;

	case msg_cmd:
		msg_cmd_in (buf);
		return;

	case msg_sil:
		msg_sil_in (buf);
		return;

	case msg_stats:
		stats (buf);
		return;

	default:
		app_diag (D_SERIOUS, "Got ? (%u)", in_header(buf, msg_type));

  }
}

static word map_rssi (word r) {

// [0, FF] -> [1, FF]
// Keep 0 reserved for praxes special needs

	return  r >> 8; // get rid of 'link quality'
#if 0

	in waiting for serious works on RF channels in VUEE
#ifdef __SMURPH__
/* temporary rough estimates
 =======================================================
 RP(d)/XP [dB] = -10 x 5.1 x log(d/1.0m) + X(1.0) - 33.5
 =======================================================

*/
	if ((r >> 8) > 151) return 3;
	if ((r >> 8) > 118) return 2;
	return 1;
#else

	if ((r >> 8) > 181) return 3;
	if ((r >> 8) > 150) return 2;
	return 1;
#endif
#endif

}



// =============
// OSS reporting
// =============
fsm oss_out (char) {

	entry OO_INIT:

		if (data == NULL) {
			show_stuff (1);
			//app_diag (D_SERIOUS, "NULL in oss_out");
			finish;
		}
		oss_stack++;

        entry OO_RETRY:

		ser_outb (OO_RETRY, data);

		if (oss_stack == 0) // never
			app_diag (D_SERIOUS, "Mayday oss_stack");
		else
			oss_stack--;

                finish;
}

fsm m_cyc {

  entry MC_INIT:

	app_diag (D_DEBUG, "m_cyc starts");

  entry MC_LOOP:

	net_opt (PHYSOPT_RXOFF, NULL);
	net_opt (PHYSOPT_TXON, NULL);
	net_qera (TCV_DSP_XMT);
	net_qera (TCV_DSP_RCV);
	set_state (ST_WARM);

  entry MC_MBEAC:

	set_rf ((touts.mc & 1) ? 0 : 1); // last one at current params!!
	// FIXME will that help?
	//delay (512, MC_MBEAC2);
	//release;

  entry MC_MBEAC2:

	msg_master_out ();
	if (touts.mc > 0) {
		touts.mc--;
		delay (512, MC_MBEAC);
		release;
	}
	net_opt (PHYSOPT_RXON, NULL);
	sattr[3].w = sattr[2].w;
	set_rf (1);
	set_ping_out;
	set_state (ST_CYC);

  entry MC_SEED:

	if (!is_ping_out) // a ping was received
		proceed MC_CYC;
	msg_ping_out (NULL, 0);
	if (seconds() - touts.ts >= touts.iv.b.sil) {
		app_diag (D_WARNING, "Silence at seeding %d %d %d",
				net_opt (PHYSOPT_GETPOWER, NULL),
				net_opt (PHYSOPT_GETRATE, NULL),
				net_opt (PHYSOPT_GETCHANNEL, NULL));
		proceed MC_NEXT;
	}
	delay (1024, MC_SEED);
	release;

  entry MC_CYC:

	when (SILENC, MC_NEXT);
	delay ((word)touts.iv.b.cyc << 10, MC_NEXT);
	release;

  entry MC_NEXT:

	if (next_cyc ())
		finish;

	proceed MC_LOOP;
}

fsm m_sil {

  entry MS_INIT:

	net_opt (PHYSOPT_RXOFF, NULL);
	net_opt (PHYSOPT_TXON, NULL);
	net_qera (TCV_DSP_XMT);
	net_qera (TCV_DSP_RCV);
	set_state (ST_COOL);

  entry MS_SBEAC:

	set_rf ((touts.mc & 1) ? 0 : 1);
	//  FIXME will that help?
	// delay (511, MS_SBEAC2);
	// release;

  entry MS_SBEAC2:

	msg_sil_out ();
	if (touts.mc > 0) {
		touts.mc--;
		delay (1024, MS_SBEAC);
		release;
	}

	set_state (ST_INIT);
	net_opt (PHYSOPT_RXON, NULL);
	net_opt (PHYSOPT_TXOFF, NULL);
	finish;
}

void show_stuff (word s) {

	word mmin, mem;
	mem = memfree(0, &mmin);
	diag ("?%u %u %u %u %u %u %u", s, mmin, mem, oss_stack,
			 net_qsize (TCV_DSP_XMT),
			 net_qsize (TCV_DSP_RCV),
			 crunning (NULL)
			 );
}

void stats (char * buf) {

	char * mbuf = NULL;
	word mmin, mem;

	if (!is_uart_out)
		return;

	if (buf == NULL) {
		mem = memfree(0, &mmin);
		mbuf = form (NULL, stats_str, "local",
			host_id, local_host, local_host >> 8, local_host & 0xFF,
			sattr[0].b.pl, sattr[1].b.pl, sattr[2].b.pl,
			sattr[0].b.ra, sattr[1].b.ra, sattr[2].b.ra,
			sattr[0].b.ch, sattr[1].b.ch, sattr[2].b.ch,
			touts.mc, cters[0], cters[1], cters[2],
			touts.iv.b.sil, touts.iv.b.warm, touts.iv.b.cyc,
			seconds(), touts.ts,
			app_flags, mem, mmin, sstate, 
			net_opt (PHYSOPT_STATUS, NULL));
	} else {
		mbuf = form (NULL, stats_str, "remote",
			in_stats(buf, hid), in_stats(buf, lh),
			in_stats(buf, lh) >> 8, in_stats(buf, lh) & 0xFF,
			in_stats(buf, satt[0]).b.pl,
			in_stats(buf, satt[1]).b.pl,
			in_stats(buf, satt[2]).b.pl,
			in_stats(buf, satt[0]).b.ra,
			in_stats(buf, satt[1]).b.ra,
			in_stats(buf, satt[2]).b.ra,
			in_stats(buf, satt[0]).b.ch,
			in_stats(buf, satt[1]).b.ch,
			in_stats(buf, satt[2]).b.ch,
			in_stats(buf, to).mc, 77, 77, 77,
			in_stats(buf, to).iv.b.sil,
			in_stats(buf, to).iv.b.warm,
			in_stats(buf, to).iv.b.cyc,
			in_stats(buf, secs),
			in_stats(buf, to).ts,
			in_stats(buf, fl),
			in_stats(buf, mem), in_stats(buf, mmin), 77, 77);
	}

	if (runfsm oss_out (mbuf) == 0) {
		app_diag (D_SERIOUS, "oss_out fork");
		ufree (mbuf);
	}
}

/*
   --------------------
   Receiver process
   RS_ <-> Receiver State
   --------------------
*/

// In this model, a single rcv is forked once, and runs / sleeps all the time
fsm rcv {

	shared int packet_size;
	shared char *buf_ptr;
	shared word rssi;

	entry RC_INIT:

		packet_size = 0;
		buf_ptr = NULL;
		rssi = 0;

	entry RC_TRY:

		if (buf_ptr != NULL) {
			ufree (buf_ptr);
			buf_ptr = NULL;
			packet_size = 0;
		}
		delay ((word)touts.iv.b.sil << 10, RC_SIL);
    		packet_size = net_rx (RC_TRY, &buf_ptr, &rssi, 0);
		if (packet_size <= 0) {
			show_stuff (0);
			//app_diag (D_SERIOUS, "net_rx failed (%d)",
			//	packet_size);
			proceed RC_TRY;
		}

		app_diag (D_DEBUG, "RCV (%d): %x-%u-%u-%u-%u-%u\r\n",			  
		packet_size, in_header(buf_ptr, msg_type),
			  in_header(buf_ptr, seq_no) & 0xffff,
			  in_header(buf_ptr, snd),
			  in_header(buf_ptr, rcv),
			  in_header(buf_ptr, hoc) & 0xffff,
			  in_header(buf_ptr, hco) & 0xffff);

		// that's how we could check which plugin is on
		// if (net_opt (PHYSOPT_PLUGINFO, NULL) != INFO_PLUG_TARP)

	entry RC_MSG:

		// FIXME should we throttle this one too??
		if (net_qsize (TCV_DSP_RCV) > DEF_XMT_THOLD) {
			cters[0]++;
			proceed RC_TRY;
		}

		// in case non-uart nodes engage in endless exchange:
		if (sstate != ST_INIT && !is_uart_out &&
				seconds() - touts.ts >
				DEF_I_SIL + DEF_I_WARM + DEF_I_CYC) {
			app_diag (D_WARNING, "Reset on unobserved ping-pong");
			reset();
		}

		process_incoming (RC_MSG, buf_ptr, packet_size,
			map_rssi(rssi));
		proceed RC_TRY;

	entry RC_SIL:

		trigger (SILENC);
		app_diag (D_DEBUG, "SILENCE trigger %d %d %d",
				net_opt (PHYSOPT_GETPOWER, NULL),
				 net_opt (PHYSOPT_GETRATE, NULL),
				  net_opt (PHYSOPT_GETCHANNEL, NULL));

		if (sstate != ST_INIT && !is_uart_out &&
				seconds() - touts.ts >
				DEF_I_SIL + DEF_I_WARM + DEF_I_CYC) {
			app_diag (D_WARNING, "Reset on long silence");
			reset();
		}

		proceed RC_TRY;
}

/*
   --------------------
   cmd_in process
   CS_ <-> Command State
   --------------------
*/
fsm cmd_in {

	shared char *ui_ibuf = NULL;

	entry CS_INIT:

		if (ui_ibuf == NULL)
			ui_ibuf = get_mem (CS_INIT, UI_BUFLEN);

	entry CS_IN:

		// hangs on the uart_a interrupt or polling
		ser_in (CS_IN, ui_ibuf, UI_BUFLEN);
		if (strlen(ui_ibuf) == 0)
			// CR on empty line would do it
			proceed CS_IN;

	entry CS_WAIT:

		if (cmd_line != NULL) {
			when (CMD_WRITER, CS_WAIT);
			release;
		}

		cmd_line = get_mem (CS_WAIT, strlen(ui_ibuf) +1);
		strcpy (cmd_line, ui_ibuf);
		trigger (CMD_READER);
		proceed CS_IN;
}

void oss_ping_out (char * buf, word rssi) {

	char * lbuf = NULL;
	char * my_str;

	my_str = is_fmt_hi ? (char *)ping_ascii_def : (char *)ping_ascii_raw;

	lbuf = form (NULL, my_str, seconds(),
			in_ping(buf, in_sattr).b.pl,
			in_ping(buf, in_sattr).b.ra,
			in_ping(buf, in_sattr).b.ch,
			in_ping(buf, rssi),
			in_ping(buf, pings),
			in_ping(buf, id) >> 8, in_ping(buf, id) & 0xFF,
			in_header(buf, snd) >> 8, in_header(buf, snd) & 0xFF,
			in_ping(buf, out_sattr).b.pl, 
			in_ping(buf, out_sattr).b.ra,
			in_ping(buf, out_sattr).b.ch, rssi, touts.mc,
			in_header(buf, snd) >> 8, in_header(buf, snd) & 0xFF,
			local_host >> 8, local_host & 0xFF);

	if (runfsm oss_out (lbuf) == 0 ) {
		app_diag (D_SERIOUS, "oss_out failed");
		ufree (lbuf);
    }	
}

// ==========================================================================

/*
   --------------------
   Root process
   RS_ <-> Root State
   --------------------
*/

fsm root {

	shared char *ui_obuf = NULL;

	sint	i1, i2, i3, i4, i5;
	nvm_t	nvm;

	entry RS_INIT:

		ser_out (RS_INIT, welcome_str);
		local_host = (word)host_id;
		net_id = DEF_NID;
		tarp_ctrl.param = DEF_TARP;

		init();
		ui_obuf = get_mem (RS_INIT, UI_BUFLEN);

		if (net_init (INFO_PHYS_DEV, INFO_PLUG_TARP) < 0) {
			app_diag (D_FATAL, "net_init failed");
			reset();
		}
		net_opt (PHYSOPT_SETSID, &net_id);
		init();
		runfsm rcv;
		runfsm cmd_in;
		if (local_host == master_host) {
			app_diag (D_UI, "%ds to warmup", DEF_I_SIL);
			delay (DEF_I_SIL << 10, RS_WARMUP);
			when (CMD_READER, RS_RCMD);
			release;
		}
		clr_uart_out;
		proceed RS_RCMD;

	entry RS_FREE:

		ufree (cmd_line);
		cmd_line = NULL;
		trigger (CMD_WRITER);

	entry RS_RCMD:

		if (!is_uart_out)
			app_diag (D_UI, "I'm mute!");

	entry RS_RCMD2:

		if (cmd_line == NULL) {
			when (CMD_READER, RS_RCMD2);
			release;
		}

	entry RS_DOCMD:

		switch (cmd_line[0]) {
			case ' ': proceed RS_FREE; // ignore blank
			case 'q': reset();
			case 's': proceed RS_SETS;
			case 't': proceed RS_TOUTS;
			case 'o': proceed RS_OPERS;
			case '?':
			case 'h': ser_out (RS_DOCMD, welcome_str);
				  proceed RS_FREE;
			default:
				form (ui_obuf, ill_str, cmd_line);
				proceed RS_UIOUT;
		}

	entry RS_SETS:

		if ((i1 = strlen (cmd_line)) != 1 && sstate != ST_INIT) {
			app_diag (D_WARNING, "Careful: not in INIT state (%d)",
					sstate);
			//form (ui_obuf, bad_state_str, sstate);
			//proceed RS_UIOUT;
		}

		if (i1 == 1) {
			stats(NULL);
			proceed RS_FREE;
		}

		i1 = i2 = i3 = i4 = i5 = -1;
		i5 = scan (cmd_line +2, "%d %d %d %d", &i1, &i2, &i3, &i4);
		switch (cmd_line [1]) {
			case 'f':
				i4 = 0;
				// fall through
			case 't':
				if (i4 == -1)
					i4 = 1;
				if (i1 >= 0 && i1 < 8)
					sattr[i4].b.pl = i1;
				if (i2 >= 0 && i2 < 4)
					sattr[i4].b.ra = i2;
				if (i3 >= 0 && i3 < 256)
					sattr[i4].b.ch = i3;
				break;
			case 'i':
				if (i5 != 4) {
					form (ui_obuf, bad_str, cmd_line);
					proceed RS_UIOUT;
				}
				if ((i5 = (i1 << 8) + i2) == local_host) {
					local_host = (i3 << 8) | i4;
					break;
				}
				msg_cmd_out ('i', i5, (i3 << 8) | i4);
				break;
			case 'u':
				if (i1 > 0)
					set_uart_out;
				else if (i1 == 0)
					clr_uart_out;
				break;
			case 'y':
				if (i1 > 0)
					set_fmt_hi;
				else if (i1 == 0)
					clr_fmt_hi;
				break;
			case 'a':
				if (i1 > 0)
					set_all_out;
				else if (i1 == 0)
					clr_all_out;
				break;
			default:
				form (ui_obuf, ill_str, cmd_line);
				proceed RS_UIOUT;
		}
		stats (NULL);
		proceed RS_FREE;

	entry RS_TOUTS:

		if (sstate != ST_INIT) {
			form (ui_obuf, bad_state_str, sstate);
			proceed RS_UIOUT;
		}
		i1 = i2 = i3 = -1;
		scan (cmd_line +2, "%d %d %d", &i1, &i2, &i3);

		if (i1 >= 0 && i1 < 16)
			touts.iv.b.sil = i1;
		if (i2 >= 0 && i2 < 16)
			touts.iv.b.warm = i2;
		if (i3 >= 0 && i3 < 256)
			touts.iv.b.cyc = i3;
		stats (NULL);
		proceed RS_FREE;

	entry RS_OPERS:

		switch (cmd_line [1]) {

			case 's':
				if (local_host != master_host) {
					app_diag (D_WARNING, "master only");
					proceed RS_FREE;
				}

				if (running (m_sil)) {
					app_diag (D_WARNING, "already running");
					proceed RS_FREE;
				}

				killall (m_cyc);
				runfsm m_sil;
				break;

			case 'g':
				if (local_host != master_host) {
					app_diag (D_WARNING, "master only");
					proceed RS_FREE;
				}

				if (running (m_cyc)) {
					app_diag (D_WARNING, "already running");
					proceed RS_FREE;
				}

				killall (m_sil);
				sattr[2].w = sattr[0].w;
				runfsm m_cyc;
				break;

			case 'm':
				net_opt (PHYSOPT_RXOFF, NULL);
				net_opt (PHYSOPT_TXOFF, NULL);
				net_qera (TCV_DSP_XMT);
				net_qera (TCV_DSP_RCV);
				killall (m_cyc);
				killall (m_sil);
				master_host = local_host;
				set_state (ST_INIT);
				break;

			case 'a':
				nvm.mh = master_host;
				nvm.lh = local_host;
				nvm.iv.w = touts.iv.w;
				nvm.attr[0].w = sattr[0].w;
				nvm.attr[1].w = sattr[1].w;
				if (ee_write (WNONE, NVM_OSET, (byte *)&nvm,
						sizeof (nvm_t)))
					app_diag (D_SERIOUS,
						"Can't write eeprom");
				break;

			case 'e':
				if (ee_erase (WNONE, NVM_OSET,
						NVM_OSET + sizeof (nvm_t)))
					app_diag (D_SERIOUS,
						"Can't erase eeprom");
				break;

			default:
				form (ui_obuf, bad_str, cmd_line);
				proceed RS_UIOUT;
		}

		stats (NULL);
		proceed RS_FREE;

	entry RS_UIOUT:

		if (is_uart_out)
			ser_out (RS_UIOUT, ui_obuf);
		proceed RS_FREE;

	entry RS_WARMUP:

		if (!running (m_cyc))
			runfsm m_cyc;
		else
			app_diag (D_SERIOUS, "Cycle is running");
		proceed RS_FREE;
}
