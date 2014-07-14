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

typedef struct statsStruct {
	word	type :8;
	word	len	 :8;
	word	nid;
	word	mid;
	word	up_l;
	word	up_h;
	word	mem;
	word	min;
} stats_t; // 14B

static void stats () {
	word mmin, mem;
	stats_t * b = (stats_t *)get_mem (sizeof(stats_t), NO);
	if (b == NULL)
		return;

	mem = memfree(0, &mmin);
	b->type = 0xD1;
	b->len = 14 + 3;
	b->nid = local_host;
	b->mid = master_host;
	b->up_h = (word)(seconds() >> 16);
	b->up_l = (word)seconds();
	b->mem = mem;
	b->min = mmin;

	_oss_out ((char *)b, NO);
}

static void sack (byte req, word add, Boolean ok) {
	char *b;

	if ((b = get_mem (5, NO)) == NULL)
		return;

	b [0] = (req | 0x80);
	b [1] = 5+3;
	
	((address)b) [1] = add;
	
	b [4] = ok ? 0x06 : 0x15;
	
	_oss_out (b, YES);
}

static void reply11 (char *pkt) {

	char *b;
	word len;

	len = tcv_left ((address)pkt);

	if (len < 6 || (b = get_mem (8, NO)) == NULL) {
		app_diag_S ("rep11 %u", len);
		return;
	}

	b [0] = 0x91;
	b [1] = 8+3;

	((address)b) [1] = ((address)pkt) [1];
	((address)b) [2] = ((address)pkt) [2];
	switch (((address)b) [2]) { // will be more
		case 0x0002:
			((address)b) [3] = (local_host == master_host);
			break;
		case 0x0003:
			((address)b) [3] = (tarp_ctrl.param & 1);
			break;
		default:
			((address)b) [3] = local_host;
	}
	
	_oss_out (b, YES);
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

		if ((l = tcv_left ((address)ib)) < 5) { // what do we do? ack?
			app_diag_S ("cmdlen %u", l);
			tcv_endp ((address)ib);
			proceed LISN;
		}
		
		if (osync.type) { // only (n)acks allowed
			if ((osync.type | 0x80) == (byte)ib[0]) {
				trigger (TRIG_OSSIN);
			} else {
				app_diag_S ("badack %x", ((word)osync.type << 8) | *ib);
			}
			tcv_endp ((address)ib);
			proceed LISN;
		}
		
		// NOTE that with (n)ack pending, we won't get here
		switch ((byte)(*ib)) {
			case 0x11:
				reply11 (ib);
				break;

			case 0x12:
				if (ib[1] != 11) {
					sack (0x12, 0, NO);
					break;
				}
				switch (((address)ib)[2]) {
					case 0x0001:
						if (((address)ib)[3]) {
							local_host = ((address)ib)[3];
							sack (0x12, ((address)ib)[1], YES);
						} else {
							sack (0x12, ((address)ib)[1], NO);
						}
						break;
						
					case 0x0002:
						// ack on bad, as requested
						if (local_host != ((address)ib)[3]) {
							sack (0x12, ((address)ib)[1], YES);
							break;
						}
						
						if (local_host != master_host) {
							master_host = local_host;
							tarp_ctrl.param = 0xB0;
							tagList.block = YES;
							reset_tags();
							killall (looper);
							runfsm mbeacon;
							app_diag_W ("I am M");
#ifdef MASTER_STATUS_LED
							leds (MASTER_STATUS_LED, 2);
#endif
						}
						// apparently, it comes to the master for a twisted fun
						sack (0x12, ((address)ib)[1], YES);
						break;

					case 0x0003:
						if (((address)ib)[3])
							tarp_ctrl.param |= 1;
						else
							tarp_ctrl.param &= 0xFE;
							
						sack (0x12, ((address)ib)[1], YES);
						break;
						
					case 0x0004:
						// in case sb does need to send mbeacon (out of spec)
						if (local_host == master_host) {
							if (running (mbeacon)) {
								trigger (TRIG_MBEAC);
							} else {
								runfsm mbeacon;
							}
							sack (0x12, ((address)ib)[1], YES);
						} else {
							sack (0x12, ((address)ib)[1], NO);
						}

					default:
						sack (0x12, ((address)ib)[1], NO);
				}
				break;

			case 0x41:
			case 0x42:
			case 0x43:
				if (((address)ib)[1] != local_host) {
				    l = ib[1] -3 + sizeof(msgFwdType);

				    if ((b = get_mem (l, NO)) != NULL) {
						memset (b, 0, l);
						in_header(b, msg_type) = msg_fwd;
						in_header(b, rcv) = *((word *)(ib +2));
						in_fwd(b, ref) = (word)seconds();
						memcpy (b+sizeof(msgFwdType), ib, l - sizeof(msgFwdType));
						talk (b, l, TO_NET); // not to stubborn renensas i/f
						// send data to emul for tests in vuee
						app_diag_U ("fwd %x to %u #%u", ((address)ib)[0],
							in_header(b, rcv), in_fwd(b, ref));
						ufree (b);
						sack (ib[0], ((address)ib)[1], YES);
						break;
				    }
				}
				sack (ib[0], ((address)ib)[1], NO); // will error code be invented?
				break;

			default: // ?
				// ignore? ack? nack?
				app_diag_W ("ign cmd %x", ((address)ib)[0]);
				// if we ever get there, we'll have a 0xD. dispatch here
		}
		tcv_endp ((address)ib);
		proceed LISN;
}
///////////////////////////////

