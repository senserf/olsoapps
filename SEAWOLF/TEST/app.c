/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

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

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_ERR		11
#define	RS_LRES		30
#define	RS_LOFF		35
#define	RS_LGEN		40
#define	RS_SETI		42
#define	RS_SETC		43
#define	RS_DISP		44
#define	RS_ERASE	50
#define	RS_FONT		60
#define	RS_TAREA	70
#define	RS_ELINES	80
#define	RS_ALINE	85
#define	RS_WLINES	87
#define	RS_SHOW		90
#define RS_MENU		95
#define	RS_RCMN		100

#define	IBUFSIZE	128
#define	MAXLINES	32

static byte *PIXT = NULL;
static word NPIX = 0;
static word Aux, Status = 0;

#define	MENU_MAXLINES	32
#define	CCHAR		"*"

typedef struct {

	const char *Lines [MENU_MAXLINES];
	word LLength [MENU_MAXLINES];
	word NLines, MaxLL;
	byte Width, Height;	// In characters
	word FL,		// First line shown in menu
	     SH;		// Shift (horizontal scroll)
	word SE;		// Selected line

} menu_t;

byte MW, MH;

static menu_t MENU;

void menu_reset (menu_t *m, byte w, byte h) {
//
// After a change in parameters
//
	m->FL = m->SH = m->SE = 0;
	m->Width = w;
	m->Height = h;
}

void menu_init (menu_t *m) {
//
// Initialize memory (reduntant in this case)
//
	m->NLines = 0;
	menu_reset (m, 0, 0);
}

void menu_clear (menu_t *m) {
//
// Deallocate menu
//
	word i;

	for (i = 0; i < m->NLines; i++)
		ufree (m->Lines [i]);

	menu_init (m);
}

word menu_addline (menu_t *m, const char *ln) {
//
// Add a line to the menu
//
	word ll;

	if (m->NLines == MENU_MAXLINES)
		return ERROR;

	ll = strlen (ln);

	if ((m->Lines [m->NLines] = (const char*)umalloc (ll+1)) == NULL)
		return ERROR;

	m->LLength [m->NLines] = ll;
	if (ll > m->MaxLL)
		m->MaxLL = ll;
	strcpy ((char*)(m->Lines [m->NLines]), ln);
	m->NLines++;
}

byte menu_ell (menu_t *m, word ln) {
//
// Return effective line length
//
	word r;

	if (m->LLength [ln] <= m->SH)
		return 0;

	r = m->LLength [ln] - m->SH;
	if (r > m->Width - 1)
		r = m->Width - 1;
	return (byte) r;
}

void menu_show (menu_t *m) {
//
// Show the menu in the text area
//
	word i;

	// Make sure the area is initially empty
	lcdg_el (0, m->Height);
	m->SH = 0;
	m->FL = 0;
	m->SE = 0;
	for (i = 0; i < m->NLines; i++) {
		if (i == m->Height)
			break;
		lcdg_wl (m->Lines [i], m->SH, 1, i);
		if (m->LLength [i] < m->Width - 1)
			lcdg_ec (m->LLength [i] + 1, i, m->Width);
	}

	// Select the first line
	if (m->NLines)
		lcdg_wl (CCHAR, 0, 0, 0);
}

void menu_down (menu_t *m) {
//
// Select next item down the list
//
	word i, j, d;
	byte a, b;

	if (m->NLines < 2)
		// Do nothing
		return;

	if (m->SE >= m->NLines - 1)
		// Cursor at the last line
		return;

	// Relative to the window
	a = (byte) (m->SE - m->FL);
	// Cursor must be erased whatever comes next ...
	lcdg_ec (0, a, 1);
	// ... and advanced
	m->SE++;

	if (a < m->Height - 1) {
		// Just advance the cursor
		lcdg_wl (CCHAR, 0, 0, a+1);
		return;
	}

	// At the bottom (and there are more lines)

	// Previous FL
	d = m->FL;
	// Advance
	m->FL += m->Height;
	if (m->FL + m->Height > m->NLines)
		// Less than a window-full of lines left
		m->FL = m->NLines - m->Height;

	// The actual advancement
	d = m->FL - d;

	for (i = 0; i < m->Height; i++) {
		// New line index
		j = i + m->FL;
		lcdg_wl (m->Lines [j], m->SH, 1, i);
		// Check how much to clean after the line
		if ((a = menu_ell (m, j)) >= m->Width - 1)
			// No need to clear
			continue;
		if (a >= (b = menu_ell (m, j-d)))
			// No need to clear
			continue;
		// Number of characters to clear
		b -= a;
		lcdg_ec (a + 1, i, b);
	}

	// Display the cursor
	lcdg_wl (CCHAR, 0, 0, m->SE - m->FL);
}

