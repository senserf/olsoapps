/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"
#include "iflash_sys.h"
#include "storage.h"

#include "ser.h"
#include "serf.h"
#include "form.h"

#include "hold.h"

#include "phys_cc1100.h"
#include "plug_null.h"
#include "cc1100.h"
#include "rtc_cc430.h"

#include "buttons.h"
#include "switches.h"
#include "netid.h"

#define	MIN_PACKET_LENGTH	24
#define	MAX_PACKET_LENGTH	(CC1100_MAXPLEN - 2)

#define	IBUFLEN			82
#define MAXPLEN			(MAX_PACKET_LENGTH + 2)

static sint 	sfd = -1, off, packet_length, rcvl, b;
static word	rssi, err, w, len, bs, nt, sl, ss, dcnt;
static lword	val, last_snt, last_rcv, s, u;
static byte	silent;
static char	*ibuf;
static address	packet;
static word	send_interval = 512;

static switches_t sw;

rtc_time_t dtime;

static void radio_start (word);
static void radio_stop ();

// ============================================================================

static word gen_packet_length (void) {

#if MIN_PACKET_LENGTH >= MAX_PACKET_LENGTH
	return MIN_PACKET_LENGTH;
#else
	return ((rnd () % (MAX_PACKET_LENGTH - MIN_PACKET_LENGTH + 1)) +
			MIN_PACKET_LENGTH) & 0xFFE;
#endif

}

fsm sender {

  state SN_SEND:

	packet_length = gen_packet_length ();

	if (packet_length < 10)
		packet_length = 10;
	else if (packet_length > MAX_PACKET_LENGTH)
		packet_length = MAX_PACKET_LENGTH;

  state SN_NEXT:

	sint pl, pp;

	packet = tcv_wnp (SN_NEXT, sfd, packet_length + 2);

	packet [0] = 0;
	packet [1] = 0xBABA;

	// In words
	pl = packet_length / 2;
	((lword*)packet)[1] = wtonl (last_snt);

	for (pp = 4; pp < pl; pp++)
		packet [pp] = (word) entropy;

	tcv_endp (packet);

  state SN_MESS:

	if (!silent) {
		ser_outf (SN_MESS, "Sent: %lu [%d]\r\n", last_snt,
			packet_length);
	}
	last_snt++;
	delay (send_interval, SN_SEND);
}

fsm receiver {

  state RC_TRY:

  	address packet;

	packet = tcv_rnp (RC_TRY, sfd);
	last_rcv = ntowl (((lword*)packet) [1]);
	rcvl = tcv_left (packet) - 2;
	rssi = packet [rcvl >> 1];
	tcv_endp (packet);

  state RC_MESS:

	if (!silent) {
		ser_outf (RC_MESS, "Rcv: %lu [%d], RSSI = %d, QUA = %d\r\n",
			last_rcv, rcvl, (rssi >> 8) & 0x00ff, rssi & 0x00ff);
	}
	proceed RC_TRY;

}

static void radio_start (word d) {

	if (sfd < 0)
		return;

	if (d & 1) {
		tcv_control (sfd, PHYSOPT_RXON, NULL);
		if (!running (receiver))
			runfsm receiver;
	}
	if (d & 2) {
		tcv_control (sfd, PHYSOPT_TXON, NULL);
		if (!running (sender))
			runfsm sender;
	}
}

static void radio_stop () {

	if (sfd < 0)
		return;

	killall (sender);
	killall (receiver);

	tcv_control (sfd, PHYSOPT_RXOFF, NULL);
	tcv_control (sfd, PHYSOPT_TXOFF, NULL);
}

// ============================================================================

