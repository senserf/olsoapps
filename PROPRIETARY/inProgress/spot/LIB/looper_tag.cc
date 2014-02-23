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
#include "commons.h"
#include "hold.h"
#include "sensors.h"

word	heartbeat = 60; // seconds

fsm pong; // external

#define _LOO_DBG	1

/* VOLTAGE is consistent across the boards, but I'd like to see how this
   works for common functionality on board-specific resources.
*/

#ifdef BOARD_CHRONOS
#include "chro.h"
#define VOLTAGE &chronos.volt
#endif

#ifndef VOLTAGE
#err where is VOLTAGE
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
                        app_diag_W ("lazy pong");

		if (heartbeat == 0) {
			app_diag_W ("looper exits");
			finish;
		}
#if _LOO_DBG
		app_diag_U ("LOO: hold %u", (word)(htime - seconds()));
#endif
        state HOLD:
                when (TRIG_ALRM, BEG); // it'll reset htime (doesn't have to)
                hold (HOLD, htime);
                proceed BEG;
}

