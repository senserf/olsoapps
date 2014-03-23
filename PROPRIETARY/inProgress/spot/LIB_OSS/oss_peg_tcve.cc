/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2014                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "commons.h"
#include "vartypes.h"
#include "diag.h"
#include "tag_mgr.h"
#include "tarp.h"
#include "inout.h"
#include "tcvphys.h"
#include "phys_uart.h"

#include "tcve_insert.cc"

// note that field ids (fid_*) requested by AT don't make sense here
typedef struct statsStruct {
	word	type :8;
	word	len	 :8;
	word	nid;	// Node Address?
//	word	fid_mid;
	word	mid;
//	word	fid_up
	word	up_h;
	word	up_l;
//	word	fid_mem;
	word	mem;
	word	min;
} stats_t; // 14B

static void stats () {
	word mmin, mem;
	stats_t * b = (stats_t *)get_mem (sizeof(stats_t), NO);
	if (b == NULL)
		return;

	mem = memfree(0, &mmin);
	b->type = 0x91;
	b->len = 14 + 3;
	b->nid = local_host;
	b->mid = master_host;
	b->up_h = (word)(seconds() >> 16);
	b->up_l = (word)seconds();
	b->mem = mem;
	b->min = mmin;
	_oss_out ((char *)b);
}

static void byteout (byte typ, byte val) {
	char * cb = get_mem (5+3, NO);
	if (cb == NULL)
		return;
	cb[0] = typ;
	cb[1] = 8;
	((address)cb)[1] = local_host;
	cb[4] = val;
	_oss_out (cb);
}
	
///////////////// oss in ////////////////

/* no need for that here, but we could build appropriate structs here and
   call process_incoming() as fsm hear does for RF in.
*/

fsm mbeacon;
fsm looper;
fsm cmd_in {

	state START:
		stats ();

	state LISN: // LISTEN keyword??
		word l;
		char * b;
		char * ib = (char *) tcv_rnp (LISN, oss_fd);

		switch ((byte)(*ib)) {
			case 0x11:	// mutated 'get node'
				stats();
				break;

			case 0xFF: reset();

			case 0x12:	// I shortcut 'set node' to 'm' command
				if (local_host == master_host) {
					if (running (mbeacon)) {
					    trigger (TRIG_MBEAC);
					} else {
					    runfsm mbeacon;
					}
					byteout (0x12+0x80, 0x15);	// lets send nack here
				} else {
					master_host = local_host;
					tarp_ctrl.param = 0xB0;
					tagList.block = YES;
					reset_tags();
					killall (looper);
					runfsm mbeacon;
					byteout (0x12+0x80, 0x06); // and ack here
				}
				break;

			case 0x41:
			case 0x42:
			case 0x43:
				if (*((word *)(ib +2)) != local_host) {
				    l = ib[1] -3 + sizeof(msgFwdType);

				    if ((b = get_mem (l, NO)) != NULL) {
						memset (b, 0, l);
						in_header(b, msg_type) = msg_fwd;
						in_header(b, rcv) = *((word *)(ib +2));
						in_fwd(b, ref) = (word)seconds();
						memcpy (b+sizeof(msgFwdType), ib, l - sizeof(msgFwdType));
						talk (b, l, TO_ALL);
						ufree (b);
				    } else {
						byteout (0x12+0x80, 0x15);
					}
				} else {
					byteout (0x12+0x80, 0x15);
				}
				break;

			default: // ?
				// ignore acks, nacks, etc.
				app_diag_W ("ign cmd %x fr %u dat %x", ib[0], ((address)ib)[1], ib[4]); 
		}
		tcv_endp ((address)ib);
		proceed LISN;
}
///////////////////////////////


// this is the main out on the oss i/f, I don't know at all if this makes
// more sense than no generic i/f at all
// NO parameter sanitization
void oss_tx (char * b, word siz) {
	char *bu;
	Boolean incoming;

	// I'm not sure (now) what to do with these 2 spec. cases FIXME dupa
	// likely, we'll need a dbg i/f

	// allocated, freed after output
	if (siz == MAX_WORD) {
		app_diag_U (b);
		ufree (b);
		return;
	}
	if (siz == 0) { // copied for direct output
		app_diag_U (b);
		if (bu)
			app_diag_U (b);
		return;
	}

	// b[0] doesn't have to be msg_type; if not, must be off the enum
	switch (b[0]) {
		case msg_report:
			incoming = in_header(b, rcv) == local_host ? YES : NO;
			bu = get_mem (siz + 4 - sizeof(headerType), NO);
			if (bu == NULL)
				return;
				
			memcpy (bu +4, b +sizeof(headerType), siz - sizeof(headerType));
			bu[0] = 0x01;
			bu[1] = siz + 4 - sizeof(headerType) +3;
			((address)bu)[1] = in_header(b, snd);
			_oss_out (bu);
			break;

	    case msg_reportAck:
			bu = get_mem (4+2+2+1+1, NO);
			if (bu == NULL)
				return;
			bu[0] = 0x81;
			bu[1] = 11;
			((address)bu)[1] = in_reportAck(b, tagid);
			((address)bu)[2] = in_fwd(b, ref);
			bu[6] = tagList.alrms;
			bu[7] = tagList.evnts;
			_oss_out (bu);
			break;

	    case msg_fwd:
			incoming = in_header(b, snd) != 0 &&
				in_header(b, snd) != local_host;
			
			bu = get_mem (*(b+sizeof(msgFwdType) +1) -3, NO);
			if (bu == NULL)
				return;
				
			memcpy (bu, b+sizeof(msgFwdType), *(b +sizeof(msgFwdType) +1) -3);
			
			if (incoming)
				((address)bu)[1] = in_header(b, snd);
			else { // ok, I replace node addr with ref#
				bu[0] += 0x80;
				((address)bu)[1] = in_fwd(b, ref);
			}
			_oss_out (bu);
			break;

	    case msg_fwdAck:
			bu = get_mem (6, NO);
			if (bu == NULL)
				return;
			bu[0] = 0x44; // I've made it up
			bu[1] = 9;
			((address)bu)[1] = in_header(b, snd);
			((address)bu)[2] = in_fwd(b, ref);
			_oss_out (bu);
			break;

	    default:
			app_diag_S ("dupa unfinished?");
	}
}

