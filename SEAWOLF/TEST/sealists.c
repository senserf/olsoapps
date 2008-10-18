#include "sysio.h"
#include "sealists.h"

static char *getstr (word sptr) {
//
// Allocates and returns a string from the string pool
//
	char *str;
	word len;
	lword ep;

	if ((ep = SEA_EOFF_TXT + sptr) >= SEA_EOFF_ETX)
		// Something wrong
		return NULL;

	// The length
	len = 0;
	ee_read (ep, bytepw (0, len), 1);

	if ((str = (char*) umalloc (len + 1)) == NULL) {
		LCDG_DM_STATUS = LCDG_DMERR_NOMEM;
		return NULL;
	}

	// Technically, we admit zero-length strings
	ee_read (ep + 1, (byte*)str, len);
	str [len] = '\0';
	return str;
}

lcdg_dm_obj_t *seal_mkcmenu (Boolean ev) {
//
// Create a categories menu (ev -> Event, otherwise my)
//
	lword ep;
	char **lines;
	lcdg_dm_obj_t *res;
	word i, wc, nc;

	// Pointer to the category list
	ep = ev ? SEA_EOFF_CAT : SEA_EOFF_CAT + 2 * SEA_NCATS;

	// Determine how many categories are in fact used
	for (nc = 0; nc < SEA_NCATS * 2; nc += 2) {
		ee_read (ep + nc, (byte*)(&wc), 2);
		if (wc == WNONE)
			break;
	}

	if (nc == 0) {
		// No categories
		LCDG_DM_STATUS = LCDG_DMERR_GARBAGE;
		return NULL;
	}

	// Number of entries
	nc >>= 1;

	// Allocate memory for strings
	if ((lines = lcdg_dm_asa (nc)) == NULL)
		return NULL;

	for (i = 0; i < nc; i++, ep += 2) {
		// Get hold of the text
		ee_read (ep, (byte*)(&wc), 2);
		if ((lines [i] = getstr (wc)) == NULL) {
NoMem:
			lcdg_dm_csa (lines, nc);
			return NULL;
		}
	}

	// Everything ready

	if ((res = lcdg_dm_newmenu ((const char**) lines, nc,
						SEA_MENU_CAT_FONT,
						SEA_MENU_CAT_BG,
						SEA_MENU_CAT_FG,
						SEA_MENU_CAT_X,
						SEA_MENU_CAT_Y,
						SEA_MENU_CAT_W,
						SEA_MENU_CAT_H )) == NULL)
		goto NoMem;

	return res;
}

lcdg_dm_obj_t *seal_mkrmenu (word class) {
//
// Make a nickname-based record menu
// 
	lword ep;
	char **lines;
	lcdg_dm_obj_t *res;
	address rp;
	word i, nc, wc [2];

	// Determine the number of entries
	for (nc = 0, ep = SEA_EOFF_REC + SEA_ROFF_CL; ep < SEA_EOFF_TXT;
							ep += SEA_RECSIZE) {
		// This covers "class" + nickname; in addition to record of
		// wrong class, we also ignore those where the nickname is
		// is empty (note that nickname defaults to name). An empty
		// nickname is only legal for an ignore-class record, which
		// is otherwise empty; we do not include such records in
		// the menu
		ee_read (ep, (byte*)(&(wc[0])), 4);
		if (wc [0] != class || wc [1] == WNONE)
			continue;
		nc++;
	}

	if (nc == 0) {
		// No records
		LCDG_DM_STATUS = LCDG_DMERR_GARBAGE;
		return NULL;
	}

	// The Extras array will include record pointers for easy lookup on
	// menu events
	if ((rp = (address) umalloc (sizeof (word) * nc)) == NULL) {
		LCDG_DM_STATUS = LCDG_DMERR_NOMEM;
		return NULL;
	}

	if ((lines = lcdg_dm_asa (nc)) == NULL) {
MRet:
		ufree (rp);
		return NULL;
	}

	for (ep = SEA_EOFF_REC+SEA_ROFF_CL, i = 0; i < nc; ep += SEA_RECSIZE) {
		if (ep >= SEA_EOFF_TXT) {
			// Playing it safe
			lcdg_dm_csa (lines, nc);
			LCDG_DM_STATUS = LCDG_DMERR_GARBAGE;
			goto MRet;
		}
		ee_read (ep, (byte*)(&(wc[0])), 4);
		if (wc [0] != class || wc [1] == WNONE)
			continue;
		if ((lines [i] = getstr (wc [1])) == NULL) {
NoMem:
			lcdg_dm_csa (lines, nc);
			goto MRet;
		}

		rp [i] = (word)(ep - (SEA_EOFF_REC + SEA_ROFF_CL));
		i++;
	}

	if ((res = lcdg_dm_newmenu ((const char**) lines, nc,
						SEA_MENU_REC_FONT,
						SEA_MENU_REC_BG,
						SEA_MENU_REC_FG,
						SEA_MENU_REC_X,
						SEA_MENU_REC_Y,
						SEA_MENU_REC_W,
						SEA_MENU_REC_H )) == NULL)
		goto NoMem;

	res->Extras = rp;
	return res;
}

