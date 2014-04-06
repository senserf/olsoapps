/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2014                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* Reusable module looping for heartbeats with voltage reading.
     Heartbeat == 0 calls a single pong.
     Proceed to VOLT instead of BEG will carry on with the existing htime.
     I don't think separate alarm pools are a good idea, hence TRIG_ALRM.
*/

#include "sysio.h"
#include "diag.h"
#include "vartypes.h"
#include "hold.h"
#include "sensors.h"
#include "pong.h"

word	heartbeat = 60; // seconds

#define _LOO_DBG	0

/* VOLTAGE is consistent across the boards, but I'd like to see how this
   works for common functionality on board-specific resources...
   I don't think VOLTAGE should be defined here, but it should be #defined
   somewhere... and likely not in variants.h. Let's see if we need some
   sensor-related blocks.
*/

#if BTYPE == BTYPE_CHRONOS
#include "chro_tag.h"
#define VOLTAGE &chronos.volt
#endif

#if BTYPE == BTYPE_WARSAW
#include "war_tag.h"
#define VOLTAGE &warsaw.volt
#endif

#if BTYPE == BTYPE_AT_BUT6
#include "ap319_tag.h"
#define VOLTAGE &ap319.volt
#endif

#if BTYPE == BTYPE_AT_BUT1
#include "ap320_tag.h"
#define VOLTAGE &ap320.volt
#endif

#ifndef VOLTAGE
#error where is VOLTAGE
#endif

fsm looper {
	lword htime;

	state BEG:
	
#if _LOO_DBG
		app_diag_U ("LOO: BEG (%u)", (word)seconds());
#endif
		htime = seconds() + heartbeat;

	state VOLT:
		read_sensor (VOLT, -1, VOLTAGE);

		if (!running (pong))
			runfsm pong;
		else
			stash_pframe ();

		if (heartbeat == 0) {
			app_diag_W ("looper exits");
			finish;
		}
#if _LOO_DBG
		app_diag_U ("LOO: hold %u", (word)(htime - seconds()));
#endif

	// this is temporary, I think: heartbeat should be global, settable from pong
	state HOLD:
		when (TRIG_ALRM, BEG); // it'll reset htime (doesn't have to)
		when (TRIG_RONIN, RONIN);
		when (TRIG_DORO, DORO);
		hold (HOLD, htime);
		proceed BEG;
		
	state RONIN:
		if (heartbeat < 3600) {
			heartbeat += 60;
			htime += 60;
		}
		proceed HOLD; // sameas is better?
		
	state DORO:
		if (heartbeat != 60) {
			heartbeat = 60;
			htime = seconds() + 60;
		}
		proceed HOLD;
		
}

