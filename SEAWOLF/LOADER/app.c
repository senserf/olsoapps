#include "sysio.h"
#include "tcvphys.h"
#include "tcvplug.h"
#include "board_rf.h"

//+++ "hostid.c"
//+++ "images.c"
//+++ "neighbors.c"

heapmem {100};

#include "phys_cc1100.h"
#include "phys_uart.h"

#include "params.h"
#include "neighbors.h"
#include "images.h"

void *DHook = NULL;	// Common data hook for transaction-related structures

byte RQType,		// Request type (for an object transaction request)
     RQFlag,		// And flags (generally object specific)
     SFlags,		// Status flags
     Retries,		// Retry counter for persistent queries

     MyORqN = 0xff,	// My request number for object requests
     MySRqN = 0xff,	// My request number for non-object queries

     YrORqN,		// Request number of the other party: object
     YrSRqN;		// non-object

static word YrOLink,	// Link Id of the other party
       	    YrSLink;

// ============================================================================
// 
// We have two interfaces, but only one request is allowed to be "active" at a
// time. Meaning: if we are receiving an object on one interface, we cannot be
// receiving or sending an object on the other interface, nor can we be sending
// receiving anothe object on the same interface. Thus, there is a global busy
// lock to indicate our activity to the other thread.
//
// ============================================================================
static int RSFD = WNONE,	// Radio interface SID
	   USFD = WNONE;	// UART SID

// Array of object plug-ins indexed by object types
const static fun_pak_t *OPLUGS [] = { &img_pak, &imgl_pak, &nei_pak };

#define	NOFTYPES	(sizeof (OPLUGS) / sizeof (fun_pak_t*))

#include "plug.h"

// Interface ID with a pending request or a request in progress. When something
// is requested (one thing at a time), the requestor sets this to the FD of the
// responsible listener. This remains set for as long as the request is being
// processed; thus, it can be used as a busy indicator.
static int FDRQ = WNONE;

static word OType,		// Object type
	    OID;		// Object Id

static int  IIF = WNONE;	// Issuer's interface

static	const fun_pak_t *FPack;	// Function package for current transaction

#define	FUN_LKP_SND(n)		(FPack->fun_lkp_snd) (n)
#define	FUN_RTS_SND(p)		(FPack->fun_rts_snd) (p)
#define	FUN_CLS_SND(p)		(FPack->fun_cls_snd) (p)
#define	FUN_CNK_SND(p)		(FPack->fun_cnk_snd) (p)

#define	FUN_INI_RCP(p)		(FPack->fun_ini_rcp) (p)
#define	FUN_STP_RCP()		(FPack->fun_stp_rcp) ()
#define	FUN_CNK_RCP(s,p)	(FPack->fun_cnk_rcp) (s,p)
#define	FUN_CLS_RCP(p)		(FPack->fun_cls_rcp) (p)

#define	LBUSY	(FDRQ != WNONE)

static void oss_in (address, int);

// OSS packet size table (-request type/ordinal)
const byte oss_psize [] = {
	 0, // PING
	 8, // GET
	 4, // QUERY	otw,oiw
	 2, // CLEAN	otw, [listw]
	 2, // SHOW
	 2, // LCDP	cmb,nab, [listb]
	 2, // BUZZ
	 2  // RFPARAM
#ifdef	DEBUGGING
,	 6, // DUMP	tpb,nbb,adl
	 8  // EE	frl,upl
#endif
};

// ============================================================================

#ifdef	DEBUGGING

static void send_ack (word, word, byte, int);

void diagg (const char *msg, word a, word b) {
//
// Send a debug packet
//
	word ln;
	address packet;
	byte bc;

	ln = strlen (msg) + 1;

	if ((ln & 1)) 
		ln++;
	
	if ((packet = tcv_wnpu (WNONE, USFD, ln + 8)) == NULL)
		return;

	bc = PT_DEBUG;
	put1 (packet, bc);
	// This means 'text message follows'
	bc = 0;
	put1 (packet, bc);
	put2 (packet, a);
	put2 (packet, b);
	strcpy ((char*) (packet + 4), msg);
	tcv_endp (packet);
}

