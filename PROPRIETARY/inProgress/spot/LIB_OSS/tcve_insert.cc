/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2014                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

///////////////// oss out ////////////////
// this is consistent with ser oss but I think it would be only needed for
// 'immediate acks' and retries. Acks that require context (and have longer
// and non-blocking retries) would need additional buffer for inventory and
// audit. In here we don't retry, ignore acks, but keep fifek.
/////////////////////////////////////////////////////////////////

static fifek_t oss_cb;
static sint oss_fd;

// if we compile with the 2 #ifs switched off, we'll be overwriting circ. buf;
// i.e. old lines will be lost (instead of new ones).
static word _oss_out (char * b) {

	if (b == NULL)
		return 2;
#if 1
	if (!fifek_full (&oss_cb)) {
#endif
		fifek_push (&oss_cb, b);
		trigger (TRIG_OSSO);
		return 0;
#if 1
	}
	// we (arbitrary) decide: don't overwrite
	app_diag_S ("OSS oflow (%u)", (word)seconds());
	ufree (b);
	return 1;
#endif
}
 
fsm perp_oss_out () {
	char * ptr;
	address pkt;

	state CHECK:
		if (fifek_empty (&oss_cb)) {
			when (TRIG_OSSO, CHECK);
			release;
		}
		ptr = fifek_pull (&oss_cb);
		if (ptr[1] < 5) { // wrong len
			app_diag_S ("OSSO len %d", (sint)ptr[1]);
			ufree (ptr);
			proceed CHECK;
		}
		
	state WNP:
		pkt = tcv_wnp (WNP, oss_fd, (sint)ptr[1] -3);
		memcpy ((char *)pkt, ptr,  (sint)ptr[1] -3);
		tcv_endp (pkt);
		ufree (ptr);
		proceed (CHECK);
}
#undef FIFEK_SIZ

/////////////////

fsm cmd_in;
void oss_ini () {

	phys_uart (1, 80, 0); // 80B, not used anywhere else(?)

	// we should be returning, the caller blinking, whatever
	if ((oss_fd = tcv_open (WNONE, 1, 0)) < 0) {
		app_diag_F ("TCVE error");
		reset();
	}
	
	fifek_ini (&oss_cb, 15);
	if (!running (perp_oss_out))
		runfsm perp_oss_out;

	if (!running (cmd_in))
		runfsm cmd_in;
}

