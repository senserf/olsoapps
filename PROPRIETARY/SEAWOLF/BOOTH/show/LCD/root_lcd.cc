/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "app_lcd_data.h"
#include "tarp.h"
#include "net.h"
#include "msg.h"
#include "chro.h"
#include "diag.h"
#include "form.h"

#if CC1100
#define INFO_PHYS_DEV INFO_PHYS_CC1100
#else
#error "UNDEFINED RF"
#endif

#define PASSWD_RUDY 102

static lword hts, hstart;
static word hunt[4] = {1000, 1001, 1002, 1003};
static word hunt_ind = 0;

static char hlo[6] = "THUNT";
static char hhi[5] = "-";
#define WIN_RSSI	190
#define HUNT_END	3

word    host_pl 		= 7;

profi_t profi_att = (profi_t) PREINIT (PROF_LCD, "PROFI");
profi_t p_inc = (profi_t) PREINIT (0, "PINC");
profi_t p_exc = (profi_t) PREINIT (0xFFFF, "PEXC");

#ifdef __SMURPH__
// a bit costly, do it for vuee only, see strcpy at root's init
const char * _nick_att_ini = (char *) PREINIT ("uninit", "NICK");
const char * _desc_att_ini = (char *) PREINIT ("uninit desc", "DESC");
const char * _d_biz_ini = (char *) PREINIT ("uninit biz", "DBIZ");
const char * _d_priv_ini = (char *) PREINIT ("uninit priv", "DPRIV");
#endif

char    nick_att  [NI_LEN +1]           = "VIP";
char    desc_att  [PEG_STR_LEN +1]      = "celeb";
char    d_biz  [PEG_STR_LEN +1]         = "";
char    d_priv  [PEG_STR_LEN +1]        = "";
char    d_alrm  [PEG_STR_LEN +1]        = {'\0'};

fsm buzz {

        state BON:
                buzzer_on();
                delay (500, BOF);
                release;

        state BOF:
                buzzer_off();
                finish;
}

fsm mbeacon {

    state MB_START:
		delay (2048 + rnd() % 2048, MB_SEND); // 2 - 4s
		release;

    initial state MB_SEND:
		msg_profi_out (0);
		proceed (MB_START);

}

fsm abeacon {

    state START:
		delay (2048 + rnd() % 2048, SEND); // 2 - 4s
		release;

    initial state SEND:
		msg_alrm_out (PINDA_ID, 0, NULL);
		if (!running (buzz))
			runfsm buzz;
		proceed (START);

}

/* likely, we'll do chronos's oss_out more consistent, as it is in SUI,
	with in_alrm !TT! for Android. Later?
*/
static void process_incoming (word state, char * buf, word size, word rssi) {

	word hunting = 0;
	
	if (in_header(buf, msg_type) != msg_alrm ||
			!(profi_att & PROF_HUNT) || hunt_ind >= HUNT_END ||
			in_header (buf, snd) != hunt[hunt_ind])
		return;

	// current hunt
	hts = seconds();
	if (in_header (buf, hoc) <= 1) {

		if (rssi >= WIN_RSSI) {

			rssi = WIN_RSSI;

			if (++hunt_ind >= HUNT_END) {

		// ASAP even if somewhat inconsistent: we can't kill rcv here
				net_opt (PHYSOPT_RXOFF, NULL);

				hunting = 2;
			} else {
				hunting = 1;
				rssi = 0; // a new one counts
				hlo[0] = '0' + HUNT_END - hunt_ind;
				hhi[0] = '-'; hhi[1] = '\0';
				chro_hi (hhi);
				chro_lo (hlo);
				runfsm buzz;
			}

		} else {
			form (hhi, "R%u", WIN_RSSI - rssi);
			chro_hi (hhi);
		}

	} else {
		form (hhi, "H%u", in_header (buf, hoc));
		chro_hi (hhi);
	}

	form (d_alrm, "!TH!&%u&%u&%u&%u&%u", (word)(hts - hstart),
		in_header (buf, hoc), WIN_RSSI - rssi, 
		HUNT_END - hunt_ind, hunting);	

	if (hunting >= 2) {
		if (!running (abeacon)) {
			runfsm abeacon;
		}
		chro_hi ("WON");
		chro_nn (0, (word)(seconds() - hstart));
	} else {
		msg_alrm_out (PINDA_ID, 0, NULL);
	}
}

static word map_rssi (word r) {
	return r >> 8;
}

fsm rcv {

	sint	packet_size = 0;
	word	rssi = 0;
	char   *buf_ptr = NULL;

	state RC_TRY:
		if (buf_ptr != NULL) {
			ufree (buf_ptr);
			buf_ptr = NULL;
			packet_size = 0;
		}
    		packet_size = net_rx (RC_TRY, &buf_ptr, &rssi, 0);
		if (packet_size <= 0) {
			app_diag_S ("net_rx failed (%d)", packet_size);
			proceed RC_TRY;
		}

	state RC_MSG:
		process_incoming (RC_MSG, buf_ptr, packet_size,
			map_rssi(rssi));

		proceed RC_TRY;

}

static word buttons;

static void do_butt (word but) {
	buttons = but; // just the pressed button
	trigger (&buttons);
}

