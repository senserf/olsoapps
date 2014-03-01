/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2014.                          */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* Board-specifics... for SPOT it seems that this is only for tags */
/* Note that variants.h where ploead types are defined is NOT just for tags */

#ifdef PGMLABEL_peg
#error nur fur tags
error nur fur tags
#endif

#include "variants.h"

#ifdef BOARD_CHRONOS
#include "chro_tag.h"
void load_pframe () {
	in_pong(pong_frame, pd.dupeq) += 1;
	in_pong(pong_frame, pd.alrm_id) = chronos.alrm_id;
	in_pong(pong_frame, pd.alrm_seq) = chronos.alrm_seq;
	in_pong(pong_frame, pd.fl2) = chronos.acc_mode;
	in_pongPload(pong_frame, volt) = chronos.volt;
	in_pongPload(pong_frame, move_ago) =
		(word)(seconds() - chronos.move_ts);
	in_pongPload(pong_frame, move_nr) = chronos.move_nr;
}

void set_alrm (word a) {
	chronos.alrm_id = a;
	chronos.alrm_seq++;
	chro_hi ("ALAR");
	chro_lo ("ON");
	if (!running (beep))
		runfsm beep(1);
	trigger (TRIG_ALRM);
}

void clr_alrm () {
	chronos.alrm_id = 0;
}

#endif

#if defined BOARD_WARSAW || defined BOARD_WARSAW_BLUE
#include "war_tag.h"
void load_pframe () {
        in_pong(pong_frame, pd.dupeq) += 1;
        in_pong(pong_frame, pd.alrm_id) = warsaw.alrm_id;
        in_pong(pong_frame, pd.alrm_seq) = warsaw.alrm_seq;
        in_pong(pong_frame, pd.fl2)++; // vy not?
        in_pongPload(pong_frame, volt) = warsaw.volt;
	in_pongPload(pong_frame, random_shit) = warsaw.random_shit;
	in_pongPload(pong_frame, steady_shit)++;
}

void set_alrm (word a) {
	warsaw.random_shit = (word)rnd();
	warsaw.alrm_id = a > 7 ? 7 : a;
	warsaw.alrm_seq++;
	trigger (TRIG_ALRM);
}

void clr_alrm () {
	warsaw.alrm_id = 0;
	warsaw.random_shit = 0;
}

#endif

// this could be in pong.cc as well... we'll see if some board-specifics
// will come...
char pong_frame [sizeof(msgPongType) + sizeof(pongPloadType)];

void init_pframe () {
                in_header(pong_frame, msg_type) = msg_pong;
                in_header(pong_frame, rcv) = 0;
                in_header(pong_frame, hco) = 1;
                in_header(pong_frame, prox) = 1;
                in_pong(pong_frame, pd.btyp) = BTYPE;
                in_pong(pong_frame, pd.len) = sizeof(pongPloadType);
};

void upd_pframe (word pl, word tnr) {
	in_pong(pong_frame, pd).plev = pl;
	in_pong(pong_frame, pd).trynr = tnr;
}

