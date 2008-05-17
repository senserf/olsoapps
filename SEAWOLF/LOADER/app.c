#include "sysio.h"
#include "tcvphys.h"
#include "tcvplug.h"
#include "board_rf.h"

//+++ "hostid.c"

heapmem {100};

#include "form.h"
#include "phys_cc1100.h"
#include "ser.h"
#include "serf.h"

#include "params.h"
#include "neighbors.h"
#include "images.h"

void *DHook = NULL;	// Common data hook for transaction-related structures

byte RQType,		// Request type
     RQFlag;		// And flags (generally object specific)

// ============================================================================
// 
// We have two interfaces, but only one request is allowed to be handled at a
// time. Meaning, if we are receiving an object on one interface, we cannot be
// receiving or sending an object on the other interface, nor can we be sending
// on object on the same interface. Thus, there is a global busy lock to 
// indicate our activity to the other thread.
//
// ============================================================================
static int RSFD = NONE,	// Radio interface SID
	   USFD = NONE;

// Array of object plug-ins indexed by object types
const static fun_pak_t *OPLUGS [] = { img_pak, imgl_pak, neil_pak };

#define	NOFTYPES	(sizeof (OPLUGS) / sizeof (fun_pak_t*))

#include "plug.h"

// Interface ID with a pending reauest or a request in progress. When something
// is requested (one thing at a time), the requestor sets this to the FD of the
// responsible listener. This remains set for as long as the request is being
// processed; thus, it is used by the other listener as a busy indicator.
static int FDRQ = NONE;

static byte Retries,		// Retry counter for persistent queries
	    MRQN = 0xff,	// My request number, cannot be zero
	    YRQN;		// Request number of the other party

// Parameters of the outstanding request: only one can be present at a time
static word YLink;		// Link ID od the other party
	    OType,		// Object type
	    OID;		// Object Id
static int  IIF = NONE;		// Issuer's interface

static	fun_pak_t *FPack;	// Function package for current transaction

#define	FUN_LKP_SND(n)		(FPack->fun_lkp_snd) (n)
#define	FUN_RTS_SND(p)		(FPack->fun_rts_snd) (p)
#define	FUN_CLS_SND(p)		(FPack->fun_cls_snd) (p)
#define	FUN_CNK_SND(p)		(FPack->fun_cnk_snd) (p)

#define	FUN_INI_RCP(p)		(FPack->fun_ini_rcp) (p)
#define	FUN_STP_RCP()		(FPack->fun_stp_rcp) ()
#define	FUN_CNK_RCP(p,s)	(FPack->fun_cnk_rcp) (p,s)
#define	FUN_CLS_RCP(p)		(FPack->fun_cls_rcp) (p)

#define	BUSY	(FDRQ != NONE)

// ============================================================================

void *alloc_dhook (word size) {
//
// Re-allocate the data hook
//
	if (DHook != NULL)
		ufree (DHook);

	DHook = umalloc (size);

	return DHook;
}

static void send_back (byte co, int ifa) {
//
// Send a kick packet
//
	address packet;
	word status;
	byte pt;

	// This is not meant as a reliable indication; merely intended to give
	// the issuer a kick

	if ((packet = tcv_wnp (NONE, ifa, (co == OSS_DONE ? PSIZE_BACK_D :
	    PSIZE_BACK)) == NULL)
		// Don't go out of your way
		return;

	packet [0] = MCN;
	pt = PT_OSS;
	put1 (packet, pt);
	put1 (packet, co);

	if (co == OSS_DONE) {
		images_status (packet);
		neighbors_status (packet);
	}
	tcv_endp (packet);
}

static void send_nak (word code, word lnk, byte rq, int ifa) {
//
// Send a NAK
//
	address packet;
	byte pt;

	if ((packet = tcv_wnp (NONE, ifa, PSIZE_NAK)) == NULL)
		return;

	packet [0] = lnk;
	pt = PT_NAK;
	put1 (packet, pt);
	put1 (packet, rq);
	put2 (packet, code);
	tcv_endp (packet);
}

static void clear_request (Boolean ok) {
//
// Called at the end of a transaction to clear the request
//
	if (DHook != NULL) {
		ufree (DHook);
		DHook = NULL;
	}

	FDRQ = NONE;

	if (IIF != NONE) {
		// There was an external issuer
		send_back (ok ? OSS_DONE : OSS_FAILED, IIF);
		IIF = NONE;
	}

	RQType = 0;
	trigger (&RQType);
}

// ============================================================================

static Boolean send_setup (address packet) {
//
// Check if have the requested object and set things up for transmission
//
	word otype, oid;

	get2 (packet, otype);
	get2 (packet, oid);

	if (otype >= NOFTYPES)
		return NO;

	FPack = OPLUGS [otype];

	return FUN_LKP_SND (oid);
}

// ============================================================================
//
// Chunk handlers
//
#define	ctally_missing(ct,cn)	((ct)->cstat [(cn) >> 3] &  (1 << ((cn) & 7)))
#define ctally_update(ct)	do { while ((ct)->f < (ct)->n && \
					!ctally_missing (ct, (ct)->f)) \
						(ct)->f++; } while (0)