fsm test_ifl {

  state IF_START:

	ser_out (IF_START,
		"\r\nFLASH Test\r\n"
		"l led w -> led status [w = 0, 1, 2]\r\n"
		"b w -> blinkrate 0-low, 1-high\r\n"
		"d m -> write a diag message\r\n"
		"w adr w -> write word to info flash\r\n"
		"r adr -> read word from info flash\r\n"
		"e adr -> erase info flash\r\n"
		"W adr w -> write word to code flash\r\n"
		"R adr -> read word from code flash\r\n"
		"E adr -> erase code flash\r\n"
		"T adr -> flash overwrite test\r\n"
		"q -> return to main test\r\n"
	);

  state IF_RCMD:

	err = 0;
	ser_in (IF_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'l': proceed IF_LED;
		case 'b': proceed IF_BLI;
		case 'd': proceed IF_DIA;
		case 'w': proceed IF_FLW;
		case 'r': proceed IF_FLR;
		case 'e': proceed IF_FLE;
		case 'W': proceed IF_CLW;
		case 'R': proceed IF_CLR;
		case 'E': proceed IF_CLE;
		case 'T': proceed IF_COT;
		case 'q': { finish; };
	}
	
  state IF_RCMDP1:

	ser_out (IF_RCMDP1, "Illegal\r\n");
	proceed IF_START;

  state IF_LED:

	scan (ibuf + 1, "%u %u", &bs, &nt);
	leds (bs, nt);
	proceed IF_RCMD;

  state IF_BLI:

	scan (ibuf + 1, "%u", &bs);
	fastblink (bs);
	proceed IF_RCMD;

  state IF_DIA:

	diag ("MSG %d (%x) %u: %s", dcnt, dcnt, dcnt, ibuf+1);
	dcnt++;
	proceed IF_RCMD;

  state IF_FLR:

	scan (ibuf + 1, "%u", &w);
	if (w >= IFLASH_SIZE)
		proceed IF_RCMDP1;
	diag ("IF [%u] = %x", w, IFLASH [w]);
	proceed IF_RCMD;

  state IF_FLW:

	scan (ibuf + 1, "%u %u", &w, &bs);
	if (w >= IFLASH_SIZE)
		proceed IF_RCMDP1;
	if (if_write (w, bs))
		diag ("FAILED");
	else
		diag ("OK");
	goto Done;

  state IF_FLE:

	b = -1;
	scan (ibuf + 1, "%d", &b);
	if_erase (b);
	goto Done;

  state IF_CLR:

	scan (ibuf + 1, "%u", &w);
	diag ("CF [%u] = %x", w, *((address)w));
	proceed IF_RCMD;

  state IF_CLW:

	scan (ibuf + 1, "%u %u", &w, &bs);
	cf_write ((address)w, bs);
Done:
	diag ("OK");
	proceed IF_RCMD;

  state IF_CLE:

	b = 0;
	scan (ibuf + 1, "%d", &b);
	cf_erase ((address)b);
	goto Done;

  state IF_COT:

	w = 0;
	scan (ibuf + 1, "%u", &w);

	if (*((address)w) != 0xffff) {
		diag ("Word not erased: %x", *((address)w));
		proceed IF_RCMD;
	}

	for (b = 1; b <= 16; b++) {
		nt = 0xffff << b;
		cf_write ((address)w, nt);
		sl = *((address)w);
		diag ("Written %x, read %x", nt, sl);
	}
	goto Done;

}

// ============================================================================

fsm test_delay {

  state DE_INIT:

	ser_out (DE_INIT,
		"\r\nRF Power Test\r\n"
		"l s -> hold\r\n"
		"d -> PD mode\r\n"
		"u -> PU mode\r\n"
		"s n -> spin test for n sec\r\n"
		"q -> return to main test\r\n"
	);

  state DE_RCMD:

	ser_in (DE_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
#if GLACIER
		case 'f': proceed DE_FRE;
#endif
		case 'l': proceed DE_LHO;
		case 'd': proceed DE_PDM;
		case 'u': proceed DE_PUM;
		case 's': proceed DE_SPN;
		case 'q': { finish; }
	}
	
  state DE_RCMDP1:

	ser_out (DE_RCMDP1, "Illegal\r\n");
	proceed DE_INIT;

  state DE_LHO:

	nt = 0;
	scan (ibuf + 1, "%u", &nt);
	val = (lword) nt + seconds ();
	diag ("Start %u", (word) seconds ());

  state DE_LHOP1:

	hold (DE_LHOP1, val);
	diag ("Stop %u", (word) seconds ());
	proceed DE_RCMD;

  state DE_PDM:

	diag ("Entering PD mode");
	powerdown ();
	proceed DE_RCMD;

  state DE_PUM:

	diag ("Entering PU mode");
	powerup ();
	proceed DE_RCMD;

  state DE_SPN:

	nt = 0;
	scan (ibuf + 1, "%u", &nt);

	// Wait for the nearest round second
	s = seconds ();

  state DE_SPNP1:

	if ((u = seconds ()) == s)
		proceed DE_SPNP1;

	u += nt;
	s = 0;

  state DE_SPNP2:

	if (seconds () != u) {
		s++;
		proceed DE_SPNP2;
	}

  state DE_SPNP3:

	ser_outf (DE_SPNP2, "Done: %lu cycles\r\n", s);
	proceed DE_RCMD;

}

// ============================================================================

