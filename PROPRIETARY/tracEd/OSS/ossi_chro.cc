/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* dupa: we have a fundamental problem: what is OSSI, what is UI, what are
   'other' i/f. There is no clear division (e.g. and actualtor can belong
   anywhere, depending on the application). I'm afraid grand unifications
   like these attempts here must end with crap good for nothing but standard
   bodies discussions.
*/
#include "app.h"
#include "app_dcl.h"
#include "chronos.h"
#include "msg.h"
#include "msg_dcl.h"
#include "oss_dcl.h"
#include "form.h"
#include "net.h"

static void ini_but (word b);
static void do_but  (word b);

fsm ossi_in;

#ifdef __SMURPH__
/* ser -specific ossi (setting praxis-specific cmd_d struct)
   is needed for buttons in vuee only; mimic Picos buttons with ser_in
*/

#include "ser.h"

static void (*baction) (word) = NULL;
static void _buttons_action (void (*a)(word));
fsm smurph_but;

static void _buttons_action (void (*a)(word)) {
	if ((baction = a) == NULL) {
		killall (smurph_but);
	} else
		if (!running (smurph_but))
			runfsm smurph_but;
}

fsm smurph_but {
	char b[2];

	state SB:
		ser_in (SB, b, 2); // this gives just one char string
		if (*b >= '0' && *b < '5')
			(*baction) ((word)(*b - '0'));
		else
			emul (0, "Bad button %c", *b);

		proceed (SB);
}

#else

#include "buttons.h"
#define _buttons_action buttons_action

#endif

fsm ossi_init {
	state OI_UP:
		_buttons_action ( ini_but );
		ezlcd_init (); // dupa: would double _init be harmful?
		buzzer_init (); // buzzer is a part of UI

	state OI_LOOP:
		if (app_flags.f.oss_a == YES) {
			buzzer_off();
			_buttons_action (NULL);
			runfsm ossi_in;
			finish;
		}

		ezlcd_on ();
		chro_nn (1, local_host);
		chro_lo ("UIOFF"); // this is equivalent of init_str
		when (TRIG_OSSI, OI_LOOP);
		delay (5000, OI_LOFF);
		release;

	state OI_LOFF:
		ezlcd_off();
		when (TRIG_OSSI, OI_LOOP);
		delay (OSS_QUANT, OI_LOOP);
		release;
}

fsm ossi_in {

	state OI_INIT:
		ezlcd_on ();
		chro_lo ("TRACE"); // welcome_str with ser
		chro_nn (1, local_host);
		_buttons_action (do_but);
		

	state OI_LOOP:
		if (app_flags.f.oss_a == NO) {
			_buttons_action (NULL);
			runfsm ossi_init;
			finish;
		}
		when (TRIG_OSSI, OI_LOOP);
		release;
}


// see sister routine in ossi_ser; here things seem unnecessary...
#define out_d	((cmd_t *)data)
fsm ossi_out (char *) {

	entry OO_START:
		if (data == NULL || out_d->buf == NULL) {
			ufree (data);
			finish;
		}

		buzzer_on ();	
		switch (out_d->code) {
			case ' ':
			case 'b':
				chro_hi (out_d->buf);
				if (strlen (out_d->buf) > 4)
					chro_lo (&out_d->buf[4]);
				break;

			case 'f':
			case 'F':
				chro_nn (1, out_d->argv_w[0]);
				chro_nn (0, out_d->argv_w[1]);
				break;
		}

		if (out_d->code == 'b'|| out_d->code == 'F')
			ufree (out_d->buf);
		ufree (out_d);

		delay (300, OO_FIN);
		release;

	state OO_FIN:
		buzzer_off();
		finish;
}
#undef out_d

void ossi_stats_out (char * b) {
	char li [12]; // fit strings below, even too long for ezlcd

	form (li, "s%u%u", app_flags.f.rx, b ? in_header(b, snd) : local_host);
	chro_hi (li);
	form (li, "%u%u", beac.b ? in_header(beac.b,msg_type) : 0,
		b ? in_stats(b, mhost) : master_host);
	chro_lo (li);
}

void ossi_trace_out (char * buf, word rssi) {
        char li [8];

	form (li, "t%u", in_header(buf, snd));
	chro_hi (li);
	form (li, "%u%u%u", in_header(buf, msg_type),
		(in_header(buf, msg_type) == msg_trace1) ?
			in_header(buf, seq_no) :
			in_traceAck(buf, fcount),
		in_header(buf, hoc) & 0x7F);
	chro_lo (li);
}

