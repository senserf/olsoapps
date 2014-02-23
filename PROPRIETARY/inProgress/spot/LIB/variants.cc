/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2014.                          */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* Board-specifics (tag? peg?) */

#include "variants.h"

#ifdef BOARD_CHRONOS
#include "chro.h"
#ifndef PGMLABEL_peg
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
#endif

#endif

// not for pegs
#ifndef PGMLABEL_peg

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

#endif

