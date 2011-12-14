#include "app.h"
#include "storage.h"

/*
 * Illustrates images, menus, OEP
 */

// ============================================================================

//+++ "hostid.c"

// ============================================================================

#ifdef __SMURPH__

#define	SFD		_daprx (SFD)
#define	objects		_daprx (objects)
#define	ibuf		_daprx (ibuf)

threadhdr (root, SeaNode) {

#include "app_root_static.h"

	states {
			RS_INIT,
			RS_RDCMD,
			RS_RDCME,
			RS_SST,
			RS_DSP,
			RS_CRE,
			RS_GME,
			RS_GME1,
			RS_GTE,
			RS_GTE1,
			RS_LIS,
			RS_LDM,
			RS_LDM1,
			RS_LIM,
			RS_LIM1,
			RS_DIS,
			RS_ERA,
			RS_SEN,
			RS_RCV,
			RS_ISW,
			RS_ESW,
			RS_MEM,
			RS_MEM1,
			RS_FME,
			RS_LNB,
			RS_NBH,
			RS_HOS,
			RS_HOS1
	};

	perform;
};

#else

#define RS_INIT         0
#define RS_RDCMD        1
#define RS_RDCME        2
#define RS_SST          3
#define RS_DSP          4
#define RS_CRE          5
#define RS_GME          6
#define RS_GME1         7
#define RS_GTE          8
#define RS_GTE1         9
#define RS_LIS          10
#define RS_LDM          11
#define RS_LDM1         12
#define RS_LIM          13
#define RS_LIM1         14
#define RS_DIS          15
#define RS_ERA          16
#define RS_SEN          17
#define RS_RCV          18
#define RS_ISW          19
#define RS_ESW          20
#define RS_MEM          21
#define RS_MEM1         22
#define RS_FME          23
#define RS_LNB          24
#define RS_NBH          25
#define RS_HOS          26
#define RS_HOS1         27

#include "app_node_data.h"

#endif

// ============================================================================
// ============================================================================
// ============================================================================

static Boolean free_object (word ix) {
//
// Deallocate an object
//
	lcdg_dm_obj_t *co;

	if ((co = objects [ix]) == NULL)
		return NO;

	if (lcdg_dm_shown (co))
		return YES;

	lcdg_dm_free (co);

	return NO;
}

// ============================================================================
// ============================================================================

// very very temporary and crappy

static void disp_cats () {

	int i, j;

	j = 0;
	lcdg_font (0);
	lcdg_setc (SEA_MENU_CATEV_BG, SEA_MENU_CATEV_FG);

	for (i = 0; i < 16; i++) {
		if ((curr_rec->MCats >> i) & 1) {
			lcdg_sett (0, 10 + j*8, 21, 1);
			lcdg_wl (&((lcdg_dm_men_t*)(objects [1]))->Lines[i][3],
				0, 0, 0);
			if (j < 15)
				j++;
		}
	}

	lcdg_setc (SEA_MENU_CATMY_BG, SEA_MENU_CATMY_FG);
	for (i = 0; i < 16; i++) {
		if ((curr_rec->ECats >> i) & 1) {
			lcdg_sett (0, 10 + j*8, 21, 1);
			lcdg_wl (&((lcdg_dm_men_t*)(objects [0]))->Lines[i][3],
				0, 0, 0);
			if (j < 15)
				j++;
		}
	}
}

// ============================================================================

static void preset_menus () {

	word i, j;

	for (i = 0; i < 4; i++)
		if (objects [i] != NULL)
			return;

	objects [0] = seal_mkcmenu (NO);
	objects [1] = seal_mkcmenu (YES);
	lcd_menu  = (lcdg_dm_men_t*)mkrmenu ();
	if (objects [0] == NULL || objects [1] == NULL ||
			lcd_menu == NULL) {
		diag ("Amen"); // leds
		return; // halt();
	}
	if (nbh_menu.li < 4)
		goto RetHier;

	// 3 fist must be 1 bin digit, or demo nh bitmaps are blocked
	for (i = 0; i < 3; i++) {
		j = 0x8000;
		while (j) {
			if (j == nbh_menu.mm [i].id)
				break;
			j >>= 1;
		}
		if (j == 0)
			goto RetHier;
	}

	cxt_flag.top = cxt_flag.cxt = TOP_NH;
	return;
RetHier:
	cxt_flag.top = TOP_HIER;
	cxt_flag.cxt = TOP_NONH;
	lcdg_dm_newtop ((lcdg_dm_obj_t*)lcd_menu);
}