#define ctally_add(ct,cn)	((ct)->cstat [(cn) >> 3] |= (1 << ((cn) & 7)))
#define	ctally_ctsize(nc)	(((nc)+7) >> 3)

Boolean croster_init (croster_t *cr, address packet) {
//
// Read the chunk list from the packet and initialize for processing
//
	word	nc;

	nc = tcv_left (packet);

	if (nc > MAXCHUNKLIST)
		// For sanity
		nc = MAXCHUNKLIST;
	else if (nc < 2)
		// No chunks
		return NO;

	memcpy ((byte*)(cr->list), (byte*)(packet+2), nc);
	cr->n = (nc >> 1);
	cr->p = 0;
	return YES;
}

word croster_next (croster_t *cr) {
//
// Advance to next chunk
//
	word ci, cn;

	if (cr->p >= cr->n)
		// No more chunks
		return WNONE;

	if (cr->p == cr->n - 1 || cr->list [cr->p] < cr->list [cr->p+1]) {
		// This means: the last entry in the list or the next entry is
		// bigger than the current one, i.e., a single chunk indicated
		// by this entry
		return cr->list [(cr->p)++];
	}
	// A range: keep incrementing the second entry until it equals first

	cn = cr->list [ci = cr->p+1];
	if (cn == cr->list [cr->p])
		// The end of range
		cr->p += 2;
	else
		// Increment
		cr->list [ci] ++;
	return cn;
}

void ctally_init (ctally_t *ct, word nc) {
//
// Initializes the chunk tally
//
	ct->n = nc;
	ct->f = 0;

	// The number of bytes
	nc = (nc + 7) >> 3;

	while (nc)
		ct->cstat [--nc] = 0;
}

word ctally_fill (ctally_t *ct, address p) {
//
// Fills the requested chunk list (if p != NULL), returns the length of the
// chunk list in words
//
	word cl, cn, cm;

	ctally_update (ct);

	if (ct->f == ct->n)
		return 0;

	cn = ct->f;
	cl = 0;

	while (1) {
		// If we are here, cn is missing; check the next one
		if (cn+1 == ct->n || !ctally_missing (ct, cn+1)) {
			// No range
			if (p)
				*p++ = cn;
			cl++;
			if (cl == MAXCHUNKS)
				return MAXCHUNKS;
			// Skip the next one, which is present
			cn++;
		} else {
			// A range
			if (cl == MAXCHUNKS-1) {
				// The last one to fill, range won't fit
				if (p)
					*p = cn;
				return MAXCHUNKS;
			}
			// cn+1 is missing as well, keep scanning the range
			cm = cn + 2;
			while (cm < ct->n && ctally_missing (ct, cm))
				cm++;

			if (p) {
				*p++ = cm-1;
				*p++ = cn;
			}
			cl += 2;
			cn = cm + 1;
		}
		// Locate next missing
		do {
			if (cn >= ct->n)
				return cl;
		} while (!ctally_missing (ct, cn));
	}
}

Boolean ctally_full (ctally_t *ct) {

	ctally_update (ct);

	return (ct->f >= ct->n);
}

// ============================================================================

typedef	struct {

	int 	fd;
	address	packet;

} listener_data_t;

strand (listener, listener_data_t)

    address packet;
    word wsc, wsd;
    byte cmd, req;
    Boolean cnd;

