#include "sysio.h"
#include "oss.h"
#include "plug_null.h"

// Test praxis for the OSS framework

#define	UART_PHY	0
#define	UART_PLUG	0

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

		// Not needed, this is the default
		// ab_mode (AB_MODE_PASSIVE);

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
//	2 - stop watch: hold for up to 16 sec and then return code OK
//	3 - reset the node
//	4 - write a diag message
//
	byte *buf;
	word bufl;

	switch (OSS_buf [0]) {

		case 0:

			if ((buf = (byte*) umalloc (bufl = 6)) == NULL) {
				// Now we have two choices:
				//	1. wait for memory
				umwait (st);
				release;
				// 	2. return "try again later"
				// 		oss_clear ();
				// 		return OSS_CODE_STA_LATER;
				// Note that in the latter case we have to do
				// oss_clear () to deallocate the buffer. The
				// request handler is responsible for that!!!
			}

			// Fill the buffer with stuff. Because the exchange is
			// highly assymetric, we may assume a simple convention
			// whereby responses use the same headers (command
			// codes) as commands.
			buf [0] = 0;
			buf [1] = 0;
			*((lword*)(buf + 2)) = seconds ();
			// Replace the buffer; don't forget to release the
			// request
			oss_clear ();

			OSS_buf = buf;
			OSS_bufl = bufl;
			return OSS_CODE_STA_OK;

		case 1:

			// Just echo the command, i.e., do nothing
			return OSS_CODE_STA_OK;

		case 2:

			if (RQ_State) {
				// Delay expired
				RQ_State = 0;
Rtn_OK:
				oss_clear ();
				return OSS_CODE_STA_OK;
			}

			// Set up the alarm clock
			if ((bufl = OSS_buf [1]) > 16)
				bufl = 16;

			RQ_State = 1;
			delay (bufl * 1024, st);
			release;

		case 3:
			reset ();

		case 4:

			diag ((char*) (OSS_buf + 2));
			goto Rtn_OK;
	}

	// Unimplemented; we should be prepared for that, so we can at least
	// deallocate the buffer

	oss_clear ();
	return OSS_CODE_STA_NOP;
}
