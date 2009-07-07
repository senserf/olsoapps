/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009.			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "msg.h"
#include "applib.h"
#include "sealists.h"
#include "net.h"
#include "storage.h"

#ifdef	__SMURPH__

#else

#include "applib_node_data.h"

#endif

// ============================================================================

// I have moved here the part that made sealists.c dependent on applib - so I
// can test sealists.c independently; you can move it back there once things
// work

lcdg_dm_obj_t *mkrmenu () {

	lword ep;
	word nc, i;
	word wc [SEA_ROFF_ME >> 1]; // up to Class

	lcdg_dm_obj_t *res;

#define	rem	((lcdg_dm_men_t*)res)

	if ((res = seal_mkrmenu ()) == NULL)
		return NULL;
	
	nc = rem -> NL;

	ufree (nbh_menu.mm); // harmless if NULL?

	// Extras not needed (I think), as no records are skipped, and thus line
	// numbers can be trivially converted to record pointers

	if ((nbh_menu.mm = (nbh_t *) umalloc (sizeof (nbh_t) * nc)) == NULL) {
		LCDG_DM_STATUS = LCDG_DMERR_NOMEM;
URet:
		lcdg_dm_free (res);
		return NULL;
	}

	nbh_menu.li = nc;

	for (ep = SEA_EOFF_REC, i = 0; i < nc; ep += SEA_RECSIZE) {

		if (ep >= SEA_EOFF_TXT) {
			LCDG_DM_STATUS = LCDG_DMERR_GARBAGE;
			goto URet;

		}

		// Up to and including ME
		ee_read (ep, (byte*)(&(wc[0])), SEA_ROFF_ME);

		nbh_menu.mm [i].id = wc [SEA_ROFF_ID >> 1];
		nbh_menu.mm [i].ts = 0;
		nbh_menu.mm [i].st = HS_SILE;

		// This is now a byte - as requested by Wlodek
		rem -> Lines [i][1] =
			nbh_menu.mm [i] . gr = (byte) wc [SEA_ROFF_CL >> 1];
		i++;
	}

	return res;
#undef	rem
}

// ============================================================================

void display_rec (word rh) {
//
// Display the record (picture + (optional) meter) represented by the index
//
        if (curr_rec != NULL) {
                ufree (curr_rec);
                curr_rec = NULL;
        }
        if ((curr_rec = seal_getrec (rh)) == NULL) {
		diag ("seal_getrec failed");
                // Failed to get the record
                return;
	}

        seal_disprec (curr_rec, cxt_flag.cxt == TOP_NH ? NO : YES);
        cxt_flag.top = TOP_DATA;
}

// ============================================================================

void paint_scr (word c) {
	word i;

	if (nbh_menu.scr == 0)
		return;

	i = 10 + nbh_menu.scr * 120 / nbh_menu.num - 1; // bottom

	lcdg_set (0, i - 9, 4, i);
	lcdg_setc (c, c);
	lcdg_clear ();
}

// ============================================================================

static word color_c0 (word i) {

	// traditional(?) char -> word
	switch (lcd_menu->Lines [i][0]) {

		case MLI0_NOSY:
			return COL_NOSY;

		case MLI0_SYMM:
			return COL_SYMM;

		case MLI0_ISND:
			return COL_ISND;

		case MLI0_IRCV:
			return COL_IRCV;

		case MLI0_MATC:
			return COL_MATC;

		case MLI0_DECL:
			return COL_DECL;

		case MLI0_IDEC:
			return COL_IDEC;
	}
	return WNONE;
}

// ============================================================================

static void paint_c0 (word i) {
	word co, j = 0, po = 0;

	if (i > 2 || !(nbh_menu.cid & nbh_menu.mm[i].id) ||
			(co = color_c0 (i)) == WNONE)
		return;

	while (j < i) {
		if (nbh_menu.cid & nbh_menu.mm[j].id)
			po++;
		j++;
	}

	po = 10 + po * 120 / nbh_menu.num;
	lcdg_set (125, po, 129, po + 5);
	lcdg_setc (co , co);
	lcdg_clear ();
}

// ============================================================================

void paint_nh () {
	word i = handle_nbh (NBH_FIND, nbh_menu.cid);

	if (nbh_menu.num == 0) {
		lcdg_dm_dtop ();
		return;
	}

	if (i == WNONE)
		return;

	display_rec (i);
	// what a crap! this is not TOP_DATA, restore top after display_rec():
	// double crap: single node is a schizo - treat it as TOP_NH:
	if (i > 2 || nbh_menu.num == 1)
		cxt_flag.top = TOP_NH;

	nbh_menu.num = 0;
	for (i = 0; i < 3; i++)
		if (nbh_menu.cid & nbh_menu.mm[i].id)
			nbh_menu.num++;

	if (nbh_menu.num == 1 || nbh_menu.scr > nbh_menu.num)
		nbh_menu.scr = nbh_menu.num;

	paint_c0 (0); paint_c0 (1); paint_c0 (2);
	paint_scr (COLOR_WHITE);
}