#define MYFD 	(data->fd)
#define	PACKET	(data->packet)

    entry (LI_IDLE)

	if (FDRQ == MYSFD) {
		// Looks like a request to us
		if (RQType == 0) {
			// Sanity check, this means something is wrong
			FDRQ = NONE;
		} else {
			// This can only be GET at present: handle it
			if (++MRQN == 0)
				MRQN = 1;
			proceed (LI_GET);
		}
	}

	// Make sure to send HELLO at pertinent intervals
	delay (INTV_HELLO, LI_HELLO);

	// Expect requests
	when (&FDRQ, LI_IDLE);

	// And try to receive stuff at your leisure
	PACKET = tcv_rnp (LI_IDLE, MYFD);

	// Packet type
	get1 (PACKET, cmd);

	switch (cmd) {

	    case PT_HELLO:

		if (MYFD == RSFD)
			// Only if via RF
			hello_in (PACKET);
		break;

	    case PT_OSS:

		if (PACKET [0] == MCN)
			oss_in (PACKET, MYFD);
		break;

	    case PT_WTR:

		if (tcv_left (PACKET) != PSIZE_WTR - 3)
			// Subtract Link id and cmd (CRC doesn't count)
			// Illegal packet ignore
			break;
			
		// Request number of the sender
		get1 (PACKET, req);

		// This should be our channel number
		get2 (PACKET, wsd);
		if (wsd != MCN)
			// It isn't - just ignore
			break;

		if (BUSY) {
			wsd = NAK_BUSY;
			goto PT_wtr_nak;
		}

		if (send_setup (PACKET)) {
			tcv_endp (PACKET);
			FDRQ = MYFD;
			proceed (LI_SEND);
		}

		// Object not found (we may add other reasons later)
		wsd = NAK_NF;
PT_wtr_nak:
		// Link Id of the sender
		wsc = PACKET [0];
		tcv_endp (PACKET);
		send_nak (wsd, wsc, req, MYFD);
		proceed (LI_IDLE);

	    // ================================================================
	    // No other packets are legitimate in this state. We may add some
	    // negative ACKs for such packets later.
	    // ================================================================

	}

	tcv_endp (PACKET);
	proceed (LI_IDLE);

// ============================================================================
// SEND OBJECT ================================================================
// ============================================================================

    entry (LI_SEND)

	// Send the OK (RTS) message
	Retries = MAXTRIES;

    entry (LI_SEND_PT)

	// Get here after timeout
	if (Retries == 0) {

BTIF:		// Back To Idle (fail)
		clear_request (NO);
		proceed (LI_IDLE);
	}

	Retries--;

    entry (LI_SEND_POLL)

	PACKET = tcv_wnp (LI_SEND_POLL, MYFD, PSIZE_RTS + FUN_RTS_SND (NULL));
	PACKET [0] = YLink;		// Recipient's link number
	put1 (PACKET, PT_RTS);		// Paket type: Ready To Send
	put1 (PACKET, YRQN);		// Recipient's request number

	// Fill in object parameters; note that the RTS packet length
	// generally depends on the object type
	FUN_RTS_SND (PACKET);

	tcv_endp (PACKET);

	delay (INTV_REPLY, LI_SEND_PT);

    entry (LI_SEND_WR)

	// Wait for a reply: it should be GO with a list of chunks
