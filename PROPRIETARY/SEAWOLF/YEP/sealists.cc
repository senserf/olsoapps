#include "sysio.h"
#include "sealists.h"
#include "storage.h"

lcdg_dm_obj_t *seal_mkcmenu (Boolean ev) {
//
// Create a categories menu (ev -> Event, otherwise my)
//
	word mp;

	// Menu pointer
	ee_read ((lword)(ev ? SEA_EOFF_ECP : SEA_EOFF_MCP), (byte*)(&mp), 2);
	return lcdg_dm_newmenu_e ((lword) (mp + SEA_EOFF_TXT));
}

lcdg_dm_obj_t *seal_mkrmenu () {
//
// Create a nickname-based record menu
// 
	word mp;

	// Menu pointer
	ee_read ((lword) SEA_EOFF_REP, (byte*)(&mp), 2);
	return lcdg_dm_newmenu_e ((lword) (mp + SEA_EOFF_TXT));
}

sea_rec_t *seal_getrec (word rn) {
//
// Get record by its number (allocates the record dynamically)
//
// Note: the argument is not a record offset, but a record number: 0, 1, ...;
//       we do not allocate strings in this versions; there are NO strings,
//	 only text objects, which can be retrieved upon demands based on
//	 offsets stored in the record structure
//
	sea_rec_t *res;

	if ((res = (sea_rec_t*) umalloc (sizeof (sea_rec_t))) == NULL) {
		LCDG_DM_STATUS = LCDG_DMERR_NOMEM;
		return NULL;
	}

	ee_read ((lword)(SEA_EOFF_REC + rn * SEA_RECSIZE), (byte*) res, 
		SEA_RECSIZE);

	return res;
}

word seal_findrec (lword id) {
//
// Find record by ID (returns the record number: 0, 1, ..., or WNONE)
//
	lword ep, dd;
	word ix;

	for (ix = 0, ep = SEA_EOFF_REC; ep < SEA_EOFF_TXT;
	    ix++, ep += SEA_RECSIZE) {
		ee_read (ep, (byte*)(&dd), 4);
		if (dd == id)
			// Found
			return ix;
	}
	return WNONE;
}

void seal_disprec (sea_rec_t *r, Boolean b) {
//
//
//
	lcdg_dm_obj_t * nt;
	address p;
	word i;

	if (r->IM != WNONE) {
		if ((nt = lcdg_dm_newimage_e ((lword)
		    (r->IM + SEA_EOFF_TXT))) != NULL) {
			// The image is there to be displayed
			lcdg_dm_display (nt);
			lcdg_dm_free (nt);
		}
	}

	if (!b)
		return;

	p = &(r->ME);
	for (i = 0; i < 3; i++) {
		// ME, MY, NT
		if (*p != WNONE) {
			// This check is just a precaution
			if ((nt = seal_gettext (*p)) != NULL) {
				lcdg_dm_display (nt);
				lcdg_dm_free (nt);
			}
		}
		p++;
	}
}