sea_rec_t *seal_getrec (word rn) {
//
// Get record by its offset (allocates the record dynamically)
//
	word i, rw [4];
	sea_rec_t *res;
	char *s;

#define	IST(a)	((&(res->NI))[a])

	// NI, NM, NT, IM
	ee_read ((lword)(SEA_EOFF_REC + SEA_ROFF_NI) + rn,
					(byte*)(&(rw [0])), sizeof (rw));

	if (rw [0] == WNONE) {
		// No nickname, return NULL
		LCDG_DM_STATUS = LCDG_DMERR_GARBAGE;
		return NULL;
	}

	if ((res = (sea_rec_t*) umalloc (sizeof (sea_rec_t))) == NULL) {
		LCDG_DM_STATUS = LCDG_DMERR_NOMEM;
		return NULL;
	}

	// EC, MC, CL
	ee_read ((lword)(SEA_EOFF_REC + SEA_ROFF_EC) + rn, (byte*) res, 6);

	// IM
	res->IM = rw [3];

	// Initialize in case we have to deallocate
	res->NI = res->NM = res->NT = NULL;

	for (i = 0; i < 3; i++) {
		if (rw [i] != WNONE) {
			if ((s = getstr (rw [i])) == NULL) {
				// No memory, deallocate what you have and
				// return NULL
				seal_freerec (res);
				ufree (res);
				return NULL;
			}
		} else {
			s = NULL;
		}
		IST (i) = s;
	}

	return res;
}

void seal_freerec (sea_rec_t *rec) {
//
// Release memory allocated to the record
//
	if (rec->NI)
		ufree (rec->NI);

	if (rec->NM)
		ufree (rec->NM);

	if (rec->NT)
		ufree (rec->NT);

	ufree (rec);
}

word seal_findrec (lword id) {
//
// Find record by ID
//
	lword ep, dd;

	for (ep = SEA_EOFF_REC; ep < SEA_EOFF_TXT; ep += SEA_RECSIZE) {
		ee_read (ep, (byte*)(&dd), 4);
		if (dd == id)
			// Found
			return (word)(ep - SEA_EOFF_REC);
	}

	return WNONE;
}

byte seal_meter (word f0, word f1) {
//
// Draw a meter
//
// .||__||__.....__||____||__||__.....__||__||.
//       16 x 4 + 16 x 4 + 2 = 130 pixels
//
//
	char line [17];
	word i;

	if (lcdg_font (SEA_METER_FONT)) {
ERet:
		return (LCDG_DM_STATUS = LCDG_DMERR_GARBAGE);
	}

	line [17] = '\0';

	lcdg_setc (SEA_METER_BG0, SEA_METER_FG0);

	if (lcdg_sett (0, SEA_METER_Y, 16, 1))
		goto ERet;

	for (i = 0; i < 16; i++)
		line [i] = ((f0 >> i) & 1) ? 'a' : 'b';

	lcdg_wl (line, 0, 0, 0);

	lcdg_setc (SEA_METER_BG1, SEA_METER_FG1);

	if (lcdg_sett (66, SEA_METER_Y, 16, 1))
		goto ERet;

	for (i = 0; i < 16; i++)
		line [i] = ((f1 >> i) & 1) ? 'a' : 'b';

	lcdg_wl (line, 0, 0, 0);
	return 0;
}