static void ini () {

#ifdef __SMURPH__
	strncpy (nick_att, _nick_att_ini, NI_LEN);
	strncpy (desc_att, _desc_att_ini, PEG_STR_LEN);
	strncpy (d_biz, _d_biz_ini, PEG_STR_LEN);
	strncpy (d_priv, _d_priv_ini, PEG_STR_LEN);
#endif

	net_id = DEF_NID;
	local_host = (word)host_id;
	tarp_ctrl.param = 0xA2; // level 2, rec 2, slack 1, fwd off
	if (net_init (INFO_PHYS_DEV, INFO_PLUG_TARP) < 0) {
		chro_lo ("FATAL");
		reset();
	}
	net_opt (PHYSOPT_SETSID, &net_id);
	net_opt (PHYSOPT_SETPOWER, &host_pl);
	net_opt (PHYSOPT_RXOFF, NULL);

	buttons_action (do_butt);
	ezlcd_init();
	ezlcd_on();
	powerdown();
}


/* it would be cleaner to start from SLEEP but we need to reset before real
action, as the watch can be sleeping for ages and we shortcut to 64K s of
action... For now, it'll do. Later, we may play with FIM, etc.

Another angle: if profi_att, p_inc, p_exc are not precanned, it may be
easier to do setups via cycling 4-hex-digit numbers. For that, we'd have to
write things to FIM, which I want to avoid right now.
*/
fsm root {
	// word tempek;

	state S_SLEEP:
		chro_lo ("SLEEP");
		chro_hi ("-");
		// wholesale slaughterhouse
		net_opt (PHYSOPT_RXOFF, NULL);
		killall (rcv);
		killall (mbeacon);
		killall (abeacon);
		killall (buzz);
		buzzer_off();
	state CONT_SLEEP:
		when (&buttons, IN_SLEEP);
		release;

	state IN_SLEEP:
		if (buttons != 4)
			proceed CONT_SLEEP;
		reset();
#if 0
	// this below is commented out as too difficult for an average bear,
	// but we could manipulate
	// ids and perhaps have an id-based (say 111) immobility sensor

	state S_PASSWD:
		chro_lo ("-_-_");
		chro_nn (1, tempek);
		when (&buttons, IN_PASSWD);
		release;
		
	state IN_PASSWD:
		switch (buttons) {
			case 4:
				if (tempek == PASSWD_RUDY) {
					tempek = local_host;
					proceed S_ID;
				}
				proceed S_CELEB;
			
			case 3: // cancel
				tempek = 0;
				proceed S_PASSWD;
				
			default:
				// 2 or 1 or 0
				tempek = tempek * 10 + buttons;
				proceed S_PASSWD;
		}
		
	state S_ID:
		chro_lo ("ID");
		chro_nn (1, tempek);
		when (&buttons, IN_ID);
		release;
		
	state IN_ID:
		switch (buttons) {
			case 4:	// new id
				if (tempek != 0)
					local_host = tempek;
				
				proceed S_CELEB;
			
			case 3: // cancel
				tempek = local_host;
				proceed S_ID;
			
			case 2: // 100s
				if (tempek >= 900)
					tempek -= 900;
				else
					tempek += 100;
				break;
				
			case 1: // 10s
				if (tempek % 100 >= 90)
					tempek -= 90;
				else
					tempek += 10;
				break;
					
			case 0:
				if (tempek % 10 == 9)
					tempek -= 9;
				else
					tempek++;
		}
		proceed S_ID;
#endif

	initial state S_CELEB:
		ini ();
		chro_lo ("CELEB");
		chro_nn (1, local_host);
		if (!running (mbeacon))
			runfsm mbeacon;
		when (&buttons, IN_CELEB);
		release;

	state IN_CELEB:
		if (buttons == 4)
			proceed S_HUNT;
		else {
			form (d_alrm, "!BU!&%u", buttons);
			msg_alrm_out (PINDA_ID, 0, NULL);
		}
		when (&buttons, IN_CELEB);
		release;

	state S_HUNT:
		profi_att |= PROF_HUNT;
		p_exc &= ~PROF_TAG;
		hunt_ind = 0;

		hlo[0] = '0' + HUNT_END;
		hunt_ind = 0;
		hstart= seconds();

		chro_hi (hhi);
		chro_lo (hlo);

		killall (mbeacon);
		runfsm rcv;
		net_opt (PHYSOPT_RXON, NULL);
		buttons = 0;

	state IN_HUNT:
		if (buttons == 4) {
			profi_att &= ~PROF_HUNT;
			p_exc |= PROF_TAG;
			proceed S_SLEEP; // abeacon will die there
		} else
			buttons = 0;

		if (hunt_ind >= HUNT_END) {
			// done already: net_opt (PHYSOPT_RXOFF, NULL);
			killall (rcv);

		} else {
			if (seconds() - hts > 10) { // 10s
				form (d_alrm, "!TH!&%u&%u&%u&%u&%u", 
					(word)(seconds() - hstart),
			                0, 0, HUNT_END - hunt_ind, 0);
				hhi[0] = '-'; hhi[1] = '\0';
				msg_alrm_out (PINDA_ID, 0, NULL);
				chro_hi (hhi);
			}
			delay (3072, IN_HUNT); // 3s arbitrary, sort of audit
		}

		when (&buttons, IN_HUNT);
		release;
}