void dump_mem (word ad, word n) {
//
// Dump n bytes of memory from ad
//
	address packet;
	byte bc;

	if ((n & 1))
		n++;

	if (n > MAXCHUNKLIST)
		n = MAXCHUNKLIST;

	if ((packet = tcv_wnp (WNONE, IIF, n + 4)) == NULL)
		return;

	bc = PT_DEBUG;
	put1 (packet, bc);
	// This means 'memory dump'
	bc = 1;
	put1 (packet, bc);
	memcpy ((char*) (packet + 2), (byte*)ad, n);
	tcv_endp (packet);
}

void dump_eeprom (lword ad, word n) {
// 
// The same with EEPROM
//
	address packet;
	byte bc;

	if ((n & 1))
		n++;

	if (n > MAXCHUNKLIST)
		n = MAXCHUNKLIST;

	if ((packet = tcv_wnp (WNONE, IIF, n + 4)) == NULL)
		return;

	bc = PT_DEBUG;
	put1 (packet, bc);
	// This means 'EEPROM dump'
	bc = 2;
	put1 (packet, bc);

	ee_read (ad, (byte*)(packet+2), n);
	tcv_endp (packet);
}

#endif

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

static void send_status () {
//
// Sends a status packet on the indicated interface
//
	address packet;
	word status;
	byte pt;

	if ((packet = tcv_wnpu (WNONE, IIF, PSIZE_STAT)) == NULL)
		// Don't go out of your way
		return;

	packet [0] = YrSLink;
	pt = PT_STAT;
	put1 (packet, pt);
	put1 (packet, YrSRqN);

	images_status (packet);
	neighbors_status (packet);
	tcv_endp (packet);
}

static void send_ack (word code, word lnk, byte rq, int ifa) {
//
// Send a NAK
//
	address packet;
	byte pt;

	// Make it an urgent packet to bump the queue size limit
	if ((packet = tcv_wnpu (WNONE, ifa, PSIZE_ACK)) == NULL)
		return;

	packet [0] = lnk;
	pt = PT_ACK;
	put1 (packet, pt);
	put1 (packet, rq);
	put2 (packet, code);
	tcv_endp (packet);
}

static void send_oss_ack (word code) {
//
// Send an ACK to an OSS request
//
	send_ack (code, YrSLink, YrSRqN, IIF);
}

