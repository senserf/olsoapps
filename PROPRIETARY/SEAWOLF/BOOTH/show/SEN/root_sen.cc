/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

// **** real quick hack **** //

#include "app.h"
#include "tarp.h"
#include "net.h"
#include "msg.h"
#include "diag.h"
#include "form.h"

#ifdef BOARD_CHRONOS
#include "chro.h"
#else
#include "ser.h"
#include "serf.h"
#include "storage.h"
#endif

#if CC1100
#define INFO_PHYS_DEV INFO_PHYS_CC1100
#else
#error "UNDEFINED RF"
#endif

#define LED_R 2
#define LED_G 1
#define LED_B 0

#define BATT_TRIG 	2250

#define WIRE_TRIG1 	3000
#define WIRE_TRIG2	1900
#define SHT_TRIG	1800

// it's hard to be more wasteful... of course, this is just for now ;-)
word    host_pl = 7;
word	sval[4];
word	sstate;
word	thr[3];
word	dia_fl = 0;

profi_t profi_att = (PROF_SENS + PROF_RXOFF),
	p_inc   = 0,
	p_exc   = 0xFFFF;

char    nick_att  [NI_LEN +1]           = "SENSOR";
char    desc_att  [PEG_STR_LEN +1]      = "-";
char    d_biz  [PEG_STR_LEN +1]         = "";
char    d_priv  [PEG_STR_LEN +1]        = "";
char    d_alrm  [PEG_STR_LEN +1]        = {'\0'};

// exact copy from msg_io_* As soon as we know what we're doing, all is trivial to consolidate.
static void msg_alrm_out (nid_t peg, word level, char * desc) {
	char * buf_out = get_mem (WNONE, sizeof(msgAlrmType));

	if (buf_out == NULL)
		return;

	in_header(buf_out, msg_type) = msg_alrm;
	in_header(buf_out, rcv) = peg; 
	in_header(buf_out, hco) = (level == 9 ? 0 : 1);
	in_header(buf_out, prox) = (level == 9 ? 0 : 1);
	in_alrm(buf_out, level) = level;
	in_alrm(buf_out, profi) = profi_att;
	strncpy (in_alrm(buf_out, nick), nick_att, NI_LEN);

	if (desc)
		strncpy (in_alrm(buf_out, desc), desc, PEG_STR_LEN);
	else
		strncpy (in_alrm(buf_out, desc), d_alrm, PEG_STR_LEN);

	send_msg (buf_out, sizeof (msgAlrmType));
	ufree (buf_out);
}

fsm abeacon {

    state INIT:
		delay (1048, SEND); // give sensors a chance...

    state LOOP:
		delay ((60 << 10) + rnd() % 2048, SEND); // 60 - 62s
		when (&sstate, SEND);
		release;

    state SEND:
		if (sstate != 0) {
			form (d_alrm, "!SE!&%u&%u&%u&%u&%u", sstate -1,
					sval[0], sval[1], sval[2], sval[3]); 
			msg_alrm_out (PINDA_ID, 0, NULL);
			if (dia_fl)
				diag ("%u %s", (word)seconds(), d_alrm);
			proceed (LOOP);
		}
    		diag ("hit sens");
		when (&sstate +1, SEND);
		delay (1024, LOOP);
}

#ifdef BOARD_CHRONOS
static word buttons = 0;

static void do_butt (word but) {
	if (but == 4) {
		if (buttons)
			reset();
		buttons = but;
		trigger (&buttons);
	}
}

fsm buzz {

        state BON:
                buzzer_on();
                delay (500, BOF);
                release;

        state BOF:
                buzzer_off();
		finish;
                // delay (10000, BON);
                // release;
}

fsm sens {

	lword lastm;

	state INI:
		ezlcd_init ();
		ezlcd_on ();
		buttons_action (do_butt);
		chro_hi ("SENS");
		chro_lo ("SLEEP");
		when (&buttons, ACTION);
		release;

	state ACTION:
		runfsm abeacon;
		lastm = seconds();

	state CYCLE:
		read_sensor (CYCLE, -1, &sval[0]);

		cma3000_on (0, 1, 3);
		chro_hi ("SENS");
		chro_nn (0, (word)(seconds() - lastm));

		if (sval[0] < thr[0])
			sstate = 2;
		else
			sstate = 1;
#ifdef __SMURPH__
		// hack in timestamp and ++, just for tests
		sval[2] = (word)seconds();
		sval[3]++;
#endif

//conceptual	state IMMOB:

		sval[1] = 0;
		delay ((60 << 10) + rnd() % 2048, CYCLE); // 60 - 62s
		wait_sensor (0, MOTION);
		// wait_sensor embeds release

	state MOTION:
		lastm = seconds();
		cma3000_off ();
		sval[1] = 1;
		sstate += 2;

		chro_hi ("ALRM");
		if (!running (buzz))
			runfsm buzz;

		trigger (&sstate);
		delay (2048 + rnd() % 2048, CYCLE);
		release;
}
#endif

