/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "msg.h"
#include "applib.h"
#include "sealists.h"
#include "net.h"

lcdg_dm_men_t 	* lcd_menu;
nbh_menu_t	nbh_menu;
rf_rcv_t	rf_rcv, ad_rcv;
word		top_flag;
char 		* ad_buf;
sea_rec_t	* curr_rec;

extern const lword host_id;

void update_line (word sel, word i, word s, word c0) {
    switch (sel) {
	case ULSEL_C0:
		nbh_menu.mm[i].st = s;
		lcd_menu->Lines [i][0] = c0;
		break;

	case ULSEL_C1:
	    switch (nbh_menu.mm[i].gr) {
		case GR_YE:
			nbh_menu.mm[i].gr = GR_IY;
			lcd_menu->Lines [i][1] = MLI1_IY;
			break;
		case GR_NO:
			 nbh_menu.mm[i].gr = MLI1_IN;
			 lcd_menu->Lines [i][1] = MLI1_IN;
			 break;
		case GR_IY:
			 nbh_menu.mm[i].gr = GR_YE;
			 lcd_menu->Lines [i][1] = MLI1_YE;
			 break;
		case GR_IN:
			 nbh_menu.mm[i].gr = GR_NO;
			 lcd_menu->Lines [i][1] = MLI1_NO;
	    }
    }

    if (top_flag == TOP_HIER && LCDG_DM_TOP == (lcdg_dm_obj_t *)lcd_menu)
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
	memset (&rf_rcv, 0, sizeof(nbh_menu_t));
	memset (&nbh_menu, 0, sizeof(nbh_menu_t));
	lcd_menu = NULL;
	ad_buf = NULL;
	top_flag = TOP_HIER;
	curr_rec = NULL;
}

int handle_ad (word act, word which) {
    sea_rec_t *rec;

    switch (act) {

	case AD_GOT:

	    if (ad_rcv.buf != NULL ||
		((rec = seal_getrec (lcd_menu->Extras [which])) == NULL)) {
		msg_reply (HS_DECL);

	    } else {
		msg_reply (HS_MATC);
		ad_rcv.buf = rf_rcv.buf;
		ad_rcv.len = rf_rcv.len;
		rf_rcv.buf = NULL;
		rf_rcv.len = 0;
		if (rec->IM != WNONE)
			lcdg_im_disp (rec->IM, 0, 0);
		lcdg_font (2);

		// improvise... 
		ad_rcv.buf[ad_rcv.len -1] = '\0';
		switch (nbh_menu.mm[which].id) {
			case AD_OLSO_ID:
				lcdg_setc (COLOR_WHITE, COLOR_BLUE);
				lcdg_sett (64, 10, 8, 1);
				lcdg_wl (&ad_rcv.buf[sizeof(msgAdType)],0,0,0);
				break;

			case AD_COMBAT_ID:
				lcdg_setc (COLOR_BLACK, COLOR_WHITE);
				lcdg_sett (0, 43, 16, 1);
				break;

			default:
				lcdg_setc (COLOR_BROWN, COLOR_WHITE);
				lcdg_sett (0, 66, 16, 1);
		}
		lcdg_wl (&ad_rcv.buf[sizeof(msgAdType)], 0, 0, 0);
		seal_freerec (rec);
		top_flag = TOP_AD;
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

    int i = handle_nbh (NBH_FIND, in_header (rf_rcv.buf, snd));

    if (in_header (rf_rcv.buf, msg_type) != MSG_BEAC)
        diag ("Got %u.%u fr %u (%u) t%u", in_header (rf_rcv.buf, msg_type),
		    in_header (rf_rcv.buf, seq_no),
		    in_header (rf_rcv.buf, snd), i, (word)seconds());
    // beacons
    if (in_header(rf_rcv.buf, msg_type) == MSG_BEAC) {
	if (i >= 0) {
		nbh_menu.mm[i].ts = (word)seconds();
		if (nbh_menu.mm[i].st == HS_SILE)
			update_line (ULSEL_C0, i, HS_BEAC, MLI0_INNH);
	}
	return;
    }

    // all else unwanted
    if (i < 0 || nbh_menu.mm[i].gr != GR_YE) {
	msg_reply (HS_NOSY);
	return;
    }

    if (in_header(rf_rcv.buf, msg_type) == MSG_AD) {
	handle_ad (AD_GOT, i);
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
int handle_nbh (word what, word id) {
    int i, j = nbh_menu.li;

    for (i = 0; i < j; i++) {
	switch (what) {

	    case NBH_INIT:
		    nbh_menu.mm[i].ts = 0;
		    nbh_menu.mm[i].st = HS_SILE;
		    if (nbh_menu.mm[i].gr == GR_IY)
			    nbh_menu.mm[i].gr = GR_YE;
		    if (nbh_menu.mm[i].gr == GR_IN)
			    nbh_menu.mm[i].gr = GR_NO;
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
    return -1;
}