// ============================================================================

void update_line (word sel, word i, word s, word c0) {

    switch (sel) {

	case ULSEL_C0:

		nbh_menu.mm[i].st = s;
		lcd_menu->Lines [i][0] = c0;

		if (cxt_flag.cxt != TOP_NH || i > 2)
			break;

		switch (c0) {
			case MLI0_INNH:
				nbh_menu.cid |= nbh_menu.mm[i].id;
				nbh_menu.num++;
				paint_nh ();
				break;

			case MLI0_GONE:
				nbh_menu.cid &= ~nbh_menu.mm[i].id;
				nbh_menu.num--;
				paint_nh ();
				break;

			default:
				paint_c0 (i);
		}
		break;

	case ULSEL_C1:

	    // AFAICT, the proper character is there already
	    switch ((char)(nbh_menu.mm[i].gr)) {

		case MLI1_YE:
			nbh_menu.mm[i].gr = MLI1_IY;
			lcd_menu->Lines [i][1] = MLI1_IY;
			break;
		case MLI1_NO:
			 nbh_menu.mm[i].gr = MLI1_IN;
			 lcd_menu->Lines [i][1] = MLI1_IN;
			 break;
		case MLI1_IY:
			 nbh_menu.mm[i].gr = MLI1_YE;
			 lcd_menu->Lines [i][1] = MLI1_YE;
			 break;
		case MLI1_IN:
			 nbh_menu.mm[i].gr = MLI1_NO;
			 lcd_menu->Lines [i][1] = MLI1_NO;
	    }
    }

    if (cxt_flag.top == TOP_HIER && LCDG_DM_TOP == (lcdg_dm_obj_t *)lcd_menu)
	lcdg_dm_update (lcd_menu, i);
}

void init_glo () {

	local_host = (word)host_id;

	// msg
	tarp_ctrl.param = 0xB1; // level 2, rec 3, slack 0, fwd on
	memset (&myBeac, 0, sizeof(msgBeacType));
	memset (&myAct, 0, sizeof(msgActType));
	myBeac.header.msg_type = MSG_BEAC;
	myBeac.header.hco = 1;
	myAct.header.hco = 1;

	// applib
	memset (&rf_rcv, 0, sizeof(rf_rcv_t));
	memset (&ad_rcv, 0, sizeof(rf_rcv_t));
	memset (&nbh_menu, 0, sizeof(nbh_menu_t));
	lcd_menu = NULL;
	ad_buf = NULL;
	curr_rec = NULL;
}

word handle_ad (word act, word which) {

    sea_rec_t *rec;
    lcdg_dm_obj_t *im;

    switch (act) {

	case AD_GOT:

	    if (ad_rcv.buf != NULL ||
		((rec = seal_getrec (which)) == NULL)) {
		msg_reply (HS_DECL);

	    } else {

		msg_reply (HS_MATC);
		ad_rcv.buf = rf_rcv.buf;
		ad_rcv.len = rf_rcv.len;
		rf_rcv.buf = NULL;
		rf_rcv.len = 0;

		if (rec->IM != WNONE && (im = lcdg_dm_newimage_e ((lword)
		    (rec->IM + SEA_EOFF_TXT))) != NULL) {

			lcdg_dm_display (im);
			lcdg_dm_free (im);
		}
			
		lcdg_font (2);

		// improvise... 
		ad_rcv.buf[ad_rcv.len -1] = '\0';
		switch (nbh_menu.mm[which].id) {
			case AD_GBN_ID:
				lcdg_setc (COLOR_BLUE, COLOR_GREEN);
				lcdg_sett (7, 40, 11, 1);
				//lcdg_sett (10, 43, 8, 1);
				//lcdg_sett (10, 43, 16, 1);
				break;

			case EV_ORG_ID:
				lcdg_setc (COLOR_BLACK, COLOR_WHITE);
				lcdg_sett (5, 83, 11, 1);
				break;

			default:
				lcdg_setc (COLOR_BROWN, COLOR_WHITE);
				lcdg_sett (10, 66, 16, 1);
		}
		lcdg_wl (&ad_rcv.buf[sizeof(msgAdType)], 0, 0, 0);
		ufree (rec);
		cxt_flag.top = TOP_AD;
	    }
	    break;

	case AD_RPLY:
	    in_header(ad_rcv.buf, msg_type) = MSG_ADACK;
	    in_header(ad_rcv.buf, rcv) = in_header(ad_rcv.buf, snd);
	    in_adAck(ad_rcv.buf, ack) = which;
	    net_tx (WNONE, ad_rcv.buf, sizeof(msgAdAckType), 0);
	    // fall through

	case AD_CANC:
	    ufree (ad_rcv.buf);
	    ad_rcv.buf = NULL;
	    ad_rcv.len = 0;
    }
    return 0;
}

