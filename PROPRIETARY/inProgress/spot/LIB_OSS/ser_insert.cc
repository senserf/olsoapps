/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2014                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

///////////////// oss out ////////////////
// this is better than multiple fsm oss_out I used before
//
// 15 is max with the sizes in FIFEK
#define FIFEK_SIZ 15
static struct { // yes, not all field are truly needed but same size it is
	char	*buf [FIFEK_SIZ];
	word	h :4;
	word	t :4;
	word	n :4;
	word	o :4;
} fifek;

static word _oss_out (char * b) {

	if (b == NULL)
		return 2;

	if (fifek.n < FIFEK_SIZ) {
		fifek.buf[fifek.h] = b;
		++fifek.n; ++fifek.h; fifek.h %= FIFEK_SIZ;
		trigger (TRIG_OSSO);
		return 0;
	}
	app_diag_S ("OSS oflow (%u) %u", seconds(), ++fifek.o);
	ufree (b);
	return 1;
}
 
fsm perp_oss_out () {

        state CHECK:
		if (fifek.n == 0) {
			when (TRIG_OSSO, CHECK);
			release;
		}

        state RETRY:
                ser_outb (RETRY, fifek.buf[fifek.t]);
                --fifek.n; ++fifek.t; fifek.t %= FIFEK_SIZ;
                proceed (CHECK);
}
#undef FIFEK_SIZ

/////////////////

fsm cmd_in;
void oss_ini () {

#ifdef BOARD_WARSAW_BLUE
	// Use UART 2 via Bluetooth
	ser_select (1);
#endif
	if (!running (perp_oss_out))
		runfsm perp_oss_out;

	if (!running (cmd_in))
		runfsm cmd_in;
}

