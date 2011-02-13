/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "msg_col.h"

#include "diag.h"
#include "app_col.h"
#include "app_col_data.h"
#include "oss_fmt.h"

#include "net.h"

void msg_pongAck_in (char * buf, word rssi) {

	upd_on_ack (in_pongAck(buf, ds), in_pongAck(buf, refdate),
			in_pongAck(buf, syfreq),
			in_pongAck(buf, plotid), in_header(buf, snd), rssi);
}

void upd_on_ack (lint ds, lint rd, word syfr, word pi, word fr,
		word rssi) {

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
#ifdef BOARD_CHRONOS
		chro_nn (1, fr); chro_nn(0, rssi);
#else
		diag ("CHRO (%u,%u)", fr, rssi);
#endif
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

	if (plot_id != pi) {
		plot_id = pi;
	}

}

void msg_setTag_in (char * buf, word rssi) {
	word w[6];
	char * out_buf;

	upd_on_ack (in_setTag(buf, ds), in_setTag(buf, refdate),
			in_setTag(buf, syfreq),
			in_setTag(buf, plotid), in_header(buf, snd), rssi);

	if (in_setTag(buf, pow_levels) != 0xFFFF) {
		w[0] = in_setTag(buf, pow_levels);
#if 0
		that was for just a single p.lev multiplied 4 times
		if (w[0] > 7)
			w[0] = 7;
		w[0] |= (w[0] << 4) | (w[0] << 8) | (w[0] << 12);
#endif
		if (pong_params.rx_lev != 0) // 'all' stays
			pong_params.rx_lev =
				max_pwr(w[0]);

		if (pong_params.pload_lev != 0)
			pong_params.pload_lev = pong_params.rx_lev;

		pong_params.pow_levels = w[0];
	}

	if (in_setTag(buf, freq_maj) != 0xFFFF) {
		if (pong_params.freq_maj != in_setTag(buf, freq_maj)) {
			pong_params.freq_maj = in_setTag(buf, freq_maj);
		}

		if (pong_params.freq_maj != 0 && !running(pong))
			runthread (pong);
	}

	if (in_setTag(buf, freq_min) != 0xFFFF)
		pong_params.freq_min = in_setTag(buf, freq_min);

	if (in_setTag(buf, rx_span) != 0xFFFF &&
			pong_params.rx_span != in_setTag(buf, rx_span)) {

		pong_params.rx_span = in_setTag(buf, rx_span);

		switch (in_setTag(buf, rx_span)) {
			case 0:
				killall (rxsw);
				net_opt (PHYSOPT_RXOFF, NULL);
				break;
			case 1:
				killall (rxsw);
				net_opt (PHYSOPT_RXON, NULL);
				break;
			default:
				if (!running(rxsw))
					runthread (rxsw);
		}
	}

	if (in_setTag(buf, ev) != 0xffff)
		do_butt (in_setTag(buf, ev));
	else { // don't send stats if there is an event (alarm goes out)
		if ((out_buf = get_mem_t (WNONE, sizeof(msgStatsTagType))) ==
			       NULL)
			return;

#if (RADIO_OPTIONS & 0x04)
		net_opt (PHYSOPT_ERROR, w);
#else
		memset (&w, 0, 8);
#endif
		w[4] = memfree (0, &w[5]);

		in_header(out_buf, hco) = 1; // no mhopping
		in_header(out_buf, msg_type) = msg_statsTag;
		in_header(out_buf, rcv) = in_header(buf, snd);
		in_statsTag(out_buf, lhid) = (word)host_id;
		in_statsTag(out_buf, clh) = local_host;
		in_statsTag(out_buf, ltime) = seconds();
		in_statsTag(out_buf, c_fl) =
			handle_c_flags (in_setTag(buf, c_fl));

		in_statsTag(out_buf, maj) = pong_params.freq_maj;
		in_statsTag(out_buf, min) = pong_params.freq_min;
		in_statsTag(out_buf, span) = pong_params.rx_span;
		in_statsTag(out_buf, pl) = pong_params.pow_levels;
		in_statsTag(out_buf, vtstats[0]) = w[0];
		in_statsTag(out_buf, vtstats[1]) = w[1];
		in_statsTag(out_buf, vtstats[2]) = w[2];
		in_statsTag(out_buf, vtstats[3]) = w[3];
		in_statsTag(out_buf, vtstats[4]) = w[4];
		in_statsTag(out_buf, vtstats[5]) = w[5];

		// should be SETPOWER here... FIXME?
		net_opt (PHYSOPT_TXON, NULL);
		send_msg_t (out_buf, sizeof(msgStatsTagType));
		net_opt (PHYSOPT_TXOFF, NULL);
		ufree (out_buf);
	}
}