void process_incoming () {

    word i = handle_nbh (NBH_FIND, in_header (rf_rcv.buf, snd));

    if (in_header (rf_rcv.buf, msg_type) != MSG_BEAC)
        diag ("Got %u.%u fr %u (%u) t%u", in_header (rf_rcv.buf, msg_type),
		    in_header (rf_rcv.buf, seq_no),
		    in_header (rf_rcv.buf, snd), i, (word)seconds());
    // beacons
    if (in_header(rf_rcv.buf, msg_type) == MSG_BEAC) {
	if (i != WNONE) {
		nbh_menu.mm[i].ts = (word)seconds();
		if (nbh_menu.mm[i].st == HS_SILE)
			update_line (ULSEL_C0, i, HS_BEAC, MLI0_INNH);
	}
	return;
    }

    // all else unwanted
    if (i == WNONE || nbh_menu.mm[i].gr != MLI1_YE) {
	msg_reply (HS_NOSY);
	return;
    }

    if (in_header(rf_rcv.buf, msg_type) == MSG_AD) {
	handle_ad (AD_GOT, i);
	return;
    }

    // for now
    if (in_header(rf_rcv.buf, msg_type) == MSG_ADACK) {

	diag ("Ad Ack:%u from %u ref %u", in_adAck(rf_rcv.buf, ack),
		in_header(rf_rcv.buf, snd), in_adAck(rf_rcv.buf, ref));
	return;
    }

    if (in_header(rf_rcv.buf, msg_type) != MSG_ACT)
	    return;

    // only incoming actions (MSG_ACT) left
    switch (in_act(rf_rcv.buf, act)) {

	case HS_ISND:

	    switch ((nbh_menu.mm[i].st)) {

		case HS_SILE:
		case HS_BEAC:
			update_line (ULSEL_C0, i, HS_IRCV, MLI0_IRCV);
			// fall through
		case HS_IRCV:
			msg_reply (HS_IRCV);
			break;

		case HS_IDEC:
			msg_reply (HS_DECL);
			break;

		case HS_NOSY:
		case HS_DECL:
		case HS_SYMM:
		case HS_ISND:
			update_line (ULSEL_C0, i, HS_MATC, MLI0_MATC);
			// fall through 
		case HS_MATC:
			msg_reply (HS_MATC);
	    }
	    break;

	case HS_IRCV:

	    if (nbh_menu.mm[i].st == HS_ISND)
		    update_line (ULSEL_C0, i, HS_SYMM, MLI0_SYMM);
	    break; // ignore for anything else

	case HS_NOSY:
	    if (nbh_menu.mm[i].st != HS_NOSY)
		    update_line (ULSEL_C0, i, HS_NOSY, MLI0_NOSY);
	    break;

	case HS_MATC:
	    if (nbh_menu.mm[i].st != HS_MATC)
		    update_line (ULSEL_C0, i, HS_MATC, MLI0_MATC);
	    break;

	case HS_DECL:
	    update_line (ULSEL_C0, i, HS_DECL, MLI0_DECL);

        // _SILE, _BEAC, _SYMM can't come in
    }
}

// expecting more 'whats' and trading op speed for code size...
word handle_nbh (word what, word id) {
    word i, j = nbh_menu.li;

    for (i = 0; i < j; i++) {
	switch (what) {

	    case NBH_INIT:
		    nbh_menu.mm[i].ts = 0;
		    nbh_menu.mm[i].st = HS_SILE;
		    if (nbh_menu.mm[i].gr == MLI1_IY)
			    nbh_menu.mm[i].gr = MLI1_YE;
		    if (nbh_menu.mm[i].gr == MLI1_IN)
			    nbh_menu.mm[i].gr = MLI1_NO;
		    lcd_menu->Lines [i][0] = MLI0_INIT;
		    break;

	    case NBH_AUDIT:
		if (nbh_menu.mm[i].st != HS_SILE &&
		    (word)seconds() - nbh_menu.mm[i].ts > BEAC_FREQ * 3)
			update_line (ULSEL_C0, i, HS_SILE, MLI0_GONE);
		break;

	    case NBH_FIND:

		if (nbh_menu.mm[i].id == id)
			return i;
	}
    }
    return WNONE;
}