void menu_up (menu_t *m) {
//
// Select next item down the list
//
	word i, j, d;
	byte a, b;

	if (m->NLines < 2)
		// Do nothing
		return;

	if (m->SE == 0)
		// Cursor at the first line
		return;

	// Relative to the window
	a = (byte) (m->SE - m->FL);
	// Cursor must be erased whatever comes next ...
	lcdg_ec (0, a, 1);
	// ... and advanced
	m->SE--;

	if (a > 0) {
		// Just advance the cursor
		lcdg_wl (CCHAR, 0, 0, a-1);
		return;
	}

	// At the top

	// Previous FL
	d = m->FL;
	// Advance
	if (m->FL < m->Height)
		m->FL = 0;
	else
		m->FL -= m->Height;
	d -= m->FL;

	for (i = 0; i < m->Height; i++) {
		// New line index
		j = i + m->FL;
		lcdg_wl (m->Lines [j], m->SH, 1, i);
		// Check how much to clean after the line
		if ((a = menu_ell (m, j)) >= m->Width - 1)
			// No need to clear
			continue;
		if (a >= (b = menu_ell (m, j+d)))
			// No need to clear
			continue;
		// Number of characters to clear
		b -= a;
		lcdg_ec (a + 1, i, b);
	}

	// Display the cursor
	lcdg_wl (CCHAR, 0, 0, m->SE - m->FL);
}

void menu_left (menu_t *m) {
//
// Scroll right
//
	word i, j;

	if (m->SH + m->Width - 1 >= m->MaxLL)
		// No need to scroll any further
		return;
	m->SH++;

	for (i = 0; i < m->Height; i++) {
		j = i + m->FL;
		if (j >= m->NLines)
			return;
		lcdg_wl (m->Lines [j], m->SH, 1, i);
		// Should a character be erased
		j = menu_ell (m, j);
		if (j < m->Width - 1)
			lcdg_ec (j + 1, i, 1);
	}
}

void menu_right (menu_t *m) {
//
// Scroll right
//
	word i, j;

	if (m->SH == 0)
		return;
	m->SH--;

	for (i = 0; i < m->Height; i++) {
		j = i + m->FL;
		if (j >= m->NLines)
			return;
		lcdg_wl (m->Lines [j], m->SH, 1, i);
	}
}

