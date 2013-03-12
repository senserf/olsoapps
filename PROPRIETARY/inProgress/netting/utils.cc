/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012        			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "app.h"
#include "app_dcl.h"
#include "msg_dcl.h"
#include "oss_dcl.h"
#include "net.h"
#include "storage.h"
#if defined BOARD_WARSAW_BLUE && ! defined __SMURPH__
#include "ser.h"
#endif


char * get_mem (word state, word len) {
	char * buf = (char *)umalloc (len);
	if (buf == NULL) {
		diag ("RAM out");

		if (state != WNONE) {
			umwait (state);
			release;
		}
#if 0
	       	else
			reset(); // is THIS good for ANY praxis?
#endif	
	} else
		memset (buf, 0, len); // in most cases, this is needed

	return buf;
}

#define DEF_PLEV	7
void init () {
	word pl;

	master_host = 0; // DEF_MHOST
	net_id =  12345; // DEF_NID
	local_host = (word)host_id;

	// tarp_ctrl.param = 0xA3 in tarp.h; level 2, rec 2, slack 1, fwd on
	// inited in tarp

	(void)fim_read();

	if (net_init (INFO_PHYS_CC1100, INFO_PLUG_TARP) < 0) {
		// diag issued from failing net_init diag ("net_init");
		halt();
	}

// FIXME: gerry and bolutek
// #if defined BOARD_WARSAW_BLUE && ! defined __SMURPH__ && ! defined BT_MODULE_BOLUTEK
#if defined BOARD_WARSAW_BLUE && ! defined __SMURPH__
	ser_select (1);
#endif

	tarp_ctrl.param = fim_set.f.tparam;
	pl = fim_set.f.polev;
	if (fim_set.f.rx)
		net_opt (PHYSOPT_RXON, NULL);
	else
		net_opt (PHYSOPT_RXOFF, NULL);

	net_opt (PHYSOPT_SETSID, &net_id);
	net_opt (PHYSOPT_SETPOWER, &pl);
	runfsm rcv;
	runfsm ossi_in;
}

word fim_read() {
	word i;
	for (i = 0; i < IFLASH_SIZE; i++)
		if (if_read (i) == 0xFFFF)
			break;

	if (i > 0) {
		fim_set.w = if_read (--i);
		fim_set.f.stran = 0; // not FIM, just the flag's placeholder
		fim_set.f.ofmt = 0;  // same
		return i;
	}

	// let's put defaults in
	fim_set.f.tparam = tarp_ctrl.param;
	fim_set.f.polev = DEF_PLEV;
	fim_set.f.rx = 1;
	fim_set.f.stran = 0;
	fim_set.f.ofmt = 0;
	return IFLASH_SIZE;
}

word fim_write() {
	word i;
	fim_t f;

	for (i = 0; i < IFLASH_SIZE; i++)
		if (if_read (i) == 0xFFFF)
			break;

	if (i == IFLASH_SIZE) {
		if_erase (-1);
		return fim_write();
	}

	if (i > 0) {
		f.w = if_read (i-1);
		if (f.f.tparam == fim_set.f.tparam &&
			f.f.polev == fim_set.f.polev &&
			f.f.rx == fim_set.f.rx)
			return i-1;
	}
	if_write (i, fim_set.w);
	return i;
}