static void top_switch () {
//
// Switch views
//
	if (LCDG_DM_HEAD == NULL) {
		diag ("To LIV");
		lcdg_dm_refresh();
		cxt_flag.top =  cxt_flag.cxt = TOP_HIER;
		lcdg_dm_newtop ((lcdg_dm_obj_t*)lcd_menu);

	} else if (cxt_flag.cxt != TOP_NONH) {
			diag ("To NBV");
			lcdg_dm_remove (NULL);
			cxt_flag.top = cxt_flag.cxt = TOP_NH;
			paint_nh ();
			diag ("All painted");
	}
}

// ============================================================================

static void buttons (word but) {

	nbh_t * mmm = NULL;
	word i =0, skip;

	if (but == BUTTON_1) {
		diag ("Contrast?");
		return;
	}

	if (cxt_flag.top == TOP_AD) {
		if (but == JOYSTICK_N)
			handle_ad (AD_RPLY, 1);
		else if (but == JOYSTICK_S)
			handle_ad (AD_RPLY, 0);
		else
			handle_ad (AD_CANC, 0);
	}

	if (but == BUTTON_0) {
		top_switch();
		return;
	}

	if (cxt_flag.top == TOP_AD)
		goto Refresh;

	if (cxt_flag.cxt == TOP_NH) {
		if (nbh_menu.scr != 0) {
			skip = nbh_menu.scr;
			for (i = 0; i < 3; i++)
				if (nbh_menu.cid & nbh_menu.mm[i].id &&
						--skip == 0)
					break;
			mmm = &nbh_menu.mm[i];
		}
	} else if (lcd_menu) {
		i = lcdg_dm_menu_c(lcd_menu);
		mmm = &nbh_menu.mm [i];
	}

	if (mmm)
		diag ("butt found %u %u", i, mmm->id);
	else {
		diag ("butt null mmm");
		if (but != JOYSTICK_W && but != JOYSTICK_E)
			return;
	}

	if (but == JOYSTICK_PUSH) { // drill down
		if (cxt_flag.top == TOP_HIER || (cxt_flag.top == TOP_NH &&
				nbh_menu.num > 1)) {
			display_rec (i);
			return;
		}
		if (cxt_flag.top == TOP_DATA) {
			if (cxt_flag.cxt != TOP_NH) {
				disp_cats();
				return;
			}
			goto Refresh;
		}
		return;
	}

	if (but == JOYSTICK_W) { // up
		if (cxt_flag.top == TOP_DATA)
			goto Refresh;
		if (cxt_flag.cxt != TOP_NH)
			lcdg_dm_menu_u ((lcdg_dm_men_t*)LCDG_DM_TOP);
		else if (nbh_menu.num) {
			paint_scr (COLOR_BLACK);
			if (nbh_menu.scr < 2)
				nbh_menu.scr = nbh_menu.num;
			else
				nbh_menu.scr--;
			paint_scr (COLOR_WHITE);
		}
		return;
	}

	if (but == JOYSTICK_E) {
		if (cxt_flag.top == TOP_DATA)
			goto Refresh;
		if (cxt_flag.cxt != TOP_NH)
			lcdg_dm_menu_d ((lcdg_dm_men_t*)LCDG_DM_TOP);
		else if (nbh_menu.num) {
			paint_scr (COLOR_BLACK);
			// .scr is :2
			if (nbh_menu.scr == 3 ||
					++nbh_menu.scr > nbh_menu.num)
				nbh_menu.scr = 1;
			paint_scr (COLOR_WHITE);
		}
		return;
	}

	// only positive _N (left) and negative _S (right) left
	// keep the group +/-, even if now it doesn't make sense:
	if (mmm->gr != MLI1_YE)
		goto Refresh;

	if (but == JOYSTICK_N) {
		// this cascade may be 'entered' at any if:
		if (mmm->st == HS_BEAC)
			update_line (ULSEL_C0, i, HS_ISND, MLI0_ISND);

		if (mmm->st == HS_IRCV || mmm->st == HS_IDEC)
			update_line (ULSEL_C0, i, HS_MATC, MLI0_MATC);

		if (mmm->st == HS_ISND || mmm->st == HS_MATC)
			msg_send (MSG_ACT, mmm->id, 1, mmm->st, 0, 0, NULL);

		goto Refresh;
	}
#if 0
that was 'ignore' for TOP_HIER:
if (LCDG_DM_TOP == (lcdg_dm_obj_t *)lcd_menu)
	update_line (ULSEL_C1, i, 0, 0);
#endif


	if (but == JOYSTICK_S) {

		if (mmm->st == HS_IRCV) {
			update_line (ULSEL_C0, i, HS_IDEC, MLI0_IDEC);
			msg_send (MSG_ACT, mmm->id, 1, HS_DECL, 0, 0, NULL);
		}
		goto Refresh;
	}

Refresh:

	if (cxt_flag.top != TOP_NH && cxt_flag.top != TOP_HIER) {

		cxt_flag.top = cxt_flag.cxt == TOP_NH ? TOP_NH : TOP_HIER;

		if (cxt_flag.cxt == TOP_NH)
			paint_nh ();
		else
			lcdg_dm_refresh ();
	}
}