LI_send_wr:

	PACKET = tcv_rnp (LI_SEND_WR, MYFD);

	// Check if this is the correct reply
	get1 (PACKET, cmd);
	get1 (PACKET, req);
	// The other party uses their Link Id in the reply
	if (PACKET [0] == YLink && req == YRQN) {
		// A packet related to this request
		if (req != PT_GO) {

// End of packet + back to idle (fail)
EPBTIF:
			tcv_endp (PACKET);
			goto BTIF;
		}

		// This is a valid GO, unpack the list of chunks to send; note
		// the format of the list of chunks is object-type independent
		if (FUN_CLS_SND (PACKET)) {
// End of packet + send chunks
EPSCH:
			tcv_endp (PACKET);
			// Send object chunks
			proceed (LI_SEND_CH);
		} else {
			// All done
EPBTIK:	// End of Packet + Back To Idle (OK)
			tcv_endp (PACKET);
BTIK:	// Back To Idle OK
			clear_request (YES);
			proceed (LI_IDLE);
		}
	}
	// Ignore it and keep waiting
	tcv_endp (PACKET);
	snooze (LI_SEND_PT);
	goto LI_send_wr;

    entry (LI_SEND_CH)

	// Send the requested chunks, one at a time
	wsc = FUN_CNK_SND (NULL);
	if (wsc) {
		PACKET = tcv_wnp (LI_SEND_CH, MYFD, PSIZE_CHK + wsc);
		PACKET [0] = YLink;
		put1 (PACKET, PT_CHUNK);
		put1 (PACKET, YRQN);
		// This one also advances to the next chunk
		FUN_CNK_SND (PACKET);
		tcv_endp (PACKET);
		delay (INTV_CHUNK, LI_SEND_CH);
		release;
	}

	// Last chunk has been sent, keep sending an empty chunk until
	// acknowledged

	Retries = MAXTRIES;

    entry (LI_SEND_CT)

	// Get here after timeout
	if (Retries == 0)
		goto BTIF;

	Retries--;

    entry (LI_SEND_EC)

	PACKET = tcv_wnp (LI_SEND_EC, MYFD, PSIZE_CHK);
	PACKET [0] = YLink;
	put1 (PACKET, PT_CHUNK);
	put1 (PACKET, YRQN);
	tcv_endp (PACKET);

	delay (INTV_REPLY, LI_SEND_CT);

    entry (LI_SEND_WA)

	// Wait for a GO
LI_send_wa:

	PACKET = tcv_rnp (LI_SEND_WA, MYFD);

	get1 (PACKET, cmd);
	get1 (PACKET, req);

	if (PACKET [0] == YLink && req == YRQN) {
		// A packet related to this request
		if (req != PT_GO)
			// Almost ss good as a final ack
			goto EPBTIF;

		// Any more chunks to send?
		if (FUN_CLS_SND (PACKET))
			goto EPSCH;
		else
			goto EPBTIK;
	}
	// Ignore it and keep waiting
	tcv_endp (PACKET);
	snooze (LI_SEND_CT);
	goto LI_send_wa;

// ============================================================================
// GET OBJECT =================================================================
// ============================================================================

    entry (LI_GET)

	// Request to retrieve an object

	Retries = MAXTRIES;

    entry (LI_GET_PT)

	// Poll timeout?
	if (Retries == 0)
		goto BTIF;

	Retries--;

    entry (LI_GET_POLL)

	PACKET = tcv_wnp (LI_GET_POLL, MYFD, PSIZE_WTR);
	// ID of the other party
	PACKET [0] = MCN;
	cmd = PT_WTR;	// Want To Receive
	put1 (cmd);
	put1 (MRQN);	// Request number
	put2 (YLink);	// Recipient's channel Id
	put2 (OType);
	put2 (OID);	// Object coordinates
	tcv_endp (PACKET);

	delay (INTV_REPLY, LI_GET_PT);

    entry (LI_GET_WR)

LI_get_wr:

	PACKET = tcv_rnp (LI_GET_WR, MYFD);

	get1 (PACKET, cmd);
	get1 (PACKET, req);

	if (PACKET [0] == MCN && req == MRQN) {
		if (req != PT_RTS)
			// Reject
			goto EPBTIF;
		// Extract object params and the number of chunks
		cnd = FUN_INI_RCP (PACKET);
		tcv_endp (PACKET);
		if (cnd)
			proceed (LI_GET_CH);
		else
			// Don't want it after all
			proceed (LI_REJECT);
	}

	// Ignore and keep waiting
	tcv_endp (PACKET);
	snooze (LI_GET_PT);
	goto Li_get_wr;

    entry (LI_GET_CH)

	Retries = MAXTRIES;

    entry (LI_GET_CT)

	if (Retries == 0) {
		// Cancel or stop
SRBTI:
		if (FUN_STP_RCP ())
			goto BTIK;
		else
			goto BTIF;
	}

	Retries--;

    entry (LI_GET_SL)

	// Length of the list of remaining chunks
	wsc = FUN_CLS_RCP (NULL);

	if (wsc == 0) {
		// All chunks received
		FUN_STP_RCP ();
		proceed (LI_GET_EN);
	}

	// Chunks still missing, prepare a GO packet with list of chunks
	PACKET = tcv_wnp (LI_GET_CH, MYFD, PSIZE_GO + wsc);
	PACKET [0] = MCN;
	cmd = PT_GO;
	put1 (PACKET, cmd);
	put1 (PACKET, MRQN);
	// Fill in the chunk list
	FUN_CLS_RCP (PACKET);
	tcv_endp (PACKET);

