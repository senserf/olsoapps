/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "diag.h"
#include "app_tag.h"
#include "msg_tag.h"

#ifdef	__SMURPH__

#include "node_tag.h"
#include "stdattr.h"

#else	/* PICOS */

#include "net.h"

#endif	/* SMURPH or PICOS */

#include "attnames_tag.h"

__PUBLF (NodeTag, void, msg_pongAck_in) (char * buf) {
	byte b;

	if (in_pongAck(buf, reftime) != 0) {
		ref_clock.sec = in_pongAck(buf, reftime);
		ref_time = seconds();
	}

	if (sens_data.ee.status = SENS_COLLECTED &&
			sens_data.ee.ts == in_pongAck(buf, ts)) {
		leds (0, 0);
		b = sens_data.ee.status = SENS_CONFIRMED;

		if (sens_data.eslot == EE_SENS_MAX -1)
			return; // don;t update the last slot: eeprom full

		if (ee_write (WNONE, sens_data.eslot * EE_SENS_SIZE, &b, 1)) {
			app_diag (D_SERIOUS, "ee upd failed %x %x",
					(word)(sens_data.eslot >> 16),
					(word)sens_data.eslot);
		}

	} else
		app_diag (D_DEBUG, "Redundant pongAck %x %x from %u",
				(word)(sens_data.eslot >> 16),
				(word)sens_data.eslot,
				in_header(buf, snd));
}

__PUBLF (NodeTag, void, msg_setTag_in) (char * buf) {
	word mmin, mem;
	char * out_buf = get_mem (WNONE, sizeof(msgStatsTagType));

	if (out_buf == NULL)
		return;

	if (in_setTag(buf, pow_levels) != 0xFFFF) {
		mmin = in_setTag(buf, pow_levels);
		if (mmin > 7)
			mmin = 7;
		mmin |= (mmin << 4) | (mmin << 8) | (mmin << 12);

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

	// in_statsTag(out_buf, slot) is really # of entries
	if (sens_data.eslot == EE_SENS_MIN && sens_data.ee.status == SENS_FF)
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

