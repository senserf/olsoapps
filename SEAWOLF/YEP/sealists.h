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
#define	SEA_RECSIZE	20	// Record size in bytes
#define	SEA_NCATS	16	// The number of categories (of each type)

// These are derived from the above ===========================================

// Font area
#define	SEA_EOFF_FNT	0

// Pointer to E categories menu
#define	SEA_EOFF_ECP	(SEA_EOFF_FNT + SEA_FONTSIZE)

// Pointer to M categories menu
#define	SEA_EOFF_MCP	(SEA_EOFF_ECP + 2)

// Pointer to records menu
#define	SEA_EOFF_REP	(SEA_EOFF_MCP + 2)

// Number of records
#define	SEA_EOFF_RNM	(SEA_EOFF_REP + 2)

// The records area
#define	SEA_EOFF_REC	(SEA_EOFF_RNM + 2)

// Text pool area
#define	SEA_EOFF_TXT	(SEA_EOFF_REC + (SEA_RECSIZE * SEA_NRECORDS))

// End of text area (+1) == the beginning of image area
#define	SEA_EOFF_ETX	((lword) SEA_FIMPAGE * (lword) LCDG_IM_PAGESIZE)

// Record offsets =============================================================

#define	SEA_ROFF_ID	0	// Network ID		(4)
#define	SEA_ROFF_EC	4	// Event categories	(2)
#define	SEA_ROFF_MC	6	// My categories	(2)
#define	SEA_ROFF_CL	8	// Class		(2)
#define	SEA_ROFF_ME	10	// Meter E		(2)
#define	SEA_ROFF_MM	12	// Meter M		(2)
#define	SEA_ROFF_NO	14	// Note			(2)
#define	SEA_ROFF_IM	16	// Image handle		(2)
#define	SEA_ROFF_NM	18	// Name			(2)

typedef struct {
//
// Record layout
//
	lword	NID;
	word	ECats,
		MCats,
		CL,
		ME,
		MY,
		NT,
		IM,
		NM;

} sea_rec_t;

lcdg_dm_obj_t *seal_mkcmenu (Boolean);
lcdg_dm_obj_t *seal_mkrmenu ();
sea_rec_t *seal_getrec (word);
word seal_findrec (lword);
void seal_disprec (sea_rec_t*);

#define seal_objaddr(ptr)	((lword)((ptr)+SEA_EOFF_TXT))
#define seal_gettext(off) 	lcdg_dm_newtext_e (seal_objaddr (off))

#endif
