/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Uncomment diags to see exactly what happens !!!

#include "tcvplug.h"
#include "plug_ack.h"

// The size of the hooks table (we keep there the hooks to outstanding packets
// that have been transmitted and await acknowledgments)
#define	N_HOOKS		6

#define	PTYPE_ACK	0xFF	// Acknowledgement packet type
#define	PTYPE_NORMAL	0x00	// Data packet (everything other than 0xFF is
				// data)
#define	ACK_LENGTH	6
#define	MAX_RTIMES	8	// Max number of retransmissions for unacked
				// packets

#define	RETR_DELAY	1024	// Retransmission interval

static int tcv_ope_ack (int, int, va_list);
static int tcv_clo_ack (int, int);
static int tcv_rcv_ack (int, address, int, int*, tcvadp_t*);
static int tcv_frm_ack (address, int, tcvadp_t*);
static int tcv_out_ack (address);
static int tcv_xmt_ack (address);

//
// This is the timeout function of the plugin; it used to be NULL in our
// previous plugins; in order to use it, you must #define TCV_TIMERS 1;
// the plugin also assumes that you have #define TCV_HOOKS 1 (see options.sys)
//
static int tcv_tmt_ack (address);

trueconst tcvplug_t plug_ack =
		{ tcv_ope_ack, tcv_clo_ack, tcv_rcv_ack, tcv_frm_ack,
			tcv_out_ack, tcv_xmt_ack, tcv_tmt_ack,
				0x0011 /* Plugin Id */ };

//
// Assumes there is never more than one session (at a time); our old wisdom
// tells us to use an array here, but I am presently in the mood of eliminating
// features that have never been used; one thing that will probably go is the
// plugin ID (like 0x0011 above); has anybody ever used them for anything?
//
static int desc = NONE, phys = NONE;

static address hooks [N_HOOKS];

word   NFreeHooks = N_HOOKS;	// We keep track of how many free hooks we have;
				// this can even be used by the praxis, e.g.,
				// as a congestion indicator

static byte rtimes [N_HOOKS];	// Retransmission counters (per hook)

static int tcv_ope_ack (int phy, int fd, va_list plid) {

	int i;

	if (desc != NONE)
		return ERROR;

	desc = fd;
	phys = phy;

	return 0;
}

static int tcv_clo_ack (int phy, int fd) {

	if (desc != fd || phys != phy)
		return ERROR;

	desc = NONE;
	return 0;
}

static int tcv_rcv_ack (int phy, address p, int len, int *ses,
							     tcvadp_t *bounds) {
//
// Called at packet reception
//
	word i;
	address ap;

	if (desc == NONE || phy != phys)
		// We are closed or not our PHY
		return TCV_DSP_PASS;

	if (len == ACK_LENGTH && ptype (p) == PTYPE_ACK) {
		// This is an ACK: search the hooks for a matching data
		// packet
		// diag ("R-A %u %u", psernum (p), NFreeHooks);
		for (i = 0; i < N_HOOKS; i++) {
			if ((ap = hooks [i]) != NULL &&
			    psernum (ap) == psernum (p)) {
				// diag ("A-F %x %u", ap, i);
				// Drop the data packet as it has been
				// acknowledged
				tcv_drop (ap);
				hooks [i] = NULL;
				NFreeHooks++;	// One hook becomes free
				// The praxis may want to know
				trigger (&NFreeHooks);
				break;
			}
		}
		// An ACK is always dropped, so the praxis doesn't see it
		return TCV_DSP_DROP;
	}

	// Data packet; we shortcut and do the actual reception, instead of
	// just returning the disposition code at the end, because we want
	// to make sure that the packet has actually made it, which we will
	// not know until we have successfully stored in the a buffer, like
	// this:
	if ((ap = tcvp_new (len, TCV_DSP_RCV, desc)) == NULL) {
		// No room, no reception, no ACK
		// diag ("R-D %u %u", psernum (p), NFreeHooks);
		return TCV_DSP_DROP;
	}
	// This is also why we don't have to set sec or bounds: the caller
	// (tcvphy_rcv) will not get to actually using them, as we do its
	// job

	// Copy the payload
	memcpy (ap, p, len);
	
	// diag ("R-U %x %u %u", ap, psernum (p), NFreeHooks);

	// Create an ACK (quietly ignore, if no room)
	if ((ap = tcvp_new (ACK_LENGTH, TCV_DSP_XMTU, desc)) != NULL) {
		// diag ("R-K %x %u", ap, psernum (p));
		ptype (ap) = PTYPE_ACK;
		psernum (ap) = psernum (p);
		// Note that the disposition code, transmit urgent, is set by
		// tcvp_new, so we don't have to say anything else
	} else {
		// diag ("R-N %u", psernum (p));
		CNOP;
	}
	
	// Done, the disposition code says there is nothing more to be done
	// about the packet
	return TCV_DSP_DROP;
}

static int tcv_frm_ack (address p, int phy, tcvadp_t *bounds) {

	// As before
	return bounds->head = bounds->tail = 0;
}

static int tcv_out_ack (address p) {

	// As before: disposition for a "wnp" packet
	return TCV_DSP_XMT;

}

static int tcv_xmt_ack (address p) {
//
// Called when a packet has been transmitted by the PHY
//
	word i;

	if (tcvp_length (p) == ACK_LENGTH && ptype (p) == PTYPE_ACK) {
		// This is an ACK, just drop it, nothing more to be done
		// diag ("X-A %x %d", p, psernum (p));
		return TCV_DSP_DROP;
	}

	// This is a data packet
	// diag ("X-U %x %d", p, psernum (p));

	for (i = 0; i < N_HOOKS; i++) {
		// Check if pointed to by a hook
		if (hooks [i] == p) {
			// Found, check how many times transmitted so far
			// diag ("X-R %d", rtimes [i]);
			if (rtimes [i] == MAX_RTIMES) {
				// Max reached, remove from hooks and drop
				// diag ("X-M");
				// Note that the hooks entry will be cleared
				// automatically by TCV
				trigger (&NFreeHooks);
				NFreeHooks++;	// A hook becomes free
				hooks [i] = NULL;
				return TCV_DSP_DROP;
			}
			rtimes [i]++;	// Increment retransmission count
HoldIt:
			// Set the timer (the packet goes into the timer queue)
			// diag ("X-S %x", p);
			tcvp_settimer (p, RETR_DELAY);
			// Detach
			return TCV_DSP_PASS;
		}
	}

	// Not found in hooks, first time, find a free entry
	// diag ("X-N %d", NFreeHooks);
	for (i = 0; i < N_HOOKS; i++) {
		if (hooks [i] == NULL) {
			// Got it
			// diag ("X-H %x %d", p, i);
			// This hooks the packet (and sets the hook value)
			hooks [i] = p;
			// Initialize retransmission counter
			rtimes [i] = 0;
			// Tally the hook out
			NFreeHooks--;
			goto HoldIt;
		}
	}
	// diag ("X-O %x", p);
	
	// No room in hooks, the packet is dropped
	return TCV_DSP_DROP;
}

static int tcv_tmt_ack (address p) {
//
// Called when the packet's timer goes off; very simple: retransmit at high
// priority
//
	// diag ("X-T %x", p);
	return TCV_DSP_XMTU;
}