// ============================================================================

static void fibuf () {

	if (ibuf) {
		ufree (ibuf);
		ibuf = NULL;
	}
}

thread (root)

    word i;
    char *line;
    lword ef, el;

// With __SMUPRH__, the dirst include (at the top of this file) will take
// precedence
#include "app_root_static.h"

    entry (RS_INIT)

	ee_open ();

	init_glo();

	if (net_init (INFO_PHYS_CC1100, INFO_PLUG_TARP) < 0)
		syserror (ERESOURCE, "cc1100");
	phys_uart (1, OEP_MAXRAWPL, 0);
	tcv_plug (1, &plug_null);
	oep_setphy (1);

	// OSS link
	SFD = tcv_open (WNONE, 1, 1);
	if (SFD < 0)
		syserror (ENODEVICE, "uart");

	// Initialize EEPROM
	lcdg_im_init (SEA_FIMPAGE, 0);

	// Initialize OEP
	if (oep_init () == NO)
		syserror (ERESOURCE, "oep_init");

	// Initialize the PHY
	i = 0xffff;
	tcv_control (SFD, PHYSOPT_SETSID, &i);
	tcv_control (SFD, PHYSOPT_TXON, NULL);
	tcv_control (SFD, PHYSOPT_RXON, NULL);

	// Buttons
	buttons_action (buttons);

	// Initialize the AB UART link
	ab_init (SFD);
	ab_mode (AB_MODE_ACTIVE);

	// And run the show
	net_opt (PHYSOPT_SETSID, &net_id);
	i = 7;
	net_opt (PHYSOPT_SETPOWER, &i);
	net_opt (PHYSOPT_TXON, NULL);
	net_opt (PHYSOPT_RXON, NULL);
	lcdg_dm_dtop();
	lcdg_on (0);
	preset_menus();
	//top_switch ();
	runthread (beacon);
	runthread (rcv);

    entry (RS_RDCMD)

	// Line of text
	ibuf = ab_in (RS_RDCMD);

	// Reset the parsed values
	for (i = 0; i < NPVALUES; i++)
		c [i] = WNONE;

	switch (*ibuf) {

		// off			display switched off
		// on [u]		on + contrast set
		// or                   refresh
		case 'o' : proceed (RS_DSP);

		// ci n h x y			create an image object
		// cm n nl fo bg fg x y w h	create a menu object
		// ct n fo bg fg x y w		create a text object
		case 'c' : proceed (RS_CRE);

		// fc n c			create event category list
		// fr n c			create a record menu
		case 'f' : proceed (RS_FME);

		// lo				list dm objects
		// li				list images
	        // ln				list neighbours, boombuzz, ign
		case 'l' : proceed (RS_LIS);

		// ns				nhood snd
		// nr				nhood rcv
		// nm				nhood move to / fr ign
		case 'n': proceed (RS_NBH);

		// da n				add object to dlist
		// dd n				delete object from dlist
		case 'd' : proceed (RS_DIS);

		// ee f l			erase eeprom
		// ei handle			erase image
		case 'e' : proceed (RS_ERA);

		// =========================

		// si lid rqn handle		send indicated image
		// se lid rqn f l		send EEPROM chunk
		case 's' : proceed (RS_SEN);

		// ri lid rqn x y label		receive image
		// re lid rqn f l		receive EEPROM
		case 'r' : proceed (RS_RCV);

		// m
		case 'm' : proceed (RS_MEM);

	   	// host change
		case 'h': proceed (RS_HOS);
	}
