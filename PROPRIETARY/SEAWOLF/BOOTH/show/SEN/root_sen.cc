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

#if CC1100
#define INFO_PHYS_DEV INFO_PHYS_CC1100
#else
#error "UNDEFINED RF"
#endif

#define LED_R 0
#define LED_G 1
#define LED_B 2

word    host_pl 		= 7;
word	sval[4];
word	sstate;

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

    state START:
		delay ((60 << 10) + rnd() % 2048, SEND); // 60 - 62s
		when (&sstate, SEND);
		release;

    initial state SEND:
		if (sstate != 0) {
			form (d_alrm, "!SE!,%u,%u,%u,%u,%u", sstate -1,
					sval[0], sval[1], sval[2], sval[3]); 
			msg_alrm_out (PINDA_ID, 0, NULL);
		}
		proceed (START);

}

#define BATT_TRIG 	2250

#ifdef BOARD_WARSAW_SZAMBO
#define WIRE_TRIG1 	3000
#define WIRE_TRIG2	1900

fsm sens {
	word alrm;

	state LOOP:
		sstate = alrm = 0; // stay away
		
	state S_1:
		read_sensor (S_1, -1,&sval[0]);
		if (sval[0] < BATT_TRIG)
			alrm = 1;
			
	state S0:
		read_sensor (S0, 0, &sval[1]);
		if (sval[1] < WIRE_TRIG1) {
			alrm += 2;
			if (sval[1] < WIRE_TRIG2) {
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
// hack in timestamp and ++, just for testing
		sval[2] = (word)seconds();
		sval[3]++;
#endif
		delay (2048 + rnd() % 2048, LOOP);
}
#endif

#ifdef BOARD_WARSAW_10SHT
#define SHT_TRIG	100

fsm sens {
	word tmp[10];
	word alrm;

	state LOOP:
		sstate = alrm = 0; // stay away
		
	state S_1:
		read_sensor (S_1, &sval[0]);
		if (sval[0] < BATT_TRIG)
			alrm = 1;
			
	state S1:
		read_sensor (S1, &tmp[0]); // only humidity (0 - temp; 1 - RH)
		sval[1] = tmp[0];
		sval[2] = tmp[1];
		sval[3] = tmp[2];
		
		if (sval[1] > SHT_TRIG)
			alrm += 2;
		if (sval[2] > SHT_TRIG)
			alrm += 4;
		if (sval[3] > SHT_TRIG)
			alrm += 8;

		state = alrm +1;
		if (alrm) {
			leds (LED_G, 0);
			leds (LED_R, 1);
			trigger (&sstate);
		} else {
			leds (LED_R, 0);
			leds (LED_G, 1);
		}
		delay (2048, LOOP);
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
	leds (LED_G, 1);
	powerdown();
}

fsm root {
	word tempek;

	state INI:
		ini();
		runfsm sens;
		runfsm abeacon;
		finish;
}
