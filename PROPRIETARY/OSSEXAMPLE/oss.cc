#include "sysio.h"
#include "oss.h"

byte	*OSS_buf;
word	OSS_bufl;

void oss_clear () {

	if (OSS_buf) {
		ufree (OSS_buf);
		OSS_buf = NULL;
	}
}

static void oss_outmsg (word st, word code, word status) {
//
// Allocates an outgoing message of type code, length len
//
	byte *buf;
	buf = abb_outf (st, 2);
	*((word*)buf) = oss_hdr (code, status);
}

#ifdef	OSS_MEMORY_ACCESS

static void dump_memory (word st, address add, word len) {
//
// Creates a memory dump message, len is the number of bytes, which must not
// exceed the maximum buffer length - 2 (for our header)
//
	OSS_bufl = (len > OSS_MAX_BLEN-2) ?  OSS_MAX_BLEN : len + 2;

	if ((OSS_buf = (byte*) umalloc (OSS_bufl)) == NULL) {
		umwait (st);
		release;
	}

	OSS_bufw [0] = oss_hdr (OSS_CODE_MEMDUMP, OSS_STAT_OK);

	memcpy (OSS_buf + 2, add, OSS_bufl - 2);
}

#endif

fsm oss_handler {

	word ws, wa, CMD;

	// ====================================================================
	// Upon startup, issue two "reset messages" to synchronize the line
	// ====================================================================
	state INIT_0:
		oss_outmsg (INIT_0, OSS_CODE_RSA, OSS_STAT_OK);

	state INIT_1:
		oss_outmsg (INIT_1, OSS_CODE_RSB, OSS_STAT_OK);

	// ====================================================================

	state LOOP:

		OSS_buf = abb_in (LOOP, &OSS_bufl);

		switch (CMD = oss_cmd (OSS_bufw)) {

			case OSS_CODE_RSA:

				// Resync 0 -> ignore completely
				oss_clear ();
				proceed LOOP;
					
			case OSS_CODE_RSB:

				// Resync 1 -> respond with resync 1; the
				// message is ready in OSS_buf
				proceed REPLY;
#ifdef	OSS_MEMORY_ACCESS
			case OSS_CODE_MEMDUMP:

				// Memory dump: expect address & length in two
				// words
				if (OSS_bufl < 6)
					proceed CLEAR_ERROR;

				wa = OSS_bufw [1];
				ws = OSS_bufw [2];
				oss_clear ();
				proceed MEMDUMP;

			case OSS_CODE_MEMSET:

				// Memory set
				if (OSS_bufl < 5)
					proceed CLEAR_ERROR;

				memcpy (OSS_bufw [1], OSS_buf + 4,
					OSS_bufl - 4);

				proceed CLEAR_OK;
#endif
		}

		// Process regular (praxis) requests

	state RQHANDLE:

		ws = oss_request (RQHANDLE);

		if (OSS_buf != NULL)
			// There is a specific reply
			proceed REPLY;

	state RETSTAT:

		oss_outmsg (RETSTAT, CMD, ws);
		proceed LOOP;

	state REPLY:

		abb_out (REPLY, OSS_buf, OSS_bufl);
		proceed LOOP;

	state CLEAR_ERROR:

		ws = OSS_STAT_ERR;
		oss_clear ();
		sameas RETSTAT;

	state CLEAR_OK:

		ws = OSS_STAT_OK;
		oss_clear;
		sameas RETSTAT;

#ifdef	OSS_MEMORY_ACCESS

	state MEMDUMP:

		dump_memory (MEMDUMP, (address) wa, ws);
		proceed REPLY;
#endif

}