// ============================================================================
Err:
	fibuf ();

    entry (RS_RDCME)

	ab_outf (RS_RDCME, "Illegal command or parameter");
	proceed (RS_RDCMD);

Ret:
	fibuf ();

    entry (RS_SST)

	if (Status)
		ab_outf (RS_SST, "Err: %u", Status);
	else
		ab_outf (RS_SST, "OK");

	Status = 0;
	proceed (RS_RDCMD);

// ============================================================================

    entry (RS_DSP)

	switch (ibuf [1]) {
		case 'f' :
			// Turn it off
			lcdg_off ();
			goto Ret;
		case 'n' :
			// Turn it on
			scan (ibuf+2, "%u", c+0);
			if (c [0] == WNONE)
				// No contrast parameter, use default
				c [0] = 0;
			lcdg_on ((byte)(c[0]));
			goto Ret;
		case 'r' :
			// Refresh
			Status = lcdg_dm_refresh ();
			goto Ret;
	}
	goto Err;

// ============================================================================

    entry (RS_CRE)

	switch (ibuf [1]) {
		case 'i' : goto Gim;
		case 'm' : goto Men;
		case 't' : goto Tex;
	}
	goto Err;

Gim:	// ====================================================================

#define	OIX	(c [0])
	
	c [2] = c [3] = 0;
	scan (ibuf+1, "%u %u %u %u", &c+0, c+1, c+2, c+3);

#define	X	((byte)(c [2]))
#define	Y	((byte)(c [3]))

	if (OIX >= MAXOBJECTS || X >= LCDG_MAXX || Y >= LCDG_MAXY)
		goto Err;

	if (free_object (OIX)) {
EFree:
		Status = 101;
		goto Ret;
	}

	if ((objects [OIX] = (lcdg_dm_obj_t*) lcdg_dm_newimage (c [1], X, Y)) ==
	    NULL)
		Status = LCDG_DM_STATUS;
	else
		objects [OIX] -> Extras = NULL;

	goto Ret;

#undef X
#undef Y

Men:	// ====================================================================

	scan (ibuf+1, "%u %u %u %u %u %u %u %u %u", c+0, c+1, c+2, c+3, c+4,
		c+5, c+6, c+7, c+8);

#define	NL	       (c [ 1])
#define	FO	((byte)(c [ 2]))
#define	BG	((byte)(c [ 3]))
#define	FG	((byte)(c [ 4]))
#define	X	((byte)(c [ 5]))
#define	Y	((byte)(c [ 6]))
#define	W	((byte)(c [ 7]))
#define	H	((byte)(c [ 8]))

