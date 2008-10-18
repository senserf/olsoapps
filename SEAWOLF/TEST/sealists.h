#ifndef __pg_sealists_h__
#define __pg_sealists_h__

#include "oep.h"
#include "lcdg_images.h"
#include "lcdg_dispman.h"

//+++ "sealists.c"

// Offsets into EEPROM (must agree with those in mkeeprom.tcl) ================

#define	SEA_FONTSIZE	4096	// This takes less than half a page
#define	SEA_NRECORDS	128	// Limit on the number of records
#define	SEA_FIMPAGE	4	// The first page of the image area
#define	SEA_RECSIZE	18	// Record size in bytes
#define	SEA_NCATS	16	// The number of categories (of each type)

// These are derived from the above ===========================================

// Font area
#define	SEA_EOFF_FNT	0

// Categories area
#define	SEA_EOFF_CAT	(SEA_EOFF_FNT + SEA_FONTSIZE)

// Record area: two sets of text pointers
#define	SEA_EOFF_REC	(SEA_EOFF_CAT + (4 * SEA_NCATS))

// Text pool area
#define	SEA_EOFF_TXT	(SEA_EOFF_REC + (SEA_RECSIZE * SEA_NRECORDS))

// End of text area (+1) == the beginning of image area
#define	SEA_EOFF_ETX	((lword) SEA_FIMPAGE * (lword) LCDG_IM_PAGESIZE)

// Record offsets =============================================================

#define	SEA_ROFF_ID	0	// Network ID		(4)
#define	SEA_ROFF_EC	4	// Event categories	(2)
#define	SEA_ROFF_MC	6	// My categories	(2)
#define	SEA_ROFF_CL	8	// Class		(2)
#define	SEA_ROFF_NI	10	// Nickname		(2)
#define	SEA_ROFF_NM	12	// Name			(2)
#define	SEA_ROFF_NO	14	// Note			(2)
#define	SEA_ROFF_IM	16	// Image handle		(2)

// Menu parameters ============================================================

#define	SEA_MENU_CAT_FONT	1
#define	SEA_MENU_CAT_BG		COLOR_BLACK
#define	SEA_MENU_CAT_FG		COLOR_GREEN
#define	SEA_MENU_CAT_X		14
#define	SEA_MENU_CAT_Y		14
#define	SEA_MENU_CAT_W		12
#define	SEA_MENU_CAT_H		8

#define	SEA_MENU_REC_FONT	0
#define	SEA_MENU_REC_BG		COLOR_BLUE
#define	SEA_MENU_REC_FG		COLOR_YELLOW
#define	SEA_MENU_REC_X		10
#define	SEA_MENU_REC_Y		10
#define	SEA_MENU_REC_W		12
#define	SEA_MENU_REC_H		10

#define	SEA_METER_FONT		3	// Special font used by the meter
#define	SEA_METER_Y		(130-8)	// The bottom of the display

#define	SEA_METER_BG0		COLOR_BLACK
#define	SEA_METER_FG0		COLOR_RED
#define	SEA_METER_BG1		COLOR_GREEN
#define	SEA_METER_FG1		COLOR_RED

typedef struct {
//
// Record layout, ID ignored
//
	word	ECats, MCats, CL, IM;
	char	*NI, *NM, *NT;

} sea_rec_t;

lcdg_dm_obj_t *seal_mkcmenu (Boolean);
lcdg_dm_obj_t *seal_mkrmenu (word);
sea_rec_t *seal_getrec (word);
void seal_freerec (sea_rec_t*);
word seal_findrec (lword);
byte seal_meter (word, word);

#endif