fsm test_rtc {

  state RT_MEN:

	ser_out (RT_MEN,
		"\r\nRTC Test\r\n"
		"s y m d dw h m s -> set the clock\r\n"
		"r -> read the clock\r\n"
		"q -> quit\r\n"
	);

  state RT_RCM:

	ser_in (RT_RCM, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

	    case 's': proceed RT_SET;
	    case 'r': proceed RT_GET;
	    case 'q': {
			finish;
	    }

	}

  state RT_ERR:

	ser_out (RT_ERR, "Illegal\r\n");
	proceed RT_MEN;

  state RT_SET:

	sl = WNONE;
	scan (ibuf + 1, "%u %u %u %u %u %u %u",
		&rssi, &err, &w, &len, &bs, &nt, &sl);

	if (sl == WNONE)
		proceed RT_ERR;

	dtime.year = rssi;
	dtime.month = err;
	dtime.day = w;
	dtime.dow = len;
	dtime.hour = bs;
	dtime.minute = nt;
	dtime.second = sl;

	rtc_set (&dtime);

  state RT_SETP1:

	ser_out (RT_SETP1, "Done\r\n");
	proceed RT_RCM;

  state RT_GET:

	bzero (&dtime, sizeof (dtime));
	rtc_get (&dtime);

  state RT_GETP1:

	ser_outf (RT_GETP1, "Date = %u %u %u %u %u %u %u\r\n",
				dtime.year,
				dtime.month,
				dtime.day,
				dtime.dow,
				dtime.hour,
				dtime.minute,
				dtime.second);
	proceed RT_RCM;
}

// ============================================================================

fsm test_sensors {

  state SE_INIT:

	ser_out (SE_INIT,
		"\r\nSensor Test\r\n"
		"r s -> read sensor s\r\n"
		"c s d n -> read continually at d ms, n times\r\n"
		"q -> quit\r\n"
		);

  state SE_RCMD:

	ser_in (SE_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'r' : proceed SE_GSEN;
		case 'c' : proceed SE_CSEN;
	    	case 'q': { finish; };
	}

  state SE_RCMDP1:

	ser_out (SE_RCMDP1, "Illegal\r\n");
	proceed SE_INIT;

  state SE_OK:

	ser_out (SE_OK, "OK\r\n");
	proceed SE_RCMD;

  state SE_GSEN:

	b = 0;
	scan (ibuf + 1, "%d", &b);

  state SE_GSENP1:

	// To detect absent sensors, value ABSENT01
	val = 0xAB2E4700;
	read_sensor (SE_GSENP1, b, (address)(&val));

  state SE_GSENP2:

	ser_outf (SE_GSENP2, "Val: %lx [%u] <%d, %d, %d, %d>\r\n",
		val,
		*((word*)(&val)),
		((char*)(&val)) [0],
		((char*)(&val)) [1],
		((char*)(&val)) [2],
		((char*)(&val)) [3]);
	proceed SE_RCMD;

  state SE_CSEN:

	b = 0;
	bs = 0;
	nt = 0;

	scan (ibuf + 1, "%d %u %u", &b, &bs, &nt);
	if (nt == 0)
		nt = 1;

  state SE_CSENP1:

	val = 0xAB2E4700;
	read_sensor (SE_CSENP1, b, (address)(&val));
	nt--;

  state SE_CSENP2:

	ser_outf (SE_CSENP2, "Val: %lx [%u] <%d, %d, %d, %d> (%u left)\r\n",
		val,
		*((word*)(&val)),
		((char*)(&val)) [0],
		((char*)(&val)) [1],
		((char*)(&val)) [2],
		((char*)(&val)) [3], nt);

	if (nt == 0)
		proceed SE_RCMD;
	
	delay (bs, SE_CSENP1);
	release;

}

// ============================================================================

static word Buttons;

fsm button_thread {

	state BT_LOOP:

		if (Buttons == 0) {
			when (&Buttons, BT_LOOP);
			release;
		}

		ser_outf (BT_LOOP, "Press: %x\r\n", Buttons);
		Buttons = 0;
		sameas BT_LOOP;
}

static void butpress (word but) {

	Buttons |= (1 << but);
	trigger (&Buttons);
}

fsm test_buttons {

	state TB_INIT:

		ser_out (TB_INIT, "Press button, q to quit\r\n");

		buttons_action (butpress);

		if (!running (button_thread))
			runfsm button_thread;

	state TB_WAIT:

		ser_in (TB_WAIT, ibuf, IBUFLEN-1);

		if (ibuf [0] == 'q') {
			buttons_action (NULL);
			killall (button_thread);
			finish;
		}

		sameas TB_WAIT;
}

