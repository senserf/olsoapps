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

// this could be in pong.cc as well... we'll see if some board-specifics
// will come...

char pong_frame [sizeof(msgPongType) + sizeof(pongPloadType)];

// I truly dislike this idea of stashing alarms, even if I understand this may
// be needed in some praxes... That's why I want a solid pong_frame and
// dynamic stash, even if it seems weird and a bit wasteful.
// It is tempting to stash not thw whole frames, but info to load them;
// however it wouldn't be truly frame stashing and sooner or later we would
// have to maintain a separate struct.
fifek_t pframe_stash;

#ifdef BOARD_CHRONOS
#include "chro_tag.h"
void load_pframe () {
	char * pf;

    in_pong(pong_frame, pd.dupeq) += 1;

    if (fifek_empty (&pframe_stash)) {
	in_pong(pong_frame, pd.alrm_id) = chronos.alrm_id;
	in_pong(pong_frame, pd.alrm_seq) = chronos.alrm_seq;
	in_pong(pong_frame, pd.fl2) = chronos.acc_mode;
	in_pongPload(pong_frame, volt) = chronos.volt;
	in_pongPload(pong_frame, move_ago) =
		(word)(seconds() - chronos.move_ts);
	in_pongPload(pong_frame, move_nr) = chronos.move_nr;
    } else {
        pf = fifek_pull (&pframe_stash);
        in_pong(pong_frame, pd.alrm_id) = in_pong(pf, pd.alrm_id);
        in_pong(pong_frame, pd.alrm_seq) = in_pong(pf, pd.alrm_seq);
        in_pong(pong_frame, pd.fl2) = in_pong(pf, pd.fl2);
        in_pongPload(pong_frame, volt) = in_pongPload(pf, volt);
        in_pongPload(pong_frame, move_ago) =
                (word)(seconds() - in_pongPload(pf, move_ago));
        in_pongPload(pong_frame, move_nr) = in_pongPload(pf, move_nr);
	ufree (pf);
    }
}

// we're overwriting alrms older than the last and this - it is trivial to
// increase the stash - 2nd param in call to fifek_ini().
void stash_pframe () {
	char * pf;

	if (chronos.alrm_id == 0)
		return; // only alrms count

	if ((pf = get_mem (sizeof(msgPongType) + sizeof(pongPloadType), 
			NO)) == NULL)
		return;

        in_pong(pf, pd.alrm_id) = chronos.alrm_id;
        in_pong(pf, pd.alrm_seq) = chronos.alrm_seq;
        in_pong(pf, pd.fl2) = chronos.acc_mode;
        in_pongPload(pf, volt) = chronos.volt;
        in_pongPload(pf, move_ago) = chronos.move_ts;
        in_pongPload(pf, move_nr) = chronos.move_nr;
	fifek_push(&pframe_stash, pf);
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
	char * pf;

    in_pong(pong_frame, pd.dupeq) += 1;
    in_pong(pong_frame, pd.fl2)++; // vy not?
    in_pongPload(pong_frame, steady_shit)++;

    if (fifek_empty (&pframe_stash)) {
        in_pong(pong_frame, pd.alrm_id) = warsaw.alrm_id;
        in_pong(pong_frame, pd.alrm_seq) = warsaw.alrm_seq;
        in_pongPload(pong_frame, volt) = warsaw.volt;
	in_pongPload(pong_frame, random_shit) = warsaw.random_shit;
    } else {
	pf = fifek_pull (&pframe_stash);
	in_pong(pong_frame, pd.alrm_id) = in_pong(pf, pd.alrm_id);
	in_pong(pong_frame, pd.alrm_seq) = in_pong(pf, pd.alrm_seq);
	in_pongPload(pong_frame, volt) = in_pongPload(pf, volt);
	in_pongPload(pong_frame, random_shit) = in_pongPload(pf, random_shit);
	ufree (pf);
    }
}

// we're overwriting alrms older than the last and this - it is trivial to
// increase the stash - 2nd param in call to fifek_ini().
void stash_pframe () {
        char * pf;

        if (warsaw.alrm_id == 0)
                return; // only alrms count

        if ((pf = get_mem (sizeof(msgPongType) + sizeof(pongPloadType),
                        NO)) == NULL)
                return;

        in_pong(pf, pd.alrm_id) = warsaw.alrm_id;
        in_pong(pf, pd.alrm_seq) = warsaw.alrm_seq;
        in_pongPload(pf, volt) = warsaw.volt;
        in_pongPload(pf, random_shit) = warsaw.random_shit;
	fifek_push(&pframe_stash, pf);
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

// the same for all boards (so far)
void init_pframe () {
                in_header(pong_frame, msg_type) = msg_pong;
                in_header(pong_frame, rcv) = 0;
                in_header(pong_frame, hco) = 1;
                in_header(pong_frame, prox) = 1;
                in_pong(pong_frame, pd.btyp) = BTYPE;
                in_pong(pong_frame, pd.len) = sizeof(pongPloadType);
		fifek_ini (&pframe_stash, 2); // 2-elem alarm stash
};

void upd_pframe (word pl, word tnr) {
	in_pong(pong_frame, pd).plev = pl;
	in_pong(pong_frame, pd).trynr = tnr;
}

