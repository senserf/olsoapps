/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "app.h"
#include "diag.h"
#include "msg_rtag.h"
#include "net.h"

#if CHIPCON
#define DEF_PLEV	9
#else
#define DEF_PLEV	0
#endif

// on the lib_app_if interface:
id_t	host_id = 0xBACA;
long   master_delta = 0;
appCountType app_count = {0, 0, 0};
word   pow_level = DEF_PLEV;
word    beac_freq = 0;

int beacon (word, address); // process
char * get_mem (word state, int len);
void send_msg (char * buf, int size);
word check_passwd (lword p1, lword p2);

// let's not include _if on any _if implementation, but list all externs:
extern id_t local_host;
extern void msg_master_out (word state, char** buf_out, id_t rcv);
extern void msg_info_out (word state, char** buf_out, id_t rcv);

#define BS_ITER 00
#define BS_SEND 10
process (beacon, char)
	static word len = 0;

	entry (BS_ITER)
		if (beac_freq == 0) {
			app_diag (D_UI, "Beacon off");
			ufree (data);
			kill (0);
		}
		delay (beac_freq, BS_SEND);
		release;

	entry (BS_SEND)
		switch (in_header(data, msg_type)) {
		  case msg_info:
			msg_info_out (NONE, &data, in_header(data, rcv));
			send_msg (data, sizeof(msgInfoType));
			break;

		  case msg_master:
			msg_master_out (NONE, &data, in_header(data, rcv));
			send_msg (data, sizeof(msgMasterType));
			break;

		  case msg_trace:
			send_msg (data, sizeof(msgTraceType));
			break;

		  case msg_rpc:
			// for the kludge's origin, see oss_rpc_in
			if (len == 0) {
				len = (word)in_header(data, snd);
				in_header(data, snd) = local_host;
			}
			send_msg (data, len);
			break;

		  default:
			app_diag (D_WARNING, "Beacon failure");
			beac_freq = 0;
		}
		proceed (BS_ITER);
endprocess (1)
#undef	BS_ITER
#undef	BS_SEND

word check_passwd (lword p1, lword p2) {
	const lword host_passwd = 0x0000BACA; // we'll se how it goes
	if (host_passwd == p1)
		return 1;
	if (host_passwd == p2)
		return 2;
	app_diag (D_WARNING, "Failed passwd");
	return 0;
}

char * get_mem (word state, int len) {
	char * buf = (char *)umalloc (len);
	if (buf == NULL) {
		app_diag (D_WARNING, "Waiting for memory");
		umwait (state);
	}
	return buf;
}

void send_msg (char * buf, int size) {
	// it doesn't seem like a good place to filter out
	// local host, but it's convenient, for now...

	// this shouldn't be... WARNING to see why it is needed...
	if (in_header(buf, rcv) == local_host) {
		app_diag (D_WARNING, "Dropped msg(%u) to lh",
			in_header(buf, msg_type));
		return;
	}

	if (net_tx (NONE, buf, size) == 0) {
		app_count.snd++;
		app_diag (D_DEBUG, "Sent %u to %lu",
			in_header(buf, msg_type),
			in_header(buf, rcv));
	} else
		app_diag (D_SERIOUS, "Tx %u failed",
			in_header(buf, msg_type));
 }