// ============================================================================

fsm root {

  state RS_INIT:

#if STACK_GUARD
	sl = stackfree ();
#endif
#if MALLOC_STATS
	ss = memfree (0, NULL);
#endif
#if STACK_GUARD || MALLOC_STATS

  state RS_SHOWMEM:

	ser_outf (RS_SHOWMEM, "MEM: %d %d\r\n", sl, ss);
#endif

	ibuf = (char*) umalloc (IBUFLEN);

	diag ("");
	diag ("");

	diag ("Radio ...");

	phys_cc1100 (0, MAXPLEN);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

	off = NETWORK_ID;
	tcv_control (sfd, PHYSOPT_SETSID, (address)&off);

	// Start in powerdown
	powerdown ();

  state RS_RCMDM2:

	ser_out (RS_RCMDM2,
		"\r\nRoot:\r\n"
		"r s -> start radio\r\n"
		"i d -> xmit interval\r\n"
		"p v  -> xmit pwr\r\n"
		"c v  -> channel\r\n"
		"q -> stop radio\r\n"
#if STACK_GUARD || MALLOC_STATS
		"m -> memstat\r\n"
#endif
		"n -> reset\r\n"
		"d -> pwr: 0-d, 1-u\r\n"
		"F -> flash test\r\n"
		"D -> power\r\n"
		"V -> sensors\r\n"
		"T -> RTC\r\n"
		"b -> buttons\r\n"
		"O -> option switches\r\n"
	);

  state RS_RCMD:

	ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

		case 'r' : {

			char *p;

			w = 0;
			p = ibuf + 1;
			while (!isdigit (*p) && *p != '\0')
				p++;
			if (*p != '\0') {
				w = (*p) - '0';
				p++;
			}
			if ((w & 3) == 0)
				w |= 3;
StartRadio:
			radio_start (w);

RS_Loop:		proceed RS_RCMD;
		}

		case 'i' : {

			send_interval = 512;
			b = -1;
			scan (ibuf + 1, "%u %d", &send_interval, &b);
			if (b >= 0)
				silent = b;
			goto RS_Loop;
		}
	
		case 'p' : {
			// Setpower, default = max
			off = 255;
			scan (ibuf + 1, "%d", &off);
			tcv_control (sfd, PHYSOPT_SETPOWER, (address)&off);
			goto RS_Loop;
		}

		case 'c' : {
			off = 0;
			scan (ibuf + 1, "%d", &off);
			tcv_control (sfd, PHYSOPT_SETCHANNEL, (address)&off);
			goto RS_Loop;
		}

		case 'q' : {
			radio_stop ();
			goto RS_Loop;
		}

		case 'd' : {
			off = 0;
			scan (ibuf + 1, "%d", &off);
			if (off)
				powerup ();
			else
				powerdown ();
			goto RS_Loop;
		}

		case 'n' : reset ();

		case 'F' : {
				runfsm test_ifl;
				joinall (test_ifl, RS_RCMDM2);
				release;
		}

		case 'D' : {
				runfsm test_delay;
				joinall (test_delay, RS_RCMDM2);
				release;
		}

		case 'V' : {
				runfsm test_sensors;
				joinall (test_sensors, RS_RCMDM2);
				release;
		}

		case 'T' : {
				runfsm test_rtc;
				joinall (test_rtc, RS_RCMDM2);
				release;
		}

#if STACK_GUARD || MALLOC_STATS
		case 'm' : {
#if STACK_GUARD
			sl = stackfree ();
#else
			sl = 0;
#endif
#if MALLOC_STATS
			ss = memfree (0, &bs);
			w = maxfree (0, &len);
#else
			ss = w = bs = len = 0;
#endif
			diag ("S = %u, MF = %u, FA = %u, MX = %u, NC = %u",
				sl, ss, bs, w, len);
			proceed RS_RCMD;
		}
#endif

		case 'b' : {
			runfsm test_buttons;
			joinall (test_buttons, RS_RCMDM2);
			release;
		}

		case 'O' : {

			read_switches (&sw);
			w = sw.S0 + sw.S1 * 10;
			proceed RS_SHOWOSW;
		}
	}

  state RS_RCMDP1:

	ser_out (RS_RCMDP1, "Illegal\r\n");
	proceed RS_RCMDM2;

  state RS_SHOWOSW:

	ser_outf (RS_SHOWOSW, "NN = %x (%u), SA = %x, SB = %x\r\n",
		sw.S0 | (sw.S1 << 4), w, sw.S2, sw.S3);

	proceed RS_RCMD;
}
