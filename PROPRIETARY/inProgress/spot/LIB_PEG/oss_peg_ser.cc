/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2014                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "variants.h"
#include "form.h"
#include "ser.h"
#include "diag.h"
#include "tag_mgr.h"
#include "tarp.h"

fsm mbeacon;

///////////////// oss out ////////////////
// this is better than multiple fsm oss_out I used before
//
// 15 is max with the sizes in FIFEK
#define FIFEK_SIZ 15
static struct { // yes, not all field are truly needed but same size it is
	char	*buf [FIFEK_SIZ];
	word	h :4;
	word	t :4;
	word	n :4;
	word	o :4;
} fifek;

static word _oss_out (char * b) {

	if (b == NULL)
		return 2;

	if (fifek.n < FIFEK_SIZ) {
		fifek.buf[fifek.h] = b;
		++fifek.n; ++fifek.h; fifek.h %= FIFEK_SIZ;
		trigger (TRIG_OSSO);
		return 0;
	}
	app_diag_S ("OSS oflow (%u) %u", seconds(), ++fifek.o);
	ufree (b);
	return 1;
}
 
fsm perp_oss_out () {

        state CHECK:
		if (fifek.n == 0) {
			when (TRIG_OSSO, CHECK);
			release;
		}

        state RETRY:
                ser_outb (RETRY, fifek.buf[fifek.t]);
                --fifek.n; ++fifek.t; fifek.t %= FIFEK_SIZ;
                proceed (CHECK);
}
#undef FIFEK_SIZ

///////////////// ? where should it be? ////////////////

static trueconst char welcome_str[] = "SPOT(1.0) Peg:\r\n"
  "\t? - status quo\r\n\tm - become master / force mbeacon\r\n"
  "\th - help\r\nq - quit\r\n";

static trueconst char stats_str[] = "Node %u uptime %u.%u:%u:%u "
	"master %u mem %u %u oflow %u\r\n";

static char * stats () {
        char * b = NULL;
        word mmin, mem;
	mem = memfree(0, &mmin);
        b = form (NULL, stats_str, local_host, (word)(seconds() / 86400),
		(word)((seconds() % 86400) / 3600), 
		(word)((seconds() % 3600) / 60),
		(word)(seconds() % 60), master_host,
                mem, mmin, fifek.o);
	return b;
}
///////////////// oss in ////////////////

/* no need for that here, but we could build appropriate structs here and
   call process_incoming() as fsm hear does for RF in.
*/

fsm cmd_in {
        char * obuf;
        char   ibuf [UART_INPUT_BUFFER_LENGTH];

	state START:
		obuf = (char *)welcome_str;
		proceed UIOUT;

        state INI:
                ibuf[0] = '\0';

        state LISN: // LISTEN keyword??
                ser_in (LISN, ibuf, UART_INPUT_BUFFER_LENGTH);
                if (strlen(ibuf) == 0 // CR on empty line would do it
                        || ibuf[0] == ' ') // ignore lines starting blank
                                proceed LISN;

		switch (ibuf[0]) {
			case 'h':
				obuf = (char *)welcome_str;
				proceed UIOUT;

			case 'q': reset();

			case 'm':
				if (local_host == master_host) {
					if (running (mbeacon)) {
					    trigger (TRIG_MBEAC);
					    obuf = (char *)"Sent mbeacon\r\n";
					} else {
					    runfsm mbeacon;
					    obuf = (char *)"?tarted mbeacon\r\n";
					}
				} else {
					master_host = local_host;
					runfsm mbeacon;
					obuf = (char *)"I am M\r\n";
				}
				proceed UIOUT;

			default: // ?
				obuf = stats();
				_oss_out (obuf);
				proceed INI;
		}

	state UIOUT:
                ser_out (UIOUT, obuf);
		proceed INI;
}

void oss_ini () {

#ifdef BOARD_WARSAW_BLUE
	// Use UART 2 via Bluetooth
	ser_select (1);
#endif
	if (!running (perp_oss_out))
		runfsm perp_oss_out;

	if (!running (cmd_in))
		runfsm cmd_in;
}

#define _ppp	(p + sizeof(pongDataType))
static char * board_out (char * p) {
	char * b;

	switch (((pongDataType *)p)->btyp) {
		case 0: // chronoses
		case 1:
			b = form (NULL, "V %u move %u.%u",
				((pongPloadType0 *)_ppp)->volt,
				((pongPloadType0 *)_ppp)->move_ago,
				((pongPloadType0 *)_ppp)->move_nr);
			break;
		case 2:
			 b = form (NULL, "V %u",
				((pongPloadType2 *)_ppp)->volt);
			break;
		case 3:
			b = form (NULL, "V %u dial %u.%u",
				((pongPloadType3 *)_ppp)->volt,
				((pongPloadType3 *)_ppp)->dial >> 8,
				((pongPloadType3 *)_ppp)->dial & 0xFF);
			break;
                case 4:
                         b = form (NULL, "V %u",
                                ((pongPloadType4 *)_ppp)->volt);
			break;
                case 5:
                        b = form (NULL, "V %u s %u.%u",
                                ((pongPloadType5 *)_ppp)->volt,
                                ((pongPloadType5 *)_ppp)->random_shit,
                                ((pongPloadType5 *)_ppp)->steady_shit);
		default:
			app_diag_W ("btyp %u", ((pongDataType *)p)->btyp);
			b = NULL;
	}
	return b;
}
#undef _ppp

// this is the main out on the oss i/f, I don't know at all if this makes
// more sense than no generic i/f at all
// NO parameter sanitization
void oss_tx (char * b, word siz) {
	char *bu, *bt;

	// allocated, freed after output
	if (siz == MAX_WORD) {
		_oss_out (b);
		return;
	}
	if (siz == 0) { // copied for direct output
		bu = form (NULL, "%s", b);
		if (bu)
			_oss_out (bu);
		return;
	}

	// b[0] doesn't have to be msg_type; if not, must be off the enum
	switch (b[0]) {
	    case msg_report:
		bt = board_out (b + sizeof(msgReportType)); 
		bu = form (NULL, "%s Report #%u tag %u->%u rss %u ago %u "
		    "btyp %u plev %u alrm %u.%u fl %u try %u: %s\r\n",
			in_header(b, rcv) == local_host ? 
				"IN" : "OUT",
			in_report(b, ref), in_report(b, tagid),
			in_header(b, rcv) == local_host ?
				in_header(b, snd) : 0,
			in_report(b, rssi), in_report(b, ago),
			((pongDataType *)(b + sizeof(msgReportType)))->btyp,
			((pongDataType *)(b + sizeof(msgReportType)))->plev,
			((pongDataType *)(b + sizeof(msgReportType)))->alrm_id,
			((pongDataType *)(b + sizeof(msgReportType)))->alrm_seq,
			((pongDataType *)(b + sizeof(msgReportType)))->fl2,
			((pongDataType *)(b + sizeof(msgReportType)))->trynr,
			bt ? bt : "?");
		ufree (bt);
		_oss_out (bu);
		break;
	    case msg_reportAck:
		bu = form (NULL, "RepAck #%u tag %u (%u+%u)\r\n",
			in_reportAck(b, ref), in_reportAck(b, tagid),
			tagList.alrms, tagList.evnts);
		_oss_out (bu);
		break;
	    default:
		app_diag_U ("dupa unfinished?");
	}
}

