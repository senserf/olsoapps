/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "app.h"
#include "msg_rtag.h"
#include "net.h"

// FIXIT: do something...
#define DEF_PLEV	0

nid_t   net_id;

// on the lib_app_if interface:
const lword	host_id = 0xBACA0001;
lint	master_delta = 0;
appCountType app_count = {0, 0, 0};
//nid_t	net_id  = 7;
word	app_flags = DEFAULT_APP_FLAGS;
word	pow_level = DEF_PLEV;
word    beac_freq = 0;
byte	cyc_ctrl = 0;
byte	cyc_ap = 0;
word	cyc_sp = 0;
char * cmd_line  = NULL;
cmdCtrlType cmd_ctrl = {0,0,0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

int beacon (word, address); // process
int cyc_man (word, address); // process

char * get_mem (word state, int len);
void send_msg (char * buf, int size);

// let's not include _if on any _if implementation, but list all externs:
extern nid_t local_host;
extern void msg_master_out (word state, char** buf_out, nid_t rcv);
//extern void msg_info_out (word state, char** buf_out, nid_t rcv);

// cycle alignement is done to arbitrary mod 4 == 0 on the master's clock
#define CS_INIT 	00
#define CS_ACT		10
#define CS_SLEEP	20
process (cyc_man, void*)
	word align4;
	nodata;

	entry (CS_INIT)
		// jic, shouldn't happen
		if (cyc_ap == 0 && cyc_sp == 0)
			kill (0);

		align4 = 4 - (((word)(master_delta + seconds())) & 0x0003);
		if (align4 != 0) {
			delay (align4 << 10, CS_ACT);
			release;
		}

	entry (CS_ACT)
		if (cyc_ctrl & CYC_SLEEPING) {
			cyc_ctrl &= ~CYC_SLEEPING;
			clockup();
			powerup();
			net_opt (PHYSOPT_RXON, NULL);
			net_opt (PHYSOPT_TXON, NULL);
		}
		if (cyc_ctrl & CYC_AU) // active unit set <=> in minutes
			ldelay (cyc_ap, CS_SLEEP);
		else
			delay ((word)cyc_ap << 10, CS_SLEEP);
		release;

	entry (CS_SLEEP)
		net_opt (PHYSOPT_RXOFF, NULL);
		net_opt (PHYSOPT_TXOFF, NULL);
		powerdown();
		clockdown();
		cyc_ctrl |= CYC_SLEEPING;
		if (cyc_ctrl & CYC_SU)
			ldelay (cyc_sp, CS_ACT);
		else
			delay (cyc_sp << 10, CS_ACT);
		release;

endprocess
#undef CS_INIT
#undef CS_ACT
#undef CS_SLEEP

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
			// better kill master's beacon if cyc changes
			in_master(data, mtime) = wtonl(seconds());
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

	if (net_tx (NONE, buf, size, 0 /* encr_data */) == 0) {
		app_count.snd++;
		//diag ("Sent %u to %u",
		//	in_header(buf, msg_type),
		//	in_header(buf, rcv));
	} else
		diag ("Tx %u failed", in_header(buf, msg_type));
 }

