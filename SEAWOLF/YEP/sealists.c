#include "sysio.h"
#include "sealists.h"
#include "applib.h"

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

	if (ev) {
	    if ((res = lcdg_dm_newmenu (lines, nc,
					SEA_MENU_CATEV_FONT,
					SEA_MENU_CATEV_BG,
					SEA_MENU_CATEV_FG,
					SEA_MENU_CATEV_X,
					SEA_MENU_CATEV_Y,
					SEA_MENU_CATEV_W,
					SEA_MENU_CATEV_H )) == NULL)
		goto NoMem;
	} else {
	    if ((res = lcdg_dm_newmenu (lines, nc,
					    SEA_MENU_CATMY_FONT,
					    SEA_MENU_CATMY_BG,
					    SEA_MENU_CATMY_FG,
					    SEA_MENU_CATMY_X,
					    SEA_MENU_CATMY_Y,
					    SEA_MENU_CATMY_W,
					    SEA_MENU_CATMY_H )) == NULL)
		goto NoMem;
	}

	return res;
}

lcdg_dm_obj_t *seal_mkrmenu () {
//
// Make a nickname-based record menu
// 
	lword ep;
	char **lines;
	lcdg_dm_obj_t *res;
	address rp;
	word i, nc, wc [SEA_ROFF_NM >> 1]; // up to Name, in words (6 now)

	// Determine the number of entries
	/* if we go for one list with +/- attrib for 'yes' and 'no' people,
	   we should add the number of entries to the data from noombuzz
	*/
	for (nc = 0, ep = SEA_EOFF_REC + SEA_ROFF_CL; ep < SEA_EOFF_TXT;
							ep += SEA_RECSIZE) {
		ee_read (ep, (byte*)(&(wc[0])), 4);
		if (wc [0] != 0 && wc [1] != WNONE)
			nc++;
	}

	if (nc == 0) {
		// No records
		LCDG_DM_STATUS = LCDG_DMERR_GARBAGE;
		return NULL;
	}

	ufree (nbh_menu.mm); // harmless if NULL?

	// The Extras array will include record pointers for easy lookup on
	// menu events
	if ((rp = (address) umalloc (sizeof (word) * nc)) == NULL ||
	    (nbh_menu.mm = (nbh_t *) umalloc (sizeof (nbh_t) * nc)) == NULL) {
		LCDG_DM_STATUS = LCDG_DMERR_NOMEM;
		return NULL;
	}

	if ((lines = lcdg_dm_asa (nc)) == NULL) {
MRet:
		ufree (rp);
		ufree (nbh_menu.mm);
		return NULL;
	}
	nbh_menu.li = nc;

	for (ep = SEA_EOFF_REC, i = 0; i < nc; ep += SEA_RECSIZE) {
		if (ep >= SEA_EOFF_TXT) {
			// Playing it safe
			lcdg_dm_csa (lines, nc);
			LCDG_DM_STATUS = LCDG_DMERR_GARBAGE;
			goto MRet;
		}

		ee_read (ep, (byte*)(&(wc[0])), SEA_ROFF_NM);

		if (wc [SEA_ROFF_CL >>1] == 0 ||   // ignore was a mistake
		    wc [SEA_ROFF_NI >>1] == WNONE) // skip empty nicknames
			continue;

		if ((lines [i] = getstr (wc [SEA_ROFF_NI >>1])) == NULL) {
NoMem:
			lcdg_dm_csa (lines, nc);
			goto MRet;
		}

		rp [i] = (word)(ep - SEA_EOFF_REC);
		nbh_menu.mm[i].id = wc [SEA_ROFF_ID >> 1];
		nbh_menu.mm[i].ts = 0;
		nbh_menu.mm[i].st = HS_SILE;
		if (wc [SEA_ROFF_CL >>1] == 1) {
			lines [i][1] = MLI1_YE;
			nbh_menu.mm[i].gr = GR_YE;
		} else {
			lines [i][1] = MLI1_NO;
			nbh_menu.mm[i].gr = GR_NO;
		}
		i++;
	}

	if ((res = lcdg_dm_newmenu (lines, nc,
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

	// NID, EC, MC, CL
	ee_read ((lword)(SEA_EOFF_REC + SEA_ROFF_ID) + rn, (byte*) res, 10);

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

byte seal_disprec () {

// use global curr_rec

//
// Draw a meter
//
// .||__||__.....__||____||__||__.....__||__||.
//       16 x 4 + 16 x 4 + 2 = 130 pixels
//
//
	char line [17];
	word i, j;
	lcdg_dm_obj_t * nt;

	// name, note
	if (lcdg_font (0))
		goto ERet;
	lcdg_setc (COLOR_BLACK, COLOR_WHITE);

#if 0
	if (rec->NM != NULL && (i = strlen(rec->NM)) > 0) {
		if (i > 21)
			i = 21; // 130 / 6
		if (lcdg_sett (0, SEA_METER_Y - 16, i, 1))
			goto ERet;
		lcdg_wl (rec->NM, 0, 0, 0);
	}
#endif

	// 21 = 130 / 6 (when we know what we're doing, use
	// lcdg_cwidth, _cheight
	if (curr_rec->NT != NULL && (i = strlen(curr_rec->NT)) > 0) {
		i =  i / 21 + 1;
		nt = lcdg_dm_newtext (curr_rec->NT, 0, COLOR_BLACK, COLOR_WHITE,
				0, SEA_METER_Y - 8 * i, 21);

		lcdg_dm_display (nt);
		ufree (nt);
#if 0
		if (lcdg_sett (0, SEA_METER_Y - 8 * i, 21, i))
			goto ERet;
		lcdg_wl (rec->NT, 0, 0, 0);
#endif
	} else {
		i = 0;
	}

	if (curr_rec->NM != NULL && (j = strlen(curr_rec->NM)) > 0) {
		if (lcdg_sett (0, SEA_METER_Y - 8 * i -8, j < 21 ? j : 21, 1))
			goto ERet;
		lcdg_wl (curr_rec->NM, 0, 0, 0);
	}

	if (lcdg_font (SEA_METER_FONT)) {
ERet:
		return (LCDG_DM_STATUS = LCDG_DMERR_GARBAGE);
	}

	line [17] = '\0';

	lcdg_setc (SEA_METER_BG0, SEA_METER_FG0);

	if (lcdg_sett (0, SEA_METER_Y, 16, 1))
		goto ERet;

	for (i = 0; i < 16; i++)
		line [i] = ((curr_rec->ECats >> i) & 1) ? 'a' : 'b';

	lcdg_wl (line, 0, 0, 0);

	lcdg_setc (SEA_METER_BG1, SEA_METER_FG1);

	if (lcdg_sett (66, SEA_METER_Y, 16, 1))
		goto ERet;

	for (i = 0; i < 16; i++)
		line [i] = ((curr_rec->MCats >> i) & 1) ? 'a' : 'b';

	lcdg_wl (line, 0, 0, 0);
	return 0;
}