static void clear_request (Boolean ok) {
//
// Called at the end of a transaction to clean up after the request
//
	if (DHook != NULL) {
		ufree (DHook);
		DHook = NULL;
	}

	FDRQ = WNONE;

	if (IIF != WNONE) {
		// There was an external issuer: this sends an ACK (positive
		// or negative) to an OSS request
		send_oss_ack (ok ? ACK_OK : ACK_FAILED);
		IIF = WNONE;
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

Boolean croster_init (croster_t *cr, address packet) {
//
// Read the chunk list from the packet and initialize for processing
//
	word	nc;

	nc = tcv_left (packet);

	if (nc > MAXCHUNKLIST)
		// For sanity
		nc = MAXCHUNKLIST;
	else if (nc < 2) {
		// No chunks
		// diagg ("CIN DONE", 0, 0);
		return NO;
	}

	memcpy ((byte*)(cr->list), (byte*)(packet+2), nc);
	cr->n = (nc >> 1);
	cr->p = 0;
	// diagg ("CIN", cr->n, cr->list [0]);
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

	if (ct->f >= ct->n)
		return 0;

	cn = ct->f;
	cl = 0;

	if (p)
		// Skip the packet header
		p += 2;

	while (1) {
		// If we are here, cn is missing; check the next one
		if (cn+1 == ct->n || ctally_present (ct, cn+1)) {
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
			cn++;
			if (cn >= ct->n)
				return cl;

		} while (ctally_present (ct, cn));
	}
}

Boolean ctally_full (ctally_t *ct) {

	ctally_update (ct);

	return (ct->f >= ct->n);
}

Boolean ctally_add (ctally_t *ct, word cn) {

	byte b;

	b = (1 << (cn & 7));
	cn >>= 3;
	if ((ct->cstat [cn] & b))
		return YES;
	ct->cstat [cn] |= b;
	return NO;
}

word ctally_absent (ctally_t *ct) {
//
// Count the missing chunks (for debugging)
//
	word wc, cn;

	for (cn = wc = 0; wc < ct->n; wc++)
		if (ctally_missing (ct, wc))
			cn++;

	return cn;
}
		
// ============================================================================

typedef	struct {

	int 	fd;
	address	packet;

} listener_data_t;

strand (listener, listener_data_t)

    word wsc, wsd;
    byte cmd, req;
    Boolean cnd;

#define MYFD 	(data->fd)
#define	PACKET	(data->packet)

#define	LI_IDLE		0
#define	LI_SEND		1
#define	LI_SEND_PT	2
#define	LI_SEND_POLL	3
#define	LI_SEND_WR	4
#define	LI_SEND_CH	5
#define	LI_SEND_CT	6
#define	LI_SEND_EC	7
#define	LI_SEND_WA	8
#define	LI_GET		9
#define	LI_GET_PT	10
#define	LI_GET_POLL	11
#define	LI_GET_WR	12
#define	LI_GET_CH	13
#define	LI_GET_CT	14
#define	LI_GET_SL	15
#define	LI_GET_WC	16
#define	LI_GET_CK	17
#define	LI_GET_EN	18
#define	LI_GET_EM	19
#define	LI_HELLO	20

    entry (LI_IDLE)

	if (FDRQ == MYFD) {
		// Looks like a request for us
		if (RQType == 0) {
			// Sanity check, this means something is wrong
			FDRQ = WNONE;
		} else {
			// This can only be GET at present: handle it
			if (++MyORqN == 0)
				MyORqN = 1;
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

		oss_in (PACKET, MYFD);
		break;

	    case PT_WTR:

		if (tcv_left (PACKET) != PSIZE_WTR - 1)
			// Subtract Link id and cmd (CRC doesn't count)
			// Illegal packet ignore
			break;
			
		// Request number of the sender
		get1 (PACKET, YrORqN);

		// This should be our channel number
		get2 (PACKET, wsd);
		if (wsd != MyLink)
			// It isn't - just ignore
			break;

		YrOLink = PACKET [0];

		if (LBUSY) {
			wsd = ACK_BUSY;
			goto PT_wtr_nak;
		}

		if (send_setup (PACKET)) {
			tcv_endp (PACKET);
			FDRQ = MYFD;
			proceed (LI_SEND);
		}

		// Object not found (we may add other reasons later)
		wsd = ACK_NOTFOUND;
PT_wtr_nak:
		tcv_endp (PACKET);
		send_ack (wsd, YrOLink, YrORqN, MYFD);
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

	// Send the RTS message
	Retries = MAXTRIES;

    entry (LI_SEND_PT)

	// Get here after timeout
	if (Retries == 0) {
		// diagg ("LI_SEND_PT RETRIES", 0, 0);
BTIF:		// Back To Idle (fail)
		clear_request (NO);
		proceed (LI_IDLE);
	}

	Retries--;

    entry (LI_SEND_POLL)

	PACKET = tcv_wnp (LI_SEND_POLL, MYFD, PSIZE_RTS + FUN_RTS_SND (NULL));
	PACKET [0] = YrOLink;		// Recipient's link number
	cmd = PT_RTS;
	put1 (PACKET, cmd);		// Paket type: Ready To Send
	put1 (PACKET, YrORqN);		// Recipient's request number

	// Fill in object parameters; note that the RTS packet length
	// generally depends on the object type
	FUN_RTS_SND (PACKET);
	tcv_endp (PACKET);

    entry (LI_SEND_WR)

	// Wait for a reply: it should be GO with a list of chunks

	delay (INTV_REPLY, LI_SEND_PT);

	PACKET = tcv_rnp (LI_SEND_WR, MYFD);
	if (PACKET [0] != YrOLink) {
		// Ignore and keep waiting
LI_send_wr_wait:
		tcv_endp (PACKET);
		proceed (LI_SEND_WR);
	}

	// Check if this is the correct reply
	get1 (PACKET, cmd);
	get1 (PACKET, req);

	if (req != YrORqN)
		// Wrong session
		goto LI_send_wr_wait;

	if (cmd == PT_GO) {
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
			// diagg ("LI_SEND_WR DONE", 0, 0);
EPBTIK:			// End of Packet + Back To Idle (OK)
			tcv_endp (PACKET);
BTIK:			// Back To Idle OK
			clear_request (YES);
			proceed (LI_IDLE);
		}
	}

	if (cmd == PT_WTR)
		// An old WTR
		goto LI_send_wr_wait;

	// Anything else counts as a NAK
	// diagg ("LI_SEND_WR UNEXP", cmd, 0);
	// End of packet + back to idle (fail)
EPBTIF:
	tcv_endp (PACKET);
	goto BTIF;

    entry (LI_SEND_CH)

	// Send the requested chunks
	wsc = FUN_CNK_SND (NULL);
	if (wsc) {
		PACKET = tcv_wnp (LI_SEND_CH, MYFD, PSIZE_CHK + 2 + wsc);
		PACKET [0] = YrOLink;
		cmd = PT_CHUNK;
		put1 (PACKET, cmd);
		put1 (PACKET, YrORqN);
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
	if (Retries == 0) {
		// diagg ("LI_SEND_CT RETRIES", 0, 0);
		goto BTIF;
	}

	Retries--;

    entry (LI_SEND_EC)

	PACKET = tcv_wnp (LI_SEND_EC, MYFD, PSIZE_CHK);
	PACKET [0] = YrOLink;
	cmd = PT_CHUNK;
	put1 (PACKET, cmd);
	put1 (PACKET, YrORqN);
	tcv_endp (PACKET);

    entry (LI_SEND_WA)

	// Wait for a GO
	delay (INTV_REPLY, LI_SEND_CT);

	PACKET = tcv_rnp (LI_SEND_WA, MYFD);

	get1 (PACKET, cmd);
	get1 (PACKET, req);

	if (PACKET [0] == YrOLink && req == YrORqN) {
		// A packet related to this request
		if (cmd != PT_GO) {
			// diagg ("LI_SEND_WA NO GO", 0, 0);
			// Almost as good as a final ack
			goto EPBTIF;
		}

		// Any more chunks to send?
		if (FUN_CLS_SND (PACKET)) {
			// diagg ("LI_SEND_WA MORE", 0, 0);
			goto EPSCH;
		} else {
			// diagg ("LI_SEND_WA NO MORE", 0, 0);
			goto EPBTIK;
		}
	}
	// Ignore it and keep waiting
	tcv_endp (PACKET);
	proceed (LI_SEND_WA);

// ============================================================================
// GET OBJECT =================================================================
// ============================================================================

    entry (LI_GET)

	// Request to retrieve an object

	Retries = MAXTRIES;

    entry (LI_GET_PT)

	// Poll timeout?
	if (Retries == 0) {
		// diagg ("LI_GET_PT RETRIES", 0, 0);
		goto BTIF;
	}

	Retries--;

    entry (LI_GET_POLL)

	PACKET = tcv_wnp (LI_GET_POLL, MYFD, PSIZE_WTR);
	// ID of the other party
	PACKET [0] = MyLink;
	cmd = PT_WTR;		// Want To Receive
	put1 (PACKET, cmd);
	put1 (PACKET, MyORqN);	// Request number
	put2 (PACKET, YrOLink);	// Recipient's channel Id
	put2 (PACKET, OType);
	put2 (PACKET, OID);	// Object coordinates
	tcv_endp (PACKET);

    entry (LI_GET_WR)

	delay (INTV_REPLY, LI_GET_PT);

	PACKET = tcv_rnp (LI_GET_WR, MYFD);

	get1 (PACKET, cmd);
	get1 (PACKET, req);

	if (PACKET [0] == MyLink && req == MyORqN) {
		if (cmd != PT_RTS) {
			// Reject
			// diagg ("LI_GET_WR NAK", cmd, 0);
			goto EPBTIF;
		}
		// Extract object params and the number of chunks
		cnd = FUN_INI_RCP (PACKET);
		tcv_endp (PACKET);
		if (cnd)
			proceed (LI_GET_CH);
		else {
			// diagg ("LI_GET_WR REJECT", 0, 0);
			// Don't want it after all
			send_ack (ACK_REJECT, MyLink, MyORqN, MYFD);
			goto BTIF;
		}
	}

	// Ignore and keep waiting
	tcv_endp (PACKET);
	proceed (LI_GET_WR);

    entry (LI_GET_CH)

	Retries = MAXTRIES;

    entry (LI_GET_CT)

	if (Retries == 0) {
		// Cancel or stop
		// diagg ("LI_GET_CT RETRIES", 0, 0);
SRBTI:
		if (FUN_STP_RCP ()) {
			// diagg ("LI_GET_CT OK", 0, 0);
			goto BTIK;
		} else { 
			// diagg ("LI_GET_CT FAIL", 0, 0);
			goto BTIF;
		}
	}

	Retries--;

    entry (LI_GET_SL)

	// Length of the list of remaining chunks
	wsc = FUN_CLS_RCP (NULL);

	if (wsc == 0) {
		// All chunks received
		// diagg ("LI_GET_SL: ALL RCVD", 0, 0);
		FUN_STP_RCP ();
		proceed (LI_GET_EN);
	}

	// Chunks still missing, prepare a GO packet with list of chunks
	PACKET = tcv_wnp (LI_GET_SL, MYFD, PSIZE_GO + wsc);
	PACKET [0] = MyLink;
	cmd = PT_GO;
	put1 (PACKET, cmd);
	put1 (PACKET, MyORqN);
	// Fill in the chunk list
	FUN_CLS_RCP (PACKET);
	tcv_endp (PACKET);

    entry (LI_GET_WC)

	// Keep waiting for a chunk packet
	delay (INTV_REPLY, LI_GET_CT);

	PACKET = tcv_rnp (LI_GET_WC, MYFD);

	if (PACKET [0] != MyLink) {
		// Not ours: ignore and keep waiting
LI_get_ig:
		tcv_endp (PACKET);
		proceed (LI_GET_WC);
	}

	get1 (PACKET, cmd);
	get1 (PACKET, req);
	if (req != MyORqN || cmd != PT_CHUNK) {
		// diagg ("LI_GET_WC IGNORED", cmd, req);
		goto LI_get_ig;
	}

	// This is an actual chunk to receive

    entry (LI_GET_CK)

	// The reception may involve blocking (say, when writing to EEPROM)
	cnd = FUN_CNK_RCP (LI_GET_CK, PACKET);
	tcv_endp (PACKET);

	if (cnd) {
		proceed (LI_GET_WC);
	} else {
		// diagg ("LI_GET_CK EOR", cmd, req);
		// End of round
		proceed (LI_GET_SL);
	}

    entry (LI_GET_EN)

	// Complete reception: tell the other guy we are done
	Retries = NSTOPS;

    entry (LI_GET_EM)

	// Null GO packet
	PACKET = tcv_wnp (LI_GET_CH, MYFD, PSIZE_GO);
	PACKET [0] = MyLink;
	cmd = PT_GO;
	put1 (PACKET, cmd);
	put1 (PACKET, MyORqN);
	tcv_endp (PACKET);

	if (--Retries) {
		// More
		delay (INTV_STOP, LI_GET_EM);
		release;
	}

	goto BTIK;

// ============================================================================
// HELLO ======================================================================
// ============================================================================

    entry (LI_HELLO)

	// Don't hang if memory short
	if ((PACKET = tcv_wnp (WNONE, MYFD, PSIZE_HELLO)) != NULL) {
		PACKET [0] = 0;
		cmd = PT_HELLO;
		req = 0;
		put1 (PACKET, cmd);
		put1 (PACKET, req);
		put4 (PACKET, ESN);
		tcv_endp (PACKET);
	}
	proceed (LI_IDLE);

endstrand

// ============================================================================

static void oss_in (address packet, int ifa) {
//
// Handle OSS packets; a response to such a packet consists of at most
// one packet (type OSS) sent with the Link Id of this node.
//
	lword ac, ad;

#define	cw	(* ((word*)(&ac))   )
#define	tp	(*(((byte*)(&ac))+3))
#define	nb	(*(((byte*)(&ac))+2))
#define	cv	(*(((word*)(&ac))+1))

	// Request number
	get1 (packet, tp);

	if (tcv_left (packet) < PSIZE_OSS-PSIZE_FRAME)
		// Ignore
		return;
		
	// Target link Id
	get2 (packet, cw);

	if (cw != MyLink)
		// Not for us, ignore
		return;

	if (LBUSY) {
		send_ack (ACK_BUSY, cw, tp, ifa);
		return;
	}

	// Initiate processing
	YrSRqN = tp;
	YrSLink = cw;
	IIF = ifa;

	get1 (packet, tp);
	// This is to even things up (ignored)
	get1 (packet, nb);

	if (tp >= sizeof (oss_psize)) {
Error:
		send_oss_ack (ACK_FORMAT);
		IIF = WNONE;
		return;
	}

	switch (tp) {

	    case OSS_GET:

		get2 (packet, YrOLink);
		get2 (packet, OType);
		get2 (packet, OID);

		// Interface selector
		get1 (packet, tp);

		if (OType > OTYPE_MAX)
			goto Error;

		// Check if this is an image that you already have
		if (OType == OTYPE_IMAGE && image_find (OID)) {
			send_oss_ack (ACK_ALREADY);
			return;
		}

		FPack = OPLUGS [OType];
		// Incoming interface
		RQType = RQ_GET;
		// Function set

		if (tp)
			FDRQ = USFD;
		else
			FDRQ = RSFD;

		// Request flags
		get1 (packet, RQFlag);
		trigger (&FDRQ);

		return;

	    case OSS_QUERY:

		get2 (packet, OType);
		get2 (packet, OID);

		if (OType > OTYPE_MAX)
			goto Error;

		tp = NO;

		switch (OType) {

		    case OTYPE_IMAGE:

			tp = image_find (OID);
			break;

		    case OTYPE_ILIST:

			tp = (OID == 0 || OID == 1 && images_haveilist ());
			break;

		    case OTYPE_NLIST:

			tp = (OID == 0 || OID == 1 && neighbors_havenlist ());
			break;
		}

		send_oss_ack (tp ? ACK_OK : ACK_FAILED);
Ret:
		IIF = WNONE;
		return;

	    case OSS_PING:

		send_status ();
		goto Ret;

	    case OSS_CLEAN:

		// Object type
		get2 (packet, OType);

		switch (OType) {

		    case OTYPE_IMAGE:

			images_clean (packet + 5, tcv_left (packet) >> 1);
			break;

		    case OTYPE_ILIST:

			images_cleanil ();
			break;

		    case OTYPE_NLIST:

			neighbors_clean (packet + 5, tcv_left (packet) >> 1);
			break;
		}
ORet:
		send_oss_ack (ACK_OK);
		goto Ret;
		
	    case OSS_SHOW:

		// Display image number i
		get2 (packet, cw);
		send_oss_ack (image_show (cw) ? ACK_OK : ACK_FAILED);
		goto Ret;

	    case OSS_LCDP:

		get1 (packet, tp);
		get1 (packet, nb);

		if (nb > tcv_left (packet)) 
			goto Error;

		lcdg_cmd (tp, (byte*)(packet+4), nb);
		goto ORet;

 	    case OSS_BUZZ:

		// To test the buzzer
		get2 (packet, cw);
		buzz (cw);
		goto ORet;

	    case OSS_RFPARAM:

		get1 (packet, tp);
		get1 (packet, nb);
		cw = nb;

		switch (tp) {

		    case 0:	// On/Off

			if (cw) {
				tcv_control (RSFD, PHYSOPT_TXON, NULL);
				tcv_control (RSFD, PHYSOPT_RXON, NULL);
			} else {
				tcv_control (RSFD, PHYSOPT_TXOFF, NULL);
				tcv_control (RSFD, PHYSOPT_RXOFF, NULL);
			}

			break;

		    case 1:	// Power

			tcv_control (RSFD, PHYSOPT_SETPOWER, &cw);
			break;

		    case 2:	// Channel

			tcv_control (RSFD, PHYSOPT_SETCHANNEL, &cw);
			break;

		    case 3:	// Rate

			tcv_control (RSFD, PHYSOPT_SETRATE, &cw);
			break;

		    case 4:	// Reset, argument ignored

			tcv_control (RSFD, PHYSOPT_RESET, NULL);
			break;

		    default:

			send_oss_ack (ACK_UNIMPL);
			goto Ret;
		}

		goto ORet;

#ifdef	DEBUGGING

	    case OSS_DUMP:

		// Memory dump
		get1 (packet, tp);
		get1 (packet, nb);
		get4 (packet, ad);

		if (tp) 
			dump_eeprom (ad, nb);
		else
			dump_mem ((word)ad, nb);
		goto Ret;

	    case OSS_EE:

		// EEPROM erase
		get4 (packet, ac);
		get4 (packet, ad);
		ee_erase (WNONE, ac, ad);
		goto ORet;
#endif
	}

	send_oss_ack (ACK_UNIMPL);
	goto Ret;

#undef	cw
#undef	tp
#undef	nb

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
		if ((olsd_cch = croster_next (&olsd_ros)) == WNONE)
			return (olsd_ccs = 0);

		// Calculate the chunk size
		cn = (olsd_cch + 1) * CHUNKSIZE;
		olsd_ccs = CHUNKSIZE;
		if (cn > olsd_size)
			olsd_ccs -= (cn - olsd_size);
		return olsd_ccs;
	}

	// Fill the packet

	cn = CHUNKSIZE * olsd_cch;
	put2 (packet, olsd_cch);
	memcpy ((byte*)(packet+3), olsd_cbf + cn, olsd_ccs);
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

Boolean objl_cnk_rcp (address packet, olist_t **tl) {
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
		memcpy ((*tl)->buf + pt, (byte*)(packet+3), ln);
	}

	return YES;
}

word objl_cls_rcp (address packet) {
//
// Return the length of the chunk list and (optionally) fill the packet
//
	return ctally_fill (&olrd_cta, packet) << 1;
}

Boolean objl_stp_rcp (olist_t **tl) {
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

thread (buttons_handler)

#define	BH_LOOP		0

    entry (BH_LOOP)

	if (PRESSED_BUTTON0) {
		// diagg ("BUT", 0, 0);
		images_show_next ();
	} else if (PRESSED_BUTTON1) {
		// diagg ("BUT", 1, 1);
        	images_show_previous ();
	}

	if (JOYSTICK_N) diagg ("JOY N", 0, 0);
	if (JOYSTICK_E) diagg ("JOY E", 0, 0);
	if (JOYSTICK_S) diagg ("JOY S", 0, 0);
	if (JOYSTICK_W) diagg ("JOY W", 0, 0);
	if (JOYSTICK_PUSH) diagg ("JOY PUSH", 0, 0);

	when (BUTTON_PRESSED_EVENT, BH_LOOP);
	release;

endthread

// ============================================================================

thread (root)

#define	RS_INIT		0

    word scr;
    listener_data_t *d;

    entry (RS_INIT)

	lcdg_on (0);
	lcdg_set (0, 0, 0, 0, 0);
	lcdg_clear (COLOR_WHITE);

	phys_cc1100 (0, MAXPLEN);
	phys_uart (1, MAXPLEN, 0);
	tcv_plug (0, &plug_sea);

	RSFD = tcv_open (WNONE, 0, 0);
	USFD = tcv_open (WNONE, 1, 0);

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

	d = (listener_data_t*) umalloc (sizeof (listener_data_t));
	d->fd = RSFD;
	runstrand (listener, d);

	d = (listener_data_t*) umalloc (sizeof (listener_data_t));
	d->fd = USFD;
	runstrand (listener, d);

	// Neighbor table ager
	runthread (nager);

	// Button service
	buttons_init ();
	runthread (buttons_handler);

	// We are not needed any more
	finish;

endthread
