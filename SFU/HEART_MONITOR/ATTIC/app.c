/* ============================================================= */
/* Copyright (C) Olsonet Communications, 2007                    */
/* All rights reserved.                                          */
/* ============================================================= */

#include "sysio.h"
#include "tcvphys.h"

heapmem {100};

#include "form.h"

#include "phys_cc1100.h"
#include "phys_uart.h"
#include "plug_null.h"

#define	ESN	0x80000001

#define	MAXPLEN	60

#define	PK_LENGTH(tp)	(tp) == PT_HELLO) ? 10 : \

#define	PK_NETID(p)	(*(p))			// First word = netid
#define	PK_CMND(p)	(((byte*)(p)) [2])	// Command code
#define	PK_SERNUM(p)	(((byte*)(p)) [3])	// Request serial number

int	USFD, RSFD,	// Session IDs UART / RF
	BSFD = NONE;	// Which one is bound

byte	Status;

word	PLen;

#define	MYFD	((int)data)

// ============================================================================

strand (listener, int)

    address packet;
    word len;
    byte cmd;

    entry (LI_INIT)

	// This is where we start listening. The first thing we should do
	// is to bind ourselves somehwere.

	if (BSFD == MYFD)
		// We are bound already
		proceed (LI_GETCMD);

	if (BSFD == NONE) {
		// We are not bound at all. Send a HELLO packet. We don't do
		// that if we are bound on the other interface, but we listen
		// on this one in case somebody wants to re-bind us here.
		packet = tcv_wnp (LI_INIT, MYFD, PK_LENGTH (PT_HELLO));
		PK_NETID (packet) = 0;
		PK_TYPE (packet) = PT_HELLO;
		PK_SERNUM (packet) = 0;
		PK_ESN (packet) = ESN;
		tcv_endp (packet);
	}

    entry (LI_WBIND)

	// Keep waiting
	delay (BIND_INTERVAL, LI_INIT);
	packet = tcv_rnp (LI_WBIND, MYFD);
	if (Busy || ((len = tcv_left (packet) - 2) < 4 || 
	    PK_CMND (packet) != PT_BIND || pkget_32 (packet, 4) != ESN) {
		tcv_endp (packet);
		proceed (LI_WBIND);
	}

	// Bind
	BSFD = MYSFD;
	// NetId
	len = pkget_16 (packet, 8);
	tcv_control (MYFD, PHYSOPT_SETSID, &len);
	APID = pkget_32 (packet, 10);
	LCmd = 0;




// Need a private data structure
// Acking process? Send Ack here with number






    entry (LI_GETCMD)

	// This is the main request acquisition loop
	if (BSFD != MYFD)
		// We have lost our binding
		proceed LI_INIT;

	packet = tcv_rnp (LI_GETCMD, MYFD);
	// Sanity check first
	if ((len = tcv_length (packet) - 2) < 4) {
GC_ign:
		tcv_endp (packet);
		proceed (LI_GETCMD);
	}

	cmd = PKT_CMND (packet);
	// Another quick sanity check
	if ((cmd & 0x01))
		// Command addressed to an access point
		goto GC_ign;

	// Check if BIND, UNBIND, if busy, force abort and carry out




	// Check if our AP







	if ((PK_SERNUM (packet) == LCmd) {
		// Old serial number: send ACK

	

	

	



	




	







	// Only some commands are available while busy
	



	

	







    entry (LI_WAIT)

	packet = tcv_rnp (LI_WAIT, MYFD);
	len = tcv_left (packet) - 2;
	if (len < 4 || ((cmd = PK_CMD (packet)) & 0x01)) {
		// Too short or addressed to an access point
		tcv_endp (packet);
		proceed (LI_WAIT);
	}

	

	





	if (len > 32)
		len = 32;

	((char*)packet) [len] = '\0';
	lcd_clear (0, 0);
	lcd_write (0, (char*)(packet+1));

	tcv_endp (packet);
	proceed (LI_WAIT);

endstrand

// ============================================================================

thread (root)

#define	RS_INIT		0

    word scr;

    entry (RS_INIT)

	phys_cc1100 (0, MAXPLEN);
	phys_uart (1, MAXPLEN, 0);
	tcv_plug (0, &plug_null);

	RSFD = tcv_open (NONE, 0, 0);
	USFD = tcv_open (NONE, 1, 0);

	scr = 0;
	tcv_control (RSFD, PHYSOPT_SETSID, &scr);
	tcv_control (USFD, PHYSOPT_SETSID, &scr);

	runstrand (binder, USFD);

	runstrand (listener, USFD);
	runstrand (listener, RSFD);

	// We are not needed any more
	finish;

endthread
