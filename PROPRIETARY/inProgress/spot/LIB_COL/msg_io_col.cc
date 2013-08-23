/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "msg_col.h"

#include "diag.h"
#include "app_col.h"
#include "app_col_data.h"
#include "oss_fmt.h"
#include "chro_col.h"
#include "net.h"

void msg_pongAck_in (char * buf, word rssi) {

	upd_on_ack (in_pongAck(buf, ds), in_pongAck(buf, refdate),
			in_pongAck(buf, syfreq),
			in_header(buf, snd), rssi);
}

void upd_on_ack (lint ds, lint rd, word syfr, word fr, word rssi) {

	lint dd;

	if (sens_data.stat != SENS_COLLECTED ||
			sens_data.ds != ds)
		return;

	if (rd != 0) { // set on the agg.
		// don't change for 'worse'
		if (rd < -SID * 90 || ref_date > -SID * 90) {
			if ((dd = wall_date_t (0) - rd) > TIME_TOLER ||
					dd < -TIME_TOLER) {
				ref_date = rd;
				ref_ts = seconds();
			}
		}
		// diag ...
	}
#if 0
	moved up in April 2009... no idea why it was here
	if (sens_data.ee.s.f.status != SENS_COLLECTED ||
			sens_data.ee.ds != ds)
		return;
#endif

	//leds (LED_R, LED_OFF);

	// only if all buttons cleared (?)
	if (buttons == 0) {
		chro_nn (1, fr); chro_nn (0, rssi);
	}

	sens_data.stat = SENS_CONFIRMED;
	trigger (ACK_IN);

	if (syfr != 0) {
		if (!is_synced) {
			set_synced;
		}

		if (pong_params.freq_maj != syfr) {
			diag (OPRE_APP_ACK "sync freq change %u -> %u",
				pong_params.freq_maj, syfr);
			pong_params.freq_maj = syfr;
		}

	} else {
		if (is_synced) {
			clr_synced;
		}
	}

}
