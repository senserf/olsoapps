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

static byte *oss_outmsg (word st, byte code, word len) {
//
// Allocates an outgoing message of type code, length len
//
	byte *buf;
	buf = abb_outf (st, len);
	buf [0] = code;
	return buf;
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

	OSS_buf [0] = OSS_CODE_MTN;
	OSS_buf [1] = OSS_CODE_MTN_MD;

	memcpy (OSS_buf + 2, add, OSS_bufl - 2);
}

#endif

fsm oss_handler {

	word ws, wa;

	// ====================================================================
	// Upon startup, issue two "reset messages" to synchronize the two ends
	// ====================================================================

	state INIT_0:

		oss_outmsg (INIT_0, OSS_CODE_MTN, 2) [1] = OSS_CODE_MTN_RSA;

	state INIT_1:

		oss_outmsg (INIT_1, OSS_CODE_MTN, 2) [1] = OSS_CODE_MTN_RSB;

	// ====================================================================

	state LOOP:

		OSS_buf = abb_in (LOOP, &OSS_bufl);

		if (OSS_buf [0] == OSS_CODE_MTN) {

			switch (OSS_buf [1]) {

				case OSS_CODE_MTN_RSA:
				case OSS_CODE_MTN_RSB:

					// Resync request
					ufree (OSS_buf);
					proceed INIT_0;
#ifdef	OSS_MEMORY_ACCESS
				case OSS_CODE_MTN_MD:

					// Memory dump: expect address & length
					// in two words
					if (OSS_bufl < 6) {
						ws = 2;
						proceed RETSTAT;
					}
					wa = OSS_bufw [1];
					ws = OSS_bufw [2];
					ufree (OSS_buf);
					proceed MEMDUMP;

				case OSS_CODE_MTN_MS:

					// Memory set
					if (OSS_bufl < 5) {
						ws = 2;
						proceed RETSTAT;
					}

					memcpy (OSS_bufw [1], OSS_buf + 4,
						OSS_bufl - 4);

					ufree (OSS_buf);
					ws = 0;
					proceed RETSTAT;
#endif
			}

			// Ignore
			ws = 1;
			proceed RETSTAT;
		}

	state RQHANDLE:

		ws = oss_request (RQHANDLE);
		if (OSS_buf != NULL)
			// There is a specific reply
			proceed REPLY;

	state RETSTAT:

		// Return command status
		oss_outmsg (RETSTAT, OSS_CODE_STA, 2) [1] = (byte) ws;
		proceed LOOP;

	state REPLY:

		abb_out (REPLY, OSS_buf, OSS_bufl);
		proceed LOOP;

#ifdef	OSS_MEMORY_ACCESS

	state MEMDUMP:

		dump_memory (MEMDUMP, (address) wa, ws);
		proceed REPLY;
#endif

}
