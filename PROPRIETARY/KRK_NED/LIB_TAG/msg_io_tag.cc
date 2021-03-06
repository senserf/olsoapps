/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "diag.h"
#include "app_tag.h"
#include "msg_tag.h"
#include "oss_fmt.h"
#include "net.h"

#include "app_tag_data.h"

void msg_pongAck_in (char * buf) {

	upd_on_ack (in_pongAck(buf, ds), in_pongAck(buf, refdate),
			in_pongAck(buf, syfreq), in_pongAck(buf, ackflags),
			in_pongAck(buf, plotid));
}

void upd_on_ack (lint ds, lint rd, word syfr, word ackf, word pi) {

	lint dd;
	word what = MARK_FF;

	if (sens_data.ee.s.f.status != SENS_COLLECTED ||
			sens_data.ee.ds != ds)
		return;

	if (rd != 0) { // set on the agg.
		// don't change for 'worse'
		if (rd < -SID * 90 || ref_date > -SID * 90) {
			if ((dd = wall_date (0) - rd) > TIME_TOLER ||
					dd < -TIME_TOLER) {
				ref_date = rd;
				ref_ts = seconds();
				what = MARK_DATE;
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
	sens_data.ee.s.f.status = SENS_CONFIRMED;
	trigger (ACK_IN);

	if (syfr != 0) {
		if (!is_synced) {
			set_synced;
			what = MARK_SYNC;
		}

		if (pong_params.freq_maj != syfr) {
			diag (OPRE_APP_ACK "sync freq change %u -> %u",
				pong_params.freq_maj, syfr);
			pong_params.freq_maj = syfr;
			if (what != MARK_SYNC)
				what = MARK_FREQ;
		}

	} else {
		if (is_synced) {
			clr_synced;
			what = MARK_SYNC;
		}
	}

	if (plot_id != pi) {
		plot_id = pi;
		what =  MARK_PLOT;
	}

	if (what != MARK_FF)
		write_mark (what);

	if (!is_eew_conf && ackf == 0) { // != 0 nack

		if (sens_data.eslot > 0) {
			sens_data.eslot--;
		} else {
			sens_data.ee.s.f.status = SENS_FF;
		}

		return;
	}

	if (ee_write (WNONE, sens_data.eslot * EE_SENS_SIZE,
		       (byte *)&sens_data.ee, EE_SENS_SIZE)) {
		app_diag (D_SERIOUS, "ee write failed %x %x",
				(word)(sens_data.eslot >> 16),
				(word)sens_data.eslot);
		if (sens_data.eslot > 0)
			sens_data.eslot--;
		else
			sens_data.ee.s.f.status = SENS_FF;
	}
}

void msg_setTag_in (char * buf) {

	word mmin, mem;
	char * out_buf = get_mem (WNONE, sizeof(msgStatsTagType));

	if (out_buf == NULL)
		return;

	upd_on_ack (in_setTag(buf, ds), in_setTag(buf, refdate),
			in_setTag(buf, syfreq), in_setTag(buf, ackflags),
			in_setTag(buf, plotid));

	if (in_setTag(buf, pow_levels) != 0xFFFF) {
		mmin = in_setTag(buf, pow_levels);
#if 0
		that was for just a single p.lev multiplied 4 times
		if (mmin > 7)
			mmin = 7;
		mmin |= (mmin << 4) | (mmin << 8) | (mmin << 12);
#endif
		if (pong_params.rx_lev != 0) // 'all' stays
			pong_params.rx_lev =
				max_pwr(mmin);

		if (pong_params.pload_lev != 0)
			pong_params.pload_lev = pong_params.rx_lev;

		pong_params.pow_levels = mmin;
	}

	if (in_setTag(buf, freq_maj) != 0xFFFF) {
		if (pong_params.freq_maj != in_setTag(buf, freq_maj)) {
			pong_params.freq_maj = in_setTag(buf, freq_maj);
			write_mark (MARK_FREQ);
		}

		if (pong_params.freq_maj != 0)
			tmpcrap (1);
	}

	if (in_setTag(buf, freq_min) != 0xFFFF)
		pong_params.freq_min = in_setTag(buf, freq_min);

	if (in_setTag(buf, rx_span) != 0xFFFF &&
			pong_params.rx_span != in_setTag(buf, rx_span)) {

		pong_params.rx_span = in_setTag(buf, rx_span);

		switch (in_setTag(buf, rx_span)) {
			case 0:
				tmpcrap (2); //killall (rxsw);
				net_opt (PHYSOPT_RXOFF, NULL);
				break;
			case 1:
				tmpcrap (2); //killall (rxsw);
				net_opt (PHYSOPT_RXON, NULL);
				break;
			default:
				tmpcrap (0);
		}
	}

	mem = memfree (0, &mmin);
	in_header(out_buf, hco) = 1; // no mhopping
	in_header(out_buf, msg_type) = msg_statsTag;
	in_header(out_buf, rcv) = in_header(buf, snd);
	in_statsTag(out_buf, hostid) = host_id;
	in_statsTag(out_buf, ltime) = seconds();
	in_statsTag(out_buf, c_fl) = handle_c_flags (in_setTag(buf, c_fl));

	// in_statsTag(out_buf, slot) is really # of entries
	if (sens_data.eslot == EE_SENS_MIN &&
			sens_data.ee.s.f.status == SENS_FF)
		in_statsTag(out_buf, slot) = 0;
	else
		in_statsTag(out_buf, slot) = sens_data.eslot -
			EE_SENS_MIN +1;

	in_statsTag(out_buf, maj) = pong_params.freq_maj;
	in_statsTag(out_buf, min) = pong_params.freq_min;
	in_statsTag(out_buf, span) = pong_params.rx_span;
	in_statsTag(out_buf, pl) = pong_params.pow_levels;
	in_statsTag(out_buf, mem) = mem;
	in_statsTag(out_buf, mmin) = mmin;

	// should be SETPOWER here... FIXME?
    	net_opt (PHYSOPT_TXON, NULL);
	send_msg (out_buf, sizeof(msgStatsTagType));
	net_opt (PHYSOPT_TXOFF, NULL);
	ufree (out_buf);
}

