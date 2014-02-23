/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2014                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "variants.h"
#include "inout.h"
#include "diag.h"
#include "net.h"

// retry delay, try  nr, rx span, spare bits, power levels
pongParamsType  pong_params = { 5, 3, 2, 0, 0x7531};

#define PONG_DBG 0

fsm pong {

        word tr;  // many times forgot that tr is NOT on stack = 0;

	state LOAD:
		tr = 0;
		load_pframe ();
#if PONG_DBG
	app_diag_U ("PONG BEG (%u) retry %u.%u rx %u pl %x", (word)seconds(),
		pong_params.retry_del, pong_params.retry_nr,
		pong_params.rx_span, pong_params.pow_levels);
#endif
        state ITER:

                word   level;

                level = tr < 3 ?
			((pong_params.pow_levels >> (tr << 2)) & 0x000f) :
			pong_params.pow_levels 	>> 12;

                if (level > 0) {
			upd_pframe (level, tr);
                        net_opt (PHYSOPT_SETPOWER, &level);
			net_opt (PHYSOPT_RXON, NULL);

			talk (pong_frame, sizeof(msgPongType) +
				sizeof(pongPloadType), TO_NET);
#if PONG_DBG
                        app_diag_U ("PONG out (%u) l %u tr %u", (word)seconds(),
				level, tr);
#endif
                        when (TRIG_ACK, ACKIN);
			delay (pong_params.rx_span << 10, ROFF);

                } else {
#if PONG_DBG
                        app_diag_U ("PONG skip level (%u) %u", 
				(word)seconds(), tr);
#endif
			delay (pong_params.retry_del << 10, PAUSE);
                }

                release;

	state ROFF:
		net_opt (PHYSOPT_RXOFF, NULL);
#if PONG_DBG
		 app_diag_U ("PONG rxoff (%u)", (word)seconds());
#endif
		delay ((pong_params.retry_del - pong_params.rx_span) <<  10,
			PAUSE);
		release;

        state PAUSE:
                if (++tr < pong_params.retry_nr)
                        proceed ITER;

		trigger (TRIG_RONIN);
		proceed FIN;

	state ACKIN:
		trigger (TRIG_DORO);

	state FIN:
		net_opt (PHYSOPT_RXOFF, NULL); // superfluous doesn't hurt
#if PONG_DBG
		app_diag_U ("PONG ends (%u)", seconds());
#endif
                finish;
}
#undef PONG_DBG

