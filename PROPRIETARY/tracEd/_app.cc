/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "app.h"
#include "app_dcl.h"
#include "tarp.h"
#include "pins.h"
#include "sensors.h"
#include "msg_dcl.h"

// globals init to 0
lword		master_ts;
word 		bat;
appfl_t		app_flags;

#define BAT_WARN	2250
fsm root {

	state RO_INIT:
		init();
		delay (DEL_QUANT, RO_AUDIT);
		release; // start things going for a short while

	// let's keep root running as an auditing process...  good in general?
	state RO_AUDIT:
		read_sensor (RO_AUDIT, SENSOR_BATTERY, &bat);

		if (bat < BAT_WARN)
			leds (LED_B, LED_ON);
		else
			leds (LED_B, LED_OFF);

		// this may not be such a good idea in some situations, e.g.
		// on CHRONOS with RX OFF most of the time:
		if (seconds() - master_ts > 3* (MAS_QUANT >> 10)) {
			master_host = 0;
			master_ts = seconds();
			leds (LED_G, LED_OFF);
		}
		// what else to audit?

		delay (AUD_QUANT, RO_AUDIT);
		release;
}
#undef BAT_WARN

