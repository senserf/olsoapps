/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "app.h"
#include "msg_rtag.h"
#include "net.h"

#if CC1000
#define DEF_PLEV	9
#else
#define DEF_PLEV	0
#endif

// on the lib_app_if interface:
lword	host_id = 0xBACA;
lint   master_delta = 0;
appCountType app_count = {0, 0, 0};
id_t	net_id  = 7;
word   pow_level = DEF_PLEV;
word    beac_freq = 0;
char * cmd_line  = NULL;
cmdCtrlType cmd_ctrl = {0, 0, 0, 0x00, 0x00, 0x00, 0x00};

int beacon (word, address); // process
char * get_mem (word state, int len);
void send_msg (char * buf, int size);

// let's not include _if on any _if implementation, but list all externs:
extern id_t local_host;
extern void msg_master_out (word state, char** buf_out, id_t rcv);
//extern void msg_info_out (word state, char** buf_out, id_t rcv);

#define BS_ITER 00
#define BS_SEND 10
process (beacon, char*)

	entry (BS_ITER)
		if (beac_freq == 0) {
			diag ("Beacon off");
			ufree (data);
			kill (0);
		}
		delay (beac_freq << 10, BS_SEND);
		release;

	entry (BS_SEND)
		switch (in_header(data, msg_type)) {

		  case msg_master:
			msg_master_out (NONE, &data, in_header(data, rcv));
			send_msg (data, sizeof(msgMasterType));
			break;

		  case msg_trace:
			send_msg (data, sizeof(msgTraceType));
			break;

		  default:
			diag ("Beacon failure");
			beac_freq = 0;
		}
		proceed (BS_ITER);
endprocess
#undef	BS_ITER
#undef	BS_SEND

char * get_mem (word state, int len) {
	char * buf = (char *)umalloc (len);
	if (buf == NULL) {
		diag ("Waiting for memory");
		umwait (state);
	}
	return buf;
}

void send_msg (char * buf, int size) {

	// this shouldn't be... WARNING to see why it is needed...
	if (in_header(buf, rcv) == local_host) {
		diag ("Dropped msg(%u) to lh",
			in_header(buf, msg_type));
		return;
	}

	if (net_tx (NONE, buf, size) == 0) {
		app_count.snd++;
		//diag ("Sent %u to %u",
		//	in_header(buf, msg_type),
		//	in_header(buf, rcv));
	} else
		diag ("Tx %u failed", in_header(buf, msg_type));
 }