LI_get_mc:	// More chunks

	delay (INTV_REPLY, LI_GET_CT);

    entry (LI_GET_WC)

LI_get_wc:	// Keep waiting for a chunk packet

	PACKET = tcv_rnp (LI_GET_WC, MYFD);
	if (PACKET [0] != MCN) {
		// Not ours: ignore and keep waiting
LI_get_ig:
		tcv_endp (PACKET);
		snooze (LI_GET_CT);
		goto LI_get_wc;
	}

	get1 (PACKET, cmd);
	get1 (PACKET, req);
	if (req != MRQN)
		goto LI_get_ig;

	if (cmd != PT_CHUNK) {
		// Something wrong: abort
		tcv_endp (PACKET);
		goto SRBTI;
	}

	// This is an actual chunk to receive

    entry (LI_GET_CH)

	cnd = FUN_CNK_RCP (PACKET, LI_GET_CH);
	tcv_endp (PACKET);

	if (cnd)
		// More chunks to come
		goto LI_get_mc;
	else
		// End of round
		proceed (LI_GET_SL);

    entry (LI_GET_EN)

	// Complete reception: tell the other guy we are done
	Retries = NSTOPS;

    entry (LI_GET_EM)

	// Null GO packet
	PACKET = tcv_wnp (LI_GET_CH, MYFD, PSIZE_GO);
	PACKET [0] = MCN;
	cmd = PT_GO;
	put1 (PACKET, cmd);
	put1 (PACKET, MRQN);
	tcv_end (PACKET);

	if (--Retries) {
		// More
		delay (INTV_STOP, LI_GET_EM);
		release;
	}

	goto BTIK;

endstrand

// ============================================================================

static void oss_in (address packet, int ifa) {
//
// Handle OSS packets; a response to such a packet consists of at most
// one packet (type OSS) sent with the Link Id of this node.
//
	word ifc;
	byte tp;

	if (tcv_left (packet) == 0) {
Error:
		send_back (OSS_BAD, ifa);
		return;
	}

	get1 (packet, tp);

	switch (tp) {

	    case OSS_GETI:

		// Get image from another node
		if (tcv_left (packet) < 6)
			// Link, number, interface (0,1)
			goto Error;

		if (BUSY) {
			send_back (OSS_BUSY, ifa);
			return;
		}

		get2 (packet, YLink);
		get2 (packet, OID);
		get2 (packet, ifc);
		OType = OTYPE_IMAGE;
		// Incoming interface
		IIF = ifa;
		RQType = RQ_GET;
		if (ifc)
			FDRQ = USFD;
		else
			FDRQ = RSFD;

		trigger (&FDRQ);

		return;

	    case OSS_PING:
DRet:
		send_back (OSS_DONE, ifa);
		return;

	    case OSS_CLEAN:

		// Images
		get1 (packet, tp);
		if (tp != 0)
			images_clean (tp);

		// Neighbors
		get1 (packet, tp);
		if (tp != 0)
			neighbors_clean (tp);
		goto DRet;
	}

	send_back (OSS_UNIMPL, ifa);
}

// ============================================================================
// Generic transaction plug-in functions ======================================
// ============================================================================

word objl_rts_snd (address packet) {
//
// Send the parameters of the list
//
	word cchks;

	if (packet != NULL) {
		cchks = (olsd_size + CHUNKSIZE - 1) / CHUNKSIZE;
		// Number of chunks
		put2 (packet, cchks);
		// Actual (exact) size
		put2 (packet, olsd_size);
	}

	// Returns the packet length (the specific part)
	return 4;
}

Boolean objl_cls_snd (address packet) {
//
// Unpack the chunk list
//
	return croster_init (&olsd_ros, packet);
}