#define	CN	       (c [ 9])

	if (OIX >= MAXOBJECTS || NL == 0 || H == BNONE)
		// This is a crude check for the number of args,
		// lcdg_dm_newmenu will do the complete verification
		goto Err;

	// Create the line table
	if ((lines = (char**) umalloc (NL * sizeof (char*))) == NULL) {
		Status = 103;
		goto Ret;
	}
	for (i = 0; i < NL; i++)
		lines [i] = NULL;

	CN = 0;

    entry (RS_GME)

	// fibuf is idempotent
	fibuf ();
	ab_outf (RS_GME, "Enter %u lines", NL - CN);

    entry (RS_GME1)

	ibuf = ab_in (RS_GME1);

	for (i = 0; ibuf [i] == ' ' || ibuf [i] == '\t'; i++);

	if (ibuf [i] == '\0')
		proceed (RS_GME);

	if ((lines [CN] = (char*) umalloc (strlen (ibuf + i) + 1)) == NULL) {
		lcdg_dm_csa (lines, CN);
		Status = 104;
		goto Ret;
	}
	strcpy (lines [CN], ibuf + i);

	if (++CN < NL)
		proceed (RS_GME);

	// Create the menu object
	if (free_object (OIX)) {
		Status = 101;
		lcdg_dm_csa (lines, NL);
		goto Ret;
	}

	if ((objects [OIX] =
	  (lcdg_dm_obj_t*) lcdg_dm_newmenu (lines, X, Y,
	    NL, FO, BG, FG, W, H)) == NULL) {
		lcdg_dm_csa (lines, NL);
		Status = LCDG_DM_STATUS;
	} else {
		objects [OIX] -> Extras = NULL;
	}

	goto Ret;
	
#undef	NL
#undef	FO
#undef	BG
#undef	FG
#undef	X
#undef	Y
#undef	W
#undef	H

#undef	CN

Tex:	// ====================================================================

	scan (ibuf+1, "%u %u %u %u %u %u %u", c+0, c+1, c+2, c+3, c+4, c+5,
		c+6);

#define	FO	((byte)(c [ 1]))
#define	BG	((byte)(c [ 2]))
#define	FG	((byte)(c [ 3]))
#define	X	((byte)(c [ 4]))
#define	Y	((byte)(c [ 5]))
#define	W	((byte)(c [ 6]))

	if (OIX >= MAXOBJECTS || W == BNONE)
		goto Err;

	// Expect a line of text

    entry (RS_GTE)

	fibuf ();
	ab_outf (RS_GTE, "Enter a line");

    entry (RS_GTE1)

	ibuf = ab_in (RS_GTE1);

	for (i = 0; ibuf [i] == ' ' || ibuf [i] == '\t'; i++);

	if (ibuf [i] == '\0')
		proceed (RS_GTE);

	if ((line = (char*) umalloc (strlen (ibuf + i) + 1)) == NULL) {
		Status = 103;
		goto Ret;
	}

	strcpy (line, ibuf + i);

	// Create the text object
	if (free_object (OIX)) {
		Status = 101;
		ufree (line);
		goto Ret;
	}

	if ((objects [OIX] = (lcdg_dm_obj_t*) lcdg_dm_newtext (line, X, Y,
	    FO, BG, FG, W)) == NULL) {
		ufree (line);
		Status = LCDG_DM_STATUS;
	} else {
		objects [OIX] -> Extras = NULL;
	}
	goto Ret;
	
#undef	FO
#undef	BG
#undef	FG
#undef	X
#undef	Y
#undef	W

// ============================================================================

    entry (RS_LIS)

	switch (ibuf [1]) {
		case 'o' : goto Ldm;
		case 'i' : goto Lim;
		case 'n' : goto Lnb;
	}

	goto Err;

Lnb: // =======================================================================

	c[0] = 0;

    entry (RS_LNB)

	ab_outf (RS_LNB, "NBH[%u] %u: %u %c %u", c[0], 
			nbh_menu.mm[c[0]].id,
			(word)(seconds() - nbh_menu.mm[c[0]].ts),
			nbh_menu.mm[c[0]].gr, nbh_menu.mm[c[0]].st);

	if (++c[0] >= nbh_menu.li)
		goto Ret;

	proceed (RS_LNB);
		
