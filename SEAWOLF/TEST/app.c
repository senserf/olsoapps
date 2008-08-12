/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "board_pins.h"

/* ================================================================== */
/* To test boards with Nokia 6100 LCD display (and a few other things */
/* along the way.                                                     */
/* ================================================================== */

heapmem {100};

#if	LEDS_DRIVER
#include "led.h"
#endif

#include "ser.h"
#include "serf.h"
#include "dispman.h"

#define	IBUFSIZE	128
#define	MAXLINES	32
#define	MAXOBJECTS	32

static dobject_t *objects [MAXOBJECTS];

// ============================================================================

static void free_lines (char **lines, word nl) {
//
// Deallocate an array of strings
//
	word i;

	for (i = 0; i < nl; i++)
		if (lines [i] != NULL)
			ufree (lines [i]);
	ufree (lines);
}

static Boolean empty_object (word ix) {
//
// Deallocate an object
//
	dobject_t *co;
	word i;

#define	COI	((dimage_t*)co)
#define	COM	((dmenu_t*) co)
#define	COT	((dtext_t*) co)

	if ((co = objects [ix]) == NULL)
		return NO;

	if (dm_displayed (co))
		return YES;

	switch (co->Type) {

		case DOTYPE_MENU:

			// Deallocate the text
			free_lines ((char**)(COM->Lines), COM->NL);
			break;

		case DOTYPE_TEXT:

			ufree (COT->Line);
			break;
	}

	ufree (co);
	objects [ix] = NULL;
	return NO;

#undef	COI
#undef	COM
#undef	COT
}

// ============================================================================

static void buttons (word but) {

	if (IS_JOYSTICK (but)) {
		if (DM_TOP == NULL || DM_TOP->Type != DOTYPE_MENU)
			// Ignore the joystick, if the top object is not a menu
			return;

		switch (but) {
		    case JOYSTICK_W:	dm_menu_u ((dmenu_t*)DM_TOP);	return;
		    case JOYSTICK_E:	dm_menu_d ((dmenu_t*)DM_TOP);	return;
		    case JOYSTICK_N:	dm_menu_l ((dmenu_t*)DM_TOP);	return;
		    case JOYSTICK_S:	dm_menu_r ((dmenu_t*)DM_TOP);	return;
		    default:
			// Push (ignore for now)
			return;
		}
	}

	if (but == BUTTON_0) {
		// Refresh
		dm_refresh ();
		return;
	}

	if (but == BUTTON_1) {
		// Remove top object
		dm_delete (NULL);
		return;
	}
}

// ============================================================================

process (root, void)
/* =========================================== */
/* This is the main program of the application */
/* =========================================== */

#define	RS_INIT		0
#define	RS_LOOP		1
#define	RS_RCMD		2
#define	RS_ERR		3
#define	RS_PRO		4
#define	RS_LON		10
#define	RS_LOF		20
#define	RS_GIM		30
#define	RS_GME		40
#define	RS_GTE		50
#define RS_LIS		60
#define	RS_REF		70
#define	RS_ADD		80
#define	RS_DEL		90

