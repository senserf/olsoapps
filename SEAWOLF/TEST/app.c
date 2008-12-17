#include "sysio.h"
#include "pins.h"
#include "oep.h"
#include "oep_ee.h"
#include "lcdg_images.h"
#include "lcdg_dispman.h"
#include "plug_null.h"
#include "phys_uart.h"
#include "ab.h"
#include "sealists.h"

/*
 * Illustrates images, menus, OEP
 */

// ============================================================================

//+++ "hostid.c"
extern lword host_id;

#define	MLID	((word)host_id)

// ============================================================================

static int SFD;

// ============================================================================
// ============================================================================

#define	MAXLINES	32
#define	MAXOBJECTS	32

static lcdg_dm_obj_t *objects [MAXOBJECTS];

// ============================================================================

static Boolean free_object (word ix) {
//
// Deallocate an object
//
	lcdg_dm_obj_t *co;

#define	COI	((lcdg_dm_img_t*)co)
#define	COM	((lcdg_dm_men_t*) co)
#define	COT	((lcdg_dm_tex_t*) co)

	if ((co = objects [ix]) == NULL)
		return NO;

	if (lcdg_dm_shown (co))
		return YES;

	switch (co->Type) {

		case LCDG_DMTYPE_MENU:

			// Deallocate the text
			lcdg_dm_csa ((char**)(COM->Lines), COM->NL);
			break;

		case LCDG_DMTYPE_TEXT:

			ufree (COT->Line);
			break;
	}

	if (co->Extras != NULL)
		ufree (co->Extras);

	ufree (co);
	objects [ix] = NULL;
	return NO;

#undef	COI
#undef	COM
#undef	COT
}

// ============================================================================

static void display_rec (word rh) {
//
// Display the record (picture + meter) represented by the handle
//
	sea_rec_t *rec;

	if ((rec = seal_getrec (rh)) == NULL)
		// Failed to get the record
		return;

	if (rec->IM != WNONE)
		lcdg_im_disp (rec->IM, 0, 0);

	seal_meter (rec->ECats, rec->MCats);

	seal_freerec (rec);
}

// ============================================================================

static void preset_menus () {

	word i;

	for (i = 0; i < 4; i++)
		if (objects [i] != NULL)
			return;

	if ((objects [0] = seal_mkcmenu (NO)) != NULL)
		lcdg_dm_newtop (objects [0]);
	if ((objects [1] = seal_mkcmenu (YES)) != NULL)
		lcdg_dm_newtop (objects [1]);
	if ((objects [2] = seal_mkrmenu (2)) != NULL)
		lcdg_dm_newtop (objects [2]);
	if ((objects [3] = seal_mkrmenu (1)) != NULL)
		lcdg_dm_newtop (objects [3]);
}

static void top_switch () {
//
// Rotate the top object
//
	if (LCDG_DM_HEAD == NULL) {
		lcdg_dm_dtop ();
		preset_menus ();
		lcdg_on (0);
		return;
	}

	if (LCDG_DM_HEAD != LCDG_DM_TOP)
		// More than one item
		lcdg_dm_newtop (LCDG_DM_HEAD);
}

// ============================================================================

static void buttons (word but) {

	if (IS_JOYSTICK (but)) {

		if (but == JOYSTICK_PUSH) {
			top_switch ();
			return;
		}

		if (LCDG_DM_TOP == NULL ||
		    LCDG_DM_TOP->Type != LCDG_DMTYPE_MENU)
			// Ignore the joystick, if the top object is not a menu
			return;

		switch (but) {

		    case JOYSTICK_W:

			lcdg_dm_menu_u ((lcdg_dm_men_t*)LCDG_DM_TOP);
			return;

		    case JOYSTICK_E:

			lcdg_dm_menu_d ((lcdg_dm_men_t*)LCDG_DM_TOP);
			return;

		    case JOYSTICK_N:

			lcdg_dm_menu_l ((lcdg_dm_men_t*)LCDG_DM_TOP);
			return;

		    case JOYSTICK_S:

			lcdg_dm_menu_r ((lcdg_dm_men_t*)LCDG_DM_TOP);
			return;

		    default:
			// Impossible
			return;
		}
	}

	if (but == BUTTON_0) {
		// Refresh
		lcdg_dm_refresh ();
		return;
	}

	if (but == BUTTON_1) {
		// Draw picture and meter, if record menu
		if (LCDG_DM_TOP != NULL &&
		    LCDG_DM_TOP->Type == LCDG_DMTYPE_MENU &&
		    LCDG_DM_TOP->Extras != NULL)
			// A record menu (Extras != NULL only in that case)
			display_rec (LCDG_DM_TOP->
				Extras [lcdg_dm_menu_c (LCDG_DM_TOP)]);
		// Used to be this: lcdg_dm_remove (NULL);
		return; // Intentionally superfluous
	}
}

// ============================================================================

#define	NPVALUES	12
#define	LABSIZE		(LCDG_IM_LABLEN+2)

static char *ibuf = NULL;

static void fibuf () {

	if (ibuf) {
		ufree (ibuf);
		ibuf = NULL;
	}
}