#ifdef BOARD_WARSAW_SZAMBO
fsm sens {
	word alrm;

	state LOOP:
		sstate = alrm = 0; // stay away
		
	state S_1:
		read_sensor (S_1, -1, &sval[0]);
		if (sval[0] < thr[0])
			alrm = 1;
			
	state S0:
		read_sensor (S0, 0, &sval[1]);
		if (sval[1] < thr[1]) {
			alrm += 2;
			if (sval[1] < thr[2]) {
				alrm += 2; // 4 all together
			}
		}

		sstate = alrm +1;
		if (alrm) {
			leds (LED_G, 0);
			leds (LED_R, 1);
			trigger (&sstate);
		} else {
			leds (LED_R, 0);
			leds (LED_G, 1);
		}

#ifdef __SMURPH__
// hack in timestamp and ++, just for tests
		sval[2] = (word)seconds();
		sval[3]++;
#endif
		trigger (&sstate +1);
		delay (2048 + rnd() % 2048, LOOP);
}
#endif

#ifdef BOARD_WARSAW_10SHT
fsm sens {
	word tmp[10];
	word alrm;

	state LOOP:
		sstate = alrm = 0; // stay away
		
	state S_1:
		read_sensor (S_1, -1, &sval[0]);
		if (sval[0] < thr[0])
			alrm = 1;
			
	state S1:
		read_sensor (S1, 1, &tmp[0]); // only rh (0 - temp; 1 - RH)
		// for some reason, SHT have a run-up period of time with -1
		sval[1] = tmp[0];
		sval[2] = tmp[1];
		sval[3] = tmp[2];

		if (sval[1] > thr[1])
			alrm += 2;
		if (sval[2] > thr[1])
			alrm += 4;
		if (sval[3] > thr[1])
			alrm += 8;

		sstate = alrm +1;
		if (alrm) {
			leds (LED_G, 0);
			leds (LED_R, 1);
			trigger (&sstate);
		} else {
			leds (LED_R, 0);
			leds (LED_G, 1);
		}
		trigger (&sstate +1);
		delay (2048 + rnd() % 2048, LOOP);
}
#endif

static void ini () {
	net_id = DEF_NID;
	local_host = (word)host_id;
	tarp_ctrl.param = 0xA2; // level 2, rec 2, slack 1, fwd off
	if (net_init (INFO_PHYS_DEV, INFO_PLUG_TARP) < 0) {
		leds (LED_R, 1);
		app_diag_F ("FATAL");
		reset();
    	}

	net_opt (PHYSOPT_SETSID, &net_id);
	net_opt (PHYSOPT_SETPOWER, &host_pl);
	net_opt (PHYSOPT_RXOFF, NULL);

#ifdef BOARD_CHRONOS
	thr[0] = BATT_TRIG;
#else
	leds (LED_G, 1);
#endif

#ifdef BOARD_WARSAW_SZAMBO
	if (if_read (0) == 0xFFFF) {
		thr[0] = BATT_TRIG;
		thr[1] = WIRE_TRIG1;
		thr[2] = WIRE_TRIG2;
	} else {
		thr[0] = if_read (0);
		thr[1] = if_read (1);
		thr[2] = if_read (2);
	}
#endif

#ifdef BOARD_WARSAW_10SHT
	if (if_read (0) == 0xFFFF) {
		thr[0] = BATT_TRIG;
		thr[1] = SHT_TRIG;
	} else {
		thr[0] = if_read (0);
		thr[1] = if_read (1);
	}
#endif

	powerdown();
}

//added UI and FIM for thresholds
#define IBUF_LEN	40

fsm root {
	word tempek;
	char ibuf[IBUF_LEN];
	
	state INI:
		ini();
		runfsm sens;
#ifdef BOARD_CHRONOS
		finish;
#else
		runfsm abeacon;
	
	state WELCOME:
		ser_outf (WELCOME, "%s thresholds: %u %u %u\r\ns "
				"[<thr 0> [<thr 1> [< thr 2>]]]\r\nSA\r\n"
				"d(iag)(%u) [1|0]\r\nq\r\n",
			if_read (0) == 0xFFFF ? "Default" : "FIM", 
			thr[0], thr[1], thr[2], dia_fl);

		ibuf[0] = '\0';

	state CMD:
		word w[3];
		
		ser_in (CMD, ibuf, IBUF_LEN);
		
		if (ibuf[0] == 'q') reset();
		
		if (ibuf[0] == 'S' && ibuf[1] == 'A') {
			if (if_read (0) != 0xFFFF)
				if_erase (0);
			if_write (0, thr[0]);
			if_write (1, thr[1]);
#ifdef BOARD_WARSAW_SZAMBO
			if_write (2, thr[2]);
#endif
			proceed WELCOME;
		}
		
		if (ibuf[0] == 's') {
			w[0] = w[1] = w[2] = 0;
			scan (ibuf +1, "%u %u %u", &w[0], &w[1], &w[2]);
			if (w[0]) thr[0] = w[0];
			if (w[1]) thr[1] = w[1];
#ifdef BOARD_WARSAW_SZAMBO
			if (w[2]) thr[2] = w[2];
#endif
		}

		if (ibuf[0] == 'd') {
			w[0] = 0;
			scan (ibuf +1, "%u", &w[0]);
			dia_fl = w[0] ? 1 : 0;
		}
		
		proceed WELCOME;
#endif		
}