#define _ppp	(p + sizeof(pongDataType))
static void board_out (char * p, char * b) {

	// for boards other tham ap319, 320: just for fun
	switch (((pongDataType *)p)->btyp) {
		case BTYPE_CHRONOS:
		case BTYPE_CHRONOS_WHITE:
			b[11] = (byte)((((pongPloadType0 *)_ppp)->volt - 1000) >> 3);
			b[9] = (byte)((pongPloadType0 *)_ppp)->move_ago; // vy not
			b[14] = (byte)((pongPloadType0 *)_ppp)->move_nr;
			break;

		case BTYPE_AT_BASE:
			b[11] = (byte)((((pongPloadType2 *)_ppp)->volt - 1000) >> 3);
			b[9] = 9;
			b[14] = 14;
			break;
			
		case BTYPE_AT_BUT6:
			b[9] = ((pongPloadType3 *)_ppp)->glob;
			b[11] = (byte)((((pongPloadType3 *)_ppp)->volt - 1000) >> 3);
			b[14] = ((pongPloadType3 *)_ppp)->dial;
			break;

		case BTYPE_AT_BUT1:
			b[9] = 1;
			b[11] = (byte)((((pongPloadType4 *)_ppp)->volt - 1000) >> 3);
			b[14] = 0;
			break;
			
		case BTYPE_WARSAW:
			b[9] = (byte)((pongPloadType5 *)_ppp)->random_shit;
			b[11] = (byte)((((pongPloadType5 *)_ppp)->volt - 1000) >> 3);
			b[14] = (byte)((pongPloadType5 *)_ppp)->steady_shit;
			break;

		default:
			app_diag_W ("btyp %u", ((pongDataType *)p)->btyp);
	}
}
#undef _ppp

// this is the main out on the oss i/f, I don't know at all if this makes
// more sense than no generic i/f at all
// NO parameter sanitization
void oss_tx (char * b, word siz) {
	char *bu;

	// I'm not sure (now) what to do with these 2 spec. cases
	// likely, we'll need a dbg i/f; for now dump to emul

	// allocated, freed after output
	if (siz == MAX_WORD) {
		app_diag_U (b);
		ufree (b);
		return;
	}
	if (siz == 0) { // copied for direct output
		app_diag_U (b);
		return;
	}

	// b[0] doesn't have to be msg_type; if not, must be off the enum
	switch (b[0]) {
	
		case msg_report:
			
			if ((bu = get_mem (16, NO)) == NULL)
				return;
			
			bu[0] = 0x01;
			bu[1] = 16+3;
			((address)bu)[1] = local_host;
			((address)bu)[2] = in_header(b, snd) == 0 ? local_host : in_header(b, snd);
			((address)bu)[3] = in_report(b, tagid);
			bu[8] = ((pongDataType *)(b + sizeof(msgReportType)))->alrm_id;
			// board specific bu[9] = global flag
			bu[10] = ((pongDataType *)(b + sizeof(msgReportType)))->alrm_seq;
			// board-specific bu[11] = voltage
			bu[12] = in_report(b, rssi);
			bu[13] = ((pongDataType *)(b + sizeof(msgReportType)))->plev;
			// board-specific bu[14] = dial
			board_out (b + sizeof(msgReportType), bu); // sets board-specifics
			bu[15] = in_report(b, ago);

			_oss_out (bu, NO);
			break;

	    case msg_reportAck:
		// renesas doesn't want to see it
		
			app_diag_U ("repAck #%u t%u l%u.%u", in_reportAck(b, ref),
				in_reportAck(b, tagid), tagList.alrms, tagList.evnts);
			break;

	    case msg_fwd:
			if (in_header(b, snd) == 0 || in_header(b, snd) == local_host) {
				app_diag_S ("fwd??");
				break;
			}
			
			if ((bu = get_mem (*(b+sizeof(msgFwdType)+1) -3, NO)) == NULL) {
				app_diag_S ("fwdlen %u", (word)*(b+sizeof(msgFwdType)+1-3));
				return;
			}
			
			memcpy (bu, b+sizeof(msgFwdType), *(b +sizeof(msgFwdType) +1) -3);
			// bu[0] |= 0x80; not wanted
			((address)bu)[1] = in_header(b, snd);
			
			_oss_out (bu, YES);
			break;

	    case msg_fwdAck:
		// renesas doesn't want to see it
		
			app_diag_U ("fwdAck #%u fr%u", in_fwdAck(b, ref),
				in_header(b, snd));
			break;

	    default:
			app_diag_S ("unfinished? %x %u", *(address)b, siz);
	}
}