#define	NPVALUES	12
#define	LABSIZE		32

	static char *ibuf = NULL;
	static char **lines;
	static word Status, c [NPVALUES];
	static char lbl [LABSIZE];

	word i;
	char *line;

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFSIZE);
	Status = 0;
	dm_refresh ();
	lcdg_on (0);
	buttons_action (buttons);

  entry (RS_LOOP)

	ser_out (RS_LOOP, "\r\n"
		"Commands:\r\n"
		"  i ix lab x y             : create image object\r\n"
		"  m ix nl fo bg fg x y w h : create a menu object\r\n"
		"  t ix fo bg fg x y w      : create a text object\r\n"
		"  -----\r\n"
		"  l    : list\r\n"
		"  r    : refresh\r\n"
		"  a ix : add object\r\n"
		"  d ix : delete object\r\n"
		"  o c  : display on\r\n"
		"  f    : display off\r\n"
	);

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, IBUFSIZE);

	// Reset the parsed values
	for (i = 0; i < NPVALUES; i++)
		c [i] = WNONE;

	switch (ibuf [0]) {

		case 'o' : proceed (RS_LON);
		case 'f' : proceed (RS_LOF);

		case 'i' : proceed (RS_GIM);
		case 'm' : proceed (RS_GME);
		case 't' : proceed (RS_GTE);

		case 'l' : proceed (RS_LIS);

		case 'r' : proceed (RS_REF);

		case 'a' : proceed (RS_ADD);
		case 'd' : proceed (RS_DEL);
	}

  entry (RS_ERR)

	ser_out (RS_ERR, "Illegal command or parameter\r\n");
	proceed (RS_LOOP);

  entry (RS_PRO)

	if (Status)
		ser_outf (RS_PRO, "Err: %u\r\n", Status);
	else
		ser_out (RS_PRO, "OK\r\n");

	Status = 0;
	proceed (RS_RCMD);

// ============================================================================

  entry (RS_LON)

	scan (ibuf+1, "%u", c+0);
	if (c [0] == WNONE)
		c [0] = 0;
	lcdg_on ((byte)(c[0]));
	proceed (RS_PRO);

  entry (RS_LOF)

	lcdg_off ();
	proceed (RS_PRO);

// ============================================================================

  entry (RS_GIM)

#define	OIX	(c [0])
	
	lbl [0] = '\0';
	scan (ibuf+1, "%u %s %u %u", &c+0, lbl, c+1, c+2);

#define	X	((byte)(c [1]))
#define	Y	((byte)(c [2]))

	if (OIX >= MAXOBJECTS || X >= LCDG_MAXX || Y >= LCDG_MAXY)
		proceed (RS_ERR);

	if (empty_object (OIX)) {
		Status = 101;
		proceed (RS_PRO);
	}

	if ((objects [OIX] = (dobject_t*) dm_newimage (lbl, X, Y)) == NULL)
		Status = DM_STATUS;

	proceed (RS_PRO);

#undef X
#undef Y

// ============================================================================

  entry (RS_GME)

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
		// This is a crude check for the number of args, dm_newmenu will
		// do the verification
		proceed (RS_ERR);

	// Create the line table
	if ((lines = (char**) umalloc (NL * sizeof (char*))) == NULL) {
		Status = 102;
		proceed (RS_PRO);
	}
	for (i = 0; i < NL; i++)
		lines [i] = NULL;

	CN = 0;

  entry (RS_GME+1)

	ser_outf (RS_GME+1, "Enter %u lines\r\n", NL - CN);

  entry (RS_GME+2)

	ser_in (RS_GME+2, ibuf, IBUFSIZE);

	for (i = 0; ibuf [i] == ' ' || ibuf [i] == '\t'; i++);

	if (ibuf [i] == '\0')
		proceed (RS_GME+1);

	if ((lines [CN] = (char*) umalloc (strlen (ibuf + i) + 1)) == NULL) {
		free_lines (lines, CN);
		Status = 103;
		proceed (RS_PRO);
	}
	strcpy (lines [CN], ibuf + i);

	if (++CN < NL)
		proceed (RS_GME+1);

	// Create the menu object
	if (empty_object (OIX)) {
		Status = 101;
		free_lines (lines, NL);
		proceed (RS_PRO);
	}

	if ((objects [OIX] = (dobject_t*) dm_newmenu ((const char**)lines,
	    NL, FO, BG, FG, X, Y, W, H)) == NULL) {
		free_lines (lines, NL);
		Status = DM_STATUS;
	}
	proceed (RS_PRO);
	
#undef	NL
#undef	FO
#undef	BG
#undef	FG
#undef	X
#undef	Y
#undef	W
#undef	H

#undef	CN

// ============================================================================

  entry (RS_GTE)

	scan (ibuf+1, "%u %u %u %u %u %u %u", c+0, c+1, c+2, c+3, c+4, c+5,
		c+6);