Ldm: // =======================================================================

	OIX = 0;

    entry (RS_LDM)

	while (OIX < MAXOBJECTS) {
		if (objects [OIX] != NULL)
			break;
		OIX++;
	}

	if (OIX == MAXOBJECTS)
		goto Ret;

    entry (RS_LDM1) 

	switch (objects [OIX] -> Type & LCDG_DMTYPE_MASK) {

#define	COI	((lcdg_dm_img_t*) (objects [OIX]))
#define	COM	((lcdg_dm_men_t*) (objects [OIX]))
#define	COT	((lcdg_dm_tex_t*) (objects [OIX]))

		case LCDG_DMTYPE_IMAGE:

			ab_outf (RS_LDM1,
				"%u = IMAGE: %x [%u,%u] [%u,%u]", OIX,
					COI->EPointer,
					COI->XL, COI->YL, COI->XH, COI->YH);
			break;

		case LCDG_DMTYPE_MENU:

			ab_outf (RS_LDM1,
			"%u = MENU: Fo%u, Bg%u, Fg%u, Nl%u [%u,%u] [%u,%u]",
					OIX,
					COM->Font, COM->BG, COM->FG, COM->NL,
					COM->XL, COM->YL,
					COM->Width, COM->Height);
			break;

		case LCDG_DMTYPE_TEXT:

			ab_outf (RS_LDM1,
			"%u = TEXT: Fo%u, Bg%u, Fg%u, Ll%u [%u,%u] [%u]",
					OIX,
					COT->Font, COT->BG, COT->FG,
					strlen (COT->Line),
					COT->XL, COT->YL, COT->Width);
			break;

		default:
			ab_outf (RS_LDM1, "%u = UNKNOWN [%u]\r\n", OIX,
				objects [OIX] -> Type);
	}

#undef	COI
#undef	COM
#undef	COT

	OIX++;
	proceed (RS_LDM);

Lim: // =======================================================================

	OIX = WNONE;
	if ((isig = (lcdg_im_hdr_t*)umalloc (sizeof (lcdg_im_hdr_t))) == NULL) {
		Status = 103;
		goto Ret;
	}

    entry (RS_LIM)

	if ((OIX = lcdg_im_find (NULL, 0, OIX)) == WNONE) {
		// No more
		ufree (isig);
		goto Ret;
	}

	// We have one image
	if (lcdg_im_hdr (OIX, isig) != 0) {
		// Skip it
NLim:
		// OIX++;
		proceed (RS_LIM);
	}

	isig->Label [LCDG_IM_LABLEN-1] = '\0';

    entry (RS_LIM1)

	ab_outf (RS_LIM1, "%u [%u,%u]: %s",
		OIX, isig->X, isig->Y, isig->Label);
	goto NLim;

// ============================================================================

    entry (RS_NBH)

	// s/r: type, who, hco, pload, [ads: lev, len <string>]
	// m: id
	c[0] = c[1] = c[2] = c[3] = c[4] = c[5] = 0;
	scan (ibuf+2, "%u %u %u %u %u %u", &c[0], &c[1], &c[2], &c[3], &c[4],
			&c[5]);
	if (c[0] == 0)
		goto Err;

	switch (ibuf [1]) {
	  case 's' : goto Nbs;
	  case 'r' : goto Nbr;
	  case 'm' : goto Nbm;
	}
        goto Err;

Nbs:
	if (c[0] == MSG_AD) {
		if (c[5] == 0 || c[5] >= LABSIZE ||
			       	(c[6] = strlen(ibuf)) <= c[5])
			goto Err;
		strncpy (lbl, ibuf + c[6] - c[5], c[5]);
	} else {
		c[5] = 0;
	}

	msg_send (c[0], c[1], c[2], c[3], c[4], c[5], lbl);
	goto Ret;