process (root, void)
/* =========================================== */
/* This is the main program of the application */
/* =========================================== */

	static char *ibuf = NULL;
	word i, j, n, m, c [4];

  entry (RS_INIT)

	if (ibuf == NULL)
		ibuf = (char*) umalloc (IBUFSIZE);

	ser_out (RS_INIT, "\r\n"
		"Commands:\r\n"
		"  r              : reset\r\n"
		"  R n            : LCD reset\r\n"
		"  O              : LCD off\r\n"
		"  G m n c c c c  : generate a pix table\r\n"
		"  S x y w h m    : set area\r\n"
		"  C x y          : bgr/fgr colors\r\n"
		"  D x y          : render the pix table\r\n"
		"  E [n]          : erase [to color]\r\n"
		"  F n            : set font\r\n"
		"  T x y nc nl    : text area\r\n"
		"  X              : erase line table\r\n"
		"  A line         : add new line\r\n"
		"  L              : show lines\r\n"
		"  W              : display lines\r\n"
		"  M[+-]          : selection up or down\r\n"
	);

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, IBUFSIZE);

	switch (ibuf [0]) {

		case 'r' : reset ();
		case 'R' : proceed (RS_LRES);
		case 'O' : proceed (RS_LOFF);
		case 'G' : proceed (RS_LGEN);
		case 'C' : proceed (RS_SETC);
		case 'S' : proceed (RS_SETI);
		case 'D' : proceed (RS_DISP);
		case 'E' : proceed (RS_ERASE);
		case 'F' : proceed (RS_FONT);
		case 'T' : proceed (RS_TAREA);
		case 'X' : proceed (RS_ELINES);
		case 'A' : proceed (RS_ALINE);
		case 'L' : proceed (RS_SHOW);
		case 'W' : proceed (RS_WLINES);
		case 'M' : proceed (RS_MENU);
	}

  entry (RS_ERR)

	ser_out (RS_ERR, "Illegal command or parameter\r\n");
	proceed (RS_INIT);

  entry (RS_LRES)

	n = 0;
	scan (ibuf+1, "%u", &n);
	lcdg_on ((byte)n);
	proceed (RS_RCMN);

  entry (RS_LOFF)

	lcdg_off ();
	proceed (RS_RCMN);

  entry (RS_LGEN)

	// Generate a pix table
	m = 0;
	n = 16;
	c [0] = 0; c [1] = 0; c [2] = 0xffff; c [3] = 0xffff;
	scan (ibuf+1, "%u %u %x %x %x %x", &m, &n, c+0, c+1, c+2, c+3);
	if (PIXT != NULL)
		ufree (PIXT);

	i = m ? n : (n + 1) * 12 / 8;

	if ((PIXT = (byte*) umalloc (i)) == NULL)
		proceed (RS_ERR);

	NPIX = n;

	if (m) {
		for (i = 0; i < n; i++)
			PIXT [i] = (byte) c [i & 0x3];
	} else {
		m = (n + 1) / 2;
		i = 0;
		j = 0;
		while (m--) {
			PIXT [i] = (byte)  (c [j] >> 4); i++;
			PIXT [i] = (byte) ((c [j] & 0xf) << 4);
			if (j == 3)
				j = 0;
			else
				j++;
			PIXT [i] |= (byte) ((c [j] >> 8) & 0xf); i++;
			PIXT [i] = (byte) c [j]; i++;
			if (j == 3)
				j = 0;
			else
				j++;
		}
	}

	proceed (RS_RCMN);

  entry (RS_SETI)

	
	c [0] = 0; c [1] = 0; c [2] = 130; c [3] = 130;
	scan (ibuf+1, "%u %u %u %u %u", c+0, c+1, c+2, c+3, &m);

	lcdg_set (
		(byte)(c[0]),
		(byte)(c[1]),
		(byte)(c[2]),
		(byte)(c[3]), m);

	proceed (RS_RCMN);

  entry (RS_SETC)

	c [0] = BNONE;
	c [1] = BNONE;
	scan (ibuf+1, "%u %u", c+0, c+1);

	lcdg_setc ((byte)(c[0]), (byte)(c[1]));
	proceed (RS_RCMN);
	
  entry (RS_DISP)

	if (PIXT == NULL)
		proceed (RS_ERR);

	c [0] = 0;
	c [1] = 0;
	scan (ibuf+1, "%u %u", c+0, c+1);

	lcdg_render ((byte)(c[0]), (byte)(c[1]), PIXT, NPIX);
	proceed (RS_RCMN);
	
  entry (RS_ERASE)

	m = 0;
	scan (ibuf+1, "%u", &m);
	lcdg_clear ((byte)m);
	proceed (RS_RCMN);

  entry (RS_FONT)

	m = 0;
	scan (ibuf+1, "%u", &m);
	Status = lcdg_font ((byte)m);
	proceed (RS_RCMN);

  entry (RS_TAREA)

	c [0] = 0; c [1] = 0; c [2] = 10; c [3] = 10;
	scan (ibuf+1, "%u %u %u %u %u", c+0, c+1, c+2, c+3);

	lcdg_set (0, 0, 130, 130, 0);
	lcdg_clear (BNONE);

	Status = lcdg_sett ((byte)(c[0]), (byte)(c[1]), (byte)(c[2]),
		(byte)(c[3]));
	MW = c [2];
	MH = c [3];
MRes:
	menu_reset (&MENU, MW, MH);
	proceed (RS_RCMN);

  entry (RS_ELINES)

	menu_clear (&MENU);
	goto MRes;

  entry (RS_ALINE)

	Status = menu_addline (&MENU, ibuf + 1);
	proceed (RS_RCMN);

  entry (RS_WLINES)

	menu_show (&MENU);
	proceed (RS_RCMN);

  entry (RS_MENU)

	if (ibuf [1] == '+')
		menu_down (&MENU);
	else if (ibuf [1] == '-')
		menu_up (&MENU);
	else if (ibuf [1] == '<')
		menu_left (&MENU);
	else if (ibuf [1] == '>')
		menu_right (&MENU);
	else
		proceed (RS_ERR);

	proceed (RS_RCMN);

  entry (RS_SHOW)

	Aux = 0;

  entry (RS_SHOW+1)

	if (Aux >= MENU.NLines)
		proceed (RS_RCMN);

	ser_outf (RS_SHOW+1, "Line %u: '%s'\r\n", Aux, MENU.Lines [Aux]);
	Aux++;
	proceed (RS_SHOW+1);

  entry (RS_RCMN)

	ser_outf (RS_RCMN, "Done: %u\r\n", Status);
	Status = 0;
	proceed (RS_RCMD);
	
endprocess (1)
