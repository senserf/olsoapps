/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "diag.h"
#include "app_tag.h"
#include "msg_tag.h"
#include "oss_fmt.h"

#ifdef	__SMURPH__

#include "node_tag.h"
#include "stdattr.h"

#else	/* PICOS */

#include "net.h"

#endif	/* SMURPH or PICOS */

#include "attnames_tag.h"


__PUBLF (NodeTag, void, msg_pongAck_in) (char * buf) {

	upd_on_ack (in_pongAck(buf, ts), in_pongAck(buf, reftime),
			in_pongAck(buf, syfreq), in_pongAck(buf, ackflags));
}

__PUBLF (NodeTag, void, upd_on_ack) (lword ts, lword rt, word syfr, word ackf) {

	if (rt != 0) {
		ref_clock.sec = rt;
		ref_time = seconds();
	}

	if (sens_data.ee.s.f.status != SENS_COLLECTED ||
			sens_data.ee.ts != ts)
		return;

	//leds (LED_R, LED_OFF);
	sens_data.ee.s.f.status = SENS_CONFIRMED;
	trigger (ACK_IN);

	if (syfr != 0) {
		set_synced;

		if (pong_params.freq_maj != syfr) {
			diag (OPRE_APP_ACK "sync freq change %u -> %u",
				pong_params.freq_maj, syfr);
			pong_params.freq_maj = syfr;
		}

	} else {
		clr_synced;
	}

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
		if (sens_data.eslot > 0) { 
			sens_data.eslot--;
		} else {
			sens_data.ee.s.f.status = SENS_FF;
		}
	}
}

__PUBLF (NodeTag, void, msg_setTag_in) (char * buf) {
	word mmin, mem;
	char * out_buf = get_mem (WNONE, sizeof(msgStatsTagType));

	if (out_buf == NULL)
		return;

	upd_on_ack (in_setTag(buf, ts), in_setTag(buf, reftime),
			in_setTag(buf, syfreq), in_setTag(buf, ackflags));

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
		pong_params.freq_maj = in_setTag(buf, freq_maj);
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