void ossi_help_out () {
	chro_nn (1, (word)seconds());
	chro_lo ("help");

}

void ossi_beac_out () {
	chro_nn (1, beac.f);
	chro_lo ("beac");

}

static void echoless_but (cmd_t *c) {
	char li[4];

	form (li, "%c%u", c->code, c->arg_c);
	chro_hi (li);
	chro_nn (0, c->argv_w[0]);
}

fsm del_lcdoff {
	state DL_START:
		delay (3000, DL_OFF);
		release;

	state DL_OFF:
		ezlcd_off ();
		finish;
}

// #define dupa(a...) emul(77,a)

static void b4 () {
	
	if (!running (ossi_init) && !running (ossi_in)) {
		// same actions in ini_ and do_but
		ezlcd_on();
		chro_lo ("RESET");
		reset();
	}

	ezlcd_on();
	chro_lo ("SLEEP");
	runfsm del_lcdoff;
	net_opt (PHYSOPT_RXOFF, NULL);
	buzzer_off ();

	// killall all
	killall (mbeacon);
	killall (msgbeac);
	killall (rcv);

	// keep these last; we won't return from here as we were called from
	// one of them: (dupa: this is a bit weird(?))
	killall (ossi_init);
	killall (ossi_in);
}

static void ini_but (word b) {

	if (b == 4) {
		b4 (); // #4 toggles SLEEP <-> reset (no UI)

	} else {
		if (running (ossi_init)) { // we're not in sleep

			if (b == 3) { // bring OSS ON
				app_flags.f.oss_a = YES;
			}

			trigger (TRIG_OSSI); // all buttons to remind UI OFF
		}
	}
}

/*
ossi_in() and ibuf2cmd() from ossi_ser are equivalent to ossi_in() and do_but()
here. It would be easier to skip the req_t req struct and process directly from
do_but(), but simplicity is not the goal here... Also, remember that reqs may
be remote.

dupa: Does all this crap make any sense? Can we have multiple i/f's structured
similarly without too much overhead on one of them? Note that a common structs
may lead to ossi libraries...
*/
static void do_but (word b) {

static lword tstamp = 0;
req_t *req = NULL;

	if (b == 4) { // this is truly unique, don't go via req_t structs
		b4 ();
		return; // just for esthetics, we'll be stuck in b4() anyway
	}

	if ((req = (req_t *)get_mem (WNONE, sizeof (req_t))) == NULL ||
	    (req->cmd = (cmd_t *)get_mem (WNONE, sizeof (cmd_t))) == NULL)
		goto IllFree;

	switch (b) {

		case 3: // O(ff) UI
			req->cmd->code = 'O';
			break;

		case 2: // rx switch
			req->cmd->code = 'r';
			req->cmd->arg_c = 1;
			if (app_flags.f.rx)
				req->cmd->argv_w[0] = 0;
			else
				req->cmd->argv_w[0] = 1;
			break;

		case 1: // trace
			req->cmd->code = 't';
			req->cmd->arg_c = 3;
			req->cmd->argv_w[1] = 1; // always dir == seeded fwf

			if ((req->cmd->argv_w[0] = master_host) == 0)
				req->cmd->argv_w[2] = 1; // only 1-nhood

			echoless_but (req->cmd);
			break;

		case 0: // stats / beacon
			if (seconds() - tstamp < 3) { // duble-push: beacon
				if (beac.f == 0) {
					req->cmd->argv_w[0] = 10; // arbitrary
					req->cmd->arg_c = 1;
				}

/*				implicit (default):
				else
					req->cmd->argv_w[0] = 0;
*/
				req->cmd->arg_c = 1;
				req->cmd->code = 'b';

			} else { // stats out
				tstamp = seconds();
				req->cmd->arg_c = 1;
				req->cmd->code = 's';

				if (master_host)
					req->cmd->argv_w[0] = master_host;
				else
					req->cmd->argv_w[0] = 0xFFFF;

				echoless_but (req->cmd);
			}
			break;
		default:
			goto IllFree;
	}

	if (req_in (req) == 0) // all good
		return;

	goto Ill;	

IllFree:
	ufree (req->cmd);
	ufree (req);
Ill:
	chro_lo ("ERROR");
}