Nbr:
	switch (c[0]) {
	    case MSG_ACT:
		    rf_rcv.len = sizeof (msgActType);
		    rf_rcv.buf = (char *)umalloc(rf_rcv.len);
		    if (rf_rcv.buf == NULL) {
			    Status = 100;
			    goto Ret;
		    }
		    memset (rf_rcv.buf, 0, rf_rcv.len);
		    in_act(rf_rcv.buf, act) = c[3];
		    break;

	    case MSG_ADACK:
		    rf_rcv.len = sizeof (msgAdAckType);
		    rf_rcv.buf = (char *)umalloc(rf_rcv.len);
		    if (rf_rcv.buf == NULL) {
			    Status = 100;
			    goto Ret;
		    }
		    memset (rf_rcv.buf, 0, rf_rcv.len);
		    in_adAck(rf_rcv.buf, ack) = c[3];
		    break;

	    case MSG_AD:
		     rf_rcv.len = sizeof (msgAdType) + c[5];
		     rf_rcv.buf = (char *)umalloc(rf_rcv.len);
		     if (rf_rcv.buf == NULL) {
			     Status = 100;
			     goto Ret;
		     }
		     memset (rf_rcv.buf, 0, rf_rcv.len);
		     in_ad(rf_rcv.buf, ref) = c[3];
		     in_ad(rf_rcv.buf, lev) = c[4];
		     in_ad(rf_rcv.buf, len) = c[5];
		     strncpy (rf_rcv.buf + sizeof (msgAdType),
				     ibuf + strlen(ibuf) - c[5], c[5]);
		     break;
	    default:
		     goto Err;
	}

	in_header(rf_rcv.buf, msg_type) = c[0];
	in_header(rf_rcv.buf, snd) = c[1];
	in_header(rf_rcv.buf, hco) = c[2];
	process_incoming();
	goto Ret;

Nbm:
	if ((c[1] = handle_nbh (NBH_FIND, c[0])) == WNONE) {
		Status = 101;
		goto Ret;
	}
	update_line (ULSEL_C1, c[1], 0, 0);
	lcdg_dm_update (lcd_menu, c[1]);
	goto Ret;

// ============================================================================

    entry (RS_DIS)

	scan (ibuf+2, "%u", c+0);

	switch (ibuf [1]) {
		case 'a' : goto ADis;
		case 'd' : goto DDis;
	}
	goto Err;

ADis:
	if (OIX >= MAXOBJECTS || objects [OIX] == NULL)
		goto Err;
	Status = lcdg_dm_newtop (objects [OIX]);
	goto Ret;

DDis:
	if (OIX >= MAXOBJECTS || objects [OIX] == NULL)
		goto Err;
	lcdg_dm_remove (objects [OIX]);
	goto Ret;

// ============================================================================

    entry (RS_ERA)

	switch (ibuf [1]) {

		case 'e' :

			// Erase EEPROM
			ef = el = 0;
			scan (ibuf+2, "%lu %lu", &ef, &el);
			Status = ee_erase (WNONE, ef, el) ? 105 : 0;
			goto Ret;

		case 'i' :

			// Erase image (by handle)
			scan (ibuf+2, "%u", c+0);
			Status = lcdg_im_purge (OIX);
			goto Ret;
	}

	goto Err;

// ============================================================================
// ============================================================================
// ============================================================================

    entry (RS_SEN)

	switch (ibuf [1]) {

		case 'i' : goto ISen;
		case 'e' : goto ESen;

	}

	goto Err;

    entry (RS_RCV)

	switch (ibuf [1]) {

		case 'i' : goto IRcv;
		case 'e' : goto ERcv;
	}

	goto Err;

// ============================================================================

ISen:
	scan (ibuf+2, "%u %u %u", c+0, c+1, c+2);
	fibuf ();
	if ((Status = oep_im_snd (c [0], (byte)(c [1]), c [2])) != 0)
		proceed (RS_SST);
	// Accepted

    entry (RS_ISW)

	Status = oep_wait (RS_ISW);
	oep_im_cleanup ();
	if (Status == OEP_STATUS_DONE)
		Status = 0;
	proceed (RS_SST);