#define	FO	((byte)(c [ 1]))
#define	BG	((byte)(c [ 2]))
#define	FG	((byte)(c [ 3]))
#define	X	((byte)(c [ 4]))
#define	Y	((byte)(c [ 5]))
#define	W	((byte)(c [ 6]))

	if (OIX >= MAXOBJECTS || W == BNONE)
		proceed (RS_ERR);

	// Expect a line of text

  entry (RS_GTE+1)

	ser_out (RS_GTE+1, "Enter a line\r\n");

  entry (RS_GTE+2)

	ser_in (RS_GTE+2, ibuf, IBUFSIZE);

	for (i = 0; ibuf [i] == ' ' || ibuf [i] == '\t'; i++);

	if (ibuf [i] == '\0')
		proceed (RS_GTE+1);

	if ((line = (char*) umalloc (strlen (ibuf + i) + 1)) == NULL) {
		Status = 102;
		proceed (RS_PRO);
	}

	strcpy (line, ibuf + i);

	// Create the text object
	if (empty_object (OIX)) {
		Status = 101;
		ufree (line);
		proceed (RS_PRO);
	}

	if ((objects [OIX] = (dobject_t*) dm_newtext (line, FO, BG, FG, X, Y,
	    W)) == NULL) {
		ufree (line);
		Status = DM_STATUS;
	}
	proceed (RS_PRO);
	
#undef	FO
#undef	BG
#undef	FG
#undef	X
#undef	Y
#undef	W

// ============================================================================

  entry (RS_LIS)

	OIX = 0;

  entry (RS_LIS+1)

	while (OIX < MAXOBJECTS) {
		if (objects [OIX] != NULL)
			break;
		OIX++;
	}

	if (OIX == MAXOBJECTS)
		proceed (RS_PRO);

  entry (RS_LIS+2) 

	switch (objects [OIX] -> Type) {

#define	COI	((dimage_t*) (objects [OIX]))
#define	COM	((dmenu_t*)  (objects [OIX]))
#define	COT	((dtext_t*)  (objects [OIX]))

		case DOTYPE_IMAGE:

			ser_outf (RS_LIS+2,
				"%u = IMAGE: %lx [%u,%u] [%u,%u]\r\n", OIX,
					COI->EPointer,
					COI->XL, COI->YL, COI->XH, COI->YH);
			break;

		case DOTYPE_MENU:

			ser_outf (RS_LIS+2,
			"%u = MENU: Fo%u, Bg%u, Fg%u, Nl%u [%u,%u] [%u,%u]\r\n",
					OIX,
					COM->Font, COM->BG, COM->FG, COM->NL,
					COM->XL, COM->YL,
					COM->Width, COM->Height);
			break;

		case DOTYPE_TEXT:

			ser_outf (RS_LIS+2,
			"%u = TEXT: Fo%u, Bg%u, Fg%u, Ll%u [%u,%u] [%u]\r\n",
					OIX,
					COT->Font, COT->BG, COT->FG,
					strlen (COT->Line),
					COT->XL, COT->YL, COT->Width);
			break;

		default:
			ser_outf (RS_LIS+2, "%u = UNKNOWN [%u]\r\n", OIX,
				objects [OIX] -> Type);
	}

#undef	COI
#undef	COM
#undef	COT

	OIX++;
	proceed (RS_LIS+1);

// ============================================================================

  entry (RS_REF)

	Status = dm_refresh ();
	proceed (RS_PRO);

// ============================================================================

  entry (RS_ADD)

	scan (ibuf+1, "%u", c+0);

	if (OIX >= MAXOBJECTS || objects [OIX] == NULL)
		proceed (RS_ERR);

	Status = dm_top (objects [OIX]);
	proceed (RS_PRO);

// ============================================================================

  entry (RS_DEL)

	scan (ibuf+1, "%u", c+0);

	if (OIX >= MAXOBJECTS || objects [OIX] == NULL)
		proceed (RS_ERR);

	dm_delete (objects [OIX]);
	proceed (RS_PRO);

endprocess (1)
