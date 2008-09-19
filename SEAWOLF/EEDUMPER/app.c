#include "sysio.h"
#include "oep.h"
#include "oep_ee.h"
#include "plug_null.h"
#include "phys_uart.h"

/*
 * OEP-based flash (EEPROM) dumper and loader (illustrates how to use OEP)
 */

// ============================================================================

//+++ "hostid.c"
extern lword host_id;

#define	MLID	((word)host_id)

// ============================================================================

static int SFD;

address packet;
byte Status;

#define	PF_LID	(packet[0])
#define	PF_CMD	(((byte*)packet)[2])
#define	PF_RQN	(((byte*)packet)[3])
#define	PF_PAY	(packet+2)

#define	CMD_PING	0xF0
#define	CMD_SEND	0xF1
#define	CMD_RECV	0xF2
#define	CMD_ERAS	0xF3

#define	MINPL		6	// Minimum packet length (LID, HDR, CHS)

//
// Packet types (PF_CMD) [praxis -> OSS]:
//
// PING	(sent every second if idle): RQN = last received RQN, payload = ESN
// also in response to ERAS
//
//			 [OSS -> praxis]:
//
// SEND (RQN indicates transmission RQN for OEP, payload = 2 lwords FWA, LEN)
// RECV (receive using RQN from this command, payload = 2 lwords)
// ERAS (RQN identifies request, payload = FWA, LEN)
//

// ============================================================================

static void send_ping () {

	if ((packet = tcv_wnp (WNONE, SFD, MINPL + 6)) == NULL)
		return;

	PF_LID = MLID;
	PF_CMD = CMD_PING;
	PF_RQN = oep_getrqn ();

	*((lword*)PF_PAY) = host_id;
	*(PF_PAY+2) = (word) Status;

	tcv_endp (packet);
}

// ============================================================================

#define	LI_LOOP		0
#define	LI_WAIT		1
#define	LI_PING		2

thread (listener)

    entry (LI_LOOP)

	delay (2048, LI_PING);
	packet = tcv_rnp (LI_LOOP, SFD);

	// Commands between the OSS program and the praxis:
	//
	// 

	switch (PF_CMD) {

		case CMD_ERAS:

			if (PF_RQN == oep_getrqn ()) {
Ignore:
				// Send last RQN ping and ignore the packet
				tcv_endp (packet);
Done:
				send_ping ();
				proceed (LI_LOOP);
			}

			// A new erase request
			oep_setrqn (PF_RQN);
			Status = ee_erase (WNONE, ((lword*)PF_PAY) [0],
				((lword*)PF_PAY) [1]) ? OEP_STATUS_PARAM : 0;
			goto Done;

		case CMD_SEND:

			if (PF_RQN == oep_getrqn ())
				goto Ignore;
			// Use this RQN to send the stuff
			oep_setrqn (PF_RQN);
			Status = oep_ee_snd (PF_LID, oep_getrqn (),
				((lword*)PF_PAY) [0], ((lword*)PF_PAY) [1]);
ERq:
			tcv_endp (packet);
			if (Status != 0)
				// Failure
				goto Done;

			proceed (LI_WAIT);

		case CMD_RECV:

			if (PF_RQN == oep_getrqn ())
				goto Ignore;
			oep_setrqn (PF_RQN);
			Status = oep_ee_rcv (((lword*)PF_PAY) [0],
							((lword*)PF_PAY) [1]);
			goto ERq;
	}

	tcv_endp (packet);
	proceed (LI_LOOP);

    entry (LI_WAIT)

	Status = oep_wait (LI_WAIT);
	// Note: this is the single return point, no matter how the exchange
	// ends, assuming it has actually started
	oep_ee_cleanup ();

    entry (LI_PING)

	goto Done;

endthread

// ============================================================================

thread (root)

#define	RS_INIT		0

    word scr;

    entry (RS_INIT)

	phys_uart (0, OEP_MAXRAWPL, 0);
	tcv_plug (0, &plug_null);

	// OSS link
	SFD = tcv_open (WNONE, 0, 0);

	if (SFD < 0)
		syserror (ENODEVICE, "uart");

	// Initialize OEP
	if (oep_init () == NO)
		syserror (ERESOURCE, "oep_init");

	oep_setlid (MLID);

	// Initialize the PHY
	scr = 0xffff;
	tcv_control (SFD, PHYSOPT_SETSID, &scr);
	tcv_control (SFD, PHYSOPT_TXON, NULL);
	tcv_control (SFD, PHYSOPT_RXON, NULL);

	if (runthread (listener) == 0)
		syserror (ERESOURCE, "runthread listener");

	// We are not needed any more
	finish;

endthread