// ============================================================================

ESen:
	scan (ibuf+2, "%u %u %lu %lu", c+0, c+1, &ef, &el);
	fibuf ();
	if ((Status = oep_ee_snd (c [0], (byte) (c [1]), ef, el)) != 0)
		proceed (RS_SST);
	// Accepted

    entry (RS_ESW)

	Status = oep_wait (RS_ESW);
	oep_ee_cleanup ();
	if (Status == OEP_STATUS_DONE)
		Status = 0;
	proceed (RS_SST);

// ============================================================================

IRcv:
	lbl [0] = '\0';
	scan (ibuf+2, "%u %u %u %u %s", c+0, c+1, c+2, c+3, lbl);
	if ((i = strlen (lbl)) == 0)
		goto Err;
	if (c [2] < 8 || c [2] > LCDG_MAXX+1 ||
	    c [3] < 8 || c [3] > LCDG_MAXY+1)
		goto Err;
	fibuf ();
	if ((isig = (lcdg_im_hdr_t*) umalloc (sizeof (lcdg_im_hdr_t))) ==
	    NULL) {
		Status = 103;
		proceed (RS_SST);
	}
	isig->X = (byte)(c [2]);
	isig->Y = (byte)(c [3]);

	if (i > LCDG_IM_LABLEN - 1)
		i = LCDG_IM_LABLEN - 1;

	memcpy (&(isig->Label [0]), lbl, i);
	bzero (&(isig->Label [i]), LCDG_IM_LABLEN - i);

	oep_setlid (c [0]);
	oep_setrqn (c [1]);
	Status = oep_im_rcv (isig, BNONE, BNONE);
	ufree (isig);
	if (Status != 0)
		proceed (RS_SST);

	// Waiting: same as for sending
	proceed (RS_ISW);

// ============================================================================

ERcv:
	scan (ibuf+2, "%u %u %lu %lu", c+0, c+1, &ef, &el);
	fibuf ();
	oep_setlid (c [0]);
	oep_setrqn (c [1]);
	if ((Status = oep_ee_rcv (ef, el)) != 0)
		proceed (RS_SST);

	// Waiting
	proceed (RS_ESW);

    entry (RS_MEM)

	c [0] = memfree (0, c + 1);
	c [2] = stackfree ();

    entry (RS_MEM1)

	ab_outf (RS_MEM1, "MEMORY: %u %u %u", c [0], c [1], c [2]);
	goto Ret;

// ============================================================================

    entry (RS_HOS)

	c[0] = local_host;
	scan (ibuf+1, "%u", &c[0]);

    entry (RS_HOS1)

	ab_outf (RS_HOS1, "lhost %u -> %u", local_host, c[0]);
	if (local_host != c[0]) {
		local_host = c[0];
		handle_nbh (NBH_INIT, 0);
		lcdg_dm_refresh ();
	}

	goto Ret;

// ============================================================================

    entry (RS_FME)

	c [1] = 0;
	scan (ibuf+2, "%u %u", c+0, c+1);

	if (OIX >= MAXOBJECTS)
		goto Err;

	switch (ibuf [1]) {
		case 'c' : goto FCa;
		case 'r' : goto FRe;
	}
	goto Err;

FCa:	// ====================================================================

	if (free_object (OIX))
		goto EFree;

	if ((objects [OIX] = seal_mkcmenu ((Boolean)(c [1]))) == NULL) 
		Status = LCDG_DM_STATUS;
	else 
		objects [OIX] -> Extras = NULL;

	goto Ret;

FRe:	// ====================================================================

	if (free_object (OIX))
		goto EFree;

	if ((objects [OIX] = seal_mkrmenu ()) == NULL)
		Status = LCDG_DM_STATUS;

	goto Ret;
	
endthread

// ============================================================================

praxis_starter (SeaNode);