word objl_cnk_snd (address packet) {
//
// Handle the next outgoing chunk
//
	word cn;

	if (packet == NULL) {
		// Assumes that we are always called first with packet == NULL.
		// The chunk number acquired in the first turn is cached for the
		// second call
		if ((olsd_cch = croster_next (&olsd_ros)) == NONE)
			return (olsd_ccs = 0);

		// Calculate the chunk size
		cn = (olsd_cch + 1) * CHUNKSIZE;

		if (cn <= olsd_size) 
			olsd_ccs = CHUNKSIZE;
		else if ((cn = olsd_size - cn) >= CHUNKSIZE)
			// Something wrong
			olsd_ccs = 0;
		else
			olsd_ccs = CHUNKSIZE - cn;
		return olsd_ccs;
	}

	// Fill the packet

	cn = CHUNKSIZE * olsd_cch;
	put2 (packet, olsd_cch);
	memcpy ((byte*)(packet+3), olsd_cbf, olsd_ccs);
	return olsd_ccs;
}

Boolean objl_ini_rcp (address packet, olist_t **tl) {
//
// Initialize for reception
//
	word nc, sz;

	// Remove any old list that still lingers around
	if (*tl != NULL)
		ufree (*tl);

	// Determine how much memory we need
	get2 (packet, nc);
	get2 (packet, sz);

	// Allocate the structure for received list
	if ((*tl = (olist_t*) umalloc (sizeof (olist_t) + sz)) == NULL)
		// No way
		return NO;

	if (alloc_dhook (sizeof (old_r_t) + ctally_ctsize (nc)) == NULL) {
		// No way
		ufree (*tl);
		*tl = NULL;
		return NO;
	}

	ctally_init (&olrd_cta, nc);
	(*tl)->size = sz;

	return YES;
}

static Boolean objl_cnk_rcp (address packet, olist_t **tl) {
//
// Receive a list chunk
//
	word cn, ln, pt;

	if ((ln = tcv_left (packet)) < 4)
		// The minimum non-empty chunk packet is chunk number + 2 bytes
		return NO;

	ln -= 2;
	get2 (packet, cn);

	if (cn < olrd_cta.n) {
		// OK, ignore if out of range
		ctally_add (&olrd_cta, cn);
		// Make it foolproof regarding the size
		pt = cn * CHUNKSIZE;
		if (cn == olrd_cta.n - 1) {
			if (pt + ln > (*tl)->size)
				ln = (*tl)->size - pt;
		}
		memcpy ((*tl)->buf + pt, (byte*)(packet+2), ln);
	}

	return YES;
}

word objl_cls_rcp (address packet) {
//
// Return the length of the chunk list and (optionally) fill the packet
//
	return ctally_fill (&olrd_cta, packet) << 1;
}

static Boolean objl_stp_rcp (olist_t **tl) {
//
// Terminate reception
//
	if (ctally_full (&olrd_cta)) {
		// OK
		return YES;
	}

	ufree (*tl);
	*tl = NULL;
	return NO;
}

// ============================================================================

thread (root)

#define	RS_INIT		0

    word scr;
    listener_data_t *d;

    entry (RS_INIT)

	lcdg_on (0);
	lcdg_set (0, 0, 0, 0, 0);
	lcdg_clear (COLOR_BLACK);

	phys_uart (0, MAXPLEN, 0);
	phys_cc1100 (0, MAXPLEN);
	tcv_plug (0, &plug_sea);

	RSFD = tcv_open (NONE, 0, 0);
	USFD = tcv_open (NONE, 1, 0);

	if (RSFD < 0 || USFD < 0)
		syserror (ENODEVICE, "root");

	// Accept all, honor packet's SID
	scr = 0xffff;
	tcv_control (RSFD, PHYSOPT_SETSID, &scr);
	tcv_control (USFD, PHYSOPT_SETSID, &scr);

	tcv_control (RSFD, PHYSOPT_TXON, NULL);
	tcv_control (RSFD, PHYSOPT_RXON, NULL);
	scr = XMIT_POWER;
	tcv_control (RSFD, PHYSOPT_SETPOWER, &scr);

	tcv_control (USFD, PHYSOPT_TXON, NULL);
	tcv_control (USFD, PHYSOPT_RXON, NULL);

	// EEPROM check-in
	images_init ();

	d = (listener_data_t*) umalloc (sizeof (listener_data_t);
	d->fd = RSFD;
	runstrand (listener, d);

	d = (listener_data_t*) umalloc (sizeof (listener_data_t);
	d->fd = USFD;
	runstrand (listener, d);

	// Neighbor table ager
	runthread (nager);

	// We are not needed any more
	finish;

endthread