thread (root)

#define	RS_INIT		0
#define	RS_RDCMD	1
#define	RS_RDCME	2
#define	RS_SST		3
#define	RS_DSP		4
#define	RS_CRE		5
#define	RS_GME		6
#define	RS_GTE		8
#define	RS_LIS		10
#define	RS_LDM		11
#define	RS_LIM		13
#define	RS_DIS		15
#define	RS_ERA		16
#define	RS_SEN		17
#define	RS_RCV		18
#define	RS_ISW		19
#define	RS_ESW		20
#define	RS_MEM		21
#define	RS_FME		23

    word i;
    char *line;
    lword ef, el;

    static word c [NPVALUES];
    static byte Status;
    static char lbl [LABSIZE];
    static char **lines;
    static lcdg_im_hdr_t *isig;

    entry (RS_INIT)

	phys_uart (0, OEP_MAXRAWPL, 0);
	tcv_plug (0, &plug_null);

	// OSS link
	SFD = tcv_open (WNONE, 0, 0);

	if (SFD < 0)
		syserror (ENODEVICE, "uart");

	// Initialize EEPROM
	lcdg_im_init (SEA_FIMPAGE, 0);

	// Initialize OEP
	if (oep_init () == NO)
		syserror (ERESOURCE, "oep_init");

	// oep_setlid (MLID);

	// Initialize the PHY
	i = 0xffff;
	tcv_control (SFD, PHYSOPT_SETSID, &i);
	tcv_control (SFD, PHYSOPT_TXON, NULL);
	tcv_control (SFD, PHYSOPT_RXON, NULL);

	// Buttons
	buttons_action (buttons);

	// Initialize the AB UART link
	ab_init (SFD);

	// And run the show

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
		// os n v		set color table
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
		case 'l' : proceed (RS_LIS);

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
		case 's' :
			// Set color table
			scan (ibuf+2, "%u %x", c+0, c+1);
			lcdg_setct ((byte)(c [0]), c [1]);
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

    entry (RS_GME+1)

	ibuf = ab_in (RS_GME+1);

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
	  (lcdg_dm_obj_t*) lcdg_dm_newmenu (lines,
	    NL, FO, BG, FG, X, Y, W, H)) == NULL) {
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

    entry (RS_GTE+1)

	ibuf = ab_in (RS_GTE+1);

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

	if ((objects [OIX] = (lcdg_dm_obj_t*) lcdg_dm_newtext (line, FO, BG,
	    FG, X, Y, W)) == NULL) {
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
	}

	goto Err;

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

    entry (RS_LDM+1) 

	switch (objects [OIX] -> Type) {

#define	COI	((lcdg_dm_img_t*) (objects [OIX]))
#define	COM	((lcdg_dm_men_t*) (objects [OIX]))
#define	COT	((lcdg_dm_tex_t*) (objects [OIX]))

		case LCDG_DMTYPE_IMAGE:

			ab_outf (RS_LDM+1,
				"%u = IMAGE: %x [%u,%u] [%u,%u]", OIX,
					COI->EPointer,
					COI->XL, COI->YL, COI->XH, COI->YH);
			break;

		case LCDG_DMTYPE_MENU:

			ab_outf (RS_LDM+1,
			"%u = MENU: Fo%u, Bg%u, Fg%u, Nl%u [%u,%u] [%u,%u]",
					OIX,
					COM->Font, COM->BG, COM->FG, COM->NL,
					COM->XL, COM->YL,
					COM->Width, COM->Height);
			break;

		case LCDG_DMTYPE_TEXT:

			ab_outf (RS_LDM+1,
			"%u = TEXT: Fo%u, Bg%u, Fg%u, Ll%u [%u,%u] [%u]",
					OIX,
					COT->Font, COT->BG, COT->FG,
					strlen (COT->Line),
					COT->XL, COT->YL, COT->Width);
			break;

		default:
			ab_outf (RS_LDM+1, "%u = UNKNOWN [%u]\r\n", OIX,
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

    entry (RS_LIM+1)

	ab_outf (RS_LIM+1, "%u [%u,%u]: %s",
		OIX, isig->X, isig->Y, isig->Label);
	goto NLim;

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
	// ab_off ();

    entry (RS_ISW)

	Status = oep_wait (RS_ISW);
	oep_im_cleanup ();
	// ab_on ();
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
	// ab_off ();

    entry (RS_ESW)

	Status = oep_wait (RS_ESW);
	oep_ee_cleanup ();
	// ab_on ();
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

	// ab_off ();
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

	// ab_off ();
	// Waiting
	proceed (RS_ESW);

    entry (RS_MEM)

	c [0] = memfree (0, c + 1);
	c [2] = stackfree ();

    entry (RS_MEM+1)

	ab_outf (RS_MEM+1, "MEMORY: %u %u %u", c [0], c [1], c [2]);
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

	if ((objects [OIX] = seal_mkrmenu (c [1])) == NULL)
		Status = LCDG_DM_STATUS;

	// Extras is now being used to store record handles
	goto Ret;
	
endthread
