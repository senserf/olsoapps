#include "sysio.h"
#include "oss.h"
#include "plug_null.h"

// Test praxis for the OSS framework

#define	UART_PHY	0
#define	UART_PLUG	0

// Praxis-specific OSS commands
#define	OSS_CODE_TIME	0
#define	OSS_CODE_ECHO	1
#define	OSS_CODE_DELAY	2
#define	OSS_CODE_RESET	3
#define	OSS_CODE_DIAG	4

fsm root {

	state START:

		int sfd;
		word w;

		// This is a pretty much standard prelude for setting up UART
		// for this kind of operation
		phys_uart (UART_PHY, OSS_MAX_PLEN, UART_A);
		tcv_plug (UART_PLUG, &plug_null);
		if ((sfd = tcv_open (WNONE, UART_PHY, UART_PLUG)) < 0)
			syserror (ENODEVICE, "uart");
		// Required to get rid of station ID
		w = 0xffff;
		tcv_control (sfd, PHYSOPT_SETSID, &w);
		// Not needed, UART starts in the ON state
		// tcv_control (sfd, PHYSOPT_TXON, NULL);
		// tcv_control (sfd, PHYSOPT_RXON, NULL);
		ab_init (sfd);

		// Not needed, this is the default; the node should be PASSIVE
		// ab_mode (AB_MODE_PASSIVE);

		// Start the driver
		runfsm oss_handler;
	
		finish;
}

static	word RQ_State = 0;

word oss_request (word st) {
//
// A simple demonstration of request handler:
//
//	0 - tell time (the number of seconds since reset)
//	1 - echo
//	2 - stop watch: hold for the given number of milliseconds
//	3 - reset the node
//	4 - write a diag message
//
	byte *buf;
	word bufl, CMD;

	switch (CMD = oss_cmd (OSS_bufw)) {

		case OSS_CODE_TIME:

			if ((buf = (byte*) umalloc (bufl = 6)) == NULL) {
				// Now we have two choices:
				//	1. wait for memory
				umwait (st);
				release;
				// 	2. return "try again later"
				// 		oss_clear ();
				// 		return OSS_STAT_LATER;
				// Note that in the latter case we have to do
				// oss_clear () to deallocate the buffer. The
				// request handler is responsible for that!!!
			}

			// Fill the buffer with stuff
			*((word*)buf) = oss_hdr (CMD, OSS_STAT_OK);
			*((lword*)(buf + 2)) = seconds ();

			// Replace the buffer
			oss_clear ();
			OSS_buf = buf;
			OSS_bufl = bufl;
			// Return value irrelevant as OSS_buf != NULL
			return 0;

		case OSS_CODE_ECHO:

			// Just echo the command payload
			return 0;

		case OSS_CODE_DELAY:

			if (RQ_State) {
				// Delay expired
				RQ_State = 0;
				goto Rtn_OK;
			}

			// Set up the alarm clock
			if (OSS_bufl < 4) {
				// Error
				oss_clear ();
				return OSS_STAT_ERR;
			}

			RQ_State = 1;
			delay (OSS_bufw [1], st);
			release;

		case OSS_CODE_RESET:

			reset ();

		case OSS_CODE_DIAG:

			diag ((char*) (OSS_buf + 2));
Rtn_OK:			oss_clear ();
			return OSS_STAT_OK;
	}

	// Unimplemented; we should be prepared for this, so we can at least
	// deallocate the buffer

	oss_clear ();
	return OSS_STAT_UNIMPL;
}
